#include "stdafx.h"
#include "DynaExcConMustang.h"
#include "DynaModel.h"
#include "DerlagContinuous.h"

using namespace DFW2;

CDynaExcConMustang::CDynaExcConMustang() : CDevice(),
	Lag(this, Uf, { &LagIn }),
	LagIn(V_USUM,Usum),
	dVdt(this, dVdtOutValue[0], { &dVdtIn }, {dVdtOutValue[1] }),
	dEqdt(this, dEqdtOutValue[0], { &dEqdtIn }, { dEqdtOutValue[1] }),
	dSdt(this, dSdtOutValue[0], { &dSdtIn }, { dSdtOutValue[1] })
{
}

VariableIndexRefVec& CDynaExcConMustang::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDevice::GetVariables(JoinVariables({ Uf, 
												 Usum, 
												 Svt, 
												 dVdtOutValue[0],
												 dVdtOutValue[1],
												 dEqdtOutValue[0],
												 dEqdtOutValue[1],
												 dSdtOutValue[0],
												 dSdtOutValue[1]
												}, ChildVec));
}

double* CDynaExcConMustang::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p(nullptr);
	switch (nVarIndex)
	{
		MAP_VARIABLE(Uf.Value, V_UF)
		MAP_VARIABLE(Usum.Value, V_USUM)
		MAP_VARIABLE(Svt.Value, V_SVT)
		MAP_VARIABLE(dVdtOutValue[0].Value, V_DVDT)
		MAP_VARIABLE(dEqdtOutValue[0].Value, V_EQDT)
		MAP_VARIABLE(dSdtOutValue[0].Value, V_SDT)
		MAP_VARIABLE(dVdtOutValue[1].Value, V_DVDT + 1)
		MAP_VARIABLE(dEqdtOutValue[1].Value, V_EQDT + 1)
		MAP_VARIABLE(dSdtOutValue[1].Value, V_SDT + 1)
	}
	return p;
}

eDEVICEFUNCTIONSTATUS CDynaExcConMustang::Init(CDynaModel* pDynaModel)
{
	if (Tf <= 0)
		Tf = 0.1;

	eDEVICEFUNCTIONSTATUS Status = eDEVICEFUNCTIONSTATUS::DFS_OK;

	*dVdtOutValue = *dEqdtOutValue = *dSdtOutValue = 0.0;
	Svt = Uf = Usum = 0.0;
	double Eqnom, Unom, Eqe0;

	CDevice *pExciter = GetSingleLink(DEVTYPE_EXCITER);

	if (!InitConstantVariable(Unom, pExciter, CDynaGeneratorMotion::m_cszUnom))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	if (!InitConstantVariable(Eqnom, pExciter, CDynaGenerator1C::m_cszEqnom))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	if (!InitConstantVariable(Eqe0, pExciter, CDynaGenerator1C::m_cszEqe, DEVTYPE_GEN_1C))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;

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
		case eDEVICESTATE::DS_ON:
		{
			bool bRes = true;
			Vref = dVdtIn.Value();
			bRes = Lag.Init(pDynaModel);
			Status = bRes ? eDEVICEFUNCTIONSTATUS::DFS_OK : eDEVICEFUNCTIONSTATUS::DFS_FAILED;
		}
		break;
		case eDEVICESTATE::DS_OFF:
			Status = eDEVICEFUNCTIONSTATUS::DFS_OK;
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
	pDynaModel->SetElement(Usum, Usum, 1.0);

	if (IsStateOn())
	{
		// dUsum / dV
		pDynaModel->SetElement(Usum.Index, dVdtIn.Index(), K0u);
		// dUsum / Sv
		pDynaModel->SetElement(Usum.Index, dSdtIn.Index(), -K0u * Alpha * Vref - K0f);
		// dUsum / Svt
		pDynaModel->SetElement(Usum, Svt, K0f);
		// dUsum / dVdt
		pDynaModel->SetElement(Usum, dVdtOutValue[0], 1.0);
		// dUsum / dEqdt
		pDynaModel->SetElement(Usum, dEqdtOutValue[0], 1.0);
		// dUsum / dSdt
		pDynaModel->SetElement(Usum, dSdtOutValue[0], -1.0);
		//dSvt / dSvt
		pDynaModel->SetElement(Svt, Svt, -1.0 / Tf);
		//dSvt / dSv
		pDynaModel->SetElement(Svt.Index, dSdtIn.Index(), -1.0 / Tf);
	}
	else
	{
		pDynaModel->SetElement(Usum.Index, dVdtIn.Index(), 0.0);
		pDynaModel->SetElement(Usum.Index, dSdtIn.Index(), 0.0);
		pDynaModel->SetElement(Usum, Svt, 0.0);
		pDynaModel->SetElement(Usum, dVdtOutValue[0], 0.0);
		pDynaModel->SetElement(Usum, dEqdtOutValue[0], 0.0);
		pDynaModel->SetElement(Usum, dSdtOutValue[0], 0.0);
		pDynaModel->SetElement(Svt, Svt, 1.0);
		pDynaModel->SetElement(Svt.Index, dSdtIn.Index(), 0.0);
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
		RightVector *pRV = pDynaModel->GetRightVector(Usum);
		/*
		if ((m_Id == 16) && pDynaModel->GetStepNumber() >= 399)
		{
			ATLTRACE(_T("\nId=%d V=%g dSdtIn=%g Svt=%g dVdt=%g dEqdt=%g dSdt=%g NordUsum=%g"), m_Id, NodeV, dSdtIn.Value(), Svt, *dVdtOutValue, *dEqdtOutValue, *dSdtOutValue, pRV->Nordsiek[0]);
		}
		*/
		pDynaModel->SetFunction(Usum, dSum);
		pDynaModel->SetFunctionDiff(Svt, (dSdtIn.Value() - Svt) / Tf);
	}
	else
	{
		pDynaModel->SetFunction(Usum, 0.0);
		pDynaModel->SetFunctionDiff(Svt, 0.0);
	}

	CDevice::BuildRightHand(pDynaModel);
	return true;
}

bool CDynaExcConMustang::BuildDerivatives(CDynaModel *pDynaModel)
{
	bool bRes = CDevice::BuildDerivatives(pDynaModel);
	if (IsStateOn())
		pDynaModel->SetDerivative(Svt, (dSdtIn.Value() - Svt) / Tf);
	else
		pDynaModel->SetDerivative(Svt, 0.0);
	return true;
}


eDEVICEFUNCTIONSTATUS CDynaExcConMustang::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = eDEVICEFUNCTIONSTATUS::DFS_NOTREADY;
	_ASSERTE(CDevice::IsFunctionStatusOK(GetSingleLink(DEVTYPE_EXCITER)->DiscontinuityProcessed()));
	if (IsStateOn())
	{
		double V = dVdtIn.Value();
		Usum = K0u * (Vref * (1.0 + Alpha * dSdtIn.Value()) - V) + K0f * (dSdtIn.Value() - Svt) - *dVdtOutValue - *dEqdtOutValue + *dSdtOutValue;
		Status = CDevice::ProcessDiscontinuity(pDynaModel);
	}
	else
		Status = eDEVICEFUNCTIONSTATUS::DFS_OK;

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
	LagIn.Index(m_nMatrixRow + V_USUM);
	return eRes;
}


void CDynaExcConMustang::UpdateSerializer(SerializerPtr& Serializer)
{
	CDevice::UpdateSerializer(Serializer);
	Serializer->AddProperty(_T("sta"), TypedSerializedValue::eValueType::VT_STATE);
	Serializer->AddProperty(_T("Name"), TypedSerializedValue::eValueType::VT_NAME);
	Serializer->AddProperty(_T("Id"), TypedSerializedValue::eValueType::VT_ID);
	Serializer->AddProperty(_T("Alpha"), Alpha);
	Serializer->AddProperty(_T("Trv"), Tr, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(_T("Ku"), K0u);
	Serializer->AddProperty(_T("Ku1"), K1u);
	Serializer->AddProperty(_T("Kif1"), K1if);
	Serializer->AddProperty(_T("Kf"), K0f);
	Serializer->AddProperty(_T("Kf1"), K1f);
	Serializer->AddProperty(_T("Tf"), Tf, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(_T("Urv_min"), Umin, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(_T("Urv_max"), Umax, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(_T("Vref"), Vref, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(_T("Usum"), Usum, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(_T("Uf"), Uf, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(_T("Svt"), Svt, eVARUNITS::VARUNIT_PU);
}

const CDeviceContainerProperties CDynaExcConMustang::DeviceProperties()
{
	CDeviceContainerProperties props;
	props.SetType(DEVTYPE_EXCCON);
	props.SetType(DEVTYPE_EXCCON_MUSTANG);
	props.SetClassName(CDeviceContainerProperties::m_cszNameExcConMustang, CDeviceContainerProperties::m_cszSysNameExcConMustang);
	props.AddLinkFrom(DEVTYPE_EXCITER, DLM_SINGLE, DPD_MASTER);

	props.nEquationsCount = CDynaExcConMustang::VARS::V_LAST;
	

	props.m_VarMap.insert(std::make_pair(_T("Uf"), CVarIndex(CDynaExcConMustang::V_UF, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair(_T("Usum"), CVarIndex(CDynaExcConMustang::V_USUM, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair(_T("Svt"), CVarIndex(CDynaExcConMustang::V_SVT, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair(_T("dVdt"), CVarIndex(CDynaExcConMustang::V_DVDT, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair(_T("dEqdt"), CVarIndex(CDynaExcConMustang::V_EQDT, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair(_T("dSdt"), CVarIndex(CDynaExcConMustang::V_SDT, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair(_T("dVdtLag"), CVarIndex(CDynaExcConMustang::V_DVDT + 1, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair(_T("dEqdtLag"), CVarIndex(CDynaExcConMustang::V_EQDT + 1, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair(_T("dSdtLag"), CVarIndex(CDynaExcConMustang::V_SDT + 1, VARUNIT_PU)));
	return props;
}
