#include "stdafx.h"
#include "DynaExciterMustang.h"
#include "DynaModel.h"

using namespace DFW2;



CDynaExciterMustang::CDynaExciterMustang() : CDynaExciterBase(),
											 EqLimit(this, &EqOutputValue, V_EQE, &EqInput)
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
	if (!pDynaModel->Status())
		return pDynaModel->Status();

	bool bRes = true;


	if (ExtUf.Indexed())
	{
		//dEqsum / dUexc
		pDynaModel->SetElement(A(V_EQSUM), A(ExtUf.Index()), -1.0);
	}

	if (ExtUdec.Indexed())
	{
		//dEqsum / dUdec
		pDynaModel->SetElement(A(V_EQSUM), A(ExtUdec.Index()), 1.0);
	}

	double Ig = GetIg();
	if (Ig > 0)
		Ig = Kig / Ig;

	//double dEqsum = Eqsum - (Eqe0 + Uexc + Kig * (GetIg() - Ig0) + Kif * (m_pGenerator->Eq - Eq0) + Udec);

	//dEqsum / dEqsum
	pDynaModel->SetElement(A(V_EQSUM), A(V_EQSUM), 1.0);
	
	//dEqsum / dId
	pDynaModel->SetElement(A(V_EQSUM), A(GenId.Index()), -Ig * GenId.Value());
	//dEqsum / dIq
	pDynaModel->SetElement(A(V_EQSUM), A(GenIq.Index()), -Ig * GenIq.Value());

	//dEqsum / dEq
	pDynaModel->SetElement(A(V_EQSUM), A(EqInput.Index()), -Kif);

	double  V = Ug0;
	if (bVoltageDependent)
		V = ExtVg.Value();

	//dEqe / dEqe
	pDynaModel->SetElement(A(V_EQE), A(V_EQE), 1.0);
	//dEqe / dEqeV
	pDynaModel->SetElement(A(V_EQE), A(V_EQEV), -ZeroDivGuard(V, Ug0));
	//dEqe / dV
	//pDynaModel->SetElement(A(V_EQE), m_pGenerator->m_pNode->A(CDynaNode::V_V), -ZeroDivGuard(EqeV, Ug0));
	pDynaModel->SetElement(A(V_EQE), A(ExtVg.Index()), -ZeroDivGuard(EqeV, Ug0));
	bRes = bRes && CDynaExciterBase::BuildEquations(pDynaModel);
	return pDynaModel->Status() && bRes;
}


bool CDynaExciterMustang::BuildRightHand(CDynaModel* pDynaModel)
{
	double Ig = GetIg();
	if (Ig > 0)
		Ig = Kig / Ig;


	double dEqsum = Eqsum - (Eqe0 + ExtUf.Value() + Kig * (GetIg() - Ig0) + Kif * (EqInput.Value() - Eq0) + ExtUdec.Value());
	double  V = Ug0;
	if (bVoltageDependent)
		V = ExtVg.Value();

	pDynaModel->SetFunction(A(V_EQSUM), dEqsum);
	pDynaModel->SetFunction(A(V_EQE), Eqe - EqeV * ZeroDivGuard(V, Ug0));
	CDevice::BuildRightHand(pDynaModel);
	return pDynaModel->Status();
}

eDEVICEFUNCTIONSTATUS CDynaExciterMustang::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = CDynaExciterBase::Init(pDynaModel);
		
	Udec = 0.0;
	double Eqnom, Inom;

	if (!InitConstantVariable(Eqnom, GetSingleLink(DEVTYPE_GEN_1C), CDynaGenerator1C::m_cszEqnom, DEVTYPE_GEN_1C))
		Status = DFS_FAILED;
	if (!InitConstantVariable(Inom, GetSingleLink(DEVTYPE_GEN_1C), CDynaGenerator1C::m_cszInom, DEVTYPE_GEN_1C))
		Status = DFS_FAILED;

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
			Status = DFS_FAILED;
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
	eDEVICEFUNCTIONSTATUS Status = DFS_NOTREADY;
	// wait for generator to process disco

	_ASSERTE(CDevice::IsFunctionStatusOK(GetSingleLink(DEVTYPE_GEN_1C)->DiscontinuityProcessed()));

	if (IsStateOn())
	{
		Eqsum = Eqe0 + ExtUf.Value() + Kig * (GetIg() - Ig0) + Kif * (EqInput.Value() - Eq0) + ExtUdec.Value();
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
		}
		else
			Status = DFS_FAILED;
	}
	else
		Status = DFS_OK;

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
	props.m_strClassName = CDeviceContainerProperties::m_cszNameExciterMustang;

	props.m_VarMap.insert(make_pair(CDynaGenerator1C::m_cszEqe, CVarIndex(CDynaExciterMustang::V_EQE, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(_T("EqeV"), CVarIndex(CDynaExciterMustang::V_EQEV, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(_T("Eqsum"), CVarIndex(CDynaExciterMustang::V_EQSUM, VARUNIT_PU)));

	return props;
}