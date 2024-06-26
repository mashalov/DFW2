﻿#include "stdafx.h"
#include <immintrin.h>
#include <algorithm>
#include "LoadFlow.h"
#include "DynaModel.h"
#include "limits"
#include "BranchMeasures.h"
#include "MathUtils.h"

using namespace DFW2;

CLoadFlow::CLoadFlow(CDynaModel* pDynaModel) : pDynaModel(pDynaModel), Parameters(pDynaModel->Parameters()) {}

bool CLoadFlow::NodeInMatrix(const CDynaNodeBase* pNode)
{
	// если тип узла не базисный и узел включен - узел должен войти в матрицу Якоби
	return (!pNode->IsLFTypeSlack()) && pNode->IsStateOn();
}

bool CDynaModel::LoadFlow()
{
	CLoadFlow LoadFlow(this);
	return LoadFlow.Run();
}

void CLoadFlow::AllocateSupernodes()
{
	MatrixSize_ = NonZeroCount_ = 0;
	// создаем привязку узлов к информации по строкам матрицы
	// размер берем по количеству суперузлов. Реальный размер матрицы будет меньше на
	// количество отключенных узлов и БУ. Также он корректируется на количество требуемых 
	// искусственных узлов

	pMatrixInfo_ = std::make_unique<_MatrixInfo[]>(pNodes->Count() + ExtraNodes_->Count());
	_MatrixInfo* pMatrixInfo{ pMatrixInfo_.get() };

	// базисные узлы держим в отдельном списке, так как они не в матрице, но
	// результаты по ним нужно обновлять
	std::list<CDynaNodeBase*> SlackBuses;

	for (auto&& it : pNodes->DevVec)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(it) };
		// обрабатываем только включенные узлы и узлы без родительского суперузла
		if (!pNode->IsStateOn() || pNode->pSuperNodeParent)
			continue;
		// обновляем VreVim узла
		pNode->UpdateVreVim();
		// добавляем БУ в список БУ для дальнейшей обработки
		if (pNode->IsLFTypeSlack())
			SlackBuses.push_back(pNode);

		// для узлов которые в матрице считаем количество ветвей
		// и ненулевых элементов
		if (NodeInMatrix(pNode))
		{
			pNode->SetMatrixRow(MatrixSize_);
			// для узлов, которые попадают в матрицу нумеруем включенные узлы строками матрицы
			pMatrixInfo->Store(pNode);

			for (const auto& pv : pNode->VirtualBranches())
				if(NodeInMatrix(pv.pNode))
					pMatrixInfo->nRowCount += 2;	// количество ненулевых элементов = ветвей - ветвей на БУ

			MatrixSize_ += 2;				// на узел 2 уравнения в матрице
			pMatrixInfo->nRowCount += 2;	// считаем диагональный элемент
			NonZeroCount_ += 2 * pMatrixInfo->nRowCount;	// количество ненулевых элементов увеличиваем на количество подсчитанных элементов в строке (4 double на элемент)
			pMatrixInfo++;
		}
	}

	// корректируем размерности с учетом искусственных узлов
	pMatrixInfo = ExtraNodes_->UpdateDimensions(pMatrixInfo);

	if (!MatrixSize_)
		throw dfw2error(CDFW2Messages::m_cszNoNodesForLF);
	
	// конец инфо по матрице для узлов, которые в матрице
	pMatrixInfoEnd = pMatrixInfo;							

	// базисные узлы добавляем "под" - матрицу. Они в нее не входят
	// но должны быть под рукой в общем векторе узлов в расчете
	for (auto&& sit : SlackBuses)
	{
		const auto& pNode{ sit };
		// обнуляем ограничения БУ, чтобы
		// они не влияли на суммарные ограничения
		// суперузла, который может быть образован
		// от данного БУ
		pNode->LFQmin = pNode->LFQmax = 0.0;
		pMatrixInfo->Store(pNode);
		// для узлов в базисном суперузле один
		// раз на старте задаем напряжение
		pNode->UpdateVreVimSuper();
		pMatrixInfo++;
	}
		
	klu.SetSize(MatrixSize_, NonZeroCount_);
	pMatrixInfoSlackEnd = pMatrixInfo;
	pRh = std::make_unique<double[]>(klu.MatrixSize());		// невязки до итерации
}

void CLoadFlow::Estimate()
{
	ptrdiff_t* pAi = klu.Ai();
	ptrdiff_t* pAp = klu.Ap();
	ptrdiff_t nRowPtr = 0;

	_MatrixInfo* pMatrixInfo{ pMatrixInfo_.get() };
	for (; pMatrixInfo < pMatrixInfoEnd; pMatrixInfo++)
	{
		const auto& pNode{ pMatrixInfo->pNode };
		// формируем указатели строк матрицы по две на узел
		*pAi = nRowPtr;	pAi++;
		nRowPtr += pMatrixInfo->nRowCount;
		*pAi = nRowPtr;	pAi++;
		nRowPtr += pMatrixInfo->nRowCount;

		// формируем номера столбцов в двух строках уравнений узла
		*pAp = pNode->A(0);
		*(pAp + pMatrixInfo->nRowCount) = pNode->A(0);
		pAp++;
		*pAp = pNode->A(1);
		*(pAp + pMatrixInfo->nRowCount) = pNode->A(1);
		pAp++;

		for (const auto& pv : pMatrixInfo->VirtualBranches())
		{
			const CDynaNodeBase* pOppNode{ pv.pNode };

			if (NodeInMatrix(pOppNode))
			{
				// если оппозитный узел в матрице формируем номера столбцов для него
				*pAp = pOppNode->A(0);
				*(pAp + pMatrixInfo->nRowCount) = pOppNode->A(0);
				pAp++;
				*pAp = pOppNode->A(1);
				*(pAp + pMatrixInfo->nRowCount) = pOppNode->A(1);
				pAp++;
			}
		}
		pAp += pMatrixInfo->nRowCount;
	}

	*pAi = klu.NonZeroCount();

	// формируем вектор ветвей для контроля взаимного угла
	// учитываем включенные ветви с сопротивлением выше минимального
	// вектор таких ветвей построить проще по исходным ветвям, чем собирать его из виртуальных ветвей узлов
	CDeviceContainer* pBranchContainer{ pDynaModel->GetDeviceContainer(DEVTYPE_BRANCH) };
	BranchAngleCheck.reserve(pBranchContainer->Count());
	for (auto&& it : *pBranchContainer)
	{
		const auto& pBranch{ static_cast<CDynaBranch*>(it) };
		if (pBranch->BranchState_ == CDynaBranch::BranchState::BRANCH_ON && !pBranch->InSuperNode())
			BranchAngleCheck.push_back(pBranch);
	}
}

void CDynaNodeBase::StartLF(bool bFlatStart, double ImbTol)
{
	// для всех включенных и небазисных узлов
	if (IsStateOn())
	{
		if (!IsLFTypeSlack())
		{
			// если у узла заданы пределы по реактивной мощности и хотя бы один из них ненулевой + задано напряжение
			if (LFQmax > LFQmin && (std::abs(LFQmax) > ImbTol || std::abs(LFQmin) > ImbTol) && LFVref > 0.0)
			{
				// узел является PV-узлом
				eLFNodeType_ = CDynaNodeBase::eLFNodeType::LFNT_PV;
				if (bFlatStart)
				{
					// если требуется плоский старт
					V = LFVref;										// напряжение задаем равным Vref
					Qgr = LFQmin + (LFQmax - LFQmin) / 2.0;			// реактивную мощность ставим в середину диапазона
					Delta = pSyncZone->Mj;							// угол берем из синхронной зоны (средний угол базисных узлов)
				}
				else if (V > LFVref && Qgr >= LFQmin + ImbTol)
				{
					V = LFVref;
				}
				else if (V < LFVref && Qgr <= LFQmax - ImbTol)
				{
					V = LFVref;
				}
				else if (Qgr >= LFQmax - ImbTol)
				{
					// для неплоского старта приводим реактивную мощность в ограничения
					// и определяем тип ограничения узла
					Qgr = LFQmax;
					eLFNodeType_ = CDynaNodeBase::eLFNodeType::LFNT_PVQMAX;
				}
				else if (LFQmin + ImbTol >= Qgr)
				{
					Qgr = LFQmin;
					eLFNodeType_ = CDynaNodeBase::eLFNodeType::LFNT_PVQMIN;
				}
				else
					V = LFVref; // если реактивная мощность в диапазоне - напряжение равно уставке
			}
			else
			{
				// для PQ-узлов на плоском старте ставим напряжение равным номинальному
				eLFNodeType_ = CDynaNodeBase::eLFNodeType::LFNT_PQ;
				if (bFlatStart)
				{
					V = Unom;
					Delta = 0.0;
				}
			}
		}
		else
		{
			// у базисного узла сбрасываем мощность в ноль 
			Pgr = Qgr = 0.0;
			// а также обнуляем Qmin и Qmax, чтобы корректно распределить Qg
			// по обычным узлам внутри базисного суперузла в CLoadFlow::UpdateSupernodesPQ()
			// !!!!! нет !!!!! мы не обнуляем ограничения суперузла, образованного БУ !
			// мы используем рассчитанные ограничения такого узла чтобы определить  какую долю
			// реактивной мощности положить в диапазон генераторных узлов, а излишек - в БУ
			// здесь остается суммарный диапазон генераторных узлов
			// заданные в исходных данных ненулевые ограничения БУ
			// сбрасываются в ноль ранее, поэтому эти ограничения здесь
			// не учитываются 
			// LFQmax = LFQmin = 0.0;

		}
	}
	else
	{
		// у отключенных узлов обнуляем напряжение
		V = Delta = 0.0;
	}
	// используем инициализацию узла для расчета УР
	UpdateVreVim();
}

void CLoadFlow::Start()
{
	// отключаем висячие ветви и узлы, проверяем острова на наличие базисных узлов
	pNodes->PrepareLFTopology();
	// создаем суперузлы. Важно - базисные суперузлы имеют узлом представителем один из базисных узлов
	// здесь для CreateShuntParts нужны V0 и СХН для УР
	pNodes->CreateSuperNodesStructure();
	// Рассчитываем проводимости узлов с устранением отрицательных сопротивлений, если
	// используется метод Зейделя или без устранения в противном случае
	pNodes->CalculateSuperNodesAdmittances(Parameters.Startup == CLoadFlow::eLoadFlowStartupMethod::Seidell);

	// в режиме отладки запоминаем что было в узлах после расчета Rastr для сравнения результатов
#ifdef _DEBUG
	for (auto&& it : *pNodes)
		static_cast<CDynaNodeBase*>(it)->GrabRastrResult();
#endif

	// создаем внешний контроллер для искусственных узлов
	ExtraNodes_ = std::make_unique<CExtraNodes>(*this);
	// собираем данные по компенсаторам, требующим ввода дополнительных узлов
	CollectSVCData();

	// обновляем данные в PV-узлах по заданным в генераторах реактивным мощностям
	UpdatePQFromGenerators();

	// инициализируем все узлы, чтобы корректно определить их тип
	for (auto&& it : *pNodes)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(it) };
		pNode->Pgr = pNode->Pg;
		pNode->Qgr = pNode->Qg;
		pNode->StartLF(Parameters.Flat, Parameters.Imb);
	}

	// выбираем узлы, которые войдут в матрицу (все суперузлы), а так же базисные суперузлы.
	AllocateSupernodes();
	// формируем систему дополнительных ветвей к искусственным узлам
	ExtraNodes_->UpdateVirtualBranches();

	for (_MatrixInfo* pMatrixInfo = pMatrixInfo_.get(); pMatrixInfo < pMatrixInfoSlackEnd; pMatrixInfo++)
	{
		const auto& pNode{ pMatrixInfo->pNode };
		double QrangeMax{ pNode->LFQmax - pNode->LFQmin };

		// собираем информацию для суперузла только для узлов
		// которые были в исходном контейнере. Для искусственных узлов
		// пропускаем обработку суперузла

		const CLinkPtrCount* const pNodeLink{ pNode->GetSuperLink(0) };
		LinkWalker<CDynaNodeBase> pSlaveNode;

		while (pNodeLink->In(pSlaveNode))
		{
			// в суперузел суммируем все мощности входящих узлов
			pNode->Pgr += pSlaveNode->Pgr;
			pNode->Qgr += pSlaveNode->Qgr;

			switch (pSlaveNode->eLFNodeType_)
			{
			case CDynaNodeBase::eLFNodeType::LFNT_BASE:
				// если в суперузле базисный узел - узел представитель тоже должен быть
				// базисным, потому что так должны строиться суперузлы в CreateSuperNodes
				_ASSERTE(pNode->eLFNodeType_ == pSlaveNode->eLFNodeType_);
				break;
			case CDynaNodeBase::eLFNodeType::LFNT_PQ:
				// если в суперузел входит нагрузочный - учитываем его генерацию как неуправляемую
				pMatrixInfo->UncontrolledP += pSlaveNode->Pgr;
				pMatrixInfo->UncontrolledQ += pSlaveNode->Qgr;
				break;
			default:
				// генераторный узел не может входить в PQ-суперузел
				_ASSERTE(pNode->eLFNodeType_ != CDynaNodeBase::eLFNodeType::LFNT_PQ);
				// если в суперузел входит генераторный - учитываем его активную генерацию как неуправляемую
				pMatrixInfo->UncontrolledP += pSlaveNode->Pgr;
				// считаем диапазон реактивной мощности суперзула
				pNode->LFQmin += pSlaveNode->LFQmin;
				pNode->LFQmax += pSlaveNode->LFQmax;
				if (pNode->IsLFTypeSlack())
				{
					// если генераторный узел входит в базисный суперузел
					// уставка по напряжению равна напряжению базисного узла
					pSlaveNode->LFVref = pNode->LFVref;
				}
				else
				{
					const double Qrange{ pSlaveNode->LFQmax - pSlaveNode->LFQmin };
					// выбираем заданное напряжение по узлу с максимальным диапазоном Qmin/Qmax
					if (QrangeMax < Qrange)
					{
						pNode->LFVref = pSlaveNode->LFVref;
						QrangeMax = Qrange;
					}
				}
			}
		}
		// инициализируем суперузел
		pNode->StartLF(Parameters.Flat, Parameters.Imb);
		// корректируем неуправляемую генерацию

		pNode->DebugLog(fmt::format("{} G[{},{}] L[{},{}] Qlim[{},{}] Vref {} Uncontrolled[{},{}]",
			pNode->GetId(),
			pNode->Pgr, pNode->Qgr,
			pNode->Pnr, pNode->Qnr,
			pNode->LFQmin, pNode->LFQmax,
			pNode->LFVref,
			pMatrixInfo->UncontrolledP, pMatrixInfo->UncontrolledQ
		));

		for (const auto& pv : pMatrixInfo->VirtualBranches())
		{
			const auto& pOppNode{ pv.pNode };
			pNode->DebugLog(fmt::format("{} y={}", pv.pNode->GetId(), pv.Y));
		}

		switch (pNode->eLFNodeType_)
		{
		case CDynaNodeBase::eLFNodeType::LFNT_BASE:
			// в базисном учитываем и активную и реактивную
			break;
		case CDynaNodeBase::eLFNodeType::LFNT_PQ:
			// в нагрузочном узле нет неуправляемой генерации
			pMatrixInfo->UncontrolledP = pMatrixInfo->UncontrolledQ = 0.0;
			break;
		default:
			// в PV, PQmin, PQmax узлах учитываем реактивную неуправляемую генерацию
			pMatrixInfo->UncontrolledP = 0.0;
		}
	}
}

// функция сортировки PV-узлов для определения порядка их обработки в Зейделе
bool CLoadFlow::SortPV(const _MatrixInfo* lhs, const _MatrixInfo* rhs)
{
	_ASSERTE(!lhs->pNode->IsLFTypePQ());
	_ASSERTE(!lhs->pNode->IsLFTypePQ());
	// сортируем по убыванию диапазона
	const double diffQ = (lhs->pNode->LFQmax - lhs->pNode->LFQmin) - (rhs->pNode->LFQmax - rhs->pNode->LFQmin);
	if (diffQ == 0.0)
		return lhs->pNode->GetId() > rhs->pNode->GetId();
	return diffQ > 0.0;
}

void CLoadFlow::BuildSeidellOrder2(MATRIXINFO& SeidellOrder)
{
	const ptrdiff_t NodeCount{ pMatrixInfoSlackEnd - pMatrixInfo_.get() };
	SeidellOrder.reserve(NodeCount);

	QUEUE queue;

	_MatrixInfo* pMatrixInfo;

	// сбрасываем признаки посещения
	for (pMatrixInfo = pMatrixInfo_.get(); pMatrixInfo <= pMatrixInfoEnd; pMatrixInfo++)
		pMatrixInfo->bVisited = false;

	// в начало добавляем БУ
	for (pMatrixInfo = pMatrixInfoSlackEnd - 1; pMatrixInfo >= pMatrixInfoEnd; pMatrixInfo--)
	{
		pMatrixInfo->bVisited = true;
		queue.push_back(pMatrixInfo);
		SeidellOrder.push_back(pMatrixInfo);
	}

	std::vector<const VirtualBranch*> branches;
	branches.reserve(NodeCount);

	const auto OppMatrixInfo = [this](const CDynaNodeBase* pNode)
	{
		return &pMatrixInfo_[pNode->A(0) / 2];
	};

	const auto SortOrder = [](const VirtualBranch* b1, const VirtualBranch* b2)
	{
		const auto& node1{ b1->pNode }, node2{ b2->pNode };

		_ASSERTE(!node1->IsLFTypeSlack());
		_ASSERTE(!node2->IsLFTypeSlack());

		ptrdiff_t n1Weight{ node1->IsLFTypePV() ? 1 : 0 }, n2Weight{ node2->IsLFTypePV() ? 1: 0 };

		n1Weight -= n2Weight;
		if(n1Weight != 0)
			return n1Weight > 0;

		_ASSERTE(node1->eLFNodeType_ == node2->eLFNodeType_);

		// узлы имеют одинаковые типы
		if (n2Weight)
		{
			// оба PV
			const double rangeDiff{ node1->LFQmax - node2->LFQmax - node1->LFQmin + node2->LFQmin };
			if (rangeDiff != 0.0)
				return rangeDiff > 0.0;
		}

		const double ydiff{ std::abs(b1->Y) - std::abs(b2->Y) };

		if (ydiff != 0.0)
			return ydiff > 0.0;

		return node1->GetId() > node2->GetId();
	};

	while (!queue.empty())
	{
		branches.clear();

		pMatrixInfo = queue.front();
		queue.pop_front();

		for (const auto& pv : pMatrixInfo->VirtualBranches())
		{
			if (pv.pNode->IsLFTypeSlack() || OppMatrixInfo(pv.pNode)->bVisited)
				continue;
			branches.push_back(&pv);
		}

		std::sort(branches.begin(), branches.end(), SortOrder);

		pMatrixInfo->pNode->DebugLog(fmt::format("{}----", pMatrixInfo->pNode->GetId()));
		for (auto&& branch : branches)
		{
			pMatrixInfo->pNode->DebugLog(fmt::format("{} Vref={}, Qlim=[{};{}] Y={}",
				branch->pNode->GetId(),
				branch->pNode->LFVref,
				branch->pNode->LFQmax, branch->pNode->LFQmin,
				std::abs(branch->Y)));

			auto matrixInfo{ OppMatrixInfo(branch->pNode) };
			matrixInfo->bVisited = true;
			queue.push_back(matrixInfo);
			SeidellOrder.push_back(OppMatrixInfo(branch->pNode));
		}
	}
}

void CLoadFlow::BuildSeidellOrder(MATRIXINFO& SeidellOrder)
{
	SeidellOrder.reserve(pMatrixInfoSlackEnd - pMatrixInfo_.get());

	// добавляет узел в очередь для Зейделя
	const auto AddToQueue = [this](_MatrixInfo* pMatrixInfo, QUEUE& queue)
	{
		// просматриваем список ветвей узла
		for (const auto& pv : pMatrixInfo->VirtualBranches())
		{
			const auto& pOppNode{ pv.pNode };
			// мы обходим узлы, но кроме данных узлов нам нужны данные матрицы, чтобы просматривать
			// признак посещения
			if (pOppNode->IsLFTypePQ() && (!pOppNode->IsLFTypeSlack()))
			{
				_MatrixInfo* pOppMatrixInfo = &pMatrixInfo_[pOppNode->A(0) / 2]; // находим оппозитный узел в матрице
				_ASSERTE(pOppMatrixInfo->pNode == pOppNode);
				// если оппозитный узел еще не был посещен, добавляем его в очередь и помечаем как посещенный
				if (!pOppMatrixInfo->bVisited)
				{
					queue.push_back(pOppMatrixInfo);
					pOppMatrixInfo->bVisited = true;
				}
			}
		}
	};

	_MatrixInfo* pMatrixInfo;

	for (pMatrixInfo = pMatrixInfo_.get(); pMatrixInfo <= pMatrixInfoEnd; pMatrixInfo++)
		pMatrixInfo->bVisited = false;

	// в начало добавляем БУ
	for (pMatrixInfo = pMatrixInfoSlackEnd - 1; pMatrixInfo >= pMatrixInfoEnd; pMatrixInfo--)
	{
		SeidellOrder.push_back(pMatrixInfo);
		pMatrixInfo->bVisited = true;
	}

	// затем PV узлы
	for (; pMatrixInfo >= pMatrixInfo_.get(); pMatrixInfo--)
	{
		if (!pMatrixInfo->pNode->IsLFTypePQ())
		{
			SeidellOrder.push_back(pMatrixInfo);
			pMatrixInfo->bVisited = true;
		}
	}

	// сортируем PV узлы по убыванию диапазона реактивной мощности
	std::sort(SeidellOrder.begin() + (pMatrixInfoSlackEnd - pMatrixInfoEnd), SeidellOrder.end(), SortPV);

	// добавляем узлы в порядок обработки Зейделем с помощью очереди
	// очередь строим от базисных и PV-узлов по связям. Порядок очереди 
	// определяем по мере удаления от узлов базисных и PV-узлов 
	QUEUE queue;
	for (auto&& it : SeidellOrder)
		AddToQueue(it, queue);


	// пока в очереди есть узлы
	while (!queue.empty())
	{
		// достаем узел из очереди
		pMatrixInfo = queue.front();
		queue.pop_front();
		// добавляем узел в очередь Зейделя
		SeidellOrder.push_back(pMatrixInfo);
		// и добавляем оппозитные узлы добавленного узла
		AddToQueue(pMatrixInfo, queue);
	}
}

void CLoadFlow::Seidell()
{
	pDynaModel->Log(DFW2MessageStatus::DFW2LOG_INFO, CDFW2Messages::m_cszLFRunningSeidell);

	MATRIXINFO SeidellOrder;
	BuildSeidellOrder(SeidellOrder);

	_ASSERTE(SeidellOrder.size() == pMatrixInfoSlackEnd - pMatrixInfo_.get());

	double dPreviousImb{ -1.0 }, dPreviousImbQ{ -1.0 };

	size_t& SeidellIterations{ pNodes->IterationControl().Number };

	for (SeidellIterations = 0; SeidellIterations < Parameters.SeidellIterations; SeidellIterations++)
	{
		// множитель для ускорения Зейделя
		double dStep{ Parameters.SeidellStep };

		if (SeidellIterations > 2)
		{
			dPreviousImbQ = 0.5 * std::abs(pNodes->IterationControl().MaxImbQ.GetDiff());

			// если сделали более 2-х итераций начинаем анализировать небалансы
			if (dPreviousImb < 0.0)
			{
				// первый небаланс, если еще не рассчитывали
				dPreviousImb = ImbNorm(pNodes->IterationControl().MaxImbP.GetDiff(), dPreviousImbQ);
			}
			else
			{
				// если есть предыдущий небаланс, рассчитываем отношение текущего и предыдущего
				const double dCurrentImb{ ImbNorm(pNodes->IterationControl().MaxImbP.GetDiff(), pNodes->IterationControl().MaxImbQ.GetDiff()) };
				const double dImbRatio{ dCurrentImb / dPreviousImb };
				/*
				dPreviousImb = dCurrentImb;
				if (dImbRatio < 1.0)
					dStep = 1.0;
				else
				{
					dStep = 1.0 + exp((1 - dImbRatio) * 10.0);
				}
				*/
			}
		}

		// сбрасываем статистику итерации
		pNodes->IterationControl().Reset();
		// определяем можно ли выполнять переключение типов узлов (по количеству итераций)
		bool bPVPQSwitchEnabled{ SeidellIterations >= Parameters.EnableSwitchIteration };
		// для всех узлов в порядке обработки Зейделя

		for (auto&& it : SeidellOrder)
		{
			_MatrixInfo* pMatrixInfo{ it };
			const auto& pNode{ pMatrixInfo->pNode };
			// рассчитываем нагрузку по СХН
			double& Pe{ pMatrixInfo->ImbP }, &Qe{ pMatrixInfo->ImbQ };
			// рассчитываем небалансы
			GetNodeImb(pMatrixInfo);
			double Q{ Qe + pNode->Qgr };	// расчетная генерация в узле

			cplx I1{ dStep / std::conj(pNode->VreVim) / pNode->YiiSuper };

			switch (pNode->eLFNodeType_)
			{
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMAX:
				pNodes->IterationControl().Update(pMatrixInfo);

				// если узел на верхнем пределе и напряжение больше заданного
				if (it->NodeVoltageViolation() > Parameters.Vdifference)
				{
					if (Q < pNode->LFQmin - Parameters.Imb)
					{
						pNode->Qgr = Q;
						LogNodeSwitch(it, "PQmax->PQmin");
						pNode->Qgr = NodeLFQmin(pNode).Q;
						pNode->eLFNodeType_ = CDynaNodeBase::eLFNodeType::LFNT_PVQMIN;
						pNodes->IterationControl().QviolatedCount++;
						Qe = Q - pNode->Qgr;
						const cplx dU{ I1 * cplx(Pe, -Qe) };
						pNode->Vre += dU.real();
						pNode->Vim += dU.imag();
					}
					else
					{
						// снимаем узел с ограничения и делаем его PV
						pNode->Qgr = Q;
						LogNodeSwitch(it, "PQmax->PV");
						pNode->eLFNodeType_ = CDynaNodeBase::eLFNodeType::LFNT_PV;
						pMatrixInfo->PVSwitchCount++;
						pNodes->IterationControl().QviolatedCount++;
						pNode->Qgr = Q;
						cplx dU{ I1 * cplx(Pe, 0) };
						dU += pNode->VreVim;
						dU = pNode->LFVref * dU / std::abs(dU);
						pNode->Vre = dU.real();
						pNode->Vim = dU.imag();
					}
				}
				else
				{
					// если напряжение не выше заданного - вводим ограничение реактивной мощности
					pNode->Qgr = NodeLFQmax(pNode).Q;
					const cplx dU{ I1 * cplx(Pe, -Qe) };
					pNode->Vre += dU.real();
					pNode->Vim += dU.imag();
				}
				break;
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMIN:
				pNodes->IterationControl().Update(pMatrixInfo);

				// если узел на нижнем пределе и напряжение меньше заданного
				if (it->NodeVoltageViolation() < - Parameters.Vdifference)
				{
					if (Q > pNode->LFQmax + Parameters.Imb)
					{
						pNode->Qgr = Q;
						LogNodeSwitch(it, "PQmin->PQmax");
						pNode->eLFNodeType_ = CDynaNodeBase::eLFNodeType::LFNT_PVQMAX;
						pNodes->IterationControl().QviolatedCount++;
						pNode->Qgr = NodeLFQmax(pNode).Q;
						Qe = Q - pNode->Qgr;
						const cplx dU{ I1 * cplx(Pe, -Qe) };
						pNode->Vre += dU.real();
						pNode->Vim += dU.imag();
					}
					else
					{
						// снимаем узел с ограничения
						pNode->Qgr = Q;
						LogNodeSwitch(it, "PQmin->PV");
						pNode->eLFNodeType_ = CDynaNodeBase::eLFNodeType::LFNT_PV;
						pNodes->IterationControl().QviolatedCount++;
						pMatrixInfo->PVSwitchCount++;
						pNode->Qgr = Q;
						cplx dU{ I1 * cplx(Pe, 0) };
						dU += pNode->VreVim;
						dU = pNode->LFVref * dU / std::abs(dU);
						pNode->Vre = dU.real();
						pNode->Vim = dU.imag();
					}
				}
				else
				{
					// если напряжение не меньше заданного - вводим ограничение реактивной мощности
					pNode->Qgr = NodeLFQmin(pNode).Q;
					const cplx dU{ I1 * cplx(Pe, -Qe) };
					pNode->Vre += dU.real();
					pNode->Vim += dU.imag();
				}
				break;
			case CDynaNodeBase::eLFNodeType::LFNT_PV:
			{
				pNodes->IterationControl().Update(pMatrixInfo);
				if (bPVPQSwitchEnabled)
				{
					// PV-узлы переключаем если есть разрешение (на первых итерациях переключение блокируется, можно блокировать по небалансу)
					if (Q > pNode->LFQmax + Parameters.Imb + dPreviousImbQ)
					{
						pNode->Qgr = Q;
						LogNodeSwitch(it, "PV->PQmax");
						pNode->eLFNodeType_ = CDynaNodeBase::eLFNodeType::LFNT_PVQMAX;
						pNodes->IterationControl().QviolatedCount++;
						pNode->Qgr = NodeLFQmax(pNode).Q;
						Qe = Q - pNode->Qgr;
					}
					else if (Q < pNode->LFQmin - Parameters.Imb - dPreviousImbQ)
					{
						pNode->Qgr = Q;
						LogNodeSwitch(it, "PV->PQmin");
						pNode->eLFNodeType_ = CDynaNodeBase::eLFNodeType::LFNT_PVQMIN;
						pNodes->IterationControl().QviolatedCount++;
						pNode->Qgr = NodeLFQmin(pNode).Q;
						Qe = Q - pNode->Qgr;
					}
					else
					{
						pNode->Qgr = Q;
						Qe = 0.0;
					}
					cplx dU{ I1 * cplx(Pe, -Qe) };
					dU += pNode->VreVim;
					dU = pNode->LFVref * dU / std::abs(dU);
					pNode->Vre = dU.real();
					pNode->Vim = dU.imag();
				}
				else
				{
					// до получения разрешения на переключение PV-узел считаем PQ-узлом (может лучше считать PV и распускать реактив)

					/*
					cplx dU = I1 * cplx(Pe, -Qe);
					pNode->Vre += dU.real();
					pNode->Vim += dU.imag();
					*/
					///*
					cplx dU{ I1 * cplx(Pe, -Qe) };
					dU += pNode->VreVim;
					dU = pNode->LFVref * dU / std::abs(dU);
					pNode->Vre = dU.real();
					pNode->Vim = dU.imag();
					//*/
				}
			}
			break;
			case CDynaNodeBase::eLFNodeType::LFNT_PQ:
			{
				pNodes->IterationControl().Update(pMatrixInfo);
				cplx dU{ I1 * cplx(Pe, -Qe) };
				pNode->Vre += dU.real();
				pNode->Vim += dU.imag();
			}
			break;
			case CDynaNodeBase::eLFNodeType::LFNT_BASE:
				pNode->Pgr += Pe;
				pNode->Qgr += Qe;
				Pe = Qe = 0.0;
				break;
			}

			pNode->UpdateVDeltaSuper();

			// Вариант с переупорядочиванием узлов в Зейделе по убыванию квадрата небалансов

			/* 
			for (_MatrixInfo* pMatrixInfo = m_pMatrixInfo.get(); pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
				pMatrixInfo->ImbSquare = ImbNorm(pMatrixInfo->m_dImbP, pMatrixInfo->m_dImbQ);

			std::sort(SeidellOrder.begin(), SeidellOrder.end(), [](const _MatrixInfo* lhs, const _MatrixInfo* rhs)
				{
					return lhs->ImbSquare > rhs->ImbSquare;
				});
			*/
		}

		if (!CheckLF())
			// если итерация привела не недопустимому режиму - выходим
			throw dfw2error(CDFW2Messages::m_cszUnacceptableLF);

		pNodes->DumpIterationControl();

		// если достигли заданного небаланса - выходим
		if (pNodes->IterationControl().Converged(Parameters.Imb))
			break;
	}
}

// строит матрицу якоби и правую части в формулировке по токам - все уравнения
// в мощностях разделены на модуль напряжения в узле

void CLoadFlow::BuildMatrixCurrent()
{
	double* pb{ klu.B() }, *pAx{ klu.Ax() };

	// обходим только узлы в матрице
	for (_MatrixInfo* pMatrixInfo = pMatrixInfo_.get(); pMatrixInfo < pMatrixInfoEnd; pMatrixInfo++)
	{
		// здесь считаем, что нагрузка СХН в Node::pnr/Node::qnr уже в расчете и анализе небалансов
		const auto& pNode{ pMatrixInfo->pNode };
		GetPnrQnrSuper(pNode);
		cplx Sneb;
		// диагональные производные проще чем в мощностях, их досчитываем в конце
		double dPdDelta{ 0.0 }, dQdDelta{ 0.0 };
		double dPdV{ 0.0 }, dQdV{ 0.0 };
		double* pAxSelf{ pAx };
		pAx += 2;
		// обратная величина от модуля напряжения в узле
		const double Vinv{ 1.0 / pNode->V };
		const double VinvSq{ Vinv * Vinv };
		// сопряженное напряжение деленное на модуль
		const cplx UnodeConjByV{ std::conj(pNode->VreVim) * Vinv };

		if (pNode->IsLFTypePQ())
		{
			// для PQ-узлов формируем оба уравнения

			// узлы в которых есть зависимые ограничения Q - рассчитываем и формируем производные dQ/dV
			if (pNode->eLFNodeType_ == CDynaNodeBase::eLFNodeType::LFNT_PVQMAX)
			{
				const auto Limit{ NodeLFQmax(pNode) };
				pNode->Qgr = Limit.Q;
				dQdV -= Limit.dIdV;
			}
			else if (pNode->eLFNodeType_ == CDynaNodeBase::eLFNodeType::LFNT_PVQMIN)
			{
				const auto Limit{ NodeLFQmin(pNode) };
				pNode->Qgr = Limit.Q;
				dQdV -= Limit.dIdV;
			}
			else
				dQdV += pNode->Qg * VinvSq;

			for (const auto& pv : pMatrixInfo->VirtualBranches())
			{
				const auto& pOppNode{ pv.pNode };
				// вычисляем компоненты производных по комплексным напряжениям в узлах (Давыдов, стр. 75)
				// при этом используем не напряжение в узле, а частное от напряжения и модуля (так как уравнения разделены на модуль)
				const cplx mult{ UnodeConjByV * pOppNode->VreVim * pv.Y };

				// небаланс по ветвям
				Sneb -= pOppNode->VreVim * pv.Y;

				// компоненты диагональных производных по углу
				dPdDelta -= mult.imag();
				dQdDelta += -mult.real();

				if (NodeInMatrix(pOppNode))
				{
					// внедиагональные производные (уже разделены на модуль, так как компоненты рассчитаны по UnodeConjByV
					const double dPdDeltaM{ mult.imag() };
					const double dPdVM{ -CDevice::ZeroDivGuard(mult.real(), pOppNode->V) };
					const double dQdDeltaM{ mult.real() };
					const double dQdVM{ CDevice::ZeroDivGuard(mult.imag(), pOppNode->V) };

					_CheckNumber(dPdDeltaM);
					_CheckNumber(dPdVM);
					_CheckNumber(dQdDeltaM);
					_CheckNumber(dQdVM);

					*pAx = dPdDeltaM;
					*(pAx + pMatrixInfo->nRowCount) = dQdDeltaM;
					pAx++;
					*pAx = dPdVM;
					*(pAx + pMatrixInfo->nRowCount) = dQdVM;
					pAx++;
				}
			}
		}
		else
		{
			// для PV-только уравнение для P
			for (const auto& pv : pMatrixInfo->VirtualBranches())
			{
				const auto& pOppNode{ pv.pNode };
				const cplx mult{ UnodeConjByV * pOppNode->VreVim * pv.Y };

				Sneb -= pOppNode->VreVim * pv.Y;

				dPdDelta -= mult.imag();

				if (NodeInMatrix(pOppNode))
				{
					// внедиагональные производные (уже разделены на модуль, так как компоненты рассчитаны по UnodeConjByV
					const double dPdDeltaM{ mult.imag() };
					const double dPdVM{ -CDevice::ZeroDivGuard(mult.real(), pOppNode->V) };

					_CheckNumber(dPdDeltaM);
					_CheckNumber(dPdVM);

					*pAx = dPdDeltaM;
					*(pAx + pMatrixInfo->nRowCount) = 0.0;
					pAx++;
					*pAx = dPdVM;
					*(pAx + pMatrixInfo->nRowCount) = 0.0;
					pAx++;
				}
			}
		}

		// генерация в узле (включая неуправляемую из суперузлов)
		const cplx NodeInjG(pNode->Pgr + pMatrixInfo->UncontrolledP, pNode->Qgr + pMatrixInfo->UncontrolledQ);
		// расчетная по СХН нагрузка в узле 
		const cplx NodeInjL(pNode->Pnr, pNode->Qnr);
		// небаланс в узле
		Sneb -= pNode->VreVim * pNode->YiiSuper;
		// небалансы в токах - поэтому делим мощности на модуль
		Sneb = Vinv * (std::conj(Sneb) * pNode->VreVim + NodeInjL - NodeInjG);

		// диагональные производные по напряжению
		// в уравнении разность генерации и нагрузки, деленная на напряжение. Генерация не зависит от напряжения.
		// Нагрузка с СХН - функция напряжения, поэтому генерацию просто делим на квадрат напряжения, а производную
		// нагрузки считаем по правилу частного (см док)

		dPdV = pNode->dLRCLoad.real() * Vinv - (pNode->Pnr - NodeInjG.real()) * VinvSq - pNode->YiiSuper.real();

		if (pNode->IsLFTypePQ())
			dQdV += pNode->dLRCLoad.imag() * Vinv - pNode->Qnr * VinvSq + pMatrixInfo->UncontrolledQ * VinvSq + pNode->YiiSuper.imag();
		else
		{
			dQdV = 1.0;
			Sneb.imag(0.0);	// если узел не PQ - обнуляем уравнение
		}


		_CheckNumber(dPdDelta);
		_CheckNumber(dPdV);
		_CheckNumber(dQdDelta);
		_CheckNumber(dQdV);

		*pAxSelf = dPdDelta;
		*(pAxSelf + pMatrixInfo->nRowCount) = dQdDelta;
		pAxSelf++;
		*pAxSelf = dPdV;
		*(pAxSelf + pMatrixInfo->nRowCount) = dQdV;

		pAx += pMatrixInfo->nRowCount;

		_CheckNumber(std::abs(Sneb));

		*pb = Sneb.real(); pb++;
		// Для PV узла обязательно обуляем уравнение
		*pb = Sneb.imag(); pb++;
	}
}

void CLoadFlow::BuildMatrixPower()
{
	double* pb = klu.B();
	double* pAx = klu.Ax();

	// обходим только узлы в матрице
	for (_MatrixInfo* pMatrixInfo = pMatrixInfo_.get(); pMatrixInfo < pMatrixInfoEnd; pMatrixInfo++)
	{
		// здесь считаем, что нагрузка СХН в Node::pnr/Node::qnr уже в расчете и анализе небалансов
		const auto& pNode{ pMatrixInfo->pNode };
		GetPnrQnrSuper(pNode);
		cplx Sneb;
		double dPdDelta{ 0.0 }, dQdDelta{ 0.0 };
		double dPdV{ pNode->GetSelfdPdV() }, dQdV{ 0.0 };
		double* pAxSelf = pAx;
		pAx += 2;

		const cplx UnodeConj(std::conj(pNode->VreVim));

		if (pNode->IsLFTypePQ())
		{
			// для PQ-узлов формируем оба уравнения

			dQdV = pNode->GetSelfdQdV();

			// узлы в которых есть зависимые ограничения Q - рассчитываем и формируем производные dQ/dV
			if (pNode->eLFNodeType_ == CDynaNodeBase::eLFNodeType::LFNT_PVQMAX)
			{
				const auto Limit{ NodeLFQmax(pNode) };
				pNode->Qgr = Limit.Q;
				dQdV -= Limit.dQdV;
			}
			else if (pNode->eLFNodeType_ == CDynaNodeBase::eLFNodeType::LFNT_PVQMIN)
			{
				const auto Limit{ NodeLFQmin(pNode) };
				pNode->Qgr = Limit.Q;
				dQdV -= Limit.dQdV;
			}

			for (const auto& pv : pMatrixInfo->VirtualBranches())
			{
				const auto& pOppNode{ pv.pNode };
				const cplx mult{ UnodeConj * pOppNode->VreVim * pv.Y };

				Sneb -= pOppNode->VreVim * pv.Y;

				dPdDelta -= mult.imag();
				dPdV += -CDevice::ZeroDivGuard(mult.real(), pNode->V);
				dQdDelta += -mult.real();
				dQdV += CDevice::ZeroDivGuard(mult.imag(), pNode->V);

				if (NodeInMatrix(pOppNode))
				{
					const double dPdDeltaM{ mult.imag() };
					const double dPdVM{ -CDevice::ZeroDivGuard(mult.real(), pOppNode->V) };
					const double dQdDeltaM{ mult.real() };
					const double dQdVM{ CDevice::ZeroDivGuard(mult.imag(), pOppNode->V) };

					_CheckNumber(dPdDeltaM);
					_CheckNumber(dPdVM);
					_CheckNumber(dQdDeltaM);
					_CheckNumber(dQdVM);

					*pAx = dPdDeltaM;
					*(pAx + pMatrixInfo->nRowCount) = dQdDeltaM;
					pAx++;
					*pAx = dPdVM;
					*(pAx + pMatrixInfo->nRowCount) = dQdVM;
					pAx++;
				}
			}
		}
		else
		{
			dQdV = 1.0;
			// для PV-только уравнение для P
			for (const auto& pv : pMatrixInfo->VirtualBranches())
			{
				const auto& pOppNode{ pv.pNode };
				const cplx mult{ UnodeConj * pOppNode->VreVim * pv.Y };

				Sneb -= pOppNode->VreVim * pv.Y;

				dPdDelta -= mult.imag();
				dPdV += -CDevice::ZeroDivGuard(mult.real(), pNode->V);

				if (NodeInMatrix(pOppNode))
				{
					double dPdDeltaM = mult.imag();
					double dPdVM = -CDevice::ZeroDivGuard(mult.real(), pOppNode->V);

					_CheckNumber(dPdDeltaM);
					_CheckNumber(dPdVM);

					*pAx = dPdDeltaM;
					*(pAx + pMatrixInfo->nRowCount) = 0.0;
					pAx++;
					*pAx = dPdVM;
					*(pAx + pMatrixInfo->nRowCount) = 0.0;
					pAx++;
				}
			}
		}

		const cplx NodeInjection(pNode->Pnr - pNode->Pgr - pMatrixInfo->UncontrolledP, pNode->Qnr - pNode->Qgr - pMatrixInfo->UncontrolledQ);
		Sneb -= std::conj(UnodeConj) * pNode->YiiSuper;
		Sneb = std::conj(Sneb) * std::conj(UnodeConj) + NodeInjection;

		_CheckNumber(dPdDelta);
		_CheckNumber(dPdV);
		_CheckNumber(dQdDelta);
		_CheckNumber(dQdV);

		*pAxSelf = dPdDelta;
		*(pAxSelf + pMatrixInfo->nRowCount) = dQdDelta;
		pAxSelf++;
		*pAxSelf = dPdV;
		*(pAxSelf + pMatrixInfo->nRowCount) = dQdV;

		pAx += pMatrixInfo->nRowCount;

		_CheckNumber(std::abs(Sneb));

		*pb = Sneb.real(); pb++;
		// Для PV узла обязательно обуляем уравнение
		*pb = pNode->IsLFTypePQ() ? Sneb.imag() : 0.0;  pb++;
	}
}

// расчет небаланса в узле полностью на SSE
void CLoadFlow::GetNodeImbSSE(_MatrixInfo* pMatrixInfo)
{
	const auto& pNode{ pMatrixInfo->pNode };
	// нагрузка по СХН
	GetPnrQnrSuper(pNode);

	__m128d neg = _mm_setr_pd(0.0, -0.0);

	//-pNode->VreVim * pNode->YiiSuper

	__m128d sV0 = _mm_load_pd(reinterpret_cast<double(&)[2]>(pNode->VreVim));
	__m128d sYs = _mm_load_pd(reinterpret_cast<double(&)[2]>(pNode->YiiSuper));

	__m128d sV1 = sV0;	// сохраняем VreVim, так как далее мы его испортим в умножении

	__m128d sI = _mm_xor_pd(sV1, sV1);
	{
		__m128d v3 = _mm_mul_pd(sV1, sYs);
		sV1 = _mm_permute_pd(sV1, 0x5);
		sYs = _mm_xor_pd(sYs, neg);
		__m128d v4 = _mm_mul_pd(sV1, sYs);
		sYs = _mm_hsub_pd(v3, v4);
		sI = _mm_sub_pd(sI, sYs);
	}


	for (const auto& pv : pMatrixInfo->VirtualBranches())
		//for (VirtualBranch* pBranch = pMatrixInfo->pNode->pVirtualBranchBegin_; pBranch < pMatrixInfo->pNode->pVirtualBranchEnd_; pBranch++)
	{
		const auto& pOppNode{ pv.pNode };

		//I -= pOppNode->VreVim * pBranch->Y;

		__m128d yb = _mm_load_pd(reinterpret_cast<const double(&)[2]>(pv.Y));
		__m128d ov = _mm_load_pd(reinterpret_cast<const double(&)[2]>(pv.pNode->VreVim));
		__m128d v3 = _mm_mul_pd(yb, ov);
		yb = _mm_permute_pd(yb, 0x5);
		ov = _mm_xor_pd(ov, neg);
		__m128d v4 = _mm_mul_pd(yb, ov);
		yb = _mm_hsub_pd(v3, v4);
		sI = _mm_sub_pd(sI, yb);
	}


	//I = std::conj(I) * pNode->VreVim;

	sV1 = sV0;
	{
		__m128d sIconj = _mm_xor_pd(sI, neg);		// сопряжение тока
		__m128d v3 = _mm_mul_pd(sIconj, sV1);
		sIconj = _mm_permute_pd(sIconj, 0x5);
		sV1 = _mm_xor_pd(sV1, neg);
		__m128d v4 = _mm_mul_pd(sIconj, sV1);
		sI = _mm_hsub_pd(v3, v4);
	}

	//I += cplx(pNode->Pnr - pNode->Pgr - pMatrixInfo->UncontrolledP, pNode->Qnr - pNode->Qgr - pMatrixInfo->UncontrolledQ);

	sV1 = _mm_load_pd(&pNode->Pnr);
	sV1 = _mm_sub_pd(sV1, _mm_load_pd(&pNode->Pgr));
	sV1 = _mm_sub_pd(sV1, _mm_load_pd(&pMatrixInfo->UncontrolledP));
	sI = _mm_add_pd(sI, sV1);

	_mm_store_pd(&pMatrixInfo->ImbP, sI);
}


// расчет небаланса в узле
void CLoadFlow::GetNodeImb(_MatrixInfo* pMatrixInfo)
{
	GetNodeImbSSE(pMatrixInfo);
	return;
	const auto& pNode{ pMatrixInfo->pNode };
	// нагрузка по СХН
	GetPnrQnrSuper(pNode);

	alignas(16) cplx I { -pNode->VreVim * pNode->YiiSuper };

	for (const auto& pv : pMatrixInfo->VirtualBranches())
	{
		const auto& pOppNode{ pv.pNode };
		// в отладке можем посмотреть мощность перетока по ветви
		//cplx sx{ std::conj(cplx(pOppNode->Vre, pOppNode->Vim) * pBranch->Y) * Vnode };
		I -= pOppNode->VreVim * pv.Y;
	}

	I = std::conj(I) * pNode->VreVim;
	I += cplx(pNode->Pnr - pNode->Pgr - pMatrixInfo->UncontrolledP, pNode->Qnr - pNode->Qgr - pMatrixInfo->UncontrolledQ);
	CDevice::FromComplex(pMatrixInfo->ImbP, pMatrixInfo->ImbQ, I);
}

bool CLoadFlow::Run()
{
	bool bRes{ true };

	pNodes = static_cast<CDynaNodeContainer*>(pDynaModel->GetDeviceContainer(DEVTYPE_NODE));
	if (!pNodes)
		throw dfw2error("CLoadFlow::Start - node container unavailable");

	try
	{
		// вводим СХН УР и V0 = Unom
		pNodes->SwitchLRCs(false);

		Start();
		Estimate();
		
		//Newton();

		switch (Parameters.Startup)
		{
		case CLoadFlow::eLoadFlowStartupMethod::Seidell:
			// испольузем Z0-оценку для Зейделя на плоском старте
			if (Parameters.Flat)
				Z0();
			Seidell();
			pNodes->CalculateSuperNodesAdmittances(false);
			// после коррекции проводимостей
			// обновляем систему дополнительных ветвей
			ExtraNodes_->UpdateVirtualBranches();
			break;
		case CLoadFlow::eLoadFlowStartupMethod::Tanh:
			NewtonTanh();
			break;
		case CLoadFlow::eLoadFlowStartupMethod::RKF:
			ContinuousNewton();
			break;
		}

		try
		{
			Newton();
			RestoreSuperNodes();
#ifdef _DEBUG
			CompareWithRastr();
#endif
			// выводим сравнение с Растр до обмена СХН
			DumpNodes();
		}
		catch (dfw2error&)
		{
			RestoreSuperNodes();
#ifdef _DEBUG
			CompareWithRastr();
#endif
			// выводим сравнение с Растр до обмена СХН
			DumpNodes();
			throw;
		}

		pNodes->SwitchLRCs(true);

		for (auto&& it : pNodes->DevVec)
		{
			const auto& pNode{ static_cast<CDynaNodeBase*>(it) };

			if (!pNode->IsStateOn())
				continue;

			if (!pNode->pSuperNodeParent)
			{
				// этот вызов портит qnr
				// если его вызвать после 
				// переключения СХН на динамику

				// до переключения он просто не работает
				// с СХН на постоянную мощность
				// которые в УР заданы nullptr
				pNode->GetPnrQnrSuper();

				// небаланс рассчитывается через
				// матрицу, поэтому создаем временную строку
				// и привязываем к ней узел
				_MatrixInfo mx;
				mx.Store(pNode);
				GetNodeImb(&mx);
				//_ASSERTE(std::abs(mx.ImbP) < Parameters.Imb && std::abs(mx.ImbQ) < Parameters.Imb);
				pNode->GetPnrQnr();
			}
		}

		UpdateQToGenerators();
		CheckFeasible();
	}
	catch (const dfw2error&)
	{
		pNodes->SwitchLRCs(true);
		throw;
	}
	return bRes;
}

struct NodePair
{
	const CDynaNodeBase* pNodeIp;
	const CDynaNodeBase* pNodeIq;
	bool operator < (const NodePair& rhs) const
	{
		ptrdiff_t nDiff = pNodeIp - rhs.pNodeIp;

		if (!nDiff)
			return pNodeIq < rhs.pNodeIq;

		return nDiff < 0;
	}

	NodePair(const CDynaNodeBase* pIp, const CDynaNodeBase* pIq) : pNodeIp(pIp), pNodeIq(pIq)
	{
		if (pIp > pIq)
			std::swap(pNodeIp, pNodeIq);
	}
};


bool CLoadFlow::CheckLF()
{
	bool bRes = true;

	std::set <NodePair> ReportedBranches;

	for (_MatrixInfo* pMatrixInfo = pMatrixInfo_.get(); pMatrixInfo < pMatrixInfoEnd; pMatrixInfo++)
	{
		const auto& pNode{ pMatrixInfo->pNode };
		double dV = pNode->V / pNode->Unom;
		if (dV > 2.0)
		{
			pNode->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszLFNodeVTooHigh, pNode->GetVerbalName(), dV));
			bRes = false;
		}
		else if (dV < 0.5)
		{
			pNode->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszLFNodeVTooLow, pNode->GetVerbalName(), dV));
			bRes = false;
		}

		for (const auto& pv : pMatrixInfo->VirtualBranches())
		{
			// рассчитываем угол с устранением проворота. До проворота не должны дойти
			// так как ограничиваем шаг по углам
			const double dDelta{ MathUtils::AngleRoutines::WrapPosNegPI(pNode->Delta - pv.pNode->Delta) };

			if (dDelta > M_PI_2 || dDelta < -M_PI_2)
			{
				bRes = false;
				if (ReportedBranches.insert(NodePair(pNode, pv.pNode)).second)
					pDynaModel->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(
						CDFW2Messages::m_cszLFBranchAngleExceeds90,
						pNode->GetVerbalName(),
						pv.pNode->GetVerbalName(),
						dDelta * 180.0 / M_PI));
			}
		}
	}
	return bRes;
}


void CLoadFlow::UpdatePQFromGenerators()
{

	if (!pNodes->CheckLink(1))
		return;	// если генераторов нет - выходим. В узлах остаются инъеции pg/qg;

	for (auto&& it : pNodes->DevVec)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(it) };

		if (!pNode->IsStateOn())
			continue;

		// проходим по генераторам, подключенным к узлу
		const  CLinkPtrCount* const pGenLink{ pNode->GetLink(1) };
		LinkWalker< CDynaPowerInjector> pGen;

		// сбрасываем суммарные ограничения Q генераторов
		pNode->LFQminGen = pNode->LFQmaxGen = 0.0;

		if (pGenLink->Count())
		{
			pNode->Pg = pNode->Qg = 0.0;
			pNode->LFQmin = pNode->LFQmax = 0.0;
			while (pGenLink->In(pGen))
			{
				// инжектор типа компенсатора - пропускаем
				// он учитывается не так, как генератор, а
				// в отдельном искусственном узле
				if (pGen->IsKindOfType(DEVTYPE_SVC))
					continue;

				if (pGen->IsStateOn())
				{
					if (pGen->Kgen <= 0)
					{
						pGen->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszWrongGeneratorsNumberFixed,
							pGen->GetVerbalName(),
							pGen->Kgen));
						pGen->Kgen = 1.0;
					}

					pNode->Pg += pGen->P;

					// проверяем ограничения по реактивной мощности
					if (pGen->LFQmin > pGen->LFQmax)
					{
						pGen->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszWrongGeneratorQlimitsFixed,
							pGen->LFQmin,
							pGen->LFQmax,
							pGen->LFQmax));

						// если Qmin>Qmax ставим Qmin=Qmax (нужно больше реактивки)
						pGen->LFQmin = pGen->LFQmax;

					}
									
					// вводим Q генератора в диапазон
					pGen->Q = (std::max)((std::min)(static_cast<double>(pGen->Q), pGen->LFQmax * pGen->Kgen), pGen->LFQmin * pGen->Kgen);
					pNode->Qg += pGen->Q;
					// рассчитываем суммарные ограничения по генераторам
					// в узлах остаются два ограничения - обычные (для суперузла - сумма)
					// и ограничения по сумме генераторов (в суперузлы не суммируются и для каждого узла индивидуальные)
					pNode->LFQminGen = pNode->LFQmin += pGen->LFQmin * pGen->Kgen;
					pNode->LFQmaxGen = pNode->LFQmax += pGen->LFQmax * pGen->Kgen;
				}
			}
		}
	}
}

void CLoadFlow::UpdateQToGenerators()
{
	bool bAllOk{ true };

	if (!pNodes->CheckLink(1))
		return;	// если генераторов нет - выходим

	for (auto&& it : pNodes->DevVec)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(it) };

		if (!pNode->IsStateOn())
			continue;

		const CLinkPtrCount* const pGenLink{ pNode->GetLink(1) };
		LinkWalker<CDynaPowerInjector> pGen;

		std::list<CDynaPowerInjector*> PSlackGens, QSlackGens, QRangeGens;
		if (pGenLink->Count())
		{
			const double NodeQrange{ pNode->LFQmax - pNode->LFQmin };
			double Qspread(0.0), Qgmin(0.0), Qgmax(0.0);

			while (pGenLink->In(pGen))
			{
				if (!pGen->IsStateOn())
				{
					pGen->Q = 0.0;		// отключенные генераторы 
					continue;			// просто обнуляются
				}

				if (pGen->IsKindOfType(DEVTYPE_SVC))
				{
					Qspread -= pGen->Q; // мощности компенсаторов уже рассчитаны
					continue;			// поэтому просто изымаем их из диапазона
				}

				if (pGen->LFQmin == 0.0 && pGen->LFQmax == 0.0)
					QSlackGens.emplace_back(pGen);	// генераторы без диапазона
				else
					QRangeGens.emplace_back(pGen);

				pGen->Q = 0.0;
			}

			for (auto&& gen : QRangeGens)
			{
				if (NodeQrange > 0.0)
				{
					gen->Q = gen->Kgen * (gen->LFQmin + (gen->LFQmax - gen->LFQmin) / NodeQrange * (pNode->Qg - pNode->LFQmin));
					Qgmin += gen->LFQmin;
					Qgmax += gen->LFQmax;
					_CheckNumber(gen->Q);
				}
				else
				{
					gen->Q = gen->Kgen * (gen->LFQmin + (pNode->Qg - pNode->LFQmin) / QRangeGens.size());
					Qgmin += gen->Q;
					Qgmax += gen->Q;
				}
				Qspread += gen->Q;
			}

			// если есть балансирующие по Q генераторы - распределяем в них весь оставшийся небаланс поровну
			const auto QspreadSource{ Qspread };
			for (auto&& gen : QSlackGens)
			{
				Qgmin = Qgmax = gen->Q = gen->Kgen * ((pNode->Qg - QspreadSource) / QSlackGens.size());
				Qspread += gen->Q;
				Qgmin += gen->LFQmin;
				Qgmax += gen->LFQmax;
			}

			// если узел балансирующий, распределяем активную мощность по генераторам
			// пропорционально Pном
			if (pNode->IsLFTypeSlack())
			{
				// собираем все включенные генераторы в общий список
				QRangeGens.splice(QRangeGens.end(), QSlackGens);
				double Psum{ 0.0 };
				// рассчитываем общую мощность генераторов в исходном режиме
				for (auto&& gen : QRangeGens)
					Psum += std::abs(gen->P);
				// если генераторы были нагружены в исходном режиме
				// распределяем мощность из базисного узла
				// пропорционально нагрузке, иначе просто поровну
				for (auto&& gen : QRangeGens)
					if (Psum > 0.0)
						gen->P *= gen->Kgen * std::abs(pNode->Pgr) / Psum;
					else
						gen->P = gen->Kgen * pNode->Pgr / QRangeGens.size();
			}

			if (std::abs(pNode->Qg - Qspread) > Parameters.Imb)
			{
				pNode->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszLFWrongQrangeForNode,
					pNode->GetVerbalName(),
					pNode->Qg,
					Qgmin,
					Qgmax));
				bAllOk = false;
			}
		}
	}
	if (!bAllOk)
		throw dfw2error(CDFW2Messages::m_cszLFError);
}

//! собирает данные по компенсаторам и определяет необходимое количество искусственных узлов

void CLoadFlow::CollectSVCData()
{
	// собираем все типы компенсаторов
	for (const auto& container : pDynaModel->DeviceContainers())
	{
		if (!container->IsKindOfType(DEVTYPE_SVC))
			continue;

		for (auto&& svc : *container)
			if (svc->IsStateOn())
			{
				CDevice::IsFunctionStatusOK(svc->PreInit(pDynaModel));
				SVCs_.emplace_back(SVCExtra{static_cast<CDynaSVCBase*>(&*svc)});
			}
	}

	if (!SVCs_.empty())
		ExtraNodes_->CreateSVCNodes();
}

void CLoadFlow::GetPnrQnrSuper(CDynaNodeBase* pNode)
{
	GetPnrQnr(pNode);
	const CLinkPtrCount* const pLink{ pNode->GetSuperLink(0) };

	if (pLink->Count())
	{
		__m128d load = _mm_load_pd(&pNode->Pnr);
		__m128d lrc = _mm_load_pd(reinterpret_cast<double(&)[2]>(pNode->dLRCLoad));

		LinkWalker<CDynaNodeBase> pSlaveNode;
		while (pLink->In(pSlaveNode))
		{
			GetPnrQnr(pSlaveNode);

			// pNode->Pnr += pSlaveNode->Pnr;
			// pNode->Qnr += pSlaveNode->Qnr;
			load = _mm_add_pd(load, _mm_load_pd(&pSlaveNode->Pnr));
			// pNode->dLRCLoad += pSlaveNode->dLRCLoad;
			lrc = _mm_add_pd(lrc, _mm_load_pd(reinterpret_cast<double(&)[2]>(pSlaveNode->dLRCLoad)));
		}
		_mm_store_pd(&pNode->Pnr, load);
		_mm_store_pd(reinterpret_cast<double(&)[2]>(pNode->dLRCLoad), lrc);
	}
}

void CLoadFlow::GetPnrQnr(CDynaNodeBase* pNode)
{
	// по умолчанию нагрузка равна заданной в УР
	pNode->Pnr = pNode->Pn;
	pNode->Qnr = pNode->Qn;
	double VdVnom = pNode->V / pNode->V0;

	if (VdVnom < 0.0)
	{
		//throw dfw2error(fmt::format("CLoadFlow::GetPnrQnr {} negative V/Vnom {:.3f}", pNode->GetVerbalName(), VdVnom));
		pNode->dLRCLoad = pNode->Pnr = pNode->Qnr = 0.0;
		return;
	}

	pNode->dLRCLoad = 0.0;

	// если есть СХН нагрузки, рассчитываем
	// комплексную мощность и производные по напряжению

	if (pNode->pLRC)
	{
		// для узлов с отрицательными нагрузками СХН не рассчитываем
		// если такие СХН не разрешены в параметрах

		double& re{ reinterpret_cast<double(&)[2]>(pNode->dLRCLoad)[0] };
		double& im{ reinterpret_cast<double(&)[2]>(pNode->dLRCLoad)[1] };

		// тут бы конечно не использовать параметры, а просто обнулить указатель
		// на СХН для узлов с P/Q <= 0
		if (Parameters.AllowNegativeLRC || pNode->Pn > 0.0)
		{
			pNode->Pnr *= pNode->pLRC->P()->GetBoth(VdVnom, re, pNode->LRCVicinity);
			re *= pNode->Pn;
		}

		if (Parameters.AllowNegativeLRC || pNode->Qn > 0.0)
		{
			pNode->Qnr *= pNode->pLRC->Q()->GetBoth(VdVnom, im, pNode->LRCVicinity);
			im *=  pNode->Qn;
		}

		pNode->dLRCLoad /= pNode->V0;
	}
}

void CLoadFlow::DumpNodes()
{
	std::ofstream dump(pDynaModel->Platform().ResultFile("resnodes.csv"));
	if (dump.is_open())
	{
		dump << "N;V;D;Pn;Qn;Pnr;Qnr;Pg;Qg;Qgr;Type;Qmin;Qmax;Vref;VR;DeltaR;QgR;QnR" << std::endl;
		for (auto&& it : pNodes->DevVec)
		{
			const auto& pNode{ static_cast<CDynaNodeBase*>(it) };
#ifdef _DEBUG
			dump << fmt::format("{};{};{};{};{};{};{};{};{};{};{};{};{};{};{};{};{};{}",
				pNode->GetId(),
				pNode->V.Value,
				pNode->Delta.Value / M_PI * 180.0,
				pNode->Pn,
				pNode->Qn,
				pNode->Pnr,
				pNode->Qnr,
				pNode->Pg,
				pNode->Qg,
				pNode->Qgr,
				pNode->eLFNodeType_,
				pNode->LFQmin,
				pNode->LFQmax,
				pNode->LFVref,
				pNode->Vrastr,
				pNode->Deltarastr / M_PI * 180.0,
				pNode->Qgrastr,
				pNode->Qnrrastr) << std::endl;
#else
			dump << fmt::format("{};{};{};{};{};{};{};{};{};{};{};{};{}",
				pNode->GetId(),
				pNode->V.Value,
				pNode->Delta.Value / M_PI * 180.0,
				pNode->Pn,
				pNode->Qn,
				pNode->Pnr,
				pNode->Qnr,
				pNode->Pgr,
				pNode->Qg,
				pNode->eLFNodeType_,
				pNode->LFQmin,
				pNode->LFQmax,
				pNode->LFVref) << std::endl;
#endif
		}

	}
}

double CLoadFlow::ImbNorm(double x, double y)
{
	return x * x + y * y;
}

void CLoadFlow::SolveLinearSystem()
{
	if (!klu.Analyzed())
		klu.Analyze();

	if (klu.Factored())
	{
		if (!klu.TryRefactor())
			klu.Factor();
	}
	else
		klu.Factor();

	klu.Solve();
}

void CLoadFlow::Newton()
{
	pDynaModel->Log(DFW2MessageStatus::DFW2LOG_INFO, CDFW2Messages::m_cszLFRunningNewton);
	size_t& it{ pNodes->IterationControl().Number };

	Limits limits(*this);

	NewtonStepRatio.eStepCause = DFW2::LFNewtonStepRatio::eStepLimitCause::None;
	NewtonStepRatio.Ratio_ = 1.0;

	// квадраты небалансов до и после итерации
	double g0(0.0), g1(0.1);
	lambda_ = 1.0;
	NodeTypeSwitchesDone = 1;
	it = 0;
	
	while (1)
	{
		if (!CheckLF())
			throw dfw2error(CDFW2Messages::m_cszUnacceptableLF);

		if (it > Parameters.MaxIterations)
			throw dfw2error(CDFW2Messages::m_cszLFNoConvergence);

		pNodes->IterationControl().Reset();
		limits.Clear();

		// считаем небаланс по всем узлам кроме БУ
		_MatrixInfo* pMatrixInfo{ pMatrixInfo_.get() };
		for (; pMatrixInfo < pMatrixInfoEnd; pMatrixInfo++)
		{
			const auto& pNode{ pMatrixInfo->pNode };
			// небаланс считается с учетом СХН
			GetNodeImb(pMatrixInfo);
			// обновляем данные итерации от текущего узла
			pNodes->IterationControl().Update(pMatrixInfo);
			limits.Select(pMatrixInfo);
		}

		// общее количество узлов с нарушенными ограничениями и подлежащих переключению
		pNodes->IterationControl().QviolatedCount = limits.ViolatedCount();

		UpdateSlackBusesImbalance();

		g1 = GetSquaredImb();

		// отношение квадратов невязок
		pNodes->IterationControl().ImbRatio = g0 > 0.0 ? g1 / g0 : 0.0;

		DumpNewtonIterationControl();
		it++;

		if (pNodes->IterationControl().Converged(Parameters.Imb))
			break;

		// если не было переключений типов узлов на прошлой итерации 
		// и небаланс увеличился, делаем backtrack
		if (!NodeTypeSwitchesDone && g1 > g0)
		{
			const double gs1v{ -CDynaModel::gs1(klu, pRh, klu.B()) };
			// знак gs1v должен быть "-" ????
			lambda_ *= -0.5 * gs1v / (g1 - g0 - gs1v);
			// если шаг слишком мал и есть узлы для переключений,
			// несмотря на не снижающийся небаланс переходим к переключениям узлов
			if (lambda_ > Parameters.ForceSwitchLambda)
			{
				// ограничение шага не достигло значения
				// для принудительного переключения
				RestoreVDelta();
				UpdateVDelta(-lambda_);
				NewtonStepRatio.Ratio_ = lambda_;
				NewtonStepRatio.eStepCause = LFNewtonStepRatio::eStepLimitCause::Backtrack;
				continue;
			}
		}

		// сохраняем исходные значения переменных
		StoreVDelta();

		limits.Apply();

		lambda_ = 1.0;
		g0 = g1;

		switch (Parameters.LFFormulation)
		{
		case eLoadFlowFormulation::Power:
			BuildMatrixPower();
			break;
		case eLoadFlowFormulation::Current:
			BuildMatrixCurrent(); 
			break;
		}

		// сохраняем небаланс до итерации
		std::copy(klu.B(), klu.B() + klu.MatrixSize(), pRh.get());

		// Находим узел с максимальной невязкой до шага Ньютона
		ptrdiff_t iMax(0);
		double maxb{ klu.FindMaxB(iMax) };
		CDynaNodeBase* pNode1(pMatrixInfo_.get()[iMax / 2].pNode);

		SolveLinearSystem();

		// Находим узел с максимальным приращением
		maxb = klu.FindMaxB(iMax);
		CDynaNodeBase* pNode2(pMatrixInfo_.get()[iMax / 2].pNode);

		// обновляем переменные
		double MaxRatio = GetNewtonRatio(klu.B());
		UpdateVDelta(-MaxRatio);
		if (NewtonStepRatio.eStepCause != LFNewtonStepRatio::eStepLimitCause::None)
			NodeTypeSwitchesDone = 1;
	}

	// обновляем реактивную генерацию в суперузлах
	UpdateSupernodesPQ();
}

// обновляет инъекции в суперзулах
void CLoadFlow::UpdateSupernodesPQ()
{
	bool bAllOk(true);

	for (_MatrixInfo* pMatrixInfo = pMatrixInfo_.get(); pMatrixInfo < pMatrixInfoSlackEnd; pMatrixInfo++)
	{
		CDynaNodeBase*& pNode{ pMatrixInfo->pNode };
		// диапазон реактивной мощности узла
		double Qrange{ pNode->LFQmax - pNode->LFQmin };
		double Qspread{ 0.0 }, PgSource{ pNode->Pgr }, QgSource{ pNode->Qgr }, DropToSlack{ 0.0 };
		const CLinkPtrCount* const pLink{ pNode->GetSuperLink(0) };
		LinkWalker<CDynaNodeBase> pSlaveNode;
		std::list<CDynaNodeBase*> SlackBuses;

		if (!pNode->pSuperNodeParent)
		{
			GetNodeImb(pMatrixInfo);
			_ASSERTE(std::abs(pMatrixInfo->ImbP) < Parameters.Imb && std::abs(pMatrixInfo->ImbQ) < Parameters.Imb);
		}

		// если узел нагрузочный в нем ничего не может измениться
		if (pNode->eLFNodeType_ != CDynaNodeBase::eLFNodeType::LFNT_PQ)
		{
			// все остальные узлы - PV, PVQmin, PVQmax обрабатываем по общим правилам
			Qrange = (Qrange > 0.0) ? (QgSource - pNode->LFQmin) / Qrange : 0.0;

			// если узел базисный, вносим его в список базисных узлов данного суперузла
			if (pNode->IsLFTypeSlack())
				SlackBuses.push_back(pNode);

			while (pLink->In(pSlaveNode))
			{
				// распределяем реактивную мощность по ненагрузочным узлам
				if (pSlaveNode->IsStateOn())
				{
					switch (pSlaveNode->eLFNodeType_)
					{
					case CDynaNodeBase::eLFNodeType::LFNT_BASE:
						SlackBuses.push_back(pSlaveNode);
						break;
					case CDynaNodeBase::eLFNodeType::LFNT_PV:
					case CDynaNodeBase::eLFNodeType::LFNT_PVQMIN:
					case CDynaNodeBase::eLFNodeType::LFNT_PVQMAX:
						Qspread += pSlaveNode->Qg = pSlaveNode->Qgr = pSlaveNode->LFQmin + (pSlaveNode->LFQmax - pSlaveNode->LFQmin) * Qrange;
					case CDynaNodeBase::eLFNodeType::LFNT_PQ:
						//PgSource -= pSlaveNode->Pg;
						_ASSERTE(pSlaveNode->Pg == pSlaveNode->Pgr);
						break;

					}
				}
				else
					pSlaveNode->Qg = pSlaveNode->Qgr = 0.0;
			}

			if (!pNode->IsLFTypeSlack())
				Qspread += pNode->Qgr = pNode->Qg = pMatrixInfo->LFQmin + (pMatrixInfo->LFQmax - pMatrixInfo->LFQmin) * Qrange;

			for (auto&& sb : SlackBuses)
			{
				sb->Pgr = sb->Pg = PgSource / SlackBuses.size();
				sb->Qgr = sb->Qg = (QgSource - Qspread) / SlackBuses.size();
				Qspread = QgSource;
			}

			if (std::abs(Qspread - QgSource) > Parameters.Imb)
			{
				bAllOk = false;
				pNode->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszLFWrongQrangeForSuperNode,
					pNode->GetVerbalName(),
					QgSource,
					pNode->LFQmin,
					pNode->LFQmax));
			}
		}
	}

	if (!bAllOk)
		throw dfw2error(CDFW2Messages::m_cszLFError);
}

void CLoadFlow::RestoreSuperNodes()
{
	// после расчета УР переносим мощности
	// компенсаторов в генерацию узлов

	ExtraNodes_->Restore();
	for (_MatrixInfo* pMatrixInfo = pMatrixInfo_.get(); pMatrixInfo < pMatrixInfoSlackEnd; pMatrixInfo++)
		pMatrixInfo->Restore();

	for (auto&& it : pNodes->DevVec)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(it) };
		if (!pNode->pSuperNodeParent)
			GetPnrQnr(pNode);
	}
}

// Возвращает коэффициент шага Ньютона

double CLoadFlow::GetNewtonRatio(double const* b)
{
	NewtonStepRatio.Reset();

	const _MatrixInfo* pMatrixInfo{ pMatrixInfo_.get() };

	// проходим по всем узлам
	for (; pMatrixInfo < pMatrixInfoEnd; pMatrixInfo++)
	{
		const auto& pNode{ pMatrixInfo->pNode };
		double const* pb{ b + pNode->A(0) };
		// рассчитываем новый угол в узле
		const double newDelta{ pNode->Delta - *pb };

		// поиск ограничения шага по углу узла
		const double Dstep{ std::abs(MathUtils::AngleRoutines::GetAbsoluteDiff2Angles(*pb, 0.0)) };

		if (Dstep > Parameters.NodeAngleNewtonStep)
			NewtonStepRatio.UpdateNodeAngle(Parameters.NodeAngleNewtonStep / Dstep, pNode);

		// рассчитываем новый модуль напряжения в узле
		pb++;
		const double newV{ pNode->V - *pb };
		const double Ratio{ newV / pNode->Unom };

		// поиск ограничения по модулю напряжения
		for (const double d : {0.5, 2.0})
		{
			if ((d > 1.0) ? Ratio > d : Ratio < d)
				NewtonStepRatio.UpdateVoltageOutOfRange((pNode->V - d * pNode->Unom) / *pb, pNode);
		}

		// поиск ограничения шага по напряжению
		const double VstepPU{ std::abs(*pb) / pNode->Unom };
		if (VstepPU > Parameters.VoltageNewtonStep)
			NewtonStepRatio.UpdateVoltage(Parameters.VoltageNewtonStep / VstepPU, pNode);
	}

	// поиск ограничения по взимному углу
	for (auto&& it : BranchAngleCheck)
	{
		const auto& pBranch{ static_cast<CDynaBranch*>(it) };
		const auto& pNodeIp{ pBranch->pNodeIp_ }, &pNodeIq{ pBranch->pNodeIq_ };
		// текущий взаимный угол
		const double CurrentDelta{ pNodeIp->Delta - pNodeIq->Delta };
		// приращения углов для узлов, которые в матрице или нули, если не в матрице
		const double dIpDelta{ NodeInMatrix(pNodeIp) ? *(b + pNodeIp->A(0)) : 0.0 };
		const double dIqDelta{ NodeInMatrix(pNodeIq) ? *(b + pNodeIq->A(0)) : 0.0 };

		// полное изменение угла от Ньютона с проворотами
		const double DeltaDiff{ dIqDelta -  dIpDelta };
		// это же изменение угла но с удалением проворотов
		const double DeltaDiffWrapped{ MathUtils::AngleRoutines::GetAbsoluteDiff2Angles(dIqDelta, dIpDelta) };
		//_ASSERTE(Equal(DeltaDiff, dIpDelta - dIqDelta));

		// новый взаимный угол с удалением проворотов
		const double NewDelta{ CurrentDelta + DeltaDiffWrapped };
		// модуль угла изменения с проворотами
		const double AbsDeltaDiff{ std::abs(DeltaDiff) };

		// проверяем новый взаимный угол на ограничения в 90 градусов

		const double NewDeltaWrapped{ MathUtils::AngleRoutines::WrapPosNegPI(NewDelta) };
		
//		if (NewDeltaWrapped > M_PI_2 || NewDeltaWrapped < -M_PI_2)
//			NewtonStepRatio.UpdateBranchAngleOutOfRange(
//				(std::min)(std::abs(MathUtils::AngleRoutines::GetAbsoluteDiff2Angles(CurrentDelta, -M_PI_2)),
//						   std::abs(MathUtils::AngleRoutines::GetAbsoluteDiff2Angles(CurrentDelta, M_PI_2))) / AbsDeltaDiff, pNodeIp, pNodeIq);

		// если новый угол превысил +90
		// считаем разность между текущим углом и +90
		// и делим на полный шаг Ньютона с проворотами
		// таким образом ограничиваем шаг Ньютона до первого "пересечения" с +90
		// точно так же поступаем с -90
		if (NewDeltaWrapped > M_PI_2)
			NewtonStepRatio.UpdateBranchAngleOutOfRange(std::abs(MathUtils::AngleRoutines::GetAbsoluteDiff2Angles(CurrentDelta, M_PI_2)) / AbsDeltaDiff, pNodeIp, pNodeIq);
		else if (NewDeltaWrapped < -M_PI_2)
			NewtonStepRatio.UpdateBranchAngleOutOfRange(std::abs(MathUtils::AngleRoutines::GetAbsoluteDiff2Angles(CurrentDelta, -M_PI_2)) / AbsDeltaDiff, pNodeIp, pNodeIq);

		// проверяем приращение взаимного угла
		if (AbsDeltaDiff > Parameters.BranchAngleNewtonStep)
			NewtonStepRatio.UpdateBranchAngle(Parameters.BranchAngleNewtonStep / AbsDeltaDiff, pNodeIp, pNodeIq);
	}

	return NewtonStepRatio.Ratio_;
}

void CLoadFlow::UpdateVDelta(double const* b, double Mult)
{
	_MatrixInfo* pMatrixInfo{ pMatrixInfo_.get() };
	double const* pb{ b };
	for (; pMatrixInfo < pMatrixInfoEnd; pMatrixInfo++)
	{
		const auto& pNode{ pMatrixInfo->pNode };
		const double* pb{ b + pNode->A(0) };
		const double newDelta{ pNode->Delta + *pb * Mult };
		pb++;
		const double newV{ pNode->V + *pb * Mult};
		pNode->Delta = newDelta;
		pNode->V = newV;
		pNode->UpdateVreVimSuper();
	}
}

void CLoadFlow::UpdateVDelta(double dStep)
{
	UpdateVDelta(klu.B(), dStep);
}


void CLoadFlow::StoreVDelta()
{
	if (!pVbackup)
		pVbackup = std::make_unique<double[]>(klu.MatrixSize());
	if (!pDbackup)
		pDbackup = std::make_unique<double[]>(klu.MatrixSize());

	double* pV{ pVbackup.get() };
	double* pD{ pDbackup.get() };
	for (_MatrixInfo* pMatrix = pMatrixInfo_.get(); pMatrix < pMatrixInfoEnd; pMatrix++, pV++, pD++)
	{
		*pV = pMatrix->pNode->V;
		*pD = pMatrix->pNode->Delta;
	}
}

void CLoadFlow::RestoreVDelta()
{
	if (pVbackup && pDbackup)
	{
		double const* pV{ pVbackup.get() };
		double const* pD{ pDbackup.get() };
		for (_MatrixInfo* pMatrix = pMatrixInfo_.get(); pMatrix < pMatrixInfoEnd; pMatrix++, pV++, pD++)
		{
			pMatrix->pNode->V = *pV;
			pMatrix->pNode->Delta = *pD;
			pMatrix->pNode->UpdateVreVimSuper();
		}
	}
	else
		throw dfw2error("CLoadFlow::RestoreVDelta called before StoreVDelta");
}


void CLoadFlow::RestoreUpdateVDelta(double const* b, double Mult)
{
	if (pVbackup && pDbackup)
	{
		double const* pV{ pVbackup.get() };
		double const* pD{ pDbackup.get() };
		for (_MatrixInfo* pMatrix = pMatrixInfo_.get(); pMatrix < pMatrixInfoEnd; pMatrix++, pV++, pD++)
		{
			const auto& pNode{ pMatrix->pNode };
			const double* pb{ b + pNode->A(0) };
			const double newDelta{ *pD + *pb * Mult };
			pb++;
			const double newV{ *pV + *pb * Mult };
			pMatrix->pNode->V = newV;
			pMatrix->pNode->Delta = newDelta;
			pMatrix->pNode->UpdateVreVimSuper();
		}
	}
	else
		throw dfw2error("CLoadFlow::RestoreVDelta called before StoreVDelta");
}

double CLoadFlow::GetSquaredImb()
{
	double Imb(0.0);
	for (_MatrixInfo* pMatrixInfo = pMatrixInfo_.get(); pMatrixInfo < pMatrixInfoEnd; pMatrixInfo++)
		Imb += ImbNorm(pMatrixInfo->ImbP, pMatrixInfo->ImbQ);
	return sqrt(Imb);
}

void CLoadFlow::CheckFeasible()
{
	bool bRes{ true };

	for (auto&& it : *pNodes)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(it) };
		if (pNode->IsStateOn())
		{
			if (!pNode->IsLFTypePQ())
			{
				// если узел входит в суперузел его заданное напряжение в расчете было равно заданному напряжению суперузла,
				// которое, в свою очередь, было выбрано по узлу с наиболее широким диапазоном реактивной мощности внутри суперузла
				double LFVref = pNode->pSuperNodeParent ? pNode->pSuperNodeParent->LFVref : pNode->LFVref;
				if (pNode->V > LFVref && pNode->Qgr >= pNode->LFQmax + Parameters.Imb)
				{
					pNode->Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("{} : V > Vref [{} > {}], Q >= Qmax [{} >= {}] ",
						pNode->GetVerbalName(),
						pNode->V,
						LFVref,
						pNode->Qgr,
						pNode->LFQmax
					));
					bRes = false;
				}
				if (pNode->V < LFVref && pNode->Qgr <= pNode->LFQmin - Parameters.Imb)
				{
					pNode->Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("{} : V < Vref [{} < {}], Q <= Qmin [{} <= {}]",
						pNode->GetVerbalName(),
						pNode->V,
						LFVref,
						pNode->Qgr,
						pNode->LFQmin
					));
					bRes = false;
				}
				if (pNode->Qgr < pNode->LFQmin - Parameters.Imb)
				{
					pNode->Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("{} Q < Qmin [{} < {}]",
						pNode->GetVerbalName(),
						pNode->Qgr,
						pNode->LFQmin));
					bRes = false;
				}
				if (pNode->Qgr > pNode->LFQmax + Parameters.Imb)
				{
					pNode->Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("{} Q > Qmax [{} > {}]",
						pNode->GetVerbalName(),
						pNode->Qgr,
						pNode->LFQmax));
					bRes = false;
				}
			}
			pNode->SuperNodeLoadFlow(pDynaModel);
		}
	}

	if (bRes)
	{
		CalculateBranchFlows();
		bRes = CheckNodeBalances();
	}

	if (!bRes)
		throw dfw2error(CDFW2Messages::m_cszUnacceptableLF);
}

void CLoadFlow::UpdateSlackBusesImbalance()
{
	// досчитываем небалансы в БУ
	for (_MatrixInfo* pMatrixInfo = pMatrixInfoEnd; pMatrixInfo < pMatrixInfoSlackEnd; pMatrixInfo++)
	{
		const auto& pNode{ pMatrixInfo->pNode };
		GetNodeImb(pMatrixInfo);
		// для БУ небалансы только для результатов
		pNode->Pgr += pMatrixInfo->ImbP;
		pNode->Qgr += pMatrixInfo->ImbQ;
		// в контроле сходимости небаланс БУ всегда 0.0
		pMatrixInfo->ImbP = pMatrixInfo->ImbQ = 0.0;
		pNodes->IterationControl().Update(pMatrixInfo);
	}
}


void CLoadFlow::CalculateBranchFlows()
{
	CDeviceContainer* pBranchContainer = pDynaModel->GetDeviceContainer(DEVTYPE_BRANCH);
	for (auto&& dev : *pBranchContainer)
	{
		const auto& pBranch{ static_cast<CDynaBranch*>(dev) };
		// пропускаем ветви с нулевым сопротивлением, для них потоки 
		// уже рассчитаны в SuperNodeLoadFlow
		if (pBranch->IsZeroImpedance()) continue;
		pBranch->Sb = pBranch->Se = { 0.0, 0.0 };
		// в отключенных ветвях потоки просто обнуляем
		if (pBranch->BranchState_ == CDynaBranch::BranchState::BRANCH_OFF) continue;
		cplx cIb, cIe, cSb, cSe;
		CDynaBranchMeasure::CalculateFlows(pBranch, cIb, cIe, pBranch->Sb, pBranch->Se);
	}
}

bool CLoadFlow::CheckNodeBalances()
{
	bool bRes{ true };
	cplx y;
	for (auto&& dev : pNodes->DevVec)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(dev) };
		// проводимость узла собираем из шунта узла и реакторов
		// вообще еще добавится шунт КЗ, но он при расчете УР равен нулю
		static_cast<CDynaNodeBase*>(dev)->GetGroundAdmittance(y);
		const auto imb{ pNode->V * pNode->V * y };
		pNode->dLRCGen = { pNode->Pnr - pNode->Pgr + imb.real() , pNode->Qnr - pNode->Qgr - imb.imag()};
	}

	const CDeviceContainer* pBranchContainer{ pDynaModel->GetDeviceContainer(DEVTYPE_BRANCH) };
	for (auto&& dev : *pBranchContainer)
	{
		const auto& pBranch{ static_cast<CDynaBranch*>(dev) };
		pBranch->pNodeIp_->dLRCGen -= pBranch->Sb;
		pBranch->pNodeIq_->dLRCGen += pBranch->Se;
	}

	for (const auto& dev : pNodes->DevVec)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(dev) };
		if (pNode->GetState() == eDEVICESTATE::DS_OFF) continue;
		if (std::abs(pNode->dLRCGen.real()) > Parameters.Imb ||
			std::abs(pNode->dLRCGen.imag()) > Parameters.Imb)
		{
			bRes = false;
			pNode->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszLFNodeImbalance,
				pNode->GetVerbalName(),
				cplx(pNode->dLRCGen.real(), pNode->dLRCGen.imag())));
		}
	}
	return bRes;
}

void CLoadFlow::DumpNewtonIterationControl() const
{

	const char* causes[] = {
							"",				// None
							"dV",			// Node voltage step too high
							"dD",			// Node angle step too high,
							"dB",			// Branch angle step too high
							"vV",			// Node voltage range violation
							"vD",			// Branch angle range violation
							"Bt"			// Newton backtrack
	};

	std::string objectName;
	switch (NewtonStepRatio.eStepCause)
	{
	case LFNewtonStepRatio::eStepLimitCause::Voltage:
	case LFNewtonStepRatio::eStepLimitCause::NodeAngle:
	case LFNewtonStepRatio::eStepLimitCause::VoltageOutOfRange:
		objectName = fmt::format("{}", NewtonStepRatio.pNode_->GetId());
		break;
	case LFNewtonStepRatio::eStepLimitCause::BrancheAngleOutOfRange:
	case LFNewtonStepRatio::eStepLimitCause::BranchAngle:
		objectName = fmt::format("{}-{}", 
			NewtonStepRatio.pNode_->GetId(),
			NewtonStepRatio.pNodeBranch_->GetId());
		break;
	}
	pDynaModel->Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("{} {:15f} {:6.2f} {:4} {}", pNodes->GetIterationControlString(),
		pNodes->IterationControl().ImbRatio,
		NewtonStepRatio.Ratio_,
		causes[static_cast<ptrdiff_t>(NewtonStepRatio.eStepCause)],
		objectName
		));
}

void CLoadFlow::LogNodeSwitch(_MatrixInfo* pNode, std::string_view Title)
{
	pDynaModel->Log(DFW2MessageStatus::DFW2LOG_DEBUG,
		fmt::format("{} {} V={:.3}, Vref={:.3}, Q={:.3}, Qrange[{:.3};{:.3}] switches done {}",
			Title,
			pNode->pNode->GetVerbalName(),
			pNode->pNode->V,
			pNode->pNode->LFVref,
			pNode->pNode->Qgr,
			pNode->pNode->LFQmin,
			pNode->pNode->LFQmax,
			pNode->PVSwitchCount));
}


void CLoadFlow::Gauss()
{
	KLUWrapper<std::complex<double>> klu;
	ptrdiff_t nNodeCount{ pMatrixInfoEnd - pMatrixInfo_.get() };
	size_t nBranchesCount{ pDynaModel->Branches.Count() };
	// оценка количества ненулевых элементов
	size_t nNzCount{ nNodeCount + 2 * nBranchesCount };

	klu.SetSize(nNodeCount, nNzCount);
	double* const Ax{ klu.Ax() };
	double* const B{ klu.B() };
	ptrdiff_t* Ap{ klu.Ai() };
	ptrdiff_t* Ai{ klu.Ap() };

	// вектор указателей на диагональ матрицы
	auto pDiags{ std::make_unique<double* []>(nNodeCount) };
	double** ppDiags{ pDiags.get() };
	double* pB{ B };

	double* pAx{ Ax };
	ptrdiff_t* pAp{ Ap };
	ptrdiff_t* pAi{ Ai };
	
	for (_MatrixInfo* pMatrixInfo = pMatrixInfo_.get(); pMatrixInfo < pMatrixInfoEnd; pMatrixInfo++)
	{
		_ASSERTE(pAx < Ax + nNzCount * 2);
		_ASSERTE(pAi < Ai + nNzCount);
		_ASSERTE(pAp < Ap + nNodeCount);
		const auto& pNode{ static_cast<CDynaNodeBase*>(pMatrixInfo->pNode) };

		*pAp = (pAx - Ax) / 2;    pAp++;
		*pAi = pNode->A(0) / 2;	  pAi++;
		// первый элемент строки используем под диагональ
		// и запоминаем указатель на него
		*ppDiags = pAx;
		*pAx = 1.0; pAx++;
		*pAx = 0.0; pAx++;
		ppDiags++;

		for (const auto& pv : pMatrixInfo->VirtualBranches())
		{
			// связи с базисным узлом и отключеннымив матрицу не входят
			if (NodeInMatrix(pv.pNode))
			{
				*pAx = pv.Y.real();   pAx++;
				*pAx = pv.Y.imag();   pAx++;
				*pAi = pv.pNode->A(0) / 2; pAi++;
			}
		}
	}

	nNzCount = (pAx - Ax) / 2;		// рассчитываем получившееся количество ненулевых элементов (делим на 2 потому что комплекс)
	Ap[nNodeCount] = nNzCount;

	size_t& nIteration{ pNodes->IterationControl().Number};

	for (nIteration = 0; nIteration < 20; nIteration++)
	{
		pNodes->IterationControl().Reset();

		ppDiags = pDiags.get();
		pB = B;

		// проходим по узлам вне зависимости от их состояния, параллельно идем по диагонали матрицы
		for (_MatrixInfo* pMatrixInfo = pMatrixInfo_.get(); pMatrixInfo < pMatrixInfoEnd; pMatrixInfo++)
		{
			CDynaNodeBase* pNode{ static_cast<CDynaNodeBase*>(pMatrixInfo->pNode) };
			GetNodeImb(pMatrixInfo);
			pNodes->IterationControl().Update(pMatrixInfo);

			_ASSERTE(pB < B + nNodeCount * 2);

			// для всех узлов которые включены и вне металлического КЗ

			cplx Unode(pNode->Vre, pNode->Vim);

			cplx I{ pNode->IconstSuper };
			cplx Y{ pNode->YiiSuper };		// диагональ матрицы по умолчанию собственная проводимость узла

			// рассчитываем задающий ток узла от нагрузки
			// можно посчитать ток, а можно посчитать добавку в диагональ
			I += std::conj(cplx(pNode->Pnr - pNode->Pgr - pMatrixInfo->UncontrolledP, pNode->Qnr - pNode->Qgr - pMatrixInfo->UncontrolledQ) / Unode);

			//if (pNode->V > 0.0)
			//	Y += std::conj(cplx(pNode->Pgr - pNode->Pnr, pNode->Qgr - pNode->Qnr) / pNode->V.Value / pNode->V.Value);
			//Y -= conj(cplx(Pnr, Qnr) / pNode->V / pNode->V);

			_CheckNumber(I.real());
			_CheckNumber(I.imag());
			_CheckNumber(Y.real());
			_CheckNumber(Y.imag());

			// и заполняем вектор комплексных токов
			*pB = I.real(); pB++;
			*pB = I.imag(); pB++;

			// диагональ матрицы формируем по Y узла
			**ppDiags = Y.real();
			*(*ppDiags + 1) = Y.imag();
			ppDiags++;
		}

		// добавляем токи от связей с базисными узлами в вектор токов
		for (_MatrixInfo* pMatrixInfo = pMatrixInfoEnd; pMatrixInfo < pMatrixInfoSlackEnd; pMatrixInfo++)
		{
			const auto& pNode{ pMatrixInfo->pNode };
			GetNodeImb(pMatrixInfo);
			// для БУ небалансы только для результатов
			pNode->Pgr += pMatrixInfo->ImbP;
			pNode->Qgr += pMatrixInfo->ImbQ;

			for (const auto& pv : pMatrixInfo->VirtualBranches())
			{
				for (const auto& pvb : pv.pNode->VirtualBranches())
				{
					if (pvb.pNode == pNode)
					{
						cplx Ix{ -pNode->VreVim * pvb.Y };
						klu.B()[pv.pNode->A(0)] += Ix.real();
						klu.B()[pv.pNode->A(0) + 1] += Ix.imag();
					}
				}
			}
		}

		pNodes->DumpIterationControl();
		klu.FactorSolve();

		// KLU может делать повторную факторизацию матрицы с начальным пивотингом
		// это быстро, но при изменении пивотов может вызывать численную неустойчивость.
		// У нас есть два варианта факторизации/рефакторизации на итерации LULF
		double* pB = B;

		for (_MatrixInfo* pMatrixInfo = pMatrixInfo_.get(); pMatrixInfo < pMatrixInfoEnd; pMatrixInfo++)
		{
			const auto& pNode{ static_cast<CDynaNodeBase*>(pMatrixInfo->pNode) };
			// напряжение после решения системы в векторе задающий токов
			pNode->Vre = *pB;		pB++;
			pNode->Vim = *pB;		pB++;
			pNode->UpdateVDeltaSuper();
		
		}
	}
}

// Выполняет оценку начального приближения УР "Z0"-методом
// Аюев, Давыдов, Ерохин, Неуймин, Khokhlov. 
// (2021).Исследование стартовых алгоритмов расчета установившихся режимов 
// электрических систем методом Ньютона.Известия Российской академии наук.Энергетика. 109 - 121. 10.31857 / 
// DOI: 10.31857/S0002331021010039
// Идея - поставить нулевые углы по базисным во всех PV узлах 
// (что по островам не очень понятно) и рассчитать YU=I для холостого хода

void CLoadFlow::Z0()
{
	// Z0-оценка ставит нулевые углы в PV-узлах
	// и сохраняет модули. Поэтому это по-сути и есть плоский старт.
	// Если мы не хотим с него считать - то Z0-оценку лучше не использовать.
	if (!Parameters.Flat)
	{
		pDynaModel->Log(DFW2MessageStatus::DFW2LOG_MESSAGE, CDFW2Messages::m_cszLFZ0isForFlatOnly);
		return;
	}
	else
		pDynaModel->Log(DFW2MessageStatus::DFW2LOG_INFO, CDFW2Messages::m_cszLFRunningZ0);
		
	KLUWrapper<std::complex<double>> klu;

	// в матрице участвуют только PQ-узлы
	std::vector<_MatrixInfo*> PQnodes;
	PQnodes.reserve(pMatrixInfoEnd - pMatrixInfo_.get());

	for (auto node{ pMatrixInfo_.get() }; node < pMatrixInfoEnd; node++)
		if (node->pNode->eLFNodeType_ == CDynaNodeBase::eLFNodeType::LFNT_PQ)
		{
			// связываем PQ-узлы со строками матрицы
			node->pNode->SetMatrixRow(PQnodes.size());
			// добавляем PQ-узел в список
			PQnodes.push_back(node);
		}

	// PQ-узлов нет
	if (PQnodes.empty())
		return;

	const size_t nBranchesCount{ pDynaModel->Branches.Count() };
	// оценка количества ненулевых элементов
	size_t nNzCount{ PQnodes.size() + 2 * nBranchesCount};

	klu.SetSize(PQnodes.size(), nNzCount);

	double* const Ax{ klu.Ax() };
	double* const B{ klu.B() };
	ptrdiff_t* Ap{ klu.Ai() };
	ptrdiff_t* Ai{ klu.Ap() };

	// вектор указателей на диагональ матрицы
	auto pDiags{ std::make_unique<double* []>(PQnodes.size()) };
	double** ppDiags{ pDiags.get() };
	double* pB{ B };

	double* pAx{ Ax };
	ptrdiff_t* pAp{ Ap };
	ptrdiff_t* pAi{ Ai };

	for (const auto& mx : PQnodes)
	{
		// проходим по PQ-узлам
		const auto& pNode{ static_cast<CDynaNodeBase*>(mx->pNode) };
		*pAp = (pAx - Ax) / 2;    pAp++;
		*pAi = pNode->A(0);		  pAi++;
		// первый элемент строки используем под диагональ
		// и запоминаем указатель на него
		*ppDiags = pAx;
		*pAx = 1.0; pAx++;
		*pAx = 0.0; pAx++;
		ppDiags++;

		// формируем внедиагонгональные элементы
		for (const auto& pv : mx->VirtualBranches())
		{
			if (NodeInMatrix(pv.pNode) && pv.pNode->eLFNodeType_ == CDynaNodeBase::eLFNodeType::LFNT_PQ)
			{
				*pAx = pv.Y.real();    pAx++;
				*pAx = pv.Y.imag();    pAx++;
				*pAi = pv.pNode->A(0); pAi++;
			}
		}
	}
	
	nNzCount = (pAx - Ax) / 2;		// рассчитываем получившееся количество ненулевых элементов (делим на 2 потому что комплекс)
	Ap[PQnodes.size()] = nNzCount;

	size_t& nIteration{ pNodes->IterationControl().Number };

	for (nIteration = 0; nIteration < 1; nIteration++)
	{
		pNodes->IterationControl().Reset();

		ppDiags = pDiags.get();
		pB = B;

		// для PQ-узлов
		for (const auto& mx : PQnodes)
		{
			CDynaNodeBase* pNode{ static_cast<CDynaNodeBase*>(mx->pNode) };
			// рассчитываем небаланс и собираем статистику
			GetNodeImb(mx);
			pNodes->IterationControl().Update(mx);
			//cplx Unode(pNode->Vre, pNode->Vim);

			// учитываем ток суперузла
			cplx I{ pNode->IconstSuper };
			// диагональ матрицы по умолчанию собственная проводимость узла
			cplx Y{ pNode->YiiSuper };		
			// заполняем диагональ матрицы
			**ppDiags = Y.real();
			*(*ppDiags + 1) = Y.imag();
			ppDiags++;

			// собираем токи от инцидентных узлов. Нагрузку не учитываем
			for (const auto& pv : mx->VirtualBranches())
			{
				if (pv.pNode->eLFNodeType_ != CDynaNodeBase::eLFNodeType::LFNT_PQ)
					I -= pv.pNode->VreVim * pv.Y;
			}

			*pB = I.real(); pB++;
			*pB = I.imag(); pB++;
		}

		// рассчитываем YU=I
		pNodes->DumpIterationControl();
		klu.FactorSolve();

		// KLU может делать повторную факторизацию матрицы с начальным пивотингом
		// это быстро, но при изменении пивотов может вызывать численную неустойчивость.
		// У нас есть два варианта факторизации/рефакторизации на итерации LULF
		double* pB = B;

		// обновляем напряжения в PQ-узлах
		for (const auto& mx : PQnodes)
		{
			const auto& pNode{ static_cast<CDynaNodeBase*>(mx->pNode) };
			pNode->Vre = *pB;		pB++;
			pNode->Vim = *pB;		pB++;
			pNode->UpdateVDeltaSuper();
		}
	}

	// восстанавливаем привязку узлов к строкам полной матрицы после привязки
	// матрице PQ-узлов
	for (auto node{ pMatrixInfo_.get() }; node < pMatrixInfoEnd; node++)
		node->pNode->SetMatrixRow((node - pMatrixInfo_.get()) * 2);
}


CLoadFlow::Limits::Limits(CLoadFlow& LoadFlow) :
	LoadFlow_(LoadFlow)
{
	const size_t PlannedSize(LoadFlow_.klu.MatrixSize() / 2);
	PV_PQmax.reserve(PlannedSize);
	PV_PQmin.reserve(PlannedSize);
	PQmax_PV.reserve(PlannedSize);
	PQmin_PV.reserve(PlannedSize);
}

void CLoadFlow::Limits::Clear()
{
	PV_PQmax.clear();
	PV_PQmin.clear();
	PQmax_PV.clear();
	PQmin_PV.clear();
}

void CLoadFlow::Limits::Select(_MatrixInfo* pMatrixInfo)
{
	const auto& pNode{ pMatrixInfo->pNode };
	const auto& Parameters{ LoadFlow_.Parameters };

	// получаем расчетную генерацию узла
	const double Qg{ pNode->Qgr + pMatrixInfo->ImbQ };

	switch (pNode->eLFNodeType_)
	{
		// если узел на минимуме Q и напряжение ниже уставки, он должен стать PV
	case CDynaNodeBase::eLFNodeType::LFNT_PVQMIN:
		if (pMatrixInfo->NodeVoltageViolation() < -Parameters.Vdifference)
		{
			if (pMatrixInfo->PVSwitchCount < Parameters.MaxPVPQSwitches)
				PQmin_PV.push_back(pMatrixInfo);
			else
				FailedPVPQNodes.insert(pNode);
		}
		break;
		// если узел на максимуме Q и напряжение выше уставки, он должен стать PV
	case CDynaNodeBase::eLFNodeType::LFNT_PVQMAX:
		if (pMatrixInfo->NodeVoltageViolation() > Parameters.Vdifference)
		{
			if (pMatrixInfo->PVSwitchCount < Parameters.MaxPVPQSwitches)
				PQmax_PV.push_back(pMatrixInfo);
			else
				FailedPVPQNodes.insert(pNode);
		}
		break;
		// если узел PV, но реактивная генерация вне диапазона, делаем его PQ
	case CDynaNodeBase::eLFNodeType::LFNT_PV:
		pNode->Qgr = Qg;	// если реактивная генерация в пределах - обновляем ее значение в узле
		// рассчитываем нарушение ограничения по генерации реактивной мощности
		if ((pMatrixInfo->NodePowerViolation_ = Qg - pNode->LFQmax) > Parameters.Imb)
			PV_PQmax.push_back(pMatrixInfo);
		else if ((pMatrixInfo->NodePowerViolation_ = Qg - pNode->LFQmin) < -Parameters.Imb)
			PV_PQmin.push_back(pMatrixInfo);
		break;
	}
}

size_t CLoadFlow::Limits::ViolatedCount() const
{
	return PV_PQmax.size() + PV_PQmin.size() + PQmax_PV.size() + PQmin_PV.size();
};

void CLoadFlow::Limits::Apply()
{
	const auto& pNodes{ LoadFlow_.pNodes };
	const auto& Parameters{ LoadFlow_.Parameters };

	// сортируем узлы с ограничениями по величине нарушения

	std::sort(PQmin_PV.begin(), PQmin_PV.end(), [](const _MatrixInfo* lhs, const _MatrixInfo* rhs)
		{
			return std::abs(lhs->NodeVoltageViolation_) > std::abs(rhs->NodeVoltageViolation_);
		});

	std::sort(PQmax_PV.begin(), PQmax_PV.end(), [](const _MatrixInfo* lhs, const _MatrixInfo* rhs)
		{
			return std::abs(lhs->NodeVoltageViolation_) > std::abs(rhs->NodeVoltageViolation_);
		});

	std::sort(PV_PQmax.begin(), PV_PQmax.end(), [](const _MatrixInfo* lhs, const _MatrixInfo* rhs)
		{
			return std::abs(lhs->NodePowerViolation_) > std::abs(rhs->NodePowerViolation_);
		});

	std::sort(PV_PQmin.begin(), PV_PQmin.end(), [](const _MatrixInfo* lhs, const _MatrixInfo* rhs)
		{
			return std::abs(lhs->NodePowerViolation_) > std::abs(rhs->NodePowerViolation_);
		});

	// обрезаем векторы узлов с ограничениями по допустимой размерности
	// равной количеству переключений на итерации

	if(Parameters.PVPQSwitchPerIt)
		for (auto&& vec : std::array<MATRIXINFO*, 4>{&PQmin_PV, & PQmax_PV, & PV_PQmax, & PV_PQmin})
			if (vec->size() > Parameters.PVPQSwitchPerIt)
				vec->resize(Parameters.PVPQSwitchPerIt);

	// функция возврата узла из ограничения Q в PV
	const auto fnToPv = [this](auto& node, std::string_view Title)
	{
		node->PVSwitchCount++;
		CDynaNodeBase*& pNode = node->pNode;
		LoadFlow_.LogNodeSwitch(node, Title);
		pNode->eLFNodeType_ = CDynaNodeBase::eLFNodeType::LFNT_PV;
		pNode->V = pNode->LFVref;
		pNode->UpdateVreVimSuper();
		// считаем количество переключений типов узлов
		// чтобы исключить выбор шага по g1 > g0
		LoadFlow_.NodeTypeSwitchesDone++;
	};

	// запоминаем были ли переключения узлов на предыдущей итерации
	const bool PreviousSwitches{ LoadFlow_.NodeTypeSwitchesDone > 0 && false};
	// сбрасываем количество переключений узлов
	LoadFlow_.NodeTypeSwitchesDone = 0;
	const double absImbRatio{ std::abs(pNodes->IterationControl().ImbRatio) };

	// Сначала снимаем узлы с минимумов Q, чтобы избежать
	// дефицита реактивной мощности

	for (auto&& it : PQmin_PV)
		fnToPv(it, "PQmin->PV");

	// если узлов на минимуме нет - снимаем узлы с максимумов Q
	if (PQmin_PV.empty())
		for (auto&& it : PQmax_PV)
			fnToPv(it, "PQmax->PV");

	// Разрешаем переключать узлы PV в случае, если 
	// 1. небаланс по P снизился до заданного порога
	// 2. снижение небаланса относительно мало
	// 3. шаг Ньютона ограничен до уровня m_Parameters.ForceSwitchLambda из-за бэктрэка

	if ((std::abs(pNodes->IterationControl().MaxImbP.GetDiff()) < 100.0 * Parameters.Imb &&
		 std::abs(pNodes->IterationControl().MaxImbQ.GetDiff()) < 100.0 * Parameters.Imb) ||
		(absImbRatio > 0.95 && !PreviousSwitches) ||
		std::abs(LoadFlow_.lambda_) <= Parameters.ForceSwitchLambda)
	{
		for (auto&& it : PV_PQmax)
		{
			LoadFlow_.LogNodeSwitch(it, "PV->PQmax");
			CDynaNodeBase*& pNode = it->pNode;
			pNode->eLFNodeType_ = CDynaNodeBase::eLFNodeType::LFNT_PVQMAX;
			pNode->Qgr = LoadFlow_.NodeLFQmax(pNode).Q;
			LoadFlow_.NodeTypeSwitchesDone++;
		}

		if (PV_PQmax.empty())
		{
			for (auto&& it : PV_PQmin)
			{
				LoadFlow_.LogNodeSwitch(it, "PV->PQmin");
				CDynaNodeBase*& pNode = it->pNode;
				pNode->eLFNodeType_ = CDynaNodeBase::eLFNodeType::LFNT_PVQMIN;
				pNode->Qgr = LoadFlow_.NodeLFQmin(pNode).Q;
				LoadFlow_.NodeTypeSwitchesDone++;
			}
		}
	}
}

void CLoadFlow::Limits::CheckFeasible()
{
	for (const auto& it : FailedPVPQNodes)
		it->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszLFOverswitchedNode, it->GetVerbalName(),
			LoadFlow_.Parameters.MaxPVPQSwitches));
}

const ControlledLimit& CLoadFlow::NodeLFQmin(CDynaNodeBase* pNode)
{
	auto& Matrix{ pMatrixInfo_[pNode->A(0) / 2] };
	auto& Controllers{ Matrix.LimitControllers_ };
	auto& cl{ Matrix.ControlledLimit_ };
	cl = {};
	if (Controllers.empty())
	{
		cl.Q = pNode->LFQmin;
		cl.dIdV = -cl.Q / pNode->V / pNode->V;
		return cl;
	}
	for (auto&& controller : Controllers)
	{
		CDynaSVCBase* pSVC{ static_cast<CDynaSVCBase*>(controller) };
		// оценка напряжения в управляемом узле
		const double Voriginal{ pNode->V / (1.0 - pSVC->Bmax_ * pSVC->xsl_) };
		cl.Q += -pSVC->Bmax_ * Voriginal * pNode->V;
		cl.dQdV += -2.0 * pSVC->Bmax_ * Voriginal;
		cl.dIdV += -pSVC->Bmax_ * Voriginal / pNode->V;
	}
	return cl;
}

const ControlledLimit& CLoadFlow::NodeLFQmax(CDynaNodeBase* pNode)
{
	auto& Matrix{ pMatrixInfo_[pNode->A(0) / 2] };
	auto& Controllers{ Matrix.LimitControllers_ };
	auto& cl{ Matrix.ControlledLimit_ };
	cl = {};
	if (Controllers.empty())
	{
		cl.Q = pNode->LFQmax;
		cl.dIdV = -cl.Q / pNode->V / pNode->V;
		return cl;
	}
	for (auto&& controller : Controllers)
	{
		CDynaSVCBase* pSVC{ static_cast<CDynaSVCBase*>(controller) };
		// оценка напряжения в управляемом узле
		const double Voriginal{ pNode->V / (1.0 - pSVC->Bmin_ * pSVC->xsl_) };
		cl.Q += -pSVC->Bmin_ * Voriginal * pNode->V;
		cl.dQdV += -2.0 * pSVC->Bmin_ * Voriginal;
		cl.dIdV += -pSVC->Bmin_ * Voriginal / pNode->V;
	}
	return cl;
}


void CLoadFlow::CExtraNodes::CreateSVCNodes()
{
	// создаем нужное количество узлов
	CDynaNodeBase::DeviceProperties(Nodes_.ContainerProps());
	Nodes_.CreateDevices(LoadFlow_.SVCs_.size());
	// создаем пустые ссылки для суперузлов нового контейнера
	Nodes_.SuperLinks.emplace_back(&Nodes_, Nodes_.Count());

	// ищем номер узла, с которого пронумеруем новые узлы
	ptrdiff_t NewNodesId{ 0 };
	for (const auto& originalNode : *OriginalNodes_)
		NewNodesId = std::max(NewNodesId, originalNode->GetId());

	auto svc{ LoadFlow_.SVCs_.begin() };
	
	// собираем все данные в двух измерениях
	// 1 - список УШР с привязанными узлами: искусственным и управляемыми
	// 2 - карта управляемых узлов, с привязанными списками УШР и искусственных узлов
	for (auto&& newNode : Nodes_)
	{
		newNode->SetId(++NewNodesId);
		newNode->SetName(fmt::format("SVC {} ref ", svc->SVC->GetVerbalName()));
		svc->OriginalNode = static_cast<CDynaNodeBase*>(svc->SVC->GetSingleLink(DEVTYPE_NODE));
		// искусственный узел УШР
		svc->ExtraNode = static_cast<CDynaNodeBase*>(newNode);
		// тип - PV
		svc->ExtraNode->eLFNodeType_ = CDynaNodeBase::eLFNodeType::LFNT_PV;
		// уставка по напряжению
		svc->ExtraNode->LFVref = svc->ExtraNode->Unom = svc->SVC->Vref_;
		// задаем ограничения
		const double vmax{ svc->SVC->Vref_ / (1.0 - svc->SVC->Bmax_ * svc->SVC->xsl_) };
		const double vmin{ svc->SVC->Vref_ / (1.0 - svc->SVC->Bmin_ * svc->SVC->xsl_) };
		
		svc->ExtraNode->LFQmax = -svc->SVC->Bmin_ * svc->SVC->Vref_ * vmin;
		svc->ExtraNode->LFQmin = -svc->SVC->Bmax_ * svc->SVC->Vref_ * vmax;
		 
		// включаем искусственный узел в синхронную зону
		// управляемого узла
		svc->ExtraNode->pSyncZone = svc->OriginalNode->pSyncZone;
		// добавляем управляемый узел в карту
		OriginalNodeSet_[svc->OriginalNode].emplace_back(&*svc);
		svc++;
	}

	// определяем необходимое количество дополнительных виртуальных ветвей
	// мы заменяем виртуальные ветви исходных узлов новыми ветвями
	// к которым будут добавлены ветви от искусственных узлов
	// поэтому нужное количество ветвей равно сумме существующих ветвей
	// управляемых узлов с добавлением двух дополнительных: к искусственному
	// узлу и к исходному узлу
	ptrdiff_t ExtraVirtualBranchCount{ 0 };
	for(const auto& originalNode : OriginalNodeSet_)
		ExtraVirtualBranchCount += originalNode.first->VirtualBranches().size() + 2 * originalNode.second.size();
	Branches_ = std::make_unique<VirtualBranch[]>(ExtraVirtualBranchCount);
}

void CLoadFlow::CExtraNodes::UpdateVirtualBranches()
{
	// всю работу с ветвями делаем не из узлов, а из матрицы
	// что позволяет заменить ветви из узлов новыми, в которых 
	// есть дополнительные ветви для искусственных узлов
	for (_MatrixInfo* pMatrixInfo{ LoadFlow_.pMatrixInfo_.get() }; pMatrixInfo < LoadFlow_.pMatrixInfoSlackEnd; pMatrixInfo++)
	{
		// копируем диапазоны ветвей из узлов в матрицу
		pMatrixInfo->pVirtualBranchBegin_ = pMatrixInfo->pNode->pVirtualBranchBegin_;
		pMatrixInfo->pVirtualBranchEnd_ = pMatrixInfo->pNode->pVirtualBranchEnd_;
	}

	VirtualBranch* pVb{ Branches_.get() };

	// сначала добавляем виртуальные ветви к исходным узлам
	for (auto&& originalNode : OriginalNodeSet_)
	{
		// каждый исходный узел проходится один раз, несмотря на то
		// что к нему может быть подключено более одного искусственного
		_MatrixInfo& OriginalNodeMatrix{ LoadFlow_.pMatrixInfo_[originalNode.first->A(0) / 2] };
		// копируем исходные ветви оригинального узла в новый буфер ветвей
		std::copy(originalNode.first->pVirtualBranchBegin_, originalNode.first->pVirtualBranchEnd_, pVb);
		OriginalNodeMatrix.pVirtualBranchBegin_ = pVb;
		pVb += originalNode.first->VirtualBranches().size();

		// добавляем новые виртуальные ветви до каждого искусственного узда
		for (const auto& extraNode : originalNode.second)
		{
			const auto y{ 1.0 / cplx(0.0, extraNode->SVC->xsl_) };
			pVb->pNode = extraNode->ExtraNode;
			pVb->Y = y;
			pVb++;
			originalNode.first->YiiSuper -= y;
		}
		OriginalNodeMatrix.pVirtualBranchEnd_ = pVb;
	}

	// далее формируем ветви от искусственных узлов к исходным
	for (const auto& svc : LoadFlow_.SVCs_)
	{
		const auto y{ 1.0 / cplx(0.0, svc.SVC->xsl_) };
		_MatrixInfo& ExtraNodeMatrix{ LoadFlow_.pMatrixInfo_[svc.ExtraNode->A(0) / 2] };
		ExtraNodeMatrix.pVirtualBranchBegin_ = svc.ExtraNode->pVirtualBranchBegin_ = pVb;
		ExtraNodeMatrix.pVirtualBranchEnd_ = svc.ExtraNode->pVirtualBranchEnd_ = pVb + 1;
		pVb->pNode = svc.OriginalNode;
		pVb->Y = y;
		pVb++;
		svc.ExtraNode->YiiSuper = -y;
	}
}

_MatrixInfo* CLoadFlow::CExtraNodes::UpdateDimensions(_MatrixInfo* pMatrixInfo)
{
	for(auto&& svc : LoadFlow_.SVCs_)
	{
		CDynaNodeBase* pNode{ static_cast<CDynaNodeBase*>(svc.ExtraNode) };
		pNode->SetMatrixRow(LoadFlow_.MatrixSize_);

		pMatrixInfo->Store(pNode);
		if (NodeInMatrix(svc.OriginalNode))
			pMatrixInfo->nRowCount += 2;	// 2 элемента для ветви до УШР

		pMatrixInfo->LimitControllers_.emplace_back(svc.SVC);

		LoadFlow_.MatrixSize_ += 2;		// 2 уравнения на узел УШР
		pMatrixInfo->nRowCount += 2;	// считаем диагональный элемент
		LoadFlow_.NonZeroCount_ += 2 * pMatrixInfo->nRowCount;	// количество ненулевых элементов увеличиваем на количество подсчитанных элементов в строке (4 double на элемент)
		pMatrixInfo++;

		_MatrixInfo& OriginalNodeMatrix{ LoadFlow_.pMatrixInfo_[svc.OriginalNode->A(0) / 2] };
		OriginalNodeMatrix.nRowCount += 2;	// добавляем два элемента в строку оригинального узла
		LoadFlow_.NonZeroCount_ += 4; // и 4 элемента в ненулевые

	}
	return pMatrixInfo;
}


void CLoadFlow::CExtraNodes::Restore()
{
	for (const auto& svc : LoadFlow_.SVCs_)
	{
		// номер исходного узла УШР (ссылка могла быть заменена на суперузел)
		const auto SVCNodeId{ static_cast<ptrdiff_t>(svc.SVC->NodeId) };
		// проходим по ветвям искусственного узла (она одна)
		for (const auto& vb : svc.ExtraNode->VirtualBranches())
		{
			// ток от искусственного узла к контролируемому с учетом
			// собственной проводимости
			const cplx Ig{ svc.ExtraNode->VreVim * vb.Y - vb.pNode->VreVim * vb.Y };
			const cplx Sg{ vb.pNode->VreVim * std::conj(Ig) };
			svc.SVC->Q = -Sg.imag();

			// по умолчанию отдаем мощность УШР в узел, к которому ведет ссылка
			auto SVCNode{ svc.OriginalNode };
			// проверяем что номер узла, к которому подключен УШР
			// совпадает с номером узла, к которому ведет ссылка
			if (SVCNode->GetId() != SVCNodeId)
			{
				// если это не так - находим узел по номеру
				// из множества узлов суперузла
				const CLinkPtrCount* const pNodeLink{ SVCNode->GetSuperLink(0) };
				LinkWalker<CDynaNodeBase> pSlaveNode;
				while (pNodeLink->In(pSlaveNode))
					if (pSlaveNode->GetId() == SVCNodeId)
					{
						SVCNode = pSlaveNode;
						break;
					}
			}

			// переносим мощность в генерацию узла
			SVCNode->Qg = (SVCNode->Qgr -= svc.SVC->Q);
			// восстанавливаем собственную проводимость
			// контролируемого узла (корректировали в AllocateSVCs())
			vb.pNode->YiiSuper += vb.Y;
		}
	}

	for (_MatrixInfo* pMatrixInfo = LoadFlow_.pMatrixInfo_.get(); pMatrixInfo < LoadFlow_.pMatrixInfoSlackEnd; pMatrixInfo++)
	{
		pMatrixInfo->pVirtualBranchBegin_ = pMatrixInfo->pNode->pVirtualBranchBegin_;
		pMatrixInfo->pVirtualBranchEnd_ = pMatrixInfo->pNode->pVirtualBranchEnd_;
	}
}


SerializerPtr CLoadFlow::LoadFlowParameters::GetSerializer()
{
	SerializerPtr Serializer{ std::make_unique<CSerializerBase>(new CSerializerDataSourceBase()) };
	Serializer->SetClassName("Parameters");

	Serializer->AddProperty(m_cszLFImbalance, Imb);
	Serializer->AddProperty(m_cszLFFlat, Flat);

	Serializer->AddEnumProperty(m_cszLFStartup,
		new CSerializerAdapterEnum<CLoadFlow::eLoadFlowStartupMethod>(Startup, m_cszLFStartupNames));

	Serializer->AddProperty(m_cszLFSeidellStep, SeidellStep);
	Serializer->AddProperty(m_cszLFSeidellIterations, SeidellIterations);
	Serializer->AddProperty(m_cszLFEnableSwitchIteration, EnableSwitchIteration);
	Serializer->AddProperty(m_cszLFMaxIterations, MaxIterations);
	Serializer->AddProperty(m_cszLFNewtonMaxVoltageStep, VoltageNewtonStep);
	Serializer->AddProperty(m_cszLFNewtonMaxNodeAngleStep, NodeAngleNewtonStep);
	Serializer->AddProperty(m_cszLFNewtonMaxBranchAngleStep, BranchAngleNewtonStep);
	Serializer->AddProperty(m_cszLFForceSwitchLambda, ForceSwitchLambda);
	Serializer->AddEnumProperty(m_cszLFFormulation,
		new CSerializerAdapterEnum<CLoadFlow::eLoadFlowFormulation>(LFFormulation, m_cszLFFormulationTypeNames));
	Serializer->AddProperty(m_cszLFAllowNegativeLRC, AllowNegativeLRC);
	Serializer->AddProperty(m_cszLFLRCMinSlope, LRCMinSlope);
	Serializer->AddProperty(m_cszLFLRCMaxSlope, LRCMaxSlope);
	Serializer->AddProperty(m_cszMaxPVPQSwitches, MaxPVPQSwitches);
	Serializer->AddProperty(m_cszPVPQSwitchPerIt, PVPQSwitchPerIt);

	return Serializer;
}

SerializerValidatorRulesPtr CLoadFlow::LoadFlowParameters::GetValidator() const
{
	auto Validator{ std::make_unique<CSerializerValidatorRules>() };
	Validator->AddRule(
		{ 
			m_cszLFImbalance, 
			m_cszLFMaxIterations,
			m_cszLFNewtonMaxVoltageStep,
			m_cszLFSeidellIterations,
			m_cszLFNewtonMaxNodeAngleStep,
			m_cszLFNewtonMaxBranchAngleStep,
			m_cszLFForceSwitchLambda,
			m_cszLFSeidellStep,
			m_cszLFLRCMaxSlope,
			m_cszMaxPVPQSwitches,
		},
		&CSerializerValidatorRules::BiggerThanZero);

	Validator->AddRule(m_cszLFLRCMinSlope, &MinLRCSLopeValidator);
	Validator->AddRule(m_cszLFEnableSwitchIteration, &CSerializerValidatorRules::NonNegative);
	Validator->AddRule(m_cszPVPQSwitchPerIt, &CSerializerValidatorRules::NonNegative);
	return Validator;
}

