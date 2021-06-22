#include "stdafx.h"
#include "DynaModel.h"
#include "DynaGeneratorMotion.h"
#include "klu.h"
#include "cs.h"
using namespace DFW2;


void CDynaModel::Log(CDFW2Messages::DFW2MessageStatus Status, std::string_view Message, ptrdiff_t nDbIndex)
{
	if (m_Parameters.m_bLogToConsole)
	{
#ifdef _MSC_VER
		// для Windows делаем разные цвета в консоли
		HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(hCon, &csbi);

		switch (Status)
		{
		case CDFW2Messages::DFW2MessageStatus::DFW2LOG_FATAL:
			SetConsoleTextAttribute(hCon, BACKGROUND_RED | FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN);
			break;
		case CDFW2Messages::DFW2MessageStatus::DFW2LOG_ERROR:
			SetConsoleTextAttribute(hCon, FOREGROUND_RED | FOREGROUND_INTENSITY);
			break;
		case CDFW2Messages::DFW2MessageStatus::DFW2LOG_WARNING:
			SetConsoleTextAttribute(hCon, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			break;
		case CDFW2Messages::DFW2MessageStatus::DFW2LOG_MESSAGE:
		case CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO:
			SetConsoleTextAttribute(hCon, FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN);
			break;
		case CDFW2Messages::DFW2MessageStatus::DFW2LOG_DEBUG:
			SetConsoleTextAttribute(hCon, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			break;
		}

		SetConsoleOutputCP(CP_UTF8);
#endif
		std::cout << Message << std::endl;

#ifdef _MSC_VER
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY | FOREGROUND_RED);
#endif
	}

	if (LogFile.is_open() && m_Parameters.m_bLogToFile)
		LogFile << Message << std::endl;
}

void CDynaModel::ChangeOrder(ptrdiff_t Newq)
{
	if (sc.q != Newq)
	{
		sc.RefactorMatrix();
		// если меняется порядок - то нужно обновлять элементы матрицы считавшиеся постоянными, так как 
		// изменяются коэффициенты метода.
		// Для подавления рингинга обновление элементов не требуется, так как постоянные элементы
		// относятся к алгебре, а подавление рингинга адамса выполняется в дифурах
		sc.UpdateConstElements();
	}

	sc.q = Newq;

	switch (sc.q)
	{
	case 1:
		break;
	case 2:
		break;
	}
}

struct RightVector* CDynaModel::GetRightVector(const InputVariable& Variable)
{
	return GetRightVector(Variable.Index);
}

struct RightVector* CDynaModel::GetRightVector(const VariableIndexBase& Variable)
{
	return GetRightVector(Variable.Index);
}

struct RightVector* CDynaModel::GetRightVector(const ptrdiff_t nRow)
{
	if (nRow < 0 || nRow >= m_nEstimatedMatrixSize)
		throw dfw2error(fmt::format("CDynaModel::GetRightVector matrix size overrun Row {} MatrixSize {}", nRow, m_nEstimatedMatrixSize));
	return pRightVector + nRow;
}

void CDynaModel::StopProcess()
{
	sc.m_bStopCommandReceived = true;
}

CDeviceContainer* CDynaModel::GetDeviceContainer(eDFW2DEVICETYPE Type)
{
	auto&& it = find_if(m_DeviceContainers.begin(), m_DeviceContainers.end(), [&Type](const auto& Cont) { return Cont->GetType() == Type; });

	if (it == m_DeviceContainers.end())
		return nullptr;
	else
		return *it;
}

void CDynaModel::RebuildMatrix(bool bRebuild)
{
	if (!m_bRebuildMatrixFlag)
		m_bRebuildMatrixFlag = sc.m_bRefactorMatrix = bRebuild;
}

void CDynaModel::ProcessTopologyRequest()
{
	sc.m_bProcessTopology = true;
	sc.UpdateConstElements();
}

void CDynaModel::DiscontinuityRequest()
{
	sc.m_bDiscontinuityRequest = true;
}

double CDynaModel::GetWeightedNorm(double *pVector)
{
	RightVector *pVectorBegin = pRightVector;
	RightVector *pVectorEnd = pRightVector + klu.MatrixSize();
	double *pb = pVector;
	double dSum = 0.0;
	for (; pVectorBegin < pVectorEnd; pVectorBegin++, pb++)
	{
		double weighted = pVectorBegin->GetWeightedError(*pb, *pVectorBegin->pValue);
		dSum += weighted * weighted;
	}
	return sqrt(dSum);
}

double CDynaModel::GetNorm(double *pVector)
{
	double dSum = 0.0;
	double *pb = pVector;
	double *pe = pVector + klu.MatrixSize();
	while (pb < pe)
	{
		dSum += *pb * *pb;
		pb++;
	}
	return sqrt(dSum);
}

void CDynaModel::ServeDiscontinuityRequest()
{
	if (sc.m_bDiscontinuityRequest)
	{
		sc.m_bDiscontinuityRequest = false;
		EnterDiscontinuityMode();
		ProcessDiscontinuity();
	}
}

bool CDynaModel::SetStateDiscontinuity(CDiscreteDelay *pDelayObject, double dDelay)
{
	return m_Discontinuities.SetStateDiscontinuity(pDelayObject, GetCurrentTime() + dDelay);
}

bool CDynaModel::RemoveStateDiscontinuity(CDiscreteDelay *pDelayObject)
{
	return m_Discontinuities.RemoveStateDiscontinuity(pDelayObject);
}

bool CDynaModel::CheckStateDiscontinuity(CDiscreteDelay *pDelayObject)
{
	return m_Discontinuities.CheckStateDiscontinuity(pDelayObject);
}

void CDynaModel::NotifyRelayDelay(const CRelayDelayLogic* pRelayDelay)
{
	m_Automatic.NotifyRelayDelay(pRelayDelay);
}

void CDynaModel::PushVarSearchStack(CDevice*pDevice)
{
	if (pDevice)
	{
		if (m_setVisitedDevices.insert(pDevice).second)
		{
			if (m_ppVarSearchStackTop < m_ppVarSearchStackBase.get() + m_Parameters.nVarSearchStackDepth)
			{
				*m_ppVarSearchStackTop = pDevice;
				m_ppVarSearchStackTop++;
			}
			else
				throw dfw2error(fmt::format(CDFW2Messages::m_cszVarSearchStackDepthNotEnough, m_Parameters.nVarSearchStackDepth));
		}
	}
}

bool CDynaModel::PopVarSearchStack(CDevice* &pDevice)
{
	if (m_ppVarSearchStackTop <= m_ppVarSearchStackBase.get())
		return false;
	m_ppVarSearchStackTop--;
	pDevice = *m_ppVarSearchStackTop;
	return true;
}

void CDynaModel::ResetStack()
{
	if (!m_ppVarSearchStackBase)
		m_ppVarSearchStackBase = std::make_unique<CDevice*[]>(m_Parameters.nVarSearchStackDepth);
	m_ppVarSearchStackTop = m_ppVarSearchStackBase.get();
	m_setVisitedDevices.clear();
}

bool CDynaModel::UpdateExternalVariables()
{
	bool bRes = true;
	for (auto&& it : m_DeviceContainers)
	{
		for (auto&& dit : *it)
		{
			// пропускаем устройства которые не могут быть включены
			if (dit->IsPermanentOff())
				continue;

			switch (dit->UpdateExternalVariables(this))
			{
			case eDEVICEFUNCTIONSTATUS::DFS_DONTNEED:
				break;
			case eDEVICEFUNCTIONSTATUS::DFS_FAILED:
				bRes = false;
			default:
				continue;
			}
			break;
		}
	}
	return bRes;
}


CDeviceContainer* CDynaModel::GetContainerByAlias(std::string_view Alias)
{
	CDeviceContainer *pContainer(nullptr);

	// ищем контейнер по системному имени или псевдониму

	auto it = std::find_if(m_DeviceContainers.begin(), 
						   m_DeviceContainers.end(), 
						  [&Alias](const auto& cont) { return cont->m_ContainerProps.GetSystemClassName() == Alias || cont->HasAlias(Alias); });

	if (it != m_DeviceContainers.end())
		return *it;

	return nullptr;
}

bool CDynaModel::ApproveContainerToWriteResults(CDeviceContainer *pDevCon)
{
	return pDevCon->Count() && !pDevCon->m_ContainerProps.bVolatile && pDevCon->GetType() != DEVTYPE_BRANCH;
}

void CDynaModel::GetWorstEquations(ptrdiff_t nCount)
{
	UpdateTotalRightVector();

	std::unique_ptr<const RightVectorTotal*[]> pSortOrder = std::make_unique<const RightVectorTotal*[]>(m_nTotalVariablesCount);
	const RightVectorTotal *pVectorBegin  = pRightVectorTotal.get();
	const RightVectorTotal *pVectorEnd = pVectorBegin + m_nTotalVariablesCount;
	const RightVectorTotal **ppSortOrder = pSortOrder.get();

	while (pVectorBegin < pVectorEnd)
	{
		*ppSortOrder = pVectorBegin;
		pVectorBegin++;
		ppSortOrder++;
	}

	std::sort(pSortOrder.get(),
			  pSortOrder.get() + m_nTotalVariablesCount,
			  [](const RightVectorTotal* lhs, const RightVectorTotal* rhs) { return rhs->nErrorHits < lhs->nErrorHits; });
	
	if (nCount > m_nTotalVariablesCount)
		nCount = m_nTotalVariablesCount;

	ppSortOrder = pSortOrder.get();
	while (nCount)
	{
		pVectorBegin = *ppSortOrder;
		Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_DEBUG, 
					fmt::format("{:<6} {} {} Rtol {} Atol {}", 
								   pVectorBegin->nErrorHits,
								   pVectorBegin->pDevice->GetVerbalName(),
								   pVectorBegin->pDevice->VariableNameByPtr(pVectorBegin->pValue),
								   pVectorBegin->Rtol,
								   pVectorBegin->Atol)
		   );
		ppSortOrder++;
		nCount--;
	}
}

// ограничивает частоту изменения шага до минимального, просчитанного на серии шагов
// возвращает true, если изменение шага может быть разрешено
bool CDynaModel::StepControl::FilterStep(double dStep)
{
	// определяем минимальный шаг в серии шагов, длина которой
	// nStepsToStepChange

	if (dFilteredStepInner > dStep)
		dFilteredStepInner = dStep;

	dFilteredStep = dFilteredStepInner;

	// если не достигли шага, на котором ограничение изменения заканчивается
	if (nStepsToEndRateGrow >= nStepsCount)
	{
		// и шаг превышает заданный коэффициент ограничения, ограничиваем шаг до этого коэффициента
		if (dRateGrowLimit < dFilteredStep)
			dFilteredStep = dRateGrowLimit;
	}
	// возвращаем true если серия шагов ограничения закончилась и 
	// отфильтрованный шаг должен увеличиться
	return (--nStepsToStepChange <= 0) && dFilteredStep > 1.0;
}

// ограничивает частоту изменения порядка, просчитанного на серии шагов
// возвращает true, если порядок может быть увеличиен после контроля
// на серии шагов

bool CDynaModel::StepControl::FilterOrder(double dStep)
{

	if (dFilteredOrderInner > dStep)
		dFilteredOrderInner = dStep;

	dFilteredOrder = dFilteredOrderInner;

	if (nStepsToEndRateGrow >= nStepsCount)
	{
		if (dRateGrowLimit < dFilteredOrder)
			dFilteredOrder = dRateGrowLimit;
	}
	// возвращаем true, если отфильтрованный порядок на серии шагов
	// должен увеличиться
	return (--nStepsToOrderChange <= 0) && dFilteredOrder > 1.0;
}

csi cs_gatxpy(const cs *A, const double *x, double *y)
{
	csi p, j, n, *Ap, *Ai;
	double *Ax;
	if (!CS_CSC(A) || !x || !y) return (0);       /* check inputs */
	n = A->n; Ap = A->p; Ai = A->i; Ax = A->x;
	for (j = 0; j < n; j++)
	{
		for (p = Ap[j]; p < Ap[j + 1]; p++)
		{
			//y[Ai[p]] += Ax[p] * x[j];

			y[j] += Ax[p] * x[Ai[p]];

			_ASSERTE(Ai[p] < n);
		}
	}
	return (1);
}

void CDynaModel::DumpStateVector()
{
	std::ofstream dump(stringutils::utf8_decode(fmt::format("c:\\tmp\\statevector_{}.csv", sc.nStepsCount)));
	if (dump.is_open())
	{
		dump << "Value; db; Device; N0; N1; N2; Error; WError; Atol; Rtol; EqType; SN0; SN1; SN2; SavError; Tminus2Val; PhysEqType; PrimBlockType; ErrorHits" << std::endl;
		for (RightVector* pRv = pRightVector; pRv < pRightVector + klu.MatrixSize(); pRv++)
		{
			dump << fmt::format("{};", *pRv->pValue);
			dump << fmt::format("{};", fabs(pRv->b));
			dump << fmt::format("{} - {};", pRv->pDevice->GetVerbalName(), pRv->pDevice->VariableNameByPtr(pRv->pValue));
			dump << fmt::format("{};{};{};", pRv->Nordsiek[0], pRv->Nordsiek[1], pRv->Nordsiek[2]);
			dump << fmt::format("{};", fabs(pRv->Error));
			dump << fmt::format("{};", fabs(pRv->GetWeightedError(pRv->b, *pRv->pValue)));
			dump << fmt::format("{};{};", pRv->Atol, pRv->Rtol);
			dump << fmt::format("{};", pRv->EquationType);
			dump << fmt::format("{};{};{};", pRv->SavedNordsiek[0], pRv->SavedNordsiek[1], pRv->SavedNordsiek[2]);
			dump << fmt::format("{};", pRv->SavedError);
			dump << fmt::format("{};", pRv->Tminus2Value);
			dump << fmt::format("{};", pRv->PhysicalEquationType);
			dump << fmt::format("{};", pRv->PrimitiveBlock);
			dump << fmt::format("{}", pRv->nErrorHits) << std::endl;
		}
	}
}

void CDynaModel::Computehl0()
{
	// кэшированное значение l0 * GetH для элементов матрицы
	// пересчитывается при изменении шага и при изменении коэффициентов метода
	// для демпфирования
	// lh[i] = l[i][0] * GetH()
	for (auto&& lh : Methodlh)
		lh = Methodl[&lh - Methodlh][0] * sc.m_dCurrentH;
}

void CDynaModel::EnableAdamsCoefficientDamping(bool bEnable)
{
	if (bEnable == sc.bAdamsDampingEnabled) return;
	sc.bAdamsDampingEnabled = bEnable;
	double Alpha = bEnable ? m_Parameters.m_dAdamsDampingAlpha : 0.0;
	Methodl[3][0] = MethodlDefault[3][0] * (1.0 + Alpha);
	// Вместо MethodDefault[3][3] можно использовать честную формулу для LTE (см. Docs)
	Methodl[3][3] = 1.0 / fabs(-1.0 / MethodlDefault[3][3] - 0.5 * Alpha) / (1.0 + Alpha);
	sc.RefactorMatrix();
	Computehl0();
	Log(CDFW2Messages::DFW2LOG_DEBUG, fmt::format(DFW2::CDFW2Messages::m_cszAdamsDamping, 
														bEnable ? DFW2::CDFW2Messages::m_cszOn : DFW2::CDFW2Messages::m_cszOff));
}

bool CDynaModel::CancelProcessing() 
{
	// TODO !!!!! сделать std !!!!!
#ifdef _MSC_VER
	return WaitForSingleObject(m_hStopEvt, 0) == WAIT_OBJECT_0;
#else
	return false;
#endif
}


// https://stackoverflow.com/questions/1878907/how-can-i-find-the-difference-between-two-angles
/*
double GetAbsoluteDiff2Angles(const double x, const double y)
{
	// c can be PI (for radians) or 180.0 (for degrees);
	return M_PI - std::fabs(std::fmod(std::fabs(x - y), 2.0 * M_PI) - M_PI);
}
*/

double GetAbsoluteDiff2Angles(const double x, const double y)
{
	// https://stackoverflow.com/questions/1878907/how-can-i-find-the-difference-between-two-angles

	/*
	def f(x,y):
	import math
	return min(y-x, y-x+2*math.pi, y-x-2*math.pi, key=abs)
	*/

	std::array<double, 3> args { y - x , y - x + 2.0 * M_PI , y - x - 2 * M_PI };
	return *std::min_element(args.begin(), args.end(), [](const auto& lhs, const auto& rhs) { return std::abs(lhs) < std::abs(rhs); });
}

std::pair<bool, double> CheckAnglesCrossedPi(const double Angle1, const double Angle2, double& PreviosAngleDifference)
{
	std::pair ret{ false, 0.0 };
	// считаем минимальный угол со знаком между углами 
	const double deltaDiff(GetAbsoluteDiff2Angles(Angle1, Angle2));
	// предыдущий и текущий углы проверяем на близость к 180 (для начала зону проверки задаем 160)
	const double limitAngle = 160.0 * M_PI / 180.0;
	// если знаки углов разные, значит пересекли 180
	if (std::abs(PreviosAngleDifference) >= limitAngle && std::abs(deltaDiff) >= limitAngle && PreviosAngleDifference * deltaDiff < 0.0)
	{
		// синтезируем угол в момент проверки путем добавления к предыдущему углы минимальной разности предыдущего и текущего углов
		const double synthAngle(std::abs(PreviosAngleDifference) + std::abs(GetAbsoluteDiff2Angles(deltaDiff, PreviosAngleDifference)));
		ret.first = true;
		ret.second = synthAngle * 180.0 / M_PI;
	}
	PreviosAngleDifference = deltaDiff;
	return ret;
}

bool CDynaModel::StabilityLost()
{
	bool bStabilityLost(false);

	for (const auto& dev : Branches)
	{
		CDynaBranch* pBranch(static_cast<CDynaBranch*>(dev));
		if (pBranch->m_BranchState == CDynaBranch::BranchState::BRANCH_ON)
		{
			const auto ret = CheckAnglesCrossedPi(pBranch->m_pNodeIp->Delta, pBranch->m_pNodeIq->Delta, pBranch->deltaDiff);
			if (ret.first)
			{
				bStabilityLost = true;
				Log(CDFW2Messages::DFW2LOG_MESSAGE, fmt::format(DFW2::CDFW2Messages::m_cszBranchAngleExceedsPI,
					pBranch->GetVerbalName(),
					ret.second,
					GetCurrentTime()));
			}
		}
	}

	for (auto&& gencontainer : m_DeviceContainers)
	{
		// тут можно заранее отобрать контейнеры с генераторами

		if (gencontainer->IsKindOfType(eDFW2DEVICETYPE::DEVTYPE_GEN_MOTION))
		{
			for (auto&& gen : *gencontainer)
			{
				CDynaGeneratorMotion* pGen(static_cast<CDynaGeneratorMotion*>(gen));
				if (pGen->InMatrix())
				{
					const double nodeDelta(static_cast<const CDynaNodeBase*>(pGen->GetSingleLink(0))->Delta);
					// угол генератора рассчитывается без периодизации и не подходит для CheckAnglesCrossedPi,
					// поэтому мы должны удалить период. Имеем два варианта : atan2 (медленно но надежно) 
					// и функция WrapPosNegPI (быстро и возможны проблемы)
					//const auto ret(CheckAnglesCrossedPi(std::atan2(std::sin(pGen->Delta), std::cos(pGen->Delta)), nodeDelta, pGen->deltaDiff));
					const auto ret(CheckAnglesCrossedPi(CDynaModel::WrapPosNegPI(pGen->Delta), nodeDelta, pGen->deltaDiff));
					if (ret.first)
					{
						bStabilityLost = true;
						Log(CDFW2Messages::DFW2LOG_MESSAGE, fmt::format(DFW2::CDFW2Messages::m_cszGeneratorAngleExceedsPI,
							pGen->GetVerbalName(),
							ret.second,
							GetCurrentTime()));
					}
				}
			}
		}
	}

	return bStabilityLost;
}

bool CDynaModel::OscillationsDecayed()
{
	if (m_Parameters.m_bAllowDecayDetector)
	{
		m_OscDetector.check_pointed_values(GetCurrentTime(), GetRtol(), GetAtol());
		if (m_OscDetector.has_decay(static_cast<size_t>(m_Parameters.m_nDecayDetectorCycles)))
		{
			Log(CDFW2Messages::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszDecayDetected, GetCurrentTime()));
			return true;
		}
		else
			return false;
	}
	else
		return false;
}


template<typename T> T CDynaModel::Mod(T x, T y)
{
	// https://stackoverflow.com/questions/4633177/c-how-to-wrap-a-float-to-the-interval-pi-pi

	static_assert(!std::numeric_limits<T>::is_exact, "Mod: floating-point type expected");

	if (0. == y)
		return x;

	double m = x - y * floor(x / y);

	// handle boundary cases resulted from floating-point cut off:

	if (y > 0)              // modulo range: [0..y)
	{
		if (m >= y)           // Mod(-1e-16             , 360.    ): m= 360.
			return 0;

		if (m < 0)
		{
			if (y + m == y)
				return 0; // just in case...
			else
				return y + m; // Mod(106.81415022205296 , _TWO_PI ): m= -1.421e-14 
		}
	}
	else                    // modulo range: (y..0]
	{
		if (m <= y)           // Mod(1e-16              , -360.   ): m= -360.
			return 0;

		if (m > 0)
		{
			if (y + m == y)
				return 0; // just in case...
			else
				return y + m; // Mod(-106.81415022205296, -_TWO_PI): m= 1.421e-14 
		}
	}

	return m;
}

// wrap [rad] angle to [-PI..PI)
double CDynaModel::WrapPosNegPI(double fAng)
{
	return CDynaModel::Mod(fAng + M_PI, 2 * M_PI) - M_PI;
}


const double CDynaModel::MethodlDefault[4][4] = 
//									   l0			l1			l2			Tauq
								   { { 1.0,			1.0,		0.0,		2.0 },				//  BDF-1
									 { 2.0 / 3.0,	1.0,		1.0 /3.0,   4.5 },				//  BDF-2
									 { 1.0,			1.0,		0.0,		2.0 },				//  ADAMS-1
									 { 0.5,			1.0,		0.5,		12.0 } };			//  ADAMS-2

