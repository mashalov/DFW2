#include "stdafx.h"
#include "DynaGeneratorDQBase.h"
#include "DynaModel.h"

using namespace DFW2;

// расчет токов в осях RI из токов в DQ
void CDynaGeneratorDQBase::IfromDQ()
{
	double co(cos(Delta)), si(sin(Delta));

	Ire = Iq * co - Id * si;
	Iim = Iq * si + Id * co;
}

cplx CDynaGeneratorDQBase::Igen(ptrdiff_t nIteration)
{
	cplx YgInt = 1.0 / Zgen();

	if (!nIteration)
		m_Egen = GetEMF();
	else
	{
		cplx Ig = (m_Egen - std::polar((double)V, (double)DeltaV)) * YgInt;
		cplx Idq = Ig * std::polar(1.0, -Delta);
		FromComplex(Iq, Id, Idq);
	}

	cplx Ig = CalculateEgen() * YgInt;

	return Ig;
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorDQBase::Init(CDynaModel* pDynaModel)
{
	if (Kgen > 1)
	{
		xd1 /= Kgen;
		Pnom *= Kgen;
		xq /= Kgen;
		Mj *= Kgen;
		xd /= Kgen;
		r /= Kgen;
	}

	m_Zgen = { r , 0.5 * (xq + xd1) };

	if (std::abs(xq) < 1E-7) xq = xd1; // place to validation !!!
	if (xd <= 0) xd = xd1;
	if (xq <= 0) xq = xd1;


	return InitModel(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorDQBase::InitModel(CDynaModel* pDynaModel)
{

	if (!pDynaModel->ConsiderDampingEquation())
		Kdemp = 0.0;

	eDEVICEFUNCTIONSTATUS Status = CDynaGeneratorMotion::InitModel(pDynaModel);

	if (CDevice::IsFunctionStatusOK(Status))
	{
		Snom = Equal(cosPhinom, 0.0) ? Pnom : Pnom / cosPhinom;
		Qnom = Snom * sqrt(1.0 - cosPhinom * cosPhinom);
		Inom = Snom / Unom / M_SQRT3;
		Eqnom = (Unom * Unom * (Unom * Unom + Qnom * (xd + xq)) + Snom * Snom * xd * xq) / (Unom * sqrt(Unom * Unom * (Unom * Unom + 2.0 * Qnom * xq) + Snom * Snom * xq * xq));

		switch (GetState())
		{
		case eDEVICESTATE::DS_ON:
			CDynaGeneratorDQBase::ProcessDiscontinuity(pDynaModel);
			break;
		case eDEVICESTATE::DS_OFF:
			Vd = -V;
			Vq = V;
			Eq = 0.0;
			ExtEqe = 0.0;
			break;
		}
	}
	return Status;
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorDQBase::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes = CDynaGeneratorMotion::ProcessDiscontinuity(pDynaModel);
	if (CDevice::IsFunctionStatusOK(eRes))
	{
		if (IsStateOn())
		{
			double DeltaGT = Delta - DeltaV;
			double NodeV = V;
			Vd = -NodeV * sin(DeltaGT);
			Vq = NodeV * cos(DeltaGT);
			double det = (Vd * Vd + Vq * Vq);
			Id = (P * Vd - Q * Vq) / det;
			Iq = (Q * Vd + P * Vq) / det;
			IfromDQ();
		}
		else
		{
			Id = Iq = Eq = Ire = Iim = 0.0;
		}
	}
	return eRes;
}


double* CDynaGeneratorDQBase::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double* p = CDynaGeneratorMotion::GetVariablePtr(nVarIndex);
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(Vd.Value, V_VD)
			MAP_VARIABLE(Vq.Value, V_VQ)
			MAP_VARIABLE(Id.Value, V_ID)
			MAP_VARIABLE(Iq.Value, V_IQ)
			MAP_VARIABLE(Eq.Value, V_EQ)
		}
	}
	return p;
}



double* CDynaGeneratorDQBase::GetConstVariablePtr(ptrdiff_t nVarIndex)
{
	double* p = CDynaGeneratorMotion::GetConstVariablePtr(nVarIndex);
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(m_ExciterId, C_EXCITERID)
			MAP_VARIABLE(Eqnom, C_EQNOM)
			MAP_VARIABLE(Snom, C_SNOM)
			MAP_VARIABLE(Qnom, C_QNOM)
			MAP_VARIABLE(Inom, C_INOM)
			MAP_VARIABLE(*ExtEqe.pValue, C_EQE)
		}
	}
	return p;
}

VariableIndexRefVec& CDynaGeneratorDQBase::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDynaGeneratorMotion::GetVariables(JoinVariables({ Vd, Vq, Id, Iq, Eq }, ChildVec));
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorDQBase::UpdateExternalVariables(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes = CDynaGeneratorMotion::UpdateExternalVariables(pDynaModel);
	CDevice* pExciter = GetSingleLink(DEVTYPE_EXCITER);
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(ExtEqe, pExciter, m_cszEqe));
	return eRes;
}

void CDynaGeneratorDQBase::UpdateSerializer(CSerializerBase* Serializer)
{
	// обновляем сериализатор базового класса
	CDynaGeneratorMotion::UpdateSerializer(Serializer);
	// добавляем свойства модели одноконтурной модели генератора в ЭДС
	Serializer->AddProperty(m_cszExciterId, m_ExciterId);
	Serializer->AddState("Egen", m_Egen, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddProperty(m_cszxd, xd, eVARUNITS::VARUNIT_OHM);
	Serializer->AddState(m_cszVd, Vd, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState(m_cszVq, Vq, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState(m_cszId, Id, eVARUNITS::VARUNIT_KAMPERES);
	Serializer->AddState(m_cszIq, Iq, eVARUNITS::VARUNIT_KAMPERES);
	Serializer->AddState(m_cszEq, Eq, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState(m_cszEqe, ExtEqe, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState(m_cszEqnom, Eqnom, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState(m_cszSnom, Snom, eVARUNITS::VARUNIT_MVA);
	Serializer->AddState(m_cszQnom, Qnom, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddState(m_cszInom, Inom, eVARUNITS::VARUNIT_KAMPERES);
}

void CDynaGeneratorDQBase::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaGeneratorMotion::DeviceProperties(props);
	props.SetType(DEVTYPE_GEN_DQ);
	// название класса не ставим, потому что это базовый класс и явно использоваться не должен
	props.AddLinkTo(DEVTYPE_EXCITER, DLM_SINGLE, DPD_SLAVE, CDynaGeneratorDQBase::m_cszExciterId);
	props.nEquationsCount = CDynaGeneratorDQBase::VARS::V_LAST;

	props.m_VarMap.insert(std::make_pair(CDynaGeneratorDQBase::m_cszEq, CVarIndex(CDynaGeneratorDQBase::V_EQ, VARUNIT_KVOLTS)));
	props.m_VarMap.insert(std::make_pair(m_cszId, CVarIndex(CDynaGeneratorDQBase::V_ID, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(std::make_pair(m_cszIq, CVarIndex(CDynaGeneratorDQBase::V_IQ, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(std::make_pair(m_cszVd, CVarIndex(CDynaGeneratorDQBase::V_VD, VARUNIT_KVOLTS)));
	props.m_VarMap.insert(std::make_pair(m_cszVq, CVarIndex(CDynaGeneratorDQBase::V_VQ, VARUNIT_KVOLTS)));

	props.m_ConstVarMap.insert({ CDynaGeneratorDQBase::m_cszExciterId, CConstVarIndex(CDynaGeneratorDQBase::C_EXCITERID, VARUNIT_PIECES, eDVT_CONSTSOURCE) });
	props.m_ConstVarMap.insert({ CDynaGeneratorDQBase::m_cszEqnom, CConstVarIndex(CDynaGeneratorDQBase::C_EQNOM, VARUNIT_KVOLTS, eDVT_INTERNALCONST) });
	props.m_ConstVarMap.insert({ CDynaGeneratorDQBase::m_cszSnom, CConstVarIndex(CDynaGeneratorDQBase::C_SNOM, VARUNIT_MVA, eDVT_INTERNALCONST) });
	props.m_ConstVarMap.insert({ CDynaGeneratorDQBase::m_cszInom, CConstVarIndex(CDynaGeneratorDQBase::C_INOM, VARUNIT_KAMPERES, eDVT_INTERNALCONST) });
	props.m_ConstVarMap.insert({ CDynaGeneratorDQBase::m_cszQnom, CConstVarIndex(CDynaGeneratorDQBase::C_QNOM, VARUNIT_MVAR, eDVT_INTERNALCONST) });
	props.m_ConstVarMap.insert({ CDynaGeneratorDQBase::m_cszEqe, CConstVarIndex(CDynaGeneratorDQBase::C_EQE, VARUNIT_KVOLTS, eDVT_INTERNALCONST) });

	// запрещаем явное использование фабрики данного класса
	props.DeviceFactory = nullptr;
}

const cplx& CDynaGeneratorDQBase::CalculateEgen()
{
	return m_Egen;
}

// вводит в матрицу блок уравнении для преобразования
// тока из dq в ri и напряжения из ri в dq
void CDynaGeneratorDQBase::BuildRIfromDQEquations(CDynaModel* pDynaModel)
{
	const double co(cos(Delta)), si(sin(Delta));

	// dIre / dIre
	pDynaModel->SetElement(Ire, Ire, 1.0);
	// dIre / dId
	pDynaModel->SetElement(Ire, Id, si);
	// dIre / dIq
	pDynaModel->SetElement(Ire, Iq, -co);
	// dIre / dDeltaG
	pDynaModel->SetElement(Ire, Delta, Iq * si + Id * co);

	// dIim / dIim
	pDynaModel->SetElement(Iim, Iim, 1.0);
	// dIim / dId
	pDynaModel->SetElement(Iim, Id, -co);
	// dIim / dIq
	pDynaModel->SetElement(Iim, Iq, -si);
	// dIim / dDeltaG
	pDynaModel->SetElement(Iim, Delta, Id * si - Iq * co);


	// dVd/dVd
	pDynaModel->SetElement(Vd, Vd, 1);
	// dVd/dVre
	pDynaModel->SetElement(Vd, Vre, si);
	// dVd/dVim
	pDynaModel->SetElement(Vd, Vim, -co);
	// dVd/dDeltaG
	pDynaModel->SetElement(Vd, Delta, Vre * co + Vim * si);

	// dVd/dVd
	pDynaModel->SetElement(Vq, Vq, 1);
	// dVd/dVre
	pDynaModel->SetElement(Vq, Vre, -co);
	// dVd/dVim
	pDynaModel->SetElement(Vq, Vim, -si);
	// dVd/dDeltaG
	pDynaModel->SetElement(Vq, Delta, Vre * si - Vim * co);
}

// вводит в правую часть уравнения для преобразования 
// тока из dq в ri и напряжения из ri в dq
void CDynaGeneratorDQBase::BuildRIfromDQRightHand(CDynaModel* pDynaModel)
{
	const double co(cos(Delta)), si(sin(Delta));
	pDynaModel->SetFunction(Ire, Ire - Iq  * co + Id  * si);
	pDynaModel->SetFunction(Iim, Iim - Iq  * si - Id  * co);
	pDynaModel->SetFunction(Vd,  Vd  + Vre * si - Vim * co);
	pDynaModel->SetFunction(Vq,  Vq  - Vre * co - Vim * si);
}


void CDynaGeneratorDQBase::BuildMotionEquationBlock(CDynaModel* pDynaModel)
{
	// Вариант уравнения движения с расчетом момента от частоты тока
	const double omega(ZeroGuardSlip(1.0 + s)), omegav(ZeroGuardSlip(1.0 + Sv)), MjOmegav(Mj * omegav);
	pDynaModel->SetElement(s, Id, (Vd - 2.0 * Id * r) / MjOmegav);
	pDynaModel->SetElement(s, Iq, (Vq - 2.0 * Iq * r) / MjOmegav);
	pDynaModel->SetElement(s, Vd, Id / MjOmegav);
	pDynaModel->SetElement(s, Vq, Iq / MjOmegav);
	pDynaModel->SetElement(s, s, -(Kdemp + Pt / omega / omega) / Mj);
	pDynaModel->SetElement(s, Sv, -(Id * Vd + Iq * Vq - r * (Id * Id + Iq * Iq)) / MjOmegav / omegav);
	BuildAngleEquationBlock(pDynaModel);
}

void CDynaGeneratorDQBase::BuildMotionEquationRightHand(CDynaModel* pDynaModel)
{
	// Вариант уравнения движения с расчетом момента от частоты тока
	const double omega(ZeroGuardSlip(1.0 + s));
	const double eS ((Pt / omega - Kdemp * s - (Vd * Id + Vq * Iq - (Id*Id + Iq*Iq) * r) / (1.0 + Sv)) / Mj);
	pDynaModel->SetFunctionDiff(s, eS);
	BuildAngleEquationRightHand(pDynaModel);
}