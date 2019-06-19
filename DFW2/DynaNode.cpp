#include "stdafx.h"
#include "DynaBranch.h"
#include "DynaModel.h"

using namespace DFW2;

// Мы хотим использовать одновременно
// обычную и комплексную версии KLU для x86 и x64
// Для комплексной версии потребоваля собственный маппинг x86 и x64
// параллельный стоковому klu_version.h
#ifdef DLONG
#define klu_z_factor klu_zl_factor
#define klu_z_refactor klu_zl_refactor
#define klu_z_tsolve klu_zl_tsolve
#define klu_z_free_numeric klu_zl_free_numeric
#define klu_free_symbolic klu_l_free_symbolic
#endif

CDynaNodeBase::CDynaNodeBase() : CDevice(),
								 m_pLRC(nullptr),
								 m_pLRCLF(nullptr),
								 m_pSyncZone(nullptr),
								 dLRCVicinity(0.0),
								 //m_dLRCKdef(1.0),
								 m_bInMetallicSC(false)
{
}


CDynaNodeBase::~CDynaNodeBase()
{
}

// запоминает значение модуля напряжения с предыдущей итерации
// и рассчитываем напряжение в декартовых координатах
void CDynaNodeBase::UpdateVreVim()
{
	Vold = V;
	cplx VreVim(polar(V, Delta));
	Vre = VreVim.real();
	Vim = VreVim.imag();
}

// рассчитывает нагрузку узла с учетом СХН
void CDynaNodeBase::GetPnrQnr()
{
	// по умолчанию нагрузка равна заданной в УР
	Pnr = Pn;
	Qnr = Qn;
	double VdVnom = V / V0;

	dLRCPn = 0.0;
	dLRCQn = 0.0;
	
	// если есть СХН, рассчитываем
	// комплексную мощность и проивзодные по напряжению
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

	if (GetId() == 319 && !pDynaModel->EstimateBuild())
		bRes = true;

	GetPnrQnr();

	CLinkPtrCount *pBranchLink = GetLink(0);
	CDevice **ppBranch = nullptr;
	CLinkPtrCount *pGenLink = GetLink(1);
	CDevice **ppGen = nullptr;

	bool bInMetallicSC = m_bInMetallicSC || (V < DFW2_EPSILON && IsStateOn());

	//_ASSERTE(!bInMetallicSC);
		
	//double Pe = Pnr - Pg - V * V * Yii.real();			// небалансы по P и Q
	//double Qe = Qnr - Qg + V * V * Yii.imag();

	/*
	double Pe = GetSelfImbP();
	double Qe = GetSelfImbQ(); 
	double dPdDelta = 0.0;
	double dPdV = GetSelfdPdV(); 
	double dQdDelta = 0.0;
	double dQdV = GetSelfdQdV(); 

	CLinkPtrCount *pBranchLink = GetLink(0);
	CDevice **ppBranch = nullptr;

	ResetVisited();
	while (pBranchLink->In(ppBranch))
	{
		CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppBranch);
		// определяем узел на противоположном конце инцидентной ветви
		CDynaNodeBase *pOppNode = pBranch->GetOppositeNode(this);
		cplx mult = conj(VreVim);
		// определяем взаимную проводимость со смежным узлом
		cplx *pYkm = pBranch->m_pNodeIp == this ? &pBranch->Yip : &pBranch->Yiq;

		mult *= pOppNode->VreVim ** pYkm;

		Pe -= mult.real();
		Qe += mult.imag();

		// diagonals 2

		dPdDelta -=  mult.imag();
		dPdV	 += -ZeroDivGuard(mult.real(), V);
		dQdDelta += -mult.real();
		dQdV     +=  ZeroDivGuard(mult.imag(), V);
			

		bool bDup = CheckAddVisited(pOppNode) >= 0;
		// dP/dDelta
		pDynaModel->SetElement(A(V_DELTA), pOppNode->A(V_DELTA), mult.imag(), bDup);
		// dP/dV
		pDynaModel->SetElement(A(V_DELTA), pOppNode->A(V_V), -ZeroDivGuard(mult.real(), pOppNode->V), bDup);
		// dQ/dDelta
		pDynaModel->SetElement(A(V_V), pOppNode->A(V_DELTA), mult.real(), bDup);
		// dQ/dV
		pDynaModel->SetElement(A(V_V), pOppNode->A(V_V), ZeroDivGuard(mult.imag(), pOppNode->V), bDup);
	}

	if (!IsStateOn() || bInMetallicSC)
	{
		dQdV = dPdDelta = 1.0;
		dPdV = dQdDelta = 0.0;
		Pe = Qe = 0;
		V = Delta = 0.0;
	}

	pDynaModel->SetElement(A(V_DELTA), A(V_DELTA), dPdDelta);
	pDynaModel->SetElement(A(V_V), A(V_DELTA), dQdDelta);
	pDynaModel->SetElement(A(V_DELTA), A(V_V), dPdV);
	pDynaModel->SetElement(A(V_V), A(V_V), dQdV);
	*/


	double Vre2 = Vre * Vre;
	double Vim2 = Vim * Vim;
	double V2 = Vre2 + Vim2;

	double V2sqInv = V < DFW2_EPSILON ? 0.0 : 1.0 / sqrt(V2);
	double VreV2 = V < DFW2_EPSILON ? 0.0 : Vre / V2;
	double VimV2 = V < DFW2_EPSILON ? 0.0 : Vim / V2;

	double Igre(0.0), Igim(0.0);
	
	if (pGenLink->m_nCount)
	{
		Pg = Qg = 0.0;
		ppGen = nullptr;
		ResetVisited();
		while (pGenLink->In(ppGen))
		{
			CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*ppGen);
			pDynaModel->SetElement(A(V_RE), pGen->A(CDynaPowerInjector::V_IRE), -1.0);
			pDynaModel->SetElement(A(V_IM), pGen->A(CDynaPowerInjector::V_IIM), -1.0);
			Igre += pGen->Ire;
			Igim += pGen->Iim;
		}
	}

	ppBranch = nullptr;
	ResetVisited();
	while (pBranchLink->In(ppBranch))
	{
		CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppBranch);
		CDynaNodeBase *pOppNode = pBranch->GetOppositeNode(this);
		cplx *pYkm = pBranch->m_pNodeIp == this ? &pBranch->Yip : &pBranch->Yiq;

		bool bDup = CheckAddVisited(pOppNode) >= 0;
		// dIre /dVre
		pDynaModel->SetElement(A(V_RE), pOppNode->A(V_RE), -pYkm->real(), bDup);
		// dIre/dVim
		pDynaModel->SetElement(A(V_RE), pOppNode->A(V_IM), pYkm->imag(), bDup);
		// dIim/dVre
		pDynaModel->SetElement(A(V_IM), pOppNode->A(V_RE), -pYkm->imag(), bDup);
		// dIim/dVim
		pDynaModel->SetElement(A(V_IM), pOppNode->A(V_IM), -pYkm->real(), bDup);
	}

	double Pgsum =  Pnr - Pg;
	double Qgsum =  Qnr - Qg;

	double VreVim2 = 2.0 * Vre * Vim;
	double V4 = V < DFW2_EPSILON ? 0.0 : 1.0 / (V2 * V2);

	double PgVre2 = Pgsum * Vre2;
	double PgVim2 = Pgsum * Vim2;
	double QgVre2 = Qgsum * Vre2;
	double QgVim2 = Qgsum * Vim2;

	double dIredVre = -Yii.real();
	double dIredVim =  Yii.imag();
	double dIimdVre = -Yii.imag();
	double dIimdVim = -Yii.real();

	if (!IsStateOn())
	{
		dIredVre = dIimdVim = 1.0;
		dIredVim = dIimdVre = 0.0;
	}
	else
	{
		dIredVre += (-PgVre2 + PgVim2 - VreVim2 * Qgsum) * V4;
		dIredVim += (QgVre2 - QgVim2 - VreVim2 * Pgsum) * V4;
		dIimdVre += (QgVre2 - QgVim2 - VreVim2 * Pgsum) * V4;
		dIimdVim += (PgVre2 - PgVim2 + VreVim2 * Qgsum) * V4;
	}
			   
	pDynaModel->SetElement(A(V_RE), A(V_RE), dIredVre);
	pDynaModel->SetElement(A(V_RE), A(V_IM), dIredVim);
	pDynaModel->SetElement(A(V_IM), A(V_RE), dIimdVre);
	pDynaModel->SetElement(A(V_IM), A(V_IM), dIimdVim);

	pDynaModel->SetElement(A(V_V), A(V_V), 1.0);
	pDynaModel->SetElement(A(V_V), A(V_RE), -Vre * V2sqInv);
	pDynaModel->SetElement(A(V_V), A(V_IM), -Vim * V2sqInv);

	pDynaModel->SetElement(A(V_DELTA), A(V_DELTA), 1.0);
	pDynaModel->SetElement(A(V_DELTA), A(V_RE), VimV2);
	pDynaModel->SetElement(A(V_DELTA), A(V_IM), -VreV2);

	pDynaModel->SetElement(A(V_RE), A(V_V), dLRCPn * VreV2 + dLRCQn * VimV2);
	pDynaModel->SetElement(A(V_IM), A(V_V), dLRCPn * VimV2 - dLRCQn * VreV2);



	return pDynaModel->Status();
}


bool CDynaNodeBase::BuildRightHand(CDynaModel *pDynaModel)
{
	GetPnrQnr();

	CLinkPtrCount *pBranchLink = GetLink(0);
	CDevice **ppBranch = nullptr;
	CLinkPtrCount *pGenLink = GetLink(1);
	CDevice **ppGen = nullptr;

	double Ire(0.0), Iim(0.0);

	if (pGenLink->m_nCount)
	{
		Pg = Qg = 0.0;
		ResetVisited();
		while (pGenLink->In(ppGen))
		{
			CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*ppGen);
			Ire -= pGen->Ire;
			Iim -= pGen->Iim;
		}
	}

	/*
	bool bInMetallicSC = m_bInMetallicSC || V < DFW2_EPSILON;

	double Pe = Pnr - Pg - V * V * Yii.real();
	double Qe = Qnr - Qg + V * V * Yii.imag();

	double dPdDelta = 0.0;

	ResetVisited();
	while (pBranchLink->In(ppBranch))
	{
		CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppBranch);
		CDynaNodeBase *pOppNode = pBranch->GetOppositeNode(this);
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
	*/

	double V2 = Vre * Vre + Vim * Vim;
	double V2inv = V < DFW2_EPSILON ? 0.0 : 1.0 / V2;
	
	if (IsStateOn())
	{
		Ire -= Yii.real() * Vre - Yii.imag() * Vim;
		Iim -= Yii.imag() * Vre + Yii.real() * Vim;

		double Pk = Pnr - Pg;
		double Qk = Qnr - Qg;

		ppBranch = nullptr;

		ResetVisited();
		while (pBranchLink->In(ppBranch))
		{
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppBranch);
			CDynaNodeBase *pOppNode = pBranch->GetOppositeNode(this);
			cplx *pYkm = pBranch->m_pNodeIp == this ? &pBranch->Yip : &pBranch->Yiq;
			Ire -= pYkm->real() * pOppNode->Vre - pYkm->imag() * pOppNode->Vim;
			Iim -= pYkm->imag() * pOppNode->Vre + pYkm->real() * pOppNode->Vim;
		}

		Ire += (Pk * Vre + Qk * Vim) * V2inv;
		Iim += (Pk * Vim - Qk * Vre) * V2inv;
	}


	pDynaModel->SetFunction(A(V_RE), Ire);
	pDynaModel->SetFunction(A(V_IM), Iim);

	double dV = V - sqrt(V2);
	double angle = atan2(Vim, Vre);
	//double newDelta = fmod(angle - Delta + M_PI, 2 * M_PI) - M_PI + Delta;
	//double newDelta = angle + static_cast<double>(static_cast<int>(Delta / (2 * M_PI)) * 2 * M_PI);
	ptrdiff_t IntWinds = static_cast<ptrdiff_t>(Delta / 2 / M_PI);
	double newDelta = static_cast<double>(IntWinds) * 2 * M_PI + angle;
	double CheckDelta[3] = { newDelta, newDelta + 2 * M_PI, newDelta - 2 * M_PI };
	double *pMin = nullptr;
	for (double *pCheck = CheckDelta; pCheck < CheckDelta + _countof(CheckDelta); pCheck++)
		if (pMin)
		{
			if (fabs(*pMin - Delta) > fabs(*pCheck - Delta))
				pMin = pCheck;
		}
		else
			pMin = pCheck;

	newDelta = *pMin;
	/*
	RightVector *pRv = pDynaModel->GetRightVector(A(V_DELTA));
	if (pRv->Nordsiek[1] > 0.0)
	{
		_ASSERTE(newDelta > pRv->SavedNordsiek[0] - pRv->Atol);
	}
	else if (pRv->Nordsiek[1] < 0.0)
	{
		_ASSERTE(newDelta < pRv->SavedNordsiek[0] + pRv->Atol);
	}
	*/

	double dDelta = Delta - newDelta;

	pDynaModel->SetFunction(A(V_V), dV);
	pDynaModel->SetFunction(A(V_DELTA), dDelta);

	return pDynaModel->Status();
}

bool CDynaNodeBase::NewtonUpdateEquation(CDynaModel* pDynaModel)
{
	bool bRes = true;
	// only update vicinity in case node has LRC ( due to slow complex::abs() )
	if (m_pLRC)
		dLRCVicinity = 5.0 * fabs(Vold - V) / Unom;

	//dLRCVicinity = 0.0;
	Vold = V;

	return bRes;
}

eDEVICEFUNCTIONSTATUS CDynaNodeBase::Init(CDynaModel* pDynaModel)
{
	UpdateVreVim();
	V0 = (V > 0) ? V : Unom;
	return DFS_OK;
}

// в узлах используется расширенная функция
// прогноза. После прогноза рассчитывается
// напряжение в декартовых координатах
// и сбрасываются значения по умолчанию перед
// итерационным процессом решения сети
void CDynaNodeBase::Predict()
{
	dLRCVicinity = 0.0;
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
	double w0 = pDynaModel->GetOmega0();

	pDynaModel->SetElement(A(V_LAG), A(V_DELTA), -1.0 / T);
	//pDynaModel->SetElement(A(V_LAG), A(V_LAG), 1.0 + hb0 / T);
	pDynaModel->SetElement(A(V_LAG), A(V_LAG), -1.0 / T);
	
	
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

// переменные базового узла - модуль и угол
double* CDynaNodeBase::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = NULL;
	switch (nVarIndex)
	{
		MAP_VARIABLE(Delta,V_DELTA)
		MAP_VARIABLE(V, V_V)
		MAP_VARIABLE(Vre, V_RE)
		MAP_VARIABLE(Vim, V_IM)
	}
	return p;
}

// константы узла - проводимость шунта
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

// переменные узла - переменные базового узла + скольжение и лаг скольжения
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
									   m_pSynchroZones(NULL),
									   m_bDynamicLRC(true)
{
	// в контейнере требуем особой функции прогноза и обновления после
	// ньютоновской итерации
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
		Vre = Vim = 0.0;
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
		Vre = Vim = 0.0;
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
	// оценка количества ненулевых элементов
	size_t nNzCount = nNodeCount + 2 * nBranchesCount;

	// структура соответствия узла диагональному элементу матрицы
	struct NodeToMatrix
	{
		CDynaNodeBase *pNode;
		double *pMatrixElement;
	};

	// вектор смежных узлов 
	NodeToMatrix *pNodeToMatrix = new NodeToMatrix[nBranchesCount];
	
	// матрица и вектор правой части
	double *Ax = new double[2 * nNzCount];
	double *B  = new double[2 * nNodeCount];
	// индексы
	ptrdiff_t *Ap = new ptrdiff_t[nNodeCount + 1];
	ptrdiff_t *Ai = new ptrdiff_t[nNzCount];
	KLU_symbolic *Symbolic = NULL;
	KLU_numeric *Numeric = NULL;
	KLU_common Common;
	KLU_defaults(&Common);

	// вектор указателей на диагональ матрицы
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
		// стартуем с плоского
		pNode->Vre = pNode->V = pNode->Unom;
		pNode->Vim = pNode->Delta = 0.0;
		pNode->dLRCVicinity = 0.0;		// зона сглаживания СХН для начала нулевая - без сглаживания


		*pAp = (pAx - Ax) / 2;    pAp++;
		*pAi = pNode->A(0) / EquationsCount();		  pAi++;
		// первый элемент строки используем под диагональ
		// и запоминаем указатель на него
		*ppDiags = pAx;
		// если узел отключен, в матрице он все равно будет но с единичным диагональным элементом
		*pAx = 1.0; pAx++;
		*pAx = 0.0; pAx++;
		ppDiags++;

		if (pNode->IsStateOn() && !pNode->m_bInMetallicSC)
		{
			// для всех узлов, которые не отключены и не находятся в металлическом КЗ (КЗ с нулевым шунтом)
			_ftprintf(fnode, _T("%td;"), pNode->GetId());
			// Branches
			CDevice **ppBranch = NULL;
			CLinkPtrCount *pLink = pNode->GetLink(0);
			pNode->ResetVisited();
			NodeToMatrix *pNodeToMatrixEnd = pNodeToMatrix;

			// обходим ветви узла
			while (pLink->In(ppBranch))
			{
				CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppBranch);
				if (pBranch->m_BranchState == CDynaBranch::BRANCH_ON)
				{
					// находим взаимные проводимости узлов для включенных ветвей 
					CDynaNodeBase *pOppNode = pBranch->m_pNodeIp;
					cplx *pYkm = &pBranch->Yiq;
					if (pBranch->m_pNodeIp == pNode)
					{
						pOppNode = pBranch->m_pNodeIq;
						pYkm = &pBranch->Yip;
					}

					// проверяем, не добавлен ли уже этот смежный узел в матрицу
					// если да - суммируем проводимости параллельных ветвей
					NodeToMatrix *pCheck = pNodeToMatrix;
					while (pCheck < pNodeToMatrixEnd)
					{
						if (pCheck->pNode == pOppNode)
							break;
						pCheck++;
					}

					if (pCheck < pNodeToMatrixEnd)
					{
						// нашли смежный узел, добавляем проводимости
						*pCheck->pMatrixElement += pYkm->real();
						*(pCheck->pMatrixElement+1) += pYkm->imag();
					}
					else
					{
						// не нашли смежного узла, добавляем его
						pCheck->pNode = pOppNode;
						pCheck->pMatrixElement = pAx;
						pNodeToMatrixEnd++;
						_ASSERTE(pNodeToMatrixEnd - pNodeToMatrix < static_cast<ptrdiff_t>(nBranchesCount));

						*pAx = pYkm->real();   pAx++;
						*pAx = pYkm->imag();   pAx++;
						// определяем индекс этого смежного узла в строке матрицы
						*pAi = pOppNode->A(0) / EquationsCount(); pAi++;
					}
				}
			}
		}
	}

	nNzCount = (pAx - Ax) / 2;		// рассчитываем получившееся количество ненулевых элементов (делим на 2 потому что комплекс)
	Ap[nNodeCount] = nNzCount;
	Symbolic = KLU_analyze(nNodeCount, Ap, Ai, &Common);
		
	for (ptrdiff_t nIteration = 0; nIteration < 200; nIteration++)
	{
		m_IterationControl.Reset();

		ppDiags = pDiags;
		pB = B;

		_ftprintf(fnode, _T("\n%td;"), nIteration);
		_ftprintf(fgen, _T("\n%td;"), nIteration);

		// проходим по узлам вне зависимости от их состояния, параллельно идем по диагонали матрицы
		for (DEVICEVECTORITR it = m_DevVec.begin(); it != m_DevVec.end(); it++, ppDiags++)
		{
			CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);

			if (!pNode->m_bInMetallicSC && pNode->IsStateOn())
			{
				// для всех узлов которые включены и вне металлического КЗ

				cplx Unode(pNode->Vre, pNode->Vim);

				double Pnr = pNode->Pn;
				double Qnr = pNode->Qn;
				cplx I = 0.0;					// обнуляем узел
				cplx Y = pNode->Yii;			// диагональ матрицы по умолчанию собственная проводимость узла

				pNode->Vold = pNode->V;			// запоминаем напряжение до итерации для анализа сходимости

				if (pNode->m_pLRC)
				{
					// если есть СХН, считаем мощность узла с учетом СХН
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
				// проходим по генераторам
				while (pLink->In(ppDeivce))
				{
					CDynaVoltageSource *pVsource = static_cast<CDynaVoltageSource*>(*ppDeivce);
					// если в узле есть хотя бы один генератор, то обнуляем мощность генерации узла
					// если в узле нет генераторов но есть мощность генерации - то она будет учитываться
					// задающим током
					pNode->Pg = pNode->Qg = 0.0;
					// если генераторм выключен то просто пропускаем, его мощность и ток будут равны нулю
					if (!pVsource->IsStateOn())
						continue;

					pVsource->CalculatePower();
					
					if (0)
					{
						//Альтернативный вариант с расчетом подключения к сети через мощность. Что-то нестабильный
						cplx Sg(pVsource->P, pVsource->Q);
						cplx E = pVsource->GetEMF();
						cplx Yg = conj(Sg / Unode) / (E - Unode);
						I -= E * Yg;
						Y -= Yg;
					}
					else
					{
						CDynaGeneratorInfBusBase *pGen = static_cast<CDynaGeneratorInfBusBase*>(pVsource);
						// в диагональ матрицы добавляем проводимость генератора
						Y -= 1.0 / cplx(0.0, pGen->Xgen());
						I -= pGen->Igen(nIteration);
					}
					

					_CheckNumber(I.real());
					_CheckNumber(I.imag());

					_ftprintf(fgen, _T("%g;"), pVsource->P);
				}

				// рассчитываем задающий ток узла от нагрузки
				// можно посчитать ток, а можно посчитать добавку в диагональ
				//I += conj(cplx(Pnr - pNode->Pg, Qnr - pNode->Qg) / pNode->VreVim);
				if(pNode->V > 0.0)
					Y += conj(cplx(pNode->Pg - Pnr, pNode->Qg - Qnr) / pNode->V / pNode->V);
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
				*(*ppDiags+1) = Y.imag();
				_ftprintf(fnode, _T("%g;"), pNode->V / pNode->V0);
			}
			else
			{
				// если узел не включен, задающий ток для него равен нулю
				*pB = 0.0; pB++;
				*pB = 0.0; pB++;
			}
		}
		
ptrdiff_t refactorOK = 1;

// KLU может делать повторную факторизацию матрицы с начальным пивотингом
// это быстро, но при изменении пивотов может вызывать численную неустойчивость.
// У нас есть два варианта факторизации/рефакторизации на итерации LULF

#ifdef USEREFACTOR
		// делаем факторизацию/рефакторизацию и если она получилась
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
			// решаем систему
			klu_z_tsolve(Symbolic, Numeric, nNodeCount, 1, B, 0, &Common);
			double *pB = B;

			for (DEVICEVECTORITR it = m_DevVec.begin(); it != m_DevVec.end(); it++)
			{
				CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);

				// напряжение после решения системы в векторе задающий токов
				pNode->Vre = *pB;		pB++;
				pNode->Vim = *pB;		pB++;

				// считаем напряжение узла в полярных координатах
				cplx Unode(pNode->Vre, pNode->Vim);
				pNode->V = abs(Unode);
				pNode->Delta = arg(Unode);

				// рассчитываем зону сглаживания СХН (также как для Ньютона)
				/*if (pNode->m_pLRC)
					pNode->dLRCVicinity = 30.0 * fabs(pNode->Vold - pNode->V) / pNode->Unom;
				*/
				
				// считаем изменение напряжения узла между итерациями и находим
				// самый изменяющийся узел

				m_IterationControl.m_MaxV.UpdateMaxAbs(pNode, pNode->V / pNode->Vold - 1.0);
			}

			DumpIterationControl();
			
			if (fabs(m_IterationControl.m_MaxV.GetDiff()) < m_pDynaModel->GetRtolLULF())
			{
				Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(CDFW2Messages::m_cszLULFConverged, m_IterationControl.m_MaxV.GetDiff(), nIteration));
				break;
			}
		}
	}

	/*
	for (DEVICEVECTORITR it = m_DevVec.begin(); it != m_DevVec.end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);

		CDevice **ppDeivce = NULL;
		CLinkPtrCount *pLink = NULL;
		ppDeivce = NULL;
		pLink = pNode->GetLink(1);
		pNode->ResetVisited();
		// проходим по генераторам
		while (pLink->In(ppDeivce))
			static_cast<CDynaVoltageSource*>(*ppDeivce)->CalculatePower();
	}
	*/


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

void CDynaNodeContainer::SwitchLRCs(bool bSwitchToDynamicLRC)
{
	if (bSwitchToDynamicLRC != m_bDynamicLRC)
	{
		m_bDynamicLRC = bSwitchToDynamicLRC;
		for (DEVICEVECTORITR it = m_DevVec.begin(); it != m_DevVec.end(); it++)
		{
			CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
			std::swap(pNode->m_pLRCLF, pNode->m_pLRC);
			std::swap(pNode->Pn, pNode->Pnr);
			std::swap(pNode->Qn, pNode->Qnr);
			if (!m_bDynamicLRC)
				pNode->V0 = pNode->Unom;
			else
				pNode->V0 = pNode->V;
		}
	}
}

bool CDynaNodeContainer::Seidell()
{
	return LULF();
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

	// к узлу могут быть прилинкованы ветви много к одному
	props.AddLinkFrom(DEVTYPE_BRANCH, DLM_MULTI, DPD_SLAVE);
	// и инжекторы много к одному. Для них узел выступает ведущим
	props.AddLinkFrom(DEVTYPE_POWER_INJECTOR, DLM_MULTI, DPD_SLAVE);

	props.nEquationsCount = CDynaNodeBase::VARS::V_LAST;
	props.bPredict = props.bNewtonUpdate = true;

	props.m_VarMap.insert(make_pair(CDynaNodeBase::m_cszDelta, CVarIndex(V_DELTA,VARUNIT_RADIANS)));
	props.m_VarMap.insert(make_pair(CDynaNodeBase::m_cszV, CVarIndex(V_V, VARUNIT_KVOLTS)));
	props.m_VarMap.insert(make_pair(CDynaNodeBase::m_cszVre, CVarIndex(V_RE, VARUNIT_KVOLTS)));
	props.m_VarMap.insert(make_pair(CDynaNodeBase::m_cszVim, CVarIndex(V_IM, VARUNIT_KVOLTS)));
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
const _TCHAR *CDynaNodeBase::m_cszVre = _T("Vre");
const _TCHAR *CDynaNodeBase::m_cszVim = _T("Vim");
const _TCHAR *CDynaNodeBase::m_cszGsh = _T("gsh");
const _TCHAR *CDynaNodeBase::m_cszBsh = _T("bsh");

const _TCHAR *CDynaNode::m_cszS = _T("S");
const _TCHAR *CDynaNode::m_cszSz = _T("Sz");
