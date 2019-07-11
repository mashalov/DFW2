#include "stdafx.h"
#include "LoadFlow.h"
#include "DynaModel.h"

using namespace DFW2;

CLoadFlow::CLoadFlow(CDynaModel *pDynaModel) : m_pDynaModel(pDynaModel),
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
	// ������� �������� ����� � ���������� �� ������� �������
	// ������ ����� �� ���������� �����. �������� ������ ������� ����� ������ ��
	// ���������� ����������� ����� � ��
	m_pMatrixInfo = new _MatrixInfo[pNodes->Count()];
	_MatrixInfo *pMatrixInfo = m_pMatrixInfo;

	list<CDynaNodeBase*> SlackBuses;
	for (DEVICEVECTORITR it = pNodes->begin(); it != pNodes->end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		// ������������ ������ ���������� ����
		if (!pNode->IsStateOn())
			continue;
		// ��������� VreVim ����
		pNode->InitLF();
		// ��������� �� � ������ �� ��� ���������� ���������
		if (pNode->m_eLFNodeType == CDynaNodeBase::eLFNodeType::LFNT_BASE && pNode->IsStateOn())
			SlackBuses.push_back(pNode);

		// ������� ��� ����, ������� ��
		CLinkPtrCount *pBranchLink = pNode->GetLink(0);
		pNode->ResetVisited();
		CDevice **ppDevice = nullptr;
		while (pBranchLink->In(ppDevice))
		{
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppDevice);
			// ���� ����� ��������, ���� �� ��������������� ����� ����� ������ ���� �������
			if (pBranch->m_BranchState == CDynaBranch::BRANCH_ON)
			{
				CDynaNodeBase *pOppNode = pBranch->GetOppositeNode(pNode);
				if (pNode->CheckAddVisited(pOppNode) < 0)
				{
					if (NodeInMatrix(pNode))
					{
						// ��� ����� ������� � ������� ������� ���������� ������
						// � ��������� ���������
						pMatrixInfo->nBranchCount++;		// ���������� ������, ������� ����� �� ��
						if (NodeInMatrix(pOppNode))
							pMatrixInfo->nRowCount += 2;	// ���������� ��������� ��������� = ������ - ������ �� ��
					}
					else
						m_nBranchesCount++; // ��� �� ������� ����� ���������� ������
				}
			}
		}

		if (NodeInMatrix(pNode))
		{
			// ��� �����, ������� �������� � ������� �������� ���������� ���� �������� �������
			pNode->SetMatrixRow(m_nMatrixSize);
			pMatrixInfo->pNode = pNode;
			m_nMatrixSize += 2;				// �� ���� 2 ��������� � �������
			pMatrixInfo->nRowCount += 2;	// ������� ������������ �������
			m_nNonZeroCount += 2 * pMatrixInfo->nRowCount;	// ���������� ��������� ��������� ����������� �� ���������� ������������ ��������� � ������ (4 double �� �������)
			m_nBranchesCount += pMatrixInfo->nBranchCount;	// ����� ���������� ������ ��� ������� ������� ������ �� �����
			pMatrixInfo++;
		}
	}

	m_pMatrixInfoEnd = pMatrixInfo;								// ����� ���� �� ������� ��� �����, ������� � �������

	Ax = new double[m_nNonZeroCount];							// ����� �������
	b = new double[m_nMatrixSize];								// ������ ������ �����
	Ai = new ptrdiff_t[m_nMatrixSize + 1];						// ������ �������
	Ap = new ptrdiff_t[m_nNonZeroCount];						// ������� �������
	m_pVirtualBranches = new _VirtualBranch[m_nBranchesCount];	// ����� ������ ������ �� �����, ����������� ����������� ������ _MatrixInfo
	_VirtualBranch *pBranches = m_pVirtualBranches;

	ptrdiff_t *pAi = Ai;
	ptrdiff_t *pAp = Ap;
	ptrdiff_t nRowPtr = 0;

	for (_MatrixInfo *pMatrixInfo = m_pMatrixInfo; pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
	{
		CDynaNodeBase *pNode = pMatrixInfo->pNode;
		// ��������� ��������� ����� ������� �� ��� �� ����
		*pAi = nRowPtr;	pAi++;
		nRowPtr += pMatrixInfo->nRowCount;
		*pAi = nRowPtr;	pAi++;
		nRowPtr += pMatrixInfo->nRowCount;

		// ��������� ������ �������� � ���� ������� ��������� ����
		*pAp = pNode->A(0);
		*(pAp + pMatrixInfo->nRowCount) = pNode->A(0);
		pAp++;
		*pAp = pNode->A(1);
		*(pAp + pMatrixInfo->nRowCount) = pNode->A(1);
		pAp++;

		// ����������� ������ ������ � ���� ����
		pMatrixInfo->pBranches = pBranches;
		CLinkPtrCount *pBranchLink = pNode->GetLink(0);
		pNode->ResetVisited();
		CDevice **ppDevice = nullptr;
		while (pBranchLink->In(ppDevice))
		{
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppDevice);
			if (pBranch->m_BranchState == CDynaBranch::BRANCH_ON)
			{
				// ������� ���������� ����� ����� ��� � ��� �������� ������������ ����
				CDynaNodeBase *pOppNode = pBranch->GetOppositeNode(pNode);
				// �������� ������������ � ����������� ����
				cplx *pYkm = pBranch->m_pNodeIp == pNode ? &pBranch->Yip : &pBranch->Yiq;
				// ���������, ��� ������ ������ ���������� ���� ��� ���������������� ���� ��� ���
				ptrdiff_t DupIndex = pNode->CheckAddVisited(pOppNode);
				if (DupIndex < 0)
				{
					// ���� ��� - ��������� ����� � ������ ������� ���� (������� ����� �� ��)
					pBranches->Y = *pYkm;
					pBranches->pNode = pOppNode;
					if (NodeInMatrix(pOppNode))
					{
						// ���� ���������� ���� � ������� ��������� ������ �������� ��� ����
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
					(pMatrixInfo->pBranches + DupIndex)->Y += *pYkm; // ���� ���������� ���� ��� ������, ����� �� ���������, � ��������� �� ������������ ����������� � ��� ���������� ������
			}
		}
		pAp += pMatrixInfo->nRowCount;
	}

	// �������� ������������ ��
	// ��������� �� "��� �������"

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
				// ������ ��� �� ��, ��� ��� ���������� �����, �������� ��� ��������� � ��������
				// �� ������ ������ ������ �� ��
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
		return false;
	if (!UpdatePQFromGenerators())
		return false;

	for (DEVICEVECTORITR it = pNodes->begin(); it != pNodes->end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
#ifdef _DEBUG
		pNode->Vrastr = pNode->V;
		pNode->Deltarastr = pNode->Delta;
		pNode->Qgrastr = pNode->Qg;
		pNode->Pnrrastr = pNode->Pn;
		pNode->Qnrrastr = pNode->Qn;
#endif
		if (pNode->IsStateOn())
		{
			if (pNode->m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_BASE)
			{
				if (pNode->LFQmax >= pNode->LFQmin && (fabs(pNode->LFQmax) > m_Parameters.m_Imb || fabs(pNode->LFQmin) > m_Parameters.m_Imb) && pNode->LFVref > 0.0)
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
		pNode->InitLF();
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

	// � ������ ��������� ��
	for (; pMatrixInfo >= m_pMatrixInfoEnd; pMatrixInfo--)
	{
		SeidellOrder.push_back(pMatrixInfo);
		pMatrixInfo->bVisited = true;
	}

	// ����� PV ����
	for (; pMatrixInfo >= m_pMatrixInfo; pMatrixInfo--)
	{
		CDynaNodeBase *pNode = pMatrixInfo->pNode;
		if (!pNode->IsLFTypePQ())
		{
			SeidellOrder.push_back(pMatrixInfo);
			pMatrixInfo->bVisited = true;
		}
	}

	// ��������� PV ���� �� �������� ��������� ���������� ��������
	sort(SeidellOrder.begin() + (m_pMatrixInfoSlackEnd - m_pMatrixInfoEnd), SeidellOrder.end(), SortPV);
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
				dPreviousImb = ImbNorm(pNodes->m_IterationControl.m_MaxImbP.GetDiff(), pNodes->m_IterationControl.m_MaxImbQ.GetDiff());
			}
			else
			{
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

		pNodes->m_IterationControl.Reset();

		bool bPVPQSwitchEnabled = nSeidellIterations >= m_Parameters.m_nEnableSwitchIteration;

		for (MATRIXINFOITR it = SeidellOrder.begin(); it != SeidellOrder.end(); it++)
		{
			pMatrixInfo = *it;
			CDynaNodeBase *pNode = pMatrixInfo->pNode;
			pNode->GetPnrQnr();
			double& Pe = pMatrixInfo->m_dImbP;
			double& Qe = pMatrixInfo->m_dImbQ;

			Pe = pNode->GetSelfImbP();
			Qe = pNode->GetSelfImbQ();

			cplx Unode(pNode->Vre, pNode->Vim);
			for (_VirtualBranch *pBranch = pMatrixInfo->pBranches; pBranch < pMatrixInfo->pBranches + pMatrixInfo->nBranchCount; pBranch++)
			{
				CDynaNodeBase *pOppNode = pBranch->pNode;
				cplx mult = conj(Unode) * cplx(pOppNode->Vre, pOppNode->Vim) * pBranch->Y;
				Pe -= mult.real();
				Qe += mult.imag();
			}

			double Q = Qe + pNode->Qg;	// ��������� ��������� � ����

			cplx I1 = dStep / conj(Unode) / pNode->Yii;

			switch (pNode->m_eLFNodeType)
			{
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMAX:
				if (pNode->V > pNode->LFVref/* && Q < pNode->LFQmax*/)
				{
					pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PV;
					pMatrixInfo->m_nPVSwitchCount++;
					pNode->Qg = Q;
					cplx dU = I1 * cplx(Pe, 0);
					dU += Unode;
					dU = pNode->LFVref * dU / abs(dU);
					pNode->Vre = dU.real();
					pNode->Vim = dU.imag();
				}
				else
				{
					pNode->Qg = pNode->LFQmax;
					cplx dU = I1 * cplx(Pe, -Qe);
					pNode->Vre += dU.real();
					pNode->Vim += dU.imag();
				}
				break;
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMIN:
				if (pNode->V < pNode->LFVref/* && Q > pNode->LFQmin*/)
				{
					pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PV;
					pMatrixInfo->m_nPVSwitchCount++;
					pNode->Qg = Q;
					cplx dU = I1 * cplx(Pe, 0);
					dU += Unode;
					dU = pNode->LFVref * dU / abs(dU);
					pNode->Vre = dU.real();
					pNode->Vim = dU.imag();
				}
				else
				{
					pNode->Qg = pNode->LFQmin;
					cplx dU = I1 * cplx(Pe, -Qe);
					pNode->Vre += dU.real();
					pNode->Vim += dU.imag();
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

					dU += Unode;
					dU = pNode->LFVref * dU / abs(dU);
					pNode->Vre = dU.real();
					pNode->Vim = dU.imag();
				}
				else
				{
					cplx dU = I1 * cplx(Pe, -Qe);
					pNode->Vre += dU.real();
					pNode->Vim += dU.imag();

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
				pNode->Pg += Pe;
				pNode->Qg += Qe;
				break;
			}

			Unode.real(pNode->Vre);
			Unode.imag(pNode->Vim);

			pNode->V = abs(Unode);
			pNode->Delta = arg(Unode);

			if (pNode->m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_BASE)
				pNodes->IterationControl().Update(pMatrixInfo);
		}

		if (!CheckLF())
		{
			bRes = false;
			break;
		}

		pNodes->DumpIterationControl();

		if (pNodes->m_IterationControl.Converged(m_Parameters.m_Imb))
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

	// ������� ������ ���� � �������
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

		cplx Unode(pNode->Vre, pNode->Vim);
		for (_VirtualBranch *pBranch = pMatrixInfo->pBranches; pBranch < pMatrixInfo->pBranches + pMatrixInfo->nBranchCount; pBranch++)
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

// ������ ��������� � ����
void CLoadFlow::GetNodeImb(_MatrixInfo *pMatrixInfo)
{
	CDynaNodeBase *pNode = pMatrixInfo->pNode;
	// �������� �� ���
	pNode->GetPnrQnr();
	pMatrixInfo->m_dImbP = pNode->GetSelfImbP();
	pMatrixInfo->m_dImbQ = pNode->GetSelfImbQ();
	cplx Unode(pNode->Vre, pNode->Vim);
	for (_VirtualBranch *pBranch = pMatrixInfo->pBranches; pBranch < pMatrixInfo->pBranches + pMatrixInfo->nBranchCount; pBranch++)
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

	bool bRes = Start();
	bRes = bRes && Estimate();
	if (!bRes)
		return bRes;

	pNodes->SwitchLRCs(false);

	if (m_Parameters.m_bStartup)
	{
		m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, CDFW2Messages::m_cszLFRunningSeidell);
		Seidell();
	}

	m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, CDFW2Messages::m_cszLFRunningNewton);
	int it(0);

	_MatrixInfo **ppSwitch = new _MatrixInfo *[m_nMatrixSize];

	double ImbSqOld = 0.0;
	double ImbSq = 0.0;

	while (1)
	{
		if (!CheckLF())
		{
			bRes = false;
			break;
		}

		++it;
		_MatrixInfo **ppSwitchEnd = ppSwitch;
		pNodes->m_IterationControl.Reset();
		ImbSqOld = ImbSq;
		ImbSq = 0.0;

		// ������� �������� �� ���� ����� ����� ��
		_MatrixInfo *pMatrixInfo = m_pMatrixInfo;
		for (; pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
		{
			CDynaNodeBase *pNode = pMatrixInfo->pNode;
			GetNodeImb(pMatrixInfo);
			double Qg = pNode->Qg + pMatrixInfo->m_dImbQ;
			switch (pNode->m_eLFNodeType)
			{
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMIN:
				if (pNode->V < pNode->LFVref)
				{
					*ppSwitchEnd = pMatrixInfo;
					pMatrixInfo->m_nPVSwitchCount++;
					ppSwitchEnd++;
				}
				break;
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMAX:
				if (pNode->V > pNode->LFVref)
				{
					*ppSwitchEnd = pMatrixInfo;
					ppSwitchEnd++;
					pMatrixInfo->m_nPVSwitchCount++;
				}
				break;
			case CDynaNodeBase::eLFNodeType::LFNT_PV:
				if (Qg > pNode->LFQmax + m_Parameters.m_Imb)
				{
					pNodes->m_IterationControl.m_nQviolated++;
					*ppSwitchEnd = pMatrixInfo;
					ppSwitchEnd++;
				}
				else if (Qg < pNode->LFQmin - m_Parameters.m_Imb)
				{
					pNodes->m_IterationControl.m_nQviolated++;
					*ppSwitchEnd = pMatrixInfo;
					ppSwitchEnd++;
				}
				else
					pNode->Qg = Qg;
				break;
			}
			pNodes->IterationControl().Update(pMatrixInfo);
			ImbSq += ImbNorm(pMatrixInfo->m_dImbP, pMatrixInfo->m_dImbQ);
		}

		// ����������� ��������� � ��
		for (pMatrixInfo = m_pMatrixInfoEnd; pMatrixInfo < m_pMatrixInfoSlackEnd; pMatrixInfo++)
		{
			CDynaNodeBase *pNode = pMatrixInfo->pNode;
			GetNodeImb(pMatrixInfo);
			pNode->Pg += pMatrixInfo->m_dImbP;
			pNode->Qg += pMatrixInfo->m_dImbQ;
		}

		pNodes->IterationControl().m_ImbRatio = ImbSq;
		pNodes->DumpIterationControl();

		if (pNodes->m_IterationControl.Converged(m_Parameters.m_Imb))
			break;

		if (it > m_Parameters.m_nMaxIterations)
		{
			m_pDynaModel->Log(CDFW2Messages::DFW2LOG_ERROR, CDFW2Messages::m_cszLFNoConvergence);
			bRes = false;
			break;
		}
		// ����������� ���� �����
		if (pNodes->m_IterationControl.m_MaxImbP.GetDiff() < m_Parameters.m_Imb)
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
						pNodes->m_IterationControl.m_nQviolated++;
						//m_pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, _T("Qmax < Q %g < %g %d"), pNode->LFQmax, pNode->Qg, pNode->GetId());
						pNode->Qg = pNode->LFQmax;
						pNode->m_eLFNodeType = CDynaNodeBase::eLFNodeType::LFNT_PVQMAX;
					}
					else if (pNode->Qg < pNode->LFQmin)
					{
						pNodes->m_IterationControl.m_nQviolated++;
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

		ImbSq = 0.0;
		for (double *pb = b; pb < b + m_nMatrixSize; pb++)
			ImbSq += *pb * *pb;

		double dStep = 1.0;

		if (ImbSqOld > 0.0 && ImbSq > 0.0)
		{
			double ImbSqRatio = ImbSq / ImbSqOld;
			if (ImbSqRatio > 1.0)
			{
				dStep = 1.0 / ImbSqRatio;
			}
		}

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

#ifdef _DEBUG

	ATLTRACE(_T("\n%g %g"), m_pMatrixInfoEnd->pNode->Pg, m_pMatrixInfoEnd->pNode->Qg);
	FILE *s;
	fopen_s(&s, "c:\\tmp\\nodes.csv", "w+");

	CDynaNodeBase *pNodeMaxV(nullptr);
	CDynaNodeBase *pNodeMaxDelta(nullptr);
	CDynaNodeBase *pNodeMaxQg(nullptr);
	CDynaNodeBase *pNodeMaxPnr(nullptr);
	CDynaNodeBase *pNodeMaxQnr(nullptr);

	for (DEVICEVECTORITR it = pNodes->begin(); it != pNodes->end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		if (pNode->IsStateOn())
		{
			double mx = fabs(pNode->V - pNode->Vrastr);
			if (pNodeMaxV)
			{
				if (mx > fabs(pNodeMaxV->V - pNodeMaxV->Vrastr))
					pNodeMaxV = pNode;
			}
			else
				pNodeMaxV = pNode;

			mx = fabs(pNode->Delta - pNode->Deltarastr);
			if (pNodeMaxDelta)
			{
				if (mx > fabs(pNodeMaxDelta->V - pNodeMaxDelta->Vrastr))
					pNodeMaxDelta = pNode;
			}
			else
				pNodeMaxDelta = pNode;


			mx = fabs(pNode->Qg - pNode->Qgrastr);
			if (pNodeMaxQg)
			{
				if (mx > fabs(pNodeMaxQg->Qg - pNodeMaxQg->Qgrastr))
					pNodeMaxQg = pNode;
			}
			else
				pNodeMaxQg = pNode;

			mx = fabs(pNode->Qnr - pNode->Qnrrastr);
			if (pNodeMaxQnr)
			{
				if (mx > fabs(pNodeMaxQnr->Qnr - pNodeMaxQnr->Qnrrastr))
					pNodeMaxQnr = pNode;
			}
			else
				pNodeMaxQnr = pNode;

			mx = fabs(pNode->Pnr - pNode->Pnrrastr);
			if (pNodeMaxPnr)
			{
				if (mx > fabs(pNodeMaxPnr->Pnr - pNodeMaxQnr->Pnrrastr))
					pNodeMaxPnr = pNode;
			}
			else
				pNodeMaxPnr = pNode;
		}
		//ATLTRACE("\n %20f %20f %20f %20f %20f %20f", pNode->V, pNode->Delta * 180 / M_PI, pNode->Pg, pNode->Qg, pNode->Pnr, pNode->Qnr);
		fprintf(s, "%d;%20g;%20g\n", pNode->GetId(), pNode->V, pNode->Delta * 180 / M_PI);
	}
	m_pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, _T("Rastr differences V %g Delta %g Qg %g Pnr %g Qnr %g"),
		pNodeMaxV->V - pNodeMaxV->Vrastr,
		pNodeMaxDelta->Delta - pNodeMaxDelta->Deltarastr,
		pNodeMaxQg->Qg - pNodeMaxQg->Qgrastr,
		pNodeMaxPnr->Pnr - pNodeMaxPnr->Pnrrastr,
		pNodeMaxQnr->Qnr - pNodeMaxQnr->Qnrrastr);

	fclose(s);
#endif

	pNodes->SwitchLRCs(true);
	UpdateQToGenerators();
	DumpNodes();
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

	for (_MatrixInfo *pMatrixInfo = m_pMatrixInfo; pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
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

		for (_VirtualBranch *pBranch = pMatrixInfo->pBranches; pBranch < pMatrixInfo->pBranches + pMatrixInfo->nBranchCount; pBranch++)
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


bool CLoadFlow::UpdatePQFromGenerators()
{
	bool bRes = true;

	for (DEVICEVECTORITR it = pNodes->begin(); it != pNodes->end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);

		if (!pNode->IsStateOn())
			continue;

		CLinkPtrCount *pGenLink = pNode->GetLink(1);
		CDevice **ppGen = NULL;
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
	return bRes;
}

bool CLoadFlow::UpdateQToGenerators()
{
	bool bRes = true;
	for (DEVICEVECTORITR it = pNodes->begin(); it != pNodes->end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);

		if (!pNode->IsStateOn())
			continue;

		CLinkPtrCount *pGenLink = pNode->GetLink(1);
		CDevice **ppGen = NULL;
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
					else
						pGen->Q = 0.0;
			}
		}
	}
	return bRes;
}

void CLoadFlow::DumpNodes()
{
	FILE *fdump(nullptr);
	setlocale(LC_ALL, "ru-ru");
	if (!_tfopen_s(&fdump, _T("c:\\tmp\\nodes.csv"), _T("wb+")))
	{
		_ftprintf(fdump, _T("N;V;D;Pn;Qn;Pnr;Qnr;Pg;Qg;Type;Qmin;Qmax;Vref\n"));
		for (DEVICEVECTORITR it = pNodes->begin(); it != pNodes->end(); it++)
		{
			CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
#ifdef _DEBUG
			_ftprintf(fdump, _T("%d;%.12g;%.12g;%g;%g;%g;%g;%g;%g;%d;%g;%g;%g;%.12g;%.12g\n"),
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
				pNode->Deltarastr);
#else
			_ftprintf(fdump, _T("%d;%.12g;%.12g;%g;%g;%g;%g;%g;%g;%d;%g;%g;%g\n"),
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
				pNode->LFVref);
#endif
		}
		fclose(fdump);
	}
}