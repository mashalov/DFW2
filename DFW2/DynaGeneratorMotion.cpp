#include "stdafx.h"
#include "DynaGeneratorMotion.h"
#include "DynaModel.h"


using namespace DFW2;

VariableIndexRefVec& CDynaGeneratorMotion::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDynaGeneratorInfBusBase::GetVariables(JoinVariables({ s, Delta },ChildVec));
}

double* CDynaGeneratorMotion::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = CDynaGeneratorInfBusBase::GetVariablePtr(nVarIndex);
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(Delta.Value, V_DELTA)
			MAP_VARIABLE(s.Value, V_S)
		}
	}
	return p;
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorMotion::InitModel(CDynaModel* pDynaModel)
{
	if (Pnom <= 0.0)
		Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszWrongPnom, GetVerbalName(), Pnom));
	else
		if (Mj / Pnom < 0.01)
			Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszGeneratorSuspiciousMj, GetVerbalName(), Mj / Pnom));

	const cplx Slf{ P, Q };
	const double Srated = 1.05 * (Equal(cosPhinom, 0.0) ? Pnom : Pnom / cosPhinom);
	const double absSlf(std::abs(Slf));

	if (absSlf > Srated)
	{
		const std::string perCents(Srated > 0 ? fmt::format("{:.1f}", absSlf / Srated * 100.0) : "???");
		Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszGeneratorPowerExceedsRated,
			GetVerbalName(),
			Slf,
			absSlf,
			Srated,
			perCents));
	}

	const CDynaNodeBase* pNode = static_cast<const CDynaNodeBase*>(GetSingleLink(0));

	if (pNode && (Unom > pNode->Unom * 1.15 || Unom < pNode->Unom * 0.85))
		Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszUnomMismatch, GetVerbalName(), Unom, pNode->GetVerbalName(), pNode->Unom));

	// !!!!!! just for debug !!!!!!
	/*
	if (Equal(Pnom, 0.0))
	{
		Pnom = P;
		if(!Equal(Pnom, 0.0))
			Mj *= Pnom;
	}*/

	eDEVICEFUNCTIONSTATUS Status = CDynaGeneratorInfBusBase::InitModel(pDynaModel);

	if (CDevice::IsFunctionStatusOK(Status))
	{
		if (Mj < 1E-7)
			Mj = 3 * Pnom;

		Kdemp *= Pnom;
		Pt = P;
		s = 0;
		Status = eDEVICEFUNCTIONSTATUS::DFS_OK;
	}

	return Status;
}


eDEVICEFUNCTIONSTATUS CDynaGeneratorMotion::Init(CDynaModel* pDynaModel)
{

	if (Kgen > 1)
	{
		xd1 /= Kgen;
		Pnom *= Kgen;
		xq /= Kgen;
		Mj *= Kgen;
	}

	return InitModel(pDynaModel);
}


bool CDynaGeneratorMotion::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;
	if (bRes)
	{

		double NodeSv = Sv;
		double sp1 = ZeroGuardSlip(1.0 + s);
		double sp2 = ZeroGuardSlip(1.0 + NodeSv);

		if (!IsStateOn())
		{
			sp1 = sp2 = 1.0;
		}


		// dDeltaG / dS
		pDynaModel->SetElement(Delta, s, -pDynaModel->GetOmega0());
		// dDeltaG / dDeltaG
		pDynaModel->SetElement(Delta, Delta, 0.0);

		pDynaModel->SetElement(s, s, -(Kdemp + Pt / sp1 / sp1)/ Mj );
		pDynaModel->SetElement(s, Vre, Ire / Mj / sp2);
		pDynaModel->SetElement(s, Vim, Iim / Mj / sp2);
		pDynaModel->SetElement(s, Ire, Vre / Mj / sp2);
		pDynaModel->SetElement(s, Iim, Vim / Mj / sp2);
		pDynaModel->SetElement(s, Sv, -(Iim * Vim + Ire * Vre) / Mj / sp2 / sp2);

		// dIre / dIre
		pDynaModel->SetElement(Ire, Ire, 1.0);
		// dIre / dVim
		pDynaModel->SetElement(Ire, Vim, 1.0 / xd1);
		// dIre / dDeltaG
		pDynaModel->SetElement(Ire, Delta, -Eqs * cos(Delta) / xd1);

		// dIim / dIim
		pDynaModel->SetElement(Iim, Iim, 1.0);
		// dIim / dVre
		pDynaModel->SetElement(Iim, Vre, -1.0 / xd1);
		// dIim / dDeltaG
		pDynaModel->SetElement(Iim, Delta, -Eqs * sin(Delta) / xd1);

	}
	return true;
}


bool CDynaGeneratorMotion::BuildRightHand(CDynaModel *pDynaModel)
{
	bool bRes = true;
	if (bRes)
	{
		double NodeSv = Sv;
		double dVre(Vre), dVim(Vim);
		double sp1 = ZeroGuardSlip(1.0 + s);
		double sp2 = ZeroGuardSlip(1.0 + NodeSv);

		if (!IsStateOn())
		{
			sp1 = sp2 = 1.0;
		}

		pDynaModel->SetFunction(Ire, Ire - (Eqs * sin(Delta) - dVim) / xd1);
		pDynaModel->SetFunction(Iim, Iim - (dVre - Eqs * cos(Delta)) / xd1);
		double eS = (Pt / sp1 - Kdemp  * s - (dVre * Ire + dVim * Iim) / sp2) / Mj;
		pDynaModel->SetFunctionDiff(s, eS);
		pDynaModel->SetFunctionDiff(Delta, pDynaModel->GetOmega0() * s);
	}

	return true;
}


bool CDynaGeneratorMotion::BuildDerivatives(CDynaModel *pDynaModel)
{
	bool bRes = true;
	if (bRes)
	{
		double sp1 = 1.0 + s;
		double sp2 = 1.0 + Sv;

		if (IsStateOn())
		{
			pDynaModel->SetDerivative(s, (Pt / sp1 - Kdemp * s - P / sp2) / Mj);
			pDynaModel->SetDerivative(Delta, pDynaModel->GetOmega0() * s);
		}
		else
		{
			pDynaModel->SetDerivative(s, 0);
			pDynaModel->SetDerivative(Delta, 0);
		}
	}
	return true;
}

double* CDynaGeneratorMotion::GetConstVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = CDynaGeneratorInfBusBase::GetConstVariablePtr(nVarIndex);
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(Unom, C_UNOM)
		}
	}
	return p;
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorMotion::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	return CDynaGeneratorInfBusBase::UpdateExternalVariables(pDynaModel);
}


void CDynaGeneratorMotion::UpdateSerializer(CSerializerBase* Serializer)
{
	// обновляем сериализатор базового класса
	CDynaGeneratorInfBusBase::UpdateSerializer(Serializer);
	// добавляем свойства модели генератора в уравнении движения
	Serializer->AddProperty("Kdemp", Kdemp, eVARUNITS::VARUNIT_PIECES);
	Serializer->AddProperty("xq", xq, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty("Mj", Mj, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty("Pnom", Pnom, eVARUNITS::VARUNIT_MW);
	Serializer->AddProperty("Unom", Unom, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddProperty("cosPhinom", cosPhinom, eVARUNITS::VARUNIT_PU);
	// добавляем переменные состояния
	Serializer->AddState("Pt", Pt, eVARUNITS::VARUNIT_MW);
	Serializer->AddState("s", s, eVARUNITS::VARUNIT_PU);
}

void CDynaGeneratorMotion::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaGeneratorInfBusBase::DeviceProperties(props);
	props.SetType(DEVTYPE_GEN_MOTION);
	props.SetClassName(CDeviceContainerProperties::m_cszNameGeneratorMotion, CDeviceContainerProperties::m_cszSysNameGeneratorMotion);
	props.nEquationsCount = CDynaGeneratorMotion::VARS::V_LAST;

	props.m_VarMap.insert(std::make_pair("S", CVarIndex(CDynaGeneratorMotion::V_S, VARUNIT_PU)));
	props.m_VarMap.insert(std::make_pair(CDynaNodeBase::m_cszDelta, CVarIndex(CDynaGeneratorMotion::V_DELTA, VARUNIT_RADIANS)));

	props.m_ConstVarMap.insert(std::make_pair(CDynaGeneratorMotion::m_cszUnom, CConstVarIndex(CDynaGeneratorMotion::C_UNOM, VARUNIT_KVOLTS, eDVT_CONSTSOURCE)));

	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaGeneratorMotion>>();

}

const char* CDynaGeneratorMotion::m_cszUnom = "Unom";