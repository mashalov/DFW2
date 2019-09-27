﻿#include "stdafx.h"
#include "DynaBranch.h"
#include "DynaModel.h"

using namespace DFW2;

#define LOW_VOLTAGE 0.01
#define LOW_VOLTAGE_HYST LOW_VOLTAGE * 0.1

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
								 m_pLRCGen(nullptr),
								 m_pSyncZone(nullptr),
								 dLRCVicinity(0.0),
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
	Pnr = Pn;	Qnr = Qn;
	// генерация в узле также равна заданной в УР
	Pgr = Pg;	Qgr = Qg;
	double VdVnom = V / V0;

	dLRCPg = dLRCQg = dLRCPn = dLRCQn = 0.0;

	// если есть СХН нагрузки, рассчитываем
	// комплексную мощность и производные по напряжению

	if (m_pLRC)
	{
		Pnr *= m_pLRC->GetPdP(VdVnom, dLRCPn, dLRCVicinity);
		Qnr *= m_pLRC->GetQdQ(VdVnom, dLRCQn, dLRCVicinity);
		dLRCPn *= Pn / V0;
		dLRCQn *= Qn / V0;
	}

	// если есть СХН генерации (нет привязанных генераторов, но есть заданная в УР генерация)
	// рассчитываем расчетную генерацию
	if (m_pLRCGen)
	{
		Pgr *= m_pLRCGen->GetPdP(VdVnom, dLRCPg, dLRCVicinity); 
		Qgr *= m_pLRCGen->GetQdQ(VdVnom, dLRCQg, dLRCVicinity);
		dLRCPg *= Pg / V0;
		dLRCQg *= Qg / V0;
	}
}

bool CDynaNodeBase::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;

	if(GetId() == 2021 && pDynaModel->GetIntegrationStepNumber() == 2123)
		bRes = true;

	double Vre2 = Vre * Vre;
	double Vim2 = Vim * Vim;
	double V2 = Vre2 + Vim2;
	double V2sq = sqrt(V2);

	GetPnrQnr();

	double dIredVre = -Yii.real();
	double dIredVim = Yii.imag();
	double dIimdVre = -Yii.imag();
	double dIimdVim = -Yii.real();


	if (IsStateOn() && V2sq < pDynaModel->GetLRCToShuntVmin() * 0.5 * Unom)
	{
		double V02 = V0 * V0;

		if (m_pLRC)
		{
			double g = -Pn * m_pLRC->P->a2 / V02;
			double b =  Qn * m_pLRC->Q->a2 / V02;
			dIredVre += -g;
			dIredVim +=  b;
			dIimdVre += -b;
			dIimdVim += -g;
			dLRCPn = dLRCQn = Pnr = Qnr = 0.0;
		}

		if (m_pLRCGen)
		{
			double g =  Pg * m_pLRCGen->P->a2 / V02;
			double b = -Qg * m_pLRCGen->Q->a2 / V02;
			dIredVre += -g;
			dIredVim +=  b;
			dIimdVre += -b;
			dIimdVim += -g;
			dLRCPg = dLRCQg = Pgr = Qgr = 0.0;
		}
	}

	CLinkPtrCount *pBranchLink = GetSuperLink(1);
	CLinkPtrCount *pGenLink    = GetSuperLink(2);
	CDevice **ppBranch(nullptr), **ppGen(nullptr);

	bool bInMetallicSC = m_bInMetallicSC || (V < DFW2_EPSILON && IsStateOn());

	
	double VreV2 = Vre / V2;
	double VimV2 = Vim / V2;

	

	

	double Pgsum = Pnr - Pgr;
	double Qgsum = Qnr - Qgr;

	double V4 = V2 * V2;
	double VreVim2 = 2.0 * Vre * Vim;
	double PgVre2 = Pgsum * Vre2;
	double PgVim2 = Pgsum * Vim2;
	double QgVre2 = Qgsum * Vre2;
	double QgVim2 = Qgsum * Vim2;

	pDynaModel->SetElement(A(V_V), A(V_V), 1.0);
	pDynaModel->SetElement(A(V_DELTA), A(V_DELTA), 1.0);

	if (!IsStateOn())
	{
		dIredVre = dIimdVim = 1000.0;
		dIredVim = dIimdVre = 0.0;

		ppGen = nullptr;
		ResetVisited();
		while (pGenLink->In(ppGen))
		{
			CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*ppGen);
			pDynaModel->SetElement(A(V_RE), pGen->A(CDynaPowerInjector::V_IRE), 0.0);
			pDynaModel->SetElement(A(V_IM), pGen->A(CDynaPowerInjector::V_IIM), 0.0);
		}

		ppBranch = nullptr;
		ResetVisited();
		while (pBranchLink->In(ppBranch))
		{
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppBranch);
			CDynaNodeBase *pOppNode = pBranch->GetOppositeSuperNode(this);
			bool bDup = CheckAddVisited(pOppNode) >= 0;
			// dIre /dVre
			pDynaModel->SetElement(A(V_RE), pOppNode->A(V_RE), 0.0, bDup);
			// dIre/dVim
			pDynaModel->SetElement(A(V_RE), pOppNode->A(V_IM), 0.0, bDup);
			// dIim/dVre
			pDynaModel->SetElement(A(V_IM), pOppNode->A(V_RE), 0.0, bDup);
			// dIim/dVim
			pDynaModel->SetElement(A(V_IM), pOppNode->A(V_IM), 0.0, bDup);
		}
	}
	else
	{
		if (pDynaModel->FillConstantElements())
		{
			ppGen = nullptr;
			ResetVisited();
			while (pGenLink->In(ppGen))
			{
				CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*ppGen);
				pDynaModel->SetElement(A(V_RE), pGen->A(CDynaPowerInjector::V_IRE), -1.0);
				pDynaModel->SetElement(A(V_IM), pGen->A(CDynaPowerInjector::V_IIM), -1.0);
			}

			ppBranch = nullptr;
			ResetVisited();
			while (pBranchLink->In(ppBranch))
			{
				CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppBranch);
				CDynaNodeBase *pOppNode = pBranch->GetOppositeSuperNode(this);
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

			pDynaModel->CountConstElementsToSkip(A(V_RE));
			pDynaModel->CountConstElementsToSkip(A(V_IM));
		}
		else
		{
			pDynaModel->SkipConstElements(A(V_RE));
			pDynaModel->SkipConstElements(A(V_IM));
		}

	


		// check low voltage
		dIredVre -= (PgVre2 - PgVim2 + VreVim2 * Qgsum) / V4;
		dIredVim += (QgVre2 - QgVim2 - VreVim2 * Pgsum) / V4;
		dIimdVre += (QgVre2 - QgVim2 - VreVim2 * Pgsum) / V4;
		dIimdVim += (PgVre2 - PgVim2 + VreVim2 * Qgsum) / V4;

		// check low voltage
		if (m_bLowVoltage)
		{
			pDynaModel->SetElement(A(V_V), A(V_RE), 0.0);
			pDynaModel->SetElement(A(V_V), A(V_IM), 0.0);
		}
		else
		{
			pDynaModel->SetElement(A(V_V), A(V_RE), -Vre / V2sq);
			pDynaModel->SetElement(A(V_V), A(V_IM), -Vim / V2sq);
		}

		if (m_bLowVoltage)
		{
			pDynaModel->SetElement(A(V_DELTA), A(V_RE), 0.0);
			pDynaModel->SetElement(A(V_DELTA), A(V_IM), 0.0);
		}
		else
		{
			pDynaModel->SetElement(A(V_DELTA), A(V_RE), VimV2);
			pDynaModel->SetElement(A(V_DELTA), A(V_IM), -VreV2);
		}

		if (m_bLowVoltage)
		{
			pDynaModel->SetElement(A(V_RE), A(V_V), 0.0);
			pDynaModel->SetElement(A(V_IM), A(V_V), 0.0);
		}
		else
		{
			V2 = V * V;
			VreV2 = Vre / V2;	VimV2 = Vim / V2;

			dLRCPn -= dLRCPg;		dLRCQn -= dLRCQg;
			pDynaModel->SetElement(A(V_RE), A(V_V), dLRCPn * VreV2 + dLRCQn * VimV2);
			pDynaModel->SetElement(A(V_IM), A(V_V), dLRCPn * VimV2 - dLRCQn * VreV2);
		}
	}

	pDynaModel->SetElement(A(V_RE), A(V_RE), dIredVre);
	pDynaModel->SetElement(A(V_RE), A(V_IM), dIredVim);
	pDynaModel->SetElement(A(V_IM), A(V_RE), dIimdVre);
	pDynaModel->SetElement(A(V_IM), A(V_IM), dIimdVim);

	return pDynaModel->Status();
}


bool CDynaNodeBase::BuildRightHand(CDynaModel *pDynaModel)
{
	GetPnrQnr();

	double Ire(0.0), Iim(0.0);
	double V2 = Vre * Vre + Vim * Vim;
	double V2sq = sqrt(V2);
	
	if (IsStateOn())
	{
		if (V2sq < pDynaModel->GetLRCToShuntVmin() * Unom * 0.5)
		{
			double V02 = V0 * V0;

			if (m_pLRC)
			{
				double g = -Pn * m_pLRC->P->a2 / V02;
				double b = Qn * m_pLRC->Q->a2 / V02;
				Ire -= g * Vre - b * Vim;
				Iim -= b * Vre + g * Vim;
				Pnr = Qnr = 0.0;
			}

			if (m_pLRCGen)
			{
				double g = Pg * m_pLRCGen->P->a2 / V02;
				double b = -Qg * m_pLRCGen->Q->a2 / V02;
				Ire -= g * Vre - b * Vim;
				Iim -= b * Vre + g * Vim;
				Pgr = Qgr = 0.0;
			}
		}

		CLinkPtrCount *pBranchLink = GetSuperLink(1);
		CLinkPtrCount *pGenLink	   = GetSuperLink(2);
		CDevice **ppBranch(nullptr), **ppGen(nullptr);
		ResetVisited();
		while (pGenLink->In(ppGen))
		{
			CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*ppGen);
			Ire -= pGen->Ire;
			Iim -= pGen->Iim;
		}

		Ire -= Yii.real() * Vre - Yii.imag() * Vim;
		Iim -= Yii.imag() * Vre + Yii.real() * Vim;

		double Pk = Pnr - Pgr;
		double Qk = Qnr - Qgr;

		ResetVisited();
		while (pBranchLink->In(ppBranch))
		{
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppBranch);
			CDynaNodeBase *pOppNode = pBranch->GetOppositeSuperNode(this);
			cplx *pYkm = pBranch->m_pNodeIp == this ? &pBranch->Yip : &pBranch->Yiq;
			Ire -= pYkm->real() * pOppNode->Vre - pYkm->imag() * pOppNode->Vim;
			Iim -= pYkm->imag() * pOppNode->Vre + pYkm->real() * pOppNode->Vim;
		}

		// check low voltage
		Ire += (Pk * Vre + Qk * Vim) / V2;
		Iim += (Pk * Vim - Qk * Vre) / V2;
	}


	pDynaModel->SetFunction(A(V_RE), Ire);
	pDynaModel->SetFunction(A(V_IM), Iim);

	double dV = V - V2sq;
	double dDelta = Delta - atan2(Vim, Vre);

	if (m_bLowVoltage)
		dV = dDelta = 0.0;

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
	Vold = V;
	return bRes;
}

void CDynaNodeBase::InitLF()
{
	UpdateVreVim();
	V0 = (V > 0) ? V : Unom;
}

eDEVICEFUNCTIONSTATUS CDynaNodeBase::Init(CDynaModel* pDynaModel)
{
	UpdateVreVim();
	m_bLowVoltage = V < (LOW_VOLTAGE - LOW_VOLTAGE_HYST);
	V0 = (V > 0) ? V : Unom;

	if (GetLink(1)->m_nCount > 0)					// если к узлу подключены генераторы, то СХН генераторов не нужна и мощности генерации 0
		Pg = Qg = Pgr = Qgr = 0.0;
	else if (Equal(Pg, 0.0) && Equal(Qg, 0.0))		// если генераторы не подключены, и мощность генерации равна нулю - СХН генераторов не нужна
		Pgr = Qgr = 0.0;
	else
		m_pLRCGen = pDynaModel->GetLRCGen();		// если есть генерация но нет генераторов - нужна СХН генераторов

	return DFS_OK;
}

// в узлах используется расширенная функция
// прогноза. После прогноза рассчитывается
// напряжение в декартовых координатах
// и сбрасываются значения по умолчанию перед
// итерационным процессом решения сети
void CDynaNode::Predict()
{
	dLRCVicinity = 0.0;
	double newDelta = atan2(sin(Delta), cos(Delta));
	if (fabs(newDelta - Delta) > DFW2_EPSILON)
	{
		RightVector *pRvDelta = GetModel()->GetRightVector(A(V_DELTA));
		RightVector *pRvLag = GetModel()->GetRightVector(A(V_LAG));
		double dDL = Delta - Lag;
		Delta = newDelta;
		Lag = Delta - dDL;
		pRvDelta->Nordsiek[0] = Delta;
		pRvLag->Nordsiek[0] = Lag;
	}

}

CDynaNode::CDynaNode() : CDynaNodeBase()
{

}

bool CDynaNode::BuildDerivatives(CDynaModel *pDynaModel)
{
	bool bRes = true;
	double T = pDynaModel->GetFreqTimeConstant();
	pDynaModel->SetDerivative(A(V_LAG), (Delta - Lag) / T);
	/*
	if (IsStateOn())
	{
		pDynaModel->SetDerivative(A(V_SIP), 0.0);
		pDynaModel->SetDerivative(A(V_COP), 0.0);
	}
	else
	{
		pDynaModel->SetDerivative(A(V_SIP), 0.0);
		pDynaModel->SetDerivative(A(V_COP), 0.0);
	}
	*/

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

	/*
	if (IsStateOn())
	{
		double Vsq = Vre * Vre + Vim * Vim + DFW2_SQRT_EPSILON;
		double Vmod = sqrt(Vsq);
		double Divdr = T * Vsq * Vmod;

		pDynaModel->SetElement(A(V_SIP), A(V_SIP), -1.0 / T);
		pDynaModel->SetElement(A(V_SIP), A(CDynaNodeBase::V_RE), Vre * Vim / Divdr);
		pDynaModel->SetElement(A(V_SIP), A(CDynaNodeBase::V_IM), -(Vre * Vre + DFW2_SQRT_EPSILON) / Divdr);

		pDynaModel->SetElement(A(V_COP), A(V_COP), -1.0 / T);
		pDynaModel->SetElement(A(V_COP), A(CDynaNodeBase::V_RE), -(Vim * Vim + DFW2_SQRT_EPSILON) / Divdr);
		pDynaModel->SetElement(A(V_COP), A(CDynaNodeBase::V_IM), Vre * Vim / Divdr);
		pDynaModel->SetElement(A(V_SV), A(V_SV), 1.0);
		pDynaModel->SetElement(A(V_SV), A(V_COP), -(-Vim * Cop * Cop + 2 * Vre*Cop*Sip + Vim * Sip * Sip) / (T*w0* pow(Cop * Cop + Sip * Sip, 2.0) * Vmod));
		pDynaModel->SetElement(A(V_SV), A(V_SIP), (Vre*Cop * Cop + 2 * Vim*Cop*Sip - Vre * Sip * Sip) / (T*w0* pow(Cop * Cop + Sip * Sip, 2.0)* Vmod));
		pDynaModel->SetElement(A(V_SV), A(CDynaNodeBase::V_RE), (Sip*Vim*Vim + Cop * Vre*Vim + DFW2_SQRT_EPSILON * Sip) / (T*w0*(Cop * Cop + Sip * Sip) * Vmod * Vsq));
		pDynaModel->SetElement(A(V_SV), A(CDynaNodeBase::V_IM), -(Cop*Vre *Vre + Sip * Vim*Vre + Cop * DFW2_SQRT_EPSILON) / (T*w0*(Cop * Cop + Sip * Sip) * Vmod * Vsq));
	}
	else
	{
		pDynaModel->SetElement(A(V_SIP), A(V_SIP), -1.0 / T);
		pDynaModel->SetElement(A(V_COP), A(V_COP), -1.0 / T);
		pDynaModel->SetElement(A(V_SV), A(V_SV), 1.0);
	}*/
	
	
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

	/*
	if (IsStateOn())
	{
		double V2 = sqrt(Vre * Vre + Vim * Vim + DFW2_SQRT_EPSILON);
		double dSip = (Vim / V2 - Sip) / T;
		double dCop = (Vre / V2 - Cop) / T;
		pDynaModel->SetFunctionDiff(A(V_SIP), 0.0);
		pDynaModel->SetFunctionDiff(A(V_COP), 0.0);
		pDynaModel->SetFunction(A(V_SV), 0.0);

	}
	else
	{
		pDynaModel->SetFunctionDiff(A(V_SIP), 0);
		pDynaModel->SetFunctionDiff(A(V_COP), 0);
		pDynaModel->SetFunction(A(V_SV), 0);
	}
	*/

	//DumpIntegrationStep(2021, 2031);
	//DumpIntegrationStep(2143, 2031);
	//DumpIntegrationStep(2141, 2031);

	return pDynaModel->Status() && bRes;
}

eDEVICEFUNCTIONSTATUS CDynaNode::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = CDynaNodeBase::Init(pDynaModel);
	if (CDevice::IsFunctionStatusOK(Status))
	{
		S = 0.0;
		//Sv = 0.0;
		Lag = Delta;
		/*
		if (IsStateOn())
		{
			Cop = 0.0;
			Sip = 0.0;
		}
		else
			Cop = Sip = 0;
			*/
	}
	return Status;
}

// переменные базового узла - модуль и угол
double* CDynaNodeBase::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p(nullptr);
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
	double *p(nullptr);
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
				/*
			MAP_VARIABLE(Sv, V_SV)
			MAP_VARIABLE(Sip, V_SIP)
			MAP_VARIABLE(Cop, V_COP)
			*/
		}
	}
	return p;
}

CDynaNodeContainer::CDynaNodeContainer(CDynaModel *pDynaModel) : 
									   CDeviceContainer(pDynaModel),
									   m_pSynchroZones(nullptr),
									   m_bDynamicLRC(true)
{
	// в контейнере требуем особой функции прогноза и обновления после
	// ньютоновской итерации
	m_ContainerProps.bNewtonUpdate = m_ContainerProps.bPredict = true;
}

CDynaNodeContainer::~CDynaNodeContainer()
{
	ClearSuperLinks();
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
		CDevice **ppBranch(nullptr);
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
		CDevice **ppBranch(nullptr);
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
			V = Delta = Lag = S = /*Sip = Cop = Sv = */ 0.0;
			break;
		case eDEVICESTATE::DS_ON:
			V = Unom;
			Delta = 0;
			Lag = S = /*Sv = */0.0;
			Vre = V;
			Vim = 0;
			/*
			Sip = 0;
			Cop = 0;
			*/
			CalcAdmittances();
			break;
		}
	}
	return Status;
}

eDEVICEFUNCTIONSTATUS CDynaNode::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	Lag = Delta - S * pDynaModel->GetFreqTimeConstant() * pDynaModel->GetOmega0();
	SetLowVoltage(sqrt(Vre * Vre + Vim * Vim) < (LOW_VOLTAGE - LOW_VOLTAGE_HYST));
	//Delta = atan2(Vim, Vre);
	return DFS_OK;
}

CSynchroZone::CSynchroZone() : CDevice()
{
	Clear();
}

// очищает свойства зоны
void CSynchroZone::Clear()
{
	// по умолчанию зона :
	m_bEnergized = false;				// напряжение снято
	Mj = 0.0;							// суммарный момент инерции равен нулю
	S = 0.0;							// скольжение равно нулю
	m_bPassed = true;
	m_bInfPower = false;				// в зоне нет ШБМ
	m_LinkedGenerators.clear();
}

double* CSynchroZone::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p(nullptr);

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
	KLU_symbolic *Symbolic(nullptr);
	KLU_numeric *Numeric(nullptr);
	KLU_common Common;
	KLU_defaults(&Common);

	// вектор указателей на диагональ матрицы
	double **pDiags = new double*[nNodeCount];
	double **ppDiags = pDiags;
	double *pB = B;

	double *pAx = Ax;
	ptrdiff_t *pAp = Ap;
	ptrdiff_t *pAi = Ai;

	FILE *fnode(nullptr), *fgen(nullptr);
	_tfopen_s(&fnode, _T("c:\\tmp\\nodes.csv"), _T("w+, ccs=UTF-8"));
	_tfopen_s(&fgen, _T("c:\\tmp\\gens.csv"), _T("w+, ccs=UTF-8"));
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
			CDevice **ppBranch(nullptr);
			CLinkPtrCount *pLink = pNode->GetSuperLink(1);
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

				cplx I = 0.0;					// обнуляем узел
				cplx Y = pNode->Yii;			// диагональ матрицы по умолчанию собственная проводимость узла

				pNode->Vold = pNode->V;			// запоминаем напряжение до итерации для анализа сходимости

				pNode->GetPnrQnr();

				/*
				if (pNode->m_pLRC)
				{
					// если есть СХН, считаем мощность узла с учетом СХН
					double dP = 0.0, dQ = 0.0;
					double VdVnom = pNode->V / pNode->V0;
					Pnr *= pNode->m_pLRC->GetPdP(VdVnom, dP, pNode->dLRCVicinity);
					Qnr *= pNode->m_pLRC->GetQdQ(VdVnom, dQ, pNode->dLRCVicinity);
				}
				*/

				// Generators

				CDevice **ppDeivce(nullptr);
				CLinkPtrCount *pLink = pNode->GetSuperLink(2);
				pNode->ResetVisited();
				// проходим по генераторам
				while (pLink->In(ppDeivce))
				{
					CDynaVoltageSource *pVsource = static_cast<CDynaVoltageSource*>(*ppDeivce);
					// если в узле есть хотя бы один генератор, то обнуляем мощность генерации узла
					// если в узле нет генераторов но есть мощность генерации - то она будет учитываться
					// задающим током
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
						CDynaGeneratorInfBus *pGen = static_cast<CDynaGeneratorInfBus*>(pVsource);
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
					Y += conj(cplx(pNode->Pgr - pNode->Pnr, pNode->Qgr - pNode->Qnr) / pNode->V / pNode->V);
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
				if(pNode->IsStateOn())
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
		ExternalVariable ExtVar = { nullptr, -1 };

		if (m_pSyncZone)
			ExtVar = m_pSyncZone->GetExternalVariable(CDynaNode::m_cszS);
		return ExtVar;
	}
	else
		return CDevice::GetExternalVariable(cszVarName);
}


void CDynaNodeBase::StoreStates()
{
	m_bSavedLowVoltage = m_bLowVoltage;
}

void CDynaNodeBase::RestoreStates()
{
	m_bLowVoltage = m_bSavedLowVoltage;
}

void CDynaNodeBase::SetLowVoltage(bool bLowVoltage)
{
	if (m_bLowVoltage)
	{
		if (!bLowVoltage)
		{
			m_bLowVoltage = bLowVoltage;
			if (IsStateOn())
				Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(_T("Напряжение %g в узле %s выше порогового"), V, GetVerbalName()));
		}
	}
	else
	{
		if (bLowVoltage)
		{
			m_bLowVoltage = bLowVoltage;
			if (IsStateOn())
				Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(_T("Напряжение %g в узле %s ниже порогового"), V, GetVerbalName()));
		}
	}
}

double CDynaNodeBase::FindVoltageZC(CDynaModel *pDynaModel, RightVector *pRvre, RightVector *pRvim, double Hyst, bool bCheckForLow)
{
	double rH = 1.0;

	double Border = LOW_VOLTAGE + (bCheckForLow ? -Hyst : Hyst );

	const double *lm = pDynaModel->Methodl[DET_ALGEBRAIC * 2 + pDynaModel->GetOrder() - 1];
	double  h = pDynaModel->GetH();
	double Vre1 = pRvre->Nordsiek[0] + pRvre->Error * lm[0];
	double Vim1 = pRvim->Nordsiek[0] + pRvim->Error * lm[0];
	double Vcheck = sqrt(Vre1 * Vre1 + Vim1 * Vim1);
	double dVre1 = (pRvre->Nordsiek[1] + pRvre->Error * lm[1]) / h;
	double dVim1 = (pRvim->Nordsiek[1] + pRvim->Error * lm[1]) / h;
	double dVre2 = (pRvre->Nordsiek[2] + pRvre->Error * lm[2]) / h / h;
	double dVim2 = (pRvim->Nordsiek[2] + pRvim->Error * lm[2]) / h / h;
	double derr = fabs(pRvre->GetWeightedError(fabs(Vcheck - Border), Border));

	if (derr < pDynaModel->GetZeroCrossingTolerance())
	{
		SetLowVoltage(bCheckForLow);
		pDynaModel->DiscontinuityRequest();
	}
	else
	{
		if (pDynaModel->GetOrder() == 1)
		{
			double a = dVre1 * dVre1 + dVim1 * dVim1;
			double b = 2.0 * (Vre1 * dVre1 + Vim1 * dVim1);
			double c = Vre1 * Vre1 + Vim1 * Vim1 - Border * Border;
			rH = CDynaPrimitive::GetZCStepRatio(pDynaModel, a, b, c);
			if (pDynaModel->ZeroCrossingStepReached(rH))
			{
				SetLowVoltage(bCheckForLow);
				pDynaModel->DiscontinuityRequest();
			}
		}
		else
		{
			double a = dVre2 * dVre2 + dVim2 * dVim2;														// (xs2^2 + ys2^2)*t^4 
			double b = 2.0 * (dVre1 * dVre2 + dVim1 * dVim1);												// (2*xs1*xs2 + 2*ys1*ys2)*t^3 
			double c = (dVre1 * dVre1 + dVim1 * dVim1 + 2.0 * Vre1 * dVre2 + 2.0 * Vim1 * dVim2);			// (xs1^2 + ys1^2 + 2*x1*xs2 + 2*y1*ys2)*t^2 
			double d = (2.0 * Vre1 * dVre1 + 2.0 * Vim1 * dVim1);											// (2*x1*xs1 + 2*y1*ys1)*t 
			double e = Vre1 * Vre1 + Vim1 * Vim1 - Border * Border;											// x1^2 + y1^2 - e
			double t = -0.5 * h;

			for (int i = 0; i < 5; i++)
			{
				double dt = (a*t*t*t*t + b * t*t*t + c * t*t + d * t + e) / (4.0*a*t*t*t + 3.0*b*t*t + 2.0*c*t + d);
				t = t - dt;
				if (t > 0.0 || t < -h)
				{
					t = FLT_MAX;
					break;
				}
				else if (fabs(dt) < DFW2_EPSILON * 10.0)
					break;
			}
			rH = (h + t) / h;
			if (pDynaModel->ZeroCrossingStepReached(rH))
			{
				SetLowVoltage(bCheckForLow);
				pDynaModel->DiscontinuityRequest();
			}
		}
	}

	return rH;
}

double CDynaNodeBase::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	if (!IsStateOn())
		return rH;

	double Hyst = LOW_VOLTAGE_HYST;
	RightVector *pRvre = pDynaModel->GetRightVector(A(V_RE));
	RightVector *pRvim = pDynaModel->GetRightVector(A(V_IM));
	const double *lm = pDynaModel->Methodl[DET_ALGEBRAIC * 2 + pDynaModel->GetOrder() - 1];
	double Vre1 = pRvre->Nordsiek[0] + pRvre->Error * lm[0];
	double Vim1 = pRvim->Nordsiek[0] + pRvim->Error * lm[0];
	double Vcheck = sqrt(Vre1 * Vre1 + Vim1 * Vim1);

	if (m_bLowVoltage)
	{
		double Border = LOW_VOLTAGE + Hyst;
		if (Vcheck > Border)
		{
			rH = FindVoltageZC(pDynaModel, pRvre, pRvim, Hyst, false);
		}
	}
	else
	{
		double Border = LOW_VOLTAGE - Hyst;
		if (Vcheck < Border)
		{
			rH = FindVoltageZC(pDynaModel, pRvre, pRvim, Hyst, true);
		}
	}

	return rH;
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

	/*
	props.m_VarMap.insert(make_pair(_T("Sip"), CVarIndex(V_SIP, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(_T("Cop"), CVarIndex(V_COP, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(_T("Sv"), CVarIndex(V_SV, VARUNIT_PU)));
	*/


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
