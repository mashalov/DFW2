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
	bool bRes = false;
	CleanUp();
	pNodes = static_cast<CDynaNodeContainer*>(m_pDynaModel->GetDeviceContainer(DEVTYPE_NODE));
	if (!pNodes)
		return bRes;

	m_nMatrixSize = 0;		

	// создаем привязку узлов к информации по строкам матрицы
	m_pMatrixInfo = new _MatrixInfo[pNodes->Count()];
	_MatrixInfo *pMatrixInfo = m_pMatrixInfo;

	for (DEVICEVECTORITR it = pNodes->begin(); it != pNodes->end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		pNode->Init(m_pDynaModel);
		//pNode->V0 = pNode->Unom;


		if (NodeInMatrix(pNode))
		{
			// нумеруем включенные узлы строками матрицы
			pNode->SetMatrixRow(m_nMatrixSize);
			m_nMatrixSize += 2;

			pMatrixInfo->nRowCount += 2;	// считаем диагональный элемент
			CLinkPtrCount *pBranchLink = pNode->GetLink(0);
			pNode->ResetVisited();
			CDevice **ppDevice = nullptr;
			// обходим ветви и считаем количество внедиагональных элементов
			while (pBranchLink->In(ppDevice))
			{
				CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppDevice);
				if (pBranch->m_BranchState == CDynaBranch::BRANCH_ON)
				{
					CDynaNodeBase *pOppNode = pBranch->GetOppositeNode(pNode);
					if (NodeInMatrix(pOppNode))
					{
						if (pNode->CheckAddVisited(pOppNode) < 0)
							pMatrixInfo->nRowCount += 2;
					}
				}
			}
			m_nNonZeroCount += 2 * pMatrixInfo->nRowCount;	// количество ненулевых элементов увеличиваем на количество подсчитанных элементов в строке (4 double на элемент)
		}

		pMatrixInfo++;
	}

	Ax = new double[m_nNonZeroCount];
	b = new double[m_nMatrixSize];
	Ai = new ptrdiff_t[m_nMatrixSize + 1];
	Ap = new ptrdiff_t[m_nNonZeroCount];

	pMatrixInfo = m_pMatrixInfo;

	ptrdiff_t *pAi = Ai;
	ptrdiff_t *pAp = Ap;

	ptrdiff_t nRowPtr = 0;

	for (DEVICEVECTORITR it = pNodes->begin(); it != pNodes->end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		if (NodeInMatrix(pNode))
		{
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
			
			CLinkPtrCount *pBranchLink = pNode->GetLink(0);
			pNode->ResetVisited();
			CDevice **ppDevice = nullptr;
			while (pBranchLink->In(ppDevice))
			{
				CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppDevice);
				if (pBranch->m_BranchState == CDynaBranch::BRANCH_ON)
				{
					CDynaNodeBase *pOppNode = pBranch->GetOppositeNode(pNode);
					if (NodeInMatrix(pOppNode))
					{
						if (pNode->CheckAddVisited(pOppNode) < 0)
						{
							*pAp = pOppNode->A(0);
							*(pAp + pMatrixInfo->nRowCount) = pOppNode->A(0);
							pAp++;
							*pAp = pOppNode->A(1);
							*(pAp + pMatrixInfo->nRowCount) = pOppNode->A(1);
							pAp++;
						}
					}
				}
			}
			pAp += pMatrixInfo->nRowCount;
		}
		pMatrixInfo++;
	}
	*pAi = m_nNonZeroCount;

	Symbolic = KLU_analyze(m_nMatrixSize, Ai, Ap, &Common);

	return bRes = true;
}

bool CLoadFlow::BuildMatrix()
{
	bool bRes = true;
	double *pb = b;
	double *pAx = Ax;
	_MatrixInfo *pMatrixInfo = m_pMatrixInfo;

	for (DEVICEVECTORITR it = pNodes->begin(); it != pNodes->end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		if (NodeInMatrix(pNode))
		{
			pNode->GetPnrQnr();
			double Pe = pNode->GetSelfImbP(), Qe = pNode->GetSelfImbQ();
			double dPdDelta = 0.0, dQdDelta = 0.0;
			double dPdV = pNode->GetSelfdPdV(), dQdV = pNode->GetSelfdQdV();

			double *pAxSelf = pAx;
			pAx += 2;

			CLinkPtrCount *pBranchLink = pNode->GetLink(0);
			pNode->ResetVisited();
			CDevice **ppDevice = nullptr;

			bool bNodePQ = pNode->IsLFTypePQ();

			while (pBranchLink->In(ppDevice))
			{
				CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppDevice);
				if (pBranch->m_BranchState == CDynaBranch::BRANCH_ON)
				{
					// определяем узел на противоположном конце инцидентной ветви
					CDynaNodeBase *pOppNode = pBranch->GetOppositeNode(pNode);
					if (pOppNode->IsStateOn())
					{
						cplx mult = conj(pNode->VreVim);
						// определяем взаимную проводимость со смежным узлом
						cplx *pYkm = pBranch->m_pNodeIp == pNode ? &pBranch->Yip : &pBranch->Yiq;
						mult *= pOppNode->VreVim ** pYkm;
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

							ptrdiff_t DupIndex = pNode->CheckAddVisited(pOppNode);
							if (DupIndex < 0)
							{
								*pAx = dPdDeltaM;
								*(pAx + pMatrixInfo->nRowCount) = dQdDeltaM;
								pAx++;
								*pAx = dPdVM;
								*(pAx + pMatrixInfo->nRowCount) = dQdVM;
								pAx++;
							}
							else
							{
								double *pSumAx = pAxSelf + 2 + DupIndex * 2;
								//_ASSERTE(*pSumAx == dPdDeltaM);
								*pSumAx += dPdDeltaM;
								*(pSumAx + pMatrixInfo->nRowCount) += dQdDeltaM;
								pSumAx++;
								*pSumAx += dPdVM;
								*(pSumAx + pMatrixInfo->nRowCount) += dQdVM;
							}
						}
					}
				}

				/*

				{
					// dP/dDelta
					pDynaModel->SetElement(A(0), pOppNode->A(0), mult.imag(), bDup);
					// dP/dV
					pDynaModel->SetElement(A(0), pOppNode->A(1), -ZeroDivGuard(mult.real(), pOppNode->V), bDup);
					// dQ/dDelta
					pDynaModel->SetElement(A(1), pOppNode->A(0), mult.real(), bDup);
					// dQ/dV
					pDynaModel->SetElement(A(1), pOppNode->A(1), ZeroDivGuard(mult.imag(), pOppNode->V), bDup);
				}
				*/
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

			/*
			pDynaModel->SetElement(A(0), A(0), dPdDelta);
			pDynaModel->SetElement(A(1), A(0), dQdDelta);
			pDynaModel->SetElement(A(0), A(1), dPdV);
			pDynaModel->SetElement(A(1), A(1), dQdV);
			*/

			*pb = Pe; pb++;
			*pb = Qe; pb++;
		}

		pMatrixInfo++;
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

	bool bRes = Estimate();
	if (!bRes)
		return bRes;

	CDynaNodeBase *pNo = static_cast<CDynaNodeBase*>(*(pNodes->begin()));
	pNo->Pn += 5;


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
		for (DEVICEVECTORITR it = pNodes->begin(); it != pNodes->end(); it++)
		{
			CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
			if (NodeInMatrix(pNode))
			{
				pNode->Delta -= b[pNode->A(0)];
				pNode->V -= b[pNode->A(0) + 1];
				double dV = b[pNode->A(0) + 1];
				pNode->UpdateVreVim();
			}
		}


		for (DEVICEVECTORITR it = pNodes->begin(); it != pNodes->end(); it++)
		{
			CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
			if (pNode->IsStateOn() && (!pNode->IsLFTypePQ() || pNode->m_eLFNodeType == CDynaNodeBase::eLFNodeType::LFNT_BASE))
			{
				pNode->GetPnrQnr();
				double Pe = pNode->GetSelfImbP() + pNode->Pg, Qe = pNode->GetSelfImbQ() + pNode->Qg;

				CLinkPtrCount *pBranchLink = pNode->GetLink(0);
				pNode->ResetVisited();
				CDevice **ppDevice = nullptr;
				while (pBranchLink->In(ppDevice))
				{
					CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppDevice);
					if (pBranch->m_BranchState == CDynaBranch::BRANCH_ON)
					{
						CDynaNodeBase *pOppNode = pBranch->GetOppositeNode(pNode);
						if (pOppNode->IsStateOn())
						{
							cplx mult = conj(pNode->VreVim);
							cplx *pYkm = pBranch->m_pNodeIp == pNode ? &pBranch->Yip : &pBranch->Yiq;
							mult *= pOppNode->VreVim ** pYkm;
							Pe -= mult.real();
							Qe += mult.imag();
						}
					}
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