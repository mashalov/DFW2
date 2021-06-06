#include "stdafx.h"
#include "DynaExcConMustang.h"
#include "DynaModel.h"
#include "DerlagContinuous.h"

using namespace DFW2;

CDynaExcConMustang::CDynaExcConMustang() : CDevice(),
	Lag(*this, { Uf }, { Usum }),
	dVdt(*this,  { dVdtOut, dVdtOut1   }, { dVdtIn }),
	dEqdt(*this, { dEqdtOut, dEqdtOut1 }, { dEqdtIn }),
	dSdt(*this,  { dSdtOut, dSdtOut1   }, { dSdtIn })
{
}

VariableIndexRefVec& CDynaExcConMustang::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDevice::GetVariables(JoinVariables({ Uf, 
												 Usum, 
												 Svt, 
												 dVdtOut,
												 dVdtOut1,
												 dEqdtOut,
												 dEqdtOut1,
												 dSdtOut,
												 dSdtOut1
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
		MAP_VARIABLE(dVdtOut.Value, V_DVDT)
		MAP_VARIABLE(dEqdtOut.Value, V_EQDT)
		MAP_VARIABLE(dSdtOut.Value, V_SDT)
		MAP_VARIABLE(dVdtOut1.Value, V_DVDT + 1)
		MAP_VARIABLE(dEqdtOut1.Value, V_EQDT + 1)
		MAP_VARIABLE(dSdtOut1.Value, V_SDT + 1)
	}
	return p;
}

eDEVICEFUNCTIONSTATUS CDynaExcConMustang::Init(CDynaModel* pDynaModel)
{
	if (Tf <= 0)
		Tf = 0.1;

	eDEVICEFUNCTIONSTATUS Status = eDEVICEFUNCTIONSTATUS::DFS_OK;

	dVdtOut = dEqdtOut = dSdtOut = 0.0;
	Svt = Uf = Usum = 0.0;
	double Eqnom, Unom, Eqe0;

	CDevice *pExciter = GetSingleLink(DEVTYPE_EXCITER);

	if (!InitConstantVariable(Unom, pExciter, CDynaGeneratorMotion::m_cszUnom))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	if (!InitConstantVariable(Eqnom, pExciter, CDynaGenerator1C::m_cszEqnom))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	if (!InitConstantVariable(Eqe0, pExciter, CDynaGenerator1C::m_cszEqe, DEVTYPE_GEN_1C))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;

	CheckLimits(Umin, Umax);

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
			Vref = dVdtIn;
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

	// dUsum / dUsum
	pDynaModel->SetElement(Usum, Usum, 1.0);

	if (IsStateOn())
	{
		// dUsum / dV
		pDynaModel->SetElement(Usum, dVdtIn, K0u);
		// dUsum / Sv
		pDynaModel->SetElement(Usum, dSdtIn, -K0u * Alpha * Vref - K0f);
		// dUsum / Svt
		pDynaModel->SetElement(Usum, Svt, K0f);
		// dUsum / dVdt
		pDynaModel->SetElement(Usum, dVdtOut, 1.0);
		// dUsum / dEqdt
		pDynaModel->SetElement(Usum, dEqdtOut, 1.0);
		// dUsum / dSdt
		pDynaModel->SetElement(Usum, dSdtOut, -1.0);
		//dSvt / dSvt
		pDynaModel->SetElement(Svt, Svt, -1.0 / Tf);
		//dSvt / dSv
		pDynaModel->SetElement(Svt, dSdtIn, -1.0 / Tf);
	}
	else
	{
		pDynaModel->SetElement(Usum, dVdtIn, 0.0);
		pDynaModel->SetElement(Usum, dSdtIn, 0.0);
		pDynaModel->SetElement(Usum, Svt, 0.0);
		pDynaModel->SetElement(Usum, dVdtOut, 0.0);
		pDynaModel->SetElement(Usum, dEqdtOut, 0.0);
		pDynaModel->SetElement(Usum, dSdtOut, 0.0);
		pDynaModel->SetElement(Svt, Svt, 1.0);
		pDynaModel->SetElement(Svt, dSdtIn, 0.0);
	}
		
	bRes = bRes && CDevice::BuildEquations(pDynaModel);

	return true;
}

bool CDynaExcConMustang::BuildRightHand(CDynaModel* pDynaModel)
{
	if (IsStateOn())
	{
		double dSum = Usum - K0u * (Vref * (1.0 + Alpha * dSdtIn) - dVdtIn) - K0f * (dSdtIn - Svt) + dVdtOut + dEqdtOut - dSdtOut;
		RightVector *pRV = pDynaModel->GetRightVector(Usum);
		/*
		if ((m_Id == 16) && pDynaModel->GetStepNumber() >= 399)
		{
			ATLTRACE("\nId=%d V=%g dSdtIn=%g Svt=%g dVdt=%g dEqdt=%g dSdt=%g NordUsum=%g", m_Id, NodeV, dSdtIn.Value(), Svt, *dVdtOutValue, *dEqdtOutValue, *dSdtOutValue, pRV->Nordsiek[0]);
		}
		*/
		pDynaModel->SetFunction(Usum, dSum);
		pDynaModel->SetFunctionDiff(Svt, (dSdtIn - Svt) / Tf);
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
		pDynaModel->SetDerivative(Svt, (dSdtIn - Svt) / Tf);
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
		//double V = dVdtIn.Value();
		Usum = K0u * (Vref * (1.0 + Alpha * dSdtIn) - dVdtIn) + K0f * (dSdtIn - Svt) - dVdtOut - dEqdtOut + dSdtOut;
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
	return eRes;
}


void CDynaExcConMustang::UpdateSerializer(CSerializerBase* Serializer)
{
	CDevice::UpdateSerializer(Serializer);
	AddStateProperty(Serializer);
	Serializer->AddProperty("Name", TypedSerializedValue::eValueType::VT_NAME);
	Serializer->AddProperty("Id", TypedSerializedValue::eValueType::VT_ID);
	Serializer->AddProperty("Alpha", Alpha);
	Serializer->AddProperty("Trv", Tr, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty("Ku", K0u);
	Serializer->AddProperty("Ku1", K1u);
	Serializer->AddProperty("Kif1", K1if);
	Serializer->AddProperty("Kf", K0f);
	Serializer->AddProperty("Kf1", K1f);
	Serializer->AddProperty("Tf", Tf, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty("Urv_min", Umin, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty("Urv_max", Umax, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("Vref", Vref, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("Usum", Usum, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("Uf", Uf, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("Svt", Svt, eVARUNITS::VARUNIT_PU);
}

void CDynaExcConMustang::DeviceProperties(CDeviceContainerProperties& props)
{
	props.SetType(DEVTYPE_EXCCON);
	props.SetType(DEVTYPE_EXCCON_MUSTANG);
	props.SetClassName(CDeviceContainerProperties::m_cszNameExcConMustang, CDeviceContainerProperties::m_cszSysNameExcConMustang);
	props.AddLinkFrom(DEVTYPE_EXCITER, DLM_SINGLE, DPD_MASTER);

	props.nEquationsCount = CDynaExcConMustang::VARS::V_LAST;
	

	props.m_VarMap.insert(std::make_pair("Uf", CVarIndex(CDynaExcConMustang::V_UF, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("Usum", CVarIndex(CDynaExcConMustang::V_USUM, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("Svt", CVarIndex(CDynaExcConMustang::V_SVT, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("dVdt", CVarIndex(CDynaExcConMustang::V_DVDT, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("dEqdt", CVarIndex(CDynaExcConMustang::V_EQDT, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("dSdt", CVarIndex(CDynaExcConMustang::V_SDT, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("dVdtLag", CVarIndex(CDynaExcConMustang::V_DVDT + 1, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("dEqdtLag", CVarIndex(CDynaExcConMustang::V_EQDT + 1, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("dSdtLag", CVarIndex(CDynaExcConMustang::V_SDT + 1, VARUNIT_PU)));

	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaExcConMustang>>();
}
