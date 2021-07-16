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


double CDynaGeneratorDQBase::Xgen()
{
	return 0.5 * (xq + xd1);
}

cplx CDynaGeneratorDQBase::Igen(ptrdiff_t nIteration)
{
	cplx YgInt = 1.0 / cplx(0.0, Xgen());

	if (!nIteration)
		m_Egen = GetEMF();
	else
	{
		cplx Ig = (m_Egen - std::polar((double)V, (double)DeltaV)) * YgInt;
		cplx Idq = Ig * std::polar(1.0, -Delta);
		Id = Idq.imag();
		Iq = Idq.real();
	}

	//double Id0 = Id, Iq0 = Iq;
	cplx Ig = CalculateEgen() * YgInt;
	//Id = Id0; Iq = Iq0;
	return Ig;
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
	Serializer->AddProperty("r", r, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(m_cszExciterId, m_ExciterId);
	// добавляем переменные состояния модели одноконтурной модели генератора в ЭДС
	Serializer->AddState("Egen", m_Egen, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState("Vd", Vd, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState("Vq", Vq, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState("Ids", Id, eVARUNITS::VARUNIT_KAMPERES);
	Serializer->AddState("Iqs", Iq, eVARUNITS::VARUNIT_KAMPERES);
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
	props.m_VarMap.insert(std::make_pair("Id", CVarIndex(CDynaGeneratorDQBase::V_ID, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(std::make_pair("Iq", CVarIndex(CDynaGeneratorDQBase::V_IQ, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(std::make_pair("Vd", CVarIndex(CDynaGeneratorDQBase::V_VD, VARUNIT_KVOLTS)));
	props.m_VarMap.insert(std::make_pair("Vq", CVarIndex(CDynaGeneratorDQBase::V_VQ, VARUNIT_KVOLTS)));

	props.m_ConstVarMap.insert(std::make_pair(CDynaGeneratorDQBase::m_cszExciterId, CConstVarIndex(CDynaGeneratorDQBase::C_EXCITERID, eDVT_CONSTSOURCE)));
	props.m_ConstVarMap.insert(std::make_pair(CDynaGeneratorDQBase::m_cszEqnom, CConstVarIndex(CDynaGeneratorDQBase::C_EQNOM, eDVT_INTERNALCONST)));
	props.m_ConstVarMap.insert(std::make_pair(CDynaGeneratorDQBase::m_cszSnom, CConstVarIndex(CDynaGeneratorDQBase::C_SNOM, eDVT_INTERNALCONST)));
	props.m_ConstVarMap.insert(std::make_pair(CDynaGeneratorDQBase::m_cszInom, CConstVarIndex(CDynaGeneratorDQBase::C_INOM, eDVT_INTERNALCONST)));
	props.m_ConstVarMap.insert(std::make_pair(CDynaGeneratorDQBase::m_cszQnom, CConstVarIndex(CDynaGeneratorDQBase::C_QNOM, eDVT_INTERNALCONST)));
	props.m_ConstVarMap.insert(std::make_pair(CDynaGeneratorDQBase::m_cszEqe, CConstVarIndex(CDynaGeneratorDQBase::C_EQE, eDVT_INTERNALCONST)));

	// запрещаем явное использование фабрики данного класса
	props.DeviceFactory = nullptr;
}

const cplx& CDynaGeneratorDQBase::CalculateEgen()
{
	return m_Egen;
}

// вводит в матрицу блок уравнении для преобразования
// из dq в ri
bool CDynaGeneratorDQBase::BuildIfromDQEquations(CDynaModel* pDynaModel)
{

	double co(cos(Delta)), si(sin(Delta));

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

	return true;
}

// вводит в правую часть уравнения для преобразования 
// из dq в ri
bool CDynaGeneratorDQBase::BuildIfromDQRightHand(CDynaModel* pDynaModel)
{
	double co(cos(Delta)), si(sin(Delta));
	pDynaModel->SetFunction(Ire, Ire - Iq * co + Id * si);
	pDynaModel->SetFunction(Iim, Iim - Iq * si - Id * co);

	return true;
}