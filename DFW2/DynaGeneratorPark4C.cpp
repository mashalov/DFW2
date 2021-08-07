#include "stdafx.h"
#include "DynaGenerator3C.h"
#include "DynaGeneratorPark3C.h"
#include "DynaGeneratorPark4C.h"
#include "DynaModel.h"

using namespace DFW2;

eDEVICEFUNCTIONSTATUS CDynaGeneratorPark4C::PreInit(CDynaModel* pDynaModel)
{
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorPark4C::Init(CDynaModel* pDynaModel)
{
	return InitModel(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorPark4C::InitModel(CDynaModel* pDynaModel)
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
	Psi1q = Psi2q = laq * Iq;

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

double* CDynaGeneratorPark4C::GetVariablePtr(ptrdiff_t nVarIndex)
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

VariableIndexRefVec& CDynaGeneratorPark4C::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDynaGeneratorDQBase::GetVariables(JoinVariables({ Psifd, Psi1d, Psi1q, Psi2q }, ChildVec));
}

bool CDynaGeneratorPark4C::CalculateFundamentalParameters(PARK_PARAMETERS_DETERMINATION_METHOD Method)
{
	bool bRes(true);

	// взаимные индуктивности без рассеивания [3.111 и 3.112]
	lad = xd - xl; 
	laq = xq - xl;
	// сопротивление утечки обмотки возбуждения [4.29]
	double denom = lad - xd1 + xl;
	lfd = Equal(denom,0.0) ? 1E6 : lad * (xd1 - xl) / denom; 
	denom = laq - xq1 + xl;
	// сопротивление утечки первой демпферной обмотки q [4.33]
	double l1q(Equal(denom, 0.0) ?  1E6 :laq * (xq1 - xl) / denom);
	// сопротивление утечки демпферной обмотки d [4.28]
	denom = lad * lfd - (xd2 - xl) * (lfd + lad);
	double l1d(Equal(denom, 0.0) ? 1E6 : lad * lfd * (xd2 - xl) / denom );
	// сопротивление утечки второй демпферной обмотки q [4.32]
	denom = laq * l1q - (xq2 - xl) * ( l1q + laq );
	double l2q(Equal(denom, 0.0) ? 1E6 : laq * l1q * (xq2 - xl) / denom);

	lrc = 0.0;

	double lFd(lad + lfd);		// сопротивление обмотки возбуждения
	double l1D(lad + l1d);		// сопротивление демпферной обмотки d
	double l1Q(laq + l1q);		// сопротивление первой демпферной обмотки q
	double l2Q(laq + l2q);		// сопротивление второй демпферной обмотки q

	// активное сопротивление обмотки возбуждения [4.15]
	Rfd = lFd / Tdo1;
	// активное сопротивление демпферной обмотки d [4.15]
	double R1d = (lad * lfd / lFd + l1d) / Tdo2;	
	// активное сопротивление первой демпферной обмотки q [4.30]
	double R1q = l1Q / Tqo1;	
	// активное сопротивление второй демпферной обмотки q [4.31]
	double R2q = (laq * l1q / l1Q + l2q) / Tqo2;

	if (Method == PARK_PARAMETERS_DETERMINATION_METHOD::Niipt)
	{
		// методика НИИПТ сводится к расчету новых значений постоянных времени на ХХ
		double nTd01(Tdo1), nTd02(Tdo2);
		if (CDynaGeneratorPark3C::GetNIIPTTimeConstants(lad, lfd, l1d, nTd01, nTd02))
		{
			Rfd = lad / nTd01;
			R1d = (lad * lfd / lFd + l1d) / nTd02;
		}

		double nTqo1(Tqo1), nTqo2(Tqo2);
		if (CDynaGeneratorPark3C::GetNIIPTTimeConstants(laq, l1q, l2q, nTqo1, nTqo2))
		{
			R1q = l1Q / nTqo1;
			R2q = (laq * l1q / l1Q + l2q) / nTqo2;
		}
	}

	/*
	// Test
	xd = 1.77;
	xl = 0.17;
	xd1 = 0.329;
	xd2 = 0.253;
	Td01 = 0.859;
	Td02 = 0.0247;
	double Td1 = 0.859;
	double Td2 = 0.0247;
	Td01 = Td1 * xd / xd1;
	Td02 = Td2 * xd1 / xd2;
	*/

	// Методики Umans&Mallick и Canay дают фундаментальные параметры из стандартных

	struct ParkParameters 
	{
		double r1, l1, r2, l2;
	}
		NiiptD{ Rfd, lfd, R1d, l1d }, NiiptQ{ R1q, l1q, R2q, l2q },
		/*UmansD, UmansQ,*/
		CanayD, CanayQ;


	/*
	CDynaGeneratorPark3C::GetAxisParametersUmans(xd, xl, xd1, xd2, Tdo1, Tdo2, UmansD.r1, UmansD.l1, UmansD.r2, UmansD.l2);
	CDynaGeneratorPark3C::GetAxisParametersUmans(xq, xl, xq1, xq2, Tqo1, Tqo2, UmansQ.r1, UmansQ.l1, UmansQ.r2, UmansQ.l2);
	CDynaGeneratorPark3C::GetAxisParametersCanay(xd, xl, xd1, xd2, Tdo1, Tdo2, CanayD.r1, CanayD.l1, CanayD.r2, CanayD.l2);
	CDynaGeneratorPark3C::GetAxisParametersCanay(xq, xl, xq1, xq2, Tqo1, Tqo2, CanayQ.r1, CanayQ.l1, CanayQ.r2, CanayQ.l2);

	std::array<ParkParameters, 6> Axes = { NiiptD, UmansD, CanayD, NiiptQ, UmansQ, CanayQ };
	std::array<std::string, 6> AxesNames = { "NiiptD", "UmansD", "CanayD", "NiiptQ", "UmansQ", "CanayQ" };
	for (size_t j = 0 ; j < Axes.size() ; j++)
	{
		//std::array<std::string, 4> names = {"r1", "l1", "r2", "l2"};
		std::array<const double*, 4> ptr = { &Axes[j].r1, &Axes[j].l1, &Axes[j].r2, &Axes[j].l2 };
		std::string res(AxesNames[j]);
		for (size_t i = 0 ; i < ptr.size() ; i++)
			res += fmt::format(";{}", *ptr[i]);
		Log(DFW2MessageStatus::DFW2LOG_DEBUG, res);
	}
	*/

	if (Method == PARK_PARAMETERS_DETERMINATION_METHOD::Canay)
	{
		bRes = false;
		double Td1(Tdo1), Td2(Tdo2);
		if (GetShortCircuitTimeConstants_d(Td1, Td2))
		{
			if (CDynaGeneratorPark3C::GetAxisParametersCanay(xd, xl, xd1, xd2, Td1, Td2, CanayD.r1, CanayD.l1, CanayD.r2, CanayD.l2))
				bRes = true;
		}
		if (bRes)
		{
			bRes = false;
			double Tq1(Tqo1), Tq2(Tqo2);
			if (GetShortCircuitTimeConstants_q(Tq1, Tq2))
			{
				if (CDynaGeneratorPark3C::GetAxisParametersCanay(xq, xl, xq1, xq2, Tq1, Tq2, CanayQ.r1, CanayQ.l1, CanayQ.r2, CanayQ.l2))
					bRes = true;
			}
		}
	}


	lFd = (lad + lfd);		// сопротивление обмотки возбуждения
	l1D = (lad + l1d);		// сопротивление демпферной обмотки d
	l1Q = (laq + l1q);		// сопротивление первой демпферной обмотки q
	l2Q = (laq + l2q);		// сопротивление второй демпферной обмотки q


	const double C(lad + lrc), A(C + lfd), B(C + l1d);
	const double& D(l1Q), &F(l2Q);
	double detd(C * C - A * B), detq(laq * laq - D * F);

	if (Equal(detd, 0.0))
	{
		Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszCannotGetParkParametersForAxisd, GetVerbalName(), detd));
		bRes = false;
	}
	if (Equal(detq, 0.0))
	{
		Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszCannotGetParkParametersForAxisq, GetVerbalName(), detq));
		bRes = false;
	}

	if (bRes)
	{
		detd = 1.0 / detd;	detq = 1.0 / detq;

		Ed_Psi1q = laq * l2q * detq;
		Ed_Psi2q = laq * l1q * detq;

		Eq_Psifd = -lad * l1d * detd;
		Eq_Psi1d = -lad * lfd * detd;

		Psifd_Psifd = Rfd * B * detd;
		Psifd_Psi1d = -Rfd * C * detd;
		Psifd_id = -Rfd * lad * l1d * detd;

		Psi1d_Psifd = -R1d * C * detd;
		Psi1d_Psi1d = R1d * A * detd;
		Psi1d_id = -R1d * lad * lfd * detd;

		Psi1q_Psi1q = R1q * F * detq;
		Psi1q_Psi2q = -R1q * laq * detq;
		Psi1q_iq = -R1q * laq * l2q * detq;

		Psi2q_Psi1q = -R2q * laq * detq;
		Psi2q_Psi2q = R2q * D * detq;
		Psi2q_iq = -R2q * laq * l1q * detq;

		lq2 = xq2;
		ld2 = xd2;

		Psid_Psifd = -lad * l1d * detd;
		Psid_Psi1d = -lad * lfd * detd;
		Psiq_Psi1q = -laq * l2q * detq;
		Psiq_Psi2q = -laq * l1q * detq;
	}

	return bRes;
}

void CDynaGeneratorPark4C::CalculateDerivatives(CDynaModel* pDynaModel, CDevice::fnDerivative fn)
{
	if (IsStateOn())
	{
		const double omega(1.0 + s);
		const double dPsifd = Rfd * ExtEqe / lad + Psifd_Psifd * Psifd + Psifd_Psi1d * Psi1d + Psifd_id * Id;
		const double dPsi1d = Psi1d_Psifd * Psifd + Psi1d_Psi1d * Psi1d + Psi1d_id * Id;
		const double dPsi1q = Psi1q_Psi1q * Psi1q + Psi1q_Psi2q * Psi2q + Psi1q_iq * Iq;
		const double dPsi2q = Psi2q_Psi1q * Psi1q + Psi2q_Psi2q * Psi2q + Psi2q_iq * Iq;

#ifdef USE_VOLTAGE_FREQ_DAMPING
		CDynaGeneratorDQBase::CalculateDerivatives(pDynaModel, fn);
#else
		const double Te = (ld2 * Id + Psid_Psifd * Psifd + Psid_Psi1d * Psi1d) * Iq - (lq2 * Iq + Psiq_Psi1q * Psi1q + Psiq_Psi2q * Psi2q) * Id;
		(pDynaModel->*fn)(Delta, pDynaModel->GetOmega0() * s);
		(pDynaModel->*fn)(s, (Pt / omega - Kdemp * s - Te) / Mj);
#endif
		(pDynaModel->*fn)(Psifd, dPsifd);
		(pDynaModel->*fn)(Psi1d, dPsi1d);
		(pDynaModel->*fn)(Psi1q, dPsi1q);
		(pDynaModel->*fn)(Psi2q, dPsi2q);
	}
	else
	{
		(pDynaModel->*fn)(Delta, 0);
		(pDynaModel->*fn)(s, 0);
		(pDynaModel->*fn)(Psifd, 0);
		(pDynaModel->*fn)(Psi1d, 0);
		(pDynaModel->*fn)(Psi1q, 0);
		(pDynaModel->*fn)(Psi2q, 0);
	}
}

bool CDynaGeneratorPark4C::BuildEquations(CDynaModel* pDynaModel)
{
	bool bRes(true);

	const double omega = ZeroGuardSlip(1.0 + s);

	pDynaModel->SetElement(Id, Id, -r);
	pDynaModel->SetElement(Id, Iq, -lq2 * omega);
	pDynaModel->SetElement(Id, Vd, -1);
	pDynaModel->SetElement(Id, Psi1q, Ed_Psi1q * omega);
	pDynaModel->SetElement(Id, Psi2q, Ed_Psi2q * omega);
	pDynaModel->SetElement(Id, s, Ed_Psi1q * Psi1q - Iq * lq2 + Ed_Psi2q * Psi2q);

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
	pDynaModel->SetElement(Psi1q, Psi2q, -Psi1q_Psi2q);
		
	pDynaModel->SetElement(Psi2q, Iq, -Psi2q_iq);
	pDynaModel->SetElement(Psi2q, Psi1q, -Psi2q_Psi1q);
	pDynaModel->SetElement(Psi2q, Psi2q, Psi2q_Psi2q);

	pDynaModel->SetElement(Eq, Eq, 1);
	pDynaModel->SetElement(Eq, Psifd, Psifd_Psifd * lad / Rfd);
	pDynaModel->SetElement(Eq, Psi1d, Psifd_Psi1d * lad / Rfd);
	pDynaModel->SetElement(Eq, Id, Psifd_id * lad / Rfd);

	BuildRIfromDQEquations(pDynaModel);

#ifdef USE_VOLTAGE_FREQ_DAMPING
	CDynaGeneratorDQBase::BuildMotionEquationBlock(pDynaModel);
#else
	pDynaModel->SetElement(s, Id, -(Psi1q * Psiq_Psi1q + Psi2q * Psiq_Psi2q - Iq * ld2 + Iq * lq2) / Mj);
	pDynaModel->SetElement(s, Iq, (Psi1d * Psid_Psi1d + Psifd * Psid_Psifd + Id * ld2 - Id * lq2) / Mj);
	pDynaModel->SetElement(s, Psifd, Iq * Psid_Psifd / Mj);
	pDynaModel->SetElement(s, Psi1d, Iq * Psid_Psi1d / Mj);
	pDynaModel->SetElement(s, Psi1q, -Id * Psiq_Psi1q / Mj);
	pDynaModel->SetElement(s, Psi2q, -Id * Psiq_Psi2q / Mj);
	pDynaModel->SetElement(s, s, -(Kdemp + Pt / omega / omega) / Mj);
	BuildAngleEquationBlock(pDynaModel);
#endif
	return bRes;
}

cplx CDynaGeneratorPark4C::GetIdIq() const
{
	const double omega = ZeroGuardSlip(1.0 + s);
	const double omega2 = omega * omega;
	const double zsq = ZeroDivGuard(1.0, r * r + omega2 * ld2 * lq2);

	const double id = zsq * (
		-r * Vd
		+ omega * lq2 * Vq
		- omega2 * lq2 * Eq_Psifd * Psifd
		- omega2 * lq2 * Eq_Psi1d * Psi1d
		+ r * omega * Ed_Psi1q * Psi1q
		+ r * omega * Ed_Psi2q * Psi2q
		);

	const double iq = zsq * (
		-r * Vq
		- omega * ld2 * Vd
		+ r * omega * Eq_Psifd * Psifd
		+ r * omega * Eq_Psi1d * Psi1d
		+ omega2 * ld2 * Ed_Psi1q * Psi1q
		+ omega2 * ld2 * Ed_Psi2q * Psi2q
		);

	return { id,iq };
}

bool CDynaGeneratorPark4C::BuildRightHand(CDynaModel* pDynaModel)
{
	bool bRes(true);

	const double omega(1.0 + s);
	const double dEq = Eq + (Psifd_Psifd * Psifd + Psifd_Psi1d * Psi1d + Psifd_id * Id) * lad / Rfd;
	const double dId = -r * Id - omega * lq2 * Iq + omega * Ed_Psi1q * Psi1q + omega * Ed_Psi2q * Psi2q - Vd;
	const double dIq = -r * Iq + omega * ld2 * Id + omega * Eq_Psifd * Psifd + omega * Eq_Psi1d * Psi1d - Vq;
	pDynaModel->SetFunction(Id, dId);
	pDynaModel->SetFunction(Iq, dIq);
	pDynaModel->SetFunction(Eq, dEq);
	SetFunctionsDiff(pDynaModel);
	BuildRIfromDQRightHand(pDynaModel);
	return bRes;
}


bool CDynaGeneratorPark4C::BuildDerivatives(CDynaModel* pDynaModel)
{
	SetDerivatives(pDynaModel);
	return true;
}


eDEVICEFUNCTIONSTATUS CDynaGeneratorPark4C::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes = CDynaGeneratorDQBase::ProcessDiscontinuity(pDynaModel);
	if (CDevice::IsFunctionStatusOK(eRes) && IsStateOn())
	{
		CalculatePower();
		Eq = -lad * (Psifd_Psifd * Psifd + Psifd_Psi1d * Psi1d + Psifd_id * Id) / Rfd;
	}
	return eRes;
}

bool CDynaGeneratorPark4C::CalculatePower()
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

cplx CDynaGeneratorPark4C::GetEMF()
{
	cplx emf{ Eq_Psifd * Psifd + Eq_Psi1d * Psi1d, Ed_Psi1q * Psi1q + Ed_Psi2q * Psi2q };
	emf *= (1.0 + s);
	emf *= std::polar(1.0, Delta.Value);
	return emf;
}

const cplx& CDynaGeneratorPark4C::CalculateEgen()
{

	double xgen = Zgen().imag();
	cplx emf(GetEMF() * std::polar(1.0, -Delta.Value));
	const double omega = ZeroGuardSlip(1.0 + Sv);
	return m_Egen = cplx(emf.real() - omega * Id * (xgen - ld2),  emf.imag() - omega * Iq * (lq2 - xgen)) * std::polar(1.0, (double)Delta);
}

void CDynaGeneratorPark4C::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaGeneratorDQBase::DeviceProperties(props);
	props.SetType(DEVTYPE_GEN_PARK4C);
	props.SetClassName(CDeviceContainerProperties::m_cszNameGeneratorPark4C, CDeviceContainerProperties::m_cszSysNameGeneratorPark4C);
	props.nEquationsCount = CDynaGeneratorPark4C::VARS::V_LAST;
	props.m_VarMap.insert(std::make_pair(m_cszPsifd, CVarIndex(V_PSI_FD, eVARUNITS::VARUNIT_WB)));
	props.m_VarMap.insert(std::make_pair(m_cszPsi1d, CVarIndex(V_PSI_1D, eVARUNITS::VARUNIT_WB)));
	props.m_VarMap.insert(std::make_pair(m_cszPsi1q, CVarIndex(V_PSI_1Q, eVARUNITS::VARUNIT_WB)));
	props.m_VarMap.insert(std::make_pair(m_cszPsi2q, CVarIndex(V_PSI_2Q, eVARUNITS::VARUNIT_WB)));
	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaGeneratorPark4C>>();
}

void CDynaGeneratorPark4C::UpdateSerializer(CSerializerBase* Serializer)
{
	// обновляем сериализатор базового класса
	CDynaGeneratorDQBase::UpdateSerializer(Serializer);

	Serializer->AddState(m_cszPsifd, Psifd, eVARUNITS::VARUNIT_WB);
	Serializer->AddState(m_cszPsi1d, Psi1d, eVARUNITS::VARUNIT_WB);
	Serializer->AddState(m_cszPsi1q, Psi1q, eVARUNITS::VARUNIT_WB);
	Serializer->AddState(m_cszPsi2q, Psi2q, eVARUNITS::VARUNIT_WB);
	Serializer->AddProperty(m_cszxd2, xd2, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(m_cszxq1, xq1, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(m_cszxq2, xq2, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(m_cszxl, xl, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(m_csztdo1, Tdo1, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(m_csztdo2, Tdo2, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(m_csztqo1, Tqo1, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(m_csztqo2, Tqo2, eVARUNITS::VARUNIT_SECONDS);
}

void CDynaGeneratorPark4C::UpdateValidator(CSerializerValidatorRules* Validator)
{
	CDynaGeneratorDQBase::UpdateValidator(Validator);
	Validator->AddRule({ m_cszxd2,
						 m_cszxq1,
						 m_cszxq2,
						 m_cszxl,
						 m_csztdo1,
						 m_csztdo2,
						 m_csztqo1,
						 m_csztqo2 }, &CSerializerValidatorRules::BiggerThanZero);

	Validator->AddRule(m_csztdo1, &CDynaGeneratorDQBase::ValidatorTdo1);
	Validator->AddRule(m_csztqo1, &CDynaGeneratorDQBase::ValidatorTqo1);
}