/*
* Raiden - программа для моделирования длительных электромеханических переходных процессов в энергосистеме
* (С) 2018 - 2021 Евгений Машалов
* Raiden - long-term power system transient stability simulation module
* (C) 2018 - 2021 Eugene Mashalov
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
#include "FmtComplexFormat.h"
#include "PlatformFolders.h"
#include "LoadFlow.h"
#include "version.h"
#include "DynaBranch.h"
#include "Statistics.h"
#include "Logger.h"
#include "NodeMeasures.h"

namespace DFW2
{

	class CDynaModel
	{
	public:

		struct DynaModelParameters : public CLoadFlow::LoadFlowParameters
		{
			
			DynaModelParameters() : CLoadFlow::LoadFlowParameters() {}

			ACTIVE_POWER_DAMPING_TYPE eFreqDampingType = ACTIVE_POWER_DAMPING_TYPE::APDT_ISLAND;
			DEVICE_EQUATION_TYPE m_eDiffEquationType = DEVICE_EQUATION_TYPE::DET_DIFFERENTIAL;
			double m_dFrequencyTimeConstant = 0.02;
			double m_dLRCToShuntVmin = 0.5;
			double m_dZeroCrossingTolerance = 0.0;
			bool m_bDontCheckTolOnMinStep = false;
			bool m_bConsiderDampingEquation = false;
			double m_dOutStep = 0.01;
			ptrdiff_t nVarSearchStackDepth = 100;
			double m_dAtol = DFW2_ATOL_DEFAULT;
			double m_dRtol = DFW2_RTOL_DEFAULT;
			double m_dRefactorByHRatio = 1.5;
			double m_dMustangDerivativeTimeConstant = 1E-4;
			// режим подавления рингинга
			ADAMS_RINGING_SUPPRESSION_MODE m_eAdamsRingingSuppressionMode = ADAMS_RINGING_SUPPRESSION_MODE::ARSM_GLOBAL;
			ptrdiff_t m_nAdamsIndividualSuppressionCycles = 3;			// количество перемен знака переменной для обнаружения рингинга
			ptrdiff_t m_nAdamsGlobalSuppressionStep = 10;				// номер шага, на кратном которому работает глобальное подавление рингинга
			ptrdiff_t m_nAdamsIndividualSuppressStepsRange = 5;			// количество шагов, на протяжении которого работает индивидуальное подавление рингинга переменной
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
			std::string m_strWorkingFolder = "Русский тест";
			std::string m_strResultsFolder = "";
			DFW2MessageStatus m_eConsoleLogLevel = DFW2MessageStatus::DFW2LOG_ERROR;
			DFW2MessageStatus m_eFileLogLevel = DFW2MessageStatus::DFW2LOG_DEBUG;
			PARK_PARAMETERS_DETERMINATION_METHOD m_eParkParametersDetermination = PARK_PARAMETERS_DETERMINATION_METHOD::NiiptTo;
			GeneratorLessLRC m_eGeneratorLessLRC = GeneratorLessLRC::Iconst;
			double m_dProcessDuration = 150.0;
		};

		struct Parameters : public DynaModelParameters
		{
			Parameters() { }
			// сериализатор и валидатор
			SerializerPtr GetSerializer();
			SerializerValidatorRulesPtr GetValidator();

			static constexpr const char* m_cszLFFormulationTypeNames[3] = { "Power", "Current", "Tanh" };
			static constexpr const char* m_cszDiffEquationTypeNames[2] = { "Algebraic", "Differential" };
			static constexpr const char* m_cszLogLevelNames[7] = { "none", "fatal", "error", "warning", "message", "info", "debug" };
			static constexpr const char* m_cszAdamsRingingSuppressionNames[4] = { "None", "Global", "Individual", "DampAlpha" };
			static constexpr const char* m_cszParkParametersDeterminationMethodNames[4] = { "Kundur", "NiiptTo", "NiiptToTd", "Canay" };
			static constexpr const char* m_cszFreqDampingNames[2] = { "Node", "Island" };
			static constexpr const char* m_cszGeneratorLessLRCNames[2] = { "S","I" };

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
			static constexpr const char* m_cszLFStartup = "Startup";
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

			static inline CValidationRuleRange ValidatorRange01 = CValidationRuleRange(0, 1);
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

		struct ConvergenceTest
		{
			ptrdiff_t nVarsCount;
			volatile double dErrorSum;
			double dOldErrorSum;
			double dCm;
			double dCms;
			double dOldCm;
			double dErrorSums;
			volatile double dKahanC;

			void Reset()
			{
				dErrorSum = 0.0;
				nVarsCount = 0;
				dKahanC = 0.0;
			}

			void GetConvergenceRatio()
			{
				GetRMS();
				dCms = 0.7;
				if (!Equal(dOldErrorSum,0.0))
				{
					dCm = dErrorSum / dOldErrorSum;
					dCms = (std::max)(0.2 * dOldCm, dCm);
				}
				dErrorSums = dErrorSum * (std::min)(1.0, 1.5 * dCms); 
			}

			void GetRMS()
			{
				if (nVarsCount)
				{
					dErrorSum /= static_cast<double>(nVarsCount);
					dErrorSum = sqrt(dErrorSum);
				}
				else
					dErrorSum = dErrorSums = 0.0;
			}

			void ResetIterations()
			{
				dOldErrorSum = 0.0;
				dOldCm = 0.7;
			}

			void NextIteration()
			{
				dOldErrorSum = dErrorSum;
				dOldCm = dCm;
			}

			void AddErrorNeumaier(double dError)
			{
				volatile double t = dErrorSum + dError;
				if (std::abs(dErrorSum) >= std::abs(dError))
					dKahanC += (dErrorSum - t) + dError;
				else
					dKahanC += (dError - t) + dErrorSum;
				dErrorSum = t;
			}

			void AddErrorKahan(double dError)
			{
				volatile double y = dError - dKahanC;
				volatile double t = dErrorSum + y;
				dKahanC = (t - dErrorSum) - y;
				dErrorSum = t;
			}

			void AddErrorStraight(double dError)
			{
				dErrorSum += dError;
			}

			void AddError(double dError);
			void FinalizeSum();

			// обработка диапазона тестов сходимости в массиве

			typedef ConvergenceTest ConvergenceTestVec[2];

			static void ProcessRange(ConvergenceTestVec &Range, void(*ProcFunc)(ConvergenceTest& ct))
			{
				for (auto&& it : Range) ProcFunc(it);
			}

			static void Reset(ConvergenceTest& ct) { ct.Reset(); }
			static void GetConvergenceRatio(ConvergenceTest& ct) { ct.GetConvergenceRatio(); }
			static void NextIteration(ConvergenceTest& ct) { ct.NextIteration(); }
			static void FinalizeSum(ConvergenceTest& ct) { ct.FinalizeSum(); }
			static void GetRMS(ConvergenceTest& ct) { ct.GetRMS(); }
			static void ResetIterations(ConvergenceTest& ct) { ct.ResetIterations(); }
		};

		struct StepError
		{
			struct Error
			{
				RightVector *pVector = nullptr;
				double dMaxError = 0.0;

				void Reset()
				{
					pVector = nullptr;
					dMaxError = 0.0;
				}

				void Update(RightVector* pRightVector, double dError)
				{
					if (pVector == nullptr || dError > dMaxError)
					{
						dMaxError = dError;
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
			DiscontinuityLevel m_eDiscontinuityLevel = DiscontinuityLevel::None;
			const CDevice* m_pDiscontinuityDevice = nullptr;
			bool m_bEnforceOut = false;
			bool m_bBeforeDiscontinuityWritten = false;				// флаг обработки момента времени до разрыва
			double dFilteredStep = 0.0;
			double dFilteredOrder = 0.0;
			double dFilteredStepInner =0.0;								// фильтр минимального шага на серии шагов
			double dFilteredOrderInner =0.0;
			double dRateGrowLimit = FLT_MAX;
			ptrdiff_t nStepsCount = 0;
			ptrdiff_t nNewtonIterationsCount = 0;
			double dLastConditionNumber = 1.0;
			double dMaxConditionNumber = 0.0;
			double dMaxConditionNumberTime = 0.0;
			double dMaxSLEResidual = 0.0;
			double dMaxSLEResidualTime = 0.0;
			_OrderStatistics OrderStatistics[2];
			ptrdiff_t nDiscontinuityNewtonFailures = 0;
			ptrdiff_t nMinimumStepFailures = 0;
			double m_dCurrentH = 0.0;
			double m_dOldH = -1.0;
			double m_dStoredH = 0.0;
			ptrdiff_t q = 1;
			double t = 0.0;
			double t0 = 0.0;
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

			StepError Newton;
			StepError Integrator;
			double m_dLastRefactorH = 0.0;
			bool bRingingDetected = false;
			bool m_bNordsiekReset = true;		// флаг сброса Нордсика - производные равны нулю, контроль предиктора не выполняется

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
				dFilteredStep = dFilteredStepInner = FLT_MAX;
			}

			bool FilterStep(double dStep);
			bool FilterOrder(double dStep);

			// устанавливает ограничение изменения порядка на заданное количество шагов
			// сбрасывает фильтр порядка
			void OrderChanged()
			{
				nStepsToOrderChange = nStepsToOrderChangeParameter;
				dFilteredOrderInner = dFilteredOrder = FLT_MAX;
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
				//return t + m_dCurrentH;
				return t + m_dCurrentH - KahanC;
			}

			// рассчитывает текущее время перед выполнением шага
			inline void Advance_t0()
			{
				// математически функция выполняет t = t0 + m_dCurrentH;

				// но мы используем Kahan summation для устранения накопленной ошибки
				volatile double ky = m_dCurrentH - KahanC;
				volatile double temp = t0 + ky;
				// предополагается, что шаг не может быть отменен
				// и поэтому сумма Кэхэна обновляется
				KahanC = (temp - t0) - ky;
				t = temp;

				_OrderStatistics& os = OrderStatistics[q - 1];
				ky = m_dCurrentH - os.dTimePassedKahan;
				temp = os.dTimePassed + ky;
				os.dTimePassedKahan = (temp - os.dTimePassed) - ky;
				os.dTimePassed = temp;

				_ASSERTE(std::abs(OrderStatistics[0].dTimePassed + OrderStatistics[1].dTimePassed - t) < DFW2_EPSILON);
			}

			// рассчитывает текущее время перед выполнением шага, с возможностью возврата
			// к предыдущему времени
			inline void CheckAdvance_t0()
			{
				//t = t0 + m_dCurrentH;

				// do not update KahanC, as this check time
				// can be disregarded;
				double ky = m_dCurrentH - KahanC;
				double temp = t0 + ky;
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
		//CResultsWriterABI m_ResultsWriter;
#else
		CResultsWriterABI m_ResultsWriter;
#endif		
		std::map<ptrdiff_t, std::set<ptrdiff_t>> m_MatrixChecker;
		std::vector<RightVector> m_RightVectorSnapshot;

		KLUWrapper<double> klu;
		CDynaLRC *m_pLRCGen		= nullptr;		// СХН для генераторных узлов без генераторов
		CDynaLRC *m_pLRCLoad	= nullptr;		// СХН для узлов без динамической СХН

		std::unique_ptr<MatrixRow[]> m_pMatrixRowsUniq;
		std::unique_ptr<RightVector[]> pRightVectorUniq;
		std::unique_ptr<RightVectorTotal[]> pRightVectorTotal;
		RightVector *pRightVector = nullptr;
		MatrixRow *m_pMatrixRows = nullptr;

		DEVICECONTAINERS m_DeviceContainers;
		DEVICECONTAINERS m_DeviceContainersNewtonUpdate;
		DEVICECONTAINERS m_DeviceContainersPredict;
		DEVICECONTAINERS m_DeviceContainersFinishStep;

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

		bool m_bEstimateBuild;
		bool m_bRebuildMatrixFlag;
		std::vector<CDevice*> ZeroCrossingDevices;
		void ConvertToCCSMatrix();
		void SolveLinearSystem();
		void SolveRefine();
		void UpdateRcond();
		void SetDifferentiatorsTolerance();
		bool NewtonUpdate();
		bool SolveNewton(ptrdiff_t nMaxIts);
		void EstimateMatrix();
		void CreateTotalRightVector();
		void UpdateTotalRightVector();
		void UpdateNewRightVector();
		void DebugCheckRightVectorSync();
		void NewtonUpdateDevices();

		double GetRatioForCurrentOrder();
		double GetRatioForHigherOrder();
		double GetRatioForLowerOrder();
		void   ChangeOrder(ptrdiff_t Newq);
		void   Computehl0(); // рассчитывает кэшированные произведения l0 * GetH для элементов матрицы

		bool Step();

		double GetNorm(double *pVector);
		double GetWeightedNorm(double *pVector);

		void PrepareNetworkElements();
		bool Link();
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

		void Predict();
		void InitNordsiek();
		void InitDevicesNordsiek();
		static void InitNordsiekElement(struct RightVector *pVectorBegin, double Atol, double Rtol);
		static void PrepareNordsiekElement(struct RightVector *pVectorBegin);
		void RescaleNordsiek(const double r);
		void UpdateNordsiek(bool bAllowSuppression = false);
		bool DetectAdamsRinging();
		void SaveNordsiek();
		void RestoreNordsiek();
		void ConstructNordsiekOrder();
		void ReInitializeNordsiek();
		void ResetNordsiek();
		void BuildDerivatives();
		void BuildMatrix();
		void BuildRightHand();
		double CheckZeroCrossing();
		void FinishStep();

		void WriteResultsHeader();
		void WriteResults();
		void FinishWriteResults();

		ConvergenceTest::ConvergenceTestVec ConvTest;
		struct StepControl sc;

		bool SetFunctionEqType(ptrdiff_t nRow, double dValue, DEVICE_EQUATION_TYPE EquationType);
		void EnableAdamsCoefficientDamping(bool bEnable);
		void GoodStep(double rSame);
		void BadStep();
		void NewtonFailed();
		void RepeatZeroCrossing();
		void UnprocessDiscontinuity();

		bool LoadFlow();
		void DumpStateVector();
		FILE* fResult;
		mutable std::ofstream LogFile;
		static bool ApproveContainerToWriteResults(CDeviceContainer *pDevCon);

		std::atomic<bool> bStopProcessing = false;

		double m_dTimeWritten;
		const char* m_cszDampingName = nullptr;

		COscDetectorBase m_OscDetector;

		CDeviceContainer *m_pClosestZeroCrossingContainer = nullptr;

		void SetElement(ptrdiff_t nRow, ptrdiff_t nCol, double dValue, bool bAddToPrevious);
		void SetElement(ptrdiff_t nRow, ptrdiff_t nCol, double dValue);
		void SetFunction(ptrdiff_t nRow, double dValue);
		void SetFunctionDiff(ptrdiff_t nRow, double dValue);
		void SetDerivative(ptrdiff_t nRow, double dValue);

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
		CDeviceContainer GeneratorsMotion;
		CDeviceContainer GeneratorsInfBus;
		CDeviceContainer SynchroZones;
		CDeviceContainer ExcitersMustang;
		CDeviceContainer DECsMustang;
		CDeviceContainer ExcConMustang;
		CDeviceContainer BranchMeasures;
		CDeviceContainer NodeMeasures;
		CDynaNodeZeroLoadFlowContainer ZeroLoadFlow;
		CCustomDeviceContainer CustomDevice;
		CCustomDeviceCPPContainer AutomaticDevice;
		CAutomatic m_Automatic;
		CCustomDeviceCPPContainer CustomDeviceCPP;

		CDynaModel(const DynaModelParameters& ExternalParameters = {});
		virtual ~CDynaModel();
		bool RunTransient();
		bool RunLoadFlow();

		// выполняет предварительную инициализацию устройств: 
		// расчет внутренних констант, которые не зависят от связанных устройств
		// и валидация
		void PreInitDevices();
		void InitDevices();
		bool InitEquations();

		ptrdiff_t AddMatrixSize(ptrdiff_t nSizeIncrement);
		void SetElement(const VariableIndexBase& Row, const VariableIndexBase& Col, double dValue);
		void SetElement(const VariableIndexBase& Row, const InputVariable& Col, double dValue);

		// Для теста с множителями
		//bool SetElement2(ptrdiff_t nRow, ptrdiff_t nCol, double dValue, bool bAddToPrevious = false);

		void SetFunction(const VariableIndexBase& Row, double dValue);
		void SetFunctionDiff(const VariableIndexBase& Row, double dValue);
		void SetDerivative(const VariableIndexBase& Row, double dValue);


		void CorrectNordsiek(ptrdiff_t nRow, double dValue);

		// возвращает значение правой части системы уравнений
		double GetFunction(ptrdiff_t nRow);
		struct RightVector* GetRightVector(const ptrdiff_t nRow);
		struct RightVector* GetRightVector(const VariableIndexBase& Variable);
		struct RightVector* GetRightVector(const InputVariable& Variable);

		inline double GetOmega0() const
		{
			return 2 * 50.0 * M_PI;
		}

		inline ptrdiff_t GetOrder() const
		{
			return sc.q;
		}

		inline double GetH() const
		{
			return sc.m_dCurrentH;
		}

		inline bool IsNordsiekReset() const
		{
			return sc.m_bNordsiekReset;
		}

		inline void SetH(double h)
		{
			sc.m_dCurrentH = h;
			Computehl0();
		}

		inline double GetOldH() const
		{
			return sc.m_dOldH;
		}


		inline double GetFreqTimeConstant() const
		{
			return m_Parameters.m_dFrequencyTimeConstant;
		}

		inline CDynaLRC* GetLRCGen()
		{
			return m_pLRCGen;
		}

		inline CDynaLRC* GetLRCDynamicDefault()
		{
			return m_pLRCLoad;
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

		bool CountConstElementsToSkip(ptrdiff_t nRow);
		bool SkipConstElements(ptrdiff_t nRow);

		inline bool FillConstantElements() const
		{
			return sc.m_bFillConstantElements;
		}
		
		inline bool EstimateBuild() const
		{
			return m_bEstimateBuild;
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
		// возвращает абсолютную точность из параметров
		inline double GetAtol() const
		{
			return m_Parameters.m_dAtol;
		}
		// возвращает относительную точность из параметров
		inline double GetRtol() const
		{
			return m_Parameters.m_dRtol;
		}
		// Относительная погрешность решения УР по напряжению
		// с помощью линейного метода
		inline double GetRtolLULF() const
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
			return ((sc.Hmin / sc.m_dCurrentH) > 0.999) ? FLT_MAX : 100.0 * m_Parameters.m_dAtol;
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

		// возвращает текущее время интегрирования
		inline double GetCurrentTime() const
		{
			return sc.t;
		}

		inline bool ZeroCrossingStepReached(double dHstep) const
		{
			return dHstep > 0.997;
		}

		inline double GetHysteresis(double dValue) const
		{
			return std::abs(dValue) * GetRtol() * 0.01 + GetAtol() * 10.0;
		}
		void StopProcess();
		void ProcessTopologyRequest();
		void DiscontinuityRequest(CDevice& device, const DiscontinuityLevel Level);
		void ServeDiscontinuityRequest();
		bool SetStateDiscontinuity(CDiscreteDelay *pDelayObject, double dDelay);
		bool RemoveStateDiscontinuity(CDiscreteDelay *pDelayObject);
		bool CheckStateDiscontinuity(CDiscreteDelay *pDelayObject);
		void NotifyRelayDelay(const CRelayDelayLogic* pRelayDelay);

		CDeviceContainer* GetDeviceContainer(eDFW2DEVICETYPE Type); 

		void EnterDiscontinuityMode();
		void LeaveDiscontinuityMode();
		bool ProcessDiscontinuity();
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

		void Log(DFW2MessageStatus Status, std::string_view Message, ptrdiff_t nDbIndex = -1) const;

		double Methodl[4][4];
		double Methodlh[4];
		static const double MethodlDefault[4][4];

		static double gs1(KLUWrapper<double>& klu, std::unique_ptr<double[]>& Imb, const double* Sol);

		void PushVarSearchStack(CDevice*pDevice);
		bool PopVarSearchStack(CDevice* &pDevice);
		void ResetStack();

		bool InitExternalVariable(VariableIndexExternal& ExtVar, CDevice* pFromDevice, std::string_view Name);
		CDevice* GetDeviceBySymbolicLink(std::string_view Object, std::string_view Keys, std::string_view SymLink);
		CDeviceContainer *GetContainerByAlias(std::string_view Alias);
		DEVICECONTAINERS GetContainersByAlias(std::string_view Alias);
		CAutomatic& Automatic();

		// выдать в лог топ-nCount переменных по количеству ошибок
		void GetWorstEquations(ptrdiff_t nCount);
		// выдать в лог топ-nCount устройств, вызывавших zero-crossing
		void GetTopZeroCrossings(ptrdiff_t nCount);
		// выдать в лог топ устройств, запрашивавших обработку разрывов
		void GetTopDiscontinuityRequesters(ptrdiff_t nCount);

		void Serialize(const std::filesystem::path path);
		void DeSerialize(const std::filesystem::path path);

		// возвращает true, если расчет нужно прекратить (отмена пользователем)
		inline bool CancelProcessing()
		{
			return bStopProcessing;
		}
		bool StabilityLost();
		bool OscillationsDecayed();

		// кастомное деление по модулю для периодизации угла
		template<typename T>
		static T Mod(T x, T y);
		// возврщает угол приведенный к диапазону [-pi;pi) (удаление периодов)
		inline static double WrapPosNegPI(double fAng);

		const CPlatformFolders& Platform() const
		{
			return m_Platform;
		}

		void CheckFolderStructure();

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

		static constexpr const VersionInfo version = { { 1, 0, 0, 1 } };

	};
}
