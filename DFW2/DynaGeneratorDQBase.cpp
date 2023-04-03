#include "stdafx.h"
#include "DynaGeneratorDQBase.h"
#include "DynaModel.h"
#include "MathUtils.h"

using namespace DFW2;

// расчет токов в осях RI из токов в DQ
void CDynaGeneratorDQBase::IfromDQ()
{
	const double co{ cos(Delta) }, si{ sin(Delta) };
	Ire = Iq * co - Id * si;
	Iim = Iq * si + Id * co;
}

cplx CDynaGeneratorDQBase::Igen(ptrdiff_t nIteration)
{
	const cplx YgInt{ 1.0 / Zgen() };

	if (!nIteration)
		Egen_ = GetEMF();
	else
	{
		const cplx Ig{ (Egen_ - std::polar((double)V, (double)DeltaV)) * YgInt };
		const cplx Idq{ Ig * std::polar(1.0, -Delta) };
		FromComplex(Iq, Id, Idq);
	}

	return CalculateEgen() * YgInt;
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorDQBase::PreInit(CDynaModel* pDynaModel)
{
	if (Kgen > 1)
	{
		xd1 /= Kgen;
		Pnom *= Kgen;
		xq /= Kgen;
		Mj *= Kgen;
		xd /= Kgen;
		r /= Kgen;
	}

	Zgen_ = { r , 0.5 * (xq + xd1) };
	// В генераторах с dq преобразованием нельзя
	// использовать шунт Нортона, поэтому он обнуляется
	Ynorton_ = 0.0;

	const auto fnFixZeroXtoXd1 = [this](double& value, const char* cszValueName)
	{
		if (value <= 0.0)
		{
			Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format("\"{}\" : {} = {} {}. {}{}",
				GetVerbalName(),
				cszValueName,
				value,
				CDFW2Messages::m_cszValidationBiggerThanZero,
				CDFW2Messages::m_cszValidationChangedTo,
				fmt::format("{} = {}", m_cszxd1, xd1))
			);
			value = xd1; 
		}
	};

	(fnFixZeroXtoXd1)(xq, m_cszxq);
	(fnFixZeroXtoXd1)(xd, m_cszxd);

	return eDEVICEFUNCTIONSTATUS::DFS_OK;

}

eDEVICEFUNCTIONSTATUS CDynaGeneratorDQBase::Init(CDynaModel* pDynaModel)
{
	return InitModel(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorDQBase::InitModel(CDynaModel* pDynaModel)
{

	if (!pDynaModel->ConsiderDampingEquation())
		Kdemp = 0.0;

	eDEVICEFUNCTIONSTATUS Status{ CDynaGeneratorMotion::InitModel(pDynaModel) };

	if (CDevice::IsFunctionStatusOK(Status))
	{
		Snom = Equal(cosPhinom, 0.0) ? Pnom : Pnom / cosPhinom;
		Qnom = Snom * sqrt(1.0 - cosPhinom * cosPhinom);
		Inom = Snom / Unom / M_SQRT3;
		Eqnom = (Unom * Unom * (Unom * Unom + Qnom * (xd + xq)) + Snom * Snom * xd * xq) / (Unom * sqrt(Unom * Unom * (Unom * Unom + 2.0 * Qnom * xq) + Snom * Snom * xq * xq));

		switch (GetState())
		{
		case eDEVICESTATE::DS_ON:
			CDynaGeneratorDQBase::ProcessDiscontinuity(pDynaModel);
			break;
		case eDEVICESTATE::DS_OFF:
			Vd = -V;
			Vq = V;
			Eq = 0.0;
			ExtEqe = 0.0;
			break;
		}
	}
	return Status;
}

eDEVICEFUNCTIONSTATUS CDynaGeneratorDQBase::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes{ CDynaGeneratorMotion::ProcessDiscontinuity(pDynaModel) };

	if (CDevice::IsFunctionStatusOK(eRes))
	{
		if (IsStateOn())
		{
			double DeltaGT{ Delta - DeltaV }, NodeV{ V };
			Vd = -NodeV * sin(DeltaGT);
			Vq = NodeV * cos(DeltaGT);
			const double det{ Vd * Vd + Vq * Vq };
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
	double* p{ CDynaGeneratorMotion::GetVariablePtr(nVarIndex) };
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
	double* p{ CDynaGeneratorMotion::GetConstVariablePtr(nVarIndex) };
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(ExciterId, C_EXCITERID)
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
	eDEVICEFUNCTIONSTATUS eRes{ CDynaGeneratorMotion::UpdateExternalVariables(pDynaModel) };
	CDevice* pExciter{ GetSingleLink(DEVTYPE_EXCITER) };
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(ExtEqe, pExciter, m_cszEqe));
	return eRes;
}

void CDynaGeneratorDQBase::UpdateSerializer(CSerializerBase* Serializer)
{
	// обновляем сериализатор базового класса
	CDynaGeneratorMotion::UpdateSerializer(Serializer);
	// добавляем свойства модели одноконтурной модели генератора в ЭДС
	Serializer->AddProperty(m_cszExciterId, ExciterId, eVARUNITS::VARUNIT_UNITLESS);
	Serializer->AddState("Egen", Egen_, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddProperty(m_cszxd, xd, eVARUNITS::VARUNIT_OHM);
	Serializer->AddState(m_cszVd, Vd, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState(m_cszVq, Vq, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState(m_cszId, Id, eVARUNITS::VARUNIT_KAMPERES);
	Serializer->AddState(m_cszIq, Iq, eVARUNITS::VARUNIT_KAMPERES);
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
	props.EquationsCount = CDynaGeneratorDQBase::VARS::V_LAST;

	props.VarMap_.insert(std::make_pair(CDynaGeneratorDQBase::m_cszEq, CVarIndex(CDynaGeneratorDQBase::V_EQ, VARUNIT_KVOLTS)));
	props.VarMap_.insert(std::make_pair(m_cszId, CVarIndex(CDynaGeneratorDQBase::V_ID, VARUNIT_KAMPERES)));
	props.VarMap_.insert(std::make_pair(m_cszIq, CVarIndex(CDynaGeneratorDQBase::V_IQ, VARUNIT_KAMPERES)));
	props.VarMap_.insert(std::make_pair(m_cszVd, CVarIndex(CDynaGeneratorDQBase::V_VD, VARUNIT_KVOLTS)));
	props.VarMap_.insert(std::make_pair(m_cszVq, CVarIndex(CDynaGeneratorDQBase::V_VQ, VARUNIT_KVOLTS)));

	props.ConstVarMap_.insert({ CDynaGeneratorDQBase::m_cszExciterId, CConstVarIndex(CDynaGeneratorDQBase::C_EXCITERID, VARUNIT_PIECES, eDVT_CONSTSOURCE) });
	props.ConstVarMap_.insert({ CDynaGeneratorDQBase::m_cszEqnom, CConstVarIndex(CDynaGeneratorDQBase::C_EQNOM, VARUNIT_KVOLTS, eDVT_INTERNALCONST) });
	props.ConstVarMap_.insert({ CDynaGeneratorDQBase::m_cszSnom, CConstVarIndex(CDynaGeneratorDQBase::C_SNOM, VARUNIT_MVA, eDVT_INTERNALCONST) });
	props.ConstVarMap_.insert({ CDynaGeneratorDQBase::m_cszInom, CConstVarIndex(CDynaGeneratorDQBase::C_INOM, VARUNIT_KAMPERES, eDVT_INTERNALCONST) });
	props.ConstVarMap_.insert({ CDynaGeneratorDQBase::m_cszQnom, CConstVarIndex(CDynaGeneratorDQBase::C_QNOM, VARUNIT_MVAR, eDVT_INTERNALCONST) });
	props.ConstVarMap_.insert({ CDynaGeneratorDQBase::m_cszEqe, CConstVarIndex(CDynaGeneratorDQBase::C_EQE, VARUNIT_KVOLTS, eDVT_INTERNALCONST) });

	// запрещаем явное использование фабрики данного класса
	props.DeviceFactory = nullptr;
}

const cplx& CDynaGeneratorDQBase::CalculateEgen()
{
	return Egen_;
}

// вводит в матрицу блок уравнении для преобразования
// тока из dq в ri и напряжения из ri в dq
void CDynaGeneratorDQBase::BuildRIfromDQEquations(CDynaModel* pDynaModel)
{
	const double co{ cos(Delta) }, si{ sin(Delta) };

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


	// dVd/dVd
	pDynaModel->SetElement(Vd, Vd, 1);
	// dVd/dVre
	pDynaModel->SetElement(Vd, Vre, si);
	// dVd/dVim
	pDynaModel->SetElement(Vd, Vim, -co);
	// dVd/dDeltaG
	pDynaModel->SetElement(Vd, Delta, Vre * co + Vim * si);

	// dVd/dVd
	pDynaModel->SetElement(Vq, Vq, 1);
	// dVd/dVre
	pDynaModel->SetElement(Vq, Vre, -co);
	// dVd/dVim
	pDynaModel->SetElement(Vq, Vim, -si);
	// dVd/dDeltaG
	pDynaModel->SetElement(Vq, Delta, Vre * si - Vim * co);
}

// вводит в правую часть уравнения для преобразования 
// тока из dq в ri и напряжения из ri в dq
void CDynaGeneratorDQBase::BuildRIfromDQRightHand(CDynaModel* pDynaModel)
{
	const double co{ cos(Delta) }, si{ sin(Delta) };
	pDynaModel->SetFunction(Ire, Ire - Iq  * co + Id  * si);
	pDynaModel->SetFunction(Iim, Iim - Iq  * si - Id  * co);
	pDynaModel->SetFunction(Vd,  Vd  + Vre * si - Vim * co);
	pDynaModel->SetFunction(Vq,  Vq  - Vre * co - Vim * si);
}


void CDynaGeneratorDQBase::BuildMotionEquationBlock(CDynaModel* pDynaModel)
{
	// Вариант уравнения движения с расчетом момента от частоты тока
	// Момент рассчитывается от электрической мощности путем деления на скольжение
	const double omega{ ZeroGuardSlip(1.0 + s) }, omegav{ ZeroGuardSlip(1.0 + Sv) }, MjOmegav{ Mj * omegav };
	pDynaModel->SetElement(s, Id, (Vd + 2.0 * Id * r) / MjOmegav);
	pDynaModel->SetElement(s, Iq, (Vq + 2.0 * Iq * r) / MjOmegav);
	pDynaModel->SetElement(s, Vd, Id / MjOmegav);
	pDynaModel->SetElement(s, Vq, Iq / MjOmegav);
	pDynaModel->SetElement(s, s, -(Kdemp + Pt / omega / omega) / Mj);
	pDynaModel->SetElement(s, Sv, -(Id * Vd + Iq * Vq + r * (Id * Id + Iq * Iq)) / MjOmegav / omegav);
	BuildAngleEquationBlock(pDynaModel);
}

void CDynaGeneratorDQBase::CalculateDerivatives(CDynaModel* pDynaModel, CDevice::fnDerivative fn)
{
	if (IsStateOn())
	{
		const double omega{ 1.0 + s };
		(pDynaModel->*fn)(Delta, pDynaModel->GetOmega0() * s);
		(pDynaModel->*fn)(s, (Pt / omega - Kdemp * s - (Vd * Id + Vq * Iq + (Id * Id + Iq * Iq) * r) / (1.0 + Sv)) / Mj);
	}
	else
	{
		(pDynaModel->*fn)(Delta, 0);
		(pDynaModel->*fn)(s, 0);
	}
}


bool CDynaGeneratorDQBase::GetCanayTimeConstants(const double& x, double xl, double x1, double x2, double xrc, double& To1, double& To2, double& T1, double& T2)
{
	bool bRes{ false };
	auto K12 = [](double xa, double x1, double x2, double x12)
	{
		std::optional<double> ret;
		const double Mult{ (xa + x1) * (xa + x2) };
		if (!Equal(Mult, 0.0))
			ret = 1.0 / Mult * (Mult - x12 * x12);
		return ret;
	};

	double xa{ x - xl };

	if (const auto k12{ K12(xa, xrc + x1, xrc + x2, xa + xrc) }; k12.has_value())
	{
		double A0{ To1 + To2 }, B0{ k12.value() * To1 * To2 };
		double r1{ 0.0 }, r2{ 0.0 };
		if (MathUtils::CSquareSolver::RootsSortedByAbs(B0, A0, 1.0, r1, r2))
		{
			To1 = -1.0 / r1;
			To2 = -1.0 / r2;

			xa *= xl / x;
			if (const auto k12{ K12(xa, xrc + x1, xrc + x2, xa + xrc) }; k12.has_value())
			{
				A0 = T1 + T2;
				B0 = k12.value() * T1 * T2;
				if (MathUtils::CSquareSolver::RootsSortedByAbs(B0, A0, 1.0, r1, r2))
				{
					T1 = -1.0 / r1;
					T2 = -1.0 / r2;
					bRes = true;
				}
			}
		}
	}

	if (bRes)
	{
		// финт - определяем по какой оси работает функция
		// путем определения x = CDynaGeneratorDQBase::xd или x = CDynaGeneratorDQBase::xq
		// нужно только для логирования

		if (&x == &xd)
			return CheckTimeConstants(m_csztdo1, m_csztd1, m_csztdo2, m_csztd2, To1, T1, To2, T2);
		else
		{
			_ASSERTE(&x == &xq);
			return CheckTimeConstants(m_csztqo1, m_csztq1, m_csztqo2, m_csztq2, To1, T1, To2, T2);
		}
	}
	else

		// финт - определяем по какой оси работает функция
		// путем определения x = CDynaGeneratorDQBase::xd или x = CDynaGeneratorDQBase::xq
		// нужно только для логирования

		if (&x == &xd)
		{
			Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszCannotConvertShortCircuitConstants,
				GetVerbalName(),
				CDynaGeneratorDQBase::m_csztd1,
				CDynaGeneratorDQBase::m_csztd2,
				T1,
				T2));
		}
		else
		{
			_ASSERTE(&x == &xq);
			Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszCannotConvertShortCircuitConstants,
				GetVerbalName(),
				CDynaGeneratorDQBase::m_csztq1,
				CDynaGeneratorDQBase::m_csztq2,
				T1,
				T2));
		}


	return bRes;
}

// Расчет постоянных времени КЗ из постоянных времени ХХ
// I.M. Canay, Determination of model parameters of synchronous machines
// IEEPROC, Vol. 130, Pt. B, No. 2, MARCH 1983

bool CDynaGeneratorDQBase::GetShortCircuitTimeConstants(const double& x, double xl, double x1, double x2, double To1, double To2, double& T1, double& T2)
{
	_ASSERTE(To1 > To2);

	// по умолчанию рассчитываем аппроксимации постоянных времени
	// классическим методом
	T1 = To1 * x1 / x;
	T2 = To2 * x2 / x1;

	// далее пытаемся уточнить полученные значения
	// точным методом
	const double A{ 1 - x / x1 + x / x2 };
	const double B{ -To1 - To2 };
	const double C{ To1 * To2 * x2 / x1 };
	// мы ожидаем что Td2 < Td1, поэтому берем первый корень из
	// отсортированных по модулю по возрастанию в качестве T2

	if (MathUtils::CSquareSolver::RootsSortedByAbs(A, B, C, T2, T1))
	{
		// корни есть
		T1 = To1 * To2 * x2 / T2 / x;
		// проверяем
		_ASSERTE(Equal(To1 + To2 - x / x1 * T1 - (1 - x / x1 + x / x2) * T2, 0.0));
		_ASSERTE(Equal(To1 * To2 - T1 * T2 * x / x2, 0.0));
	}
	else
	{
		// финт - определяем по какой оси работает функция
		// путем определения x = CDynaGeneratorDQBase::xd или x = CDynaGeneratorDQBase::xq
		// нужно только для логирования

		if (&x == &xd)
		{
			Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszCannotConvertShortCircuitConstants,
				GetVerbalName(),
				CDynaGeneratorDQBase::m_csztd1,
				CDynaGeneratorDQBase::m_csztd2,
				T1,
				T2));
		}
		else
		{
			_ASSERTE(&x == &xq);
			Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszCannotConvertShortCircuitConstants,
				GetVerbalName(),
				CDynaGeneratorDQBase::m_csztq1,
				CDynaGeneratorDQBase::m_csztq2,
				T1,
				T2));
		}
	}

	// финт - определяем по какой оси работает функция
	// путем определения x = CDynaGeneratorDQBase::xd или x = CDynaGeneratorDQBase::xq
	// нужно только для логирования
	
	if(&x == &xd)
		return CheckTimeConstants(m_csztdo1, m_csztd1, m_csztdo2, m_csztd2, To1, T1, To2, T2);
	else
	{
		_ASSERTE(&x == &xq);
		return CheckTimeConstants(m_csztqo1, m_csztq1, m_csztqo2, m_csztq2, To1, T1, To2, T2);
	}
}

// Проверка убывания ряда постоянных времени оси

bool CDynaGeneratorDQBase::CheckTimeConstants(
	const char* cszTo1,
	const char* cszT1,
	const char* cszTo2,
	const char* cszT2,
	double To1,
	double T1,
	double To2,
	double T2) const
{
	// имена и значения постоянных времени
	std::array<const char*, 4> names = { cszTo1, cszT1, cszTo2, cszT2 };
	std::array<const double*, 4> values = { &To1, &T1, &To2, &T2 };
	// список постоянных с нарушениями
	std::list<size_t> badindexes;

	// проверяем убывание
	for (size_t i{ 0 }; i < names.size() - 1; i++)
	{
		// если следующая постоянная больше предыдущей, добавляем
		// предыдущую в список нарушений
		if (*values[i] <= *values[i + 1])
			badindexes.push_back(i);
	}

	if (badindexes.empty())
		return true;

	// если список нарушений не пустой,
	// выводим его в лог и репортим ошибку

	std::string badnames;
	for (const auto& i : badindexes)
	{
		if (!badnames.empty())
			badnames.append(", ");
		badnames.append(fmt::format("{}={} <= {}={}", 
			names[i],
			*values[i],
			names[i+1],
			*values[i+1]));
	}

	Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszWrongTimeConstants, GetVerbalName(), fmt::join(names, " > "), badnames));
	return false;
}

// Расчет параметров на оси с двумя обмотками по формулам из Kundur
// и уточнением постоянных времени по методике НИИПТ

bool CDynaGeneratorDQBase::GetAxisParametersNiipt(const double& x,
	double xl,
	double x1,
	double x2,
	double To1,
	double To2,
	double& r1,
	double& l1,
	double& r2,
	double& l2,
	PARK_PARAMETERS_DETERMINATION_METHOD Method)
{
	bool bRes{ true };

	if (Method != PARK_PARAMETERS_DETERMINATION_METHOD::Kundur &&
		Method != PARK_PARAMETERS_DETERMINATION_METHOD::NiiptTo &&
		Method != PARK_PARAMETERS_DETERMINATION_METHOD::NiiptToTd)
		throw dfw2error("CDynaGeneratorDQBase::GetAxisParametersNiipt - wrong method passed");

	double la{ x - xl };

	// l1 - Kundur (4.29)
	double denom{ la - x1 + xl };
	if (Equal(denom, 0.0))
	{
		Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszCannotGetParkParameters, GetVerbalName(), "la - x1 - xl", denom));
		return false;
	}

	l1 = la * (x1 - xl) / denom;

	// l2 - Kundur (4.28)
	denom = la * l1 - (x2 - xl) * (l1 + la);

	if (Equal(denom, 0.0))
	{
		Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszCannotGetParkParameters, GetVerbalName(), "la * l1 - (x2 - xl) * (l1 + la)", denom));
		return false;
	}

	l2 = la * l1 * (x2 - xl) / denom;

	const double L1{ la + l1 }, L2{ la + l2 };

	// пытаемся определить активные сопротивления грубо по заданным 
	// разомкнутым постоянным - Kundur (Table 4.1, 4.41 Lpl = 0.0)
	r1 = L1 / To1;							
	r2 = (la * l1 / L1 + l2) / To2;

	if (Method == PARK_PARAMETERS_DETERMINATION_METHOD::NiiptTo ||
		Method == PARK_PARAMETERS_DETERMINATION_METHOD::NiiptToTd)
	{
		// считаем что отказ методик НИИПТ не является ошибкой,
		// так как мы уже рассчитали параметры по Кундуру
		
		//bRes = false;

		double nTo1{ To1 }, nTo2{ To2 };

		// рассчитываем постоянные времени первой и второй обмоток - НИИПТ/Вольдек
		double Ts{ To1 + To2 }, det{ 0.25 * Ts * Ts - To1 * To2 / (1.0 - la * la / (la + l1) / (la + l2)) };
		if (det >= 0)
		{
			nTo1 = 0.5 * Ts + std::sqrt(det);
			r1 = L1 / nTo1;
			r2 = L2 / (Ts - nTo1);
			//bRes = true;
		}
		else
		{
			if (Method == PARK_PARAMETERS_DETERMINATION_METHOD::NiiptToTd)
			{
				// если у НИИПТ не получилось рассчитать по разомкнутым постоянным, считаем замкнутые и 
				// рассчитываем по ним

				if (CDynaGeneratorDQBase::GetShortCircuitTimeConstants(x, xl, x1, x2, To1, To2, nTo1, nTo2))
				{
					//r1 = (l1 + la * xl / x) / nTo1;
					//r2 = (l2 + la * l1 * xl / (la * l1 + l1 * xl + la * xl)) / nTo2;
					// для замкнутых постоянных работает то же выражение что и для разомкнутых (Вольдек с. 679)
					Ts = nTo1 + nTo2;
					la = xl * la / (xl + la);		// рассчитываем Lad' - Lad и xl параллельно
					det = 0.25 * Ts * Ts - nTo1 * nTo2 / (1.0 - la * la / (la + l1) / (la + l2));
					if (det >= 0)
					{
						nTo1 = 0.5 * Ts + std::sqrt(det);
						r1 = (la + l1) / nTo1;
						r2 = (la + l2) / (Ts - nTo1);
						bRes = true;
					}
					else
						Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszParkParametersNiiptPlusMethodFailed, GetVerbalName()));
				}
			}
			else
				Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszParkParametersNiiptMethodFailed,
					GetVerbalName(),
					"0.25 * (To1 + To2)^2 - To1 * To2 / (1.0 - la^2 / (la + l1) / (la + l2))",
					det));
		}
	}
	return bRes;
}


// Методика НИИПТ для одной обмотки на оси

bool CDynaGeneratorDQBase::GetAxisParametersNiipt(double x, double xl, double x1, double To1, double& r1, double& l1)
{
	bool bRes{ true };
	const double la{ x - xl };
	const double denom{ la - x1 + xl };
	if (Equal(denom, 0.0))
	{
		Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszCannotGetParkParameters, GetVerbalName(), "la - x1 - xl", denom));
		return false;
	}

	l1 = la * (x1 - xl) / denom;
	r1 = (la + l1) / To1;
	return bRes;
}

// Расчет фундаментальных параметров Canay для двух обмоток на оси
// I.M. Canay "Modelling of alternating-current machines having multiple rotor circuits", 
// IEEE Transaction on Energy Conversion, Vol. 9, No. 2, June 1993

bool CDynaGeneratorDQBase::GetAxisParametersCanay(const double& x,
	double xl,
	double x1,
	double x2,
	double To1,
	double To2,
	double& r1,
	double& l1,
	double& r2,
	double& l2)
{

	double T1{ To1 }, T2{ To2 };
	
	if (!GetShortCircuitTimeConstants(x, xl, x1, x2, To1, To2, T1, T2))
		return false;

	/*if (!GetCanayTimeConstants(x, xl, x1, x2, xrc, To1, To2, T1, T2))
		return false;*/
	
	const double A0{ x / x1 * T1 + (x / x2 - x / x1 + 1) * T2 };
	const double B0{ x / x2 * T1 * T2 };
	double root1{ 0.0 }, root2{ 0.0 };
	if (MathUtils::CSquareSolver::RootsSortedByAbs(B0, A0, 1, root1, root2))
	{
		const double To1{ -1 / root1 };
		const double To2{ -1 / root2 };

		const double xe{ -xl };
		const double A{ T1 + T2 };
		const double Ae{ 1 / (x + xe) * (x * A + xe * A0) };
		const double Be{ (x2 + xe) / (x + xe) * B0 };

		if (MathUtils::CSquareSolver::RootsSortedByAbs(Be, Ae, 1, root1, root2))
		{
			const double Teo1{ -1 / root1 };
			const double Teo2{ -1 / root2 };
			const double xe1{ (x + xe) / (1 - (Teo1 - To1) * (Teo1 - To2) / (Teo1 * (Teo1 - Teo2))) };
			const double xe2{ (x + xe) * Teo1 * Teo2 / To1 / To2 };
			const double deltay1{ 1 / xe1 - 1 / (x + xe) };
			const double deltay2{ 1 / xe2 - 1 / xe1 };
			const double newl1{ 1 / deltay1 }, newl2{ 1 / deltay2 }, newr1{ newl1 / Teo1 }, newr2{ newl2 / Teo2 };

			l1 = newl1;
			l2 = newl2;
			r1 = newr1;
			r2 = newr2;

			return true;

			// тут была идея проверять соответствие грубых и точных параметров, но они
			// сильно гуляют в зависимости от параметров СМ

			/*if (std::abs(newr1 - r1) / r1 < 0.2 && std::abs(newr2 - r2) / r2 < 0.2 &&
				std::abs(newl1 - l1) / l1 < 0.2 && std::abs(newl2 - l2) / l2 < 0.2)
			{
				return true;
			}
			else
				return false;
			*/
		}
	}

	Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszParkParametersCanayMethodFailed, GetVerbalName()));
	// если не получилось у Canay, пробуем Kundur
	return GetAxisParametersNiipt(x, xl, x1, x2, To1, To2, r1, l1, r2, l2, PARK_PARAMETERS_DETERMINATION_METHOD::Kundur);
}

// Методика определения Canay для одной обмотки по сверхпереходным параметрам

bool CDynaGeneratorDQBase::GetAxisParametersCanay(double x, double xl, double x2, double To2, double& r1, double& l1)
{
	const double xe{ -xl };
	const double T2{ To2 * x2 / x };		// рассчитываем постоянную замкнутую постоянную
	const double Te1{ 1 / (x + xe) * (x * T2 + xe * To2) };
	const double xde1{ (x + xe) * Te1 / To2 };
	const double deltay1{ 1 / xde1 - 1 / (x + xe) };
	l1 = 1 / deltay1;
	r1 = l1 / Te1;
	_CheckNumber(l1);
	_CheckNumber(r1);
	return true;
}

// Сравнение и логирование Парковских параметров, рассчитанных по разным методикам

void CDynaGeneratorDQBase::CompareParksParameterCalculation()
{
	return;
	struct ParkParameters
	{
		double r1{ 0.0 }, l1{ 0.0 }, r2{ 0.0 }, l2{ 0.0 };
	}
	NiiptD{}, NiiptQ{ }, CanayD{}, CanayQ{};

	GetAxisParametersNiipt(xd, xl, xd1, xd2, Tdo1, Tdo2, NiiptD.r1, NiiptD.l1, NiiptD.r2, NiiptD.l2, PARK_PARAMETERS_DETERMINATION_METHOD::NiiptToTd);
	GetAxisParametersCanay(xd, xl, xd1, xd2, Tdo1, Tdo2, CanayD.r1, CanayD.l1, CanayD.r2, CanayD.l2);

	switch (GetType())
	{
	case eDFW2DEVICETYPE::DEVTYPE_GEN_PARK3C:
		// для трехконтурной модели используем полиморфы для одной обмотке на q
		GetAxisParametersNiipt(xq, xl, xq2, Tqo2, NiiptQ.r1, NiiptQ.l1);
		GetAxisParametersCanay(xq, xl, xq2, Tqo2, CanayQ.r1, CanayQ.l1);
		break;
	case eDFW2DEVICETYPE::DEVTYPE_GEN_PARK4C:
		GetAxisParametersNiipt(xq, xl, xq1, xq2, Tqo1, Tqo2, NiiptQ.r1, NiiptQ.l1, NiiptQ.r2, NiiptQ.l2, PARK_PARAMETERS_DETERMINATION_METHOD::NiiptToTd);
		GetAxisParametersCanay(xq, xl, xq1, xq2, Tqo1, Tqo2, CanayQ.r1, CanayQ.l1, CanayQ.r2, CanayQ.l2);
		break;
	default:
		_ASSERTE(0);
		break;
	}

	DebugLog(fmt::format("{}-Niipt;{};{};{};{};{};{};{};{};{}-Canay;{};{};{};{};{};{};{};{}",
		GetVerbalName(),
		NiiptD.r1, NiiptD.l1, NiiptD.r2, NiiptD.l2,
		NiiptQ.r1, NiiptQ.l1, NiiptQ.r2, NiiptQ.l2,
		GetVerbalName(),
		CanayD.r1, CanayD.l1, CanayD.r2, CanayD.l2,
		CanayQ.r1, CanayQ.l1, CanayQ.r2, CanayQ.l2));
}
