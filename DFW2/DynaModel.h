﻿/*
* Raiden - программа для моделирования длительных электромеханических переходных процессов в энергосистеме
* (С) 2018 - Н.В. Евгений Машалов
* Свидетельство о государственной регистрации программы для ЭВМ №2021663231 
* https://fips.ru/EGD/c7c3d928-893a-4f47-a218-51f3c5686860
* 
* Raiden - long-term power system transient stability simulation module
* (C) 2018 - present Eugene Mashalov
* pat. RU №2021663231
* 
*/

#pragma once
#include <chrono>
#include <algorithm>
#include <atomic>
#include <cfloat>
#include "Discontinuities.h"
#include "CustomDevice.h"
#include "Automatic.h"
#include "KLUWrapper.h"
#include "Results.h"
#include "OscDetector.h"
#include "FmtFormatters.h"
#include "PlatformFolders.h"
#include "LoadFlow.h"
#include "PackageVersion.h"
#include "DynaBranch.h"
#include "Statistics.h"
#include "Logger.h"
#include "NodeMeasures.h"
#include "IntegratorBase.h"

namespace DFW2
{
	class CDynaModel
	{
	public:

		enum class eSyncLossCause
		{
			None,
			BranchOOS,
			GeneratorOOS,
			Overspeed,
			Method
		};

		enum class eItegrationMethod
		{
			MixedAdamsBDF,
			Rodas4,
			Rosenbrock23
		};

		struct DynaModelParameters : public CLoadFlow::LoadFlowParameters
		{
			
			DynaModelParameters() : CLoadFlow::LoadFlowParameters() {}
			eItegrationMethod IntegrationMethod_ = eItegrationMethod::MixedAdamsBDF;
			ACTIVE_POWER_DAMPING_TYPE eFreqDampingType = ACTIVE_POWER_DAMPING_TYPE::APDT_NODE;
			DEVICE_EQUATION_TYPE m_eDiffEquationType = DEVICE_EQUATION_TYPE::DET_DIFFERENTIAL;
			double m_dFrequencyTimeConstant = 0.02;
			double m_dLRCToShuntVmin = 0.5;
			double m_dLRCSmoothingRange = 0.001;
			double m_dZeroCrossingTolerance = 0.0;
			bool m_bDontCheckTolOnMinStep = false;
			bool m_bConsiderDampingEquation = false;
			double m_dOutStep = 1e-10;
			ptrdiff_t nVarSearchStackDepth = 100;
			double m_dAtol = Consts::atol_default;
			double m_dRtol = Consts::rtol_default;
			double m_dNewtonMaxNorm = 1.0;								// дополнительный контроль абсолютной ошибки Ньютона
			double m_dRefactorByHRatio = 1.5;
			double m_dMustangDerivativeTimeConstant = 1E-4;
			// режим подавления рингинга
			ADAMS_RINGING_SUPPRESSION_MODE m_eAdamsRingingSuppressionMode = ADAMS_RINGING_SUPPRESSION_MODE::ARSM_DAMPALPHA;
			ptrdiff_t m_nAdamsIndividualSuppressionCycles = 3;			// количество перемен знака переменной для обнаружения рингинга
			ptrdiff_t m_nAdamsGlobalSuppressionStep = 15;				// номер шага, на кратном которому работает глобальное подавление рингинга
			ptrdiff_t m_nAdamsIndividualSuppressStepsRange = 150;		// количество шагов, на протяжении которого работает индивидуальное подавление рингинга переменной
			bool m_bUseRefactor = true;
			bool m_bDisableResultsWriter = false;
			ptrdiff_t m_nMinimumStepFailures = 1;
			double m_dZeroBranchImpedance = 4e-6;
			double m_dAdamsDampingAlpha = 0.05;
			ptrdiff_t m_nAdamsDampingSteps = 10;
			bool m_bAllowUserOverrideStandardLRC = false;
			bool m_bAllowDecayDetector = false;
			ptrdiff_t m_nDecayDetectorCycles = 3;
			bool m_bStopOnBranchOOS = false;
			bool m_bStopOnGeneratorOOS = false;
			double SecuritySpinReference_ = 0.12;						// уставка глобального автомата скорости
			std::string WorkingFolder_;
			std::string ResultsFolder_;
			std::string DebugModelNameTemplate_;
			DFW2MessageStatus m_eConsoleLogLevel = DFW2MessageStatus::DFW2LOG_ERROR;
			DFW2MessageStatus m_eFileLogLevel = DFW2MessageStatus::DFW2LOG_DEBUG;
			PARK_PARAMETERS_DETERMINATION_METHOD m_eParkParametersDetermination = PARK_PARAMETERS_DETERMINATION_METHOD::NiiptTo;
			GeneratorLessLRC m_eGeneratorLessLRC = GeneratorLessLRC::Iconst;
			double m_dProcessDuration = 150.0;
			double Hmax = (std::numeric_limits<double>::max)();
			double HysteresisRtol_ = 1e-2;								// относительный гистерезис
			double HysteresisAtol_ = 3.0;								// абсолютный гистерезис
			double DerLagTolerance_ = 1.0;								// коэффициент, на который умножается Atol/Rtol Derlag
			ptrdiff_t MaxResultFilesCount_ = 0;							// ограничение количества файлов результатов
			ptrdiff_t MaxResultFilesSize_  = 0;							// ограничение объема файлов в каталоге результатов
			ptrdiff_t MaxLogFilesCount_ = 0;							// ограничение количества файлов в каталоге протокола
			ptrdiff_t MaxLogFilesSize_ = 0;								// ограничение объема файлов в каталоге протокола
			bool ChangeActionsAreCumulative_ = true;					// включает аккумуляцию изменений (наример - последовательное изменение нагрузки на дискрете)
			ptrdiff_t StepsToStepChange_ = 2;							// количество шагов до увеличения шага
			ptrdiff_t StepsToOrderChange_ = 2;							// количество шагов до изменения порядка
		};

		struct Parameters : public DynaModelParameters
		{
			Parameters() { }
			// сериализатор и валидатор
			SerializerPtr GetSerializer();
			SerializerValidatorRulesPtr GetValidator();

			static constexpr const char* m_cszIntegrationMethod = "IntegrationMethod";
			static constexpr const char* m_cszLFFormulationTypeNames[3] = { "Current", "Power", "Tanh" };
			static constexpr const char* m_cszIntegrationMethodNames[3] = { "MixedAdamsBDF", "Rodas4", "Rosenbrock23" };
			static constexpr const char* m_cszDiffEquationTypeNames[2] = { "Algebraic", "Differential" };
			static constexpr const char* m_cszLogLevelNames[7] = { "none", "fatal", "error", "warning", "message", "info", "debug" };
			static constexpr const char* m_cszAdamsRingingSuppressionNames[4] = { "None", "Global", "Individual", "DampAlpha" };
			static constexpr const char* m_cszParkParametersDeterminationMethodNames[4] = { "Kundur", "NiiptTo", "NiiptToTd", "Canay" };
			static constexpr const char* m_cszFreqDampingNames[2] = { "Node", "Island" };
			static constexpr const char* m_cszGeneratorLessLRCNames[2] = { "S","I" };

			static constexpr const char* m_cszSecuritySpinReference = "SecuritySpinReference";
			static constexpr const char* m_cszProcessDuration = "ProcessDuration";
			static constexpr const char* m_cszFrequencyTimeConstant = "FrequencyTimeConstant";
			static constexpr const char* m_cszLRCToShuntVmin = "LRCToShuntVmin";
			static constexpr const char* m_cszConsiderDampingEquation = "ConsiderDampingEquation";
			static constexpr const char* m_cszParkParametersDetermination = "ParkParametersDetermination";
			static constexpr const char* m_cszZeroCrossingTolerance = "ZeroCrossingTolerance";
			static constexpr const char* m_cszDontCheckTolOnMinStep = "DontCheckTolOnMinStep";
			static constexpr const char* m_cszOutStep = "OutStep";


			static constexpr const char* m_cszAtol = "Atol";
			static constexpr const char* m_cszRtol = "Rtol";
			static constexpr const char* m_cszRefactorByHRatio = "RefactorByHRatio";
			static constexpr const char* m_cszMustangDerivativeTimeConstant = "MustangDerivativeTimeConstant";
			static constexpr const char* m_cszAdamsIndividualSuppressionCycles = "AdamsIndividualSuppressionCycles";
			static constexpr const char* m_cszAdamsGlobalSuppressionStep = "AdamsGlobalSuppressionStep";
			static constexpr const char* m_cszAdamsIndividualSuppressStepsRange = "AdamsIndividualSuppressStepsRange";
			static constexpr const char* m_cszUseRefactor = "UseRefactor";
			static constexpr const char* m_cszDisableResultsWriter = "DisableResultsWriter";
			static constexpr const char* m_cszMinimumStepFailures = "MinimumStepFailures";
			static constexpr const char* m_cszZeroBranchImpedance = "ZeroBranchImpedance";
			static constexpr const char* m_cszAdamsDampingAlpha = "AdamsDampingAlpha";
			static constexpr const char* m_cszAdamsDampingSteps = "AdamsDampingSteps";
			static constexpr const char* m_cszAllowUserOverrideStandardLRC = "AllowUserOverrideStandardLRC";
			static constexpr const char* m_cszAllowDecayDetector ="AllowDecayDetector";
			static constexpr const char* m_cszDecayDetectorCycles = "DecayDetectorCycles";
			static constexpr const char* m_cszStopOnBranchOOS = "StopOnBranchOOS";
			static constexpr const char* m_cszStopOnGeneratorOOS = "StopOnGeneratorOOS";
			static constexpr const char* m_cszWorkingFolder = "WorkingFolder";
			static constexpr const char* m_cszResultsFolder = "ResultsFolder";
			static constexpr const char* m_cszAdamsRingingSuppressionMode = "AdamsRingingSuppressionMode";
			static constexpr const char* m_cszFreqDampingType = "FreqDampingType";
			static constexpr const char* m_cszConsoleLogLevel = "ConsoleLogLevel";
			static constexpr const char* m_cszFileLogLevel = "FileLogLevel";
			static constexpr const char* m_cszGeneratorLessLRC = "GeneratorLessLRC";
			static constexpr const char* m_cszLFImbalance = "LFImbalance";
			static constexpr const char* m_cszLFFlat = "LFFlat";
			static constexpr const char* m_cszLFStartup = "LFStartup";
			static constexpr const char* m_cszLFSeidellStep = "SeidellStep";
			static constexpr const char* m_cszLFSeidellIterations = "SeidellIterations";
			static constexpr const char* m_cszLFEnableSwitchIteration = "EnableSwitchIteration";
			static constexpr const char* m_cszLFMaxIterations = "MaxIterations";
			static constexpr const char* m_cszLFNewtonMaxVoltageStep = "VoltageNewtonStep";
			static constexpr const char* m_cszLFNewtonMaxNodeAngleStep = "NodeAngleNewtonStep";
			static constexpr const char* m_cszLFNewtonMaxBranchAngleStep = "BranchAngleNewtonStep";
			static constexpr const char* m_cszLFForceSwitchLambda = "ForceSwitchLambda";
			static constexpr const char* m_cszLFFormulation = "LFFormulation";
			static constexpr const char* m_cszLFAllowNegativeLRC = "AllowNegativeLRC";
			static constexpr const char* m_cszLFLRCMinSlope = "LRCMinSlope";
			static constexpr const char* m_cszLFLRCMaxSlope = "dLRCMaxSlope";
			static constexpr const char* m_cszLRCSmoothingRange = "LRCSmoothingRange";
			static constexpr const char* m_cszNewtonMaxNorm = "NewtonMaxNorm";
			static constexpr const char* m_cszMaxPVPQSwitches = "MaxPVPQSwitches";
			static constexpr const char* m_cszPVPQSwitchPerIt = "PVPQSwitchesPerIteration";
			static constexpr const char* m_cszLFStartupNames[4] = { "None","Seidell", "Tanh", "RKF"};
			static constexpr const char* m_cszHmax = "Hmax";
			static constexpr const char* cszHysteresisRtol = "HysteresisRtol";
			static constexpr const char* cszHysteresisAtol = "HysteresisAtol";
			static constexpr const char* cszMaxResultFilesCount = "MaxResultFilesCount";
			static constexpr const char* cszMaxResultFilesSize = "MaxResultFilesSize";
			static constexpr const char* cszChangeActionsAreCumulative = "ChangeActionsAreCumulative";
			static constexpr const char* cszDebugModelNameTemplate = "DebugModelNameTemplate";
			static constexpr const char* cszStepsToStepChange = "StepsToStepChange";
			static constexpr const char* cszStepsToOrderChange = "StepsToOrderChange";
			static constexpr const char* cszDerlagToleranceMultiplier = "DerlagToleranceMultiplier";
			static inline CValidationRuleRange ValidatorRange01 = CValidationRuleRange(0, 1);
			static inline CValidationRuleRange ValidatorRange2_10 = CValidationRuleRange(2, 10);
		};

		friend class CCustomDevice;
	protected:
		struct MatrixRow
		{
			ptrdiff_t m_nColsCount = 0;
			double *pAxRow , *pAx;
			ptrdiff_t *pApRow, *pAp;
			ptrdiff_t m_nConstElementsToSkip;
			inline void Reset() 
			{ 
				pAx = pAxRow; 
				pAp = pApRow; 
			}
		};

		CDiscontinuities m_Discontinuities;
		std::unique_ptr<IntegratorBase> Integrator_;
		
		struct StepError
		{
			struct Error
			{
				RightVector* pVector = nullptr;
				double dMaxError = 0.0;

				void Reset()
				{
					pVector = nullptr;
					dMaxError = 0.0;
				}

				void Update(RightVector* pRightVector, double dError)
				{
					const auto absValue{ std::abs(dError) };
					if (pVector == nullptr || absValue > dMaxError)
					{
						dMaxError = absValue;
						pVector = pRightVector;
					}
				}

				std::string Info()
				{
					if (pVector)
					{
						return fmt::format("Error {} at {} \"{}\" = {} predicted = [{};{}]", 
							dMaxError, 
							pVector->pDevice ? pVector->pDevice->GetVerbalName() : "???",
							pVector->pDevice ? pVector->pDevice->VariableNameByPtr(pVector->pValue) : "???",
							*pVector->pValue,
							pVector->Nordsiek[0],
							pVector->Nordsiek[1]);
					}
					else
						return fmt::format("Error {} Vector ???", dMaxError);
				}

			} 
				Absolute, Weighted;

			void Reset()
			{
				Absolute.Reset();
				Weighted.Reset();
			}
		};
		

		struct StepControl
		{
		protected:
			double CurrentH_ = 0.0;
			double UsedH_ = -1.0;
			double NordsiekScaledForH_ = 1.0;
			double NordsiekScaledForHSaved_ = 1.0;
		public:
			void StoreUsedH()
			{
				UsedH_ = CurrentH_;
			}
			inline double UsedH() const
			{
				return UsedH_;
			}
			inline double H() const
			{
				return CurrentH_;
			}
			void SetH(double h)
			{
				CurrentH_ = h;
			}

			inline double NordsiekScaledForH() const
			{
				return NordsiekScaledForH_;
			}

			inline double NordsiekScaledForHSaved() const
			{
				return NordsiekScaledForHSaved_;
			}

			inline void SetNordsiekScaledForH(double NordsiekScaledForH)
			{
				NordsiekScaledForH_ = NordsiekScaledForH;
			}

			inline void SetNordsiekScaledForHSaved(double NordsiekScaledForHSaved)
			{
				NordsiekScaledForHSaved_ = NordsiekScaledForHSaved;
			}

			enum class eIterationMode
			{
				NEWTON,
				JN,
				FUNCTIONAL
			} 
			IterationMode = eIterationMode::NEWTON;

			struct _OrderStatistics
			{
				ptrdiff_t nSteps = 0;
				ptrdiff_t nFailures = 0;
				ptrdiff_t nNewtonFailures = 0;
				ptrdiff_t nZeroCrossingsSteps = 0;
				double dTimePassed = 0.0;
				double dTimePassedKahan = 0.0;
			};

			bool m_bRefactorMatrix = false;
			bool m_bFillConstantElements = true;
			bool m_bNewtonConverged = false;
			bool m_bNordsiekSaved = false;
			bool m_bNewtonDisconverging = false;
			bool m_bNewtonStepControl = false;
			bool m_bDiscontinuityMode = false;
			bool m_bZeroCrossingMode = false;
			bool m_bRetryStep = false;
			bool m_bProcessTopology = false;
			DiscontinuityLevel DiscontinuityLevel_ = DiscontinuityLevel::None;
			const CDevice* pDiscontinuityDevice_ = nullptr;
			const CDynaPrimitive* pDiscontinuityPrimitive_ = nullptr;
			bool m_bEnforceOut = false;
			bool m_bBeforeDiscontinuityWritten = false;				// флаг обработки момента времени до разрыва
			double dFilteredStep = 0.0;
			double dFilteredOrder = 0.0;
			double dFilteredStepInner =0.0;								// фильтр минимального шага на серии шагов
			double dFilteredOrderInner =0.0;
			double dRateGrowLimit = (std::numeric_limits<double>::max)();
			ptrdiff_t nStepsCount = 0;
			ptrdiff_t DiscontinuitiesProcessed_ = 0;
			ptrdiff_t nNewtonIterationsCount = 0;
			double dLastConditionNumber = 1.0;
			double dMaxConditionNumber = 0.0;
			double dMaxConditionNumberTime = 0.0;
			double dMaxSLEResidual = 0.0;
			double dMaxSLEResidualTime = 0.0;
			_OrderStatistics OrderStatistics[2];
			ptrdiff_t nDiscontinuityNewtonFailures = 0;
			ptrdiff_t nMinimumStepFailures = 0;
			ptrdiff_t q = 1;
			const double StartupStep = 0.01;
			const double TimeOffset = -3.0 * StartupStep;
			double t = TimeOffset;
			double t0 = TimeOffset;
			// volatile нужен для гарантии корректного учета суммы кэхэна в расчете времени
			// но он же не дает мешает сериализации, поэтому временно убран. Проверить, как влияет
			// наличие или отсутствие volatile на расчет
			/*volatile */double KahanC = 0.0;
			ptrdiff_t nStepsToStepChangeParameter = 4;
			ptrdiff_t nStepsToOrderChangeParameter = 4;
			ptrdiff_t nStepsToFailParameter = 1;
			ptrdiff_t nStepsToStepChange = 0;
			ptrdiff_t nStepsToOrderChange = 0;
			ptrdiff_t nStepsToFail = 0;
			ptrdiff_t nSuccessfullStepsOfNewton = 0;
			ptrdiff_t nStepsToEndRateGrow = 0;
			ptrdiff_t nNewtonIteration = 0;
			double dRightHandNorm = 0;
			std::chrono::time_point<std::chrono::high_resolution_clock> m_ClockStart;
			bool bAdamsDampingEnabled = false;
			ptrdiff_t nNoRingingSteps = 0;
			const double Hmin = 1E-8;
			ptrdiff_t ZeroCrossingMisses = 0;
			double OldNorm0 = 0.0;	// норма алгебраических переменных на предыдущем шаге
			double OldNorm1 = 0.0;	// норма дифференциальных переменных на предыдущем шаге
			StepError Newton;
			StepError Integrator;
			double m_dLastRefactorH = 0.0;
			bool bRingingDetected = false;
			bool StepFromReset_ = true;		// флаг сброса Нордсика - производные равны нулю, контроль предиктора не выполняется

			StatisticsMaxFinder m_MaxBranchAngle, m_MaxGeneratorAngle;

			StepControl()
			{
				m_ClockStart = std::chrono::high_resolution_clock::now();
			}

			// Устанавливаем относительный лимит изменения шага на заданное количество шагов
			void SetRateGrowLimit(double RateGrowLimit, ptrdiff_t nRateGrowSteps = 10)
			{
				dRateGrowLimit = RateGrowLimit;
				// пока не будет сделано nStepsToEndRateGrow шагов, его относительное 
				// изменение будет ограничено до dRateGrowLimit
				nStepsToEndRateGrow = nStepsCount + nRateGrowSteps;
			}

			// устанавливает ограничение изменения шага после очередного изменения
			// на заданное количество шагов. Сбрасывает фильтр минимального шага
			void StepChanged()
			{
				nStepsToStepChange = nStepsToStepChangeParameter;
				dFilteredStep = dFilteredStepInner = (std::numeric_limits<double>::max)();
			}

			bool FilterStep(double dStep);
			bool FilterOrder(double dStep);

			// устанавливает ограничение изменения порядка на заданное количество шагов
			// сбрасывает фильтр порядка
			void OrderChanged()
			{
				nStepsToOrderChange = nStepsToOrderChangeParameter;
				// для повышения порядка задержка мы ждем
				// меньше, чтобы увеличить вероятность работы
				// методом второго порядка с меньшим демпфированием
				if (q == 1)
					nStepsToOrderChange /= 2;
				dFilteredOrderInner = dFilteredOrder = (std::numeric_limits<double>::max)();
			}

			void ResetStepsToFail()
			{
				nStepsToFail = nStepsToFailParameter;
			}

			bool StepFailLimit()
			{
				nStepsToFail--;
				return nStepsToFail <= 0;
			}

			void RefactorMatrix(bool bRefactor = true)
			{
				m_bRefactorMatrix = bRefactor;
			}

			void UpdateConstElements(bool bUpdate = true)
			{
				m_bFillConstantElements = bUpdate;
			}

			inline double CurrentTimePlusStep()
			{
				//return t + CurrentH_;
				return t + CurrentH_ - KahanC;
			}

			// рассчитывает текущее время перед выполнением шага
			inline void Advance_t0()
			{
				// математически функция выполняет t = t0 + CurrentH_;

				// но мы используем Kahan summation для устранения накопленной ошибки
				volatile double ky{ CurrentH_ - KahanC };
				volatile double temp{ t0 + ky };
				// предополагается, что шаг не может быть отменен
				// и поэтому сумма Кэхэна обновляется
				KahanC = (temp - t0) - ky;
				t = temp;

				_OrderStatistics& os{ OrderStatistics[q - 1] };
				ky = CurrentH_ - os.dTimePassedKahan;
				temp = os.dTimePassed + ky;
				os.dTimePassedKahan = (temp - os.dTimePassed) - ky;
				if(os.dTimePassed < temp)
					os.dTimePassed = temp;

				_ASSERTE(std::abs(OrderStatistics[0].dTimePassed + OrderStatistics[1].dTimePassed - t + TimeOffset) < DFW2_EPSILON);
			}

			// рассчитывает текущее время перед выполнением шага, с возможностью возврата
			// к предыдущему времени
			inline void CheckAdvance_t0()
			{
				//t = t0 + m_dCurrentH;

				// do not update KahanC, as this check time
				// can be disregarded;
				const double ky{ CurrentH_ - KahanC };
				const double temp{ t0 + ky };
				t = temp;
			}

			void Assign_t0()
			{
				t0 = t;
			}

			SerializerPtr GetSerializer();

			static constexpr const char* m_cszDiscontinuityLevelTypeNames[3] = { "none", "light", "hard" };
		};

		
		Parameters	m_Parameters;


		CPlatformFolders m_Platform;
		void CheckMatrixElement(ptrdiff_t nRow, ptrdiff_t nCol);
		void ResetCheckMatrixElement();

#ifdef _DEBUG
		#define CHECK_MATRIX_ELEMENT(row, col) CheckMatrixElement((row),(col));
		#define RESET_CHECK_MATRIX_ELEMENT() ResetCheckMatrixElement();
#else
		#define CHECK_MATRIX_ELEMENT(row, col) ;
		#define RESET_CHECK_MATRIX_ELEMENT() ;
#endif

#ifdef _MSC_VER
		CResultsWriterCOM m_ResultsWriter;
#else
		CResultsWriterABI m_ResultsWriter;
#endif		
		std::string ModelName_;					// имя расчетной модели
		std::map<ptrdiff_t, std::set<ptrdiff_t>> m_MatrixChecker;
		std::vector<RightVector> m_RightVectorSnapshot;

		KLUWrapper<double> klu;
		CDynaLRC *pLRCGen_		= nullptr;		// СХН для генераторных узлов без генераторов
		CDynaLRC *pLRCLoad_		= nullptr;		// СХН для узлов без динамической СХН
		CDynaLRC* pLRCYconst_	= nullptr;		// СХН c постоянным шунтом
		CDynaLRC* pLRCIconst_	= nullptr;		// СХН c постоянным током
		CDynaLRC* pLRCSconst_	= nullptr;		// СХН c постоянной мощностью

		std::unique_ptr<MatrixRow[]> m_pMatrixRowsUniq;
		std::unique_ptr<RightVector[]> pRightVectorUniq;
		std::unique_ptr<RightVectorTotal[]> pRightVectorTotal;
		RightVector *pRightVector = nullptr;
		MatrixRow *m_pMatrixRows = nullptr;

		DEVICECONTAINERS DeviceContainers_;
		DEVICECONTAINERS DeviceContainersNewtonUpdate_;
		DEVICECONTAINERS DeviceContainersPredict_;
		DEVICECONTAINERS DeviceContainersFinishStep_;
		DEVICECONTAINERS DeviceContainersStoreStates_;

		CDevice **m_ppVarSearchStackTop;
		std::unique_ptr<CDevice*[]> m_ppVarSearchStackBase;
		DEVICEPTRSET m_setVisitedDevices;

		ptrdiff_t m_nEstimatedMatrixSize;
		ptrdiff_t m_nTotalVariablesCount;
		double *pbRightHand;
		std::unique_ptr<double[]> pRightHandBackup;
		
		std::unique_ptr<CLogger> m_pLogger = std::make_unique<CLoggerConsole>();
		std::unique_ptr<CProgress> m_pProgress = std::make_unique<CProgress>();
		std::chrono::time_point<std::chrono::high_resolution_clock> m_LastProgress;

		bool EstimateBuild_;
		bool RebuildMatrixFlag_;
		std::vector<CDevice*> ZeroCrossingDevices;
		void ConvertToCCSMatrix();
		void UpdateRcond();
		void SetDifferentiatorsTolerance();
		void NewtonUpdate();
		void EstimateMatrix();
		void CreateTotalRightVector();
		void CreateUniqueRightVector();
		void UpdateTotalRightVector();
		void UpdateNewRightVector();
		void DebugCheckRightVectorSync();

		void   ChangeOrder(ptrdiff_t Newq);
		void   Computehl0(); // рассчитывает кэшированные произведения l0 * GetH для элементов матрицы

		bool Step();
		// обрабатывает автоматику между шагами без
		// определения зерокроссинга по итоговым
		// значениям переменных
		bool ProcessOffStep();

		double GetNorm(double *pVector);
		double GetWeightedNorm(double *pVector);

		void PrepareNetworkElements();
		void PrecomputeConstants();		// расчет констант на уровне DynaModel после ввода параметров расчета
		bool Link();
		void ConsiderContainerProperties();
		bool UpdateExternalVariables();
		void TurnOffDevicesByOffMasters();
		bool SetDeviceStateByMaster(CDevice *pDev, const CDevice *pMaster);

		void ResetElement();
		void ReallySetElement(ptrdiff_t nRow, ptrdiff_t nCol, double dValue, bool bAddToPrevious);
		void CountSetElement(ptrdiff_t nRow, ptrdiff_t nCol, double dValue, bool bAddToPrevious);
		void ReallySetElementNoDup(ptrdiff_t nRow, ptrdiff_t nCol, double dValue);
		void CountSetElementNoDup(ptrdiff_t nRow, ptrdiff_t nCol, double dValue);
		typedef void (CDynaModel::*ElementSetterFn)(ptrdiff_t, ptrdiff_t, double, bool);
		typedef void (CDynaModel::*ElementSetterNoDupFn)(ptrdiff_t, ptrdiff_t, double);
		void ScaleAlgebraicEquations();
		ElementSetterFn			ElementSetter;
		ElementSetterNoDupFn	ElementSetterNoDup;
		void SolveRefine();

		double CheckZeroCrossing();
		void FinishStep();
		void DumpStatistics();

		std::filesystem::path CreateResultFilePath(std::string_view FileNameMask, const std::filesystem::path& BasePath);
		void WriteResultsHeader();
		void WriteResults();
		void FinishWriteResults();

		struct StepControl sc;

		bool SetFunctionEqType(ptrdiff_t nRow, double dValue, DEVICE_EQUATION_TYPE EquationType);
		void UnprocessDiscontinuity();

		bool LoadFlow();
		void DumpStateVector();
		FILE* fResult;
		mutable std::ofstream LogFile, DebugLogFile;
		static bool ApproveContainerToWriteResults(CDeviceContainer *pDevCon);

		std::string FinalMessage_;
		eSyncLossCause SyncLossCause_ = eSyncLossCause::None;
		std::filesystem::path ResultFilePath_;
		const std::string TimeAndStep() const;
		std::atomic<bool> bStopProcessing = false;

		double TimeWritten_;
		const char* m_cszDampingName = nullptr;

		COscDetectorBase m_OscDetector;

		CDeviceContainer *m_pClosestZeroCrossingContainer = nullptr;

		void SetElement(ptrdiff_t nRow, ptrdiff_t nCol, double dValue, bool bAddToPrevious);
		void SetElement(ptrdiff_t nRow, ptrdiff_t nCol, double dValue);
		void SetFunction(ptrdiff_t nRow, double dValue);
		void SetFunctionDiff(ptrdiff_t nRow, double dValue);
		void SetDerivative(ptrdiff_t nRow, double dValue);
		bool ApplyChangesToModel();
		bool SSE2Available_ = false;
		double HysteresisRtol_ = 1e-2;
		double HysteresisAtol_ = 10.0;


	public:
		CDynaNodeContainer Nodes;
		CDynaBranchContainer Branches;
		CDeviceContainer LRCs;
		CDeviceContainer Reactors;
		CDeviceContainer Generators3C; 
		CDeviceContainer GeneratorsMustang;
		CDeviceContainer GeneratorsPark3C;
		CDeviceContainer GeneratorsPark4C;
		CDeviceContainer Generators1C;
		CDeviceContainer Generators2C;
		CDeviceContainer GeneratorsMotion;
		CDeviceContainer GeneratorsInfBus;
		CDeviceContainer GeneratorsPowerInjector;
		CDeviceContainer SynchroZones;
		CDeviceContainer ExcitersMustang;
		CDeviceContainer DECsMustang;
		CDeviceContainer ExcConMustang;
		CDeviceContainer BranchMeasures;
		CDeviceContainer NodeMeasures;
		CDeviceContainer TestDevices;
		CDynaNodeZeroLoadFlowContainer ZeroLoadFlow;
		CCustomDeviceContainer CustomDevice;
		CCustomDeviceCPPContainer AutomaticDevice, ScenarioDevice;
		CAutomatic Automatic_;
		CScenario Scenario_;
		CCustomDeviceCPPContainer CustomDeviceCPP;

		CDynaModel(const DynaModelParameters& ExternalParameters = {});
		virtual ~CDynaModel();
		bool RunTransient();
		bool RunTest();
		bool RunLoadFlow();
		void SolveNewton(ptrdiff_t nMaxIts);
		void BuildMatrix(bool SkipRightHand = false);
		void BuildRightHand();
		void StoreStates();
		void RestoreStates();
		const CDeviceContainer* ClosestZeroCrossingContainer() const { return m_pClosestZeroCrossingContainer; }
		void BuildDerivatives();
		void SolveLinearSystem(bool Refined = false);
		void NewtonUpdateDevices();
		void InitDevicesNordsiek();
		double FindZeroCrossingToConst(const RightVector* pRightVector, double dConst);
		double FindZeroCrossingOfDifference(const RightVector* pRightVector1, const RightVector* pRightVector2);
		double FindZeroCrossingOfModule(const RightVector* pRvre, const RightVector* pRvim, double Const, bool bCheckForLow);
		double NextStepValue(const RightVector* pRightVector);

		// выполняет предварительную инициализацию устройств: 
		// расчет внутренних констант, которые не зависят от связанных устройств
		// и валидация
		void PreInitDevices();
		void InitDevices();
		void InitEquations();

		ptrdiff_t AddMatrixSize(ptrdiff_t nSizeIncrement);
		void SetElement(const VariableIndexBase& Row, const VariableIndexBase& Col, double dValue);
		void SetElement(const VariableIndexBase& Row, const InputVariable& Col, double dValue);

		// Для теста с множителями
		//bool SetElement2(ptrdiff_t nRow, ptrdiff_t nCol, double dValue, bool bAddToPrevious = false);

		void SetFunction(const VariableIndexBase& Row, double dValue);
		void SetFunctionDiff(const VariableIndexBase& Row, double dValue);
		void SetDerivative(const VariableIndexBase& Row, double dValue);
		void CorrectNordsiek(ptrdiff_t nRow, double dValue);
		// Задает значение для переменной, и компонентов Нордиска. 
		// Используется, в основном, для ввода ограничения переменной
		void SetVariableNordsiek(const VariableIndex& Variable, double v0, double v1 = 0.0, double v2 = 0.0);
		// Копирует переменную вместе с компонентами Нордсика
		void CopyVariableNordsiek(const VariableIndex& Variable, const InputVariable& Source);

		// возвращает значение правой части системы уравнений
		double GetFunction(ptrdiff_t nRow) const;
		inline struct RightVector* GetRightVector() { return pRightVector; }
		struct RightVector* GetRightVector(const ptrdiff_t nRow);
		struct RightVector* GetRightVector(const VariableIndexBase& Variable);
		struct RightVector* GetRightVector(const InputVariable& Variable);

		template<class T>
		struct RangeAdapter
		{
		protected:
			T* begin_;
			const T* const end_;
		public:
			RangeAdapter(T* begin, const T* const end) : begin_(begin), end_(end) {}
			T* begin() { return begin_; }
			const T* end() const { return end_; }
			inline bool empty() const { return begin_ < end_; }
			inline T* operator++ (int) { auto temp{ begin_ };  begin_++; return temp; }
			inline T* operator++ () { begin_++; return begin_; }
			std::size_t size() const { return end_ - begin_; }
			inline T* operator-> () { return begin_; }
			inline operator T* () { return begin_; }
		};

		using RightVectorRangeT = RangeAdapter<RightVector>;
		using BRangeT = RangeAdapter<double>;

		RightVectorRangeT RightVectorRange();

		BRangeT BRange();

		StepControl& StepControl()
		{
			return sc;
		}

		inline DEVICECONTAINERS& DeviceContainersPredict()
		{
			return DeviceContainersPredict_;
		}

		inline ptrdiff_t MatrixSize() const
		{
			return klu.MatrixSize();
		}

		inline const double* B() const
		{
			return klu.B();
		}

		inline double* B()
		{
			return klu.B();
		}

		inline constexpr double GetOmega0() const
		{
			return 2 * 50.0 * M_PI;
		}

		inline ptrdiff_t Order() const
		{
			return sc.q;
		}

		inline double H() const
		{
			return sc.H();
		}

		inline double UsedH() const
		{
			return sc.UsedH();
		}

		void StoreUsedH()
		{
			sc.StoreUsedH();
		}

		inline bool StepFromReset() const
		{
			return sc.StepFromReset_;
		}

		void ResetStep(bool Reset = true)
		{
			sc.StepFromReset_ = Reset;
		}

		[[nodiscard]] static bool IsSSE2Available();

		//  возвращает отношение текущего шага к новому
		inline double SetH(double h)
		{
			h = (std::min)(h, m_Parameters.Hmax);
			const double ratio{ H() > 0 ? h / H() : 1.0};
			sc.SetH(h);
			Integrator_->UpdateStepSize();
			return ratio;
		}

		inline double GetFreqTimeConstant() const
		{
			return m_Parameters.m_dFrequencyTimeConstant;
		}

		inline CDynaLRC* GetLRCYconst() const
		{
			return pLRCYconst_;
		}

		inline CDynaLRC* GetLRCIconst() const
		{
			return pLRCIconst_;
		}

		inline CDynaLRC* GetLRCSconst() const
		{
			return pLRCSconst_;
		}

		inline CDynaLRC* GetLRCGen() const
		{
			return pLRCGen_;
		}

		inline CDynaLRC* GetLRCDynamicDefault()
		{
			return pLRCLoad_;
		}

		inline double GetMustangDerivativeTimeConstant() const
		{
			return m_Parameters.m_dMustangDerivativeTimeConstant;
		}

		inline ACTIVE_POWER_DAMPING_TYPE GetFreqDampingType() const
		{
			return m_Parameters.eFreqDampingType;
		}

		inline const char* GetDampingName() const
		{
			_ASSERTE(m_cszDampingName);
			return m_cszDampingName;
		}

		inline double DerlagTolerance() const
		{
			return m_Parameters.DerLagTolerance_;
		}

		template <typename... Vars>
		void DebugDump(const CDevice& device, const Vars& ... vars)
		{
			if (DebugLogFile.is_open())
			{
				DebugLogFile << GetCurrentTime() << ";";
				DebugLogFile << GetIntegrationStepNumber() << ";";
				for (const auto& p : { vars... })
					DebugLogFile << p.Value << ";";
				DebugLogFile << std::endl;
			}
		}

		template <typename... Vars>
		void CountConstElementsToSkip(const Vars& ... vars) 
		{
			for (const auto& p : { vars... }) 
			{
				if (p.Index >= m_nEstimatedMatrixSize || p.Index < 0)
					throw dfw2error(fmt::format("CDynaModel::CountConstElementsToSkip matrix size overrun Row {} MatrixSize {}", p.Index, m_nEstimatedMatrixSize));
				MatrixRow* pRow{ m_pMatrixRows + p.Index };
				pRow->m_nConstElementsToSkip = pRow->pAp - pRow->pApRow;
			}
		}
		template <typename... Vars>
		void SkipConstElements(const Vars& ... vars)
		{
			for (const auto& p : { vars... })
			{
				if (p.Index >= m_nEstimatedMatrixSize || p.Index < 0)
					throw dfw2error(fmt::format("CDynaModel::SkipConstElements matrix size overrun Row {} MatrixSize {}", p.Index, m_nEstimatedMatrixSize));
				MatrixRow* pRow{ m_pMatrixRows + p.Index };
				pRow->pAp = pRow->pApRow + pRow->m_nConstElementsToSkip;
				pRow->pAx = pRow->pAxRow + pRow->m_nConstElementsToSkip; 
			}
		}
						
		inline bool FillConstantElements() const
		{
			return sc.m_bFillConstantElements;
		}

		void PassTime()
		{
			m_Discontinuities.PassTime(GetCurrentTime());
		}
		
		inline bool EstimateBuild() const
		{
			return EstimateBuild_;
		}
		// возвращает тип метода для уравнения
		// используется для управления методом интегрирования дифференциальных переменных
		// алгебраические уравнения всегда интегрируются BDF. Дифференциальные - ADAMS или BDF
		inline DEVICE_EQUATION_TYPE GetDiffEquationType() const
		{
			return m_Parameters.m_eDiffEquationType;
		}

		// возвращает напряжение перехода СХН на шунт
		double GetLRCToShuntVmin() const
		{
			return (std::min)(m_Parameters.m_dLRCToShuntVmin,1.0);
		}

		bool AllowUserToOverrideStandardLRC()
		{
			return m_Parameters.m_bAllowUserOverrideStandardLRC;
		}

		bool ConsiderDampingEquation() const
		{
			return m_Parameters.m_bConsiderDampingEquation;
		}

		// возвращает флаг кумулятивности дискретных изменений
		inline double ChangeActionsAreCumulative() const
		{
			return m_Parameters.ChangeActionsAreCumulative_;
		}

		// возвращает абсолютную точность из параметров
		inline double Atol() const
		{
			return m_Parameters.m_dAtol;
		}

		// возвращает относительный гистерезиса
		inline double HysteresisRtol() const
		{
			return HysteresisRtol_;
		}

		// возвращает абсолютный гистерезис
		inline double HysteresisAtol() const
		{
			return HysteresisAtol_;
		}

		// возвращает относительную точность из параметров
		inline double Rtol() const
		{
			return m_Parameters.m_dRtol;
		}
		// Относительная погрешность решения УР по напряжению
		// с помощью линейного метода
		inline double RtolLULF() const
		{
			return m_Parameters.m_dRtol;
		}

		inline double GetZeroBranchImpedance() const
		{
			return m_Parameters.m_dZeroBranchImpedance;
		}

		inline double GetZeroCrossingInRange(double rH) const
		{
			return rH > 0.0 && rH < 1.0;
		}

		inline double GetZeroCrossingTolerance() const
		{
			return ((sc.Hmin / H()) > 0.999) ? (std::numeric_limits<double>::max)() : 0.95;
		}

		// Текущий номер итерации Ньютона
		inline ptrdiff_t GetNewtonIterationNumber() const
		{
			return sc.nNewtonIteration;
		}

		// Текущее количество шагов интегрирования
		inline ptrdiff_t GetIntegrationStepNumber() const
		{
			return sc.nStepsCount;
		}

		// возвращает итоговое сообщение о результате расчета
		const std::string& FinalMessage() const
		{
			return FinalMessage_;
		}

		// возвращает путь к файлу результатов
		std::filesystem::path ResultFilePath() const
		{
			return ResultFilePath_;
		}

		// возвращает причину завершения расчета ранее заданной длительности
		const eSyncLossCause SyncLossCause() const
		{
			return SyncLossCause_;
		}

		// возвращает текущее время интегрирования
		inline double GetCurrentTime() const
		{
			return sc.t;
		}

		// возвращает true если время положительно
		// с учетом минимального шага
		inline bool PositiveTime() const
		{
			return sc.t > -sc.Hmin;
		}

		// возвращает минимальный шаг
		inline double Hmin() const
		{
			return sc.Hmin;
		}

		inline bool ZeroCrossingStepReached(double dHstep) const
		{
			return dHstep > 0.997;
		}

		inline double GetHysteresis(double dValue) const
		{
			return std::abs(dValue) * HysteresisRtol() + HysteresisAtol();
		}

		void StopProcess();
		void ProcessTopologyRequest();
		// запрос обработки разрыва от устройства
		void DiscontinuityRequest(CDevice& device, const DiscontinuityLevel Level);
		// запрос обработки разрыва от примитива (который, в свою очередь, должен быть привязан к устройству)
		void DiscontinuityRequest(CDynaPrimitive &primitive, const DiscontinuityLevel Level);
		void ServeDiscontinuityRequest();
		bool SetStateDiscontinuity(CDiscreteDelay *pDelayObject, double dDelay);
		bool RemoveStateDiscontinuity(CDiscreteDelay *pDelayObject);
		bool CheckStateDiscontinuity(CDiscreteDelay *pDelayObject);
		void NotifyRelayDelay(const CRelayDelayLogic* pRelayDelay);

		CDeviceContainer* GetDeviceContainer(eDFW2DEVICETYPE Type); 

		void EnterDiscontinuityMode();
		void LeaveDiscontinuityMode();
		void ProcessDiscontinuity();
		inline bool IsInDiscontinuityMode() { return sc.m_bDiscontinuityMode; }
		inline bool IsInZeroCrossingMode() { return sc.m_bZeroCrossingMode; }
		ptrdiff_t GetStepNumber() {  return sc.nStepsCount;  }
		void RebuildMatrix(bool bRebuild = true);
		void AddZeroCrossingDevice(CDevice *pDevice);

		void SetLogger(std::unique_ptr<CLogger> pLogger) {	m_pLogger = std::move(pLogger);	}
		void SetProgress(std::unique_ptr<CProgress> pProgress) { m_pProgress = std::move(pProgress); }

		void StartProgress();
		CProgress::ProgressStatus UpdateProgress();
		void EndProgress();

		// вывод Message в протокол
		void Log(DFW2MessageStatus Status, std::string_view Message, ptrdiff_t DbIndex = -1) const;
		// вывод Message в протокол с добавлением метки времени и номера шага интегрирования
		void LogTime(DFW2MessageStatus Status, std::string_view Message, ptrdiff_t DbIndex = -1) const;
		void DebugLog(std::string_view Message) const;

		static double gs1(KLUWrapper<double>& klu, std::unique_ptr<double[]>& Imb, const double* Sol);

		void PushVarSearchStack(CDevice*pDevice);
		bool PopVarSearchStack(CDevice* &pDevice);
		void ResetStack();

		bool InitExternalVariable(VariableIndexExternal& ExtVar, CDevice* pFromDevice, std::string_view Name);
		CDevice* GetDeviceBySymbolicLink(std::string_view Object, std::string_view Keys, std::string_view SymLink);
		CDeviceContainer *GetContainerByAlias(std::string_view Alias);
		DEVICECONTAINERS GetContainersByAlias(std::string_view Alias);
		CAutomatic& Automatic();
		CAutomatic& Scenario();

		// выдать в лог топ-nCount переменных по количеству ошибок
		void GetWorstEquations(ptrdiff_t nCount);
		// выдать в лог топ-nCount устройств, вызывавших zero-crossing
		void GetTopZeroCrossings(ptrdiff_t nCount);
		// выдать в лог топ устройств, запрашивавших обработку разрывов
		void GetTopDiscontinuityRequesters(ptrdiff_t nCount);

		void Serialize(const std::filesystem::path path);
		void DeSerialize(const std::filesystem::path path);
		void DeserializeParameters(const std::filesystem::path path);

		// возвращает true, если расчет нужно прекратить (отмена пользователем)
		inline bool CancelProcessing()
		{
			return bStopProcessing;
		}
		bool StabilityLost();
		bool OscillationsDecayed();

		const CPlatformFolders& Platform() const
		{
			return m_Platform;
		}

		void CheckFolderStructure();

		const std::string& ModelName() const
		{
			return ModelName_;
		}

		void SetModelName(const std::string_view ModelName)
		{
			ModelName_ = ModelName;
		}

		unsigned long ThreadId() const
		{
			return m_Parameters.ThreadId_;
		}

		const std::filesystem::path& ModuleFilePath() const
		{
			return m_Parameters.ModuleFilePath_;
		}

		const DynaModelParameters& Parameters() const
		{
			return m_Parameters;
		}

		SerializerPtr GetParametersSerializer()
		{
			return m_Parameters.GetSerializer();
		}

		void WriteSlowVariable(ptrdiff_t nDeviceType,
			const ResultIds& DeviceIds,
			const std::string_view VariableName,
			double Value,
			double PreviousValue,
			const std::string_view ChangeDescription);

		void SnapshotRightVector();
		void CompareRightVector();

		void CreateZeroLoadFlow();

		static constexpr const VersionInfo version = PackageVersion;
		static bool VersionsEquals(const DFW2::VersionInfo& v1, const DFW2::VersionInfo& v2)
		{
			auto fnVersionTie = [](const DFW2::VersionInfo& v)
			{
				return std::tie(v[0], v[1], v[2], v[3]);
			};
			return fnVersionTie(v1) == fnVersionTie(v2);
		}

		static constexpr const char* cszJson = "json";
	};
}
