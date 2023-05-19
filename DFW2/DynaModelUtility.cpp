#include "stdafx.h"
#include "DynaModel.h"
#include "DynaGeneratorMotion.h"
#include "BranchMeasures.h"
#include "MathUtils.h"
#include "MixedAdamsBDF.h"
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#include <cpuid.h>
#endif


using namespace DFW2;

void CDynaModel::DebugLog(std::string_view Message) const
{
	if (DebugLogFile.is_open())
		DebugLogFile << Message << std::endl;
}

void CDynaModel::LogTime(DFW2MessageStatus Status, std::string_view Message, ptrdiff_t DbIndex) const
{
	CDynaModel::Log(Status, fmt::format("{} {}", TimeAndStep(), Message), DbIndex);
}

void CDynaModel::Log(DFW2MessageStatus Status, std::string_view Message, ptrdiff_t DbIndex) const
{
	if (m_pLogger && Status <= m_Parameters.m_eConsoleLogLevel)
		m_pLogger->Log(Status, Message, DbIndex);

	if (LogFile.is_open() && Status <= m_Parameters.m_eFileLogLevel)
		LogFile << Message << std::endl;
}

void CDynaModel::ChangeOrder(ptrdiff_t Newq)
{
	if (sc.q != Newq)
	{
		sc.RefactorMatrix();
		// если меняется порядок - то нужно обновлять элементы матрицы считавшиеся постоянными, так как 
		// изменяются коэффициенты метода.
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
	if (!bStopProcessing)
	{
		Log(DFW2MessageStatus::DFW2LOG_MESSAGE, fmt::format(CDFW2Messages::m_cszStopCommandReceived, GetCurrentTime()));
		bStopProcessing = true;
	}
}

CDeviceContainer* CDynaModel::GetDeviceContainer(eDFW2DEVICETYPE Type)
{
	auto&& it = find_if(DeviceContainers_.begin(), DeviceContainers_.end(), [&Type](const auto& Cont) { return Cont->GetType() == Type; });

	if (it == DeviceContainers_.end())
		return nullptr;
	else
		return *it;
}

void CDynaModel::RebuildMatrix(bool bRebuild)
{
	if (!RebuildMatrixFlag_)
		RebuildMatrixFlag_ = sc.m_bRefactorMatrix = bRebuild;
}

void CDynaModel::ProcessTopologyRequest()
{
	sc.m_bProcessTopology = true;
	sc.UpdateConstElements();
}

// Запрос обработки разрыва от устройства
void CDynaModel::DiscontinuityRequest(CDevice& device, const DiscontinuityLevel Level)
{
	// увеличиваем счетчик запросов от устройства
	device.IncrementDiscontinuityRequests();
	// информации о примитиве устройсва нет - отказываемся
	// от учета примитива
	sc.pDiscontinuityPrimitive_ = nullptr;
	if (sc.DiscontinuityLevel_ < Level)
	{
		// запоминаем устройство
		sc.DiscontinuityLevel_ = Level;
		sc.pDiscontinuityDevice_ = &device;
	}
}

// Запрос обработки разрыва от примитива
void CDynaModel::DiscontinuityRequest(CDynaPrimitive& primitive, const DiscontinuityLevel Level)
{
	// увеличиваем счетчик запросов от устройства
	primitive.Device().IncrementDiscontinuityRequests();
	if (sc.DiscontinuityLevel_ < Level)
	{
		// запоминаем устройство
		sc.DiscontinuityLevel_ = Level;
		sc.pDiscontinuityDevice_ = &primitive.Device();
		sc.pDiscontinuityPrimitive_ = &primitive;
	}
}

double CDynaModel::GetWeightedNorm(double *pVector)
{
	RightVector* pVectorBegin{ pRightVector };
	RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };
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
	if (sc.DiscontinuityLevel_ != DiscontinuityLevel::None)
	{
		std::string DeviceInfo(sc.pDiscontinuityDevice_ ? sc.pDiscontinuityDevice_->GetVerbalName() : "");
		if (DeviceInfo.empty())
			DeviceInfo = "???";

		if (sc.pDiscontinuityPrimitive_ && !sc.pDiscontinuityPrimitive_->Name().empty())
			DeviceInfo.append(fmt::format("/{}", sc.pDiscontinuityPrimitive_->Name()));

		LogTime(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(CDFW2Messages::m_cszDiscontinuityProcessing, DeviceInfo));

		sc.DiscontinuityLevel_ = DiscontinuityLevel::None;
		sc.pDiscontinuityDevice_ = nullptr;
		EnterDiscontinuityMode();
		ProcessDiscontinuity();
	}
}

bool CDynaModel::SetStateDiscontinuity(CDiscreteDelay *pDelayObject, double dDelay)
{
	// для разрывов из отрицательного времени ставим выдержку относительно t=0.0
	return m_Discontinuities.SetStateDiscontinuity(pDelayObject, (std::max)(GetCurrentTime(), 0.0) + dDelay);
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
	Automatic().NotifyRelayDelay(pRelayDelay);
	Scenario().NotifyRelayDelay(pRelayDelay);
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
	for (auto&& it : DeviceContainers_)
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
				break;
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

	auto it = std::find_if(DeviceContainers_.begin(), 
						   DeviceContainers_.end(), 
						  [&Alias](const auto& cont) { return cont->GetSystemClassName() == Alias || cont->HasAlias(Alias); });


	if (it != DeviceContainers_.end())
		return *it;

	return nullptr;
}

DEVICECONTAINERS CDynaModel::GetContainersByAlias(std::string_view Alias)
{
	DEVICECONTAINERS ret;
	std::copy_if(DeviceContainers_.begin(), DeviceContainers_.end(), std::back_inserter(ret), [&Alias](const CDeviceContainer* pContainer)
		{
			return pContainer->HasAlias(Alias) || pContainer->GetSystemClassName() == Alias;
		});
	return ret;
}

bool CDynaModel::ApproveContainerToWriteResults(CDeviceContainer *pDevCon)
{
	return pDevCon->Count() && !pDevCon->ContainerProps().bVolatile && pDevCon->GetType() != DEVTYPE_BRANCH;
}

// Выводит статистику по топ-устройствам, которые выдавали zero-crossing
void CDynaModel::GetTopZeroCrossings(ptrdiff_t nCount)
{
	// создаем мультисет устройств, отсортированный по количеству ZC
	auto comp = [](const CDevice* lhs, const CDevice* rhs) { return lhs->GetZeroCrossings() > rhs->GetZeroCrossings(); };
	std::multiset<const CDevice*, decltype(comp)> ZeroCrossingsSet(comp);
	ptrdiff_t nTotalZcCount(0);
	// перебираем контейнеры с устройствами
	for (const auto& container : DeviceContainers_)
		for (const auto& dev : *container)
		{
			if (auto nZc(dev->GetZeroCrossings()); nZc)
			{
				nTotalZcCount += nZc;
				// если в мультисете меньше устройств, чем задано, просто вставляем
				if (static_cast<ptrdiff_t>(ZeroCrossingsSet.size()) < nCount)
					ZeroCrossingsSet.insert(dev);
				else if (auto rb(*ZeroCrossingsSet.rbegin()); rb->GetZeroCrossings() < nZc)
				{
					// если устройств больше, чем нужно, проверяем
					// последнее устройство в мультисете, и если его zc меньше
					// чем zc текущего устройства, удаляем последнее и вставляем текущее
					ZeroCrossingsSet.erase(rb);
						ZeroCrossingsSet.insert(dev);
				}
			}
		}

	for (const auto& dev : ZeroCrossingsSet)
		Log(DFW2MessageStatus::DFW2LOG_INFO,
			fmt::format("{:<6} {} zero-crossings", dev->GetZeroCrossings(), dev->GetVerbalName()));
}

// выдать в лог топ устройств, запрашивавших обработку разрывов
void CDynaModel::GetTopDiscontinuityRequesters(ptrdiff_t nCount)
{
	// создаем мультисет устройств, отсортированный по количеству ZC
	auto comp = [](const CDevice* lhs, const CDevice* rhs) { return lhs->GetDiscontinuityRequests() > rhs->GetDiscontinuityRequests(); };
	std::multiset<const CDevice*, decltype(comp)> DiscontinuityRequesters(comp);
	ptrdiff_t nTotalRequests(0);
	// перебираем контейнеры с устройствами
	for (const auto& container : DeviceContainers_)
	for (const auto& dev : *container)
	{
		if (auto nDr(dev->GetDiscontinuityRequests()); nDr)
		{
			nTotalRequests += nDr;
			// если в мультисете меньше устройств, чем задано, просто вставляем
			if (static_cast<ptrdiff_t>(DiscontinuityRequesters.size()) < nCount)
				DiscontinuityRequesters.insert(dev);
			else if (auto rb(*DiscontinuityRequesters.rbegin()); rb->GetDiscontinuityRequests() < nDr)
			{
				// если устройств больше, чем нужно, проверяем
				// последнее устройство в мультисете, и если его zc меньше
				// чем zc текущего устройства, удаляем последнее и вставляем текущее
				DiscontinuityRequesters.erase(rb);
				DiscontinuityRequesters.insert(dev);
			}
		}
	}

	for (const auto& dev : DiscontinuityRequesters)
	Log(DFW2MessageStatus::DFW2LOG_INFO,
		fmt::format("{:<6} {} discontinuity requests", dev->GetDiscontinuityRequests(), dev->GetVerbalName()));
}

void CDynaModel::GetWorstEquations(ptrdiff_t nCount)
{
	UpdateTotalRightVector();

	std::unique_ptr<const RightVectorTotal*[]> pSortOrder = std::make_unique<const RightVectorTotal*[]>(m_nTotalVariablesCount);
	const RightVectorTotal* pVectorBegin{ pRightVectorTotal.get() };
	const RightVectorTotal* const pVectorEnd{ pVectorBegin + m_nTotalVariablesCount };
	const RightVectorTotal** ppSortOrder{ pSortOrder.get() };

	while (pVectorBegin < pVectorEnd)
	{
		*ppSortOrder = pVectorBegin;
		pVectorBegin++;
		ppSortOrder++;
	}

	std::sort(pSortOrder.get(),
			  pSortOrder.get() + m_nTotalVariablesCount,
			  [](const RightVectorTotal* lhs, const RightVectorTotal* rhs) { return rhs->ErrorHits < lhs->ErrorHits; });
	
	if (nCount > m_nTotalVariablesCount)
		nCount = m_nTotalVariablesCount;

	ppSortOrder = pSortOrder.get();
	while (nCount)
	{
		pVectorBegin = *ppSortOrder;

		// если нет переменных с ошибками - выходим
		if (!pVectorBegin->ErrorHits)
			break;

			Log(DFW2MessageStatus::DFW2LOG_INFO,
				fmt::format("{:<6} {} {} Rtol {:.3g} Atol {:.3g}",
					pVectorBegin->ErrorHits,
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

	// если находимся на минимальном шаге
	// используем любой шанс его поднять раньше
	// чем пройдет лимит по nStepsToStepChange 

	if (Equal(H(), Hmin))
		return dFilteredStep > 1.0;

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

void CDynaModel::DumpStateVector()
{
	std::ofstream dump(Platform().ResultFile(fmt::format("statevector_{}.csv", sc.nStepsCount)));
	if (dump.is_open())
	{
		dump << "Value; db; Device; N0; N1; N2; Error; WError; Atol; Rtol; EqType; SN0; SN1; SN2; SavError; Tminus2Val; PhysEqType; PrimBlockType; ErrorHits" << std::endl;
		for (RightVector* pRv = pRightVector; pRv < pRightVector + klu.MatrixSize(); pRv++)
		{
			dump << fmt::format("{};", *pRv->pValue);
			dump << fmt::format("{};", std::abs(pRv->b));
			dump << fmt::format("{} - {};", pRv->pDevice->GetVerbalName(), pRv->pDevice->VariableNameByPtr(pRv->pValue));
			dump << fmt::format("{};{};{};", pRv->Nordsiek[0], pRv->Nordsiek[1], pRv->Nordsiek[2]);
			dump << fmt::format("{};", std::abs(pRv->Error));
			dump << fmt::format("{};", std::abs(pRv->GetWeightedError(pRv->b, *pRv->pValue)));
			dump << fmt::format("{};{};", pRv->Atol, pRv->Rtol);
			dump << fmt::format("{};", pRv->EquationType);
			dump << fmt::format("{};{};{};", pRv->SavedNordsiek[0], pRv->SavedNordsiek[1], pRv->SavedNordsiek[2]);
			dump << fmt::format("{};", pRv->SavedError);
			dump << fmt::format("{};", pRv->Tminus2Value);
			dump << fmt::format("{};", pRv->PhysicalEquationType);
			dump << fmt::format("{};", pRv->PrimitiveBlock);
			dump << fmt::format("{}", pRv->ErrorHits) << std::endl;
		}
	}
}

std::pair<bool, double> CheckAnglesCrossedPi(const double Angle1, const double Angle2, double& PreviosAngleDifference)
{
	std::pair ret{ false, 0.0 };
	// считаем минимальный угол со знаком между углами 
	const double deltaDiff{ MathUtils::AngleRoutines::GetAbsoluteDiff2Angles(Angle1, Angle2) };
	// предыдущий и текущий углы проверяем на близость к 180 (для начала зону проверки задаем 160)
	const double limitAngle{ 160.0 * M_PI / 180.0 };
	// если знаки углов разные, значит пересекли 180
	if (std::abs(PreviosAngleDifference) >= limitAngle && std::abs(deltaDiff) >= limitAngle && PreviosAngleDifference * deltaDiff < 0.0)
	{
		// синтезируем угол в момент проверки путем добавления к предыдущему углы минимальной разности предыдущего и текущего углов
		const double synthAngle{ std::abs(PreviosAngleDifference) + std::abs(MathUtils::AngleRoutines::GetAbsoluteDiff2Angles(deltaDiff, PreviosAngleDifference)) };
		ret.first = true;
		ret.second = synthAngle * 180.0 / M_PI;
	}
	PreviosAngleDifference = deltaDiff;
	return ret;
}

bool CDynaModel::StabilityLost()
{
	bool bStabilityLost(false);

	if (m_Parameters.m_bStopOnBranchOOS)
	{
		for (const auto& dev : Branches)
		{
			CDynaBranch* pBranch{ static_cast<CDynaBranch*>(dev) };
			if (pBranch->BranchState_ != CDynaBranch::BranchState::BRANCH_ON) continue;
			const auto ret = CheckAnglesCrossedPi(pBranch->pNodeIp_->Delta, pBranch->pNodeIq_->Delta, pBranch->deltaDiff);
			sc.m_MaxBranchAngle.UpdateAbs(pBranch->deltaDiff, GetCurrentTime(), pBranch);
			if (ret.first)
			{
				bStabilityLost = true;
				// если возник АР, максимальный угол уже не нужен
				sc.m_MaxBranchAngle.Reset();

				FinalMessage_ = fmt::format(DFW2::CDFW2Messages::m_cszBranchAngleExceedsPI,
					pBranch->GetVerbalName(),
					ret.second,
					GetCurrentTime());

				SyncLossCause_ = eSyncLossCause::BranchOOS;

				Log(DFW2MessageStatus::DFW2LOG_MESSAGE, FinalMessage_);
			}
		}
	}

	if (m_Parameters.m_bStopOnGeneratorOOS)
	{
		for (auto&& gencontainer : DeviceContainers_)
		{
			// тут можно заранее отобрать контейнеры с генераторами

			if (gencontainer->IsKindOfType(eDFW2DEVICETYPE::DEVTYPE_GEN_MOTION))
			{
				for (auto&& gen : *gencontainer)
				{
					CDynaGeneratorMotion* pGen{ static_cast<CDynaGeneratorMotion*>(gen) };
					if (!pGen->InMatrix()) continue;
					const double nodeDelta(static_cast<const CDynaNodeBase*>(pGen->GetSingleLink(0))->Delta);
					// угол генератора рассчитывается без периодизации и не подходит для CheckAnglesCrossedPi,
					// поэтому мы должны удалить период. Имеем два варианта : atan2 (медленно но надежно) 
					// и функция WrapPosNegPI (быстро и возможны проблемы)
					//const auto ret(CheckAnglesCrossedPi(std::atan2(std::sin(pGen->Delta), std::cos(pGen->Delta)), nodeDelta, pGen->deltaDiff));
					const auto ret(CheckAnglesCrossedPi(MathUtils::AngleRoutines::WrapPosNegPI(pGen->Delta), nodeDelta, pGen->deltaDiff));
					sc.m_MaxGeneratorAngle.UpdateAbs(pGen->deltaDiff, GetCurrentTime(), pGen);
					if (ret.first)
					{
						bStabilityLost = true;
						// если возник АР, максимальный угол уже не нужен

						FinalMessage_ = fmt::format(DFW2::CDFW2Messages::m_cszGeneratorAngleExceedsPI,
							pGen->GetVerbalName(),
							ret.second,
							GetCurrentTime());

						SyncLossCause_ = eSyncLossCause::GeneratorOOS;

						Log(DFW2MessageStatus::DFW2LOG_MESSAGE, FinalMessage_);
					}
				}
			}
		}
	}

	if (bStabilityLost)
	{
		sc.m_MaxGeneratorAngle.Reset();
		sc.m_MaxBranchAngle.Reset();
	}

	return bStabilityLost;
}

bool CDynaModel::OscillationsDecayed()
{
	if (m_Parameters.m_bAllowDecayDetector)
	{
		m_OscDetector.check_pointed_values(GetCurrentTime(), Rtol(), Atol());
		if (m_OscDetector.has_decay(static_cast<size_t>(m_Parameters.m_nDecayDetectorCycles)))
		{
			Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszDecayDetected, GetCurrentTime()));
			return true;
		}
		else
			return false;
	}
	else
		return false;
}

SerializerValidatorRulesPtr CDynaModel::Parameters::GetValidator()
{
	auto Validator = std::make_unique<CSerializerValidatorRules>();
	Validator->AddRule(m_cszProcessDuration, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule(m_cszFrequencyTimeConstant, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule(m_cszLRCToShuntVmin, &ValidatorRange01);
	Validator->AddRule(m_cszFrequencyTimeConstant, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule(m_cszLRCToShuntVmin, &ValidatorRange01);
	Validator->AddRule(m_cszZeroCrossingTolerance, &CSerializerValidatorRules::NonNegative);
	Validator->AddRule(m_cszOutStep, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule(m_cszAtol, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule(m_cszRtol, &CSerializerValidatorRules::NonNegative);
	Validator->AddRule(m_cszRefactorByHRatio, &CSerializerValidatorRules::BiggerThanUnity);
	Validator->AddRule(m_cszMustangDerivativeTimeConstant, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule(m_cszAdamsIndividualSuppressionCycles, &CSerializerValidatorRules::BiggerThanUnity);
	Validator->AddRule(m_cszAdamsGlobalSuppressionStep, &CSerializerValidatorRules::BiggerThanUnity);
	Validator->AddRule(m_cszAdamsIndividualSuppressStepsRange, &CSerializerValidatorRules::BiggerThanUnity);
	Validator->AddRule(m_cszMinimumStepFailures, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule(m_cszAdamsDampingSteps, &CSerializerValidatorRules::BiggerThanUnity);
	Validator->AddRule(m_cszAdamsDampingAlpha, &ValidatorRange01);
	Validator->AddRule(m_cszProcessDuration, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule(m_cszDecayDetectorCycles, &CSerializerValidatorRules::BiggerThanUnity);
	Validator->AddRule(m_cszLFImbalance, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule(m_cszLFSeidellIterations, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule(m_cszLFEnableSwitchIteration, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule(m_cszLFEnableSwitchIteration, &CSerializerValidatorRules::NonNegative);
	Validator->AddRule(m_cszLFMaxIterations, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule(m_cszLFNewtonMaxVoltageStep, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule(m_cszLFNewtonMaxNodeAngleStep, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule(m_cszLFNewtonMaxBranchAngleStep, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule(m_cszLFForceSwitchLambda, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule(m_cszNewtonMaxNorm, &CSerializerValidatorRules::BiggerThanZero);
	Validator->AddRule(m_cszLRCSmoothingRange, &ValidatorRange01);
	Validator->AddRule(m_cszSecuritySpinReference, &CSerializerValidatorRules::NonNegative);
	Validator->AddRule(m_cszLRCSmoothingRange, &ValidatorRange01);
	Validator->AddRule(m_cszSecuritySpinReference, &CSerializerValidatorRules::NonNegative);
	Validator->AddRule(cszMaxResultFilesCount, &CSerializerValidatorRules::NonNegative);
	Validator->AddRule(cszMaxResultFilesSize, &CSerializerValidatorRules::NonNegative);
	Validator->AddRule(cszStepsToOrderChange, &ValidatorRange2_10);
	Validator->AddRule(cszStepsToStepChange, &ValidatorRange2_10);
	return Validator;
}

SerializerPtr CDynaModel::Parameters::GetSerializer()
{
	SerializerPtr Serializer = std::make_unique<CSerializerBase>(new CSerializerDataSourceBase());
	Serializer->SetClassName("Parameters");
	Serializer->AddProperty(m_cszFrequencyTimeConstant, m_dFrequencyTimeConstant, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(m_cszLRCToShuntVmin, m_dLRCToShuntVmin, eVARUNITS::VARUNIT_PU);
	Serializer->AddProperty(m_cszConsiderDampingEquation, m_bConsiderDampingEquation);
	Serializer->AddProperty(m_cszZeroCrossingTolerance, m_dZeroCrossingTolerance);
	Serializer->AddProperty(m_cszDontCheckTolOnMinStep, m_bDontCheckTolOnMinStep);
	Serializer->AddProperty(m_cszOutStep, m_dOutStep, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty("VarSearchStackDepth", nVarSearchStackDepth);
	Serializer->AddProperty(m_cszAtol, m_dAtol);
	Serializer->AddProperty(m_cszRtol, m_dRtol);
	Serializer->AddProperty(m_cszRefactorByHRatio, m_dRefactorByHRatio);
	Serializer->AddProperty(m_cszMustangDerivativeTimeConstant, m_dMustangDerivativeTimeConstant, eVARUNITS::VARUNIT_SECONDS);
	Serializer->AddProperty(m_cszAdamsIndividualSuppressionCycles, m_nAdamsIndividualSuppressionCycles, eVARUNITS::VARUNIT_PIECES);
	Serializer->AddProperty(m_cszAdamsGlobalSuppressionStep, m_nAdamsGlobalSuppressionStep, eVARUNITS::VARUNIT_PIECES);
	Serializer->AddProperty(m_cszAdamsIndividualSuppressStepsRange, m_nAdamsIndividualSuppressStepsRange, eVARUNITS::VARUNIT_PIECES);
	Serializer->AddProperty(m_cszUseRefactor, m_bUseRefactor);
	Serializer->AddProperty(m_cszDisableResultsWriter, m_bDisableResultsWriter);
	Serializer->AddProperty(m_cszMinimumStepFailures, m_nMinimumStepFailures, eVARUNITS::VARUNIT_PIECES);
	Serializer->AddProperty(m_cszZeroBranchImpedance, m_dZeroBranchImpedance, eVARUNITS::VARUNIT_OHM);
	Serializer->AddProperty(m_cszAdamsDampingAlpha, m_dAdamsDampingAlpha);
	Serializer->AddProperty(m_cszAdamsDampingSteps, m_nAdamsDampingSteps);
	Serializer->AddProperty(m_cszAllowUserOverrideStandardLRC, m_bAllowUserOverrideStandardLRC);
	Serializer->AddProperty(m_cszAllowDecayDetector, m_bAllowDecayDetector);
	Serializer->AddProperty(m_cszDecayDetectorCycles, m_nDecayDetectorCycles);
	Serializer->AddProperty(m_cszStopOnBranchOOS, m_bStopOnBranchOOS);
	Serializer->AddProperty(m_cszStopOnGeneratorOOS, m_bStopOnGeneratorOOS);
	Serializer->AddProperty(m_cszWorkingFolder, WorkingFolder_);
	Serializer->AddProperty(m_cszResultsFolder, ResultsFolder_);
	Serializer->AddProperty(m_cszProcessDuration, m_dProcessDuration);
	Serializer->AddProperty(m_cszSecuritySpinReference, SecuritySpinReference_);
	Serializer->AddProperty(m_cszHmax, Hmax);
	Serializer->AddProperty(cszHysteresisAtol, HysteresisAtol_);
	Serializer->AddProperty(cszHysteresisRtol, HysteresisRtol_);
	Serializer->AddProperty(cszMaxResultFilesCount, MaxResultFilesCount_);
	Serializer->AddProperty(cszMaxResultFilesSize, MaxResultFilesSize_);
	Serializer->AddProperty(cszChangeActionsAreCumulative, ChangeActionsAreCumulative_);
	Serializer->AddProperty(cszDebugModelNameTemplate, DebugModelNameTemplate_);
	Serializer->AddProperty(cszStepsToStepChange, StepsToStepChange_);
	Serializer->AddProperty(cszStepsToOrderChange, StepsToOrderChange_);
	Serializer->AddProperty(cszDerlagToleranceMultiplier, DerLagTolerance_);

	Serializer->AddEnumProperty(m_cszAdamsRingingSuppressionMode, 
		new CSerializerAdapterEnum<ADAMS_RINGING_SUPPRESSION_MODE>(m_eAdamsRingingSuppressionMode, m_cszAdamsRingingSuppressionNames));

	Serializer->AddEnumProperty(m_cszFreqDampingType,
		new CSerializerAdapterEnum<ACTIVE_POWER_DAMPING_TYPE>(eFreqDampingType, m_cszFreqDampingNames));

	Serializer->AddEnumProperty("DiffEquationType",
		new CSerializerAdapterEnum<DEVICE_EQUATION_TYPE>(m_eDiffEquationType, m_cszDiffEquationTypeNames));

	Serializer->AddEnumProperty(m_cszConsoleLogLevel,
		new CSerializerAdapterEnum<DFW2MessageStatus>(m_eConsoleLogLevel, m_cszLogLevelNames));

	Serializer->AddEnumProperty(m_cszFileLogLevel,
		new CSerializerAdapterEnum<DFW2MessageStatus>(m_eFileLogLevel, m_cszLogLevelNames));

	Serializer->AddEnumProperty(m_cszParkParametersDetermination,
		new CSerializerAdapterEnum<PARK_PARAMETERS_DETERMINATION_METHOD>(m_eParkParametersDetermination, m_cszParkParametersDeterminationMethodNames));

	Serializer->AddEnumProperty(m_cszGeneratorLessLRC,
		new CSerializerAdapterEnum<GeneratorLessLRC>(m_eGeneratorLessLRC, m_cszGeneratorLessLRCNames));

	// расчет УР

	Serializer->AddProperty(m_cszLFImbalance, Imb);
	Serializer->AddProperty(m_cszLFFlat, Flat);

	Serializer->AddEnumProperty(m_cszLFStartup,
		new CSerializerAdapterEnum<CLoadFlow::eLoadFlowStartupMethod>(Startup, m_cszLFStartupNames));
		
	Serializer->AddProperty(m_cszLFSeidellStep, SeidellStep);
	Serializer->AddProperty(m_cszLFSeidellIterations, SeidellIterations);
	Serializer->AddProperty(m_cszLFEnableSwitchIteration, EnableSwitchIteration);
	Serializer->AddProperty(m_cszLFMaxIterations, MaxIterations);
	Serializer->AddProperty(m_cszLFNewtonMaxVoltageStep, VoltageNewtonStep);
	Serializer->AddProperty(m_cszLFNewtonMaxNodeAngleStep, NodeAngleNewtonStep);
	Serializer->AddProperty(m_cszLFNewtonMaxBranchAngleStep, BranchAngleNewtonStep);
	Serializer->AddProperty(m_cszLFForceSwitchLambda, ForceSwitchLambda);
	Serializer->AddEnumProperty(m_cszLFFormulation, 
		new CSerializerAdapterEnum<CLoadFlow::eLoadFlowFormulation>(LFFormulation, m_cszLFFormulationTypeNames));
	Serializer->AddProperty(m_cszLFAllowNegativeLRC, AllowNegativeLRC);
	Serializer->AddProperty(m_cszLFLRCMinSlope, LRCMinSlope);
	Serializer->AddProperty(m_cszLFLRCMaxSlope, LRCMaxSlope);
	Serializer->AddProperty(m_cszLRCSmoothingRange, m_dLRCSmoothingRange);
	Serializer->AddProperty(m_cszNewtonMaxNorm, m_dNewtonMaxNorm);
	Serializer->AddProperty(m_cszMaxPVPQSwitches, MaxPVPQSwitches);
	Serializer->AddProperty(m_cszPVPQSwitchPerIt, PVPQSwitchPerIt);

	return Serializer;
}


SerializerPtr CDynaModel::StepControl::GetSerializer()
{
	SerializerPtr Serializer = std::make_unique<CSerializerBase>(new CSerializerDataSourceBase());
	Serializer->SetClassName("StepControl");
	Serializer->AddProperty("RefactorMatrix", m_bRefactorMatrix);
	Serializer->AddProperty("FillConstantElements", m_bFillConstantElements);
	Serializer->AddProperty("NewtonConverged", m_bNewtonConverged);
	Serializer->AddProperty("NordsiekSaved", m_bNordsiekSaved);
	Serializer->AddProperty("NewtonDisconverging", m_bNewtonDisconverging);
	Serializer->AddProperty("NewtonStepControl", m_bNewtonStepControl);
	Serializer->AddProperty("DiscontinuityMode", m_bDiscontinuityMode);
	Serializer->AddProperty("ZeroCrossingMode", m_bZeroCrossingMode);
	Serializer->AddProperty("RetryStep", m_bRetryStep);
	Serializer->AddProperty("ProcessTopology", m_bProcessTopology);
	Serializer->AddEnumProperty("DiscontinuityRequest", 
		new CSerializerAdapterEnum<DiscontinuityLevel>(DiscontinuityLevel_, m_cszDiscontinuityLevelTypeNames));
	Serializer->AddProperty("EnforceOut", m_bEnforceOut);
	Serializer->AddProperty("BeforeDiscontinuityWritten", m_bBeforeDiscontinuityWritten);
	Serializer->AddProperty("FilteredStep", dFilteredStep);
	Serializer->AddProperty("FilteredOrder", dFilteredOrder);
	Serializer->AddProperty("FilteredStepInner", dFilteredStepInner);
	Serializer->AddProperty("FilteredOrderInner", dFilteredOrderInner);
	Serializer->AddProperty("RateGrowLimit", dRateGrowLimit);
	Serializer->AddProperty("StepsCount", nStepsCount);
	Serializer->AddProperty("NewtonIterationsCount", nNewtonIterationsCount);
	Serializer->AddProperty("MaxConditionNumber", dMaxConditionNumber);
	Serializer->AddProperty("MaxConditionNumberTime", dMaxConditionNumberTime);
	Serializer->AddProperty("DiscontinuityNewtonFailures", nDiscontinuityNewtonFailures);
	Serializer->AddProperty("MinimumStepFailures", nMinimumStepFailures);
	Serializer->AddProperty("CurrentH", CurrentH_);
	Serializer->AddProperty("UsedH", UsedH_);
	Serializer->AddProperty("q", q);
	Serializer->AddProperty("t", t);
	//Serializer->AddProperty("KahanC", KahanC);
	Serializer->AddProperty("t0", t0);
	Serializer->AddProperty("StepsToStepChangeParameter", nStepsToStepChangeParameter);
	Serializer->AddProperty("StepsToOrderChangeParameter", nStepsToOrderChangeParameter);
	Serializer->AddProperty("StepsToFailParameter", nStepsToFailParameter);
	Serializer->AddProperty("StepsToStepChange", nStepsToStepChange);
	Serializer->AddProperty("StepsToOrderChange", nStepsToOrderChange);
	Serializer->AddProperty("StepsToFail", nStepsToFail);
	Serializer->AddProperty("SuccessfullStepsOfNewton", nSuccessfullStepsOfNewton);
	Serializer->AddProperty("StepsToEndRateGrow", nStepsToEndRateGrow);
	Serializer->AddProperty("NewtonIteration", nNewtonIteration);
	Serializer->AddProperty("RightHandNorm", dRightHandNorm);
	Serializer->AddProperty("AdamsDampingEnabled", bAdamsDampingEnabled);
	Serializer->AddProperty("NoRingingSteps", nNoRingingSteps);
	Serializer->AddProperty("LastRefactorH", m_dLastRefactorH);
	Serializer->AddProperty("RingingDetected", bRingingDetected);
	Serializer->AddProperty("LastConditionNumber", dLastConditionNumber);
	Serializer->AddProperty("NordsiekReset", m_bNordsiekReset);

	Serializer->AddProperty("Order0Steps", OrderStatistics[0].nSteps);
	Serializer->AddProperty("Order0Failures", OrderStatistics[0].nFailures);
	Serializer->AddProperty("Order0NewtonFailures", OrderStatistics[0].nNewtonFailures);
	Serializer->AddProperty("Order0ZeroCrossingsSteps", OrderStatistics[0].nZeroCrossingsSteps);
	Serializer->AddProperty("Order0TimePassed", OrderStatistics[0].dTimePassed);
	Serializer->AddProperty("Order0TimePassedKahan", OrderStatistics[0].dTimePassedKahan);

	Serializer->AddProperty("Order1Steps", OrderStatistics[1].nSteps);
	Serializer->AddProperty("Order1Failures", OrderStatistics[1].nFailures);
	Serializer->AddProperty("Order1NewtonFailures", OrderStatistics[1].nNewtonFailures);
	Serializer->AddProperty("Order1ZeroCrossingsSteps", OrderStatistics[1].nZeroCrossingsSteps);
	Serializer->AddProperty("Order1TimePassed", OrderStatistics[1].dTimePassed);
	Serializer->AddProperty("Order1TimePassedKahan", OrderStatistics[1].dTimePassedKahan);

	return Serializer;
}

void CDynaModel::CheckFolderStructure()
{
	m_Platform.CheckFolderStructure(stringutils::utf8_decode(m_Parameters.WorkingFolder_));
}


CDevice* CDynaModel::GetDeviceBySymbolicLink(std::string_view Object, std::string_view Keys, std::string_view SymLink)
{
	std::list<CDevice*> FoundDevices;
	// определяем контейнеры по имени
	DEVICECONTAINERS Containers = GetContainersByAlias(Object);
	if (!Containers.empty())
	{
		// получен список контейнеров, соответствующих алиасу или системному имени
		for (auto&& container : Containers)
		{
			// для ветви отдельная обработка, так как требуется до трех ключей
			CDevice* pFindDevice{ nullptr };
			if (container->GetType() == DEVTYPE_BRANCH)
			{
				ptrdiff_t nIp(0), nIq(0), nNp(0);
				bool bReverse = false;
#ifdef _MSC_VER
				ptrdiff_t nKeysCount = sscanf_s(std::string(Keys).c_str(), "%td,%td,%td", &nIp, &nIq, &nNp);
#else
				ptrdiff_t nKeysCount = sscanf(std::string(Keys).c_str(), "%td,%td,%td", &nIp, &nIq, &nNp);
#endif
				if (nKeysCount > 1)
				{
					// ищем ветвь в прямом, если не нашли - в обратном направлении
					// здесь надо бы ставить какой-то флаг что ветвь в реверсе
					pFindDevice = Branches.GetByKey({ nIp, nIq, nNp }) ;
					if (!pFindDevice)
						pFindDevice = Branches.GetByKey({ nIq, nIp, nNp });
					if (pFindDevice)
						FoundDevices.push_back(pFindDevice);
				}
			}
			else
			{
				// контейнер с одним ключом
				ptrdiff_t nId(0);
#ifdef _MSC_VER
				if (sscanf_s(std::string(Keys).c_str(), "%td", &nId) == 1)
#else
				if (sscanf(std::string(Keys).c_str(), "%td", &nId) == 1)
#endif
				if (pFindDevice = container->GetDevice(nId))
					FoundDevices.push_back(pFindDevice);
			}
		}

		if (FoundDevices.empty())
			Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszWrongKeyForSymbolicLink, Keys, SymLink));
	}
	else
		Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszObjectNotFoundByAlias, Object, SymLink));

	if (FoundDevices.empty())
		return nullptr;
	if (FoundDevices.size() == 1)
		return FoundDevices.front();
	else 
	{
		STRINGLIST lst;
		std::transform(FoundDevices.begin(), FoundDevices.end(), std::back_inserter(lst), [](const CDevice* pDev)
			{
				return pDev->GetVerbalName();
			});

		Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszAmbigousObjectsFoundByAlias, 
				Object, 
				SymLink, 
				fmt::join(lst,",")));

		return nullptr;
	}
}

void CDynaModel::SnapshotRightVector()
{
	// и обновляем в нем значения переменных на текущие
	m_RightVectorSnapshot.resize(klu.MatrixSize());
	struct RightVector* pCheckVectorBegin{ pRightVector };
	const struct RightVector* pCheckVectorEnd{ pRightVector + klu.MatrixSize()};
	auto itCheck{ m_RightVectorSnapshot.begin() };
	while (pCheckVectorBegin < pCheckVectorEnd)
	{
		*itCheck = *pCheckVectorBegin;
		itCheck->Error = *pCheckVectorBegin->pValue;
		pCheckVectorBegin++;
		itCheck++;
	}
}

void CDynaModel::CompareRightVector()
{
	struct RightVector* pCheckVectorBegin{ pRightVector };
	const struct RightVector* pCheckVectorEnd{ pRightVector + (std::min)(klu.MatrixSize(), static_cast<ptrdiff_t>(m_RightVectorSnapshot.size())) };

	Log(DFW2MessageStatus::DFW2LOG_DEBUG, "Right vector compare");

	auto itCheck{ m_RightVectorSnapshot.begin() };
	while (pCheckVectorBegin < pCheckVectorEnd)
	{
		itCheck->Error -= *pCheckVectorBegin->pValue;
		itCheck->b = itCheck->GetWeightedError(*itCheck->pValue);
		pCheckVectorBegin++;
		itCheck++;
	}
	// сортируем по модулю разности
	std::sort(m_RightVectorSnapshot.begin(), m_RightVectorSnapshot.end(), [this](const RightVector& lhs, const RightVector& rhs)
		{
			return lhs.b  > rhs.b;
		});
	// и смотрим какие переменные отклонились наиболее значительно
	for (size_t i{ 0 }; i < m_RightVectorSnapshot.size(); i++)
	{
		const auto& rv{ m_RightVectorSnapshot[i] };
		if (rv.b < 1.0) break;
		Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("Diff {} Weighted {} Variable {} Value {} Device {}",
			rv.Error,
			rv.b,
			rv.pDevice->VariableNameByPtr(rv.pValue),
			*rv.pValue,
			rv.pDevice->GetVerbalName())
		);
	}
}

void CDynaModel::StartProgress()
{
	m_LastProgress = std::chrono::high_resolution_clock::now();
	if (m_pProgress)
		m_pProgress->StartProgress(fmt::format(CDFW2Messages::m_cszProgressCaption, sc.t), 0, 100);
}

CProgress::ProgressStatus CDynaModel::UpdateProgress()
{
	CProgress::ProgressStatus retStatus{ CProgress::ProgressStatus::Continue };
	const auto now{ std::chrono::high_resolution_clock::now() };
	if (std::chrono::duration_cast<std::chrono::seconds>(now - m_LastProgress).count() >= 1.0)
	{
		m_LastProgress = now;
		if (m_pProgress)
			retStatus = m_pProgress->UpdateProgress(fmt::format(CDFW2Messages::m_cszProgressCaption, sc.t),
				static_cast<int>(sc.t / m_Parameters.m_dProcessDuration * 100.0));
	}

	if(retStatus == CProgress::ProgressStatus::Stop)
		StopProcess();

	return retStatus;
}

void CDynaModel::EndProgress()
{
	if (m_pProgress)
		m_pProgress->EndProgress();
}

// создает контейнер с единственным устройством для расчета 
// потокораспределения внутри суперузлов. Все нужные суперузлы
// рассчитываются в одном устройстве

void CDynaModel::CreateZeroLoadFlow()
{
	// создаем устройство для расчета суперузлов по списку суперузлов, которые
	// необходимы для расчета потоков в ветвях (CDynaBranchMeasure)

	for (auto&& bm : BranchMeasures)
		static_cast<CDynaBranchMeasure*>(bm)->TopologyUpdated();

	const size_t nCount{ ZeroLoadFlow.Count() };

	if (!nCount)
		ZeroLoadFlow.AddDevice(new CDynaNodeZeroLoadFlow(Nodes.GetZeroLFSet()));
	else if (nCount == 1)
		static_cast<CDynaNodeZeroLoadFlow*>(ZeroLoadFlow.GetDeviceByIndex(0))->UpdateSuperNodeSet();
	else
		throw dfw2error("CDynaModel::CreateZeroLoadFlow - ZeroLoadFlow must contain 0 or 1 device");
}


[[nodiscard]] bool CDynaModel::IsSSE2Available()
{
#ifdef _MSC_VER	
	int cpufeats[4];
	__cpuid(cpufeats, 1);
	return cpufeats[3] & 0x20;
#else
	unsigned int eax(0), ebx(0), ecx(0), edx(0);
	__get_cpuid(1, &eax, &ebx, &ecx, &edx);
	return edx & ( 1 << 26);
#endif	
}

bool CDynaModel::RunTest()
{
	bool bRes{ true };
	DeserializeParameters(Platform().Root() / "config.json");
	TestDevices.CreateDevices(1);
	PrecomputeConstants();
	Link();
	WriteResultsHeader();
	PreInitDevices();
	InitDevices();
	EstimateMatrix();
	m_Discontinuities.AddEvent(1000.0, new CModelActionStop());
	m_Discontinuities.Init();

	SetH(sc.StartupStep);

	bRes = bRes && InitEquations();

	// сохраняем начальные условия
	// в истории Нордсика с тем чтобы иметь 
	// возможность восстановить их при сбое Ньютона
	// на первом шаге
	SaveNordsiek();

	while (!CancelProcessing() && bRes)
	{
		bRes = bRes && Step();
		UpdateProgress();
		if (!CancelProcessing())
		{
			WriteResults();
		}
	}

	DumpStatistics();

	return bRes;
}

void CDynaModel::ConsiderContainerProperties()
{
	// по атрибутам контейнеров формируем отдельные списки контейнеров для
	// обновления после итерации Ньютона и после прогноза, чтобы не проверять эти атрибуты в основных циклах
	for (auto&& it : DeviceContainers_)
	{
		if (it->ContainerProps().bNewtonUpdate)
			DeviceContainersNewtonUpdate_.emplace_back(it);
		if (it->ContainerProps().bPredict)
			DeviceContainersPredict_.emplace_back(it);
		if (it->ContainerProps().bFinishStep)
			DeviceContainersFinishStep_.emplace_back(it);
		if (it->ContainerProps().bStoreStates)
			DeviceContainersStoreStates_.emplace_back(it);
	}
}

void CDynaModel::PrecomputeConstants()
{
	// копируем дефолтные константы методов интегрирования в константы экземпляра модели
	// константы могут изменяться, например для демпфирования
	Integrator_->Init();
	// считаем параметры гистерезиса по заданным параметрам
	HysteresisAtol_ = Atol() * m_Parameters.HysteresisAtol_;
	HysteresisRtol_ = Rtol() * m_Parameters.HysteresisRtol_;
	sc.nStepsToStepChangeParameter = m_Parameters.StepsToStepChange_;
	sc.nStepsToOrderChangeParameter = m_Parameters.StepsToOrderChange_;
}

void CDynaModel::FinishStep()
{
	// для устройств с независимыми переменными после успешного выполнения шага
	// рассчитываем актуальные значения независимых переменных
	for (auto&& it : DeviceContainersFinishStep_)
		it->FinishStep(*this);
}


void CDynaModel::DumpStatistics()
{
	Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("Steps count {}", sc.nStepsCount));
	Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("Steps by 1st order count {}, failures {} Newton failures {} zc {} Time passed {}",
		sc.OrderStatistics[0].nSteps,
		sc.OrderStatistics[0].nFailures,
		sc.OrderStatistics[0].nNewtonFailures,
		sc.OrderStatistics[0].nZeroCrossingsSteps,
		sc.OrderStatistics[0].dTimePassed));

	Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("Steps by 2nd order count {}, failures {} Newton failures {} zc {} Time passed {}",
		sc.OrderStatistics[1].nSteps,
		sc.OrderStatistics[1].nFailures,
		sc.OrderStatistics[1].nNewtonFailures,
		sc.OrderStatistics[1].nZeroCrossingsSteps,
		sc.OrderStatistics[1].dTimePassed));

	Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("Factors count {} / (Refactors {} + {} failures) Analyzings count {}",
		klu.FactorizationsCount(),
		klu.RefactorizationsCount(),
		klu.RefactorizationFailuresCount(),
		klu.AnalyzingsCount()));

	Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("Newtons count {} {:.2} per step, failures at step {} failures at discontinuity {}",
		sc.nNewtonIterationsCount,
		static_cast<double>(sc.nNewtonIterationsCount) / sc.nStepsCount,
		sc.OrderStatistics[0].nNewtonFailures + sc.OrderStatistics[1].nNewtonFailures,
		sc.nDiscontinuityNewtonFailures));

	Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("Max condition number {} at time {}",
		sc.dMaxConditionNumber,
		sc.dMaxConditionNumberTime));

	Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("Discontinuities processed {}, {} ahead",
		sc.DiscontinuitiesProcessed_,
		m_Discontinuities.EventsLeft(GetCurrentTime() + 0.9 * Hmin())));

	if (sc.dMaxSLEResidual > 0.0)
		Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("Max SLE residual {} at time {}",
			sc.dMaxSLEResidual,
			sc.dMaxSLEResidualTime));

	Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("Zerocrossing misses {}", sc.ZeroCrossingMisses));

	if (m_Parameters.m_bStopOnBranchOOS && sc.m_MaxBranchAngle.Device())
		Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszMaxBranchAngle,
			sc.m_MaxBranchAngle.Value() * 180.0 / M_PI,
			sc.m_MaxBranchAngle.Device()->GetVerbalName(),
			sc.m_MaxBranchAngle.Time()));

	if (m_Parameters.m_bStopOnGeneratorOOS && sc.m_MaxGeneratorAngle.Device())
		Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszMaxGeneratorAngle,
			sc.m_MaxGeneratorAngle.Value() * 180 / M_PI,
			sc.m_MaxGeneratorAngle.Device()->GetVerbalName(),
			sc.m_MaxGeneratorAngle.Time()));

	GetWorstEquations(10);
	GetTopZeroCrossings(10);
	GetTopDiscontinuityRequesters(10);
	std::chrono::milliseconds CalcDuration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - sc.m_ClockStart);
	Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("Duration {}", static_cast<double>(CalcDuration.count()) / 1E3));
}


bool CDynaModel::ProcessOffStep()
{
	bool bRes{ true };

	const auto OldDiscontinuityLevel{ sc.DiscontinuityLevel_ };

	// моделируем работу автоматов безопасности турбин
	if (m_Parameters.SecuritySpinReference_ > 0.0)
	{
		for (auto&& gencontainer : DeviceContainers_)
		{
			// тут можно заранее отобрать контейнеры с генераторами
			if (gencontainer->IsKindOfType(eDFW2DEVICETYPE::DEVTYPE_GEN_MOTION))
			{
				for (auto&& gen : *gencontainer)
				{
					CDynaGeneratorMotion* pGen{ static_cast<CDynaGeneratorMotion*>(gen) };
					if (!pGen->InMatrix() && pGen->GetState() != eDEVICESTATE::DS_ON)
						continue;
					if (pGen->s > m_Parameters.SecuritySpinReference_)
					{
						LogTime(DFW2MessageStatus::DFW2LOG_MESSAGE, fmt::format(CDFW2Messages::m_cszGeneratorOverspeed, 
							pGen->GetVerbalName(), 
							pGen->s, 
							m_Parameters.SecuritySpinReference_),
							pGen->GetDBIndex());

						pGen->ChangeState(eDEVICESTATE::DS_OFF, eDEVICESTATECAUSE::DSC_INTERNAL);
					}
				}
			}
		}
	}

	// если мы повысили уровень разрыва, обрабатываем
	// его внутри данной функции, чтобы выполнить
	// необходимые изменения в системе уравнений
	if (sc.DiscontinuityLevel_ > OldDiscontinuityLevel)
	{
		// обрабатываем разрыв
		ServeDiscontinuityRequest();
		// учитываем изменения в системе уравненний
		bRes = bRes && ApplyChangesToModel();	
		// возвращаем уровень разрыва к исходному, так как уже все
		// что случилось внутри этой функции обработали
		sc.DiscontinuityLevel_ = OldDiscontinuityLevel;
		// но оставляем информацию о разрыве для цикла интегрирования
	}

	return bRes;
}

const std::string CDynaModel::TimeAndStep() const
{
	return fmt::format("t={:15.012f} {:>3}", 
		GetCurrentTime(), 
		GetIntegrationStepNumber());
}


