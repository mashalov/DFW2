#include "stdafx.h"
#include "DynaGenerator3C.h"
#include "DynaGeneratorPark3C.h"
#include "DynaModel.h"

using namespace DFW2;

eDEVICEFUNCTIONSTATUS CDynaGeneratorPark3C::PreInit(CDynaModel* pDynaModel)
{
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorPark3C::Init(CDynaModel* pDynaModel)
{
	return InitModel(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorPark3C::InitModel(CDynaModel* pDynaModel)
{
	CalculateFundamentalParameters();
	eDEVICEFUNCTIONSTATUS Status = CDynaGeneratorDQBase::InitModel(pDynaModel);

	const double omega(1.0 + s);
	const double Psiq = -(Vd + r * Id) / omega;		// (7)
	const double Psid =  (Vq + r * Iq) / omega;		// (8)
	const double ifd = (Psid - xd * Id) / lad;		// (1)
	// масштабы ЭДС и тока ротора для АРВ не понятны 
	// Исходные значения одинаковые, но ток идет в масштабе lad, а ЭДС - Rfd / lad;
	ExtEqe = Eq = ifd * lad;
	Psifd = lad * Id + (lad + lrc + lfd) * ifd;		// (2)
	Psi1d = lad * Id + (lad + lrc) * ifd;			// (3)
	Psi1q = laq * Iq;

	cplx emf(GetEMF());

	Pt = (P + r * (Id * Id + Iq * Iq)) * omega;

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

	m_Zgen = { r , 0.5 * (lq2 + ld2) };
	
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
		}
	}
	return p;
}

VariableIndexRefVec& CDynaGeneratorPark3C::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDynaGeneratorDQBase::GetVariables(JoinVariables({ Psifd, Psi1d, Psi1q }, ChildVec));
}

void CDynaGeneratorPark3C::CalculateFundamentalParameters()
{
	// взаимные индуктивности без рассеивания [3.111 и 3.112]
	lad = xd - xl; 
	laq = xq - xl;
	// сопротивление утечки обмотки возбуждения [4.29]
	double denom = lad - xd1 + xl;
	lfd = Equal(denom,0.0) ? 1E6 : lad * (xd1 - xl) / denom; 
	denom = laq - xq2 + xl;
	// для трехконтурной СМ используются другое соотношение между l1q и xq2,
	// в отличие от четырехкотурной
	// сопротивление утечки первой демпферной обмотки q [4.42]
	const double l1q(Equal(denom, 0.0) ?  1E6 :laq * (xq2 - xl) / denom);
	// сопротивление утечки демпферной обмотки d [4.28]
	denom = lad * lfd - (xd2 - xl) * (lfd + lad);
	const double l1d(Equal(denom, 0.0) ? 1E6 : lad * lfd * (xd2 - xl) / denom );

	lrc = 0.0;

	const double lFd(lad + lfd);		// сопротивление обмотки возбуждения
	const double l1D(lad + l1d);		// сопротивление демпферной обмотки d
	const double l1Q(laq + l1q);		// сопротивление первой демпферной обмотки q

	// активное сопротивление обмотки возбуждения [4.15]
	Rfd = lFd / Td01;
	// активное сопротивление демпферной обмотки d [4.15]
	double R1d = (lad * lfd / lFd + l1d) / Td02;	
	// активное сопротивление первой демпферной обмотки q [4.42]
	double R1q = l1Q / Tq02;	

	const double C(lad + lrc), A(C + lfd), B(C + l1d);
	double detd(C * C - A * B);

	if (Equal(detd, 0.0))
		throw dfw2error(fmt::format("detd == 0 for {}", GetVerbalName()));
	if (Equal(l1Q, 0.0))
		throw dfw2error(fmt::format("l1Q == 0 for {}", GetVerbalName()));

	// Пересчет постоянных времени им. НИИПТ
	const double T2(Td01 + Td02), det(0.25 * T2 * T2 - Td01 * Td02 / (1.0 - lad * lad / lFd / l1D));
	if (det >= 0)
	{
		const double Td0 = 0.5 * T2 + std::sqrt(det);
		Rfd = lFd / Td0;
		R1d = l1D / (T2 - Td0);
	}
	

	detd = 1.0 / detd;

	Ed_Psi1q	= -laq / l1Q;

	Eq_Psifd	= -lad * l1d * detd;
	Eq_Psi1d	= -lad * lfd * detd;

	Psifd_Psifd	=  Rfd * B * detd;
	Psifd_Psi1d	= -Rfd * C * detd;
	Psifd_id	= -Rfd * lad * l1d * detd;

	Psi1d_Psifd	= -R1d * C * detd;
	Psi1d_Psi1d	=  R1d * A * detd;
	Psi1d_id	= -R1d * lad * lfd * detd;

	Psi1q_Psi1q	= -R1q / l1Q;
	Psi1q_iq =     R1q * laq / l1Q;

	lq2 = xq2;
	ld2 = xd2;

	Psid_Psifd = -lad * l1d * detd;
	Psid_Psi1d = -lad * lfd * detd;
	Psiq_Psi1q =  laq / l1Q;
}

bool CDynaGeneratorPark3C::BuildEquations(CDynaModel* pDynaModel)
{
	bool bRes(true);

	const double omega(1.0 + s);

	pDynaModel->SetElement(Id, Id, -r);
	pDynaModel->SetElement(Id, Iq, -lq2 * omega);
	pDynaModel->SetElement(Id, Vd, -1);
	pDynaModel->SetElement(Id, Psi1q, Ed_Psi1q * omega);
	pDynaModel->SetElement(Id, s, Ed_Psi1q * Psi1q - Iq * lq2);

	pDynaModel->SetElement(Iq, Id, ld2 * omega);
	pDynaModel->SetElement(Iq, Iq, -r);
	pDynaModel->SetElement(Iq, Vq, -1);
	pDynaModel->SetElement(Iq, Psifd, Eq_Psifd * omega);
	pDynaModel->SetElement(Iq, Psi1d, Eq_Psi1d * omega);
	pDynaModel->SetElement(Iq, s, Id * ld2 + Eq_Psi1d * Psi1d + Eq_Psifd * Psifd);

	pDynaModel->SetElement(Psifd, Id, -Psifd_id);
	pDynaModel->SetElement(Psifd, Psifd, Psifd_Psifd);
	pDynaModel->SetElement(Psifd, Psi1d, -Psifd_Psi1d);

	if (ExtEqe.Indexed())
		pDynaModel->SetElement(Psifd, ExtEqe, -Rfd / lad);

	pDynaModel->SetElement(Psi1d, Id, -Psi1d_id);
	pDynaModel->SetElement(Psi1d, Psifd, -Psi1d_Psifd);
	pDynaModel->SetElement(Psi1d, Psi1d, Psi1d_Psi1d);

	pDynaModel->SetElement(Psi1q, Iq, -Psi1q_iq);
	pDynaModel->SetElement(Psi1q, Psi1q, Psi1q_Psi1q);

	pDynaModel->SetElement(Eq, Eq, 1);
	pDynaModel->SetElement(Eq, Psifd, Psifd_Psifd * lad / Rfd);
	pDynaModel->SetElement(Eq, Psi1d, Psifd_Psi1d * lad / Rfd);
	pDynaModel->SetElement(Eq, Id, Psifd_id * lad / Rfd);

	BuildRIfromDQEquations(pDynaModel);

#ifdef USE_VOLTAGE_FREQ_DAMPING
	CDynaGeneratorDQBase::BuildMotionEquationBlock(pDynaModel);
#else
	pDynaModel->SetElement(s, Id, -(Psi1q * Psiq_Psi1q - Iq * ld2 + Iq * lq2) / Mj);
	pDynaModel->SetElement(s, Iq, (Psi1d * Psid_Psi1d + Psifd * Psid_Psifd + Id * ld2 - Id * lq2) / Mj);
	pDynaModel->SetElement(s, Psifd, Iq * Psid_Psifd / Mj);
	pDynaModel->SetElement(s, Psi1d, Iq * Psid_Psi1d / Mj);
	pDynaModel->SetElement(s, Psi1q, -Id * Psiq_Psi1q / Mj);
	pDynaModel->SetElement(s, s, -(Kdemp + Pt / omega / omega) / Mj);
	BuildAngleEquationBlock(pDynaModel);
#endif

	return bRes;
}

cplx CDynaGeneratorPark3C::GetIdIq() const
{
	const double omega(1.0 + s);
	const double omega2 = omega * omega;
	const double zsq = ZeroDivGuard(1.0, r * r + omega2 * ld2 * lq2);

	const double id = zsq * (
		-r * Vd
		+ omega * lq2 * Vq
		- omega2 * lq2 * Eq_Psifd * Psifd
		- omega2 * lq2 * Eq_Psi1d * Psi1d
		+ r * omega * Ed_Psi1q * Psi1q
		);

	const double iq = zsq * (
		-r * Vq
		- omega * ld2 * Vd
		+ r * omega * Eq_Psifd * Psifd
		+ r * omega * Eq_Psi1d * Psi1d
		+ omega2 * ld2 * Ed_Psi1q * Psi1q
		);

	return { id,iq };
}

bool CDynaGeneratorPark3C::BuildRightHand(CDynaModel* pDynaModel)
{
	bool bRes(true);
	const double omega(1.0 + s);
	const double dEq = Eq + (Psifd_Psifd * Psifd + Psifd_Psi1d * Psi1d + Psifd_id * Id) * lad / Rfd;
	const double dId = -r * Id - omega * lq2 * Iq + omega * Ed_Psi1q * Psi1q - Vd;
	const double dIq = -r * Iq + omega * ld2 * Id + omega * Eq_Psifd * Psifd + omega * Eq_Psi1d * Psi1d - Vq;
	pDynaModel->SetFunction(Id, dId);
	pDynaModel->SetFunction(Iq, dIq);
	pDynaModel->SetFunction(Eq, dEq);
	SetFunctionsDiff(pDynaModel);
	BuildRIfromDQRightHand(pDynaModel);
	return bRes;
}


void CDynaGeneratorPark3C::CalculateDerivatives(CDynaModel* pDynaModel, CDevice::fnDerivative fn)
{
	if (IsStateOn())
	{
		const double omega(1.0 + s);
		const double dPsifd = Rfd * ExtEqe / lad + Psifd_Psifd * Psifd + Psifd_Psi1d * Psi1d + Psifd_id * Id;
		const double dPsi1d = Psi1d_Psifd * Psifd + Psi1d_Psi1d * Psi1d + Psi1d_id * Id;
		const double dPsi1q = Psi1q_Psi1q * Psi1q + Psi1q_iq * Iq;
#ifdef USE_VOLTAGE_FREQ_DAMPING
		CDynaGeneratorDQBase::CalculateDerivatives(pDynaModel, fn);
#else
		const double Te = (ld2 * Id + Psid_Psifd * Psifd + Psid_Psi1d * Psi1d) * Iq - (lq2 * Iq + Psiq_Psi1q * Psi1q) * Id;
		(pDynaModel->*fn)(Delta, pDynaModel->GetOmega0() * s);
		(pDynaModel->*fn)(s, (Pt / omega - Kdemp * s - Te) / Mj);
#endif
		(pDynaModel->*fn)(Psifd, dPsifd);
		(pDynaModel->*fn)(Psi1d, dPsi1d);
		(pDynaModel->*fn)(Psi1q, dPsi1q);
	}
	else
	{
		(pDynaModel->*fn)(Delta, 0);
		(pDynaModel->*fn)(s, 0);
		(pDynaModel->*fn)(Psifd, 0);
		(pDynaModel->*fn)(Psi1d, 0);
		(pDynaModel->*fn)(Psi1q, 0);
	}
}

bool CDynaGeneratorPark3C::BuildDerivatives(CDynaModel* pDynaModel)
{
	SetDerivatives(pDynaModel);
	return true;
}


eDEVICEFUNCTIONSTATUS CDynaGeneratorPark3C::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes = CDynaGeneratorDQBase::ProcessDiscontinuity(pDynaModel);
	if (CDevice::IsFunctionStatusOK(eRes) && IsStateOn())
	{
		CalculatePower();
		Eq = -lad * (Psifd_Psifd * Psifd + Psifd_Psi1d * Psi1d + Psifd_id * Id) / Rfd;
	}
	return eRes;
}

bool CDynaGeneratorPark3C::CalculatePower()
{
	double NodeV = V;
	double DeltaGT = Delta - DeltaV;
	double cosDeltaGT = cos(DeltaGT);
	double sinDeltaGT = sin(DeltaGT);
	Vd = -NodeV * sinDeltaGT;
	Vq = NodeV * cosDeltaGT;
	FromComplex(Id,Iq, GetIdIq());
	P = Vd * Id + Vq * Iq;
	Q = Vd * Iq - Vq * Id;
	IfromDQ();
	return true;
}

cplx CDynaGeneratorPark3C::GetEMF()
{
	cplx emf{ Eq_Psifd * Psifd + Eq_Psi1d * Psi1d, Ed_Psi1q * Psi1q };
	emf *= (1.0 + s);
	emf *= std::polar(1.0, Delta.Value);
	return emf;
}

const cplx& CDynaGeneratorPark3C::CalculateEgen()
{

	double xgen = Zgen().imag();
	cplx emf(GetEMF() * std::polar(1.0, -Delta.Value));
	const double omega(1.0 + Sv);
	return m_Egen = cplx(emf.real() - omega * Id * (xgen - ld2),  emf.imag() - omega * Iq * (lq2 - xgen)) * std::polar(1.0, (double)Delta);
}

void CDynaGeneratorPark3C::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaGeneratorDQBase::DeviceProperties(props);
	props.SetType(DEVTYPE_GEN_PARK3C);
	props.SetClassName(CDeviceContainerProperties::m_cszNameGeneratorPark3C, CDeviceContainerProperties::m_cszSysNameGeneratorPark3C);
	props.nEquationsCount = CDynaGeneratorPark3C::VARS::V_LAST;
	props.m_VarMap.insert(std::make_pair(m_cszPsifd, CVarIndex(V_PSI_FD, eVARUNITS::VARUNIT_WB)));
	props.m_VarMap.insert(std::make_pair(m_cszPsi1d, CVarIndex(V_PSI_1D, eVARUNITS::VARUNIT_WB)));
	props.m_VarMap.insert(std::make_pair(m_cszPsi1q, CVarIndex(V_PSI_1Q, eVARUNITS::VARUNIT_WB)));
	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaGeneratorPark3C>>();
}

void CDynaGeneratorPark3C::UpdateSerializer(CSerializerBase* Serializer)
{
	// обновляем сериализатор базового класса
	CDynaGeneratorDQBase::UpdateSerializer(Serializer);

	Serializer->AddState(m_cszPsifd, Psifd, eVARUNITS::VARUNIT_WB);
	Serializer->AddState(m_cszPsi1d, Psi1d, eVARUNITS::VARUNIT_WB);
	Serializer->AddState(m_cszPsi1q, Psi1q, eVARUNITS::VARUNIT_WB);
	Serializer->AddProperty(CDynaGenerator3C::m_cszxd2, xd2, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(CDynaGenerator3C::m_cszxq2, xq2, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(m_cszxl, xl, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(CDynaGenerator3C::m_csztd01, Td01, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(CDynaGenerator3C::m_csztd02, Td02, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(CDynaGenerator3C::m_csztq02, Tq02, eVARUNITS::VARUNIT_SECONDS);
}

void CDynaGeneratorPark3C::UpdateValidator(CSerializerValidatorRules* Validator)
{
	CDynaGeneratorDQBase::UpdateValidator(Validator);
	Validator->AddRule({ CDynaGenerator3C::m_cszxd2,
						 CDynaGenerator3C::m_cszxq2, 
						 m_cszxl, 
						 CDynaGenerator3C::m_csztd01,
						 CDynaGenerator3C::m_csztd02,
						 CDynaGenerator3C::m_csztq02 }, &CSerializerValidatorRules::BiggerThanZero);
}