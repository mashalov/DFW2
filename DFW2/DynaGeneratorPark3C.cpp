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

void CDynaGeneratorPark3C::CalculateFundamentalParameters()
{
	// взаимные индуктивности без рассеивания [3.111 и 3.112]
	const double xad(xd - xl), xaq(xq - xl);  
	// сопротивление утечки обмотки возбуждения [4.29]
	double denom = xad - xd1 + xl;
	const double xlfd( Equal(denom,0.0) ? xad * (xd1 - xl) / denom : 1E6); 
	denom = xaq - xq1 + xl;
	// сопротивление утечки первой демпферной обмотки q [4.33]
	const double xl1q(Equal(denom, 0.0) ? xaq * (xq1 - xl) / denom : 1E6);
	// сопротивление утечки демпферной обмотки d [4.28]
	denom = xad * xlfd - (xd2 - xl) * (xlfd + xad);
	const double xl1d(Equal(denom, 0.0) ? xad * xlfd * (xd2 - xl) / denom : 1E6);
	// сопротивление утечки второй демпферной обмотки q [4.32]
	denom = xaq * xl1q - (xq2 - xl) * ( xl1q + xaq );
	const double xl2q(Equal(denom, 0.0) ? xaq * xl1q * (xq2 - xl) / denom : 1E6);

	const double xFd(xad + xlfd);		// сопротивление обмотки возбуждения
	const double x1D(xad + xl1d);		// сопротивление демпферной обмотки d
	const double x1Q(xaq + xl1q);		// сопротивление первой демпферной обмотки q
	const double x2Q(xaq + xl2q);		// сопротивление второй демпферной обмотки q

	// активное сопротивление обмотки возбуждения [4.15]
	const double Rfd = xFd / Td01;
	// активное сопротивление демпферной обмотки d [4.15]
	const double R1d = (xad * xlfd / xFd + xl1d) / Td02;	
	// активное сопротивление первой демпферной обмотки q [4.30]
	const double R1q = x1Q / Tq01;	
	// активное сопротивление второй демпферной обмотки q [4.33]
	const double R2q = (xaq * xl1q / x1Q + xl1d) / Tq02;


	


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
	Serializer->AddProperty(CDynaGenerator3C::m_csztd01, Td01, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(CDynaGenerator3C::m_csztd02, Td02, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(m_csztq01, Tq01, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(CDynaGenerator3C::m_csztq02, Tq02, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(m_cszxl, xl, eVARUNITS::VARUNIT_OHM);
}