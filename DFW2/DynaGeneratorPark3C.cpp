#include "stdafx.h"
#include "DynaGenerator3C.h"
#include "DynaGeneratorPark3C.h"

using namespace DFW2;

double* CDynaGeneratorPark3C::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double* p = CDynaGeneratorDQBase::GetVariablePtr(nVarIndex);
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(Psid.Value, V_PSI_D)
			MAP_VARIABLE(Psiq.Value, V_PSI_Q)
			MAP_VARIABLE(Psifd.Value, V_PSI_FD)
		}
	}
	return p;
}

VariableIndexRefVec& CDynaGeneratorPark3C::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDynaGeneratorDQBase::GetVariables(JoinVariables({ Psid, Psiq, Psifd }, ChildVec));
}

bool CDynaGeneratorPark3C::BuildEquations(CDynaModel* pDynaModel)
{
	bool bRes(true);

	bRes = bRes && BuildRIfromDQEquations(pDynaModel);

	return bRes;
}
bool CDynaGeneratorPark3C::BuildRightHand(CDynaModel* pDynaModel)
{
	bool bRes(true);

	bRes = bRes && BuildRIfromDQRightHand(pDynaModel);
	return bRes;
}

void CDynaGeneratorPark3C::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaGeneratorMotion::DeviceProperties(props);
	props.SetType(DEVTYPE_GEN_PARK3C);
	/*
	props.AddLinkTo(DEVTYPE_EXCITER, DLM_SINGLE, DPD_SLAVE, CDynaGenerator1C::m_cszExciterId);
	props.SetClassName(CDeviceContainerProperties::m_cszNameGenerator1C, CDeviceContainerProperties::m_cszSysNameGenerator1C);

	props.nEquationsCount = CDynaGenerator1C::VARS::V_LAST;

	props.m_VarMap.insert(std::make_pair("Eqs", CVarIndex(CDynaGenerator1C::V_EQS, VARUNIT_KVOLTS)));
	props.m_VarMap.insert(std::make_pair(CDynaGenerator1C::m_cszEq, CVarIndex(CDynaGenerator1C::V_EQ, VARUNIT_KVOLTS)));
	props.m_VarMap.insert(std::make_pair("Id", CVarIndex(CDynaGenerator1C::V_ID, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(std::make_pair("Iq", CVarIndex(CDynaGenerator1C::V_IQ, VARUNIT_KAMPERES)));
	props.m_VarMap.insert(std::make_pair("Vd", CVarIndex(CDynaGenerator1C::V_VD, VARUNIT_KVOLTS)));
	props.m_VarMap.insert(std::make_pair("Vq", CVarIndex(CDynaGenerator1C::V_VQ, VARUNIT_KVOLTS)));

	props.m_ConstVarMap.insert(std::make_pair(CDynaGenerator1C::m_cszExciterId, CConstVarIndex(CDynaGenerator1C::C_EXCITERID, eDVT_CONSTSOURCE)));
	props.m_ConstVarMap.insert(std::make_pair(CDynaGenerator1C::m_cszEqnom, CConstVarIndex(CDynaGenerator1C::C_EQNOM, eDVT_INTERNALCONST)));
	props.m_ConstVarMap.insert(std::make_pair(CDynaGenerator1C::m_cszSnom, CConstVarIndex(CDynaGenerator1C::C_SNOM, eDVT_INTERNALCONST)));
	props.m_ConstVarMap.insert(std::make_pair(CDynaGenerator1C::m_cszInom, CConstVarIndex(CDynaGenerator1C::C_INOM, eDVT_INTERNALCONST)));
	props.m_ConstVarMap.insert(std::make_pair(CDynaGenerator1C::m_cszQnom, CConstVarIndex(CDynaGenerator1C::C_QNOM, eDVT_INTERNALCONST)));
	props.m_ConstVarMap.insert(std::make_pair(CDynaGenerator1C::m_cszEqe, CConstVarIndex(CDynaGenerator1C::C_EQE, eDVT_INTERNALCONST)));
	*/

	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaGeneratorPark3C>>();
}

void CDynaGeneratorPark3C::UpdateSerializer(CSerializerBase* Serializer)
{
	// обновляем сериализатор базового класса
	CDynaGeneratorDQBase::UpdateSerializer(Serializer);

	Serializer->AddState("Psid", Psid, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState("Psiq", Psiq, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState("Psifd", Psifd, eVARUNITS::VARUNIT_KVOLTS);
	
	Serializer->AddProperty(CDynaGenerator1C::m_cszxd,  xd, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(CDynaGenerator3C::m_cszxd2, xd2, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(CDynaGenerator3C::m_cszxq1, xq1, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(CDynaGenerator3C::m_cszxq2, xq2, eVARUNITS::VARUNIT_OHM);
}