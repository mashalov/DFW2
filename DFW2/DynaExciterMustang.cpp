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

	double  V = Ug0;
	if (bVoltageDependent)
		V = ExtVg;

	//dEqe / dEqe
	pDynaModel->SetElement(Eqe, Eqe, 1.0);
	//dEqe / dEqeV
	pDynaModel->SetElement(Eqe, ExcLag, -ZeroDivGuard(V, Ug0));
	//dEqe / dV
	pDynaModel->SetElement(Eqe, ExtVg, -ZeroDivGuard(ExcLag, Ug0));
	bRes = bRes && CDynaExciterBase::BuildEquations(pDynaModel);
	return true;
}


bool CDynaExciterMustang::BuildRightHand(CDynaModel* pDynaModel)
{
	double dEqsum = Eqsum - (Eqe0 + ExtUf + Kig * (GetIg() - Ig0) + Kif * (EqInput - Eq0) + ExtUdec);
	double  V = Ug0;
	if (bVoltageDependent)
		V = ExtVg;

	pDynaModel->SetFunction(Eqsum, dEqsum);
	pDynaModel->SetFunction(Eqe, Eqe - ExcLag * ZeroDivGuard(V, Ug0));
	CDevice::BuildRightHand(pDynaModel);
	return true;
}

eDEVICEFUNCTIONSTATUS CDynaExciterMustang::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = CDynaExciterBase::Init(pDynaModel);
		
	ExtUdec = 0.0;
	double Eqnom, Inom;

	if (!InitConstantVariable(Eqnom, GetSingleLink(DEVTYPE_GEN_1C), CDynaGenerator1C::m_cszEqnom, DEVTYPE_GEN_1C))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	if (!InitConstantVariable(Inom, GetSingleLink(DEVTYPE_GEN_1C), CDynaGenerator1C::m_cszInom, DEVTYPE_GEN_1C))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;

	if (CDevice::IsFunctionStatusOK(Status))
	{
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

	_ASSERTE(CDevice::IsFunctionStatusOK(GetSingleLink(DEVTYPE_GEN_1C)->DiscontinuityProcessed()));

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
				pDynaModel->DiscontinuityRequest();

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

		rH = min(rH, rHEq);

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


const CDeviceContainerProperties CDynaExciterMustang::DeviceProperties()
{
	CDeviceContainerProperties props = CDynaExciterBase::DeviceProperties();
	props.nEquationsCount = CDynaExciterMustang::VARS::V_LAST;
	props.SetClassName(CDeviceContainerProperties::m_cszNameExciterMustang, CDeviceContainerProperties::m_cszSysNameExciterMustang);

	props.m_VarMap.insert(std::make_pair(CDynaGenerator1C::m_cszEqe, CVarIndex(CDynaExciterMustang::V_EQE, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair(_T("EqeV"), CVarIndex(CDynaExciterMustang::V_EQEV, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair(_T("Eqsum"), CVarIndex(CDynaExciterMustang::V_EQSUM, VARUNIT_PU)));

	return props;
}


void CDynaExciterMustang::UpdateSerializer(SerializerPtr& Serializer)
{
	CDynaExciterBase::UpdateSerializer(Serializer);
	Serializer->AddProperty(_T("sta"), TypedSerializedValue::eValueType::VT_STATE);
	Serializer->AddProperty(_T("Name"), TypedSerializedValue::eValueType::VT_NAME);
	Serializer->AddProperty(_T("Id"), TypedSerializedValue::eValueType::VT_ID);
	Serializer->AddProperty(_T("Texc"), Texc, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(_T("Uf_min"), Umin, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(_T("Uf_max"), Umax, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(_T("If_min"), Imin, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(_T("If_max"), Imax, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(_T("Kif"), Kif, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(_T("Kig"), Kig, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(_T("ForcerId"), DECId);
	Serializer->AddProperty(_T("ExcControlId"), RegId);
	Serializer->AddProperty(_T("Type_rg"), bVoltageDependent);
}