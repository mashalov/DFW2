#include "stdafx.h"
#include "LoadFlow.h"
#include "DynaModel.h"

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

void CLoadFlow::Estimate()
{
	ptrdiff_t nMatrixSize(0), nNonZeroCount(0);
	// создаем привязку узлов к информации по строкам матрицы
	// размер берем по количеству узлов. Реальный размер матрицы будет меньше на
	// количество отключенных узлов и БУ
	m_pMatrixInfo = make_unique<_MatrixInfo[]>(pNodes->Count());
	_MatrixInfo *pMatrixInfo = m_pMatrixInfo.get();

	// базисные узлы держим в отдельном списке, так как они не в матрице, но
	// результаты по ним нужно обновлять
	list<CDynaNodeBase*> SlackBuses;

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
			pMatrixInfo->pNode = pNode;
			nMatrixSize += 2;				// на узел 2 уравнения в матрице
			pMatrixInfo->nRowCount += 2;	// считаем диагональный элемент
			nNonZeroCount += 2 * pMatrixInfo->nRowCount;	// количество ненулевых элементов увеличиваем на количество подсчитанных элементов в строке (4 double на элемент)
			pMatrixInfo++;
		}
	}

	m_pMatrixInfoEnd = pMatrixInfo;								// конец инфо по матрице для узлов, которые в матрице
	if (!nMatrixSize)
		throw dfw2error(CDFW2Messages::m_cszNoNodesForLF);
	klu.SetSize(nMatrixSize, nNonZeroCount);

	ptrdiff_t *pAi = klu.Ai();
	ptrdiff_t *pAp = klu.Ap();
	ptrdiff_t nRowPtr = 0;

	for (_MatrixInfo *pMatrixInfo = m_pMatrixInfo.get(); pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
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

	for (auto&& sit : SlackBuses)
	{
		CDynaNodeBase *pNode = sit;
		pMatrixInfo->pNode = pNode;
		pMatrixInfo++;
	}

	m_pMatrixInfoSlackEnd = pMatrixInfo;
	*pAi = nNonZeroCount;
	m_Rh = make_unique<double[]>(klu.MatrixSize());		// невязки до итерации
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
	pNodes = static_cast<CDynaNodeContainer*>(m_pDynaModel->GetDeviceContainer(DEVTYPE_NODE));
	if (!pNodes)
		throw dfw2error(_T("CLoadFlow::Start - node container unavailable"));

	pNodes->PrepareLFTopology();
	pNodes->CreateSuperNodes();

	// обновляем данные в PV-узлах по заданным в генераторах реактивным мощностям
	UpdatePQFromGenerators();

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

	for (auto&& it : *pNodes)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(it);
		if (!pNode->IsStateOn() || pNode->m_pSuperNodeParent)
			continue;
		CLinkPtrCount *pNodeLink = pNode->GetSuperLink(0);
		CDevice **ppDevice(nullptr);
		double QrangeMax = -1.0;
		// если в узел входят другие узлы, сохраняем исходные параметры 
		// суперузла
		if (pNodeLink->m_nCount)
			m_SuperNodeParameters.push_back(StoreParameters(pNode));

		while (pNodeLink->In(ppDevice))
		{
			CDynaNodeBase *pSlaveNode = static_cast<CDynaNodeBase*>(*ppDevice);
			// в суперузел суммируем все мощности входящих узлов
			pNode->Pgr += pSlaveNode->Pgr;
			pNode->Qgr += pSlaveNode->Qgr;
			// здесь вопрос: если входящий в суперузел имеет диапазон регулирования, но не имеет уставки,
			// этот диапазон суммируется в суперузел или нет ? Сделан вариант суммирования
			// только полноценных PV узлов
			if (pSlaveNode->m_eLFNodeType == CDynaNodeBase::eLFNodeType::LFNT_BASE)
			{
				// если в суперузел входит базисный узел, весь суперузел становится базисным
				// можно попроверять соответствие напряжений нескольких базисных узлов
				pNode->m_eLFNodeType = pSlaveNode->m_eLFNodeType;
				pNode->V = pSlaveNode->V;
				pNode->Delta = pSlaveNode->Delta;
			}
			else if (!pSlaveNode->IsLFTypePQ())
			{
				// если в суперузел входит узел с заданной генерацией
				pNode->LFQmin += pSlaveNode->LFQmin;
				pNode->LFQmax += pSlaveNode->LFQmax;
				double Qrange = pSlaveNode->LFQmax - pSlaveNode->LFQmin;
				// выбираем заданное напряжение по узлу с максимальным диапазоном Qmin/Qmax
				if (QrangeMax < 0 || QrangeMax < Qrange)
				{
					pNode->LFVref = pSlaveNode->LFVref;
					QrangeMax = Qrange;
				}
			}
			else
				QrangeMax = QrangeMax;
		}
		pNode->StartLF(m_Parameters.m_bFlat, m_Parameters.m_Imb);
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

	// рассчитываем проводимости узлов с устранением отрицательных сопротивлений
	pNodes->CalcAdmittances(true);
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

			// для всех узлов кроме БУ обновляем статистику итерации
			pNodes->m_IterationControl.Update(pMatrixInfo);
		}

		if (!CheckLF())
			// если итерация привела не недопустимому режиму - выходим
			throw dfw2error(CDFW2Messages::m_cszUnacceptableLF);

		pNodes->DumpIterationControl();

		// если достигли заданного небаланса - выходим
		if (pNodes->m_IterationControl.Converged(m_Parameters.m_Imb))
			break;
	}

	// пересчитываем проводимости узлов без устранения отрицательных сопротивлений
	pNodes->CalcAdmittances(false);

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
		double Pe = pNode->GetSelfImbP(), Qe(0.0);
		double dPdDelta(0.0), dQdDelta(0.0);
		double dPdV = pNode->GetSelfdPdV(), dQdV(1.0);
		double *pAxSelf = pAx;
		pAx += 2;
		cplx Unode(pNode->Vre, pNode->Vim);
		if (pNode->IsLFTypePQ())
		{
			// для PQ-узлов формируем оба уравнения
			Qe = pNode->GetSelfImbQ();
			dQdV = pNode->GetSelfdQdV();
			for (VirtualBranch *pBranch = pMatrixInfo->pNode->m_VirtualBranchBegin; pBranch < pMatrixInfo->pNode->m_VirtualBranchEnd; pBranch++)
			{
				CDynaNodeBase *pOppNode = pBranch->pNode;
				cplx mult = conj(Unode);
				mult *= cplx(pOppNode->Vre, pOppNode->Vim) * pBranch->Y;
				Pe -= mult.real();
				Qe += mult.imag();

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
				cplx mult = conj(Unode);
				mult *= cplx(pOppNode->Vre, pOppNode->Vim) * pBranch->Y;
				Pe -= mult.real();
				dPdDelta -= mult.imag();
				dPdV += -CDevice::ZeroDivGuard(mult.real(), pNode->V);

				if (NodeInMatrix(pOppNode))
				{
					double dPdDeltaM = mult.imag();
					double dPdVM = -CDevice::ZeroDivGuard(mult.real(), pOppNode->V);
					*pAx = dPdDeltaM;
					*(pAx + pMatrixInfo->nRowCount) = 0.0;
					pAx++;
					*pAx = dPdVM;
					*(pAx + pMatrixInfo->nRowCount) = 0.0;
					pAx++;
				}
			}
		}

		*pAxSelf = dPdDelta;
		*(pAxSelf + pMatrixInfo->nRowCount) = dQdDelta;
		pAxSelf++;
		*pAxSelf = dPdV;
		*(pAxSelf + pMatrixInfo->nRowCount) = dQdV;

		pAx += pMatrixInfo->nRowCount;

		*pb = Pe; pb++;
		*pb = Qe; pb++;
	}
}

// расчет небаланса в узле
void CLoadFlow::GetNodeImb(_MatrixInfo *pMatrixInfo)
{
	CDynaNodeBase *pNode = pMatrixInfo->pNode;
	// нагрузка по СХН
	GetPnrQnrSuper(pNode);
	pMatrixInfo->m_dImbP = pNode->GetSelfImbP();
	pMatrixInfo->m_dImbQ = pNode->GetSelfImbQ();
	cplx Unode(pNode->Vre, pNode->Vim);
	for (VirtualBranch *pBranch = pMatrixInfo->pNode->m_VirtualBranchBegin; pBranch < pMatrixInfo->pNode->m_VirtualBranchEnd; pBranch++)
	{
		CDynaNodeBase *pOppNode = pBranch->pNode;
		cplx mult = conj(Unode) * cplx(pOppNode->Vre, pOppNode->Vim) * pBranch->Y;
		pMatrixInfo->m_dImbP -= mult.real();
		pMatrixInfo->m_dImbQ += mult.imag();
	}
}

#include "atlbase.h"
bool CLoadFlow::Run()
{
	m_Parameters.m_bFlat = true;
	m_Parameters.m_bStartup = true;
	m_Parameters.m_Imb = m_pDynaModel->GetAtol() * 0.1;

	bool bRes = true;

	Start();
	Estimate();

	pNodes->SwitchLRCs(false);

	if (m_Parameters.m_bStartup)
		Seidell();

	if (0)
	{
		NewtonTanh();
		CheckFeasible();
		for (auto&& it : *pNodes)
			static_cast<CDynaNodeBase*>(it)->StartLF(false, m_Parameters.m_Imb);
	}
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
			swap(pNodeIp, pNodeIq);
	}
};


bool CLoadFlow::CheckLF()
{
	bool bRes = true;

	set <NodePair> ReportedBranches;

	for (_MatrixInfo *pMatrixInfo = m_pMatrixInfo.get(); pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
	{
		CDynaNodeBase *pNode = pMatrixInfo->pNode;
		double dV = pNode->V / pNode->Unom;
		if (dV > 2.0)
		{
			m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, CDFW2Messages::m_cszLFNodeVTooHigh, pNode->GetVerbalName(), dV);
			bRes = false;
		}
		else if (dV < 0.5)
		{
			m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, CDFW2Messages::m_cszLFNodeVTooLow, pNode->GetVerbalName(), dV);
			bRes = false;
		}

		for (VirtualBranch *pBranch = pMatrixInfo->pNode->m_VirtualBranchBegin; pBranch < pMatrixInfo->pNode->m_VirtualBranchEnd; pBranch++)
		{
			double dDelta = pNode->Delta - pBranch->pNode->Delta;

			if (dDelta > M_PI_2 || dDelta < -M_PI_2)
			{
				bRes = false;
				if (ReportedBranches.insert(NodePair(pNode, pBranch->pNode)).second)
					m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, CDFW2Messages::m_cszLFBranchAngleExceeds90, pNode->GetVerbalName(), pBranch->pNode->GetVerbalName(), dDelta * 180.0 / M_PI);
			}
		}
	}
	return bRes;
}


void CLoadFlow::UpdatePQFromGenerators()
{
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
			pNode->ResetVisited();
			while (pGenLink->In(ppGen))
			{
				CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*ppGen);
				if (pGen->IsStateOn())
				{
					if (pGen->Kgen <= 0)
					{
						pGen->Kgen = 1.0;
						pGen->Log(CDFW2Messages::DFW2LOG_WARNING, Cex(CDFW2Messages::m_cszWrongGeneratorsNumberFixed, pGen->GetVerbalName(), pGen->Kgen));
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
	for (auto&& it : pNodes->m_DevVec)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(it);

		if (!pNode->IsStateOn())
			continue;

		CLinkPtrCount *pGenLink = pNode->GetLink(1);
		CDevice **ppGen(nullptr);
		if (pGenLink->m_nCount)
		{
			double Qrange = pNode->LFQmax - pNode->LFQmin;
			pNode->ResetVisited();
			while (pGenLink->In(ppGen))
			{
				CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*ppGen);
				pGen->Q = 0.0;
				if (pGen->IsStateOn())
					if (Qrange > 0.0)
					{
						double OldQ = pGen->Q;
						pGen->Q = pGen->Kgen * (pGen->LFQmin + (pGen->LFQmax - pGen->LFQmin) / Qrange * (pNode->Qg - pNode->LFQmin));
						_CheckNumber(pGen->Q);
						//					_ASSERTE(fabs(pGen->Q - OldQ) < 0.00001);
					}
					else if (pGen->GetType() == eDFW2DEVICETYPE::DEVTYPE_GEN_INFPOWER)
					{
						pGen->Q = pGen->Kgen * pNode->Qg  / pGenLink->m_nCount;
						pGen->P = pGen->Kgen * pNode->Pgr / pGenLink->m_nCount;
					}
					else
						pGen->Q = 0.0;
			}
		}
	}
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
		_ftprintf(fdump, _T("N;V;D;Pn;Qn;Pnr;Qnr;Pg;Qg;Type;Qmin;Qmax;Vref\n"));
		for (auto&& it : pNodes->m_DevVec)
		{
			CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(it);
#ifdef _DEBUG
			///*
			_ftprintf(fdump, _T("%d;%.12g;%.12g;%g;%g;%g;%g;%g;%g;%d;%g;%g;%g;%.12g;%.12g;%g\n"),
				pNode->GetId(),
				pNode->V,
				pNode->Delta / M_PI * 180.0,
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
				pNode->Qgrastr);
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
				pNode->V,
				pNode->Delta / M_PI * 180.0,
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


CLoadFlow::StoreParameters::StoreParameters(CDynaNodeBase *pNode)
{
	m_pNode = pNode;
	LFQmin = pNode->LFQmin;
	LFQmax = pNode->LFQmax;
	LFNodeType = pNode->m_eLFNodeType;
}

void CLoadFlow::StoreParameters::Restore()
{
	m_pNode->LFQmin = LFQmin;
	m_pNode->LFQmax = LFQmax;
	m_pNode->m_eLFNodeType = LFNodeType;
};

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
	vector<_MatrixInfo*> PV_PQmax, PV_PQmin, PQmax_PV, PQmin_PV;
	PV_PQmax.reserve(klu.MatrixSize() / 2);
	PV_PQmin.reserve(klu.MatrixSize() / 2);
	PQmax_PV.reserve(klu.MatrixSize() / 2);
	PQmin_PV.reserve(klu.MatrixSize() / 2);

	// квадраты небалансов до и после итерации
	double g0(0.0), g1(0.1), lambda(1.0);

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
			switch (pNode->m_eLFNodeType)
			{
				// если узел на минимуме Q и напряжение ниже уставки, он должен стать PV
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMIN:
				if (pNode->V < pNode->LFVref)
				{
					PQmax_PV.push_back(pMatrixInfo);
					pMatrixInfo->m_nPVSwitchCount++;
				}
				break;
				// если узел на максимуме Q и напряжение выше уставки, он должен стать PV
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMAX:
				if (pNode->V > pNode->LFVref)
				{
					PQmin_PV.push_back(pMatrixInfo);
					pMatrixInfo->m_nPVSwitchCount++;
				}
				break;
				// если узел PV, но реактивная генерация вне диапазона, делаем его PQ
			case CDynaNodeBase::eLFNodeType::LFNT_PV:
				if (Qg > pNode->LFQmax)
				{
					PV_PQmax.push_back(pMatrixInfo);
				}
				else if (Qg < pNode->LFQmin)
				{
					PV_PQmin.push_back(pMatrixInfo);
				}
				else
					pNode->Qgr = Qg;	// если реактивная генерация в пределах - обновляем ее значение в узле
				break;
			}
		}

		pNodes->m_IterationControl.m_nQviolated = PV_PQmax.size() + PV_PQmin.size() + PQmax_PV.size() + PQmin_PV.size();

		// досчитываем небалансы в БУ
		for (pMatrixInfo = m_pMatrixInfoEnd; pMatrixInfo < m_pMatrixInfoSlackEnd; pMatrixInfo++)
		{
			CDynaNodeBase *pNode = pMatrixInfo->pNode;
			GetNodeImb(pMatrixInfo);
			// для БУ небалансы только для результатов
			pNode->Pgr += pMatrixInfo->m_dImbP;
			pNode->Qgr += pMatrixInfo->m_dImbQ;
			// в контроле сходимости небаланс БУ всегда 0.0
			pMatrixInfo->m_dImbP = pMatrixInfo->m_dImbQ = 0.0;
			pNodes->m_IterationControl.Update(pMatrixInfo);
		}

		g1 = GetSquaredImb();

		pNodes->m_IterationControl.m_ImbRatio = g1;
		pNodes->DumpIterationControl();
		if (pNodes->m_IterationControl.Converged(m_Parameters.m_Imb))
			break;

		if (it > 1 && g1 > g0)
		{
			double gs1v = CDynaModel::gs1(klu, m_Rh, klu.B());
			// знак gs1v должен быть "-" ????
			lambda *= -0.5 * gs1v / (g1 - g0 - gs1v);
			RestoreVDelta();
			UpdateVDelta(lambda);
			continue;
		}

		lambda = 1.0;
		g0 = g1;

		// сохраняем исходные значения переменных
		StoreVDelta();

		for (auto&& it : PQmax_PV)
		{
			CDynaNodeBase*& pNode = it->pNode;
			pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PV;
			pNode->V = pNode->LFVref;
			pNode->UpdateVreVimSuper();
		}

		if (PQmax_PV.empty())
		{
			for (auto&& it : PQmin_PV)
			{
				CDynaNodeBase*& pNode = it->pNode;
				pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PV;
				pNode->V = pNode->LFVref;
				pNode->UpdateVreVimSuper();
			}
		}

		if (pNodes->m_IterationControl.m_MaxImbP.GetDiff() < 10.0 * m_Parameters.m_Imb)
		{
			for (auto&& it : PV_PQmax)
			{
				CDynaNodeBase*& pNode = it->pNode;
				pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMAX;
				pNode->Qgr = pNode->LFQmax;
			}

			if (PV_PQmax.empty())
			{
				for (auto&& it : PV_PQmin)
				{
					CDynaNodeBase*& pNode = it->pNode;
					pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMIN;
					pNode->Qgr = pNode->LFQmin;
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
		UpdateVDelta();
	}

	// обновляем реактивную генерацию в суперузлах

	for (auto && supernode : m_SuperNodeParameters)
	{
		CDynaNodeBase *pNode = supernode.m_pNode;
		double Qrange = pNode->LFQmax - pNode->LFQmin;
		Qrange = (Qrange > 0.0) ? (pNode->Qgr - pNode->LFQmin) / Qrange : 0.0;
		CLinkPtrCount *pLink = pNode->GetSuperLink(0);
		CDevice **ppDevice(nullptr);
		while (pLink->In(ppDevice))
		{
			CDynaNodeBase *pSlaveNode = static_cast<CDynaNodeBase*>(*ppDevice);
			pSlaveNode->Qg = 0.0;
			if (pSlaveNode->IsStateOn())
				pSlaveNode->Qg = pSlaveNode->LFQmin + (pSlaveNode->LFQmax - pSlaveNode->LFQmin) * Qrange;
		}
		pNode->Qgr = supernode.LFQmin + (supernode.LFQmax - supernode.LFQmin) * Qrange;
		supernode.Restore();
	}

	for (auto&& it : pNodes->m_DevVec)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(it);
		if (!pNode->m_pSuperNodeParent)
		{
			pNode->Qg = pNode->IsStateOn() ? pNode->Qgr : 0.0;
			GetPnrQnr(pNode);
		}
	}
}


void CLoadFlow::UpdateVDelta(double dStep)
{
	_MatrixInfo *pMatrixInfo = m_pMatrixInfo.get();
	double *b = klu.B();
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
		m_Vbackup = make_unique<double[]>(klu.MatrixSize());
	if(!m_Dbackup)
		m_Dbackup = make_unique<double[]>(klu.MatrixSize());

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
				if (pNode->V > pNode->LFVref && pNode->Qgr >= pNode->LFQmax)
					m_pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(_T("Infeasible %s"), pNode->GetVerbalName()));
				if (pNode->V < pNode->LFVref && pNode->Qgr <= pNode->LFQmin)
					m_pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(_T("Infeasible %s"), pNode->GetVerbalName()));
				if (pNode->Qgr < pNode->LFQmin)
					m_pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(_T("Infeasible %s"), pNode->GetVerbalName()));
				if (pNode->Qgr > pNode->LFQmax)
					m_pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(_T("Infeasible %s"), pNode->GetVerbalName()));
			}
		}
	}
}
