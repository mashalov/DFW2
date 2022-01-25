#include "stdafx.h"
#include "DynaGenerator3C.h"
#include "DynaGeneratorPark3C.h"
#include "DynaModel.h"
#include "MathUtils.h"

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
	if (!CalculateFundamentalParameters(pDynaModel->Parameters().m_eParkParametersDetermination))
		return eDEVICEFUNCTIONSTATUS::DFS_FAILED;

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

bool CDynaGeneratorPark3C::CalculateFundamentalParameters(PARK_PARAMETERS_DETERMINATION_METHOD Method)
{
	bool bRes(true);
	lad = xd - xl;	laq = xq - xl;
	
	lrc = 0.0;


	double R1d(0), R1q(0), l1d(0), l1q(0);
		
	switch (Method)
	{
		case PARK_PARAMETERS_DETERMINATION_METHOD::Kundur:
		case PARK_PARAMETERS_DETERMINATION_METHOD::NiiptTo:
		case PARK_PARAMETERS_DETERMINATION_METHOD::NiiptToTd:
			bRes = GetAxisParametersNiipt(xd, xl, xd1, xd2, Tdo1, Tdo2, Rfd, lfd, R1d, l1d, Method) &&
				   GetAxisParametersNiipt(xq, xl, xq2, Tqo2, R1q, l1q);
			break;
		case PARK_PARAMETERS_DETERMINATION_METHOD::Canay:
			bRes = GetAxisParametersCanay(xd, xl, xd1, xd2, Tdo1, Tdo2, Rfd, lfd, R1d, l1d) &&
				   GetAxisParametersCanay(xq, xl, xq2, Tqo2, R1q, l1q);
			break;
	}

	if (!bRes)
		return bRes;
	
	CompareParksParameterCalculation();

	const double lFd(lad + lfd);		// сопротивление обмотки возбуждения
	const double l1D(lad + l1d);		// сопротивление демпферной обмотки d
	const double l1Q(laq + l1q);		// сопротивление первой демпферной обмотки q

	const double C(lad + lrc), A(C + lfd), B(C + l1d);
	double detd(C * C - A * B);

	if (Equal(detd, 0.0))
	{
		Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszCannotGetParkParameters, CDynaGeneratorDQBase::m_cszBadCoeficients, GetVerbalName(), detd));
		bRes = false;
	}
	if (Equal(l1Q, 0.0))
	{
		Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszCannotGetParkParameters, GetVerbalName(), "laq + l1q", l1Q));
		bRes = false;
	}

	if (bRes)
	{
		detd = 1.0 / detd;

		Ed_Psi1q = -laq / l1Q;

		Eq_Psifd = -lad * l1d * detd;
		Eq_Psi1d = -lad * lfd * detd;

		Psifd_Psifd = Rfd * B * detd;
		Psifd_Psi1d = -Rfd * C * detd;
		Psifd_id = -Rfd * lad * l1d * detd;

		Psi1d_Psifd = -R1d * C * detd;
		Psi1d_Psi1d = R1d * A * detd;
		Psi1d_id = -R1d * lad * lfd * detd;

		Psi1q_Psi1q = -R1q / l1Q;
		Psi1q_iq = R1q * laq / l1Q;

		lq2 = xq2;
		ld2 = xd2;

		Psid_Psifd = -lad * l1d * detd;
		Psid_Psi1d = -lad * lfd * detd;
		Psiq_Psi1q = laq / l1Q;
	}

	return bRes;
}

void CDynaGeneratorPark3C::BuildEquations(CDynaModel* pDynaModel)
{
	const double omega{ 1.0 + s };
	const double omega2{ omega * omega };
	const double zsq{ 1.0 / (r * r + omega2 * ld2 * lq2) };

	pDynaModel->SetElement(Id, Id, 1);
	// dId_dPsifd =  (Eq_Psifd * lq2 * (s + 1) ^ 2) / (r ^ 2 + ld2 * lq2 * (s + 1) ^ 2)
	pDynaModel->SetElement(Id, Psifd, Eq_Psifd * lq2 * omega2 * zsq);
	// dId_dPsi1d = (Eq_Psi1d * lq2 * (s + 1) ^ 2) / (r ^ 2 + ld2 * lq2 * (s + 1) ^ 2)
	pDynaModel->SetElement(Id, Psi1d, Eq_Psi1d * lq2 * omega2 * zsq);
	// dId_dPsi1q = -(Ed_Psi1q * r * (s + 1)) / (r ^ 2 + ld2 * lq2 * (s + 1) ^ 2)
	pDynaModel->SetElement(Id, Psi1q, -Ed_Psi1q * r * omega * zsq);
	// dId_dVd = r / (r ^ 2 + ld2 * lq2 * (s + 1) ^ 2)
	pDynaModel->SetElement(Id, Vd, r * zsq);
	// dId_dVq = -(lq2 * (s + 1)) / (r ^ 2 + ld2 * lq2 * (s + 1) ^ 2)
	pDynaModel->SetElement(Id, Vq, -lq2 * omega * zsq);
	// dId_ds = -(Vq * lq2 + Ed_Psi1q * Psi1q * r - Eq_Psi1d * Psi1d * lq2 * (2 * s + 2) - Eq_Psifd * Psifd * lq2 * (2 * s + 2)) / (r ^ 2 + ld2 * lq2 * (s + 1) ^ 2) - (ld2 * lq2 * (2 * s + 2) * (Vd * r - Vq * lq2 * (s + 1) - Ed_Psi1q * Psi1q * r * (s + 1) + Eq_Psi1d * Psi1d * lq2 * (s + 1) ^ 2 + Eq_Psifd * Psifd * lq2 * (s + 1) ^ 2)) / (r ^ 2 + ld2 * lq2 * (s + 1) ^ 2) ^ 2
	pDynaModel->SetElement(Id, s, -(Vq * lq2 + Ed_Psi1q * Psi1q * r - Eq_Psi1d * Psi1d * lq2 * 2.0 * omega - 2.0 * omega * Eq_Psifd * Psifd * lq2) * zsq - (2.0 * omega * ld2 * lq2 * (Vd * r - Vq * lq2 * omega - Ed_Psi1q * Psi1q * r * omega + Eq_Psi1d * Psi1d * lq2 * omega2 + Eq_Psifd * Psifd * lq2 * omega2)) * zsq * zsq);
	

	pDynaModel->SetElement(Iq, Iq, 1);
	// dIq_dPsifd = -(Eq_Psifd * r * (s + 1)) / (r ^ 2 + ld2 * lq2 * (s + 1) ^ 2)
	pDynaModel->SetElement(Iq, Psifd, -Eq_Psifd * r * omega * zsq);
	// dIq_dPsi1d = -(Eq_Psi1d * r * (s + 1)) / (r ^ 2 + ld2 * lq2 * (s + 1) ^ 2)
	pDynaModel->SetElement(Iq, Psi1d, -Eq_Psi1d * r * omega * zsq);
	// dIq_dPsi1q = -(Ed_Psi1q * ld2 * (s + 1) ^ 2) / (r ^ 2 + ld2 * lq2 * (s + 1) ^ 2)
	pDynaModel->SetElement(Iq, Psi1q, -Ed_Psi1q * ld2 *omega2 * zsq);
	// dIq_dVd = (ld2 * (s + 1)) / (r ^ 2 + ld2 * lq2 * (s + 1) ^ 2)
	pDynaModel->SetElement(Iq, Vd, ld2 * omega * zsq);
	// dIq_dVq = r / (r ^ 2 + ld2 * lq2 * (s + 1) ^ 2)
	pDynaModel->SetElement(Iq, Vq, r * zsq);
	// dIq_ds = (ld2 * lq2 * (2 * s + 2) * (Eq_Psi1d * Psi1d * r * (s + 1) - Vd * ld2 * (s + 1) - Vq * r + Eq_Psifd * Psifd * r * (s + 1) + Ed_Psi1q * Psi1q * ld2 * (s + 1) ^ 2)) / (r ^ 2 + ld2 * lq2 * (s + 1) ^ 2) ^ 2 - (Eq_Psi1d * Psi1d * r - Vd * ld2 + Eq_Psifd * Psifd * r + Ed_Psi1q * Psi1q * ld2 * (2 * s + 2)) / (r ^ 2 + ld2 * lq2 * (s + 1) ^ 2)
	pDynaModel->SetElement(Iq, s, (2.0 * ld2 * lq2 * omega * (Eq_Psi1d * Psi1d * r * omega - Vd * ld2 * omega - Vq * r + Eq_Psifd * Psifd * r * omega + Ed_Psi1q * Psi1q * ld2 * omega2)) * zsq * zsq - (Eq_Psi1d * Psi1d * r - Vd * ld2 + Eq_Psifd * Psifd * r + 2.0 * Ed_Psi1q * Psi1q * ld2 * omega) * zsq);

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
	// используем вариант уравнения движения с демпфированием от частоты напряжения
	CDynaGeneratorDQBase::BuildMotionEquationBlock(pDynaModel);
#else
	// используем стандартное уравнение движения с демпфированием только от
	// скольжения генератора. Момент рассчитывается по потокосцеплениям.
	pDynaModel->SetElement(s, Id, -(Psi1q * Psiq_Psi1q - Iq * ld2 + Iq * lq2) / Mj);
	pDynaModel->SetElement(s, Iq, (Psi1d * Psid_Psi1d + Psifd * Psid_Psifd + Id * ld2 - Id * lq2) / Mj);
	pDynaModel->SetElement(s, Psifd, Iq * Psid_Psifd / Mj);
	pDynaModel->SetElement(s, Psi1d, Iq * Psid_Psi1d / Mj);
	pDynaModel->SetElement(s, Psi1q, -Id * Psiq_Psi1q / Mj);
	pDynaModel->SetElement(s, s, -(Kdemp + Pt / omega / omega) / Mj);
	BuildAngleEquationBlock(pDynaModel);
#endif
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

void CDynaGeneratorPark3C::BuildRightHand(CDynaModel* pDynaModel)
{
	const double dEq{ Eq + (Psifd_Psifd * Psifd + Psifd_Psi1d * Psi1d + Psifd_id * Id) * lad / Rfd };
	cplx cI{ GetIdIq() };
	pDynaModel->SetFunction(Id, Id - cI.real());
	pDynaModel->SetFunction(Iq, Iq - cI.imag());
	pDynaModel->SetFunction(Eq, dEq);
	SetFunctionsDiff(pDynaModel);
	BuildRIfromDQRightHand(pDynaModel);
}

// Так как одни и те же производные нужны и для BuildRightHand и для BuildDerivatives
// они рассчитываются в CalcDerivatives, которая получает в качестве параметра функцию ввода значения.
// В зависимости от переданной функции значение производной попадает либо в правую часть уравнений, либо 
// в вектор производных
void CDynaGeneratorPark3C::CalculateDerivatives(CDynaModel* pDynaModel, CDevice::fnDerivative fn)
{
	if (IsStateOn())
	{
		const double omega(1.0 + s);
		const double dPsifd = Rfd * ExtEqe / lad + Psifd_Psifd * Psifd + Psifd_Psi1d * Psi1d + Psifd_id * Id;
		const double dPsi1d = Psi1d_Psifd * Psifd + Psi1d_Psi1d * Psi1d + Psi1d_id * Id;
		const double dPsi1q = Psi1q_Psi1q * Psi1q + Psi1q_iq * Iq;
#ifdef USE_VOLTAGE_FREQ_DAMPING
		// используем вариант уравнения движения с демпфированием от частоты напряжения
		CDynaGeneratorDQBase::CalculateDerivatives(pDynaModel, fn);
#else
		// используем стандартное уравнение движения с демпфированием только от
		// скольжения генератора. Момент рассчитывается по потокосцеплениям.
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

void CDynaGeneratorPark3C::BuildDerivatives(CDynaModel* pDynaModel)
{
	SetDerivatives(pDynaModel);
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
	Serializer->AddProperty(m_csztdo1, Tdo1, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(m_csztdo2, Tdo2, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(m_csztqo2, Tqo2, eVARUNITS::VARUNIT_SECONDS);
}

void CDynaGeneratorPark3C::UpdateValidator(CSerializerValidatorRules* Validator)
{
	CDynaGeneratorDQBase::UpdateValidator(Validator);
	Validator->AddRule({ CDynaGenerator3C::m_cszxd2,
						 CDynaGenerator3C::m_cszxq2, 
						 m_cszxl, 
						 CDynaGenerator3C::m_csztdo1,
						 CDynaGenerator3C::m_csztdo2,
						 CDynaGenerator3C::m_csztqo2 }, &CSerializerValidatorRules::BiggerThanZero);

	Validator->AddRule(m_csztdo1, &CDynaGeneratorDQBase::ValidatorTdo1);
	Validator->AddRule(m_cszxd, &CDynaGeneratorDQBase::ValidatorXd);
	Validator->AddRule(m_cszxq, &CDynaGeneratorDQBase::ValidatorXq);
	Validator->AddRule(m_cszxq, &CDynaGeneratorPark3C::ValidatorXqXq2);
	Validator->AddRule(m_cszxd1, &CDynaGeneratorPark3C::ValidatorXd1);
	Validator->AddRule(m_cszxl, &CDynaGeneratorDQBase::ValidatorXlXd);
	Validator->AddRule(m_cszxl, &CDynaGeneratorDQBase::ValidatorXlXq);
	Validator->AddRule(m_cszxl, &CDynaGeneratorDQBase::ValidatorXlXd2);
	Validator->AddRule(m_cszxl, &CDynaGeneratorDQBase::ValidatorXlXq2);
	Validator->AddRule(m_cszr, &CSerializerValidatorRules::NonNegative);
}

/*
// Canay 1983 d-axis

const double xc = xl;
const double xad(xd - xl);
const double xdc = xd - xc;
const double xdc2 = xd2 - xc;

double Td1 = Tdo1;
double Td2 = Tdo2;

if (CDynaGeneratorPark3C::GetShortCircuitTimeConstants(xd, xd1, xd2, Tdo1, Tdo2, Td1, Td2))
{
	double Tdc1(Td1), Tdc2(Td2);
	double Ac = xdc;
	double Bc = xc * (Tdo1 + Tdo2) - xd * (Td1 + Td2);
	double Cc = Tdo1 * Tdo2 * xdc2;
	MathUtils::CSquareSolver::RootsSortedByAbs(Ac, Bc, Cc, Tdc2, Tdc1);
	Tdc1 = Tdo1 * Tdo2 * xdc2 / xdc / Tdc2;
	const double xadxdc(xad / xdc);
	const double xrc((xc - xl)* xadxdc);
	const double xdc1 = xdc * (Tdc1 - Tdc2) / (Tdo1 + Tdo2 - (1 + xdc / xdc2) * Tdc2);
	CanayD2.l2 = xdc1 * xdc2 / (xdc1 - xdc2) * xadxdc * xadxdc;
	CanayD2.l1 = xdc * xdc1 / (xdc - xdc1) * xadxdc * xadxdc;
	CanayD2.r1 = CanayD2.l1 / Tdc1;
	CanayD2.r2 = CanayD2.l2 / Tdc2;
}
*/
