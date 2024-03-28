#include "stdafx.h"
#include "DynaExciterMustang.h"
#include "DynaModel.h"

using namespace DFW2;



CDynaExciterMustang::CDynaExciterMustang() : CDynaExciterBase(),
	// лаг работает на выход через ограничитель 
	// (можем выбирать windup/non-windup режимы работы)
	OutputLimit(*this, {EqeV}, {EqeLag}),	
	EqLimit(*this, { EqOutputValue }, { EqInput })
{
	Primitives_.pop_back();
	OutputLimit.SetName(cszEqeV_);
	EqLimit.SetName(cszEqLimit_);
}

double* CDynaExciterMustang::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = CDynaExciterBase::GetVariablePtr(nVarIndex);
	return p;
}

void CDynaExciterMustang::BuildEquations(CDynaModel* pDynaModel)
{
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

	double Ig{ GetIg() };
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
		pDynaModel->SetElement(Eqe, OutputLimit, -ExtVg / Ug0);
		//dEqe / dV
		pDynaModel->SetElement(Eqe, ExtVg, -OutputLimit / Ug0);
	}
	else
	{
		// независимый возбудитель 
		 
		//dEqe / dEqeV
		pDynaModel->SetElement(Eqe, OutputLimit, -1.0);
	}
	CDynaExciterBase::BuildEquations(pDynaModel);
}


void CDynaExciterMustang::BuildRightHand(CDynaModel* pDynaModel)
{
	const double dEqsum{ Eqsum - (Eqe0 + ExtUf + Kig * (GetIg() - Ig0) + Kif * (EqInput - Eq0) + ExtUdec + Uaux) };
	// Для зависимого возбудителя рассчитываем отношение текущего
	// напряжения к исходному. Для независимого отношение 1.0
	if (bVoltageDependent)
		pDynaModel->SetFunction(Eqe, Eqe - OutputLimit * ExtVg / Ug0);
	else
		pDynaModel->SetFunction(Eqe, Eqe - OutputLimit);

	pDynaModel->SetFunction(Eqsum, dEqsum);

	CDevice::BuildRightHand(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CDynaExciterMustang::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status{ CDynaExciterBase::Init(pDynaModel) };
	ExtUdec = 0.0;
	if (CDevice::IsFunctionStatusOK(Status))
	{
		CheckLimits(Umin, Umax);
		CheckLimits(Imin, Imax);

		Kig *= Eqnom / Inom;
		Imin *= Eqnom;
		Imax *= Eqnom;
		// ограничения либо на лаге (non-windup)
		ExcLag.SetMinMaxTK(pDynaModel, Umin, Umax, Texc);
		// либо на ограничителе (windup)
		OutputLimit.SetMinMax(pDynaModel, -1e6, 1e6);
		EqLimit.SetMinMax(pDynaModel, Imin, Imax);
		EqOutputValue = Eq0;
		bool bRes{ EqLimit.Init(pDynaModel) };
		Status = CDevice::DeviceFunctionResult(CDevice::Init(pDynaModel), bRes);
	}

	return Status;
}


void CDynaExciterMustang::BuildDerivatives(CDynaModel *pDynaModel)
{
	CDynaExciterBase::BuildDerivatives(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CDynaExciterMustang::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = eDEVICEFUNCTIONSTATUS::DFS_NOTREADY;
	// wait for generator to process disco

	_ASSERTE(CDevice::IsFunctionStatusOK(GetSingleLink(DEVTYPE_GEN_DQ)->DiscontinuityProcessed()));

	if (IsStateOn())
	{
		Eqsum = Eqe0 + ExtUf + Kig * (GetIg() - Ig0) + Kif * (EqInput - Eq0) + ExtUdec + Uaux;
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
				pDynaModel->DiscontinuityRequest(*this, DiscontinuityLevel::Light);

			// and then process disco of lag
			Status = CDynaExciterBase::ProcessDiscontinuity(pDynaModel);

			double  V{ Ug0 };
			if (bVoltageDependent)
				V = ExtVg;
			Eqe = EqeV * ZeroDivGuard(V, Ug0);

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
	double rH{ 1.0 };

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
	bool bRes{ false };
	{
		bRes = true;

		// ограничения и постоянную времени
		// меняем в зависимости от желаемого режима
		// windup/non-windup
		switch (EqLimitStateResulting)
		{
		case CDynaPrimitiveLimited::eLIMITEDSTATES::Max:
			//OutputLimit.SetMinMax(pDynaModel, Umin, Imax);
			ExcLag.ChangeTimeConstant(Texc);
			ExcLag.ChangeMinMaxTK(pDynaModel, -1E6, 1E6, Texc, 1.0);
			break;
		case CDynaPrimitiveLimited::eLIMITEDSTATES::Min:
			//OutputLimit.SetMinMax(pDynaModel, Imin, Umax);
			ExcLag.ChangeTimeConstant(Texc);
			ExcLag.ChangeMinMaxTK(pDynaModel, -1E6, 1E6, Texc, 1.0);
			break;
		case CDynaPrimitiveLimited::eLIMITEDSTATES::Mid:
			//OutputLimit.SetMinMax(pDynaModel, Umin, Umax);
			ExcLag.ChangeTimeConstant(Texc);
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
	props.EquationsCount = CDynaExciterMustang::VARS::V_LAST;
	props.SetClassName(CDeviceContainerProperties::cszNameExciterMustang_, CDeviceContainerProperties::cszSysNameExciterMustang_);

	props.VarMap_.insert(std::make_pair(CDynaGenerator1C::cszEqe_, CVarIndex(CDynaExciterMustang::V_EQE, VARUNIT_PU)));
	props.VarMap_.insert(std::make_pair(cszEqeV_, CVarIndex(CDynaExciterMustang::V_EQEV, VARUNIT_PU)));
	props.VarMap_.insert(std::make_pair("Eqsum", CVarIndex(CDynaExciterMustang::V_EQSUM, VARUNIT_PU)));
	props.VarMap_.insert(std::make_pair("Lag", CVarIndex(CDynaExciterMustang::V_EQELAG, VARUNIT_PU)));

	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaExciterMustang>>();
}


void CDynaExciterMustang::UpdateSerializer(CSerializerBase* Serializer)
{
	CDynaExciterBase::UpdateSerializer(Serializer);
	AddStateProperty(Serializer);
	Serializer->AddProperty(CDevice::cszName_, TypedSerializedValue::eValueType::VT_NAME);
	Serializer->AddProperty(cszid_, TypedSerializedValue::eValueType::VT_ID);
	Serializer->AddProperty(cszTexc_, Texc, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(cszUf_min_, Umin, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(cszUf_max_, Umax, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(cszIf_min_, Imin, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(cszIf_max_, Imax, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(cszKif_, Kif, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(cszKig_, Kig, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(cszDECId_, DECId);
	Serializer->AddProperty(cszExcControlId_, RegId);
	Serializer->AddProperty(cszType_rg_, bVoltageDependent);
}

void CDynaExciterMustang::UpdateValidator(CSerializerValidatorRules* Validator)
{
	CDynaExciterBase::UpdateValidator(Validator);
	Validator->AddRule({ cszKig_, cszKif_ }, &CSerializerValidatorRules::NonNegative);
	Validator->AddRule(cszTexc_, &CSerializerValidatorRules::BiggerThanZero);
}