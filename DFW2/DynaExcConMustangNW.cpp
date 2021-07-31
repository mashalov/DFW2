#include "stdafx.h"
#include "DynaExcConMustang.h"
#include "DynaExcConMustangNW.h"
#include "DynaModel.h"
#include "DerlagContinuous.h"

using namespace DFW2;

CDynaExcConMustangNonWindup::CDynaExcConMustangNonWindup() : CDevice(),
	Lag(*this, { Uf }, { Usum }),
	dVdt(*this, { dVdtOut, dVdtOut1 }, { dVdtIn }),
	dEqdt(*this, { dEqdtOut, dEqdtOut1 }, { dEqdtIn }),
	dSdt(*this, { dSdtOut, dSdtOut1 }, { dSdtIn })
{
}

VariableIndexRefVec& CDynaExcConMustangNonWindup::GetVariables(VariableIndexRefVec& ChildVec)
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

double* CDynaExcConMustangNonWindup::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double* p(nullptr);
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

void CDynaExcConMustangNonWindup::ScaleGains(CDynaExcConMustangNonWindup& excon)
{
	const auto& scale = CDynaExcConMustangNonWindup::DefaultGains;
	//const auto& scale = CDynaExcConMustangNonWindup::UnityGains;
	excon.K1u *= scale.K1u;
	excon.K0f *= scale.K0f;
	excon.K1f *= scale.K1f;
	excon.K1if *= scale.K1if;
}

eDEVICEFUNCTIONSTATUS CDynaExcConMustangNonWindup::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = eDEVICEFUNCTIONSTATUS::DFS_OK;

	dVdtOut = dEqdtOut = dSdtOut = 0.0;
	Svt = Uf = Usum = 0.0;
	double Eqnom, Unom, Eqe0;

	CDevice* pExciter = GetSingleLink(DEVTYPE_EXCITER);

	if (!InitConstantVariable(Unom, pExciter, CDynaGeneratorMotion::m_cszUnom))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	if (!InitConstantVariable(Eqnom, pExciter, CDynaGeneratorDQBase::m_cszEqnom))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	if (!InitConstantVariable(Eqe0, pExciter, CDynaGeneratorDQBase::m_cszEqe, DEVTYPE_GEN_DQ))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;

	CheckLimits(Umin, Umax);

	if (CDevice::IsFunctionStatusOK(Status))
	{
		Lag.SetMinMaxTK(pDynaModel, Umin * Eqnom - Eqe0, Umax * Eqnom - Eqe0, Tr, 1.0);

		K0u *= Eqnom / Unom;
		K1u *= Eqnom / Unom;
		K0f *= Eqnom * pDynaModel->GetOmega0() / 2.0 / M_PI;
		K1f *= Eqnom * pDynaModel->GetOmega0() / 2.0 / M_PI;

		// забавно - если умножить на машстабы до
		// расчета о.е - изменяется количество шагов метода
		CDynaExcConMustangNonWindup::ScaleGains(*this);

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


bool CDynaExcConMustangNonWindup::BuildEquations(CDynaModel* pDynaModel)
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

bool CDynaExcConMustangNonWindup::BuildRightHand(CDynaModel* pDynaModel)
{
	if (IsStateOn())
	{
		double dSum = Usum - K0u * (Vref * (1.0 + Alpha * dSdtIn) - dVdtIn) - K0f * (dSdtIn - Svt) + dVdtOut + dEqdtOut - dSdtOut;
		RightVector* pRV = pDynaModel->GetRightVector(Usum);
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

bool CDynaExcConMustangNonWindup::BuildDerivatives(CDynaModel* pDynaModel)
{
	bool bRes = CDevice::BuildDerivatives(pDynaModel);
	if (IsStateOn())
		pDynaModel->SetDerivative(Svt, (dSdtIn - Svt) / Tf);
	else
		pDynaModel->SetDerivative(Svt, 0.0);
	return true;
}


eDEVICEFUNCTIONSTATUS CDynaExcConMustangNonWindup::ProcessDiscontinuity(CDynaModel* pDynaModel)
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


double CDynaExcConMustangNonWindup::CheckZeroCrossing(CDynaModel* pDynaModel)
{
	double rH = 1.0;
	if (IsStateOn())
		rH = CDevice::CheckZeroCrossing(pDynaModel);
	return rH;
}

eDEVICEFUNCTIONSTATUS CDynaExcConMustangNonWindup::UpdateExternalVariables(CDynaModel* pDynaModel)
{
	// производную частоты всегда брать из РДЗ узла (подумать надо ли ее во всех узлах или только в тех, которые под генераторами)
	// или может быть считать ее вообще вне узла - только в АРВ или еще там где нужно
	eDEVICEFUNCTIONSTATUS eRes = DeviceFunctionResult(InitExternalVariable(dSdtIn, GetSingleLink(DEVTYPE_EXCITER), CDynaNode::m_cszS, DEVTYPE_NODE));
	CDevice* pExciter = GetSingleLink(DEVTYPE_EXCITER);
	if (pExciter)
	{
		eRes = DeviceFunctionResult(eRes, InitExternalVariable(dVdtIn, pExciter, CDynaNode::m_cszV, DEVTYPE_NODE));
		eRes = DeviceFunctionResult(eRes, InitExternalVariable(dEqdtIn, pExciter, CDynaGeneratorDQBase::m_cszEq, DEVTYPE_GEN_DQ));
	}
	return eRes;
}

void CDynaExcConMustangNonWindup::UpdateValidator(CSerializerValidatorRules* Validator)
{
	CDevice::UpdateValidator(Validator);
	Validator->AddRule({ CDynaExcConMustang::m_cszAlpha }, &CDynaExcConMustang::ValidatorAlpha);
	Validator->AddRule(CDynaExcConMustang::m_cszTf, &CDynaExcConMustang::ValidatorTf);
	Validator->AddRule(CDynaExcConMustang::m_cszTrv, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule({ CDynaExcConMustang::m_cszKu, 
						 CDynaExcConMustang::m_cszKu1, 
						 CDynaExcConMustang::m_cszKif1, 
						 CDynaExcConMustang::m_cszKf, 
						 CDynaExcConMustang::m_cszKf1 }, &CSerializerValidatorRules::NonNegative);
}

void CDynaExcConMustangNonWindup::UpdateSerializer(CSerializerBase* Serializer)
{
	CDevice::UpdateSerializer(Serializer);
	AddStateProperty(Serializer);
	Serializer->AddProperty(CDevice::m_cszName, TypedSerializedValue::eValueType::VT_NAME);
	Serializer->AddProperty(m_cszid, TypedSerializedValue::eValueType::VT_ID);
	Serializer->AddProperty(CDynaExcConMustang::m_cszAlpha, Alpha);
	Serializer->AddProperty(CDynaExcConMustang::m_cszTrv, Tr, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(CDynaExcConMustang::m_cszKu, K0u);
	Serializer->AddProperty(CDynaExcConMustang::m_cszKu1, K1u);
	Serializer->AddProperty(CDynaExcConMustang::m_cszKif1, K1if);
	Serializer->AddProperty(CDynaExcConMustang::m_cszKf, K0f);
	Serializer->AddProperty(CDynaExcConMustang::m_cszKf1, K1f);
	Serializer->AddProperty(CDynaExcConMustang::m_cszTf, Tf, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(CDynaExcConMustang::m_cszUrv_min, Umin, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(CDynaExcConMustang::m_cszUrv_max, Umax, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("Vref", Vref, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("Usum", Usum, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("Uf", Uf, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("Svt", Svt, eVARUNITS::VARUNIT_PU);
}

void CDynaExcConMustangNonWindup::DeviceProperties(CDeviceContainerProperties& props)
{
	props.SetType(DEVTYPE_EXCCON);
	props.SetType(DEVTYPE_EXCCON_MUSTANG);
	props.SetClassName(CDeviceContainerProperties::m_cszNameExcConMustang, CDeviceContainerProperties::m_cszSysNameExcConMustang);
	props.AddLinkFrom(DEVTYPE_EXCITER, DLM_SINGLE, DPD_MASTER);

	props.nEquationsCount = CDynaExcConMustangNonWindup::VARS::V_LAST;


	props.m_VarMap.insert(std::make_pair("Uf", CVarIndex(CDynaExcConMustangNonWindup::V_UF, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("Usum", CVarIndex(CDynaExcConMustangNonWindup::V_USUM, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("Svt", CVarIndex(CDynaExcConMustangNonWindup::V_SVT, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("dVdt", CVarIndex(CDynaExcConMustangNonWindup::V_DVDT, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("dEqdt", CVarIndex(CDynaExcConMustangNonWindup::V_EQDT, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("dSdt", CVarIndex(CDynaExcConMustangNonWindup::V_SDT, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("dVdtLag", CVarIndex(CDynaExcConMustangNonWindup::V_DVDT + 1, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("dEqdtLag", CVarIndex(CDynaExcConMustangNonWindup::V_EQDT + 1, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("dSdtLag", CVarIndex(CDynaExcConMustangNonWindup::V_SDT + 1, VARUNIT_PU)));

	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaExcConMustangNonWindup>>();
}
