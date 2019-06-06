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

bool CLoadFlow::SetElement(ptrdiff_t nRow, ptrdiff_t nCol, double& dValue, bool bAdd)
{
	return true;
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

bool CLoadFlow::Run()
{
	bool bRes = Estimate();
	if (!bRes)
		return bRes;

	double *pb = b;
	double *pAx = Ax;
	_MatrixInfo *pMatrixInfo = m_pMatrixInfo;

	for (DEVICEVECTORITR it = pNodes->begin(); it != pNodes->end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		if (NodeInMatrix(pNode))
		{
			pNode->GetPnrQnr();
			double Pe = pNode->GetSelfImbP();
			double Qe = pNode->GetSelfImbQ();
			double dPdDelta = 0.0;
			double dPdV = pNode->GetSelfdPdV();
			double dQdDelta = 0.0;
			double dQdV = pNode->GetSelfdQdV();

			double *pAxSelf = pAx;
			pAx += 2;

			CLinkPtrCount *pBranchLink = pNode->GetLink(0);
			pNode->ResetVisited();
			CDevice **ppDevice = nullptr;
			while (pBranchLink->In(ppDevice))
			{
				CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppDevice);
				if (pBranch->m_BranchState == CDynaBranch::BRANCH_ON)
				{
					// определяем узел на противоположном конце инцидентной ветви
					CDynaNodeBase *pOppNode = pBranch->GetOppositeNode(pNode);
					cplx mult = conj(pNode->VreVim);
					// определяем взаимную проводимость со смежным узлом
					cplx *pYkm = pBranch->m_pNodeIp == pNode ? &pBranch->Yip : &pBranch->Yiq;
					mult *= pOppNode->VreVim ** pYkm;
					Pe -= mult.real();
					Qe += mult.imag();

					// diagonals 2
					dPdDelta -= mult.imag();
					dPdV += -CDevice::ZeroDivGuard(mult.real(), pNode->V);
					dQdDelta += -mult.real();
					dQdV += CDevice::ZeroDivGuard(mult.imag(), pNode->V);

					if (NodeInMatrix(pOppNode))
					{
						ptrdiff_t DupIndex = pNode->CheckAddVisited(pOppNode);
						if (DupIndex < 0)
						{
							//_ASSERTE(pAx - pAxSelf < pMatrixInfo->nRowCount);
							*pAx = mult.imag();
							*(pAx + pMatrixInfo->nRowCount) = mult.real();
							pAx++;
							*pAx = -CDevice::ZeroDivGuard(mult.real(), pOppNode->V);
							*(pAx + pMatrixInfo->nRowCount) = CDevice::ZeroDivGuard(mult.imag(), pOppNode->V);
							pAx++;
						}
						else
						{
							double *pSumAx = pAxSelf + 2 + DupIndex;
							*pSumAx += mult.imag();
							*(pSumAx + pMatrixInfo->nRowCount) += mult.real();
							pSumAx++;
							*pSumAx += -CDevice::ZeroDivGuard(mult.real(), pOppNode->V);
							*(pSumAx + pMatrixInfo->nRowCount) += CDevice::ZeroDivGuard(mult.imag(), pOppNode->V);
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

	KLU_numeric *Numeric = KLU_factor(Ai, Ap, Ax, Symbolic, &Common);
	int sq = KLU_tsolve(Symbolic, Numeric, m_nMatrixSize, 1, b, &Common);
	KLU_free_numeric(&Numeric, &Common);
	return bRes;
}