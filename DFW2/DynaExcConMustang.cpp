#include "stdafx.h"
#include "DynaExcConMustang.h"
#include "DynaModel.h"

using namespace DFW2;

CDynaExcConMustang::CDynaExcConMustang() : CDevice(),
										   Lag(this, &Uf, V_UF, &LagIn),
										   LagIn(V_USUM,Usum),
										   dVdt(this, dVdtOutValue, V_DVDT, &dVdtIn),
										   dEqdt(this, dEqdtOutValue, V_EQDT, &dEqdtIn),
										   dSdt(this, dSdtOutValue, V_SDT, &dSdtIn)
{
}

double* CDynaExcConMustang::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p(nullptr);
	switch (nVarIndex)
	{
		MAP_VARIABLE(Uf, V_UF)
		MAP_VARIABLE(Usum, V_USUM)
		MAP_VARIABLE(Svt, V_SVT)
		MAP_VARIABLE(dVdtOutValue[0], V_DVDT)
		MAP_VARIABLE(dEqdtOutValue[0], V_EQDT)
		MAP_VARIABLE(dSdtOutValue[0], V_SDT)
		MAP_VARIABLE(dVdtOutValue[1], V_DVDT + 1)
		MAP_VARIABLE(dEqdtOutValue[1], V_EQDT + 1)
		MAP_VARIABLE(dSdtOutValue[1], V_SDT + 1)
	}
	return p;
}

eDEVICEFUNCTIONSTATUS CDynaExcConMustang::Init(CDynaModel* pDynaModel)
{
	if (Tf <= 0)
		Tf = 0.1;

	eDEVICEFUNCTIONSTATUS Status = DFS_OK;

	*dVdtOutValue = *dEqdtOutValue = *dSdtOutValue = 0.0;
	Svt = Uf = Usum = 0.0;
	double Eqnom, Unom, Eqe0;

	CDevice *pExciter = GetSingleLink(DEVTYPE_EXCITER);

	if (!InitConstantVariable(Unom, pExciter, CDynaGeneratorMotion::m_cszUnom))
		Status = DFS_FAILED;
	if (!InitConstantVariable(Eqnom, pExciter, CDynaGenerator1C::m_cszEqnom))
		Status = DFS_FAILED;
	if (!InitConstantVariable(Eqe0, pExciter, CDynaGenerator1C::m_cszEqe, DEVTYPE_GEN_1C))
		Status = DFS_FAILED;

	if (CDevice::IsFunctionStatusOK(Status))
	{
		Lag.SetMinMaxTK(pDynaModel, Umin * Eqnom - Eqe0, Umax * Eqnom - Eqe0, Tr, 1.0);
		K0u *= Eqnom / Unom;
		K1u *= Eqnom / Unom * 0.72;
		K0f *= Eqnom * pDynaModel->GetOmega0() * 1.3 / 2.0 / M_PI;
		K1f *= Eqnom * pDynaModel->GetOmega0() * 0.5 / 2.0 / M_PI;
		K1if *= 0.2;


		dVdt.SetTK(pDynaModel->GetMustangDerivativeTimeConstant(), K1u);
		dEqdt.SetTK(pDynaModel->GetMustangDerivativeTimeConstant(), K1if);
		dSdt.SetTK(pDynaModel->GetMustangDerivativeTimeConstant(), K1f);

		dVdt.Init(pDynaModel);
		dEqdt.Init(pDynaModel);
		dSdt.Init(pDynaModel);

		switch (GetState())
		{
		case DS_ON:
		{
			bool bRes = true;
			Vref = dVdtIn.Value();
			bRes = Lag.Init(pDynaModel);
			Status = bRes ? DFS_OK : DFS_FAILED;
		}
		break;
		case DS_OFF:
			Status = DFS_OK;
			Vref = Usum = 0.0;
			Lag.Init(pDynaModel);
			break;
		}
	}
	return Status;
}


bool CDynaExcConMustang::BuildEquations(CDynaModel* pDynaModel)
{
	bool bRes = true;

	double V = dVdtIn.Value(); //pNode->V;

	// dUsum / dUsum
	pDynaModel->SetElement(A(V_USUM), A(V_USUM), 1.0);

	if (IsStateOn())
	{
		// dUsum / dV
		pDynaModel->SetElement(A(V_USUM), A(dVdtIn.Index()), K0u);
		// dUsum / Sv
		pDynaModel->SetElement(A(V_USUM), A(dSdtIn.Index()), -K0u * Alpha * Vref - K0f);
		// dUsum / Svt
		pDynaModel->SetElement(A(V_USUM), A(V_SVT), K0f);
		// dUsum / dVdt
		pDynaModel->SetElement(A(V_USUM), A(V_DVDT), 1.0);
		// dUsum / dEqdt
		pDynaModel->SetElement(A(V_USUM), A(V_EQDT), 1.0);
		// dUsum / dSdt
		pDynaModel->SetElement(A(V_USUM), A(V_SDT), -1.0);
		//dSvt / dSvt
		pDynaModel->SetElement(A(V_SVT), A(V_SVT), -1.0 / Tf);
		//dSvt / dSv
		pDynaModel->SetElement(A(V_SVT), A(dSdtIn.Index()), -1.0 / Tf);
	}
	else
	{
		pDynaModel->SetElement(A(V_USUM), A(dVdtIn.Index()), 0.0);
		pDynaModel->SetElement(A(V_USUM), A(dSdtIn.Index()), 0.0);
		pDynaModel->SetElement(A(V_USUM), A(V_SVT), 0.0);
		pDynaModel->SetElement(A(V_USUM), A(V_DVDT), 0.0);
		pDynaModel->SetElement(A(V_USUM), A(V_EQDT), 0.0);
		pDynaModel->SetElement(A(V_USUM), A(V_SDT), 0.0);
		pDynaModel->SetElement(A(V_SVT), A(V_SVT), 1.0);
		pDynaModel->SetElement(A(V_SVT), A(dSdtIn.Index()), 0.0);
	}
		
	bRes = bRes && CDevice::BuildEquations(pDynaModel);

	return true;
}

bool CDynaExcConMustang::BuildRightHand(CDynaModel* pDynaModel)
{
	if (IsStateOn())
	{
		double NodeV = dVdtIn.Value();
		double dSum = Usum - K0u * (Vref * (1.0 + Alpha * dSdtIn.Value()) - NodeV) - K0f * (dSdtIn.Value() - Svt) + *dVdtOutValue + *dEqdtOutValue - *dSdtOutValue;
		RightVector *pRV = pDynaModel->GetRightVector(A(V_USUM));
		if ((m_Id == 94) && (pDynaModel->GetStepNumber() == 1085|| pDynaModel->GetStepNumber() == 1086))
		{
			ATLTRACE(_T("\nId=%d V=%g dSdtIn=%g Svt=%g dVdt=%g dEqdt=%g dSdt=%g NordUsum=%g"), m_Id, NodeV, dSdtIn.Value(), Svt, *dVdtOutValue, *dEqdtOutValue, *dSdtOutValue, pRV->Nordsiek[0]);
		}
		pDynaModel->SetFunction(A(V_USUM), dSum);
		pDynaModel->SetFunctionDiff(A(V_SVT), (dSdtIn.Value() - Svt) / Tf);
	}
	else
	{
		pDynaModel->SetFunction(A(V_USUM), 0.0);
		pDynaModel->SetFunctionDiff(A(V_SVT), 0.0);
	}

	CDevice::BuildRightHand(pDynaModel);
	return true;
}

bool CDynaExcConMustang::BuildDerivatives(CDynaModel *pDynaModel)
{
	bool bRes = CDevice::BuildDerivatives(pDynaModel);
	if (IsStateOn())
		pDynaModel->SetDerivative(A(V_SVT), (dSdtIn.Value() - Svt) / Tf);
	else
		pDynaModel->SetDerivative(A(V_SVT), 0.0);
	return true;
}


eDEVICEFUNCTIONSTATUS CDynaExcConMustang::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = DFS_NOTREADY;
	_ASSERTE(CDevice::IsFunctionStatusOK(GetSingleLink(DEVTYPE_EXCITER)->DiscontinuityProcessed()));
	if (IsStateOn())
	{
		double V = dVdtIn.Value();
		Usum = K0u * (Vref * (1.0 + Alpha * dSdtIn.Value()) - V) + K0f * (dSdtIn.Value() - Svt) - *dVdtOutValue - *dEqdtOutValue + *dSdtOutValue;
		Status = CDevice::ProcessDiscontinuity(pDynaModel);
	}
	else
		Status = DFS_OK;

	return Status;
}


double CDynaExcConMustang::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	if (IsStateOn())
		rH = CDevice::CheckZeroCrossing(pDynaModel);
	return rH;
}

eDEVICEFUNCTIONSTATUS CDynaExcConMustang::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	// производную частоты всегда брать из РДЗ узла (подумать надо ли ее во всех узлах или только в тех, которые под генераторами)
	// или может быть считать ее вообще вне узла - только в АРВ или еще там где нужно
	eDEVICEFUNCTIONSTATUS eRes = DeviceFunctionResult(InitExternalVariable(dSdtIn, GetSingleLink(DEVTYPE_EXCITER), CDynaNode::m_cszS, DEVTYPE_NODE));
	CDevice *pExciter = GetSingleLink(DEVTYPE_EXCITER);
	if (pExciter)
	{
		eRes = DeviceFunctionResult(eRes, InitExternalVariable(dVdtIn, pExciter, CDynaNode::m_cszV, DEVTYPE_NODE));
		eRes = DeviceFunctionResult(eRes, InitExternalVariable(dEqdtIn, pExciter, CDynaGenerator1C::m_cszEq, DEVTYPE_GEN_1C));
	}
	return eRes;
}

const CDeviceContainerProperties CDynaExcConMustang::DeviceProperties()
{
	CDeviceContainerProperties props;
	props.SetType(DEVTYPE_EXCCON);
	props.SetType(DEVTYPE_EXCCON_MUSTANG);
	props.m_strClassName = CDeviceContainerProperties::m_cszNameExcConMustang;
	props.AddLinkFrom(DEVTYPE_EXCITER, DLM_SINGLE, DPD_MASTER);

	props.nEquationsCount = CDynaExcConMustang::VARS::V_LAST;
	

	props.m_VarMap.insert(make_pair(_T("Uf"), CVarIndex(CDynaExcConMustang::V_UF, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(_T("Usum"), CVarIndex(CDynaExcConMustang::V_USUM, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(_T("Svt"), CVarIndex(CDynaExcConMustang::V_SVT, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(_T("dVdt"), CVarIndex(CDynaExcConMustang::V_DVDT, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(_T("dEqdt"), CVarIndex(CDynaExcConMustang::V_EQDT, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(_T("dSdt"), CVarIndex(CDynaExcConMustang::V_SDT, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(_T("dVdtLag"), CVarIndex(CDynaExcConMustang::V_DVDT + 1, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(_T("dEqdtLag"), CVarIndex(CDynaExcConMustang::V_EQDT + 1, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(_T("dSdtLag"), CVarIndex(CDynaExcConMustang::V_SDT + 1, VARUNIT_PU)));
	return props;
}