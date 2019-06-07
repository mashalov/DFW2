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

	bool bFlat = true;

	for (DEVICEVECTORITR it = pNodes->begin(); it != pNodes->end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		if (pNode->IsStateOn())
		{
			if (pNode->LFQmax - pNode->LFQmin > m_Parameters.m_Imb && pNode->LFVref > 0.0)
			{
				pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PV;
				if (pNode->Qg - pNode->LFQmax > m_Parameters.m_Imb)
				{
					pNode->Qg = pNode->LFQmax;
					pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMAX;
				}
				else if (pNode->LFQmin - pNode->Qg > m_Parameters.m_Imb)
				{
					pNode->Qg = pNode->LFQmin;
					pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMIN;
				}

				if (bFlat)
				{
					pNode->V = pNode->LFVref;
					pNode->Qg = pNode->LFQmin + (pNode->LFQmax - pNode->LFQmin) / 2.0;
					pNode->Delta = 0.0;
				}
			}
			else if (pNode->m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_BASE)
			{
				pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PQ;
				if (bFlat)
				{
					pNode->V = pNode->Unom;
					pNode->Delta = 0.0;
				}
			}
		}
		else
		{
			pNode->V = pNode->Delta = 0.0;
		}
		/*
		pNode->V = pNode->Unom;
		pNode->Delta = 0.0;
		pNode->Init(m_pDynaModel);
		*/

	}

	return bRes;
}

bool CLoadFlow::Seidell()
{
	bool bRes = true;
	for (_MatrixInfo *pMatrixInfo = m_pMatrixInfo; pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
	{
		CDynaNodeBase *pNode = pMatrixInfo->pNode;
		pNode->GetPnrQnr();
		double Pe = pNode->GetSelfImbP(), Qe = pNode->GetSelfImbQ() + pNode->Qg;
		for (_VirtualBranch *pBranch = pMatrixInfo->pBranches; pBranch < pMatrixInfo->pBranches + pMatrixInfo->nBranchCount; pBranch++)
		{
			CDynaNodeBase *pOppNode = pBranch->pNode;
			cplx mult = conj(pNode->VreVim) * pOppNode->VreVim * pBranch->Y;
			Pe -= mult.real();
			Qe += mult.imag();
		}
	}
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

#include "atlbase.h"
bool CLoadFlow::Run()
{
	/*
	CDeviceContainer *pBranchCon = m_pDynaModel->GetDeviceContainer(DEVTYPE_BRANCH);
	CDynaBranch *pBr = static_cast<CDynaBranch*>(*(pBranchCon->begin() + 4));
	pBr->SetBranchState(CDynaBranch::BRANCH_OFF, eDEVICESTATECAUSE::DSC_INTERNAL);
	*/

	bool bRes = Start();
	bRes = bRes && Estimate();
	if (!bRes)
		return bRes;
		
	Seidell();

	
	/*
	CDynaNodeBase *pNo = static_cast<CDynaNodeBase*>(*(pNodes->begin()));
	pNo->Pn += -5;
	*/


	for (int it = 0; it < 10; it++)
	{
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

		

		_MatrixInfo *pMatrixInfo = m_pMatrixInfo;
		for (_MatrixInfo *pMatrixInfo = m_pMatrixInfo; pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
		{
			CDynaNodeBase *pNode = pMatrixInfo->pNode;
			pNode->Delta -= b[pNode->A(0)];
			pNode->V -= b[pNode->A(0) + 1];
			pNode->UpdateVreVim();
		}


		// расчет небаланса по Q для PV узлов и PQ для БУ
		for (_MatrixInfo *pMatrixInfo = m_pMatrixInfo; pMatrixInfo < m_pMatrixInfoSlackEnd; pMatrixInfo++)
		{
			CDynaNodeBase *pNode = pMatrixInfo->pNode;
			if (!pNode->IsLFTypePQ() || CDynaNodeBase::eLFNodeType::LFNT_BASE == pNode->m_eLFNodeType)
			{
				pNode->GetPnrQnr();
				double Pe = pNode->GetSelfImbP() + pNode->Pg, Qe = pNode->GetSelfImbQ() + pNode->Qg;

				for (_VirtualBranch *pBranch = pMatrixInfo->pBranches; pBranch < pMatrixInfo->pBranches + pMatrixInfo->nBranchCount; pBranch++)
				{
					CDynaNodeBase *pOppNode = pBranch->pNode;
					cplx mult = conj(pNode->VreVim) * pOppNode->VreVim * pBranch->Y;
					Pe -= mult.real();
					Qe += mult.imag();
				}
				pNode->Qg = Qe;
				if (CDynaNodeBase::eLFNodeType::LFNT_BASE == pNode->m_eLFNodeType)
					pNode->Pg = Pe;
			}
		}
	}

	for (DEVICEVECTORITR it = pNodes->begin(); it != pNodes->end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		ATLTRACE("\n %20f %20f %20f %20f %20f %20f", pNode->V, pNode->Delta * 180 / M_PI, pNode->Pg, pNode->Qg, pNode->Pnr, pNode->Qnr);
	}
	return bRes;
}