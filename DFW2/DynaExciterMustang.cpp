#include "stdafx.h"
#include "DynaExciterMustang.h"
#include "DynaModel.h"

using namespace DFW2;



CDynaExciterMustang::CDynaExciterMustang() : CDynaExciterBase(),
	EqLimit(*this, { EqOutputValue }, { EqInput })
{
	m_Primitives.pop_back();
}

double* CDynaExciterMustang::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = CDynaExciterBase::GetVariablePtr(nVarIndex);
	return p;
}

bool CDynaExciterMustang::BuildEquations(CDynaModel* pDynaModel)
{
	bool bRes = true;


	if (ExtUf.Indexed())
	{
		//dEqsum / dUexc
		pDynaModel->SetElement(Eqsum, ExtUf, -1.0);
	}

	if (ExtUdec.Indexed())
	{
		//dEqsum / dUdec
		pDynaModel->SetElement(Eqsum, ExtUdec, 1.0);
	}

	double Ig = GetIg();
	if (Ig > 0)
		Ig = Kig / Ig;

	//double dEqsum = Eqsum - (Eqe0 + Uexc + Kig * (GetIg() - Ig0) + Kif * (m_pGenerator->Eq - Eq0) + Udec);

	//dEqsum / dEqsum
	pDynaModel->SetElement(Eqsum, Eqsum, 1.0);
	
	//dEqsum / dId
	pDynaModel->SetElement(Eqsum, GenId, -Ig * GenId);
	//dEqsum / dIq
	pDynaModel->SetElement(Eqsum, GenIq, -Ig * GenIq);

	//dEqsum / dEq
	pDynaModel->SetElement(Eqsum, EqInput, -Kif);

	//dEqe / dEqe
	pDynaModel->SetElement(Eqe, Eqe, 1.0);

	if (bVoltageDependent)
	{
		// зависимый возбудитель
		// имеет дополительный элемент матрицы к напряжению узла
		
		//dEqe / dEqeV
		pDynaModel->SetElement(Eqe, ExcLag, -ExtVg / Ug0);
		//dEqe / dV
		pDynaModel->SetElement(Eqe, ExtVg, -ExcLag / Ug0);
	}
	else
	{
		// независимый возбудитель 
		 
		//dEqe / dEqeV
		pDynaModel->SetElement(Eqe, ExcLag, -1.0);
	}
	bRes = bRes && CDynaExciterBase::BuildEquations(pDynaModel);
	return true;
}


bool CDynaExciterMustang::BuildRightHand(CDynaModel* pDynaModel)
{
	double dEqsum = Eqsum - (Eqe0 + ExtUf + Kig * (GetIg() - Ig0) + Kif * (EqInput - Eq0) + ExtUdec);
	// Для зависимого возбудителя рассчитываем отношение текущего
	// напряжения к исходному. Для независимого отношение 1.0
	if (bVoltageDependent)
		pDynaModel->SetFunction(Eqe, Eqe - ExcLag * ExtVg / Ug0);
	else
		pDynaModel->SetFunction(Eqe, Eqe - ExcLag);

	pDynaModel->SetFunction(Eqsum, dEqsum);

	CDevice::BuildRightHand(pDynaModel);
	return true;
}

eDEVICEFUNCTIONSTATUS CDynaExciterMustang::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = CDynaExciterBase::Init(pDynaModel);
		
	ExtUdec = 0.0;
	double Eqnom, Inom;

	if (!InitConstantVariable(Eqnom, GetSingleLink(DEVTYPE_GEN_DQ), CDynaGenerator1C::m_cszEqnom, DEVTYPE_GEN_DQ))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	if (!InitConstantVariable(Inom, GetSingleLink(DEVTYPE_GEN_DQ), CDynaGenerator1C::m_cszInom, DEVTYPE_GEN_DQ))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;

	if (CDevice::IsFunctionStatusOK(Status))
	{

		CheckLimits(Umin, Umax);
		CheckLimits(Imin, Imax);

		Kig *= Eqnom / Inom;
		Imin *= Eqnom;
		Imax *= Eqnom;
		ExcLag.SetMinMaxTK(pDynaModel, Umin, Umax, Texc, 1.0);
		EqLimit.SetMinMax(pDynaModel, Imin, Imax);
		EqOutputValue = Eq0;
		bool bRes = ExcLag.Init(pDynaModel);
		bRes = bRes && EqLimit.Init(pDynaModel);
		if (!bRes)
			Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	}

	return Status;
}


bool CDynaExciterMustang::BuildDerivatives(CDynaModel *pDynaModel)
{
	bool bRes = CDynaExciterBase::BuildDerivatives(pDynaModel);
	return bRes;
}

eDEVICEFUNCTIONSTATUS CDynaExciterMustang::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = eDEVICEFUNCTIONSTATUS::DFS_NOTREADY;
	// wait for generator to process disco

	_ASSERTE(CDevice::IsFunctionStatusOK(GetSingleLink(DEVTYPE_GEN_DQ)->DiscontinuityProcessed()));

	if (IsStateOn())
	{
		Eqsum = Eqe0 + ExtUf + Kig * (GetIg() - Ig0) + Kif * (EqInput - Eq0) + ExtUdec;
		// pick up Eq limiter state before disco
		CDynaPrimitiveLimited::eLIMITEDSTATES EqLimitStateInitial = EqLimit.GetCurrentState();

		// process Eq disco

		if (CDevice::IsFunctionStatusOK(EqLimit.ProcessDiscontinuity(pDynaModel)))
		{
			// pick up Eq limiter state after disco
			CDynaPrimitiveLimited::eLIMITEDSTATES EqLimitStateResulting = EqLimit.GetCurrentState();
			// when state of Eq limiter changed, change lag limits
			SetUpEqLimits(pDynaModel, EqLimitStateInitial, EqLimitStateResulting);
			if (EqLimitStateInitial != EqLimitStateResulting)
				pDynaModel->DiscontinuityRequest(*this);

			// and then process disco of lag
			Status = CDynaExciterBase::ProcessDiscontinuity(pDynaModel);
/*
			double  V = Ug0;
			if (bVoltageDependent)
				V = ExtVg.Value();
			Eqe = EqeV * ZeroDivGuard(V, Ug0);
*/
		}
		else
			Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	}
	else
		Status = eDEVICEFUNCTIONSTATUS::DFS_OK;

	return Status;
}

double CDynaExciterMustang::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH = 1.0;

	if (IsStateOn())
	{
		// pick up states before and after zero crossing check of Eq limiter
		rH = CDynaExciterBase::CheckZeroCrossing(pDynaModel);
		CDynaPrimitiveLimited::eLIMITEDSTATES EqLimitStateInitial = EqLimit.GetCurrentState();
		double rHEq = EqLimit.CheckZeroCrossing(pDynaModel);
		CDynaPrimitiveLimited::eLIMITEDSTATES EqLimitStateResulting = EqLimit.GetCurrentState();

		rH = (std::min)(rH, rHEq);

		// change lag limits when eq limiter state changes
		//if (SetUpEqLimits(EqLimitStateInitial, EqLimitStateResulting))
		//	rH = rH;
		_ASSERTE(rH <= 1.0);
	}

	return rH;
}

bool CDynaExciterMustang::SetUpEqLimits(CDynaModel *pDynaModel, CDynaPrimitiveLimited::eLIMITEDSTATES EqLimitStateInitial, CDynaPrimitiveLimited::eLIMITEDSTATES EqLimitStateResulting)
{
	bool bRes = false;
	{
		bRes = true;

		switch (EqLimitStateResulting)
		{
		case CDynaPrimitiveLimited::eLIMITEDSTATES::LS_MAX:
			ExcLag.ChangeMinMaxTK(pDynaModel, Umin, Imax, Texc, 1.0);
			break;
		case CDynaPrimitiveLimited::eLIMITEDSTATES::LS_MIN:
			ExcLag.ChangeMinMaxTK(pDynaModel, Imin, Umax, Texc, 1.0);
			break;
		case CDynaPrimitiveLimited::eLIMITEDSTATES::LS_MID:
			ExcLag.ChangeMinMaxTK(pDynaModel, Umin, Umax, Texc, 1.0);
			break;
		}
	}

	return bRes;
}

eDEVICEFUNCTIONSTATUS CDynaExciterMustang::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	return CDynaExciterBase::UpdateExternalVariables(pDynaModel);
}


void CDynaExciterMustang::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaExciterBase::DeviceProperties(props);
	props.nEquationsCount = CDynaExciterMustang::VARS::V_LAST;
	props.SetClassName(CDeviceContainerProperties::m_cszNameExciterMustang, CDeviceContainerProperties::m_cszSysNameExciterMustang);

	props.m_VarMap.insert(std::make_pair(CDynaGenerator1C::m_cszEqe, CVarIndex(CDynaExciterMustang::V_EQE, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("EqeV", CVarIndex(CDynaExciterMustang::V_EQEV, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair("Eqsum", CVarIndex(CDynaExciterMustang::V_EQSUM, VARUNIT_PU)));

	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaExciterMustang>>();
}


void CDynaExciterMustang::UpdateSerializer(CSerializerBase* Serializer)
{
	CDynaExciterBase::UpdateSerializer(Serializer);
	AddStateProperty(Serializer);
	Serializer->AddProperty(CDevice::m_cszName, TypedSerializedValue::eValueType::VT_NAME);
	Serializer->AddProperty(m_cszid, TypedSerializedValue::eValueType::VT_ID);
	Serializer->AddProperty(m_cszTexc, Texc, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(m_cszUf_min, Umin, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(m_cszUf_max, Umax, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(m_cszIf_min, Imin, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(m_cszIf_max, Imax, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(m_cszKif, Kif, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(m_cszKig, Kig, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(m_cszDECId, DECId);
	Serializer->AddProperty(m_cszExcControlId, RegId);
	Serializer->AddProperty(m_cszType_rg, bVoltageDependent);
}

void CDynaExciterMustang::UpdateValidator(CSerializerValidatorRules* Validator)
{
	CDynaExciterBase::UpdateValidator(Validator);
	Validator->AddRule({ m_cszKig, m_cszKif }, &CSerializerValidatorRules::NonNegative);
	Validator->AddRule(m_cszTexc, &CSerializerValidatorRules::BiggerThanZero);
}