﻿#include "stdafx.h"
#include <algorithm>
#include "LoadFlow.h"
#include "DynaModel.h"
#include "DynaPowerInjector.h"
#include "limits"
#include "BranchMeasures.h"
#include "MathUtils.h"

using namespace DFW2;

CLoadFlow::CLoadFlow(CDynaModel* pDynaModel) : m_pDynaModel(pDynaModel), m_Parameters(pDynaModel->Parameters()) {}

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
	ptrdiff_t nMatrixSize(0), nNonZeroCount(0);
	// создаем привязку узлов к информации по строкам матрицы
	// размер берем по количеству суперузлов. Реальный размер матрицы будет меньше на
	// количество отключенных узлов и БУ
	m_pMatrixInfo = std::make_unique<_MatrixInfo[]>(pNodes->Count());
	_MatrixInfo* pMatrixInfo = m_pMatrixInfo.get();

	// базисные узлы держим в отдельном списке, так как они не в матрице, но
	// результаты по ним нужно обновлять
	std::list<CDynaNodeBase*> SlackBuses;

	for (auto&& it : pNodes->m_DevVec)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(it) };
		// обрабатываем только включенные узлы и узлы без родительского суперузла
		if (!pNode->IsStateOn() || pNode->m_pSuperNodeParent)
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
			for (VirtualBranch* pV = pNode->m_VirtualBranchBegin; pV < pNode->m_VirtualBranchEnd; pV++)
				if (NodeInMatrix(pV->pNode))
					pMatrixInfo->nRowCount += 2;	// количество ненулевых элементов = ветвей - ветвей на БУ

			// для узлов, которые попадают в матрицу нумеруем включенные узлы строками матрицы
			pNode->SetMatrixRow(nMatrixSize);
			pMatrixInfo->Store(pNode);
			nMatrixSize += 2;				// на узел 2 уравнения в матрице
			pMatrixInfo->nRowCount += 2;	// считаем диагональный элемент
			nNonZeroCount += 2 * pMatrixInfo->nRowCount;	// количество ненулевых элементов увеличиваем на количество подсчитанных элементов в строке (4 double на элемент)
			pMatrixInfo++;
		}
	}

	if (!nMatrixSize)
		throw dfw2error(CDFW2Messages::m_cszNoNodesForLF);

	m_pMatrixInfoEnd = pMatrixInfo;								// конец инфо по матрице для узлов, которые в матрице
	klu.SetSize(nMatrixSize, nNonZeroCount);
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

	m_pMatrixInfoSlackEnd = pMatrixInfo;
	m_Rh = std::make_unique<double[]>(klu.MatrixSize());		// невязки до итерации
}

void CLoadFlow::Estimate()
{
	ptrdiff_t* pAi = klu.Ai();
	ptrdiff_t* pAp = klu.Ap();
	ptrdiff_t nRowPtr = 0;

	_MatrixInfo* pMatrixInfo = m_pMatrixInfo.get();
	for (; pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
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

		for (VirtualBranch* pV = pNode->m_VirtualBranchBegin; pV < pNode->m_VirtualBranchEnd; pV++)
		{
			CDynaNodeBase*& pOppNode(pV->pNode);
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
	CDeviceContainer* pBranchContainer = m_pDynaModel->GetDeviceContainer(DEVTYPE_BRANCH);
	m_BranchAngleCheck.reserve(pBranchContainer->Count());
	for (auto&& it : *pBranchContainer)
	{
		const auto& pBranch{ static_cast<CDynaBranch*>(it) };
		if (pBranch->m_BranchState == CDynaBranch::BranchState::BRANCH_ON && !pBranch->InSuperNode())
			m_BranchAngleCheck.push_back(pBranch);
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
				m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PV;
				if (bFlatStart)
				{
					// если требуется плоский старт
					V = LFVref;										// напряжение задаем равным Vref
					Qgr = LFQmin + (LFQmax - LFQmin) / 2.0;			// реактивную мощность ставим в середину диапазона
					Delta = 0.0;
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
					m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMAX;
				}
				else if (LFQmin + ImbTol >= Qgr)
				{
					Qgr = LFQmin;
					m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMIN;
				}
				else
					V = LFVref; // если реактивная мощность в диапазоне - напряжение равно уставке
			}
			else
			{
				// для PQ-узлов на плоском старте ставим напряжение равным номинальному
				m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PQ;
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
	pNodes->CalculateSuperNodesAdmittances(m_Parameters.m_Startup == CLoadFlow::eLoadFlowStartupMethod::Seidell);

	// в режиме отладки запоминаем что было в узлах после расчета Rastr для сравнения результатов
#ifdef _DEBUG
	for (auto&& it : *pNodes)
		static_cast<CDynaNodeBase*>(it)->GrabRastrResult();
#endif

	// обновляем данные в PV-узлах по заданным в генераторах реактивным мощностям
	UpdatePQFromGenerators();

	// инициализируем все узлы, чтобы корректно определить их тип
	for (auto&& it : *pNodes)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(it) };
		pNode->Pgr = pNode->Pg;
		pNode->Qgr = pNode->Qg;
		pNode->StartLF(m_Parameters.m_bFlat, m_Parameters.m_Imb);
	}

	// выбираем узлы, которые войдут в матрицу (все суперузлы), а так же базисные суперузлы.
	AllocateSupernodes();

	for (_MatrixInfo* pMatrixInfo = m_pMatrixInfo.get(); pMatrixInfo < m_pMatrixInfoSlackEnd; pMatrixInfo++)
	{
		const auto& pNode{ pMatrixInfo->pNode };
		const CLinkPtrCount* const pNodeLink{ pNode->GetSuperLink(0) };
		LinkWalker<CDynaNodeBase> pSlaveNode;
		double QrangeMax{ pNode->LFQmax - pNode->LFQmin };

		while (pNodeLink->In(pSlaveNode))
		{
			// в суперузел суммируем все мощности входящих узлов
			pNode->Pgr += pSlaveNode->Pgr;
			pNode->Qgr += pSlaveNode->Qgr;

			switch (pSlaveNode->m_eLFNodeType)
			{
			case CDynaNodeBase::eLFNodeType::LFNT_BASE:
				// если в суперузле базисный узел - узел представитель тоже должен быть
				// базисным, потому что так должны строиться суперузлы в CreateSuperNodes
				_ASSERTE(pNode->m_eLFNodeType == pSlaveNode->m_eLFNodeType);
				break;
			case CDynaNodeBase::eLFNodeType::LFNT_PQ:
				// если в суперузел входит нагрузочный - учитываем его генерацию как неуправляемую
				pMatrixInfo->UncontrolledP += pSlaveNode->Pgr;
				pMatrixInfo->UncontrolledQ += pSlaveNode->Qgr;
				break;
			default:
				// генераторный узел не может входить в PQ-суперузел
				_ASSERTE(pNode->m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_PQ);
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
		pNode->StartLF(m_Parameters.m_bFlat, m_Parameters.m_Imb);
		// корректируем неуправляемую генерацию

		pNode->DebugLog(fmt::format("{} G[{},{}] L[{},{}] Qlim[{},{}] Vref {} Uncontrolled[{},{}]",
			pNode->GetId(),
			pNode->Pgr, pNode->Qgr,
			pNode->Pnr, pNode->Qnr,
			pNode->LFQmin, pNode->LFQmax,
			pNode->LFVref,
			pMatrixInfo->UncontrolledP, pMatrixInfo->UncontrolledQ
		));

		for (VirtualBranch* pBranch{ pNode->m_VirtualBranchBegin }; pBranch < pNode->m_VirtualBranchEnd; pBranch++)
		{
			const auto& pOppNode{ pBranch->pNode };
			pNode->DebugLog(fmt::format("{} y={}", pBranch->pNode->GetId(), pBranch->Y));
		}


		switch (pNode->m_eLFNodeType)
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
	const ptrdiff_t NodeCount{ m_pMatrixInfoSlackEnd - m_pMatrixInfo.get() };
	SeidellOrder.reserve(NodeCount);
	QUEUE queue;

	_MatrixInfo* pMatrixInfo;

	for (pMatrixInfo = m_pMatrixInfo.get(); pMatrixInfo <= m_pMatrixInfoEnd; pMatrixInfo++)
		pMatrixInfo->bVisited = false;

	// в начало добавляем БУ
	for (pMatrixInfo = m_pMatrixInfoSlackEnd - 1; pMatrixInfo >= m_pMatrixInfoEnd; pMatrixInfo--)
	{
		pMatrixInfo->bVisited = true;
		queue.push_back(pMatrixInfo);
		SeidellOrder.push_back(pMatrixInfo);
	}

	std::vector<VirtualBranch*> branches;
	branches.reserve(NodeCount);

	const auto OppMatrixInfo = [this](const CDynaNodeBase* pNode)
	{
		return &m_pMatrixInfo[pNode->A(0) / 2];
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

		_ASSERTE(node1->m_eLFNodeType == node2->m_eLFNodeType);

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

		for (VirtualBranch* pBranch = pMatrixInfo->pNode->m_VirtualBranchBegin; pBranch < pMatrixInfo->pNode->m_VirtualBranchEnd; pBranch++)
		{
			if (pBranch->pNode->IsLFTypeSlack() || OppMatrixInfo(pBranch->pNode)->bVisited)
				continue;
			branches.push_back(pBranch);
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
			SeidellOrder.push_back(pMatrixInfo);
		}
	}
}

void CLoadFlow::BuildSeidellOrder(MATRIXINFO& SeidellOrder)
{
	SeidellOrder.reserve(m_pMatrixInfoSlackEnd - m_pMatrixInfo.get());

	// добавляет узел в очередь для Зейделя
	const auto AddToQueue = [this](_MatrixInfo* pMatrixInfo, QUEUE& queue)
	{
		// просматриваем список ветвей узла
		for (VirtualBranch* pBranch = pMatrixInfo->pNode->m_VirtualBranchBegin; pBranch < pMatrixInfo->pNode->m_VirtualBranchEnd; pBranch++)
		{
			const auto& pOppNode{ pBranch->pNode };
			// мы обходим узлы, но кроме данных узлов нам нужны данные матрицы, чтобы просматривать
			// признак посещения
			if (pOppNode->IsLFTypePQ() && (!pOppNode->IsLFTypeSlack()))
			{
				_MatrixInfo* pOppMatrixInfo = &m_pMatrixInfo[pOppNode->A(0) / 2]; // находим оппозитный узел в матрице
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

	for (pMatrixInfo = m_pMatrixInfo.get(); pMatrixInfo <= m_pMatrixInfoEnd; pMatrixInfo++)
		pMatrixInfo->bVisited = false;

	// в начало добавляем БУ
	for (pMatrixInfo = m_pMatrixInfoSlackEnd - 1; pMatrixInfo >= m_pMatrixInfoEnd; pMatrixInfo--)
	{
		SeidellOrder.push_back(pMatrixInfo);
		pMatrixInfo->bVisited = true;
	}

	// затем PV узлы
	for (; pMatrixInfo >= m_pMatrixInfo.get(); pMatrixInfo--)
	{
		if (!pMatrixInfo->pNode->IsLFTypePQ())
		{
			SeidellOrder.push_back(pMatrixInfo);
			pMatrixInfo->bVisited = true;
		}
	}

	// сортируем PV узлы по убыванию диапазона реактивной мощности
	sort(SeidellOrder.begin() + (m_pMatrixInfoSlackEnd - m_pMatrixInfoEnd), SeidellOrder.end(), SortPV);

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
	//Gauss();
	m_pDynaModel->Log(DFW2MessageStatus::DFW2LOG_INFO, CDFW2Messages::m_cszLFRunningSeidell);

	MATRIXINFO SeidellOrder;
	BuildSeidellOrder(SeidellOrder);

	_ASSERTE(SeidellOrder.size() == m_pMatrixInfoSlackEnd - m_pMatrixInfo.get());

	double dPreviousImb{ -1.0 }, dPreviousImbQ{ -1.0 };

	ptrdiff_t& nSeidellIterations = pNodes->m_IterationControl.Number;

	for (nSeidellIterations = 0; nSeidellIterations < m_Parameters.m_nSeidellIterations; nSeidellIterations++)
	{
		// множитель для ускорения Зейделя
		double dStep{ m_Parameters.m_dSeidellStep };

		if (nSeidellIterations > 2)
		{
			dPreviousImbQ = 0.3 * std::abs(pNodes->m_IterationControl.m_MaxImbQ.GetDiff());

			// если сделали более 2-х итераций начинаем анализировать небалансы
			if (dPreviousImb < 0.0)
			{
				// первый небаланс, если еще не рассчитывали
				dPreviousImb = ImbNorm(pNodes->m_IterationControl.m_MaxImbP.GetDiff(), dPreviousImbQ);
			}
			else
			{
				// если есть предыдущий небаланс, рассчитываем отношение текущего и предыдущего
				double dCurrentImb = ImbNorm(pNodes->m_IterationControl.m_MaxImbP.GetDiff(), pNodes->m_IterationControl.m_MaxImbQ.GetDiff());
				double dImbRatio = dCurrentImb / dPreviousImb;
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
		pNodes->m_IterationControl.Reset();
		// определяем можно ли выполнять переключение типов узлов (по количеству итераций)
		bool bPVPQSwitchEnabled = nSeidellIterations >= m_Parameters.m_nEnableSwitchIteration;
		// для всех узлов в порядке обработки Зейделя

		

		for (auto&& it : SeidellOrder)
		{
			_MatrixInfo* pMatrixInfo{ it };
			const auto& pNode{ pMatrixInfo->pNode };
			// рассчитываем нагрузку по СХН
			double& Pe{ pMatrixInfo->m_dImbP }, &Qe{ pMatrixInfo->m_dImbQ };
			// рассчитываем небалансы
			GetNodeImb(pMatrixInfo);
			cplx Unode(pNode->Vre, pNode->Vim);

			double Q{ Qe + pNode->Qgr };	// расчетная генерация в узле

			cplx I1{ dStep / std::conj(Unode) / pNode->YiiSuper };

			switch (pNode->m_eLFNodeType)
			{
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMAX:
				pNodes->m_IterationControl.Update(pMatrixInfo);

				// если узел на верхнем пределе и напряжение больше заданного
				if (it->NodeVoltageViolation() > m_Parameters.m_dVdifference)
				{
					if (Q < pNode->LFQmin - m_Parameters.m_Imb)
					{
						pNode->Qgr = Q;
						LogNodeSwitch(it, "PQmax->PQmin");
						pNode->Qgr = pNode->LFQmin;
						pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMIN;
						pNodes->m_IterationControl.m_nQviolated++;
						Qe = Q - pNode->Qgr;
						cplx dU = I1 * cplx(Pe, -Qe);
						pNode->Vre += dU.real();
						pNode->Vim += dU.imag();
					}
					else
					{
						// снимаем узел с ограничения и делаем его PV
						pNode->Qgr = Q;
						LogNodeSwitch(it, "PQmax->PV");
						pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PV;
						pMatrixInfo->m_nPVSwitchCount++;
						pNodes->m_IterationControl.m_nQviolated++;
						pNode->Qgr = Q;
						cplx dU = I1 * cplx(Pe, 0);
						dU += Unode;
						dU = pNode->LFVref * dU / std::abs(dU);
						pNode->Vre = dU.real();
						pNode->Vim = dU.imag();
					}
				}
				else
				{
					// если напряжение не выше заданного - вводим ограничение реактивной мощности
					pNode->Qgr = pNode->LFQmax;
					cplx dU = I1 * cplx(Pe, -Qe);
					pNode->Vre += dU.real();
					pNode->Vim += dU.imag();
				}
				break;
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMIN:
				pNodes->m_IterationControl.Update(pMatrixInfo);

				// если узел на нижнем пределе и напряжение меньше заданного
				if (it->NodeVoltageViolation() < -m_Parameters.m_dVdifference)
				{
					if (Q > pNode->LFQmax + m_Parameters.m_Imb)
					{
						pNode->Qgr = Q;
						LogNodeSwitch(it, "PQmin->PQmax");
						pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMAX;
						pNodes->m_IterationControl.m_nQviolated++;
						pNode->Qgr = pNode->LFQmax;
						Qe = Q - pNode->Qgr;
						cplx dU = I1 * cplx(Pe, -Qe);
						pNode->Vre += dU.real();
						pNode->Vim += dU.imag();
					}
					else
					{
						// снимаем узел с ограничения
						pNode->Qgr = Q;
						LogNodeSwitch(it, "PQmin->PV");
						pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PV;
						pNodes->m_IterationControl.m_nQviolated++;
						pMatrixInfo->m_nPVSwitchCount++;
						pNode->Qgr = Q;
						cplx dU = I1 * cplx(Pe, 0);
						dU += Unode;
						dU = pNode->LFVref * dU / std::abs(dU);
						pNode->Vre = dU.real();
						pNode->Vim = dU.imag();
					}
				}
				else
				{
					// если напряжение не меньше заданного - вводим ограничение реактивной мощности
					pNode->Qgr = pNode->LFQmin;
					cplx dU = I1 * cplx(Pe, -Qe);
					pNode->Vre += dU.real();
					pNode->Vim += dU.imag();
				}
				break;
			case CDynaNodeBase::eLFNodeType::LFNT_PV:
			{
				pNodes->m_IterationControl.Update(pMatrixInfo);
				if (bPVPQSwitchEnabled)
				{
					// PV-узлы переключаем если есть разрешение (на первых итерациях переключение блокируется, можно блокировать по небалансу)
					if (Q > pNode->LFQmax + m_Parameters.m_Imb + dPreviousImbQ)
					{
						pNode->Qgr = Q;
						LogNodeSwitch(it, "PV->PQmax");
						pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMAX;
						pNodes->m_IterationControl.m_nQviolated++;
						pNode->Qgr = pNode->LFQmax;
						Qe = Q - pNode->Qgr;
					}
					else if (Q < pNode->LFQmin - m_Parameters.m_Imb - dPreviousImbQ)
					{
						pNode->Qgr = Q;
						LogNodeSwitch(it, "PV->PQmin");
						pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMIN;
						pNodes->m_IterationControl.m_nQviolated++;
						pNode->Qgr = pNode->LFQmin;
						Qe = Q - pNode->Qgr;
					}
					else
					{
						pNode->Qgr = Q;
						Qe = 0.0;
					}
					cplx dU = I1 * cplx(Pe, -Qe);

					dU += Unode;
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
					cplx dU = I1 * cplx(Pe, -Qe);
					dU += Unode;
					dU = pNode->LFVref * dU / std::abs(dU);
					pNode->Vre = dU.real();
					pNode->Vim = dU.imag();
					//*/
				}
			}
			break;
			case CDynaNodeBase::eLFNodeType::LFNT_PQ:
			{
				pNodes->m_IterationControl.Update(pMatrixInfo);
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

		}

		if (!CheckLF())
			// если итерация привела не недопустимому режиму - выходим
			throw dfw2error(CDFW2Messages::m_cszUnacceptableLF);

		pNodes->DumpIterationControl();

		// если достигли заданного небаланса - выходим
		if (pNodes->m_IterationControl.Converged(m_Parameters.m_Imb))
			break;
	}
}

// строит матрицу якоби и правую части в формулировке по токам - все уравнения
// в мощностях разделены на модуль напряжения в узле

void CLoadFlow::BuildMatrixCurrent()
{
	double* pb = klu.B();
	double* pAx = klu.Ax();

	// обходим только узлы в матрице
	for (_MatrixInfo* pMatrixInfo = m_pMatrixInfo.get(); pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
	{
		// здесь считаем, что нагрузка СХН в Node::pnr/Node::qnr уже в расчете и анализе небалансов
		const auto& pNode{ pMatrixInfo->pNode };
		GetPnrQnrSuper(pNode);
		cplx Sneb;
		// диагональные производные проще чем в мощностях, их досчитываем в конце
		double dPdDelta(0.0), dQdDelta(0.0);
		double dPdV(0.0), dQdV(1.0);
		double* pAxSelf = pAx;
		pAx += 2;
		// обратная величина от модуля напряжения в узле
		const double Vinv = 1.0 / pNode->V;
		// комплексное напряжение в узле
		const cplx Unode(pNode->Vre, pNode->Vim);
		// сопряженное напряжение деленное на модуль
		const cplx UnodeConjByV(std::conj(Unode) * Vinv);

		if (pNode->IsLFTypePQ())
		{
			// для PQ-узлов формируем оба уравнения

			for (VirtualBranch* pBranch = pMatrixInfo->pNode->m_VirtualBranchBegin; pBranch < pMatrixInfo->pNode->m_VirtualBranchEnd; pBranch++)
			{
				const auto& pOppNode{ pBranch->pNode };
				// вычисляем компоненты производных по комплексным напряжениям в узлах (Давыдов, стр. 75)
				// при этом используем не напряжение в узле, а частное от напряжения и модуля (так как уравнения разделены на модуль)
				cplx mult = UnodeConjByV * cplx(pOppNode->Vre, pOppNode->Vim) * pBranch->Y;

				// небаланс по ветвям
				Sneb -= cplx(pOppNode->Vre, pOppNode->Vim) * pBranch->Y;

				// компоненты диагональных производных по углу
				dPdDelta -= mult.imag();
				dQdDelta += -mult.real();

				if (NodeInMatrix(pOppNode))
				{
					// внедиагональные производные (уже разделены на модуль, так как компоненты рассчитаны по UnodeConjByV
					double dPdDeltaM = mult.imag();
					double dPdVM = -CDevice::ZeroDivGuard(mult.real(), pOppNode->V);
					double dQdDeltaM = mult.real();
					double dQdVM = CDevice::ZeroDivGuard(mult.imag(), pOppNode->V);

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
			for (VirtualBranch* pBranch = pMatrixInfo->pNode->m_VirtualBranchBegin; pBranch < pMatrixInfo->pNode->m_VirtualBranchEnd; pBranch++)
			{
				const auto& pOppNode{ pBranch->pNode };
				cplx mult = UnodeConjByV * cplx(pOppNode->Vre, pOppNode->Vim) * pBranch->Y;

				Sneb -= cplx(pOppNode->Vre, pOppNode->Vim) * pBranch->Y;

				dPdDelta -= mult.imag();

				if (NodeInMatrix(pOppNode))
				{
					// внедиагональные производные (уже разделены на модуль, так как компоненты рассчитаны по UnodeConjByV
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

		// генерация в узле (включая неуправляемую из суперузлов)
		const cplx NodeInjG(pNode->Pgr + pMatrixInfo->UncontrolledP, pNode->Qgr + pMatrixInfo->UncontrolledQ);
		// расчетная по СХН нагрузка в узле 
		const cplx NodeInjL(pNode->Pnr, pNode->Qnr);
		// небаланс в узле
		Sneb -= Unode * pNode->YiiSuper;
		// небалансы в токах - поэтому делим мощности на модуль
		Sneb = Vinv * (std::conj(Sneb) * Unode + NodeInjL - NodeInjG);

		const double VinvSq(Vinv * Vinv);

		// диагональные производные по напряжению
		// в уравнении разность генерации и нагрузки, деленная на напряжение. Генерация не зависит от напряжения.
		// Нагрузка с СХН - функция напряжения, поэтому генерацию просто делим на квадрат напряжения, а производную
		// нагрузки считаем по правилу частного (см док)

		dPdV = pNode->dLRCPn * Vinv - (pNode->Pnr - NodeInjG.real()) * VinvSq - pNode->YiiSuper.real();

		if (pNode->IsLFTypePQ())
			dQdV = pNode->dLRCQn * Vinv - (pNode->Qnr - NodeInjG.imag()) * VinvSq + pNode->YiiSuper.imag();
		else
			Sneb.imag(0.0);	// если узел не PQ - обнуляем уравнение


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
	for (_MatrixInfo* pMatrixInfo = m_pMatrixInfo.get(); pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
	{
		// здесь считаем, что нагрузка СХН в Node::pnr/Node::qnr уже в расчете и анализе небалансов
		const auto& pNode{ pMatrixInfo->pNode };
		GetPnrQnrSuper(pNode);
		cplx Sneb;
		double dPdDelta(0.0), dQdDelta(0.0);
		double dPdV = pNode->GetSelfdPdV(), dQdV(1.0);
		double* pAxSelf = pAx;
		pAx += 2;
		cplx UnodeConj(pNode->Vre, -pNode->Vim);
		if (pNode->IsLFTypePQ())
		{
			// для PQ-узлов формируем оба уравнения

			dQdV = pNode->GetSelfdQdV();
			for (VirtualBranch* pBranch = pMatrixInfo->pNode->m_VirtualBranchBegin; pBranch < pMatrixInfo->pNode->m_VirtualBranchEnd; pBranch++)
			{
				const auto& pOppNode{ pBranch->pNode };
				cplx mult = UnodeConj * cplx(pOppNode->Vre, pOppNode->Vim) * pBranch->Y;

				Sneb -= cplx(pOppNode->Vre, pOppNode->Vim) * pBranch->Y;

				dPdDelta -= mult.imag();
				dPdV += -CDevice::ZeroDivGuard(mult.real(), pNode->V);
				dQdDelta += -mult.real();
				dQdV += CDevice::ZeroDivGuard(mult.imag(), pNode->V);

				if (NodeInMatrix(pOppNode))
				{
					double dPdDeltaM = mult.imag();
					double dPdVM = -CDevice::ZeroDivGuard(mult.real(), pOppNode->V);
					double dQdDeltaM = mult.real();
					double dQdVM = CDevice::ZeroDivGuard(mult.imag(), pOppNode->V);

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
			for (VirtualBranch* pBranch = pMatrixInfo->pNode->m_VirtualBranchBegin; pBranch < pMatrixInfo->pNode->m_VirtualBranchEnd; pBranch++)
			{
				const auto& pOppNode{ pBranch->pNode };
				cplx mult = UnodeConj * cplx(pOppNode->Vre, pOppNode->Vim) * pBranch->Y;

				Sneb -= cplx(pOppNode->Vre, pOppNode->Vim) * pBranch->Y;

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

// расчет небаланса в узле
void CLoadFlow::GetNodeImb(_MatrixInfo* pMatrixInfo)
{
	const auto& pNode{ pMatrixInfo->pNode };
	// нагрузка по СХН
	GetPnrQnrSuper(pNode);
	//  в небалансе учитываем неконтролируемую генерацию в суперузлах
	pMatrixInfo->m_dImbP = pMatrixInfo->m_dImbQ = 0.0;
	cplx Vnode(pNode->Vre, pNode->Vim);
	cplx I{ -Vnode * pNode->YiiSuper };

	for (VirtualBranch* pBranch = pMatrixInfo->pNode->m_VirtualBranchBegin; pBranch < pMatrixInfo->pNode->m_VirtualBranchEnd; pBranch++)
	{
		const auto& pOppNode{ pBranch->pNode };
		// в отладке можем посмотреть мощность перетока по ветви
		//cplx sx{ std::conj(cplx(pOppNode->Vre, pOppNode->Vim) * pBranch->Y) * Vnode };
		I -= cplx(pOppNode->Vre, pOppNode->Vim) * pBranch->Y;
	}

	
	I = std::conj(I) * Vnode;
	I += cplx(pNode->Pnr - pNode->Pgr - pMatrixInfo->UncontrolledP, pNode->Qnr - pNode->Qgr - pMatrixInfo->UncontrolledQ);
	CDevice::FromComplex(pMatrixInfo->m_dImbP, pMatrixInfo->m_dImbQ, I);
}

bool CLoadFlow::Run()
{
	bool bRes{ true };

	pNodes = static_cast<CDynaNodeContainer*>(m_pDynaModel->GetDeviceContainer(DEVTYPE_NODE));
	if (!pNodes)
		throw dfw2error("CLoadFlow::Start - node container unavailable");

	try
	{
		// вводим СХН УР и V0 = Unom
		pNodes->SwitchLRCs(false);

		Start();
		Estimate();

		switch (m_Parameters.m_Startup)
		{
		case CLoadFlow::eLoadFlowStartupMethod::Seidell:
			Seidell();
			pNodes->CalculateSuperNodesAdmittances(false);
			break;
		case CLoadFlow::eLoadFlowStartupMethod::Tanh:
			NewtonTanh();
			for (auto&& it : *pNodes) static_cast<CDynaNodeBase*>(it)->StartLF(false, m_Parameters.m_Imb);
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

		for (auto&& it : pNodes->m_DevVec)
		{
			const auto& pNode{ static_cast<CDynaNodeBase*>(it) };

			if (!pNode->IsStateOn())
				continue;


			if (!pNode->m_pSuperNodeParent)
			{
				// этот вызов портит qnr
				// если его вызвать после 
				// переключения СХН на динамику

				// до переключения он просто не работает
				// с СХН на постоянную мощность
				// которые в УР заданы nullptr
				pNode->GetPnrQnrSuper();
				_MatrixInfo mx;
				mx.pNode = pNode;
				GetNodeImb(&mx);
				mx.pNode = pNode;
				_ASSERTE(std::abs(mx.m_dImbP) < m_Parameters.m_Imb && std::abs(mx.m_dImbQ) < m_Parameters.m_Imb);
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

	for (_MatrixInfo* pMatrixInfo = m_pMatrixInfo.get(); pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
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

		for (VirtualBranch* pBranch = pMatrixInfo->pNode->m_VirtualBranchBegin; pBranch < pMatrixInfo->pNode->m_VirtualBranchEnd; pBranch++)
		{
			double dDelta = pNode->Delta - pBranch->pNode->Delta;

			if (dDelta > M_PI_2 || dDelta < -M_PI_2)
			{
				bRes = false;
				if (ReportedBranches.insert(NodePair(pNode, pBranch->pNode)).second)
					m_pDynaModel->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(
						CDFW2Messages::m_cszLFBranchAngleExceeds90,
						pNode->GetVerbalName(),
						pBranch->pNode->GetVerbalName(),
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

	for (auto&& it : pNodes->m_DevVec)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(it) };

		if (!pNode->IsStateOn())
			continue;

		// проходим по генераторам, подключенным к узлу
		const  CLinkPtrCount* const pGenLink{ pNode->GetLink(1) };
		LinkWalker< CDynaPowerInjector> pGen;

		// сбрасываем суммарные ограничения Q генераторов
		pNode->LFQminGen = pNode->LFQmaxGen = 0.0;

		if (pGenLink->m_nCount)
		{
			pNode->Pg = pNode->Qg = 0.0;
			pNode->LFQmin = pNode->LFQmax = 0.0;
			while (pGenLink->In(pGen))
			{
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
					pGen->Q = (std::max)((std::min)(pGen->Q, pGen->LFQmax), pGen->LFQmin);
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
	bool bAllOk(true);

	if (!pNodes->CheckLink(1))
		return;	// если генераторов нет - выходим

	for (auto&& it : pNodes->m_DevVec)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(it) };

		if (!pNode->IsStateOn())
			continue;

		const  CLinkPtrCount* const  pGenLink{ pNode->GetLink(1) };
		LinkWalker<CDynaPowerInjector> pGen;
		if (pGenLink->m_nCount)
		{
			const double Qrange{ pNode->LFQmax - pNode->LFQmin };
			double Qspread(0.0), Qgmin(0.0), Qgmax(0.0);

			while (pGenLink->In(pGen))
			{
				pGen->Q = 0.0;
				if (pGen->IsStateOn())
				{

					if (Qrange > 0.0)
					{
						pGen->Q = pGen->Kgen * (pGen->LFQmin + (pGen->LFQmax - pGen->LFQmin) / Qrange * (pNode->Qg - pNode->LFQmin));
						Qgmin += pGen->LFQmin;
						Qgmax += pGen->LFQmax;
						_CheckNumber(pGen->Q);
					}
					else if (pGen->GetType() == eDFW2DEVICETYPE::DEVTYPE_GEN_INFPOWER)
					{
						pGen->Q = pGen->Kgen * pNode->Qg / pGenLink->m_nCount;
						pGen->P = pGen->Kgen * pNode->Pgr / pGenLink->m_nCount;
					}
					else if (pNode->IsLFTypeSlack())
					{
						double Qrange = pNode->LFQmaxGen - pNode->LFQminGen;
						pGen->Q = pGen->Kgen * (pGen->LFQmin + (pGen->LFQmax - pGen->LFQmin) / Qrange * (pNode->Qg - pNode->LFQminGen));
						Qgmin += pGen->LFQmin;
						Qgmax += pGen->LFQmax;
						_CheckNumber(pGen->Q);
					}
					else if (Qrange < m_Parameters.m_Imb)
					{
						// если у генератора пустой диапазон,
						// пишем в него его Qmin
						pGen->Q = pGen->Kgen * pGen->LFQmin;
						Qgmin += pGen->LFQmin;
						Qgmax += pGen->LFQmax;
						_CheckNumber(pGen->Q);
					}
					else
						pGen->Q = 0.0;

					Qspread += pGen->Q;
				}
			}
			if (std::abs(pNode->Qg - Qspread) > m_Parameters.m_Imb)
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

void CLoadFlow::GetPnrQnrSuper(CDynaNodeBase* pNode)
{
	GetPnrQnr(pNode);
	const CLinkPtrCount* const pLink{ pNode->GetSuperLink(0) };
	LinkWalker<CDynaNodeBase> pSlaveNode;
	while (pLink->In(pSlaveNode))
	{
		GetPnrQnr(pSlaveNode);
		pNode->Pnr += pSlaveNode->Pnr;
		pNode->Qnr += pSlaveNode->Qnr;
		pNode->dLRCPn += pSlaveNode->dLRCPn;
		pNode->dLRCQn += pSlaveNode->dLRCQn;
	}
}

void CLoadFlow::GetPnrQnr(CDynaNodeBase* pNode)
{
	// по умолчанию нагрузка равна заданной в УР
	pNode->Pnr = pNode->Pn;
	pNode->Qnr = pNode->Qn;
	double VdVnom = pNode->V / pNode->V0;

	pNode->dLRCPn = pNode->dLRCQn = 0.0;

	// если есть СХН нагрузки, рассчитываем
	// комплексную мощность и производные по напряжению

	if (pNode->m_pLRC)
	{
		// для узлов с отрицательными нагрузками СХН не рассчитываем
		// если такие СХН не разрешены в параметрах
		if (m_Parameters.m_bAllowNegativeLRC || pNode->Pn > 0.0)
		{
			pNode->Pnr *= pNode->m_pLRC->P()->GetBoth(VdVnom, pNode->dLRCPn, pNode->dLRCVicinity);
			pNode->dLRCPn *= pNode->Pn / pNode->V0;
		}
		if (m_Parameters.m_bAllowNegativeLRC || pNode->Qn > 0.0)
		{
			pNode->Qnr *= pNode->m_pLRC->Q()->GetBoth(VdVnom, pNode->dLRCQn, pNode->dLRCVicinity);
			pNode->dLRCQn *= pNode->Qn / pNode->V0;
		}
	}
}

void CLoadFlow::DumpNodes()
{
	std::ofstream dump(m_pDynaModel->Platform().ResultFile("resnodes.csv"));
	if (dump.is_open())
	{
		dump << "N;V;D;Pn;Qn;Pnr;Qnr;Pg;Qg;Qgr;Type;Qmin;Qmax;Vref;VR;DeltaR;QgR;QnR" << std::endl;
		for (auto&& it : pNodes->m_DevVec)
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
				pNode->m_eLFNodeType,
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
				pNode->m_eLFNodeType,
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
	m_pDynaModel->Log(DFW2MessageStatus::DFW2LOG_INFO, CDFW2Messages::m_cszLFRunningNewton);
	ptrdiff_t& it = pNodes->m_IterationControl.Number;
	// вектор для указателей переключаемых узлов, с размерностью в половину уравнений матрицы
	MATRIXINFO PV_PQmax, PV_PQmin, PQmax_PV, PQmin_PV;

	PV_PQmax.reserve(klu.MatrixSize() / 2);
	PV_PQmin.reserve(klu.MatrixSize() / 2);
	PQmax_PV.reserve(klu.MatrixSize() / 2);
	PQmin_PV.reserve(klu.MatrixSize() / 2);

	// квадраты небалансов до и после итерации
	double g0(0.0), g1(0.1), lambda(1.0);
	m_nNodeTypeSwitchesDone = 1;
	it = 0;
	
	while (1)
	{
		if (!CheckLF())
			throw dfw2error(CDFW2Messages::m_cszUnacceptableLF);

		if (it > m_Parameters.m_nMaxIterations)
			throw dfw2error(CDFW2Messages::m_cszLFNoConvergence);

		pNodes->m_IterationControl.Reset();
		PV_PQmax.clear();	// сбрасываем список переключаемых узлов чтобы его обновить
		PV_PQmin.clear();
		PQmax_PV.clear();
		PQmin_PV.clear();

		// считаем небаланс по всем узлам кроме БУ
		_MatrixInfo* pMatrixInfo = m_pMatrixInfo.get();
		for (; pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
		{
			const auto& pNode{ pMatrixInfo->pNode };
			// небаланс считается с учетом СХН
			GetNodeImb(pMatrixInfo);
			// обновляем данные итерации от текущего узла
			pNodes->m_IterationControl.Update(pMatrixInfo);
			// получаем расчетную генерацию узла
			double Qg = pNode->Qgr + pMatrixInfo->m_dImbQ;
			switch (pNode->m_eLFNodeType)
			{
				// если узел на минимуме Q и напряжение ниже уставки, он должен стать PV
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMIN:
				if (pMatrixInfo->NodeVoltageViolation() < -m_Parameters.m_dVdifference)
				{
					if (pMatrixInfo->m_nPVSwitchCount < m_Parameters.m_nMaxPVPQSwitches)
						PQmin_PV.push_back(pMatrixInfo);
					else
 						m_FailedPVPQNodes.insert(pNode);
				}
				break;
				// если узел на максимуме Q и напряжение выше уставки, он должен стать PV
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMAX:
				if (pMatrixInfo->NodeVoltageViolation() > m_Parameters.m_dVdifference)
				{
					if (pMatrixInfo->m_nPVSwitchCount < m_Parameters.m_nMaxPVPQSwitches)
						PQmax_PV.push_back(pMatrixInfo);
					else
						m_FailedPVPQNodes.insert(pNode);
				}
				break;
				// если узел PV, но реактивная генерация вне диапазона, делаем его PQ
			case CDynaNodeBase::eLFNodeType::LFNT_PV:
				pNode->Qgr = Qg;	// если реактивная генерация в пределах - обновляем ее значение в узле
				// рассчитываем нарушение ограничения по генерации реактивной мощности
				if ((pMatrixInfo->m_NodePowerViolation = Qg - pNode->LFQmax) > m_Parameters.m_Imb)
					PV_PQmax.push_back(pMatrixInfo);
				else if ((pMatrixInfo->m_NodePowerViolation = Qg - pNode->LFQmin) < -m_Parameters.m_Imb)
					PV_PQmin.push_back(pMatrixInfo);

				break;
			}
		}

		// общее количество узлов с нарушенными ограничениями и подлежащих переключению
		pNodes->m_IterationControl.m_nQviolated = PV_PQmax.size() + PV_PQmin.size() + PQmax_PV.size() + PQmin_PV.size();

		UpdateSlackBusesImbalance();

		g1 = GetSquaredImb();

		// отношение квадратов невязок
		pNodes->m_IterationControl.m_ImbRatio = g0 > 0.0 ? g1 / g0 : 0.0;

		DumpNewtonIterationControl();
		it++;

		if (pNodes->m_IterationControl.Converged(m_Parameters.m_Imb))
			break;

		// если не было переключений типов узлов на прошлой итерации 
		// и небаланс увеличился, делаем backtrack
		if (!m_nNodeTypeSwitchesDone && g1 > g0)
		{
			double gs1v = -CDynaModel::gs1(klu, m_Rh, klu.B());
			// знак gs1v должен быть "-" ????
			lambda *= -0.5 * gs1v / (g1 - g0 - gs1v);
			// если шаг слишком мал и есть узлы для переключений,
			// несмотря на не снижающийся небаланс переходим к переключениям узлов
			if (lambda > m_Parameters.ForceSwitchLambda)
			{
				// ограничение шага не достигло значения
				// для принудительного переключения
				RestoreVDelta();
				UpdateVDelta(lambda);
				m_NewtonStepRatio.m_dRatio = lambda;
				m_NewtonStepRatio.m_eStepCause = LFNewtonStepRatio::eStepLimitCause::Backtrack;
				continue;
			}
		}

		// сохраняем исходные значения переменных
		StoreVDelta();

		m_nNodeTypeSwitchesDone = 0;

		// функция возврата узла из ограничения Q в PV
		const auto fnToPv = [this](auto& node, std::string_view Title)
		{
			node->m_nPVSwitchCount++;
			CDynaNodeBase*& pNode = node->pNode;
			LogNodeSwitch(node, Title);
			pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PV;
			pNode->V = pNode->LFVref;
			pNode->UpdateVreVimSuper();
			// считаем количество переключений типов узлов
			// чтобы исключить выбор шага по g1 > g0
			m_nNodeTypeSwitchesDone++;
		};

		std::sort(PQmin_PV.begin(), PQmin_PV.end(), [](const _MatrixInfo* lhs, const _MatrixInfo* rhs)
			{
				return std::abs(lhs->m_NodeVoltageViolation) > std::abs(rhs->m_NodeVoltageViolation);
			});

		std::sort(PQmax_PV.begin(), PQmax_PV.end(), [](const _MatrixInfo* lhs, const _MatrixInfo* rhs)
			{
				return std::abs(lhs->m_NodeVoltageViolation) > std::abs(rhs->m_NodeVoltageViolation);
			});

		std::sort(PV_PQmax.begin(), PV_PQmax.end(), [](const _MatrixInfo* lhs, const _MatrixInfo* rhs)
			{
				return std::abs(lhs->m_NodePowerViolation) > std::abs(rhs->m_NodePowerViolation);
			});

		std::sort(PV_PQmin.begin(), PV_PQmin.end(), [](const _MatrixInfo* lhs, const _MatrixInfo* rhs)
			{
				return std::abs(lhs->m_NodePowerViolation) > std::abs(rhs->m_NodePowerViolation);
			});

		for (auto&& vec : std::array<MATRIXINFO*, 4>{&PQmin_PV, &PQmax_PV, &PV_PQmax, &PV_PQmin})
		{
			if (static_cast<ptrdiff_t>(vec->size()) > m_Parameters.m_nPVPQSwitchPerIt)
				vec->resize(m_Parameters.m_nPVPQSwitchPerIt);
		}

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

		const double absImbRatio{ std::abs(pNodes->m_IterationControl.m_ImbRatio) };

		if (std::abs(pNodes->m_IterationControl.m_MaxImbP.GetDiff()) < 100.0 * m_Parameters.m_Imb ||
			absImbRatio > 0.95 ||
			std::abs(lambda) <= m_Parameters.ForceSwitchLambda)
		{
			for (auto&& it : PV_PQmax)
			{
				LogNodeSwitch(it, "PV->PQmax");
				CDynaNodeBase*& pNode = it->pNode;
				pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMAX;
				pNode->Qgr = pNode->LFQmax;
				m_nNodeTypeSwitchesDone++;
			}

			if (PV_PQmax.empty())
			{
				for (auto&& it : PV_PQmin)
				{
					LogNodeSwitch(it, "PV->PQmin");
					CDynaNodeBase*& pNode = it->pNode;
					pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMIN;
					pNode->Qgr = pNode->LFQmin;
					m_nNodeTypeSwitchesDone++;
				}
			}
		}

		lambda = 1.0;
		g0 = g1;

		switch (m_Parameters.m_LFFormulation)
		{
		case eLoadFlowFormulation::Power:
			BuildMatrixPower();
			break;
		case eLoadFlowFormulation::Current:
			BuildMatrixCurrent();
			break;
		}

		// сохраняем небаланс до итерации
		std::copy(klu.B(), klu.B() + klu.MatrixSize(), m_Rh.get());

		// Находим узел с максимальной невязкой до шага Ньютона
		ptrdiff_t iMax(0);
		double maxb = klu.FindMaxB(iMax);
		CDynaNodeBase* pNode1(m_pMatrixInfo.get()[iMax / 2].pNode);

		SolveLinearSystem();

		// Находим узел с максимальным приращением
		maxb = klu.FindMaxB(iMax);
		CDynaNodeBase* pNode2(m_pMatrixInfo.get()[iMax / 2].pNode);

		// обновляем переменные
		double MaxRatio = GetNewtonRatio();
		UpdateVDelta(MaxRatio);
		if (m_NewtonStepRatio.m_eStepCause != LFNewtonStepRatio::eStepLimitCause::None)
			m_nNodeTypeSwitchesDone = 1;
	}

	// обновляем реактивную генерацию в суперузлах
	UpdateSupernodesPQ();
}

// обновляет инъекции в суперзулах
void CLoadFlow::UpdateSupernodesPQ()
{
	bool bAllOk(true);

	for (_MatrixInfo* pMatrixInfo = m_pMatrixInfo.get(); pMatrixInfo < m_pMatrixInfoSlackEnd; pMatrixInfo++)
	{
		CDynaNodeBase*& pNode = pMatrixInfo->pNode;
		// диапазон реактивной мощности узла
		double Qrange{ pNode->LFQmax - pNode->LFQmin };
		double Qspread{ 0.0 }, PgSource{ pNode->Pgr }, QgSource{ pNode->Qgr }, DropToSlack{ 0.0 };
		const CLinkPtrCount* const pLink{ pNode->GetSuperLink(0) };
		LinkWalker<CDynaNodeBase> pSlaveNode;
		std::list<CDynaNodeBase*> SlackBuses;

		if (!pNode->m_pSuperNodeParent)
		{
			GetNodeImb(pMatrixInfo);
			_ASSERTE(std::abs(pMatrixInfo->m_dImbP) < m_Parameters.m_Imb && std::abs(pMatrixInfo->m_dImbQ) < m_Parameters.m_Imb);
		}

		// если узел нагрузочный в нем ничего не может измениться
		if (pNode->m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_PQ)
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
					switch (pSlaveNode->m_eLFNodeType)
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

			if (std::abs(Qspread - QgSource) > m_Parameters.m_Imb)
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
	for (_MatrixInfo* pMatrixInfo = m_pMatrixInfo.get(); pMatrixInfo < m_pMatrixInfoSlackEnd; pMatrixInfo++)
		pMatrixInfo->Restore();

	for (auto&& it : pNodes->m_DevVec)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(it) };
		if (!pNode->m_pSuperNodeParent)
			GetPnrQnr(pNode);
	}
}

// Возвращает коэффициент шага Ньютона

double CLoadFlow::GetNewtonRatio()
{
	m_NewtonStepRatio.Reset();

	const _MatrixInfo* pMatrixInfo{ m_pMatrixInfo.get() };
	double* b{ klu.B() };

	// проходим по всем узлам
	for (; pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
	{
		const auto& pNode{ pMatrixInfo->pNode };
		double* pb{ b + pNode->A(0) };
		// рассчитываем новый угол в узле
		const double newDelta{ pNode->Delta - *pb };

		// поиск ограничения шага по углу узла
		const double Dstep{ std::abs(MathUtils::CAngleRoutines::GetAbsoluteDiff2Angles(*pb, 0.0)) };

		if (Dstep > m_Parameters.m_dNodeAngleNewtonStep)
			m_NewtonStepRatio.UpdateNodeAngle(m_Parameters.m_dNodeAngleNewtonStep / Dstep, pNode);

		// рассчитываем новый модуль напряжения в узле
		pb++;
		const double newV{ pNode->V - *pb };
		const double Ratio{ newV / pNode->Unom };

		// поиск ограничения по модулю напряжения
		for (const double d : {0.5, 2.0})
		{
			if ((d > 1.0) ? Ratio > d : Ratio < d)
				m_NewtonStepRatio.UpdateVoltageOutOfRange((pNode->V - d * pNode->Unom) / *pb, pNode);
		}

		// поиск ограничения шага по напряжению
		const double VstepPU{ std::abs(*pb) / pNode->Unom };
		if (VstepPU > m_Parameters.m_dVoltageNewtonStep)
			m_NewtonStepRatio.UpdateVoltage(m_Parameters.m_dVoltageNewtonStep / VstepPU, pNode);
	}

	// поиск ограничения по взимному углу
	for (auto&& it : m_BranchAngleCheck)
	{
		const auto& pBranch{ static_cast<CDynaBranch*>(it) };
		const auto& pNodeIp{ pBranch->m_pNodeIp }, &pNodeIq{ pBranch->m_pNodeIq };
		// текущий взаимный угол
		const double CurrentDelta{ pNodeIp->Delta - pNodeIq->Delta };
		// приращения углов для узлов, которые в матрице или нули, если не в матрице
		const double dIpDelta{ NodeInMatrix(pNodeIp) ? *(b + pNodeIp->A(0)) : 0.0 };
		const double dIqDelta{ NodeInMatrix(pNodeIq) ? *(b + pNodeIq->A(0)) : 0.0 };

		const double DeltaDiff{ MathUtils::CAngleRoutines::GetAbsoluteDiff2Angles(dIqDelta, dIpDelta) };
		//_ASSERTE(Equal(DeltaDiff, dIpDelta - dIqDelta));

		// новый взаимный угол
		const double NewDelta{ CurrentDelta + DeltaDiff };
		const double AbsDeltaDiff{ std::abs(DeltaDiff) };

		// проверяем новый взаимный угол на ограничения в 90 градусов
		if (std::abs(NewDelta) > M_PI_2)
			m_NewtonStepRatio.UpdateBranchAngleOutOfRange(((std::signbit(NewDelta) ? -M_PI_2 : M_PI_2) - CurrentDelta) / DeltaDiff, pNodeIp, pNodeIq);

		// проверяем приращение взаимного угла
		if (AbsDeltaDiff > m_Parameters.m_dBranchAngleNewtonStep)
			m_NewtonStepRatio.UpdateBranchAngle(m_Parameters.m_dBranchAngleNewtonStep / AbsDeltaDiff, pNodeIp, pNodeIq);
	}

	return m_NewtonStepRatio.m_dRatio;
}

void CLoadFlow::UpdateVDelta(double dStep)
{
	_MatrixInfo* pMatrixInfo = m_pMatrixInfo.get();
	double* b = klu.B();

	double MaxRatio = 1.0;

	for (; pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
	{
		const auto& pNode{ pMatrixInfo->pNode };
		const double* pb{ b + pNode->A(0) };
		const double newDelta{ pNode->Delta - *pb * dStep };		
		pb++;
		const double newV{ pNode->V - *pb * dStep };

		/*
		// Не даем узлам на ограничениях реактивной мощности отклоняться от уставки более чем на 10%
		// стратка работает в Inor при небалансах не превышающих заданный
		switch (pNode->m_eLFNodeType)
		{
		case CDynaNodeBase::eLFNodeType::LFNT_PVQMAX:
			if (newV / pNode->LFVref > 1.1)
			{
				newV = pNode->LFVref;
				pNode->Qgr = pNode->LFQmin;
				pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PV;
			}
			break;
		case CDynaNodeBase::eLFNodeType::LFNT_PVQMIN:
			if (newV / pNode->LFVref < 0.9)
			{
				newV = pNode->LFVref;
				pNode->Qgr = pNode->LFQmax;
				pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PV;
			}
			break;
		}
		*/

		pNode->Delta = newDelta;
		pNode->V = newV;
		pNode->UpdateVreVimSuper();
	}
}


void CLoadFlow::StoreVDelta()
{
	if (!m_Vbackup)
		m_Vbackup = std::make_unique<double[]>(klu.MatrixSize());
	if (!m_Dbackup)
		m_Dbackup = std::make_unique<double[]>(klu.MatrixSize());

	double* pV = m_Vbackup.get();
	double* pD = m_Dbackup.get();
	for (_MatrixInfo* pMatrix = m_pMatrixInfo.get(); pMatrix < m_pMatrixInfoEnd; pMatrix++, pV++, pD++)
	{
		*pV = pMatrix->pNode->V;
		*pD = pMatrix->pNode->Delta;
	}
}

void CLoadFlow::RestoreVDelta()
{
	if (m_Vbackup && m_Dbackup)
	{
		double* pV = m_Vbackup.get();
		double* pD = m_Dbackup.get();
		for (_MatrixInfo* pMatrix = m_pMatrixInfo.get(); pMatrix < m_pMatrixInfoEnd; pMatrix++, pV++, pD++)
		{
			pMatrix->pNode->V = *pV;
			pMatrix->pNode->Delta = *pD;
		}
	}
	else
		throw dfw2error("CLoadFlow::RestoreVDelta called before StoreVDelta");
}

double CLoadFlow::GetSquaredImb()
{
	double Imb(0.0);
	for (_MatrixInfo* pMatrixInfo = m_pMatrixInfo.get(); pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
		Imb += ImbNorm(pMatrixInfo->m_dImbP, pMatrixInfo->m_dImbQ);
	return sqrt(Imb);
}

void CLoadFlow::CheckFeasible()
{
	bool bRes{ true };

	for (const auto& it : m_FailedPVPQNodes)
		it->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszLFOverswitchedNode, it->GetVerbalName(),
			m_Parameters.m_nMaxPVPQSwitches));

	for (auto&& it : *pNodes)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(it) };
		if (pNode->IsStateOn())
		{
			if (!pNode->IsLFTypePQ())
			{
				// если узел входит в суперузел его заданное напряжение в расчете было равно заданному напряжению суперузла,
				// которое, в свою очередь, было выбрано по узлу с наиболее широким диапазоном реактивной мощности внутри суперузла
				double LFVref = pNode->m_pSuperNodeParent ? pNode->m_pSuperNodeParent->LFVref : pNode->LFVref;
				if (pNode->V > LFVref && pNode->Qgr >= pNode->LFQmax + m_Parameters.m_Imb)
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
				if (pNode->V < LFVref && pNode->Qgr <= pNode->LFQmin - m_Parameters.m_Imb)
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
				if (pNode->Qgr < pNode->LFQmin - m_Parameters.m_Imb)
				{
					pNode->Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("{} Q < Qmin [{} < {}]",
						pNode->GetVerbalName(),
						pNode->Qgr,
						pNode->LFQmin));
					bRes = false;
				}
				if (pNode->Qgr > pNode->LFQmax + m_Parameters.m_Imb)
				{
					pNode->Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("{} Q > Qmax [{} > {}]",
						pNode->GetVerbalName(),
						pNode->Qgr,
						pNode->LFQmax));
					bRes = false;
				}
			}
			pNode->SuperNodeLoadFlow(m_pDynaModel);
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
	for (_MatrixInfo* pMatrixInfo = m_pMatrixInfoEnd; pMatrixInfo < m_pMatrixInfoSlackEnd; pMatrixInfo++)
	{
		const auto& pNode{ pMatrixInfo->pNode };
		GetNodeImb(pMatrixInfo);
		// для БУ небалансы только для результатов
		pNode->Pgr += pMatrixInfo->m_dImbP;
		pNode->Qgr += pMatrixInfo->m_dImbQ;
		// в контроле сходимости небаланс БУ всегда 0.0
		pMatrixInfo->m_dImbP = pMatrixInfo->m_dImbQ = 0.0;
		pNodes->m_IterationControl.Update(pMatrixInfo);
	}
}


void CLoadFlow::CalculateBranchFlows()
{
	CDeviceContainer* pBranchContainer = m_pDynaModel->GetDeviceContainer(DEVTYPE_BRANCH);
	for (auto&& dev : *pBranchContainer)
	{
		const auto& pBranch{ static_cast<CDynaBranch*>(dev) };
		// пропускаем ветви с нулевым сопротивлением, для них потоки 
		// уже рассчитаны в SuperNodeLoadFlow
		if (pBranch->IsZeroImpedance()) continue;
		pBranch->Sb = pBranch->Se = { 0.0, 0.0 };
		// в отключенных ветвях потоки просто обнуляем
		if (pBranch->m_BranchState == CDynaBranch::BranchState::BRANCH_OFF) continue;
		cplx cIb, cIe, cSb, cSe;
		CDynaBranchMeasure::CalculateFlows(pBranch, cIb, cIe, pBranch->Sb, pBranch->Se);
	}
}

bool CLoadFlow::CheckNodeBalances()
{
	bool bRes(true);
	cplx y;
	for (auto&& dev : pNodes->m_DevVec)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(dev) };
		// проводимость узла собираем из шунта узла и реакторов
		// вообще еще добавится шунт КЗ, но он при расчете УР равен нулю
		static_cast<CDynaNodeBase*>(dev)->GetGroundAdmittance(y);
		const auto imb{ pNode->V * pNode->V * y };
		pNode->dLRCPg = pNode->Pnr - pNode->Pgr + imb.real();
		pNode->dLRCQg = pNode->Qnr - pNode->Qgr - imb.imag();
	}

	CDeviceContainer* pBranchContainer = m_pDynaModel->GetDeviceContainer(DEVTYPE_BRANCH);
	for (auto&& dev : *pBranchContainer)
	{
		const auto& pBranch{ static_cast<CDynaBranch*>(dev) };
		pBranch->m_pNodeIp->dLRCPg -= pBranch->Sb.real();
		pBranch->m_pNodeIp->dLRCQg -= pBranch->Sb.imag();
		pBranch->m_pNodeIq->dLRCPg += pBranch->Se.real();
		pBranch->m_pNodeIq->dLRCQg += pBranch->Se.imag();
	}

	for (const auto& dev : pNodes->m_DevVec)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(dev) };
		if (pNode->GetState() == eDEVICESTATE::DS_OFF) continue;
		if (std::abs(pNode->dLRCPg) > m_Parameters.m_Imb ||
			std::abs(pNode->dLRCQg) > m_Parameters.m_Imb)
		{
			bRes = false;
			pNode->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszLFNodeImbalance,
				pNode->GetVerbalName(),
				cplx(pNode->dLRCPg, pNode->dLRCQg)));
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
	switch (m_NewtonStepRatio.m_eStepCause)
	{
	case LFNewtonStepRatio::eStepLimitCause::Voltage:
	case LFNewtonStepRatio::eStepLimitCause::NodeAngle:
	case LFNewtonStepRatio::eStepLimitCause::VoltageOutOfRange:
		objectName = fmt::format("{}", m_NewtonStepRatio.m_pNode->GetId());
		break;
	case LFNewtonStepRatio::eStepLimitCause::BrancheAngleOutOfRange:
	case LFNewtonStepRatio::eStepLimitCause::BranchAngle:
		objectName = fmt::format("{}-{}", 
			m_NewtonStepRatio.m_pNode->GetId(),
			m_NewtonStepRatio.m_pNodeBranch->GetId());
		break;
	}
	m_pDynaModel->Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("{} {:15f} {:6.2f} {:4} {}", pNodes->GetIterationControlString(),
		pNodes->m_IterationControl.m_ImbRatio,
		m_NewtonStepRatio.m_dRatio,
		causes[static_cast<ptrdiff_t>(m_NewtonStepRatio.m_eStepCause)],
		objectName
		));
}

void CLoadFlow::LogNodeSwitch(_MatrixInfo* pNode, std::string_view Title)
{
	m_pDynaModel->Log(DFW2MessageStatus::DFW2LOG_DEBUG,
		fmt::format("{} {} V={:.3}, Vref={:.3}, Q={:.3}, Qrange[{:.3};{:.3}] switches done {}",
			Title,
			pNode->pNode->GetVerbalName(),
			pNode->pNode->V,
			pNode->pNode->LFVref,
			pNode->pNode->Qgr,
			pNode->pNode->LFQmin,
			pNode->pNode->LFQmax,
			pNode->m_nPVSwitchCount));
}


void CLoadFlow::Gauss()
{
	KLUWrapper<std::complex<double>> klu;
	ptrdiff_t nNodeCount{ m_pMatrixInfoEnd - m_pMatrixInfo.get() };
	size_t nBranchesCount{ m_pDynaModel->Branches.Count() };
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
	
	for (_MatrixInfo* pMatrixInfo = m_pMatrixInfo.get(); pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
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

		for (VirtualBranch* pV = pNode->m_VirtualBranchBegin; pV < pNode->m_VirtualBranchEnd; pV++)
		{
			*pAx = pV->Y.real();   pAx++;
			*pAx = pV->Y.imag();   pAx++;
			*pAi = pV->pNode->A(0) / 2 ; pAi++;
		}
	}

	nNzCount = (pAx - Ax) / 2;		// рассчитываем получившееся количество ненулевых элементов (делим на 2 потому что комплекс)
	Ap[nNodeCount] = nNzCount;

	ptrdiff_t& nIteration = pNodes->m_IterationControl.Number;

	for (nIteration = 0; nIteration < 20; nIteration++)
	{
		pNodes->m_IterationControl.Reset();

		ppDiags = pDiags.get();
		pB = B;

		// проходим по узлам вне зависимости от их состояния, параллельно идем по диагонали матрицы
		for (_MatrixInfo* pMatrixInfo = m_pMatrixInfo.get(); pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
		{
			CDynaNodeBase* pNode = static_cast<CDynaNodeBase*>(pMatrixInfo->pNode);

			GetNodeImb(pMatrixInfo);
			pNodes->m_IterationControl.Update(pMatrixInfo);

			_ASSERTE(pB < B + nNodeCount * 2);

			// для всех узлов которые включены и вне металлического КЗ

			cplx Unode(pNode->Vre, pNode->Vim);

			cplx I{ pNode->IconstSuper };
			cplx Y{ pNode->YiiSuper };		// диагональ матрицы по умолчанию собственная проводимость узла

			// рассчитываем задающий ток узла от нагрузки
			// можно посчитать ток, а можно посчитать добавку в диагональ
			I += std::conj(cplx(pNode->Pnr - pNode->Pg, pNode->Qnr - pNode->Qg) / Unode);

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

		pNodes->DumpIterationControl();

		ptrdiff_t iMax(0);
		double maxb = klu.FindMaxB(iMax);

		klu.FactorSolve();

		maxb = klu.FindMaxB(iMax);

		// KLU может делать повторную факторизацию матрицы с начальным пивотингом
		// это быстро, но при изменении пивотов может вызывать численную неустойчивость.
		// У нас есть два варианта факторизации/рефакторизации на итерации LULF
		double* pB = B;

		for (_MatrixInfo* pMatrixInfo = m_pMatrixInfo.get(); pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
		{
			const auto& pNode{ static_cast<CDynaNodeBase*>(pMatrixInfo->pNode) };
			// напряжение после решения системы в векторе задающий токов
			if (pNode->IsLFTypePQ())
			{
				pNode->Vre = *pB;		pB++;
				pNode->Vim = *pB;		pB++;
			}
			else
				pB += 2;

			pNode->UpdateVDeltaSuper();
		
		}
	}
}