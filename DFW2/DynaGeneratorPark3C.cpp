﻿#include "stdafx.h"
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

	double nTd01(Td01), nTd02(Td02);
	if (CDynaGeneratorPark3C::GetNIIPTTimeConstants(lad, lfd, l1d, nTd01, nTd02))
	{
		Rfd = lFd / nTd01;
		R1d = l1D / nTd02;
	}

	/*
	double cTd01(Td01), cTd02(Td02);
	if (CDynaGeneratorPark3C::GetCanayTimeConstants(lad, lfd, l1d, lrc, cTd01, cTd02))
	{
		Rfd = lad / cTd01;
		R1d = (lad * lfd / lFd + l1d) / cTd02;
	}
	*/

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
	Serializer->AddProperty(m_csztd01, Td01, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(m_csztd02, Td02, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(m_csztq02, Tq02, eVARUNITS::VARUNIT_SECONDS);
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

	Validator->AddRule(m_csztd01, &CDynaGeneratorDQBase::ValidatorTd01);
	Validator->AddRule(m_cszxd, &CDynaGeneratorDQBase::ValidatorXd);
	Validator->AddRule(m_cszxq, &CDynaGeneratorDQBase::ValidatorXq);
	//Validator->AddRule(m_cszxq1, &CDynaGeneratorDQBase::ValidatorXq1);
	Validator->AddRule(m_cszxd1, &CDynaGeneratorPark3C::ValidatorXd1);
	Validator->AddRule(m_cszxl, &CDynaGeneratorDQBase::ValidatorXlXd);
	Validator->AddRule(m_cszxl, &CDynaGeneratorDQBase::ValidatorXlXq);
	Validator->AddRule(m_cszr, &CSerializerValidatorRules::NonNegative);
}


bool CDynaGeneratorPark3C::GetCanayTimeConstants(double Xa, double X1s, double X2s, double Xrc, double& T1, double& T2)
{
	auto K12 = [](double Xa, double X1s, double X2s, double X12)
	{
		std::optional<double> ret;
		const double Mult((Xa + X1s) * (Xa + X2s));
		if (!Equal(Mult, 0.0))
			ret = 1.0 / Mult * (Mult - X12 * X12);
		return ret;
	};

	if (const auto k12(K12(Xa, Xrc + X1s, Xrc + X2s, Xa + Xrc)); k12.has_value())
	{
		const double A0(T1 + T2), B0(k12.value() * T1 * T2);
		double r1(0.0), r2(0.0);
		if (MathUtils::CSquareSolver::Roots(B0, A0, 1.0, r1, r2))
		{
			if (std::abs(r1) > std::abs(r2))
				std::swap(r1, r2);

			T1 = -1.0 / r1;
			T2 = -1.0 / r2;

			return true;
		}
	}
	return false;
}

// Пересчет постоянных времени им. НИИПТ
bool CDynaGeneratorPark3C::GetNIIPTTimeConstants(double Xa, double X1s, double X2s, double& T1, double& T2)
{
	const double Ts(T1 + T2), det(0.25 * Ts * Ts - T1 * T2 / (1.0 - Xa * Xa / (Xa + X1s) / (Xa + X2s)));
	if (det >= 0)
	{
		T1 = 0.5 * Ts + std::sqrt(det);
		T2 = Ts - T1;
		return true;
	}
	return false;
}

// Расчет фундаментальных  параметров Umans-Mallick 
// D.Umans, J.A.Mallickand G.L.Wilson, "Modeling of Solid Rotor Turbo-Generators - Part II - Example of Model Derivation and Use in Digital Simulation", 
// IEEE Transaction on PAS, vol.PAS - 97, no. 1, Jan. / Feb. 1978.

bool CDynaGeneratorPark3C::GetAxisParametersUmans(double Xd,
	double Xl,
	double X1,
	double X2,
	double Td01,
	double Td02,
	double& r1,
	double& l1,
	double& r2,
	double& l2)
{
	double Td1(Td01 * X1 / Xd);
	double Td2(Td02 * X2 / X1);


	CDynaGeneratorPark3C::GetShortCircuitTimeConstants(Xd, X1, X2, Td01, Td02, Td1, Td2);


	const double Md(Xd - Xl);
	const double Tdiff = (Td01 + Td02 - Td1 - Td2);
	const double A = Md * Md / (Xd * Tdiff);
	const double a = (Xd * (Td1 + Td2) - Xl * (Td01 + Td02)) / Md;
	const double b = (Xd * Td1 * Td2 - Xl * Td01 * Td02) / Md;
	const double c = (Td01 * Td02 - Td1 * Td2) / Tdiff;
	double det = a * a - 4.0 * b;
	if (det >= 0.0)
	{
		det = std::sqrt(det);
		r1 = 2.0 * A * det / (a - 2.0 * c + det);
		l1 = r1 * (a + det) / 2.0;
		r2 = 2.0 * A * det / (2 * c - a + det);
		l2 = r2 * (a - det) / 2.0;

		// в качестве параметров обмотки возбуждения принимаем
		// параметры с наибольшей постоянной времени. Должно использоваться
		// для оси d, но здесь этот же тест используется и для оси q
		if (std::abs(Xd / r1) < std::abs(Xd / r2))
		{
			std::swap(r1, r2);
			std::swap(l1, l2);
		}

		return true;
	}
	return false;
}

// Расчет фундаментальных параметров Canay
// I.M. Canay "Modelling of alternating-current machines having multiple rotor circuits", 
// IEEE Transaction on Energy Conversion, Vol. 9, No. 2, June 1993

bool CDynaGeneratorPark3C::GetAxisParametersCanay(double Xd, 
	double Xl, 
	double X1, 
	double X2, 
	double Td01, 
	double Td02, 
	double& r1, 
	double& l1, 
	double& r2, 
	double& l2)
{
	double Td1(Td01 * X1 / Xd);
	double Td2(Td02 * X2 / X1);


	CDynaGeneratorPark3C::GetShortCircuitTimeConstants(Xd, X1, X2, Td01, Td02, Td1, Td2);

	const double A0 = Xd / X1 * Td1 + (Xd / X2 - Xd / X1 + 1) * Td2;
	const double B0 = Xd / X2 * Td1 * Td2;
	double root1(0), root2(0);
	if (MathUtils::CSquareSolver::RootsSortedByAbs(B0, A0, 1, root1, root2))
	{
		Td01 = -1 / root1;
		Td02 = -1 / root2;

		const double Xe(-Xl);
		const double A = Td1 + Td2;
		const double Ae = 1 / (Xd + Xe) * (Xd * A + Xe * A0);
		const double Be = (X2 + Xe) / (Xd + Xe) * B0;

		if (MathUtils::CSquareSolver::RootsSortedByAbs(Be, Ae, 1, root1, root2))
		{
			const double Tde01 = -1 / root1;
			const double Tde02 = -1 / root2;
			const double xde1 = (Xd + Xe) / (1 - (Tde01 - Td01) * (Tde01 - Td02) / (Tde01 * (Tde01 - Tde02)));
			const double xde2 = (Xd + Xe) * Tde01 * Tde02 / Td01 / Td02;
			const double deltay1 = 1 / xde1 - 1 / (Xd + Xe);
			const double deltay2 = 1 / xde2 - 1 / xde1;
			l1 = 1 / deltay1;	
			l2 = 1 / deltay2;
			r1 = l1 / Tde01;
			r2 = l2 / Tde02;
			return true;
		}
	}
	return false;
}

// Расчет постоянных времени КЗ из постоянных времени ХХ
// I.M. Canay, Determination of model parameters of synchronous machines
// IEEPROC, Vol. 130, Pt. B, No. 2, MARCH 1983

bool CDynaGeneratorPark3C::GetShortCircuitTimeConstants(double Xd, double X1, double X2, double Td01, double Td02, double& Td1, double& Td2)
{
	const double A (1 - Xd / X1 + Xd / X2);
	const double B(-Td01 - Td02);
	const double C(Td01 * Td02 * X2 / X1);
	// мы ожидаем что Td2 < Td1, поэтому берем первый корень из
	// отсортированных по модулю по возрастанию в качестве T2
	if (MathUtils::CSquareSolver::RootsSortedByAbs(A, B, C, Td2, Td1))
	{
		Td1 = Td01 * Td02 * X2 / Td2 / Xd;
		// проверяем
		_ASSERTE(Equal(Td01 + Td02 - Xd / X1 * Td1 - (1 - Xd / X1 + Xd / X2) * Td2, 0.0));
		_ASSERTE(Equal(Td01 * Td02 - Td1 * Td2 * Xd / X2, 0.0));
		return true;
	}
	else
		return false;
}