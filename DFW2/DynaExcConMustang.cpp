#include "stdafx.h"
#include "DynaExcConMustang.h"
#include "DynaModel.h"
#include "DerlagContinuous.h"

using namespace DFW2;

CDynaExcConMustang::CDynaExcConMustang() : CDevice(),
	Limiter(*this, { UsumLmt }, { Usum }),
	dVdt(*this,  { dVdtOut, dVdtOut1   }, { dVdtIn }),
	dEqdt(*this, { dEqdtOut, dEqdtOut1 }, { dEqdtIn }),
	dSdt(*this,  { dSdtOut, dSdtOut1   }, { dSdtIn })
{
	Limiter.SetName(cszUsumLmt);
}

VariableIndexRefVec& CDynaExcConMustang::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDevice::GetVariables(JoinVariables({ Uf,
												 Usum,
												 UsumLmt,
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
		MAP_VARIABLE(UsumLmt.Value, V_USUMLMT)
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

void CDynaExcConMustang::ScaleGains(CDynaExcConMustang& excon)
{
	const auto& scale = CDynaExcConMustang::DefaultGains;
	//const auto& scale = CDynaExcConMustang::UnityGains;
	excon.K1u  *= scale.K1u;
	excon.K0f  *= scale.K0f;
	excon.K1f  *= scale.K1f;
	excon.K1if *= scale.K1if;
}

eDEVICEFUNCTIONSTATUS CDynaExcConMustang::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status{ eDEVICEFUNCTIONSTATUS::DFS_OK };

	dVdtOut = dEqdtOut = dSdtOut = 0.0;
	Svt = Uf = Usum = UsumLmt = 0.0;
	double Unom, Eqe0;

	CDevice *pExciter = GetSingleLink(DEVTYPE_EXCITER);

	if (!InitConstantVariable(Unom, pExciter, CDynaPowerInjector::m_cszUnom))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	if (!InitConstantVariable(Eqnom_, pExciter, CDynaGeneratorDQBase::m_cszEqnom))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	if (!InitConstantVariable(Eqe0, pExciter, CDynaGeneratorDQBase::m_cszEqe, DEVTYPE_GEN_DQ))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;

	CheckLimits(Umin, Umax);

	if (CDevice::IsFunctionStatusOK(Status))
	{
		Limiter.SetMinMax(pDynaModel, Umin - Eqe0 / Eqnom_, Umax - Eqe0 / Eqnom_);
		K0u /= Unom;
		K1u /= Unom;
		K0f *= pDynaModel->GetOmega0() / 2.0 / M_PI;
		K1f *= pDynaModel->GetOmega0() / 2.0 / M_PI;
		K1if /= Eqnom_;

		// забавно - если умножить на машстабы до
		// расчета о.е - изменяется количество шагов метода
		CDynaExcConMustang::ScaleGains(*this);

		dVdt.SetTK(pDynaModel->GetMustangDerivativeTimeConstant(), K1u);
		dEqdt.SetTK(pDynaModel->GetMustangDerivativeTimeConstant(), K1if);
		dSdt.SetTK(pDynaModel->GetMustangDerivativeTimeConstant(), K1f);

		switch (GetState())
		{
		case eDEVICESTATE::DS_ON:
		{
			Vref = dVdtIn;
			break;
		}
		break;
		case eDEVICESTATE::DS_OFF:
			Vref = Usum = UsumLmt = 0.0;
			break;
		}
		Status = CDevice::DeviceFunctionResult(Status, CDevice::Init(pDynaModel));
		/*
		dVdt.Init(pDynaModel);
		dEqdt.Init(pDynaModel);
		dSdt.Init(pDynaModel);
		*/



	}
	return Status;
}


void CDynaExcConMustang::BuildEquations(CDynaModel* pDynaModel)
{
	// dUsum / dUsum
	pDynaModel->SetElement(Usum, Usum, 1.0);

	if (IsStateOn())
	{
		if (pDynaModel->FillConstantElements())
		{
			// dUsum / dV
			pDynaModel->SetElement(Usum, dVdtIn, K0u);
			// dUsum / Svt
			pDynaModel->SetElement(Usum, Svt, K0f);
			// dUsum / dVdt
			pDynaModel->SetElement(Usum, dVdtOut, 1.0);
			// dUsum / dEqdt
			pDynaModel->SetElement(Usum, dEqdtOut, 1.0);
			// dUsum / dSdt
			pDynaModel->SetElement(Usum, dSdtOut, -1.0);
			// dUsum / Sv
			pDynaModel->SetElement(Usum, dSdtIn, -K0u * Alpha * Vref - K0f);

			pDynaModel->CountConstElementsToSkip(Usum);
		}
		else
			pDynaModel->SkipConstElements(Usum);

		//dSvt / dSvt
		pDynaModel->SetElement(Svt, Svt, -1.0 / Tf);
		//dSvt / dSv
		pDynaModel->SetElement(Svt, dSdtIn, -1.0 / Tf);

		pDynaModel->SetElement(Uf, UsumLmt, -Eqnom_ / Tr);
		pDynaModel->SetElement(Uf, Uf, -1.0 / Tr);

	}
	else
	{
		pDynaModel->SetElement(Usum, dVdtIn, 0.0);
		pDynaModel->SetElement(Usum, dSdtIn, 0.0);
		pDynaModel->SetElement(Usum, Svt, 0.0);
		pDynaModel->SetElement(Usum, dVdtOut, 0.0);
		pDynaModel->SetElement(Usum, dEqdtOut, 0.0);
		pDynaModel->SetElement(Usum, dSdtOut, 0.0);
		pDynaModel->SetElement(Svt, Svt, 0.0);
		pDynaModel->SetElement(Svt, dSdtIn, 0.0);
		pDynaModel->SetElement(Uf, UsumLmt, 0);
		pDynaModel->SetElement(Uf, Uf, 0.0);
	}
		
	CDevice::BuildEquations(pDynaModel);
}

void CDynaExcConMustang::BuildRightHand(CDynaModel* pDynaModel)
{
	if (IsStateOn())
	{
		const double dSum{ Usum - K0u * (Vref * (1.0 + Alpha * dSdtIn) - dVdtIn) - K0f * (dSdtIn - Svt) + dVdtOut + dEqdtOut - dSdtOut };

		/*
		if (m_Id == 115)
		{
			if (pDynaModel->GetIntegrationStepNumber() == 0)
				DebugLog(fmt::format("Step;V;S;Eq;dV;dS;dEq;Usum;dSLag"));

			DebugLog(fmt::format("{};{};{};{};{};{};{};{};{};",
				pDynaModel->GetIntegrationStepNumber(),
				dVdtIn,
				dSdtIn,
				dEqdtIn,
				dVdtOut,
				dSdtOut,
				dEqdtOut,
				Usum,
				dSdtOut1));
		}
		*/
		
		pDynaModel->SetFunction(Usum, dSum);
		pDynaModel->SetFunctionDiff(Svt, (dSdtIn - Svt) / Tf);
		pDynaModel->SetFunctionDiff(Uf, (Eqnom_ * UsumLmt - Uf) / Tr);
	}
	else
	{
		pDynaModel->SetFunction(Usum, 0.0);
		pDynaModel->SetFunctionDiff(Svt, 0.0);
		pDynaModel->SetFunctionDiff(Uf, 0.0);
	}

	CDevice::BuildRightHand(pDynaModel);
}

void CDynaExcConMustang::BuildDerivatives(CDynaModel *pDynaModel)
{
	CDevice::BuildDerivatives(pDynaModel);
	if (IsStateOn())
	{
		pDynaModel->SetDerivative(Svt, (dSdtIn - Svt) / Tf);
		pDynaModel->SetFunctionDiff(Uf, (Eqnom_ * UsumLmt - Uf) / Tr);
	}
	else
	{
		pDynaModel->SetDerivative(Svt, 0.0);
		pDynaModel->SetDerivative(Uf, 0.0);
	}
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
		eRes = DeviceFunctionResult(eRes, InitExternalVariable(dEqdtIn, pExciter, CDynaGeneratorDQBase::m_cszEq, DEVTYPE_GEN_DQ));
	}
	return eRes;
}


void CDynaExcConMustang::UpdateSerializer(CSerializerBase* Serializer)
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
	Serializer->AddState(CDynaPowerInjector::cszVref_, Vref, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("Usum", Usum, eVARUNITS::VARUNIT_PU);
	Serializer->AddState(cszUsumLmt, UsumLmt, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("Uf", Uf, eVARUNITS::VARUNIT_PU);
	Serializer->AddState("Svt", Svt, eVARUNITS::VARUNIT_PU);
}

void CDynaExcConMustang::UpdateValidator(CSerializerValidatorRules* Validator)
{
	CDevice::UpdateValidator(Validator);
	Validator->AddRule({ CDynaExcConMustang::m_cszAlpha }, &CDynaExcConMustang::ValidatorAlpha);
	Validator->AddRule(CDynaExcConMustang::m_cszTf, &CDynaExcConMustang::ValidatorTf);
	Validator->AddRule( CDynaExcConMustang::m_cszTrv, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule({ CDynaExcConMustang::m_cszKu,
						 CDynaExcConMustang::m_cszKu1,
						 CDynaExcConMustang::m_cszKif1,
						 CDynaExcConMustang::m_cszKf,
						 CDynaExcConMustang::m_cszKf1 }, &CSerializerValidatorRules::NonNegative);
}

void CDynaExcConMustang::DeviceProperties(CDeviceContainerProperties& props)
{
	props.SetType(DEVTYPE_EXCCON);
	props.SetType(DEVTYPE_EXCCON_MUSTANG);
	props.SetClassName(CDeviceContainerProperties::m_cszNameExcConMustang, CDeviceContainerProperties::m_cszSysNameExcConMustang);
	props.AddLinkFrom(DEVTYPE_EXCITER, DLM_SINGLE, DPD_MASTER);

	props.EquationsCount = CDynaExcConMustang::VARS::V_LAST;
	

	props.VarMap_.insert(std::make_pair("Uf", CVarIndex(CDynaExcConMustang::V_UF, VARUNIT_PU)));
	props.VarMap_.insert(std::make_pair("Usum", CVarIndex(CDynaExcConMustang::V_USUM, VARUNIT_PU)));
	props.VarMap_.insert(std::make_pair(cszUsumLmt, CVarIndex(CDynaExcConMustang::V_USUMLMT, VARUNIT_PU)));
	props.VarMap_.insert(std::make_pair("Svt", CVarIndex(CDynaExcConMustang::V_SVT, VARUNIT_PU)));
	props.VarMap_.insert(std::make_pair("dVdt", CVarIndex(CDynaExcConMustang::V_DVDT, VARUNIT_PU)));
	props.VarMap_.insert(std::make_pair("dEqdt", CVarIndex(CDynaExcConMustang::V_EQDT, VARUNIT_PU)));
	props.VarMap_.insert(std::make_pair("dSdt", CVarIndex(CDynaExcConMustang::V_SDT, VARUNIT_PU)));
	props.VarMap_.insert(std::make_pair("dVdtLag", CVarIndex(CDynaExcConMustang::V_DVDT + 1, VARUNIT_PU)));
	props.VarMap_.insert(std::make_pair("dEqdtLag", CVarIndex(CDynaExcConMustang::V_EQDT + 1, VARUNIT_PU)));
	props.VarMap_.insert(std::make_pair("dSdtLag", CVarIndex(CDynaExcConMustang::V_SDT + 1, VARUNIT_PU)));

	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaExcConMustang>>();
}
