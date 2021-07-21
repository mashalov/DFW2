#include "stdafx.h"
#include "DynaGenerator3C.h"
#include "DynaGeneratorPark3C.h"
#include "DynaModel.h"

using namespace DFW2;

eDEVICEFUNCTIONSTATUS CDynaGeneratorPark3C::Init(CDynaModel* pDynaModel)
{
	return InitModel(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorPark3C::InitModel(CDynaModel* pDynaModel)
{
	xq1 = xq;

	eDEVICEFUNCTIONSTATUS Status = CDynaGeneratorDQBase::InitModel(pDynaModel);

	if (CDevice::IsFunctionStatusOK(Status))
	{
		switch (GetState())
		{
		case eDEVICESTATE::DS_ON:
			break;
		case eDEVICESTATE::DS_OFF:
			break;
		}
	}

	return Status;
}

double* CDynaGeneratorPark3C::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double* p = CDynaGeneratorDQBase::GetVariablePtr(nVarIndex);
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(Psifd.Value, V_PSI_FD)
			MAP_VARIABLE(Psi1d.Value, V_PSI_1D)
			MAP_VARIABLE(Psi1q.Value, V_PSI_1Q)
			MAP_VARIABLE(Psi2q.Value, V_PSI_2Q)
		}
	}
	return p;
}

VariableIndexRefVec& CDynaGeneratorPark3C::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDynaGeneratorDQBase::GetVariables(JoinVariables({ Psifd, Psi1d, Psi1q, Psi2q }, ChildVec));
}

void CDynaGeneratorPark3C::CalculateFundamentalParameters()
{
	// взаимные индуктивности без рассеивания [3.111 и 3.112]
	const double lad(xd - xl), laq(xq - xl);  
	// сопротивление утечки обмотки возбуждения [4.29]
	double denom = lad - xd1 + xl;
	const double lfd( Equal(denom,0.0) ? lad * (xd1 - xl) / denom : 1E6); 
	denom = laq - xq1 + xl;
	// сопротивление утечки первой демпферной обмотки q [4.33]
	const double l1q(Equal(denom, 0.0) ? laq * (xq1 - xl) / denom : 1E6);
	// сопротивление утечки демпферной обмотки d [4.28]
	denom = lad * lfd - (xd2 - xl) * (lfd + lad);
	const double l1d(Equal(denom, 0.0) ? lad * lfd * (xd2 - xl) / denom : 1E6);
	// сопротивление утечки второй демпферной обмотки q [4.32]
	denom = laq * l1q - (xq2 - xl) * ( l1q + laq );
	const double l2q(Equal(denom, 0.0) ? laq * l1q * (xq2 - xl) / denom : 1E6);


	const double lrc = 0.0;

	const double lFd(lad + lfd);		// сопротивление обмотки возбуждения
	const double l1D(lad + l1d);		// сопротивление демпферной обмотки d
	const double l1Q(laq + l1q);		// сопротивление первой демпферной обмотки q
	const double l2Q(laq + l2q);		// сопротивление второй демпферной обмотки q

	// активное сопротивление обмотки возбуждения [4.15]
	const double Rfd = lFd / Td01;
	// активное сопротивление демпферной обмотки d [4.15]
	const double R1d = (lad * lfd / lFd + l1d) / Td02;	
	// активное сопротивление первой демпферной обмотки q [4.30]
	const double R1q = l1Q / Tq01;	
	// активное сопротивление второй демпферной обмотки q [4.31]
	const double R2q = (laq * l1q / l1Q + l2q) / Tq02;

	const double C(lad + lrc), A(C + lfd), B(C + l1d);
	const double& D(l1Q), &F(l2Q);
	double detd(C * C - A * B), detq(laq * laq - D * F);

	if (Equal(detd, 0.0))
		throw dfw2error(fmt::format("detd == 0 for {}", GetVerbalName()));
	if (Equal(detq, 0.0))
		throw dfw2error(fmt::format("detq == 0 for {}", GetVerbalName()));

	detd = 1.0 / detd;	detq = 1.0 / detq;

	Ed_Psi1q	= -laq * l2q * detq;
	Ed_Psi2q	= -laq * l1q * detq;

	Eq_Psifd	=  lad * l1d * detd;
	Eq_Psi1d	=  lad * lfd * detd;

	Psifd_Psifd	= -Rfd * B * detd;
	Psifd_Psi1d	= -Rfd * C * detd;
	Psifd_id	= -Rfd * lad * l1d * detd;

	Psi1d_Psifd	= -R1d * C * detd;
	Psi1d_Psi1d	=  R1d * A * detd;
	Psi1d_id	= -R1d * lad * lfd * detd;

	Psi1q_Psi1q	=  R1q * F * detq;
	Psi1q_Psi2q = -R1q * laq * detq;
	Psi1q_iq	= -R1q * laq * l2q * detq;

	Psi2q_Psi1q	= -R2q * laq * detq;
	Psi2q_Psi2q	=  R2q * D * detq;
	Psi2q_iq	= -R2q * laq * l1q * detq;


	Psid_id = (lad + xl) + lad * lad * (l1d + lfd) * detd;
	Psid_Psifd = -lad * l1d * detd;
	Psid_Psi1d = -lad * lfd * detd;

	Psiq_iq = (laq + xl) + laq * laq * (l1q + l2q) * detq;
	Psiq_Psi1q = -laq * l2q * detq;
	Psiq_Psi2q = -laq * l1q * detq;

	lq2 = xq2;
	ld2 = xd2;
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

	const double dPsifd = ExtEqe + Psifd_Psifd * Psifd + Psifd_Psi1d * Psi1d + Psifd_id * Id;
	const double dPsi1d = Psi1d_Psifd * Psifd + Psi1d_Psi1d * Psi1d + Psi1d_id * Id;
	const double dPsi1q = Psi1q_Psi1q * Psi1q + Psi1q_Psi2q * Psi2q + Psi1q_iq * Iq;
	const double dPsi2q = Psi2q_Psi1q * Psi1q + Psi2q_Psi2q * Psi2q + Psi2q_iq * Iq;

	pDynaModel->SetFunctionDiff(Psifd, dPsifd);
	pDynaModel->SetFunctionDiff(Psi1d, dPsi1d);
	pDynaModel->SetFunctionDiff(Psi1q, dPsi1q);
	pDynaModel->SetFunctionDiff(Psi2q, dPsi2q);

	const double omega = (1.0 + s);
	const double omega2 = omega * omega;
	const double zsq = ZeroDivGuard(1.0, r * r + omega2 * ld2 * lq2);

	const double id = zsq * (
		-r * Vd
		- omega * lq2 * Vq
		+ omega2 * lq2 * Eq_Psifd * Psifd
		+ omega2 * lq2 * Eq_Psi1d * Psi1d
		+ r * omega * Ed_Psi1q * Psi1q
		+ r * omega * Ed_Psi2q * Psi2q
	);

	const double iq = zsq * (
		-r * Vq
		+ omega * ld2 * Vd
		+ r * omega * Eq_Psifd * Psifd
		+ r * omega * Eq_Psi1d * Psi1d
		- omega2 * ld2 * Ed_Psi1q * Psi1q
		- omega2 * ld2 * Ed_Psi2q * Psi2q
	);

	const double torque = (Psiq_iq * Iq + Psiq_Psi1q * Psi1q + Psiq_Psi2q * Psi2q) * Id - (Psid_id * Id + Psid_Psifd * Psifd + Psid_Psi1d * Psi1d) * Iq;
		
	pDynaModel->SetFunction(Id, Id - id);
	pDynaModel->SetFunction(Iq, Iq - iq);

	bRes = bRes && BuildRIfromDQRightHand(pDynaModel);
	return bRes;
}

void CDynaGeneratorPark3C::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaGeneratorDQBase::DeviceProperties(props);
	props.SetType(DEVTYPE_GEN_PARK3C);
	props.SetClassName(CDeviceContainerProperties::m_cszNameGeneratorPark3C, CDeviceContainerProperties::m_cszSysNameGeneratorPark3C);
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

	Serializer->AddState("Psi1d", Psi1d, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState("Psi1q", Psi1q, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState("Psifd", Psifd, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddProperty(CDynaGenerator3C::m_cszxd2, xd2, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(CDynaGenerator3C::m_cszxq1, xq1, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(CDynaGenerator3C::m_cszxq2, xq2, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(CDynaGenerator3C::m_csztd01, Td01, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(CDynaGenerator3C::m_csztd02, Td02, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(m_csztq01, Tq01, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(CDynaGenerator3C::m_csztq02, Tq02, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(m_cszxl, xl, eVARUNITS::VARUNIT_OHM);
}