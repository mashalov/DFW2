#include "stdafx.h"
#include "DynaBranch.h"
#include "DynaModel.h"

using namespace DFW2;

// �� ����� ������������ ������������
// ������� � ����������� ������ KLU ��� x86 � x64
// ��� ����������� ������ ����������� ����������� ������� x86 � x64
// ������������ ��������� klu_version.h
#ifdef DLONG
#define klu_z_factor klu_zl_factor
#define klu_z_refactor klu_zl_refactor
#define klu_z_tsolve klu_zl_tsolve
#define klu_z_free_numeric klu_zl_free_numeric
#define klu_free_symbolic klu_l_free_symbolic
#endif

CDynaNodeBase::CDynaNodeBase() : CDevice(),
								 m_pLRC(NULL),
								 m_pSyncZone(NULL),
								 dLRCVicinity(0.0),
								 //m_dLRCKdef(1.0),
								 m_bInMetallicSC(false)
{
}


CDynaNodeBase::~CDynaNodeBase()
{
}

void CDynaNodeBase::UpdateVreVim()
{
	Vold = V;
	VreVim = polar(V, Delta);
}

void CDynaNodeBase::GetPnrQnr()
{
	Pnr = Pn;
	Qnr = Qn;
	double VdVnom = V / V0;

	dLRCPn = 0.0;
	dLRCQn = 0.0;
	
	if (m_pLRC)
	{
		double dP = 0.0, dQ = 0.0;
		Pnr *= m_pLRC->GetPdP(VdVnom, dLRCPn, dLRCVicinity);
		Qnr *= m_pLRC->GetQdQ(VdVnom, dLRCQn, dLRCVicinity);
		dLRCPn *= Pn / V0;
		dLRCQn *= Qn / V0;
	}
}

bool CDynaNodeBase::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;

	if (GetId() == 1023 && !pDynaModel->EstimateBuild())
		bRes = true;

	GetPnrQnr();

	CLinkPtrCount *pGenLink = GetLink(1);
	CDevice **ppGen = NULL;

	if (pGenLink->m_nCount)
	{
		Pg = Qg = 0.0;
		ResetVisited();
		while (pGenLink->In(ppGen))
		{
			CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*ppGen);
			Pg += pGen->P;
			Qg += pGen->Q;
			pDynaModel->SetElement(A(0), pGen->A(CDynaPowerInjector::V_P), -1.0);
			pDynaModel->SetElement(A(1), pGen->A(CDynaPowerInjector::V_Q), -1.0);
		}
	}

	bool bInMetallicSC = m_bInMetallicSC || V < DFW2_EPSILON;
		
	double Pe = Pnr - Pg - V * V * Yii.real();			// ��������� �� P � Q
	double Qe = Qnr - Qg + V * V * Yii.imag();
		
	double dPdDelta = 0.0;
	double dPdV = -2 * V * Yii.real() + dLRCPn;
	double dQdDelta = 0.0;
	double dQdV = 2 * V * Yii.imag()  + dLRCQn;

	CLinkPtrCount *pBranchLink = GetLink(0);
	CDevice **ppBranch = NULL;

	
	//if (Type)
	{
		ResetVisited();
		while (pBranchLink->In(ppBranch))
		{
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppBranch);
			// ���������� ���� �� ��������������� ����� ����������� �����
			CDynaNodeBase *pOppNode = pBranch->m_pNodeIp == this ? pBranch->m_pNodeIq : pBranch->m_pNodeIp;
			cplx mult = conj(VreVim);
			// ���������� �������� ������������ �� ������� �����
			cplx *pYkm = pBranch->m_pNodeIp == this ? &pBranch->Yip : &pBranch->Yiq;

			mult *= pOppNode->VreVim ** pYkm;

			Pe -= mult.real();
			Qe += mult.imag();

			// diagonals 2

			dPdDelta -=  mult.imag();
			dPdV	 += -ZeroDivGuard(mult.real(), V);
			dQdDelta += -mult.real();
			dQdV     +=  ZeroDivGuard(mult.imag(), V);
			
//			if (pOppNode->Type)
			{
				bool bDup = !CheckAddVisited(pOppNode);
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
			}
		}

		if (!IsStateOn() || bInMetallicSC)
		{
			dQdV = dPdDelta = 1.0;
			dPdV = dQdDelta = 0.0;
			Pe = Qe = 0;
			V = Delta = 0.0;
		}

		pDynaModel->SetElement(A(0), A(0), dPdDelta);
		pDynaModel->SetElement(A(1), A(0), dQdDelta);
		pDynaModel->SetElement(A(0), A(1), dPdV);
		pDynaModel->SetElement(A(1), A(1), dQdV);
	}
//	else
//	{
//		Pe = Qe = 0.0;
//		pDynaModel->SetElement(A(0), A(0), 1.0);
//		pDynaModel->SetElement(A(1), A(1), 1.0);
//	}

	return pDynaModel->Status();
}


bool CDynaNodeBase::BuildRightHand(CDynaModel *pDynaModel)
{
	GetPnrQnr();

	CLinkPtrCount *pGenLink = GetLink(1);
	CDevice **ppGen = NULL;

	if (pGenLink->m_nCount)
	{
		Pg = Qg = 0.0;
		ResetVisited();
		while (pGenLink->In(ppGen))
		{
			CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*ppGen);
			Pg += pGen->P;
			Qg += pGen->Q;
		}
	}

	bool bInMetallicSC = m_bInMetallicSC || V < DFW2_EPSILON;

	double Pe = Pnr - Pg - V * V * Yii.real();
	double Qe = Qnr - Qg + V * V * Yii.imag();

	CLinkPtrCount *pBranchLink = GetLink(0);
	CDevice **ppBranch = NULL;

	double dPdDelta = 0.0;

	ResetVisited();
	while (pBranchLink->In(ppBranch))
	{
		CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppBranch);
		CDynaNodeBase *pOppNode = pBranch->m_pNodeIp == this ? pBranch->m_pNodeIq : pBranch->m_pNodeIp;
		cplx mult = conj(VreVim);
		cplx *pYkm = pBranch->m_pNodeIp == this ? &pBranch->Yip : &pBranch->Yiq;
		mult *= pOppNode->VreVim ** pYkm;
		Pe -= mult.real();
		Qe += mult.imag();
	}

	if (!IsStateOn() || bInMetallicSC)
	{
		Pe = Qe = 0;
		V = Delta = 0.0;
	}

	pDynaModel->SetFunction(A(V_DELTA), Pe);
	pDynaModel->SetFunction(A(V_V), Qe);
	return pDynaModel->Status();
}

bool CDynaNodeBase::NewtonUpdateEquation(CDynaModel* pDynaModel)
{
	bool bRes = true;
	// only update vicinity in case node has LRC ( due to slow complex::abs() )
	//if (m_pLRC)
	//	dLRCVicinity = 30.0 * fabs(abs(VreVim) - V) / Unom;
	if (m_pLRC)
		dLRCVicinity = 30.0 * fabs(Vold - V) / Unom;

	UpdateVreVim();
	return bRes;
}

eDEVICEFUNCTIONSTATUS CDynaNodeBase::Init(CDynaModel* pDynaModel)
{
	UpdateVreVim();

	V0 = (V > 0) ? V : Unom;
		
	return DFS_OK;
}

void CDynaNodeBase::Predict()
{
	dLRCVicinity = 0.0;
	UpdateVreVim();
}

CDynaNode::CDynaNode() : CDynaNodeBase()
{

}

bool CDynaNode::BuildDerivatives(CDynaModel *pDynaModel)
{
	bool bRes = true;
	pDynaModel->SetDerivative(A(V_LAG), (Delta - Lag) / pDynaModel->GetFreqTimeConstant());
	return pDynaModel->Status();
}


bool CDynaNode::BuildEquations(CDynaModel* pDynaModel)
{
	bool bRes = CDynaNodeBase::BuildEquations(pDynaModel);
	double T = pDynaModel->GetFreqTimeConstant();
	double hb0 = pDynaModel->GetHB0();
	double w0 = pDynaModel->GetOmega0();

	pDynaModel->SetElement(A(V_LAG), A(V_DELTA), -hb0 / T);
	pDynaModel->SetElement(A(V_LAG), A(V_LAG), 1.0 + hb0 / T);
	
	
	if (!pDynaModel->IsInDiscontinuityMode())
	{
		pDynaModel->SetElement(A(V_S), A(V_DELTA), -1.0 / T / w0);
		pDynaModel->SetElement(A(V_S), A(V_LAG), IsStateOn() ? 1.0 / T / w0 : 0.0);
		pDynaModel->SetElement(A(V_S), A(V_S), 1.0);
	}
	else
	{
		pDynaModel->SetElement(A(V_S), A(V_DELTA), 0.0);
		pDynaModel->SetElement(A(V_S), A(V_LAG), 0.0);
		pDynaModel->SetElement(A(V_S), A(V_S), 1.0);
	}
	return pDynaModel->Status() && bRes;
}

bool CDynaNode::BuildRightHand(CDynaModel* pDynaModel)
{
	bool bRes = CDynaNodeBase::BuildRightHand(pDynaModel);

	if (GetId() == 392)
		bRes = bRes;

	double T = pDynaModel->GetFreqTimeConstant();
	double w0 = pDynaModel->GetOmega0();
	double dLag = (Delta - Lag) / T;
	double dS = S - (Delta - Lag) / T / w0;

	if (pDynaModel->IsInDiscontinuityMode()) 
		dS = 0.0;
	
	pDynaModel->SetFunctionDiff(A(V_LAG), dLag);
	pDynaModel->SetFunction(A(V_S), dS);

	return pDynaModel->Status() && bRes;
}

eDEVICEFUNCTIONSTATUS CDynaNode::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = CDynaNodeBase::Init(pDynaModel);
	if (CDevice::IsFunctionStatusOK(Status))
	{
		S = 0.0;
		Lag = Delta;
	}
	return Status;
}

double* CDynaNodeBase::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = NULL;
	switch (nVarIndex)
	{
		MAP_VARIABLE(Delta,V_DELTA)
		MAP_VARIABLE(V, V_V)
	}
	return p;
}

double* CDynaNodeBase::GetConstVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = NULL;
	switch (nVarIndex)
	{
		MAP_VARIABLE(Bshunt, C_BSH)
		MAP_VARIABLE(Gshunt, C_GSH)
	}
	return p;
}

double* CDynaNode::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = CDynaNodeBase::GetVariablePtr(nVarIndex);
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(Lag, V_LAG)
			MAP_VARIABLE(S, V_S)
		}
	}
	return p;
}

CDynaNodeContainer::CDynaNodeContainer(CDynaModel *pDynaModel) : 
									   CDeviceContainer(pDynaModel),
									   m_pSynchroZones(NULL)
{
	m_ContainerProps.bNewtonUpdate = m_ContainerProps.bPredict = true;
}

CDynaNodeContainer::~CDynaNodeContainer()
{

}

void CDynaNodeContainer::CalcAdmittances(bool bSeidell)
{
	for (DEVICEVECTORITR it = m_DevVec.begin(); it != m_DevVec.end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		pNode->CalcAdmittances(bSeidell);
	}
}

void CDynaNodeBase::CalcAdmittances(bool bSeidell)
{
	Yii = -cplx(G + Gshunt, B + Bshunt);

	m_bInMetallicSC = !(_finite(Yii.real()) && _finite(Yii.imag()));

	if (m_bInMetallicSC || !IsStateOn())
	{
		VreVim = 0.0;
		V = Delta = 0.0;
	}
	else
	{
		CDevice **ppBranch = NULL;
		CLinkPtrCount *pLink = GetLink(0);
		ResetVisited();
		while (pLink->In(ppBranch))
		{
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppBranch);
			pBranch->CalcAdmittances(bSeidell);

			// TODO Single-end trip

			switch (pBranch->m_BranchState)
			{
			case CDynaBranch::BRANCH_OFF:
				break;
			case CDynaBranch::BRANCH_ON:

				if (this == pBranch->m_pNodeIp)
					Yii -= (pBranch->Yips);
				else
					Yii -= (pBranch->Yiqs);
				break;

			case CDynaBranch::BRANCH_TRIPIP:
				break;
			case CDynaBranch::BRANCH_TRIPIQ:
				break;
			}
		}

		if (V < DFW2_EPSILON)
		{
			V = Unom;
			Delta = 0.0;
		}
	}
}


void CDynaNodeBase::CalcAdmittances()
{
	Yii = -cplx(G + Gshunt, B + Bshunt);

	m_bInMetallicSC = !(_finite(Yii.real()) && _finite(Yii.imag()));

	if (m_bInMetallicSC || !IsStateOn())
	{
		VreVim = 0.0;
		V = Delta = 0.0;
	}
	else
	{
		CDevice **ppBranch = NULL;
		CLinkPtrCount *pLink = GetLink(0);
		ResetVisited();
		while (pLink->In(ppBranch))
		{
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppBranch);

			// TODO Single-end trip

			switch (pBranch->m_BranchState)
			{
			case CDynaBranch::BRANCH_OFF:
				break;
			case CDynaBranch::BRANCH_ON:

				if (this == pBranch->m_pNodeIp)
					Yii -= (pBranch->Yips);
				else
					Yii -= (pBranch->Yiqs);
				break;

			case CDynaBranch::BRANCH_TRIPIP:
				break;
			case CDynaBranch::BRANCH_TRIPIQ:
				break;
			}
		}

		if (V < DFW2_EPSILON)
		{
			V = Unom;
			Delta = 0.0;
		}
	}
}

eDEVICEFUNCTIONSTATUS CDynaNode::SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause)
{
	eDEVICESTATE OldState = GetState();
	eDEVICEFUNCTIONSTATUS Status = CDevice::SetState(eState, eStateCause);

	if (OldState != eState)
	{
		ProcessTopologyRequest();

		switch (eState)
		{
		case eDEVICESTATE::DS_OFF:
			V = Delta = Lag = S = 0.0;
			break;
		case eDEVICESTATE::DS_ON:
			V = Unom;
			Delta = 0;
			Lag = S = 0.0;
			CalcAdmittances();
			break;
		}
	}
	return Status;
}

eDEVICEFUNCTIONSTATUS CDynaNode::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	Lag = Delta - S * pDynaModel->GetFreqTimeConstant() * pDynaModel->GetOmega0();
	return DFS_OK;
}

CSynchroZone::CSynchroZone() : CDevice()
{
	Clear();
}

void CSynchroZone::Clear()
{
	m_bEnergized = false;
	Mj = 0.0;
	S = 0.0;
	m_bPassed = true;
	m_bInfPower = false;
	m_LinkedGenerators.clear();
}

double* CSynchroZone::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = NULL;

	switch (nVarIndex)
	{
		MAP_VARIABLE(S, V_S)
	}
	return p;
}

bool CSynchroZone::BuildEquations(CDynaModel* pDynaModel)
{
	bool bRes = true;
	if (m_bInfPower)
	{
		pDynaModel->SetElement(A(V_S), A(V_S), 1.0);
	}
	else
	{
		pDynaModel->SetElement(A(V_S), A(V_S), 1.0);
		for (DEVICEVECTORITR it = m_LinkedGenerators.begin(); it != m_LinkedGenerators.end(); it++)
		{
			CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*it);
			if(pGen->IsKindOfType(DEVTYPE_GEN_MOTION))
			{
				CDynaGeneratorMotion *pGenMj = static_cast<CDynaGeneratorMotion*>(pGen);
				pDynaModel->SetElement(A(V_S), pGen->A(CDynaGeneratorMotion::V_S), -pGenMj->Mj / Mj);
			}
		}
	}
	return pDynaModel->Status() && bRes;
}


bool CSynchroZone::BuildRightHand(CDynaModel* pDynaModel)
{
	double dS = S;
	if (m_bInfPower)
	{
		pDynaModel->SetFunction(A(V_S), 0.0);
	}
	else
	{
		for (DEVICEVECTORITR it = m_LinkedGenerators.begin(); it != m_LinkedGenerators.end(); it++)
		{
			CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*it);
			if (pGen->IsKindOfType(DEVTYPE_GEN_MOTION))
			{
				CDynaGeneratorMotion *pGenMj = static_cast<CDynaGeneratorMotion*>(pGen);
				dS -= pGenMj->Mj * pGenMj->s / Mj;
			}
		}
		pDynaModel->SetFunction(A(V_S), dS);
	}
	return pDynaModel->Status();
}


eDEVICEFUNCTIONSTATUS CSynchroZone::Init(CDynaModel* pDynaModel)
{
	return DFS_OK;
}

bool CDynaNodeContainer::LULF()
{
	bool bRes = true;

	size_t nNodeCount = m_DevVec.size();
	size_t nBranchesCount = m_pDynaModel->Branches.Count();
	// ������ ���������� ��������� ���������
	size_t nNzCount = nNodeCount + 2 * nBranchesCount;

	// ��������� ������������ ���� ������������� �������� �������
	struct NodeToMatrix
	{
		CDynaNodeBase *pNode;
		double *pMatrixElement;
	};

	// ������ ������� ����� 
	NodeToMatrix *pNodeToMatrix = new NodeToMatrix[nBranchesCount];
	
	// ������� � ������ ������ �����
	double *Ax = new double[2 * nNzCount];
	double *B  = new double[2 * nNodeCount];
	// �������
	ptrdiff_t *Ap = new ptrdiff_t[nNodeCount + 1];
	ptrdiff_t *Ai = new ptrdiff_t[nNzCount];
	KLU_symbolic *Symbolic = NULL;
	KLU_numeric *Numeric = NULL;
	KLU_common Common;
	KLU_defaults(&Common);

	// ������ ���������� �� ��������� �������
	double **pDiags = new double*[nNodeCount];
	double **ppDiags = pDiags;
	double *pB = B;

	double *pAx = Ax;
	ptrdiff_t *pAp = Ap;
	ptrdiff_t *pAi = Ai;

	FILE *fnode(NULL), *fgen(NULL);
	_tfopen_s(&fnode, _T("c:\\tmp\\nodes.csv"), _T("w+"));
	_tfopen_s(&fgen, _T("c:\\tmp\\gens.csv"), _T("w+"));
	_ftprintf(fnode, _T(";"));

	CalcAdmittances(false);

	for (DEVICEVECTORITR it = m_DevVec.begin(); it != m_DevVec.end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		// �������� � ��������
		pNode->VreVim = pNode->V = pNode->Unom;
		pNode->Delta = 0.0;
		pNode->dLRCVicinity = 0.0;		// ���� ����������� ��� ��� ������ ������� - ��� �����������


		*pAp = (pAx - Ax) / 2;    pAp++;
		*pAi = pNode->A(0) / EquationsCount();		  pAi++;
		// ������ ������� ������ ���������� ��� ���������
		// � ���������� ��������� �� ����
		*ppDiags = pAx;
		// ���� ���� ��������, � ������� �� ��� ����� ����� �� � ��������� ������������ ���������
		*pAx = 1.0; pAx++;
		*pAx = 0.0; pAx++;
		ppDiags++;

		if (pNode->IsStateOn() && !pNode->m_bInMetallicSC)
		{
			// ��� ���� �����, ������� �� ��������� � �� ��������� � ������������� �� (�� � ������� ������)
			_ftprintf(fnode, _T("%td;"), pNode->GetId());
			// Branches
			CDevice **ppBranch = NULL;
			CLinkPtrCount *pLink = pNode->GetLink(0);
			pNode->ResetVisited();
			NodeToMatrix *pNodeToMatrixEnd = pNodeToMatrix;

			// ������� ����� ����
			while (pLink->In(ppBranch))
			{
				CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppBranch);
				if (pBranch->m_BranchState == CDynaBranch::BRANCH_ON)
				{
					// ������� �������� ������������ ����� ��� ���������� ������ 
					CDynaNodeBase *pOppNode = pBranch->m_pNodeIp;
					cplx *pYkm = &pBranch->Yiq;
					if (pBranch->m_pNodeIp == pNode)
					{
						pOppNode = pBranch->m_pNodeIq;
						pYkm = &pBranch->Yip;
					}

					// ���������, �� �������� �� ��� ���� ������� ���� � �������
					// ���� �� - ��������� ������������ ������������ ������
					NodeToMatrix *pCheck = pNodeToMatrix;
					while (pCheck < pNodeToMatrixEnd)
					{
						if (pCheck->pNode == pOppNode)
							break;
						pCheck++;
					}

					if (pCheck < pNodeToMatrixEnd)
					{
						// ����� ������� ����, ��������� ������������
						*pCheck->pMatrixElement += pYkm->real();
						*(pCheck->pMatrixElement+1) += pYkm->imag();
					}
					else
					{
						// �� ����� �������� ����, ��������� ���
						pCheck->pNode = pOppNode;
						pCheck->pMatrixElement = pAx;
						pNodeToMatrixEnd++;
						_ASSERTE(pNodeToMatrixEnd - pNodeToMatrix < static_cast<ptrdiff_t>(nBranchesCount));

						*pAx = pYkm->real();   pAx++;
						*pAx = pYkm->imag();   pAx++;
						// ���������� ������ ����� �������� ���� � ������ �������
						*pAi = pOppNode->A(0) / EquationsCount(); pAi++;
					}
				}
			}
		}
	}

	nNzCount = (pAx - Ax) / 2;		// ������������ ������������ ���������� ��������� ��������� (����� �� 2 ������ ��� ��������)
	Ap[nNodeCount] = nNzCount;
	Symbolic = KLU_analyze(nNodeCount, Ap, Ai, &Common);
		
	for (ptrdiff_t nIteration = 0; nIteration < 200; nIteration++)
	{
		ppDiags = pDiags;
		pB = B;

		_ftprintf(fnode, _T("\n%td;"), nIteration);
		_ftprintf(fgen, _T("\n%td;"), nIteration);

		CDynaNodeBase *pdVmax = NULL;		// ��������� �� ���� � ������������ ���������� ����������

		// �������� �� ����� ��� ����������� �� �� ���������, ����������� ���� �� ��������� �������
		for (DEVICEVECTORITR it = m_DevVec.begin(); it != m_DevVec.end(); it++, ppDiags++)
		{
			CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);

			if (!pNode->m_bInMetallicSC && pNode->IsStateOn())
			{
				// ��� ���� ����� ������� �������� � ��� �������������� ��

				double Pnr = pNode->Pn;
				double Qnr = pNode->Qn;
				cplx I = 0.0;					// �������� ����
				cplx Y = pNode->Yii;			// ��������� ������� �� ��������� ����������� ������������ ����

				pNode->Vold = pNode->V;			// ���������� ���������� �� �������� ��� ������� ����������

				if (pNode->m_pLRC)
				{
					// ���� ���� ���, ������� �������� ���� � ������ ���
					double dP = 0.0, dQ = 0.0;
					double VdVnom = pNode->V / pNode->V0;
					Pnr *= pNode->m_pLRC->GetPdP(VdVnom, dP, pNode->dLRCVicinity);
					Qnr *= pNode->m_pLRC->GetQdQ(VdVnom, dQ, pNode->dLRCVicinity);
				}

				// Generators

				CDevice **ppDeivce = NULL;
				CLinkPtrCount *pLink = NULL;
				ppDeivce = NULL;
				pLink = pNode->GetLink(1);
				pNode->ResetVisited();
				// �������� �� �����������
				while (pLink->In(ppDeivce))
				{
					CDynaVoltageSource *pVsource = static_cast<CDynaVoltageSource*>(*ppDeivce);
					// ���� � ���� ���� ���� �� ���� ���������, �� �������� �������� ��������� ����
					// ���� � ���� ��� ����������� �� ���� �������� ��������� - �� ��� ����� �����������
					// �������� �����
					pNode->Pg = pNode->Qg = 0.0;
					// ���� ���������� �������� �� ������ ����������, ��� �������� � ��� ����� ����� ����
					if (!pVsource->IsStateOn())
						continue;

					pVsource->CalculatePower();
					cplx Sg(pVsource->P, pVsource->Q);
					cplx E = pVsource->GetEMF();
					cplx Yg = conj(Sg / pNode->VreVim) / (E - pNode->VreVim);
					// !!!!!!!!!!!!!!! for motion equation !!!!!!!!!!!!!!!!
					//cplx Yg = 1.0 / static_cast<CDynaGeneratorMotion*>(pVsource)->GetXofEqs();

					// � �������� ���� ��������� ��� ����������
					I -= E * Yg;
					// � ��������� ������� ��������� ������������ ����������
					Y -= Yg;
					_CheckNumber(I.real());
					_CheckNumber(I.imag());

					_ftprintf(fgen, _T("%g;"), abs(I));
				}

				// ������������ �������� ��� ���� �� ��������
				// ����� ��������� ���, � ����� ��������� ������� � ���������
				//I += conj(cplx(Pnr - pNode->Pg, Qnr - pNode->Qg) / pNode->VreVim);
				if(pNode->V > 0.0)
					Y += conj(cplx(pNode->Pg - Pnr, pNode->Qg - Qnr) / pNode->V / pNode->V);
				//Y -= conj(cplx(Pnr, Qnr) / pNode->V / pNode->V);

				_CheckNumber(I.real());
				_CheckNumber(I.imag());
				_CheckNumber(Y.real());
				_CheckNumber(Y.imag());
				
				// � ��������� ������ ����������� �����
				*pB = I.real(); pB++;
				*pB = I.imag(); pB++;
				// ��������� ������� ��������� �� Y ����
				**ppDiags = Y.real();
				*(*ppDiags+1) = Y.imag();
				_ftprintf(fnode, _T("%g;"), pNode->V / pNode->V0);
			}
			else
			{
				// ���� ���� �� �������, �������� ��� ��� ���� ����� ����
				*pB = 0.0; pB++;
				*pB = 0.0; pB++;
			}
		}
		
ptrdiff_t refactorOK = 1;

// KLU ����� ������ ��������� ������������ ������� � ��������� ����������
// ��� ������, �� ��� ��������� ������� ����� �������� ��������� ��������������.
// � ��� ���� ��� �������� ������������/�������������� �� �������� LULF

#ifdef USEREFACTOR
		// ������ ������������/�������������� � ���� ��� ����������
		if (nIteration)
			refactorOK = klu_z_refactor(Ap, Ai, Ax, Symbolic, Numeric, &Common);
		else
			Numeric = klu_z_factor(Ap, Ai, Ax, Symbolic, &Common);
#else
		if (Numeric)
			klu_z_free_numeric(&Numeric, &Common);
		Numeric = klu_z_factor(Ap, Ai, Ax, Symbolic, &Common);
#endif

		if (Numeric && refactorOK)
		{
			// ������ �������
			klu_z_tsolve(Symbolic, Numeric, nNodeCount, 1, B, 0, &Common);
			double *pB = B;
			double dVmax = 0.0;

			for (DEVICEVECTORITR it = m_DevVec.begin(); it != m_DevVec.end(); it++)
			{
				CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);

				// ���������� ����� ������� ������� � ������� �������� �����
				pNode->VreVim.real(*pB);		pB++;
				pNode->VreVim.imag(*pB);		pB++;
				// ������� ���������� ���� � �������� �����������
				pNode->V = abs(pNode->VreVim);
				pNode->Delta = arg(pNode->VreVim);

				// ������������ ���� ����������� ��� (����� ��� ��� �������)
				/*if (pNode->m_pLRC)
					pNode->dLRCVicinity = 30.0 * fabs(pNode->Vold - pNode->V) / pNode->Unom;
				*/
				
				// ������� ��������� ���������� ���� ����� ���������� � �������
				// ����� ������������ ����
				double oldV = fabs(pNode->V/pNode->Vold - 1.0);
				if (pdVmax)
				{
					if (oldV > dVmax)
					{
						dVmax = oldV;
						pdVmax = pNode;
					}
				}
				else
				{
					pdVmax = pNode;
					dVmax = oldV;
				}
			}
			if (dVmax < m_pDynaModel->GetRtolLULF())
			{
				Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(CDFW2Messages::m_cszLULFConverged, dVmax, nIteration));
				break;
			}
		}
	}

	if(Numeric)
		klu_z_free_numeric(&Numeric, &Common);

	fclose(fnode);
	fclose(fgen);
	klu_free_symbolic(&Symbolic, &Common);
	delete Ax;
	delete Ap;
	delete Ai;
	delete B;
	delete pDiags;
	delete pNodeToMatrix;
	return bRes;
}


bool CDynaNodeContainer::Seidell()
{
	bool bRes = true;

	LULF();

	return true;

	int nSeidellIterations = 0;
	cplx dVmax;

	for (DEVICEVECTORITR it = m_DevVec.begin(); it != m_DevVec.end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		pNode->CalcAdmittances(true);
	}


	for (nSeidellIterations = 0; nSeidellIterations < 200; nSeidellIterations++)
	{
		dVmax = 0.0;

		double Pn = 0.0;
		double Qn = 0.0;
		cplx dSmax = 0.0;
		ptrdiff_t dSmaxNode = 0;
		ptrdiff_t dVmaxNode = 0;

		for (DEVICEVECTORITR it = m_DevVec.begin(); it != m_DevVec.end(); it++)
		{
			CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);

			if (pNode->IsStateOn())
			{
				double Pnr = pNode->Pn;
				double Qnr = pNode->Qn;

				if (pNode->m_pLRC)
				{
					double dP = 0.0, dQ = 0.0;
					double VdVnom = pNode->V / pNode->V0;
					Pnr *= pNode->m_pLRC->GetPdP(VdVnom, dP, 0.0);
					Qnr *= pNode->m_pLRC->GetQdQ(VdVnom, dQ, 0.0);

					_ASSERTE(dP >= 0.0);
					_ASSERTE(dQ >= 0.0);
				}

				Pn += Pnr;
				Qn += Qnr;

				CDevice **ppDeivce = NULL;
				CLinkPtrCount *pLink = NULL;

				ppDeivce = NULL;
				pLink = pNode->GetLink(1);
				pNode->ResetVisited();
				bool bGenerators = false;

				cplx deltaIk = 0.0;
				cplx YnodeAdd = pNode->Yii;
				
				while (pLink->In(ppDeivce))
				{
					CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*ppDeivce);
					pNode->Pg = pNode->Qg = 0.0;

					if (!pGen->IsStateOn()) 
						continue;

					switch (pGen->GetType())
					{
						case DEVTYPE_GEN_MOTION:
						case DEVTYPE_GEN_INFPOWER:
						{
							CDynaGeneratorInfBus *pGenE = static_cast<CDynaGeneratorInfBus*>(*ppDeivce);
							cplx Yg = 1.0 / pGenE->GetXofEqs();
							YnodeAdd -= Yg;
							cplx E = pGenE->GetEMF();
							deltaIk -= E * Yg;

							_CheckNumber(deltaIk.real());
							_CheckNumber(deltaIk.imag());

							cplx Sg = pNode->VreVim * conj((E - pNode->VreVim) * Yg);
							pGen->P = Sg.real();
							pGen->Q = Sg.imag();

							//if (pGen->GetId() == 305101)
							//	_tcprintf(_T("\n%g %g"), Sg.real(), Sg.imag());
						}
						break;
						case DEVTYPE_GEN_1C:
						{
							CDynaGenerator1C *pGenE = static_cast<CDynaGenerator1C*>(*ppDeivce);

							double Vd = -pNode->V * sin(pGenE->Delta - pNode->Delta);
							double Vq = pNode->V * cos(pGenE->Delta - pNode->Delta);
							double zsq = 1.0 / (pGenE->r * pGenE->r + pGenE->xq * pGenE->xd1);
							double Eqs = pGenE->Eqs;
							double xq = pGenE->xq;
							double xd1 = pGenE->xd1;
							double r = pGenE->r;
							double Id = zsq * (-r * Vd - xq * (Eqs - Vq));
							double Iq = zsq * (r * (Eqs - Vq) - xd1 * Vd);

							pGenE->P = Vd  * Id + Vq * Iq;
							pGenE->Q = Vd  * Iq - Vq * Id;
												
							cplx Sg(pGenE->P, pGenE->Q);
							cplx E = pGenE->GetEMF();
							cplx Yg = conj(Sg / pNode->VreVim) / (E - pNode->VreVim);

							//_tcprintf(_T("\ndYg %g %g "), Yg.real(), Yg.imag());

							deltaIk -= E * Yg;

							_CheckNumber(deltaIk.real());
							_CheckNumber(deltaIk.imag());

							YnodeAdd -= Yg;

						}
						break;
						case DEVTYPE_GEN_3C:
						{
							CDynaGenerator3C *pGenE = static_cast<CDynaGenerator3C*>(*ppDeivce);

							double Vd = -pNode->V * sin(pGenE->Delta - pNode->Delta);
							double Vq = pNode->V * cos(pGenE->Delta - pNode->Delta);
							double zsq = 1.0 / (pGenE->r * pGenE->r + pGenE->xd2 * pGenE->xq2);

							double Id = -zsq * (pGenE->r * (Vd - pGenE->Edss) + pGenE->xq2 * (pGenE->Eqss - Vq));
							double Iq = -zsq * (pGenE->r * (Vq - pGenE->Eqss) + pGenE->xd2 * (Vd - pGenE->Edss));

							pGenE->P = Vd  * Id + Vq * Iq;
							pGenE->Q = Vd  * Iq - Vq * Id;

							cplx Sg(pGenE->P, pGenE->Q);
							cplx E = pGenE->GetEMF();
							cplx Yg = conj(Sg / pNode->VreVim) / (E - pNode->VreVim);

							deltaIk -= E * Yg;

							_CheckNumber(deltaIk.real());
							_CheckNumber(deltaIk.imag());


							YnodeAdd -= Yg;
						}
						break;
						case DEVTYPE_GEN_MUSTANG:
						{
							CDynaGeneratorMustang *pGenE = static_cast<CDynaGeneratorMustang*>(*ppDeivce);

							double Vd = -pNode->V * sin(pGenE->Delta - pNode->Delta);
							double Vq = pNode->V * cos(pGenE->Delta - pNode->Delta);

							CDynaNode *pNodeE = static_cast<CDynaNode*>(pNode);
							double Sv = ((m_pDynaModel->GetFreqDampingType() == APDT_ISLAND) ? pNodeE->m_pSyncZone->S : pNodeE->S);

							double sp2 = 1 + Sv;
							double zsq = 1.0 / (pGenE->xd2 * pGenE->xq2);
							double Id = -zsq * (pGenE->xq2 * (sp2 * pGenE->Eqss - Vq));
							double Iq = -zsq * (pGenE->xd2 * (Vd - sp2 * pGenE->Edss));

							pGenE->P = sp2 * (pGenE->Edss  * Id + pGenE->Eqss * Iq + Id * Iq * (pGenE->xd2 - pGenE->xq2));
							pGenE->Q = Vd  * Iq - Vq * Id;

							cplx Sg(pGenE->P, pGenE->Q);
							cplx E = pGenE->GetEMF();
							cplx Yg = conj(Sg / pNode->VreVim) / (E - pNode->VreVim);

							cplx Eqss = polar(pGenE->Eqss, pGenE->Delta);
							cplx Edss = polar(pGenE->Edss, pGenE->Delta + M_PI / 2.0);

							//deltaIk -= Eqss / cplx(0, pGenE->xd2);
							//deltaIk -= Edss / cplx(0, pGenE->xq2);
							

							deltaIk -= E * Yg;

							_CheckNumber(deltaIk.real());
							_CheckNumber(deltaIk.imag());

							YnodeAdd -= Yg;

							//YnodeAdd -= 1.0 / cplx(0, pGenE->xd2);
							//YnodeAdd -= 1.0 / cplx(0, pGenE->xq2);
						}
						break;
					}
				}

				deltaIk -= YnodeAdd * pNode->VreVim;

				if (!pNode->m_bInMetallicSC)
				{
					deltaIk += conj(cplx(Pnr - pNode->Pg, Qnr - pNode->Qg) / pNode->VreVim);
					ppDeivce = NULL;
					pLink = pNode->GetLink(0);
					pNode->ResetVisited();
					while (pLink->In(ppDeivce))
					{
						CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppDeivce);
						if (pBranch->m_BranchState == CDynaBranch::BRANCH_ON)
						{
							CDynaNodeBase *pn = pBranch->m_pNodeIp == pNode ? pBranch->m_pNodeIq : pBranch->m_pNodeIp;
							cplx *pYkm = pBranch->m_pNodeIp == pNode ? &pBranch->Yip : &pBranch->Yiq;
							deltaIk -= pn->VreVim ** pYkm;
							_CheckNumber(deltaIk.real());
							_CheckNumber(deltaIk.imag());
						}
					}

					_CheckNumber(pNode->VreVim.real());
					_CheckNumber(pNode->VreVim.imag());
	 

					cplx dS = conj(pNode->VreVim) * deltaIk;
					cplx dV = dS / conj(pNode->VreVim);

					if (abs(dS) > abs(dSmax))
					{
						dSmax = dS;
						dSmaxNode = it - m_DevVec.begin();
					}


					if (abs(YnodeAdd) > DFW2_EPSILON)
						dV /= YnodeAdd;



					if (abs(dVmax) < abs(dV))
					{
						dVmax = dV;
						dVmaxNode = it - m_DevVec.begin();
					}

					pNode->VreVim += dV;
					pNode->V = abs(pNode->VreVim);
					pNode->Delta = arg(pNode->VreVim);
				}
				else
				{
					pNode->VreVim = 0.0;
					pNode->V = pNode->Delta = 0.0;
				}

			}
			else
			{
				pNode->VreVim = 0.0;
				pNode->V = pNode->Delta = 0.0;
			}
		}

		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*(m_DevVec.begin() + dVmaxNode));

		_tcprintf(_T("\ndVmax node %d %g %g %g %g "), pNode->GetId(), dVmax.real(), dVmax.imag(), pNode->V, pNode->Delta);

		pNode = static_cast<CDynaNodeBase*>(*(m_DevVec.begin() + dSmaxNode));

		_tcprintf(_T("dSmax node %d %g %g %g %g"), pNode->GetId(), dSmax.real(), dSmax.imag(), pNode->V, pNode->Delta);

		//if (abs(dVmax) < 1)
		//	break;

		/*
		for (DEVICEVECTORITR it = m_DevVec.begin(); it != m_DevVec.end(); it++)
		{
			CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
			_tcprintf(_T("\n%30s %6.2f %6.2f -> %6.2f %6.2f %6.2f %6.2f"), pNode->GetVerbalName(), pNode->V, pNode->Delta * 180 / 3.14159, abs(pNode->VreVim), arg(pNode->VreVim) * 180 / 3.14159, pNode->Pg, pNode->Qg);
			CDevice **ppDeivce = NULL;
			CLinkPtrCount *pLink = pNode->GetLink(1);
			pNode->ResetVisited();
			while (pLink->In(ppDeivce))
			{
				CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*ppDeivce);
				_tcprintf(_T("%30s %6.2f %6.2f"), pGen->GetVerbalName(), pGen->P, pGen->Q);
			}
			pNode->V = abs(pNode->VreVim);
			pNode->Delta = arg(pNode->VreVim);
		}
		*/
	}

	for (DEVICEVECTORITR it = m_DevVec.begin(); it != m_DevVec.end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		pNode->CalcAdmittances(false);
	}



	_tcprintf(_T("\nSeidell converged in %d to %g"), nSeidellIterations, abs(dVmax));

	/*
	for (DEVICEVECTORITR it = m_DevVec.begin(); it != m_DevVec.end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		_tcprintf(_T("\n%30s %6.2f %6.2f -> %6.2f %6.2f %6.2f %6.2f"), pNode->GetVerbalName(), pNode->V, pNode->Delta * 180 / 3.14159, abs(pNode->VreVim), arg(pNode->VreVim) * 180 / 3.14159, pNode->Pg, pNode->Qg);
		CDevice **ppDeivce = NULL;
		CLinkPtrCount *pLink = pNode->GetLink(1);
		pNode->ResetVisited();
		while (pLink->In(ppDeivce))
		{
			CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*ppDeivce);
			_tcprintf(_T("%30s %6.2f %6.2f"), pGen->GetVerbalName(), pGen->P, pGen->Q);
		}
		pNode->V = abs(pNode->VreVim);
		pNode->Delta = arg(pNode->VreVim);
	}

	for (DEVICEVECTORITR it = m_DevVec.begin(); it != m_DevVec.end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		CDevice **ppDeivce = NULL;
		CLinkPtrCount *pLink = NULL;
		ppDeivce = NULL;
		pLink = pNode->GetLink(1);
		pNode->ResetVisited();
		bool bGenerators = false;
		while (pLink->In(ppDeivce))
		{
			if (!bGenerators)
			{
				bGenerators = true;
				pNode->Pg = pNode->Qg = 0.0;
			}
			CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*ppDeivce);
			pNode->Pg += pGen->P;
			pNode->Qg += pGen->Q;

		}
	}
	*/
	return bRes;
}

ExternalVariable CDynaNodeBase::GetExternalVariable(const _TCHAR* cszVarName)
{
	if (!_tcscmp(cszVarName, CDynaNode::m_cszSz))
	{
		ExternalVariable ExtVar = { NULL, -1 };

		if (m_pSyncZone)
			ExtVar = m_pSyncZone->GetExternalVariable(CDynaNode::m_cszS);
		return ExtVar;
	}
	else
		return CDevice::GetExternalVariable(cszVarName);
}

const CDeviceContainerProperties CDynaNodeBase::DeviceProperties()
{
	CDeviceContainerProperties props;
	props.eDeviceType = DEVTYPE_NODE;
	props.SetType(DEVTYPE_NODE);

	props.AddLinkFrom(DEVTYPE_BRANCH, DLM_MULTI, DPD_SLAVE);
	props.AddLinkFrom(DEVTYPE_POWER_INJECTOR, DLM_MULTI, DPD_SLAVE);

	props.nEquationsCount = CDynaNodeBase::VARS::V_LAST;
	props.bPredict = props.bNewtonUpdate = true;

	props.m_VarMap.insert(make_pair(CDynaNodeBase::m_cszDelta, CVarIndex(V_DELTA,VARUNIT_RADIANS)));
	props.m_VarMap.insert(make_pair(CDynaNodeBase::m_cszV, CVarIndex(V_V, VARUNIT_KVOLTS)));
	return props;
}

const CDeviceContainerProperties CDynaNode::DeviceProperties()
{
	CDeviceContainerProperties props = CDynaNodeBase::DeviceProperties();
	props.m_strClassName = CDeviceContainerProperties::m_cszNameNode;
	props.nEquationsCount = CDynaNode::VARS::V_LAST;
	props.m_VarMap.insert(make_pair(CDynaNode::m_cszS, CVarIndex(V_S, VARUNIT_PU)));
	props.m_lstAliases.push_back(CDeviceContainerProperties::m_cszAliasNode);
	props.m_ConstVarMap.insert(make_pair(CDynaNode::m_cszGsh, CConstVarIndex(CDynaNode::C_GSH, eDVT_INTERNALCONST)));
	props.m_ConstVarMap.insert(make_pair(CDynaNode::m_cszBsh, CConstVarIndex(CDynaNode::C_BSH, eDVT_INTERNALCONST)));
	return props;
}

const CDeviceContainerProperties CSynchroZone::DeviceProperties()
{
	CDeviceContainerProperties props;
	props.eDeviceType = DEVTYPE_SYNCZONE;
	props.nEquationsCount = CSynchroZone::VARS::V_LAST;
	props.m_VarMap.insert(make_pair(CDynaNode::m_cszS, CVarIndex(0,VARUNIT_PU)));
	return props;
}


const _TCHAR *CDynaNodeBase::m_cszV = _T("V");
const _TCHAR *CDynaNodeBase::m_cszDelta = _T("Delta");
const _TCHAR *CDynaNodeBase::m_cszGsh = _T("gsh");
const _TCHAR *CDynaNodeBase::m_cszBsh = _T("bsh");

const _TCHAR *CDynaNode::m_cszS = _T("S");
const _TCHAR *CDynaNode::m_cszSz = _T("Sz");
