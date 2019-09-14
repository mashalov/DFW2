#pragma once
#include "DynaGeneratorMustang.h"
#include "DynaGeneratorInfBus.h"
#include "DynaExciterMustang.h"
#include "DynaDECMustang.h"
#include "DynaExcConMustang.h"
#include "DynaBranch.h"
#include "Discontinuities.h"
#include "CustomDevice.h"
#include "Automatic.h"
#include "klu.h"
#include "klu_version.h"
#include "cs.h"
#include "Results.h"
#include "chrono"

namespace DFW2
{
	class CDynaModel
	{
	protected:
		struct MatrixRow
		{
			ptrdiff_t m_nColsCount;
			double *pAxRow , *pAx;
			ptrdiff_t *pApRow, *pAp;
			ptrdiff_t m_nConstElementsToSkip;
			MatrixRow() : m_nColsCount(NULL) 
			{

			};

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
					dCms = max(0.2 * dOldCm, dCm);
				}
				dErrorSums = dErrorSum * min(1.0, 1.5 * dCms); 
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
				if (fabs(dErrorSum) >= fabs(dError))
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
				nVarsCount++;
			}

			void AddError(double dError);
			void FinalizeSum();
		};

		struct StepError
		{
			CDevice *pMaxErrorDevice;
			double *pMaxErrorVariable;
			double dMaxErrorVariable;
			ptrdiff_t nMaxErrorVariableEquation;

			void Reset()
			{
				pMaxErrorDevice = NULL;
				pMaxErrorVariable = NULL;
				dMaxErrorVariable = 0.0;
				nMaxErrorVariableEquation = -1;
			}

			StepError()
			{
				Reset();
			}
				 
		};
		

		struct StepControl
		{
			enum eIterationMode
			{
				NEWTON,
				JN,
				FUNCTIONAL
			} 
			IterationMode;

			struct OrderStatistics
			{
				ptrdiff_t nSteps;
				ptrdiff_t nFailures;
				ptrdiff_t nNewtonFailures;
				double dTimePassed;

				OrderStatistics() : nSteps(0),
									nFailures(0),
									nNewtonFailures(0),
									dTimePassed(0.0) 
									{ }
			};


			bool m_bRefactorMatrix;
			bool m_bFillConstantElements;
			bool m_bNewtonConverged;
			bool m_bNordsiekSaved;
			bool m_bNewtonDisconverging;
			bool m_bNewtonStepControl;
			bool m_bDiscontinuityMode;
			bool m_bZeroCrossingMode;
			bool m_bRetryStep;
			bool m_bStopCommandReceived;
			bool m_bProcessTopology;
			bool m_bDiscontinuityRequest;
			bool m_bEnforceOut;
			bool m_bBeforeDiscontinuityWritten;						// флаг обработки момента времени до разрыва
			double dFilteredStep;
			double dFilteredOrder;
			double dFilteredStepInner;								// фильтр минимального шага на серии шагов
			double dFilteredOrderInner;
			double dRateGrowLimit;
			ptrdiff_t nStepsCount;
			ptrdiff_t nNewtonIterationsCount;
			ptrdiff_t nFactorizationsCount;
			ptrdiff_t nAnalyzingsCount;
			double dMaxConditionNumber;
			double dMaxConditionNumberTime;
			OrderStatistics OrderStatistics[2];
			ptrdiff_t nDiscontinuityNewtonFailures;
			ptrdiff_t nMinimumStepFailures;
			double m_dCurrentH;
			double m_dOldH;
			double m_dStoredH;
			ptrdiff_t q;
			double t;
			double t0;
			volatile double KahanC;
			ptrdiff_t nStepsToStepChangeParameter;
			ptrdiff_t nStepsToOrderChangeParameter;
			ptrdiff_t nStepsToFailParameter;
			ptrdiff_t nStepsToStepChange;
			ptrdiff_t nStepsToOrderChange;
			ptrdiff_t nStepsToFail;
			ptrdiff_t nSuccessfullStepsOfNewton;
			ptrdiff_t nStepsToEndRateGrow;
			ptrdiff_t nNewtonIteration;
			double dRightHandNorm;
			chrono::time_point<chrono::high_resolution_clock> m_ClockStart;

			double Hmin;

			StepError Newton;
			StepError Integrator;
			double m_dLastRefactorH;

			StepControl()
			{
				dRateGrowLimit = FLT_MAX;
				m_bProcessTopology = false;
				m_bDiscontinuityRequest = false;
				m_bStopCommandReceived = false;
				nStepsToOrderChangeParameter = 4;
				nStepsToStepChangeParameter = 4;
				nStepsToFailParameter = 1;
				m_bDiscontinuityMode = false;
				m_bZeroCrossingMode = false;
				m_bRefactorMatrix = true;
				m_bBeforeDiscontinuityWritten = false;
				m_dOldH = -1.0;
				t = 0.0;
				KahanC = 0.0;
				Hmin = 1E-8;
				nSuccessfullStepsOfNewton = 0;
				nStepsToEndRateGrow = 0;
				m_bEnforceOut = false;
				nNewtonIterationsCount = 0;
				nFactorizationsCount = 0;
				nAnalyzingsCount = 0;
				nDiscontinuityNewtonFailures = 0;
				nMinimumStepFailures = 0;
				dMaxConditionNumber = 0;
				dMaxConditionNumberTime = 0;
				m_ClockStart = chrono::high_resolution_clock::now();
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


		};

		struct Parameters
		{
			ACTIVE_POWER_DAMPING_TYPE eFreqDampingType;
			DEVICE_EQUATION_TYPE m_eDiffEquationType;
			double m_dFrequencyTimeConstant;
			double m_dLRCToShuntVmin;
			double m_dZeroCrossingTolerance;
			bool m_bDontCheckTolOnMinStep;
			bool m_bConsiderDampingEquation;
			double m_dOutStep;
			ptrdiff_t nVarSearchStackDepth;
			double m_dAtol;
			double m_dRtol;
			double m_dRefactorByHRatio;
			bool m_bLogToConsole;
			bool m_bLogToFile;
			double m_dMustangDerivativeTimeConstant;
			ADAMS_RINGING_SUPPRESSION_MODE m_eAdamsRingingSuppressionMode;				// режим подавления рингинга
			ptrdiff_t m_nAdamsIndividualSuppressionCycles;								// количество перемен знака переменной для обнаружения рингинга
			ptrdiff_t m_nAdamsGlobalSuppressionStep;									// номер шага, на кратном которому работает глобальное подавление рингинга
			ptrdiff_t m_nAdamsIndividualSuppressStepsRange;								// количество шагов, на протяжении которого работает индивидуальное подавление рингинга переменной
			bool m_bUseRefactor;
			bool m_bDisableResultsWriter;
			ptrdiff_t m_nMinimumStepFailures;
			Parameters()
			{
				eFreqDampingType = APDT_ISLAND;
				m_eDiffEquationType = DET_DIFFERENTIAL;
				m_eAdamsRingingSuppressionMode = ADAMS_RINGING_SUPPRESSION_MODE::ARSM_GLOBAL;
				m_nAdamsGlobalSuppressionStep = 10;
				m_nAdamsIndividualSuppressionCycles = 3;
				m_nAdamsIndividualSuppressStepsRange = 1;
				m_dFrequencyTimeConstant = 0.02;
				m_dLRCToShuntVmin = 0.5;
				m_bDontCheckTolOnMinStep = false;
				m_bConsiderDampingEquation = false;
				m_dOutStep = 0.01;
				nVarSearchStackDepth = 100;
				m_dAtol = DFW2_ATOL_DEFAULT;
				m_dRtol = DFW2_RTOL_DEFAULT;
				m_dRefactorByHRatio = 1.5;
				m_bLogToConsole = true;
				m_bLogToFile = true;
				m_dMustangDerivativeTimeConstant = 1E-6;
				m_bUseRefactor = false;
				m_bDisableResultsWriter = false;
				m_nMinimumStepFailures = 1;
			}
		} 
			m_Parameters;

		CDynaLRC *m_pLRCGen;		// СХН для генераторных узлов без генераторов

		MatrixRow *m_pMatrixRows;

		DEVICECONTAINERS m_DeviceContainers;
		DEVICECONTAINERS m_DeviceContainersNewtonUpdate;
		DEVICECONTAINERS m_DeviceContainersPredict;

		CDevice **m_ppVarSearchStackTop, **m_ppVarSearchStackBase;
		DEVICEPTRSET m_setVisitedDevices;

		ptrdiff_t m_nMatrixSize;
		ptrdiff_t m_nNonZeroCount;

		KLU_symbolic *Symbolic;
		KLU_numeric *Numeric;
		KLU_common Common;

		double *Ax;
		double *b;
		double *pbRightHand;
		double *pRightHandBackup;

		struct RightVector *pRightVector;

		ptrdiff_t *Ap, *Ai;

		bool m_bStatus;
		bool m_bEstimateBuild;
		bool m_bRebuildMatrixFlag;

		CDevice **m_pZeroCrossingDevices;
		ptrdiff_t m_nZeroCrossingDevicesCount;

		void CleanUpMatrix(bool bSaveRightVector = false);
		bool ConvertToCCSMatrix();
		bool SolveLinearSystem();
		bool NewtonUpdate();
		bool SolveNewton(ptrdiff_t nMaxIts);
		bool EstimateMatrix();
		bool NewtonUpdateDevices();

		double GetRatioForCurrentOrder();
		double GetRatioForHigherOrder();
		double GetRatioForLowerOrder();
		bool   ChangeOrder(ptrdiff_t Newq);

		bool Step();

		double GetNorm(double *pVector);
		double GetWeightedNorm(double *pVector);

		bool PrepareGraph();
		bool PrepareYs();
		bool Link();
		bool UpdateExternalVariables();

		void ResetElement();
		bool ReallySetElement(ptrdiff_t nRow, ptrdiff_t nCol, double dValue, bool bAddToPrevious);
		bool CountSetElement(ptrdiff_t nRow, ptrdiff_t nCol, double dValue, bool bAddToPrevious);
		typedef bool (CDynaModel::*ElementSetterFn)(ptrdiff_t, ptrdiff_t, double, bool);
		void ScaleAlgebraicEquations();
		ElementSetterFn ElementSetter;
		void ReportKLUError();

		void Predict();
		void InitNordsiek();
		void InitDevicesNordsiek();
		static void InitNordsiekElement(struct RightVector *pVectorBegin, double Atol, double Rtol);
		static void PrepareNordsiekElement(struct RightVector *pVectorBegin);

		void RescaleNordsiek(double r);
		void UpdateNordsiek(bool bAllowSuppression = false);
		void SaveNordsiek();
		void RestoreNordsiek();
		void ConstructNordsiekOrder();
		void ReInitializeNordsiek();
		void ResetNordsiek();
		void BuildDerivatives();
		bool BuildMatrix();
		bool BuildRightHand();
		double CheckZeroCrossing();

		bool WriteResultsHeader();
		bool WriteResultsHeaderBinary();
		bool WriteResults();
		bool FinishWriteResults();

		struct ConvergenceTest ConvTest[2];
		struct StepControl sc;

		bool SetFunctionEqType(ptrdiff_t nRow, double dValue, DEVICE_EQUATION_TYPE EquationType);


		void GoodStep(double rSame);
		bool BadStep();
		bool NewtonFailed();
		void RepeatZeroCrossing();
		void UnprocessDiscontinuity();

		bool LoadFlow();
		void DumpMatrix(bool bAnalyzeLinearDependenies = false);
		void DumpStateVector();
		void FindMaxB(double& bmax, ptrdiff_t& nMaxIndex);


		FILE *fResult, *m_pLogFile;
		static bool ApproveContainerToWriteResults(CDeviceContainer *pDevCon);

		IResultWritePtr m_spResultWrite;
		double m_dTimeWritten;
		const _TCHAR *m_cszDampingName;
		HANDLE m_hStopEvt;

	public:
		CDynaNodeContainer Nodes;
		CDeviceContainer Branches;
		CDeviceContainer LRCs;
		CDeviceContainer Generators3C; 
		CDeviceContainer GeneratorsMustang;
		CDeviceContainer Generators1C;
		CDeviceContainer GeneratorsMotion;
		CDeviceContainer GeneratorsInfBus;
		CDeviceContainer SynchroZones;
		CDeviceContainer ExcitersMustang;
		CDeviceContainer DECsMustang;
		CDeviceContainer ExcConMustang;
		CDeviceContainer BranchMeasures;
		CCustomDeviceContainer CustomDevice;
		CCustomDeviceContainer AutomaticDevice;
		CAutomatic m_Automatic;

		CDynaModel();
		virtual ~CDynaModel();
		bool Run();

		bool InitDevices();
		bool InitEquations();

		ptrdiff_t AddMatrixSize(ptrdiff_t nSizeIncrement);
		bool SetElement(ptrdiff_t nRow, ptrdiff_t nCol, double dValue, bool bAddToPrevious = false);

		// Для теста с множителями
		//bool SetElement2(ptrdiff_t nRow, ptrdiff_t nCol, double dValue, bool bAddToPrevious = false);

		bool SetFunction(ptrdiff_t nRow, double dValue);
		bool AddFunction(ptrdiff_t nRow, double dValue);
		bool SetFunctionDiff(ptrdiff_t nRow, double dValue);
		bool SetDerivative(ptrdiff_t nRow, double dValue);
		bool CorrectNordsiek(ptrdiff_t nRow, double dValue);
		double GetFunction(ptrdiff_t nRow);
		struct RightVector* GetRightVector(ptrdiff_t nRow);
		void ResetMatrixStructure();
		bool Status();

		inline double GetOmega0()
		{
			return 2 * 50.0 * M_PI;
		}

		inline ptrdiff_t GetOrder()
		{
			return sc.q;
		}

		inline double GetH()
		{
			return sc.m_dCurrentH;
		}

		inline double GetOldH()
		{
			return sc.m_dOldH;
		}


		inline double GetFreqTimeConstant()
		{
			return m_Parameters.m_dFrequencyTimeConstant;
		}

		inline CDynaLRC* GetLRCGen()
		{
			return m_pLRCGen;
		}

		inline double GetMustangDerivativeTimeConstant()
		{
			return m_Parameters.m_dMustangDerivativeTimeConstant;
		}

		inline ACTIVE_POWER_DAMPING_TYPE GetFreqDampingType()
		{
			return m_Parameters.eFreqDampingType;
		}

		inline const _TCHAR* CDynaModel::GetDampingName()
		{
			_ASSERTE(m_cszDampingName);
			return m_cszDampingName;
		}

		bool CountConstElementsToSkip(ptrdiff_t nRow);
		bool SkipConstElements(ptrdiff_t nRow);

		inline bool FillConstantElements()
		{
			return sc.m_bFillConstantElements;
		}
		
		inline bool EstimateBuild()
		{
			return m_bEstimateBuild;
		}
		// возвращает тип метода для уравнения
		// используется для управления методом интегрирования дифференциальных переменных
		// алгебраические уравнения всегда интегрируются BDF. Дифференциальные - ADAMS или BDF
		inline DEVICE_EQUATION_TYPE CDynaModel::GetDiffEquationType()
		{
			return m_Parameters.m_eDiffEquationType;
		}

		// возвращает напряжение перехода СХН на шунт
		double GetLRCToShuntVmin()
		{
			return min(m_Parameters.m_dLRCToShuntVmin,1.0);
		}

		bool ConsiderDampingEquation()
		{
			return m_Parameters.m_bConsiderDampingEquation;
		}
		// возвращает абсолютную точность из параметров
		inline double GetAtol()
		{
			return m_Parameters.m_dAtol;
		}
		// возвращает относительную точность из параметров
		inline double GetRtol()
		{
			return m_Parameters.m_dRtol;
		}
		// Относительная погрешность решения УР по напряжению
		// с помощью линейного метода
		inline double GetRtolLULF()
		{
			return m_Parameters.m_dRtol;
		}

		inline double GetZeroCrossingInRange(double rH)
		{
			return rH > 0.0 && rH < 1.0;
		}

		inline double GetZeroCrossingTolerance()
		{
			return ((sc.Hmin / sc.m_dCurrentH) > 0.999) ? FLT_MAX : 100.0 * m_Parameters.m_dAtol;
		}

		// Текущий номер итерации Ньютона
		inline ptrdiff_t GetNewtonIterationNumber()
		{
			return sc.nNewtonIteration;
		}

		// Текущее количество шагов интегрирования
		inline ptrdiff_t GetIntegrationStepNumber()
		{
			return sc.nStepsCount;
		}

		// возвращает текущее время интегрирования
		inline double GetCurrentTime()
		{
			return sc.t;
		}

		inline bool ZeroCrossingStepReached(double dHstep)
		{
			return dHstep > 0.997;
		}

		inline double GetHysteresis(double dValue)
		{
			return fabs(dValue) * GetRtol() * 0.01 + GetAtol() * 10.0;
		}

		void StopProcess();

		void ProcessTopologyRequest();
		void DiscontinuityRequest();
		bool ServeDiscontinuityRequest();
		bool SetStateDiscontinuity(CDiscreteDelay *pDelayObject, double dDelay);
		bool RemoveStateDiscontinuity(CDiscreteDelay *pDelayObject);
		bool CheckStateDiscontinuity(CDiscreteDelay *pDelayObject);
		void NotifyRelayDelay(const CRelayDelayLogic* pRelayDelay);

		CDeviceContainer* GetDeviceContainer(eDFW2DEVICETYPE Type); 

		bool EnterDiscontinuityMode();
		bool LeaveDiscontinuityMode();
		bool ProcessDiscontinuity();
		inline bool IsInDiscontinuityMode() { return sc.m_bDiscontinuityMode; }
		inline bool IsInZeroCrossingMode() { return sc.m_bZeroCrossingMode; }
		ptrdiff_t GetStepNumber() {  return sc.nStepsCount;  }
		void RebuildMatrix(bool bRebuild = true);
		bool AddZeroCrossingDevice(CDevice *pDevice);

		void Log(CDFW2Messages::DFW2MessageStatus Status, ptrdiff_t nDBIndex, const _TCHAR* cszMessage);
		void Log(CDFW2Messages::DFW2MessageStatus Status, const _TCHAR* cszMessage, ...);
		static const double l[4][4];

		bool PushVarSearchStack(CDevice*pDevice);
		bool PopVarSearchStack(CDevice* &pDevice);
		void ResetStack();

		bool InitExternalVariable(PrimitiveVariableExternal& ExtVar, CDevice* pFromDevice, const _TCHAR* cszName);
		CDevice* GetDeviceBySymbolicLink(const _TCHAR* cszObject, const _TCHAR* cszKeys, const _TCHAR* cszSymLink);
		CDeviceContainer *GetContainerByAlias(const _TCHAR* cszAlias);
		CAutomatic& Automatic();

		void GetWorstEquations(ptrdiff_t nCount);
	};
}
