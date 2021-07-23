#include "stdafx.h"
#include "DynaGenerator3C.h"
#include "DynaGeneratorPark3C.h"
#include "DynaModel.h"

using namespace DFW2;

eDEVICEFUNCTIONSTATUS CDynaGeneratorPark3C::Init(CDynaModel* pDynaModel)
{
	r = 0;
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
	Eq = ifd;
	//const double Mdv(xd - xl);
	ExtEqe = Rfd * ifd;								// (9)
	Psifd = lad * Id + (lad + lrc + lfd) * ifd;		// (2)
	Psi1d = lad * Id + (lad + lrc) * ifd;			// (3)
	Psi1q = Psi2q = laq * Iq;

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
	lad = xd - xl; 
	laq = xq - xl;
	// сопротивление утечки обмотки возбуждения [4.29]
	double denom = lad - xd1 + xl;
	lfd = Equal(denom,0.0) ? 1E6 : lad * (xd1 - xl) / denom; 
	denom = laq - xq1 + xl;
	// сопротивление утечки первой демпферной обмотки q [4.33]
	const double l1q(Equal(denom, 0.0) ?  1E6 :laq * (xq1 - xl) / denom);
	// сопротивление утечки демпферной обмотки d [4.28]
	denom = lad * lfd - (xd2 - xl) * (lfd + lad);
	const double l1d(Equal(denom, 0.0) ? 1E6 : lad * lfd * (xd2 - xl) / denom );
	// сопротивление утечки второй демпферной обмотки q [4.32]
	denom = laq * l1q - (xq2 - xl) * ( l1q + laq );
	const double l2q(Equal(denom, 0.0) ? 1E6 : laq * l1q * (xq2 - xl) / denom);


	lrc = 0.0;

	const double lFd(lad + lfd);		// сопротивление обмотки возбуждения
	const double l1D(lad + l1d);		// сопротивление демпферной обмотки d
	const double l1Q(laq + l1q);		// сопротивление первой демпферной обмотки q
	const double l2Q(laq + l2q);		// сопротивление второй демпферной обмотки q

	// активное сопротивление обмотки возбуждения [4.15]
	Rfd = lFd / Td01;
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

	Ed_Psi1q	= laq * l2q * detq;
	Ed_Psi2q	= laq * l1q * detq;

	Eq_Psifd	= -lad * l1d * detd;
	Eq_Psi1d	= -lad * lfd * detd;

	Psifd_Psifd	=  Rfd * B * detd;
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

	const double omega = ZeroGuardSlip(1.0 + s);
	const double omega2 = omega * omega;
	const double zsq = ZeroDivGuard(1.0, r * r + omega2 * ld2 * lq2);



	// dId / dId
	pDynaModel->SetElement(Id, Id, 1);
	// dId / dVd
	pDynaModel->SetElement(Id, Vd, r * zsq);
	// dId / dVq
	pDynaModel->SetElement(Id, Vq, -lq2 * omega * zsq);
	// dId / dPsifd
	pDynaModel->SetElement(Id, Psifd, Eq_Psifd * lq2 * omega2 * zsq);
	// dId / dPsi1d
	pDynaModel->SetElement(Id, Psi1d, Eq_Psi1d * lq2 * omega2 * zsq);
	// dId / dPsi1q
	pDynaModel->SetElement(Id, Psi1q, -Ed_Psi1q * r * omega * zsq);
	// dId / dPsi2q
	pDynaModel->SetElement(Id, Psi2q, -Ed_Psi2q * r * omega * zsq);
	// dId / ds
	pDynaModel->SetElement(Id, s,
		-(Vq * lq2 + Ed_Psi1q * Psi1q * r + Ed_Psi2q * Psi2q * r - 2.0 * Eq_Psi1d * Psi1d * lq2 * omega - 2.0 * Eq_Psifd * Psifd * lq2 * (omega)) * zsq
		- (2.0 * ld2 * lq2 * omega * (Vd * r - Vq * lq2 * omega - Ed_Psi1q * Psi1q * r * omega - Ed_Psi2q * Psi2q * r * omega + Eq_Psi1d * Psi1d * lq2 * omega2 + Eq_Psifd * Psifd * lq2 * omega2)) * zsq * zsq);

	// dIq / dIq
	pDynaModel->SetElement(Iq, Iq, 1);
	// dIq / dVd
	pDynaModel->SetElement(Iq, Vd, ld2 * omega * zsq);
	// dIq / dVd
	pDynaModel->SetElement(Iq, Vq, r *  zsq);
	// dIq / dPsifd
	pDynaModel->SetElement(Iq, Psifd, -Eq_Psifd * r * omega * zsq);
	// dIq / dPsi1d
	pDynaModel->SetElement(Iq, Psi1d, -Eq_Psi1d * r * omega * zsq);
	// dIq / dPsi1q
	pDynaModel->SetElement(Iq, Psi1q, -Ed_Psi1q * ld2 * omega2 * zsq);
	// dIq / dPsi2q
	pDynaModel->SetElement(Iq, Psi2q, -Ed_Psi2q * ld2 * omega2 * zsq);
	// dIq / ds
	pDynaModel->SetElement(Iq, s, 
			(2.0 * ld2 * lq2 * omega * (Eq_Psi1d * Psi1d * r * omega - Vd * ld2 * omega - Vq * r + Eq_Psifd * Psifd * r * omega + Ed_Psi1q * Psi1q * ld2 * omega2 + Ed_Psi2q * Psi2q * ld2 * omega2)) * zsq * zsq 
			- (Eq_Psi1d * Psi1d * r - Vd * ld2 + Eq_Psifd * Psifd * r + 2.0 * Ed_Psi1q * Psi1q * ld2 * omega + 2.0 * Ed_Psi2q * Psi2q * ld2 * omega) * zsq);

	// ds / dId
	pDynaModel->SetElement(s, Id,  -(Psi1q * Psiq_Psi1q + Psi2q * Psiq_Psi2q - Iq * Psid_id + Iq * Psiq_iq) / Mj);
	// ds / dIq
	pDynaModel->SetElement(s, Iq, (Psi1d * Psid_Psi1d + Psifd * Psid_Psifd + Id * Psid_id - Id * Psiq_iq) / Mj);
	// ds / Psifd
	pDynaModel->SetElement(s, Psifd, Iq * Psid_Psifd / Mj);
	// ds / Psi1d
	pDynaModel->SetElement(s, Psi1d, Iq * Psid_Psi1d / Mj);
	// ds / Psi1q
	pDynaModel->SetElement(s, Psi1q, -Id * Psiq_Psi1q / Mj);
	// ds / Psi2q
	pDynaModel->SetElement(s, Psi2q, -Id * Psiq_Psi2q / Mj);
	// ds / ds
	pDynaModel->SetElement(s, s, (Kdemp + Pt / omega2) / Mj);

	// dDeltaG / dS
	pDynaModel->SetElement(Delta, s, -pDynaModel->GetOmega0());
	// dDeltaG / dDeltaG
	pDynaModel->SetElement(Delta, Delta, 1.0);

	// dP/dP
	pDynaModel->SetElement(P, P, 1.0);
	// dP/dVd
	pDynaModel->SetElement(P, Vd, -Id);
	// dP/dVq
	pDynaModel->SetElement(P, Vq, -Iq);
	// dP/dId
	pDynaModel->SetElement(P, Id, -Vd);
	// dP/dIq
	pDynaModel->SetElement(P, Iq, -Vq);

	
	// dQ/dQ
	pDynaModel->SetElement(Q, Q, 1.0);
	// dQ/dVd
	pDynaModel->SetElement(Q, Vd, -Iq);
	// dQ/dVq
	pDynaModel->SetElement(Q, Vq, Id);
	// dQ/dId
	pDynaModel->SetElement(Q, Id, Vq);
	// dQ/dIq
	pDynaModel->SetElement(Q, Iq, -Vd);

	// d_Psifd / dId
	pDynaModel->SetElement(Psifd, Id, -Psifd_id);
	// d_Psifd / dPsifd
	pDynaModel->SetElement(Psifd, Psifd, Psifd_Psifd);
	// d_Psifd / dPsi1d
	pDynaModel->SetElement(Psifd, Psi1d, -Psifd_Psi1d);
	// d_Psifd/ dExtEque
	if (ExtEqe.Indexed())
		pDynaModel->SetElement(Psifd, ExtEqe, -1.0);

	// d_Psi1d / dId
	pDynaModel->SetElement(Psi1d, Id, -Psi1d_id);
	// d_Psi1d / dPsifd
	pDynaModel->SetElement(Psi1d, Psifd, -Psi1d_Psifd);
	// d_Psi1d / dPsi1d
	pDynaModel->SetElement(Psi1d, Psi1d, Psi1d_Psi1d);

	// d_Psi1q / dIq
	pDynaModel->SetElement(Psi1q, Iq, -Psi1q_iq);
	// d_Psi1q / dPsi1q
	pDynaModel->SetElement(Psi1q, Psi1q, Psi1q_Psi1q);
	// d_Psi1q / dPsi2q
	pDynaModel->SetElement(Psi1q, Psi2q, -Psi1q_Psi2q);

	// d_Psi2q / dIq
	pDynaModel->SetElement(Psi2q, Iq, -Psi2q_iq);
	// d_Psi2q / dPsi1q
	pDynaModel->SetElement(Psi2q, Psi1q, -Psi2q_Psi1q);
	// d_Psi2q / dPsi2q
	pDynaModel->SetElement(Psi2q, Psi2q, Psi1q_Psi2q);

	bRes = bRes && BuildRIfromDQEquations(pDynaModel);

	pDynaModel->SetElement(Eq, Eq, 1);

	return bRes;
}

cplx CDynaGeneratorPark3C::GetIdIq() const
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

bool CDynaGeneratorPark3C::BuildRightHand(CDynaModel* pDynaModel)
{
	bool bRes(true);

	const double dPsifd = ExtEqe + Psifd_Psifd * Psifd + Psifd_Psi1d * Psi1d + Psifd_id * Id;
	const double dPsi1d = Psi1d_Psifd * Psifd + Psi1d_Psi1d * Psi1d + Psi1d_id * Id;
	const double dPsi1q = Psi1q_Psi1q * Psi1q + Psi1q_Psi2q * Psi2q + Psi1q_iq * Iq;
	const double dPsi2q = Psi2q_Psi1q * Psi1q + Psi2q_Psi2q * Psi2q + Psi2q_iq * Iq;

	const cplx idiq(GetIdIq());
	const double omega = ZeroGuardSlip(1.0 + s);


	const double psid = Psid_id * Id + Psid_Psifd * Psifd + Psid_Psi1d * Psi1d;
	const double psiq = Psiq_iq * Iq + Psiq_Psi1q * Psi1q + Psiq_Psi2q * Psi2q;

	const double Te = (Psid_id * Id + Psid_Psifd * Psifd + Psid_Psi1d * Psi1d) * Iq
					 -(Psiq_iq * Iq + Psiq_Psi1q * Psi1q + Psiq_Psi2q * Psi2q) * Id;


	double eDelta = pDynaModel->GetOmega0() * s;
	double eS = (Pt / omega - Kdemp * s - Te) / Mj;

	pDynaModel->SetFunction(P, P - Vd * Id - Vq * Iq);
	pDynaModel->SetFunction(Q, Q - Vd * Iq + Vq * Id);
	pDynaModel->SetFunction(Id, Id - idiq.real());
	pDynaModel->SetFunction(Iq, Iq - idiq.imag());
	pDynaModel->SetFunctionDiff(Psifd, dPsifd);
	pDynaModel->SetFunctionDiff(Psi1d, dPsi1d);
	pDynaModel->SetFunctionDiff(Psi1q, dPsi1q);
	pDynaModel->SetFunctionDiff(Psi2q, dPsi2q);
	pDynaModel->SetFunctionDiff(s, eS);
	pDynaModel->SetFunctionDiff(Delta, eDelta);

	pDynaModel->SetFunction(Eq, 0);


	bRes = bRes && BuildRIfromDQRightHand(pDynaModel);
	return bRes;
}



bool CDynaGeneratorPark3C::BuildDerivatives(CDynaModel* pDynaModel)
{
	bool bRes = CDynaGeneratorDQBase::BuildDerivatives(pDynaModel);
	if (bRes)
	{
		if (IsStateOn())
		{
			const cplx idiq(GetIdIq());

			const double dPsifd = ExtEqe + Psifd_Psifd * Psifd + Psifd_Psi1d * Psi1d + Psifd_id * idiq.real();
			const double dPsi1d = Psi1d_Psifd * Psifd + Psi1d_Psi1d * Psi1d + Psi1d_id * idiq.real();
			const double dPsi1q = Psi1q_Psi1q * Psi1q + Psi1q_Psi2q * Psi2q + Psi1q_iq * idiq.imag();
			const double dPsi2q = Psi2q_Psi1q * Psi1q + Psi2q_Psi2q * Psi2q + Psi2q_iq * idiq.imag();

			const double omega = ZeroGuardSlip(1.0 + s);

			const double Te = (Psid_id * idiq.real() + Psid_Psifd * Psifd + Psid_Psi1d * Psi1d) * idiq.imag()
				- (Psiq_iq * idiq.imag() + Psiq_Psi1q * Psi1q + Psiq_Psi2q * Psi2q) * idiq.real();


			double eDelta = pDynaModel->GetOmega0() * s;
			double eS = (Pt / omega - Kdemp * s - Te) / Mj;


			pDynaModel->SetFunctionDiff(Psifd, dPsifd);
			pDynaModel->SetFunctionDiff(Psi1d, dPsi1d);
			pDynaModel->SetFunctionDiff(Psi1q, dPsi1q);
			pDynaModel->SetFunctionDiff(Psi2q, dPsi2q);
			pDynaModel->SetFunctionDiff(s, eS);
			pDynaModel->SetFunctionDiff(Delta, eDelta);

		}
		else
		{
			pDynaModel->SetFunctionDiff(Psifd, 0);
			pDynaModel->SetFunctionDiff(Psi1d, 0);
			pDynaModel->SetFunctionDiff(Psi1q, 0);
			pDynaModel->SetFunctionDiff(Psi2q, 0);

		}
	}
	return true;
}


eDEVICEFUNCTIONSTATUS CDynaGeneratorPark3C::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes = CDynaGeneratorDQBase::ProcessDiscontinuity(pDynaModel);
	if (CDevice::IsFunctionStatusOK(eRes) && IsStateOn())
	{
		CalculatePower();
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
	return true;
}

cplx CDynaGeneratorPark3C::GetEMF()
{

	const double omega(1.0 + s);
	return { omega * (Eq_Psifd * Psifd + Eq_Psi1d * Psi1d),   omega * (Ed_Psi1q * Psi1d + Ed_Psi2q * Psi2q) };
}

const cplx& CDynaGeneratorPark3C::CalculateEgen()
{
	double xgen = Xgen();
	/*
	Er_q = (h_f * Psi_f + h_D * Psi_D),
		Er_d = -(h_1Q * Psi_1Q + h_2Q * Psi_2Q);


	{
		efq = (1. + su22) * Er_q + (Xdss - X_mid) * Id;
		efd = (1. + su22) * Er_d - (Xqss - X_mid) * Iq;
	}
	//	else 
	{
		//	efd=Edss; efq=Eqss;
	}*/

	return CDynaGeneratorDQBase::CalculateEgen();//m_Egen = cplx(Eqss - Id * (xgen - xd2), Edss + Iq * (xgen - xq2)) * std::polar(1.0, (double)Delta);
}

double CDynaGeneratorPark3C::Xgen() const
{
	return 0.5 * (xd2 + xq2);
}

void CDynaGeneratorPark3C::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaGeneratorDQBase::DeviceProperties(props);
	props.SetType(DEVTYPE_GEN_PARK3C);
	props.SetClassName(CDeviceContainerProperties::m_cszNameGeneratorPark3C, CDeviceContainerProperties::m_cszSysNameGeneratorPark3C);
	props.nEquationsCount = CDynaGeneratorPark3C::VARS::V_LAST;

	/*
	props.AddLinkTo(DEVTYPE_EXCITER, DLM_SINGLE, DPD_SLAVE, CDynaGenerator1C::m_cszExciterId);
	props.SetClassName(CDeviceContainerProperties::m_cszNameGenerator1C, CDeviceContainerProperties::m_cszSysNameGenerator1C);



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