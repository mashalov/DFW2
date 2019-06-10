#include "stdafx.h"
#include "LoadFlow.h"
#include "DynaModel.h"

using namespace DFW2;

CLoadFlow::CLoadFlow(CDynaModel *pDynaModel) :  m_pDynaModel(pDynaModel),
												Ax(nullptr),
												b(nullptr),
												Ai(nullptr),
												Ap(nullptr),
												m_pMatrixInfo(nullptr),
												pNodes(nullptr),
												Symbolic(nullptr)
{
	KLU_defaults(&Common);
}

CLoadFlow::~CLoadFlow()
{
	CleanUp();
}

void CLoadFlow::CleanUp()
{
	if (Ax)
		delete Ax;
	if (b)
		delete b;
	if (Ai)
		delete Ai;
	if (Ap)
		delete Ap;
	if (m_pMatrixInfo)
		delete m_pMatrixInfo;
	if (m_pVirtualBranches)
		delete m_pVirtualBranches;
	if (Symbolic)
		KLU_free_symbolic(&Symbolic, &Common);
}

bool CLoadFlow::NodeInMatrix(CDynaNodeBase *pNode)
{
	return pNode->m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_BASE && pNode->IsStateOn();
}

bool CDynaModel::LoadFlow()
{
	CLoadFlow LoadFlow(this);
	return LoadFlow.Run();
}

bool CLoadFlow::Estimate()
{
	bool bRes = true;

	m_nMatrixSize = 0;		
	// создаем привязку узлов к информации по строкам матрицы
	// размер берем по количеству узлов. Реальный размер матрицы будет меньше на
	// количество отключенных узлов и БУ
	m_pMatrixInfo = new _MatrixInfo[pNodes->Count()];
	_MatrixInfo *pMatrixInfo = m_pMatrixInfo;

	list<CDynaNodeBase*> SlackBuses;
	for (DEVICEVECTORITR it = pNodes->begin(); it != pNodes->end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		// обрабатываем только включенные узлы
		if (!pNode->IsStateOn())
			continue;
		// обновляем VreVim узла
		pNode->Init(m_pDynaModel);
		// добавляем БУ в список БУ для дальнейшей обработки
		if (pNode->m_eLFNodeType == CDynaNodeBase::eLFNodeType::LFNT_BASE && pNode->IsStateOn())
			SlackBuses.push_back(pNode);

		// обходим все узлы, включая БУ
		CLinkPtrCount *pBranchLink = pNode->GetLink(0);
		pNode->ResetVisited();
		CDevice **ppDevice = nullptr;
		while (pBranchLink->In(ppDevice))
		{
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppDevice);
			// если ветвь включена, узел на противоположном конце также должен быть включен
			if (pBranch->m_BranchState == CDynaBranch::BRANCH_ON)
			{
				CDynaNodeBase *pOppNode = pBranch->GetOppositeNode(pNode);
				if (pNode->CheckAddVisited(pOppNode) < 0)
				{
					if (NodeInMatrix(pNode))
					{
						// для узлов которые в матрице считаем количество ветвей
						// и ненулевых элементов
						pMatrixInfo->nBranchCount++;		// количество ветвей, включая ветви на БУ
						if (NodeInMatrix(pOppNode))
							pMatrixInfo->nRowCount += 2;	// количество ненулевых элементов = ветвей - ветвей на БУ
					}
					else
						m_nBranchesCount++; // для БУ считаем общее количество ветвей
				}
			}
		}

		if (NodeInMatrix(pNode))
		{
			// для узлов, которые попадают в матрицу нумеруем включенные узлы строками матрицы
			pNode->SetMatrixRow(m_nMatrixSize);
			pMatrixInfo->pNode = pNode;
			m_nMatrixSize += 2;				// на узел 2 уравнения в матрице
			pMatrixInfo->nRowCount += 2;	// считаем диагональный элемент
			m_nNonZeroCount += 2 * pMatrixInfo->nRowCount;	// количество ненулевых элементов увеличиваем на количество подсчитанных элементов в строке (4 double на элемент)
			m_nBranchesCount += pMatrixInfo->nBranchCount;	// общее количество ветвей для ведения списков ветвей от узлов
			pMatrixInfo++;
		}
	}

	m_pMatrixInfoEnd = pMatrixInfo;								// конец инфо по матрице для узлов, которые в матрице

	Ax = new double[m_nNonZeroCount];							// числа матрицы
	b = new double[m_nMatrixSize];								// вектор правой части
	Ai = new ptrdiff_t[m_nMatrixSize + 1];						// строки матрицы
	Ap = new ptrdiff_t[m_nNonZeroCount];						// столбцы матрицы
	m_pVirtualBranches = new _VirtualBranch[m_nBranchesCount];	// общий список ветвей от узлов, разделяемый указателями внутри _MatrixInfo
	_VirtualBranch *pBranches = m_pVirtualBranches;

	ptrdiff_t *pAi = Ai;
	ptrdiff_t *pAp = Ap;
	ptrdiff_t nRowPtr = 0;

	for (_MatrixInfo *pMatrixInfo = m_pMatrixInfo ; pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
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

		// привязываем список ветвей к инфо узла
		pMatrixInfo->pBranches = pBranches;
		CLinkPtrCount *pBranchLink = pNode->GetLink(0);
		pNode->ResetVisited();
		CDevice **ppDevice = nullptr;
		while (pBranchLink->In(ppDevice))
		{
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppDevice);
			if (pBranch->m_BranchState == CDynaBranch::BRANCH_ON)
			{
				// обходим включенные ветви также как и для подсчета размерностей выше
				CDynaNodeBase *pOppNode = pBranch->GetOppositeNode(pNode);
				// получаем проводимость к оппозитному узлу
				cplx *pYkm = pBranch->m_pNodeIp == pNode ? &pBranch->Yip : &pBranch->Yiq;
				// проверяем, уже прошли данный оппозитный узел для просматриваемого узла или нет
				ptrdiff_t DupIndex = pNode->CheckAddVisited(pOppNode);
				if (DupIndex < 0)
				{
					// если нет - добавляем ветвь в список данного узла (включая ветви на БУ)
					pBranches->Y = *pYkm;
					pBranches->pNode = pOppNode;
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
					pBranches++;
				}
				else
					(pMatrixInfo->pBranches + DupIndex)->Y += *pYkm; // если оппозитный узел уже прошли, ветвь не добавляем, а суммируем ее проводимость параллельно с уже пройденной ветвью
			}
		}
		pAp += pMatrixInfo->nRowCount;
	}

	// отдельно обрабатываем БУ
	// добавляем их "под матрицу"

	for (auto& sit : SlackBuses)
	{
		CDynaNodeBase *pNode = sit;
		pMatrixInfo->pNode = pNode;
		pMatrixInfo->pBranches = pBranches;

		CLinkPtrCount *pBranchLink = pNode->GetLink(0);
		pNode->ResetVisited();
		CDevice **ppDevice = nullptr;
		while (pBranchLink->In(ppDevice))
		{
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppDevice);
			if (pBranch->m_BranchState == CDynaBranch::BRANCH_ON)
			{
				// делаем все то же, что для нормальных узлов, исключая все связанное с матрицей
				// но строим список ветвей от БУ
				CDynaNodeBase *pOppNode = pBranch->GetOppositeNode(pNode);
				cplx *pYkm = pBranch->m_pNodeIp == pNode ? &pBranch->Yip : &pBranch->Yiq;
				ptrdiff_t DupIndex = pNode->CheckAddVisited(pOppNode);
				if (DupIndex < 0)
				{
					pBranches->Y = *pYkm;
					pBranches->pNode = pOppNode;
					pBranches++;
				}
				else
					(pMatrixInfo->pBranches + DupIndex)->Y += *pYkm;
			}
		}
		pMatrixInfo->nBranchCount = pBranches - pMatrixInfo->pBranches;
		pMatrixInfo++;
	}

	m_pMatrixInfoSlackEnd = pMatrixInfo;
	*pAi = m_nNonZeroCount;

	Symbolic = KLU_analyze(m_nMatrixSize, Ai, Ap, &Common);
	return bRes;
}

bool CLoadFlow::Start()
{
	bool bRes = true;
	CleanUp();
	pNodes = static_cast<CDynaNodeContainer*>(m_pDynaModel->GetDeviceContainer(DEVTYPE_NODE));
	if (!pNodes)
		return bRes;

	for (DEVICEVECTORITR it = pNodes->begin(); it != pNodes->end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		if (pNode->IsStateOn())
		{
			if (pNode->m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_BASE)
			{
				if (pNode->LFQmax - pNode->LFQmin > m_Parameters.m_Imb && pNode->LFVref > 0.0)
				{
					pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PV;
					if (m_Parameters.m_bFlat)
					{
						pNode->V = pNode->LFVref;
						pNode->Qg = pNode->LFQmin + (pNode->LFQmax - pNode->LFQmin) / 2.0;
						pNode->Delta = 0.0;
					}
					else if (pNode->Qg > pNode->LFQmax)
					{
						pNode->Qg = pNode->LFQmax;
						pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMAX;
					}
					else if (pNode->LFQmin > pNode->Qg)
					{
						pNode->Qg = pNode->LFQmin;
						pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMIN;
					}
				}
				else 
				{
					pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PQ;
					if (m_Parameters.m_bFlat)
					{
						pNode->V = pNode->Unom;
						pNode->Delta = 0.0;
					}
				}
			}
			else
			{
				pNode->Pg = pNode->Qg = 0.0;
			}
		}
		else
		{
			pNode->V = pNode->Delta = 0.0;
		}
		pNode->Init(m_pDynaModel);
	}

	return bRes;
}

double ImbNorm(double x, double y)
{
	return x * x + y * y;
}

bool CLoadFlow::SortPV(const _MatrixInfo* lhs, const _MatrixInfo* rhs)
{
	_ASSERTE(!lhs->pNode->IsLFTypePQ());
	_ASSERTE(!lhs->pNode->IsLFTypePQ());
	return (lhs->pNode->LFQmax - lhs->pNode->LFQmin) > (rhs->pNode->LFQmax - rhs->pNode->LFQmin);
}

void CLoadFlow::AddToQueue(_MatrixInfo *pMatrixInfo, QUEUE& queue)
{
	for (_VirtualBranch *pBranch = pMatrixInfo->pBranches; pBranch < pMatrixInfo->pBranches + pMatrixInfo->nBranchCount; pBranch++)
	{
		CDynaNodeBase *pOppNode = pBranch->pNode;
		if (pOppNode->IsLFTypePQ() && pOppNode->m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_BASE)
		{
			_MatrixInfo *pOppMatrixInfo = m_pMatrixInfo + pOppNode->A(0) / 2;
			_ASSERTE(pOppMatrixInfo->pNode == pOppNode);
			if (!pOppMatrixInfo->bVisited)
			{
				queue.push_back(pOppMatrixInfo);
				pOppMatrixInfo->bVisited = true;
			}
		}
	}
}

bool CLoadFlow::Seidell()
{
	bool bRes = true;
	
	MATRIXINFO SeidellOrder;
	SeidellOrder.reserve(m_pMatrixInfoSlackEnd - m_pMatrixInfo);

	_MatrixInfo* pMatrixInfo = m_pMatrixInfoSlackEnd - 1;

	// в начало добавляем БУ
	for (; pMatrixInfo >= m_pMatrixInfoEnd; pMatrixInfo--)
	{
		SeidellOrder.push_back(pMatrixInfo);
		pMatrixInfo->bVisited = true;
	}

	// затем PV узлы
	for (; pMatrixInfo >= m_pMatrixInfo; pMatrixInfo--)
	{
		CDynaNodeBase *pNode = pMatrixInfo->pNode;
		if (!pNode->IsLFTypePQ())
		{
			SeidellOrder.push_back(pMatrixInfo);
			pMatrixInfo->bVisited = true;
		}
	}

	// сортируем PV узлы по убыванию диапазона реактивной мощности
	sort(SeidellOrder.begin() + (m_pMatrixInfoSlackEnd - m_pMatrixInfoEnd) , SeidellOrder.end(), SortPV);
	QUEUE queue;

	for (MATRIXINFOITR it = SeidellOrder.begin(); it != SeidellOrder.end(); it++)
		AddToQueue(*it, queue);

	while (!queue.empty())
	{
		pMatrixInfo = queue.front();
		queue.pop_front();
		SeidellOrder.push_back(pMatrixInfo);
		AddToQueue(pMatrixInfo, queue);
	}

	_ASSERTE(SeidellOrder.size() == m_pMatrixInfoSlackEnd - m_pMatrixInfo);

	pNodes->CalcAdmittances(true);
	double dPreviousImb = -1.0;
	for (int nSeidellIterations = 0; nSeidellIterations < m_Parameters.m_nSeidellIterations; nSeidellIterations++)
	{
		double dStep = m_Parameters.m_dSeidellStep;

		if (nSeidellIterations > 2)
		{
			if (dPreviousImb < 0.0)
			{
				dPreviousImb = ImbNorm(m_IterationControl.m_MaxImbP.GetDiff(), m_IterationControl.m_MaxImbQ.GetDiff());
			}
			else
			{
				double dCurrentImb = ImbNorm(m_IterationControl.m_MaxImbP.GetDiff(), m_IterationControl.m_MaxImbQ.GetDiff());
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

		ResetIterationControl();

		bool bPVPQSwitchEnabled = nSeidellIterations >= m_Parameters.m_nEnableSwitchIteration ;

		for (MATRIXINFOITR it = SeidellOrder.begin() ; it != SeidellOrder.end() ; it++)
		{
			pMatrixInfo = *it;
			CDynaNodeBase *pNode = pMatrixInfo->pNode;
			pNode->GetPnrQnr();
			double& Pe = pMatrixInfo->m_dImbP;
			double& Qe = pMatrixInfo->m_dImbQ;

			Pe = pNode->GetSelfImbP();
			Qe = pNode->GetSelfImbQ();

			for (_VirtualBranch *pBranch = pMatrixInfo->pBranches; pBranch < pMatrixInfo->pBranches + pMatrixInfo->nBranchCount; pBranch++)
			{
				CDynaNodeBase *pOppNode = pBranch->pNode;
				cplx mult = conj(pNode->VreVim) * pOppNode->VreVim * pBranch->Y;
				Pe -= mult.real();
				Qe += mult.imag();
			}

			double Q = Qe + pNode->Qg;	// расчетная генерация в узле

			cplx I1 = dStep / conj(pNode->VreVim) / pNode->Yii;

			switch (pNode->m_eLFNodeType)
			{
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMAX:
				if (pNode->V > pNode->LFVref && Q < pNode->LFQmax)
				{
					pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PV;
					pMatrixInfo->m_nPVSwitchCount++;
					pNode->Qg = Q;
					cplx dU = I1 * cplx(Pe, 0);
					dU += pNode->VreVim;
					pNode->VreVim = pNode->LFVref * dU / abs(dU);
				}
				else
				{
					pNode->Qg = pNode->LFQmax;
					cplx dU = I1  * cplx(Pe, -Qe);
					pNode->VreVim += dU;
				}
				break;
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMIN:
				if (pNode->V < pNode->LFVref && Q > pNode->LFQmin)
				{
					pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PV;
					pMatrixInfo->m_nPVSwitchCount++;
					pNode->Qg = Q;
					cplx dU = I1 * cplx(Pe, 0);
					dU += pNode->VreVim;
					pNode->VreVim = pNode->LFVref * dU / abs(dU);
				}
				else
				{
					pNode->Qg = pNode->LFQmin;
					cplx dU = I1  * cplx(Pe, -Qe);
					pNode->VreVim += dU;
				}
				break;
			case CDynaNodeBase::eLFNodeType::LFNT_PV:
				{
					if (bPVPQSwitchEnabled)
					{
						if (Q > pNode->LFQmax)
						{
							pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMAX;
							pNode->Qg = pNode->LFQmax;
							Qe = Q - pNode->Qg;
						}
						else if (Q < pNode->LFQmin)
						{
							pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMIN;
							pNode->Qg = pNode->LFQmin;
							Qe = Q - pNode->Qg;
						}
						else
						{
							pNode->Qg = Q;
							Qe = 0.0;
						}
						cplx dU = I1 * cplx(Pe, -Qe);
						dU += pNode->VreVim;
						pNode->VreVim = pNode->LFVref * dU / abs(dU);
					}
					else
					{
						cplx dU = I1 * cplx(Pe, -Qe);
						pNode->VreVim += dU;
					}
				}
				break;
			case CDynaNodeBase::eLFNodeType::LFNT_PQ:
				{
					cplx dU = I1 * cplx(Pe, -Qe);
					pNode->VreVim += dU;
				}
			break;
			case CDynaNodeBase::eLFNodeType::LFNT_BASE:
				pNode->Pg += Pe;
				pNode->Qg += Qe;
				break;
			}

			pNode->V = abs(pNode->VreVim);
			pNode->Delta = arg(pNode->VreVim);

			if(pNode->m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_BASE)
				UpdateIterationControl(pMatrixInfo);
		}
		DumpIterationControl();

		if (m_IterationControl.Converged(m_Parameters.m_Imb))
			break;
	}
	pNodes->CalcAdmittances(false);
	return bRes;
}

bool CLoadFlow::BuildMatrix()
{
	bool bRes = true;
	double *pb = b;
	double *pAx = Ax;
	_MatrixInfo *pMatrixInfo = m_pMatrixInfo;

	// обходим только узлы в матрице
	for (_MatrixInfo *pMatrixInfo = m_pMatrixInfo; pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
	{
		CDynaNodeBase *pNode = pMatrixInfo->pNode;
		bool bNodePQ = pNode->IsLFTypePQ();
		pNode->GetPnrQnr();
		double Pe = pNode->GetSelfImbP(), Qe = pNode->GetSelfImbQ();
		double dPdDelta = 0.0, dQdDelta = 0.0;
		double dPdV = pNode->GetSelfdPdV(), dQdV = pNode->GetSelfdQdV();

		double *pAxSelf = pAx;
		pAx += 2;

		for (_VirtualBranch *pBranch = pMatrixInfo->pBranches; pBranch < pMatrixInfo->pBranches + pMatrixInfo->nBranchCount; pBranch++)
		{
			CDynaNodeBase *pOppNode = pBranch->pNode;
			cplx mult = conj(pNode->VreVim);
			mult *= pOppNode->VreVim * pBranch->Y;
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

				if (!bNodePQ)
				{
					dQdDeltaM = dQdVM = 0.0;
				}

				*pAx = dPdDeltaM;
				*(pAx + pMatrixInfo->nRowCount) = dQdDeltaM;
				pAx++;
				*pAx = dPdVM;
				*(pAx + pMatrixInfo->nRowCount) = dQdVM;
				pAx++;
			}
		}

		if (!bNodePQ)
		{
			dQdDelta = 0.0;
			dQdV = 1.0;
			Qe = 0.0;
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
	return bRes;
}

// расчет небаланса в узле
void CLoadFlow::GetNodeImb(_MatrixInfo *pMatrixInfo)
{
	CDynaNodeBase *pNode = pMatrixInfo->pNode;
	// нагрузка по СХН
	pNode->GetPnrQnr();
	pMatrixInfo->m_dImbP = pNode->GetSelfImbP();
	pMatrixInfo->m_dImbQ = pNode->GetSelfImbQ();
	for (_VirtualBranch *pBranch = pMatrixInfo->pBranches; pBranch < pMatrixInfo->pBranches + pMatrixInfo->nBranchCount; pBranch++)
	{
		CDynaNodeBase *pOppNode = pBranch->pNode;
		cplx mult = conj(pNode->VreVim) * pOppNode->VreVim * pBranch->Y;
		pMatrixInfo->m_dImbP -= mult.real();
		pMatrixInfo->m_dImbQ += mult.imag();
	}
}

#include "atlbase.h"
bool CLoadFlow::Run()
{
	m_Parameters.m_bFlat = true;

	bool bRes = Start();
	bRes = bRes && Estimate();
	if (!bRes)
		return bRes;
		
	if (m_Parameters.m_bStartup)
	{
		m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, CDFW2Messages::m_cszLFRunningSeidell);
		Seidell();
	}

	m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, CDFW2Messages::m_cszLFRunningNewton);
	int it(0);

	_MatrixInfo **ppSwitch = new _MatrixInfo *[m_nMatrixSize];

	while(1)
	{
		++it;
		_MatrixInfo **ppSwitchEnd = ppSwitch;
		ResetIterationControl();
		// считаем небаланс по всем узлам кроме БУ
		_MatrixInfo *pMatrixInfo = m_pMatrixInfo;
		for (; pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
		{
			CDynaNodeBase *pNode = pMatrixInfo->pNode;
			if (it > 10 && pNode->GetId() == 4349)
				it = it;
			GetNodeImb(pMatrixInfo);
			double Qg = pNode->Qg + pMatrixInfo->m_dImbQ;
			switch (pNode->m_eLFNodeType)
			{
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMIN:
				if (pNode->V < pNode->LFVref && Qg > pNode->LFQmin)
				{
					*ppSwitchEnd = pMatrixInfo;
					pMatrixInfo->m_nPVSwitchCount++;
					ppSwitchEnd++;
				}
				break;
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMAX:
				if (pNode->V > pNode->LFVref && Qg < pNode->LFQmax)
				{
					*ppSwitchEnd = pMatrixInfo;
					ppSwitchEnd++;
					pMatrixInfo->m_nPVSwitchCount++;
				}
				break;
			case CDynaNodeBase::eLFNodeType::LFNT_PV:
				if (Qg > pNode->LFQmax)
				{
					m_IterationControl.m_nQviolated++;
					*ppSwitchEnd = pMatrixInfo;
					ppSwitchEnd++;
				}
				else if (Qg < pNode->LFQmin)
				{
					m_IterationControl.m_nQviolated++;
					*ppSwitchEnd = pMatrixInfo;
					ppSwitchEnd++;
				}
				else
					pNode->Qg = Qg;
				break;
			}
			UpdateIterationControl(pMatrixInfo);
		}

		// досчитываем небалансы в БУ
		for (pMatrixInfo = m_pMatrixInfoEnd; pMatrixInfo < m_pMatrixInfoSlackEnd ; pMatrixInfo++)
		{
			CDynaNodeBase *pNode = pMatrixInfo->pNode;
			GetNodeImb(pMatrixInfo);
			pNode->Pg += pMatrixInfo->m_dImbP;
			pNode->Qg += pMatrixInfo->m_dImbQ;
		}

		DumpIterationControl();

		if (m_IterationControl.Converged(m_Parameters.m_Imb))
			break;
		if (it > m_Parameters.m_nMaxIterations)
		{
			m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, CDFW2Messages::m_cszNoLFConvergence);
			bRes = false;
			break;
		}
		// переключаем типы узлов
		if (m_IterationControl.m_MaxImbP.GetDiff() < m_Parameters.m_Imb)
		{
			for (_MatrixInfo **ppSwitchNow = ppSwitch; ppSwitchNow < ppSwitchEnd; ppSwitchNow++)
			{
				pMatrixInfo = *ppSwitchNow;
				CDynaNodeBase *pNode = pMatrixInfo->pNode;
				double Qg = pNode->Qg + pMatrixInfo->m_dImbQ;
				switch (pNode->m_eLFNodeType)
				{
				case CDynaNodeBase::eLFNodeType::LFNT_PVQMIN:
					pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PV;
					pMatrixInfo->m_nPVSwitchCount++;
					pNode->Qg = Qg;
					pNode->V = pNode->LFVref;
					break;
				case CDynaNodeBase::eLFNodeType::LFNT_PVQMAX:
					pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PV;
					pMatrixInfo->m_nPVSwitchCount++;
					pNode->Qg = Qg;
					pNode->V = pNode->LFVref;
					break;
				case CDynaNodeBase::eLFNodeType::LFNT_PV:
					pNode->Qg = Qg;
					if (pNode->Qg > pNode->LFQmax)
					{
						m_IterationControl.m_nQviolated++;
						//m_pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, _T("Qmax < Q %g < %g %d"), pNode->LFQmax, pNode->Qg, pNode->GetId());
						pNode->Qg = pNode->LFQmax;
						pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMAX;
					}
					else if (pNode->Qg < pNode->LFQmin)
					{
						m_IterationControl.m_nQviolated++;
						//m_pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, _T("Qmin > Q %g > %g %d"), pNode->LFQmin, pNode->Qg, pNode->GetId());
						pNode->Qg = pNode->LFQmin;
						pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMIN;
					}
					break;
				}
				pNode->UpdateVreVim();
			}
		}

		BuildMatrix();
		KLU_numeric *Numeric = KLU_factor(Ai, Ap, Ax, Symbolic, &Common);

		int nmx = 0;
		for (int i = 1; i < m_nMatrixSize; i++)
		{
			if (fabs(b[nmx]) < fabs(b[i]))
				nmx = i;
		}
		ATLTRACE("\n%g", b[nmx]);


		int sq = KLU_tsolve(Symbolic, Numeric, m_nMatrixSize, 1, b, &Common);
		
		/*nmx = 0;
		for (int i = 1; i < m_nMatrixSize; i++)
		{
			if (fabs(b[nmx]) < fabs(b[i]))
				nmx = i;
		}
		ATLTRACE("\n%g", b[nmx]);*/
		
	
		KLU_free_numeric(&Numeric, &Common);

		for (pMatrixInfo = m_pMatrixInfo; pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
		{
			CDynaNodeBase *pNode = pMatrixInfo->pNode;
			pNode->Delta -= b[pNode->A(0)];
			pNode->V -= b[pNode->A(0) + 1];
			pNode->UpdateVreVim();
		}
	}

	delete ppSwitch;
	
	for (DEVICEVECTORITR it = pNodes->begin(); it != pNodes->end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		ATLTRACE("\n %20f %20f %20f %20f %20f %20f", pNode->V, pNode->Delta * 180 / M_PI, pNode->Pg, pNode->Qg, pNode->Pnr, pNode->Qnr);
	}
	
	return bRes;
}


void CLoadFlow::ResetIterationControl()
{
	m_IterationControl = _IterationControl();
}

void CLoadFlow::UpdateIterationControl(_MatrixInfo *pMatrixInfo)
{
	if (pMatrixInfo && pMatrixInfo->pNode)
	{
		m_IterationControl.m_MaxImbP.UpdateMaxAbs(pMatrixInfo, pMatrixInfo->m_dImbP);
		m_IterationControl.m_MaxImbQ.UpdateMaxAbs(pMatrixInfo, pMatrixInfo->m_dImbQ);
		double VdVnom = pMatrixInfo->pNode->V / pMatrixInfo->pNode->Unom;
		m_IterationControl.m_MaxV.UpdateMax(pMatrixInfo, VdVnom);
		m_IterationControl.m_MinV.UpdateMin(pMatrixInfo, VdVnom);
	}
	else
		_ASSERTE(pMatrixInfo && pMatrixInfo->pNode);
}

void CLoadFlow::DumpIterationControl()
{
	// p q minv maxv
	m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, _T("%20g %6d %20g %6d %10g %6d %10g %6d %4d"), 
																			m_IterationControl.m_MaxImbP.GetDiff(), m_IterationControl.m_MaxImbP.GetId(),
																			m_IterationControl.m_MaxImbQ.GetDiff(), m_IterationControl.m_MaxImbQ.GetId(),
																			m_IterationControl.m_MaxV.GetDiff(), m_IterationControl.m_MaxV.GetId(),
																			m_IterationControl.m_MinV.GetDiff(), m_IterationControl.m_MinV.GetId(),
																			m_IterationControl.m_nQviolated);
}