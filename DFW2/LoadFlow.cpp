#include "stdafx.h"
#include "DynaModel.h"

using namespace DFW2;

bool CDynaModel::LoadFlow()
{
	bool bRes = false;
	CDynaNodeContainer *pNodes = static_cast<CDynaNodeContainer*>(GetDeviceContainer(DEVTYPE_NODE));
	if (!pNodes)
		return bRes;

	size_t nMatrixSize = 0;
	size_t nNonZeroCount = 0;

	struct _MatrixInfo
	{
		ptrdiff_t OnNodeIndex;
		size_t nRowCount;
		_MatrixInfo::_MatrixInfo() : OnNodeIndex(-1), nRowCount(0) {}
	};

	size_t nOnNodesCount = 0;

	_MatrixInfo *MatrixInfo = new _MatrixInfo[pNodes->Count()];
	_MatrixInfo *pMatrixInfo = MatrixInfo;


	for (DEVICEVECTORITR it = pNodes->begin(); it != pNodes->end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);

		if (pNode->IsStateOn())
		{
			pMatrixInfo->OnNodeIndex = nOnNodesCount++;
			pMatrixInfo->nRowCount++;
			CLinkPtrCount *pBranchLink = pNode->GetLink(0);
			pNode->ResetVisited();
			CDevice **ppDevice = nullptr;
			while (pBranchLink->In(ppDevice))
			{
				CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppDevice);
				if (pBranch->m_BranchState == CDynaBranch::BRANCH_ON)
				{
					CDynaNodeBase *pOppNode = pBranch->m_pNodeIp == pNode ? pBranch->m_pNodeIq : pBranch->m_pNodeIp;
					if (pNode->CheckAddVisited(pOppNode))
						pMatrixInfo->nRowCount++;
				}
			}
			nMatrixSize++;
			nNonZeroCount += 4 * pMatrixInfo->nRowCount;
		}
		else
			pNode->V = pNode->Delta = 0.0;

		pMatrixInfo++;
	}

	nMatrixSize *= 2;

	double *Ax = new double[nNonZeroCount];
	double *b = new double[nMatrixSize];
	ptrdiff_t *Ai = new ptrdiff_t[nMatrixSize + 1];
	ptrdiff_t *Ap = new ptrdiff_t[nNonZeroCount];

	double *pb = b;
	double *pAx = Ax;
	ptrdiff_t *pAp = Ap;

	pMatrixInfo = MatrixInfo;

	for (DEVICEVECTORITR it = pNodes->begin(); it != pNodes->end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		if (pNode->IsStateOn())
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
			*pAp = *(pAp + pMatrixInfo->nRowCount) = pMatrixInfo->OnNodeIndex;

			CLinkPtrCount *pBranchLink = pNode->GetLink(0);
			pNode->ResetVisited();
			CDevice **ppDevice = nullptr;
			while (pBranchLink->In(ppDevice))
			{
				CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppDevice);
				// определяем узел на противоположном конце инцидентной ветви
				CDynaNodeBase *pOppNode = pBranch->m_pNodeIp == pNode ? pBranch->m_pNodeIq : pBranch->m_pNodeIp;
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
				bool bDup = pNode->CheckAddVisited(pOppNode);

				*pAx = mult.imag();
				*(pAx + 1) = -CDevice::ZeroDivGuard(mult.real(), pOppNode->V);
				*(pAx + pMatrixInfo->nRowCount * 2) = mult.real();
				*(pAx + pMatrixInfo->nRowCount * 2 + 1) = CDevice::ZeroDivGuard(mult.imag(), pOppNode->V);
			

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
			*(pAxSelf + pMatrixInfo->nRowCount * 2) = dQdDelta;
			*(pAxSelf + 1) = dPdV;
			*(pAxSelf + pMatrixInfo->nRowCount * 2 + 1) = dQdV;

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