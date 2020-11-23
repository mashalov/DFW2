#include "stdafx.h"
#include "LoadFlow.h"
#include "DynaModel.h"
#include "DynaPowerInjector.h"
#include "limits"

using namespace DFW2;

CLoadFlow::CLoadFlow(CDynaModel *pDynaModel) :	m_pDynaModel(pDynaModel) {}

bool CLoadFlow::NodeInMatrix(CDynaNodeBase *pNode)
{
	// если тип узла не базисный и узел включен - узел должен войти в матрицу Якоби
	return pNode->m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_BASE && pNode->IsStateOn();
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
	_MatrixInfo *pMatrixInfo = m_pMatrixInfo.get();

	// базисные узлы держим в отдельном списке, так как они не в матрице, но
	// результаты по ним нужно обновлять
	std::list<CDynaNodeBase*> SlackBuses;

	for (auto&& it : pNodes->m_DevVec)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(it);
		// обрабатываем только включенные узлы и узлы без родительского суперузла
		if (!pNode->IsStateOn() || pNode->m_pSuperNodeParent)
			continue;
		// обновляем VreVim узла
		pNode->UpdateVreVim();
		// добавляем БУ в список БУ для дальнейшей обработки
		if (pNode->m_eLFNodeType == CDynaNodeBase::eLFNodeType::LFNT_BASE)
			SlackBuses.push_back(pNode);

		// для узлов которые в матрице считаем количество ветвей
		// и ненулевых элементов
		if (NodeInMatrix(pNode))
		{
			for (VirtualBranch *pV = pNode->m_VirtualBranchBegin; pV < pNode->m_VirtualBranchEnd; pV++)
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
		CDynaNodeBase *pNode = sit;
		// обнуляем ограничения БУ, чтобы
		// они не влияли на суммарные ограничения
		// суперузла, который может быть образован
		// от данного БУ
		pNode->LFQmin = pNode->LFQmax = 0.0;
		pMatrixInfo->Store(pNode);
		pMatrixInfo++;
	}

	m_pMatrixInfoSlackEnd = pMatrixInfo;
	m_Rh = std::make_unique<double[]>(klu.MatrixSize());		// невязки до итерации
}

void CLoadFlow::Estimate()
{
	ptrdiff_t *pAi = klu.Ai();
	ptrdiff_t *pAp = klu.Ap();
	ptrdiff_t nRowPtr = 0;

	_MatrixInfo *pMatrixInfo = m_pMatrixInfo.get();
	for (; pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
	{
		CDynaNodeBase *pNode = pMatrixInfo->pNode;
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

		for (VirtualBranch *pV = pNode->m_VirtualBranchBegin; pV < pNode->m_VirtualBranchEnd; pV++)
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
		CDynaBranch* pBranch = static_cast<CDynaBranch*>(it);
		if (pBranch->m_BranchState == CDynaBranch::BranchState::BRANCH_ON && !pBranch->IsZeroImpedance())
			m_BranchAngleCheck.push_back(pBranch);
	}
}

void CDynaNodeBase::StartLF(bool bFlatStart, double ImbTol)
{
	// для всех включенных и небазисных узлов
	if (IsStateOn())
	{
		if (m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_BASE)
		{
			// если у узла заданы пределы по реактивной мощности и хотя бы один из них ненулевой + задано напряжение
			if (LFQmax > LFQmin && (fabs(LFQmax) > ImbTol || fabs(LFQmin) > ImbTol) && LFVref > 0.0)
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
	pNodes->CalculateSuperNodesAdmittances(m_Parameters.m_bStartup);

	// обновляем данные в PV-узлах по заданным в генераторах реактивным мощностям
	UpdatePQFromGenerators();

	// инициализируем все узлы, чтобы корректно определить их тип
	for (auto&& it : *pNodes)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(it);
		// в режиме отладки запоминаем что было в узлах после расчета Rastr для сравнения результатов
#ifdef _DEBUG
		pNode->GrabRastrResult();
#endif
		pNode->Pgr = pNode->Pg;
		pNode->Qgr = pNode->Qg;
		pNode->StartLF(m_Parameters.m_bFlat, m_Parameters.m_Imb);
	}

	// выбираем узлы, которые войдут в матрицу (все суперузлы), а так же базисные суперузлы.
	AllocateSupernodes();

	for (_MatrixInfo* pMatrixInfo = m_pMatrixInfo.get() ; pMatrixInfo < m_pMatrixInfoSlackEnd ; pMatrixInfo++)
	{
		CDynaNodeBase*& pNode = pMatrixInfo->pNode;
		CLinkPtrCount *pNodeLink = pNode->GetSuperLink(0);
		CDevice **ppDevice(nullptr);
		double QrangeMax = -1.0;
		while (pNodeLink->In(ppDevice))
		{
			CDynaNodeBase *pSlaveNode = static_cast<CDynaNodeBase*>(*ppDevice);
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
				// если в суперузел входит генераторный - учитываем его активную генерацию как неуправляемую
				pMatrixInfo->UncontrolledP += pSlaveNode->Pgr;
				// считаем диапазон реактивной мощности суперзула
				pNode->LFQmin += pSlaveNode->LFQmin;
				pNode->LFQmax += pSlaveNode->LFQmax;
				if (pNode->m_eLFNodeType == CDynaNodeBase::eLFNodeType::LFNT_BASE)
				{
					// если генераторный узел входит в базисный суперузел
					// уставка по напряжению равна напряжению базисного узла
					pSlaveNode->LFVref = pNode->LFVref;
				}
				else
				{
					double Qrange = pSlaveNode->LFQmax - pSlaveNode->LFQmin;
					// выбираем заданное напряжение по узлу с максимальным диапазоном Qmin/Qmax
					if (QrangeMax < 0 || QrangeMax < Qrange)
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
		switch (pNode->m_eLFNodeType)
		{
			case CDynaNodeBase::eLFNodeType::LFNT_BASE:
				// в базисном учитываем и активную и реактивную
				break;
			case CDynaNodeBase::eLFNodeType::LFNT_PQ:
				// в нагрузочном узле нет неуправляемой генераици
				pMatrixInfo->UncontrolledP = pMatrixInfo->UncontrolledQ = 0.0;
				break;
			default:
				// в PV, PQmin, PQmax узлах учитываем реактивную неуправляемую генерацию
				pMatrixInfo->UncontrolledP = 0.0;
		}
	}
}

// функция сортировки PV-узлов для определеия порядка их обработки в Зейделе
bool CLoadFlow::SortPV(const _MatrixInfo* lhs, const _MatrixInfo* rhs)
{
	_ASSERTE(!lhs->pNode->IsLFTypePQ());
	_ASSERTE(!lhs->pNode->IsLFTypePQ());
	// сортируем по убыванию диапазона
	return (lhs->pNode->LFQmax - lhs->pNode->LFQmin) > (rhs->pNode->LFQmax - rhs->pNode->LFQmin);
}

// добавляет узел в очередь для Зейделя
void CLoadFlow::AddToQueue(_MatrixInfo *pMatrixInfo, QUEUE& queue)
{
	// просматриваем список ветвей узла
	for (VirtualBranch *pBranch = pMatrixInfo->pNode->m_VirtualBranchBegin; pBranch < pMatrixInfo->pNode->m_VirtualBranchEnd; pBranch++)
	{
		CDynaNodeBase *pOppNode = pBranch->pNode;
		// мы обходим узлы, но кроме данных узлов нам нужны данные матрицы, чтобы просматривать
		// признак посещения
		if (pOppNode->IsLFTypePQ() && pOppNode->m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_BASE)
		{
			_MatrixInfo *pOppMatrixInfo = &m_pMatrixInfo[pOppNode->A(0) / 2]; // находим оппозитный узел в матрице
			_ASSERTE(pOppMatrixInfo->pNode == pOppNode);
			// если оппозитный узел еще не был посещен, добавляем его в очередь и помечаем как посещенный
			if (!pOppMatrixInfo->bVisited)
			{
				queue.push_back(pOppMatrixInfo);
				pOppMatrixInfo->bVisited = true;
			}
		}
	}
}

void CLoadFlow::Seidell()
{
	m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, CDFW2Messages::m_cszLFRunningSeidell);

	MATRIXINFO SeidellOrder;
	SeidellOrder.reserve(m_pMatrixInfoSlackEnd - m_pMatrixInfo.get());
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
		CDynaNodeBase *pNode = pMatrixInfo->pNode;
		if (!pNode->IsLFTypePQ())
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

	_ASSERTE(SeidellOrder.size() == m_pMatrixInfoSlackEnd - m_pMatrixInfo.get());

	double dPreviousImb = -1.0;
	for (int nSeidellIterations = 0; nSeidellIterations < m_Parameters.m_nSeidellIterations; nSeidellIterations++)
	{
		// множитель для ускорения Зейделя
		double dStep = m_Parameters.m_dSeidellStep;
		
		if (nSeidellIterations > 2)
		{
			// если сделали более 2-х итераций начинаем анализировать небалансы
			if (dPreviousImb < 0.0)
			{
				// первый небаланс, если еще не рассчитывали
				dPreviousImb = ImbNorm(pNodes->m_IterationControl.m_MaxImbP.GetDiff(), pNodes->m_IterationControl.m_MaxImbQ.GetDiff());
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
			pMatrixInfo = it;
			CDynaNodeBase *pNode = pMatrixInfo->pNode;
			// рассчитываем нагрузку по СХН
			double& Pe = pMatrixInfo->m_dImbP;
			double& Qe = pMatrixInfo->m_dImbQ;
			// рассчитываем небалансы
			GetNodeImb(pMatrixInfo);
			cplx Unode(pNode->Vre, pNode->Vim);

			// для всех узлов кроме БУ обновляем статистику итерации
			pNodes->m_IterationControl.Update(pMatrixInfo);

			double Q = Qe + pNode->Qgr;	// расчетная генерация в узле

			cplx I1 = dStep / conj(Unode) / pNode->YiiSuper;

			switch (pNode->m_eLFNodeType)
			{
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMAX:
				// если узел на верхнем пределе и напряжение больше заданного
				if (pNode->V > pNode->LFVref)
				{
					if (Q < pNode->LFQmin)
					{
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
						pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PV;
						pMatrixInfo->m_nPVSwitchCount++;
						pNodes->m_IterationControl.m_nQviolated++;
						pNode->Qgr = Q;
						cplx dU = I1 * cplx(Pe, 0);
						dU += Unode;
						dU = pNode->LFVref * dU / abs(dU);
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
				// если узел на нижнем пределе и напряжение меньше заданного
				if (pNode->V < pNode->LFVref)
				{
					if (Q > pNode->LFQmax)
					{
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
						pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PV;
						pNodes->m_IterationControl.m_nQviolated++;
						pMatrixInfo->m_nPVSwitchCount++;
						pNode->Qgr = Q;
						cplx dU = I1 * cplx(Pe, 0);
						dU += Unode;
						dU = pNode->LFVref * dU / abs(dU);
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
				if (bPVPQSwitchEnabled)
				{
					// PV-узлы переключаем если есть разрешение (на первых итерациях переключение блокируется, можно блокировать по небалансу)
					if (Q > pNode->LFQmax)
					{
						pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMAX;
						pNodes->m_IterationControl.m_nQviolated++;
						pNode->Qgr = pNode->LFQmax;
						Qe = Q - pNode->Qgr;
					}
					else if (Q < pNode->LFQmin)
					{
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
					dU = pNode->LFVref * dU / abs(dU);
					pNode->Vre = dU.real();
					pNode->Vim = dU.imag();
				}
				else
				{
					// до получения разрешения на переключение PV-узел считаем PQ-узлом (может лучше считать PV и распускать реактив)

					cplx dU = I1 * cplx(Pe, -Qe);
					pNode->Vre += dU.real();
					pNode->Vim += dU.imag();
					/*
					cplx dU = I1 * cplx(Pe, -Qe);
					dU += Unode;
					dU = pNode->LFVref * dU / abs(dU);
					pNode->Vre = dU.real();
					pNode->Vim = dU.imag();
					*/
				}
			}
			break;
			case CDynaNodeBase::eLFNodeType::LFNT_PQ:
			{
				/*
				double dP = 0.0;// pNode->Pnr - pNode->Pgr - pNode->V * pNode->V * pNode->YiiSuper.real() - pMatrixInfo->UncontrolledP;
				double dQ = 0.0;// pNode->Qnr - pNode->Qgr + pNode->V * pNode->V * pNode->YiiSuper.imag() - pMatrixInfo->UncontrolledQ;
				cplx Unode(pNode->Vre, pNode->Vim);
				for (VirtualBranch *pBranch = pMatrixInfo->pNode->m_VirtualBranchBegin; pBranch < pMatrixInfo->pNode->m_VirtualBranchEnd; pBranch++)
				{
					CDynaNodeBase *pOppNode = pBranch->pNode;
					cplx mult = cplx(pOppNode->Vre, pOppNode->Vim) * pBranch->Y;
					dP += mult.real();
					dQ += mult.imag();
				}
				cplx Sl = cplx(pNode->Pnr, -pNode->Qnr);
				cplx Ysl = Sl / conj(Unode) / Unode;
				cplx newU = (cplx(- pNode->Pgr - pMatrixInfo->UncontrolledP, -(- pNode->Qgr - pMatrixInfo->UncontrolledQ)));
				newU /= conj(Unode);
				newU -= cplx(dP, dQ);
				newU /= (pNode->YiiSuper - Ysl);
				pNode->Vre = newU.real();
				pNode->Vim = newU.imag();
				*/

				cplx dU = I1 * cplx(Pe, -Qe);
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

void CLoadFlow::BuildMatrix()
{
	double *pb = klu.B();
	double *pAx = klu.Ax();

	// обходим только узлы в матрице
	for (_MatrixInfo *pMatrixInfo = m_pMatrixInfo.get(); pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
	{
		// здесь считаем, что нагрузка СХН в Node::pnr/Node::qnr уже в расчете и анализе небалансов
		CDynaNodeBase *pNode = pMatrixInfo->pNode;
		GetPnrQnrSuper(pNode);
		// учитываем неуправляемую генерацию в суперузле - UncontrolledP и
		// далее UncontrolledQ
		cplx Sneb;
		double dPdDelta(0.0), dQdDelta(0.0);
		double dPdV = pNode->GetSelfdPdV(), dQdV(1.0);
		double *pAxSelf = pAx;
		pAx += 2;
		cplx UnodeConj(pNode->Vre, -pNode->Vim);
		if (pNode->IsLFTypePQ())
		{
			// для PQ-узлов формируем оба уравнения

			dQdV = pNode->GetSelfdQdV();
			for (VirtualBranch *pBranch = pMatrixInfo->pNode->m_VirtualBranchBegin; pBranch < pMatrixInfo->pNode->m_VirtualBranchEnd; pBranch++)
			{
				CDynaNodeBase *pOppNode = pBranch->pNode;
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
			for (VirtualBranch *pBranch = pMatrixInfo->pNode->m_VirtualBranchBegin; pBranch < pMatrixInfo->pNode->m_VirtualBranchEnd; pBranch++)
			{
				CDynaNodeBase *pOppNode = pBranch->pNode;
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

		Sneb -= std::conj(UnodeConj) * pNode->YiiSuper;
		Sneb = std::conj(Sneb) * std::conj(UnodeConj) + cplx(pNode->Pnr - pNode->Pgr - pMatrixInfo->UncontrolledP, pNode->Qnr - pNode->Qgr - pMatrixInfo->UncontrolledQ);

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
	CDynaNodeBase* pNode = pMatrixInfo->pNode;
	// нагрузка по СХН
	GetPnrQnrSuper(pNode);
	//  в небалансе учитываем неконтролируемую генерацию в суперузлах
	pMatrixInfo->m_dImbP = pMatrixInfo->m_dImbQ = 0.0;  
	cplx i;
	for (VirtualBranch* pBranch = pMatrixInfo->pNode->m_VirtualBranchBegin; pBranch < pMatrixInfo->pNode->m_VirtualBranchEnd; pBranch++)
	{
		CDynaNodeBase* pOppNode = pBranch->pNode;
		i -= cplx(pOppNode->Vre, pOppNode->Vim) * pBranch->Y;
	}

	cplx Vnode(pNode->Vre, pNode->Vim);
	i -= Vnode * pNode->YiiSuper;
	i = std::conj(i) * Vnode;
	i += cplx(pNode->Pnr - pNode->Pgr - pMatrixInfo->UncontrolledP, pNode->Qnr - pNode->Qgr - pMatrixInfo->UncontrolledQ);

	pMatrixInfo->m_dImbP = i.real();
	pMatrixInfo->m_dImbQ = i.imag();

}
bool CLoadFlow::Run()
{
	m_Parameters.m_bFlat = true;
	m_Parameters.m_bStartup = true;
	m_Parameters.m_Imb = m_pDynaModel->GetAtol() * 0.1;

	bool bRes = true;

	pNodes = static_cast<CDynaNodeContainer*>(m_pDynaModel->GetDeviceContainer(DEVTYPE_NODE));
	if (!pNodes)
		throw dfw2error(_T("CLoadFlow::Start - node container unavailable"));

	// вводим СХН УР и V0 = Unom
	pNodes->SwitchLRCs(false);

	Start();
	Estimate();

	if (m_Parameters.m_bStartup)
		Seidell();

	if (0)
	{
		NewtonTanh();
		CheckFeasible();
		for (auto&& it : *pNodes)
			static_cast<CDynaNodeBase*>(it)->StartLF(false, m_Parameters.m_Imb);
	}

	// если использовался стартовый метод, была коррекция 
	// отрицательных сопротивлений, поэтому восстанавливаем
	// исходные сопротивления
	if (m_Parameters.m_bStartup)
		pNodes->CalculateSuperNodesAdmittances(false);

	Newton();

#ifdef _DEBUG
	CompareWithRastr();
#endif

	pNodes->SwitchLRCs(true);

	UpdateQToGenerators();
	DumpNodes();
	CheckFeasible();
	return bRes;
}

struct NodePair
{
	const CDynaNodeBase *pNodeIp;
	const CDynaNodeBase *pNodeIq;
	bool operator < (const NodePair& rhs) const
	{
		ptrdiff_t nDiff = pNodeIp - rhs.pNodeIp;

		if (!nDiff)
			return pNodeIq < rhs.pNodeIq;

		return nDiff < 0;
	}

	NodePair(const CDynaNodeBase *pIp, const CDynaNodeBase *pIq) : pNodeIp(pIp), pNodeIq(pIq)
	{
		if (pIp > pIq)
			std::swap(pNodeIp, pNodeIq);
	}
};


bool CLoadFlow::CheckLF()
{
	bool bRes = true;

	std::set <NodePair> ReportedBranches;

	for (_MatrixInfo *pMatrixInfo = m_pMatrixInfo.get(); pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
	{
		CDynaNodeBase *pNode = pMatrixInfo->pNode;
		double dV = pNode->V / pNode->Unom;
		if (dV > 2.0)
		{
			pNode->Log(CDFW2Messages::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszLFNodeVTooHigh, pNode->GetVerbalName(), dV));
			bRes = false;
		}
		else if (dV < 0.5)
		{
			pNode->Log(CDFW2Messages::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszLFNodeVTooLow, pNode->GetVerbalName(), dV));
			bRes = false;
		}

		for (VirtualBranch *pBranch = pMatrixInfo->pNode->m_VirtualBranchBegin; pBranch < pMatrixInfo->pNode->m_VirtualBranchEnd; pBranch++)
		{
			double dDelta = pNode->Delta - pBranch->pNode->Delta;

			if (dDelta > M_PI_2 || dDelta < -M_PI_2)
			{
				bRes = false;
				if (ReportedBranches.insert(NodePair(pNode, pBranch->pNode)).second)
					m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, fmt::format(
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
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(it);

		if (!pNode->IsStateOn())
			continue;

		// проходим по генераторам, подключенным к узлу
		CLinkPtrCount *pGenLink = pNode->GetLink(1);
		CDevice **ppGen(nullptr);
		if (pGenLink->m_nCount)
		{
			pNode->Pg = pNode->Qg = 0.0;
			pNode->LFQmin = pNode->LFQmax = 0.0;
			while (pGenLink->In(ppGen))
			{
				CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*ppGen);
				if (pGen->IsStateOn())
				{
					if (pGen->Kgen <= 0)
					{
						pGen->Log(CDFW2Messages::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszWrongGeneratorsNumberFixed, 
																			  pGen->GetVerbalName(), 
																			  pGen->Kgen));
						pGen->Kgen = 1.0;
					}
					pNode->Pg += pGen->P;
					pNode->Qg += pGen->Q;
					pNode->LFQmin += pGen->LFQmin * pGen->Kgen;
					pNode->LFQmax += pGen->LFQmax * pGen->Kgen;
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
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(it);

		if (!pNode->IsStateOn())
			continue;

#ifdef _DEBUG
		if (!pNode->m_pSuperNodeParent)
		{
			pNode->GetPnrQnrSuper();
			_MatrixInfo mx;
			mx.pNode = pNode;
			GetNodeImb(&mx);
			mx.pNode = pNode;
			_ASSERTE(fabs(mx.m_dImbP) < m_Parameters.m_Imb && fabs(mx.m_dImbQ) < m_Parameters.m_Imb);
			pNode->GetPnrQnr();
		}
#endif

		CLinkPtrCount *pGenLink = pNode->GetLink(1);
		CDevice **ppGen(nullptr);
		if (pGenLink->m_nCount)
		{
			double Qrange = pNode->LFQmax - pNode->LFQmin;
			double Qspread(0.0), Qgmin(0.0), Qgmax(0.0);
			while (pGenLink->In(ppGen))
			{
				CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*ppGen);
				pGen->Q = 0.0;
				if (pGen->IsStateOn())
				{
					if (Qrange > 0.0)
					{
						double OldQ = pGen->Q;
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
					else
						pGen->Q = 0.0;

					Qspread += pGen->Q;
				}
			}
			if (fabs(pNode->Qg - Qspread) > m_Parameters.m_Imb)
			{
				pNode->Log(CDFW2Messages::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszLFWrongQrangeForNode, 
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

void CLoadFlow::GetPnrQnrSuper(CDynaNodeBase *pNode)
{
	GetPnrQnr(pNode);
	CLinkPtrCount *pLink = pNode->GetSuperLink(0);
	CDevice **ppDevice(nullptr);
	while (pLink->In(ppDevice))
	{
		CDynaNodeBase *pSlaveNode = static_cast<CDynaNodeBase*>(*ppDevice);
		GetPnrQnr(pSlaveNode);
		pNode->Pnr += pSlaveNode->Pnr;
		pNode->Qnr += pSlaveNode->Qnr;
		pNode->dLRCPn += pSlaveNode->dLRCPn;
		pNode->dLRCQn += pSlaveNode->dLRCQn;
	}
}

void CLoadFlow::GetPnrQnr(CDynaNodeBase *pNode)
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
		pNode->Pnr *= pNode->m_pLRC->GetPdP(VdVnom, pNode->dLRCPn, pNode->dLRCVicinity);
		pNode->Qnr *= pNode->m_pLRC->GetQdQ(VdVnom, pNode->dLRCQn, pNode->dLRCVicinity);
		pNode->dLRCPn *= pNode->Pn / pNode->V0;
		pNode->dLRCQn *= pNode->Qn / pNode->V0;
	}
}

void CLoadFlow::DumpNodes()
{
	FILE *fdump(nullptr);
	setlocale(LC_ALL, "ru-ru");
	if (!_tfopen_s(&fdump, _T("c:\\tmp\\resnodes.csv"), _T("wb+")))
	{
		_ftprintf(fdump, _T("N;V;D;Pn;Qn;Pnr;Qnr;Pg;Qg;Type;Qmin;Qmax;Vref;VR;DeltaR;QgR;QnR\n"));
		for (auto&& it : pNodes->m_DevVec)
		{
			CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(it);
#ifdef _DEBUG
			///*
			_ftprintf(fdump, _T("%td;%.12g;%.12g;%g;%g;%g;%g;%g;%g;%d;%g;%g;%g;%.12g;%.12g;%.12g;%.12g\n"),
				pNode->GetId(),
				pNode->V.Value,
				pNode->Delta.Value / M_PI * 180.0,
				pNode->Pn,
				pNode->Qn,
				pNode->Pnr,
				pNode->Qnr,
				pNode->Pg,
				pNode->Qg,
				pNode->m_eLFNodeType,
				pNode->LFQmin,
				pNode->LFQmax,
				pNode->LFVref,
				pNode->Vrastr,
				pNode->Deltarastr / M_PI * 180.0,
				pNode->Qgrastr,
				pNode->Qnrrastr);
			//*/
			/*
			_ftprintf(fdump, _T("%d;%.12g;%.12g;%.12g;%.12g;%.12g;%.12g\n"),
				pNode->GetId(),
				pNode->V,
				pNode->Delta / M_PI * 180.0,
				pNode->Pnr,
				pNode->Qnr,
				pNode->Pg,
				pNode->Qg);*/
#else
			_ftprintf(fdump, _T("%td;%.12g;%.12g;%g;%g;%g;%g;%g;%g;%d;%g;%g;%g\n"),
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
				pNode->LFVref);
#endif
		}
		fclose(fdump);
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
	m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, CDFW2Messages::m_cszLFRunningNewton);
	int it(0);	// количество итераций

	// вектор для указателей переключаемых узлов, с размерностью в половину уравнений матрицы
	MATRIXINFO PV_PQmax, PV_PQmin, PQmax_PV, PQmin_PV;


	PV_PQmax.reserve(klu.MatrixSize() / 2);
	PV_PQmin.reserve(klu.MatrixSize() / 2);
	PQmax_PV.reserve(klu.MatrixSize() / 2);
	PQmin_PV.reserve(klu.MatrixSize() / 2);

	// квадраты небалансов до и после итерации
	double g0(0.0), g1(0.1), lambda(1.0);
	m_nNodeTypeSwitchesDone = 1;

	while (1)
	{
		if (!CheckLF())
			throw dfw2error(CDFW2Messages::m_cszUnacceptableLF);

		if (++it > m_Parameters.m_nMaxIterations)
			throw dfw2error(CDFW2Messages::m_cszLFNoConvergence);

		pNodes->m_IterationControl.Reset();
		PV_PQmax.clear();	// сбрасываем список переключаемых узлов чтобы его обновить
		PV_PQmin.clear();
		PQmax_PV.clear();
		PQmin_PV.clear();

		// считаем небаланс по всем узлам кроме БУ
		_MatrixInfo *pMatrixInfo = m_pMatrixInfo.get();
		for (; pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
		{
			CDynaNodeBase *pNode = pMatrixInfo->pNode;
			GetNodeImb(pMatrixInfo);	// небаланс считается с учетом СХН
			pNodes->m_IterationControl.Update(pMatrixInfo);
			double Qg = pNode->Qgr + pMatrixInfo->m_dImbQ;
			pMatrixInfo->NodeViolation = pNode->V - pNode->LFVref;
			switch (pNode->m_eLFNodeType)
			{
				// если узел на минимуме Q и напряжение ниже уставки, он должен стать PV
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMIN:
				if (pMatrixInfo->NodeViolation < 0.0)
				{
					PQmax_PV.push_back(pMatrixInfo);
					pMatrixInfo->m_nPVSwitchCount++;
				}
				break;
				// если узел на максимуме Q и напряжение выше уставки, он должен стать PV
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMAX:
				if (pMatrixInfo->NodeViolation > 0.0)
				{
					PQmin_PV.push_back(pMatrixInfo);
					pMatrixInfo->m_nPVSwitchCount++;
				}
				break;
				// если узел PV, но реактивная генерация вне диапазона, делаем его PQ
			case CDynaNodeBase::eLFNodeType::LFNT_PV:
				if ((pMatrixInfo->NodeViolation = Qg - pNode->LFQmax) > 0.0)
				{
					PV_PQmax.push_back(pMatrixInfo);
				}
				else if ((pMatrixInfo->NodeViolation = Qg - pNode->LFQmin) < 0.0)
				{
					PV_PQmin.push_back(pMatrixInfo);
				}
				else
					pNode->Qgr = Qg;	// если реактивная генерация в пределах - обновляем ее значение в узле
				break;
			}
		}

		pNodes->m_IterationControl.m_nQviolated = PV_PQmax.size() + PV_PQmin.size() + PQmax_PV.size() + PQmin_PV.size();

		UpdateSlackBusesImbalance();

		g1 = GetSquaredImb();

		pNodes->m_IterationControl.m_ImbRatio = g1;

		if(g0 > 0.0)
			pNodes->m_IterationControl.m_ImbRatio = g1 / g0;

		DumpNewtonIterationControl();
		if (pNodes->m_IterationControl.Converged(m_Parameters.m_Imb))
			break;

		if (!m_nNodeTypeSwitchesDone && g1 > g0)
		{
			double gs1v = -CDynaModel::gs1(klu, m_Rh, klu.B());
			// знак gs1v должен быть "-" ????
			lambda *= -0.5 * gs1v / (g1 - g0 - gs1v);
			RestoreVDelta();
			UpdateVDelta(lambda);
			m_NewtonStepRatio.m_dRatio = lambda;
			m_NewtonStepRatio.m_eStepCause = LFNewtonStepRatio::eStepLimitCause::Backtrack;
			continue;
		}

		lambda = 1.0;
		g0 = g1;

		// сохраняем исходные значения переменных
		StoreVDelta();

		m_nNodeTypeSwitchesDone = 0;

		for (auto&& it : PQmax_PV)
		{
			CDynaNodeBase*& pNode = it->pNode;
			pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PV;
			pNode->V = pNode->LFVref;
			pNode->UpdateVreVimSuper();
			// считаем количество переключений типов узлов
			// чтобы исключить выбор шага по g1 > g0
			m_nNodeTypeSwitchesDone++;
		}

		if (PQmax_PV.empty())
		{
			for (auto&& it : PQmin_PV)
			{
				CDynaNodeBase*& pNode = it->pNode;
				pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PV;
				pNode->V = pNode->LFVref;
				pNode->UpdateVreVimSuper();
				m_nNodeTypeSwitchesDone++;
			}
		}

		if (std::abs(pNodes->m_IterationControl.m_MaxImbP.GetDiff()) < 10.0 * m_Parameters.m_Imb)
		{
			for (auto&& it : PV_PQmax)
			{
				CDynaNodeBase*& pNode = it->pNode;
				pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMAX;
				pNode->Qgr = pNode->LFQmax;
				m_nNodeTypeSwitchesDone++;
			}

			if (PV_PQmax.empty())
			{
				for (auto&& it : PV_PQmin)
				{
					CDynaNodeBase*& pNode = it->pNode;
					pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMIN;
					pNode->Qgr = pNode->LFQmin;
					m_nNodeTypeSwitchesDone++;
				}
			}
		}

		BuildMatrix();
		// сохраняем небаланс до итерации
		std::copy(klu.B(), klu.B() + klu.MatrixSize(), m_Rh.get());

		ptrdiff_t iMax(0);
		double maxb = klu.FindMaxB(iMax);
		CDynaNodeBase *pNode1(m_pMatrixInfo.get()[iMax / 2].pNode);

		SolveLinearSystem();

		maxb = klu.FindMaxB(iMax);
		CDynaNodeBase *pNode2(m_pMatrixInfo.get()[iMax / 2].pNode);



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

	for (_MatrixInfo *pMatrixInfo = m_pMatrixInfo.get() ; pMatrixInfo < m_pMatrixInfoSlackEnd ; pMatrixInfo++)
	{
		CDynaNodeBase*& pNode = pMatrixInfo->pNode;
		// диапазон реактивной мощности узла
		double Qrange = pNode->LFQmax - pNode->LFQmin;
		double Qspread(0.0), PgSource(pNode->Pgr), QgSource(pNode->Qgr), DropToSlack(0.0);
		CLinkPtrCount *pLink = pNode->GetSuperLink(0);
		CDevice **ppDevice(nullptr);
		std::list<CDynaNodeBase*> SlackBuses;

		if (!pNode->m_pSuperNodeParent)
		{
			GetNodeImb(pMatrixInfo);
			_ASSERTE(fabs(pMatrixInfo->m_dImbP) < m_Parameters.m_Imb && fabs(pMatrixInfo->m_dImbQ) < m_Parameters.m_Imb);
		}

		switch (pNode->m_eLFNodeType)
		{
			case CDynaNodeBase::eLFNodeType::LFNT_PQ:
				// если узел нагрузочный в нем ничего не может измениться
				break;
			case CDynaNodeBase::eLFNodeType::LFNT_BASE:
				// если суперузел базисный, вносим его в список базисных узлов данного суперузла
				SlackBuses.push_back(pNode);
				// и перечисляем подчиненные узлы
				while (pLink->In(ppDevice))
				{
					CDynaNodeBase *pSlaveNode = static_cast<CDynaNodeBase*>(*ppDevice);
					if (pSlaveNode->IsStateOn())
						if (pSlaveNode->m_eLFNodeType == CDynaNodeBase::eLFNodeType::LFNT_BASE)
							SlackBuses.push_back(pSlaveNode);
				}
				// включенные базисные узлы нашли, теперь в них нужно распределить "излишки" реактивной мощности от генераторов
				// и активную мощность
				DropToSlack = pNode->Qgr - pNode->LFQmax;

				// если генерация базисного суперузла превышает максимальную - то сбрасываем излишки в подчиненные базисные узлы
				if (DropToSlack < 0)
				{
					DropToSlack = pNode->Qgr - pNode->LFQmin;
					// если генерация в пределах - в подчиненных БУ оставляем нули
					if (DropToSlack > 0)
						DropToSlack = 0.0;
				}

				// все что сбросили в БУ изымаем из  общей генерации узла
				QgSource -= DropToSlack;

				for (auto&& sb : SlackBuses)
				{
					sb->Pgr = sb->Pg = PgSource / SlackBuses.size();
					sb->Qgr = sb->Qg = DropToSlack / SlackBuses.size();
				}

			default:
				// все остальные узлы - PV, PVQmin, PVQmax обрабатываем по общим правилам
				Qrange = (Qrange > 0.0) ? (QgSource - pNode->LFQmin) / Qrange : 0.0;
				ppDevice = nullptr;
				while (pLink->In(ppDevice))
				{
					CDynaNodeBase *pSlaveNode = static_cast<CDynaNodeBase*>(*ppDevice);
					// распределяем реактивную мощность по ненагрузочным узлам
					if (pSlaveNode->IsStateOn())
					{
						if (pSlaveNode->m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_PQ && pSlaveNode->m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_BASE)
							Qspread += pSlaveNode->Qg = pSlaveNode->Qgr = pSlaveNode->LFQmin + (pSlaveNode->LFQmax - pSlaveNode->LFQmin) * Qrange;
					}
					else
						pSlaveNode->Qg = pSlaveNode->Qgr = 0.0;
				}

				if (pNode->m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_BASE)
					Qspread += pNode->Qgr = pNode->Qg = pMatrixInfo->LFQmin + (pMatrixInfo->LFQmax - pMatrixInfo->LFQmin) * Qrange;

				if (fabs(Qspread - QgSource) > m_Parameters.m_Imb)
				{
					bAllOk = false;
					pNode->Log(CDFW2Messages::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszLFWrongQrangeForSuperNode,
						pNode->GetVerbalName(),
						QgSource,
						pNode->LFQmin,
						pNode->LFQmax));
				}

		}
		pMatrixInfo->Restore();
	}

	if (!bAllOk)
		throw dfw2error(CDFW2Messages::m_cszLFError);

	for (auto&& it : pNodes->m_DevVec)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(it);
		if (!pNode->m_pSuperNodeParent)
		{
			/*// если узел не нагрузочный, обновляем расчетное значение реактивной мощности
			if (pNode->m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_PQ)
				pNode->Qg = pNode->IsStateOn() ? pNode->Qgr : 0.0;*/
			// восстанавливаем исходные значения расчетной генерации и нагрузки по сохраненным Pg,Qg,Pn,Qn
			GetPnrQnr(pNode);
		}
	}
}

// возвращает максимальное отношение V/Unom


double CLoadFlow::GetNewtonRatio()
{
	m_NewtonStepRatio.Reset();

	_MatrixInfo* pMatrixInfo = m_pMatrixInfo.get();
	double* b = klu.B();

	for (; pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
	{
		CDynaNodeBase* pNode = pMatrixInfo->pNode;
		double* pb = b + pNode->A(0);
		double newDelta = pNode->Delta - *pb;	
		
		// поиск ограничения шага по углу узла
		double Dstep = std::abs(*pb);
		if (Dstep > m_Parameters.m_dNodeAngleNewtonStep)
			m_NewtonStepRatio.UpdateNodeAngle(std::abs(m_Parameters.m_dVoltageNewtonStep / Dstep), pNode);

		pb++;
		double newV = pNode->V - *pb;
		double Ratio = newV / pNode->Unom;

		// поиск ограничения по модулю напряжения
		for (const double d : {0.5, 2.0})
		{
			if ((d > 1.0) ? Ratio > d : Ratio < d)
				m_NewtonStepRatio.UpdateVoltageOutOfRange((pNode->V - d * pNode->Unom) / *pb, pNode);
		}

		// поиск ограничения шага по напряжению
		double VstepPU = std::abs(*pb) / pNode->Unom;
		if (VstepPU > m_Parameters.m_dVoltageNewtonStep)
			m_NewtonStepRatio.UpdateVoltage(m_Parameters.m_dVoltageNewtonStep / VstepPU, pNode);
	}

	// поиск ограничения по взимному углу
	for (auto&& it : m_BranchAngleCheck)
	{
		CDynaBranch* pBranch = static_cast<CDynaBranch*>(it);
		CDynaNodeBase* pNodeIp(pBranch->m_pNodeIp);
		CDynaNodeBase* pNodeIq(pBranch->m_pNodeIq);
		double CurrentDelta = pNodeIp->Delta - pNodeIq->Delta;
		double DeltaDiff = *(b + pNodeIp->A(0)) - *(b + pNodeIq->A(0));
		double NewDelta = CurrentDelta + DeltaDiff;
		double AbsDeltaDiff = std::abs(DeltaDiff);

		if (std::abs(NewDelta) > M_PI_2) 
			m_NewtonStepRatio.UpdateBranchAngleOutOfRange( ((std::signbit(NewDelta) ? -M_PI_2 : M_PI_2) - CurrentDelta) / DeltaDiff, pNodeIp, pNodeIq);

		if (AbsDeltaDiff > m_Parameters.m_dBranchAngleNewtonStep)
			m_NewtonStepRatio.UpdateBranchAngle(m_Parameters.m_dBranchAngleNewtonStep / AbsDeltaDiff, pNodeIp, pNodeIq);
	}

	return m_NewtonStepRatio.m_dRatio;
}

void CLoadFlow::UpdateVDelta(double dStep)
{
	_MatrixInfo *pMatrixInfo = m_pMatrixInfo.get();
	double *b = klu.B();

	double MaxRatio = 1.0;

	for (; pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
	{
		CDynaNodeBase *pNode = pMatrixInfo->pNode;
		double *pb = b + pNode->A(0);
		double newDelta = pNode->Delta - *pb * dStep;		pb++;
		double newV		= pNode->V - *pb * dStep;

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
		pNode->V	 = newV;
		pNode->UpdateVreVimSuper();
	}
}


void CLoadFlow::StoreVDelta()
{
	if(!m_Vbackup)
		m_Vbackup = std::make_unique<double[]>(klu.MatrixSize());
	if(!m_Dbackup)
		m_Dbackup = std::make_unique<double[]>(klu.MatrixSize());

	double* pV = m_Vbackup.get();
	double* pD = m_Dbackup.get();
	for (_MatrixInfo *pMatrix = m_pMatrixInfo.get(); pMatrix < m_pMatrixInfoEnd; pMatrix++, pV++, pD++)
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
		for (_MatrixInfo *pMatrix = m_pMatrixInfo.get(); pMatrix < m_pMatrixInfoEnd; pMatrix++, pV++, pD++)
		{
			pMatrix->pNode->V = *pV;
			pMatrix->pNode->Delta = *pD;
		}
	}
	else
		throw dfw2error(_T("CLoadFlow::RestoreVDelta called before StoreVDelta"));
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
	for (auto&& it : *pNodes)
	{
		CDynaNodeBase* pNode(static_cast<CDynaNodeBase*>(it));
		if (pNode->IsStateOn())
		{
			
			if (!pNode->IsLFTypePQ())
			{
				// если узел входит в суперузел его заданное напряжение в расчете было равно заданному напряжению суперузла,
				// которое, в свою очередь, было выбрано по узлу с наиболее широким диапазоном реактивной мощности внутри суперузла
				double LFVref = pNode->m_pSuperNodeParent ? pNode->m_pSuperNodeParent->LFVref : pNode->LFVref;
				if (pNode->V > LFVref && pNode->Qgr >= pNode->LFQmax)
					pNode->Log(CDFW2Messages::DFW2LOG_DEBUG, fmt::format(_T("Infeasible {}"), pNode->GetVerbalName()));
				if (pNode->V < LFVref && pNode->Qgr <= pNode->LFQmin)
					pNode->Log(CDFW2Messages::DFW2LOG_DEBUG, fmt::format(_T("Infeasible {}"), pNode->GetVerbalName()));
				if (pNode->Qgr < pNode->LFQmin)
					pNode->Log(CDFW2Messages::DFW2LOG_DEBUG, fmt::format(_T("Infeasible {}"), pNode->GetVerbalName()));
				if (pNode->Qgr > pNode->LFQmax)
					pNode->Log(CDFW2Messages::DFW2LOG_DEBUG, fmt::format(_T("Infeasible {}"), pNode->GetVerbalName()));
			}

			pNode->SuperNodeLoadFlow(m_pDynaModel);
		}
	}
}

void CLoadFlow::UpdateSlackBusesImbalance()
{
	// досчитываем небалансы в БУ
	for (_MatrixInfo *pMatrixInfo = m_pMatrixInfoEnd; pMatrixInfo < m_pMatrixInfoSlackEnd; pMatrixInfo++)
	{
		CDynaNodeBase* pNode = pMatrixInfo->pNode;
		GetNodeImb(pMatrixInfo);
		// для БУ небалансы только для результатов
		pNode->Pgr += pMatrixInfo->m_dImbP;
		pNode->Qgr += pMatrixInfo->m_dImbQ;
		// в контроле сходимости небаланс БУ всегда 0.0
		pMatrixInfo->m_dImbP = pMatrixInfo->m_dImbQ = 0.0;
		pNodes->m_IterationControl.Update(pMatrixInfo);
	}
}



void CLoadFlow::DumpNewtonIterationControl()
{
	const _TCHAR* causes[] = {_T(""), _T("dV"), _T("dD"), _T("dB"), _T("vV"), _T("vD"), _T("Bt")};
	m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, fmt::format(_T("{} {:15f} {:6.2} {:4}"), pNodes->GetIterationControlString(), 
																	  pNodes->m_IterationControl.m_ImbRatio,
																	  m_NewtonStepRatio.m_dRatio,
																	  causes[static_cast<ptrdiff_t>(m_NewtonStepRatio.m_eStepCause)]));
}