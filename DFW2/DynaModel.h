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
#include "KLUWrapper.h"
#include "Results.h"
#include "chrono"
#include "SerializerXML.h"

//#define USE_FMA
namespace DFW2
{
	class CDynaModel
	{
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
			CDevice *pMaxErrorDevice;
			double *pMaxErrorVariable;
			double dMaxErrorVariable;
			ptrdiff_t nMaxErrorVariableEquation;

			void Reset()
			{
				pMaxErrorDevice = nullptr;
				pMaxErrorVariable = nullptr;
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
			bool m_bFillConstantElements;
			bool m_bNewtonConverged;
			bool m_bNordsiekSaved;
			bool m_bNewtonDisconverging;
			bool m_bNewtonStepControl;
			bool m_bDiscontinuityMode = false;
			bool m_bZeroCrossingMode = false;
			bool m_bRetryStep;
			bool m_bStopCommandReceived = false;
			bool m_bProcessTopology = false;
			bool m_bDiscontinuityRequest;
			bool m_bEnforceOut = false;
			bool m_bBeforeDiscontinuityWritten = false;				// флаг обработки момента времени до разрыва
			double dFilteredStep;
			double dFilteredOrder;
			double dFilteredStepInner;								// фильтр минимального шага на серии шагов
			double dFilteredOrderInner;
			double dRateGrowLimit = FLT_MAX;
			ptrdiff_t nStepsCount;
			ptrdiff_t nNewtonIterationsCount = 0;
			double dMaxConditionNumber = 0.0;
			double dMaxConditionNumberTime = 0.0;
			_OrderStatistics OrderStatistics[2];
			ptrdiff_t nDiscontinuityNewtonFailures = 0;
			ptrdiff_t nMinimumStepFailures = 0;
			double m_dCurrentH;
			double m_dOldH = -1.0;
			double m_dStoredH;
			ptrdiff_t q;
			double t = 0.0;
			double t0 = 0.0;
			// volatile нужен для гарантии корректного учета суммы кэхэна в расчете времени
			// но он же не дает мешает сериалиазации, поэтому временно убран. Проверить, как влияет
			// наличие или отсутствие volatile на расчет
			/*volatile */double KahanC = 0.0;
			ptrdiff_t nStepsToStepChangeParameter = 4;
			ptrdiff_t nStepsToOrderChangeParameter = 4;
			ptrdiff_t nStepsToFailParameter = 1;
			ptrdiff_t nStepsToStepChange;
			ptrdiff_t nStepsToOrderChange;
			ptrdiff_t nStepsToFail;
			ptrdiff_t nSuccessfullStepsOfNewton = 0;
			ptrdiff_t nStepsToEndRateGrow = 0;
			ptrdiff_t nNewtonIteration;
			double dRightHandNorm;
			std::chrono::time_point<std::chrono::high_resolution_clock> m_ClockStart;
			bool bAdamsDampingEnabled = false;
			ptrdiff_t nNoRingingSteps = 0;
			double Hmin = 1E-8;

			StepError Newton;
			StepError Integrator;
			double m_dLastRefactorH;
			bool bRingingDetected = false;

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

				_ASSERTE(fabs(OrderStatistics[0].dTimePassed + OrderStatistics[1].dTimePassed - t) < DFW2_EPSILON);
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

			void UpdateSerializer(SerializerPtr& Serializer)
			{
				Serializer->SetClassName(_T("StepControl"));
				Serializer->AddProperty(_T("RefactorMatrix"), m_bRefactorMatrix);
				Serializer->AddProperty(_T("FillConstantElements"), m_bFillConstantElements);
				Serializer->AddProperty(_T("NewtonConverged"), m_bNewtonConverged);
				Serializer->AddProperty(_T("NordsiekSaved"), m_bNordsiekSaved);
				Serializer->AddProperty(_T("NewtonDisconverging"), m_bNewtonDisconverging);
				Serializer->AddProperty(_T("NewtonStepControl"), m_bNewtonStepControl);
				Serializer->AddProperty(_T("DiscontinuityMode"), m_bDiscontinuityMode);
				Serializer->AddProperty(_T("ZeroCrossingMode"), m_bZeroCrossingMode);
				Serializer->AddProperty(_T("RetryStep"), m_bRetryStep);
				Serializer->AddProperty(_T("ProcessTopology"), m_bProcessTopology);
				Serializer->AddProperty(_T("DiscontinuityRequest"), m_bDiscontinuityRequest);
				Serializer->AddProperty(_T("EnforceOut"), m_bEnforceOut);
				Serializer->AddProperty(_T("BeforeDiscontinuityWritten"), m_bBeforeDiscontinuityWritten);
				Serializer->AddProperty(_T("FilteredStep"), dFilteredStep);
				Serializer->AddProperty(_T("FilteredOrder"), dFilteredOrder);
				Serializer->AddProperty(_T("FilteredStepInner"), dFilteredStepInner);
				Serializer->AddProperty(_T("FilteredOrderInner"), dFilteredOrderInner);
				Serializer->AddProperty(_T("RateGrowLimit"), dRateGrowLimit);
				Serializer->AddProperty(_T("StepsCount"), nStepsCount);
				Serializer->AddProperty(_T("NewtonIterationsCount"), nNewtonIterationsCount);
				Serializer->AddProperty(_T("MaxConditionNumber"), dMaxConditionNumber);
				Serializer->AddProperty(_T("MaxConditionNumberTime"), dMaxConditionNumberTime);
				Serializer->AddProperty(_T("DiscontinuityNewtonFailures"), nDiscontinuityNewtonFailures);
				Serializer->AddProperty(_T("MinimumStepFailures"), nMinimumStepFailures);
				Serializer->AddProperty(_T("CurrentH"), m_dCurrentH);
				Serializer->AddProperty(_T("OldH"), m_dOldH);
				Serializer->AddProperty(_T("StoredH"), m_dStoredH);
				Serializer->AddProperty(_T("q"), q);
				Serializer->AddProperty(_T("t"), t);
				//Serializer->AddProperty(_T("KahanC"), KahanC);
				Serializer->AddProperty(_T("t0"), t0);
				Serializer->AddProperty(_T("StepsToStepChangeParameter"), nStepsToStepChangeParameter);
				Serializer->AddProperty(_T("StepsToOrderChangeParameter"), nStepsToOrderChangeParameter);
				Serializer->AddProperty(_T("StepsToFailParameter"), nStepsToFailParameter);
				Serializer->AddProperty(_T("StepsToStepChange"), nStepsToStepChange);
				Serializer->AddProperty(_T("StepsToOrderChange"), nStepsToOrderChange);
				Serializer->AddProperty(_T("StepsToFail"), nStepsToFail);
				Serializer->AddProperty(_T("SuccessfullStepsOfNewton"), nSuccessfullStepsOfNewton);
				Serializer->AddProperty(_T("StepsToEndRateGrow"), nStepsToEndRateGrow);
				Serializer->AddProperty(_T("NewtonIteration"), nNewtonIteration);
				Serializer->AddProperty(_T("RightHandNorm"), dRightHandNorm);
				Serializer->AddProperty(_T("AdamsDampingEnabled"), bAdamsDampingEnabled);
				Serializer->AddProperty(_T("NoRingingSteps"), nNoRingingSteps);
				Serializer->AddProperty(_T("Hmin"), Hmin);
				Serializer->AddProperty(_T("LastRefactorH"), m_dLastRefactorH);
				Serializer->AddProperty(_T("RingingDetected"), bRingingDetected);

				Serializer->AddProperty(_T("Order0Steps"), OrderStatistics[0].nSteps);
				Serializer->AddProperty(_T("Order0Failures"), OrderStatistics[0].nFailures);
				Serializer->AddProperty(_T("Order0NewtonFailures"), OrderStatistics[0].nNewtonFailures);
				Serializer->AddProperty(_T("Order0ZeroCrossingsSteps"), OrderStatistics[0].nZeroCrossingsSteps);
				Serializer->AddProperty(_T("Order0TimePassed"), OrderStatistics[0].dTimePassed);
				Serializer->AddProperty(_T("Order0TimePassedKahan"), OrderStatistics[0].dTimePassedKahan);

				Serializer->AddProperty(_T("Order1Steps"), OrderStatistics[1].nSteps);
				Serializer->AddProperty(_T("Order1Failures"), OrderStatistics[1].nFailures);
				Serializer->AddProperty(_T("Order1NewtonFailures"), OrderStatistics[1].nNewtonFailures);
				Serializer->AddProperty(_T("Order1ZeroCrossingsSteps"), OrderStatistics[1].nZeroCrossingsSteps);
				Serializer->AddProperty(_T("Order1TimePassed"), OrderStatistics[1].dTimePassed);
				Serializer->AddProperty(_T("Order1TimePassedKahan"), OrderStatistics[1].dTimePassedKahan);
			}
		};

		struct Parameters
		{
			ACTIVE_POWER_DAMPING_TYPE eFreqDampingType = APDT_ISLAND;
			DEVICE_EQUATION_TYPE m_eDiffEquationType = DET_DIFFERENTIAL;
			double m_dFrequencyTimeConstant = 0.02;
			double m_dLRCToShuntVmin = 0.5;
			double m_dZeroCrossingTolerance;
			bool m_bDontCheckTolOnMinStep = false;
			bool m_bConsiderDampingEquation = false;
			double m_dOutStep = 0.01;
			ptrdiff_t nVarSearchStackDepth = 100;
			double m_dAtol = DFW2_ATOL_DEFAULT;
			double m_dRtol = DFW2_RTOL_DEFAULT;
			double m_dRefactorByHRatio = 1.5;
			bool m_bLogToConsole = true;
			bool m_bLogToFile = true;
			double m_dMustangDerivativeTimeConstant = 1E-6;
			ADAMS_RINGING_SUPPRESSION_MODE m_eAdamsRingingSuppressionMode = ADAMS_RINGING_SUPPRESSION_MODE::ARSM_GLOBAL;				// режим подавления рингинга
			ptrdiff_t m_nAdamsIndividualSuppressionCycles = 3;								// количество перемен знака переменной для обнаружения рингинга
			ptrdiff_t m_nAdamsGlobalSuppressionStep = 10;									// номер шага, на кратном которому работает глобальное подавление рингинга
			ptrdiff_t m_nAdamsIndividualSuppressStepsRange = 5;								// количество шагов, на протяжении которого работает индивидуальное подавление рингинга переменной
			bool m_bUseRefactor = false;
			bool m_bDisableResultsWriter = false;
			ptrdiff_t m_nMinimumStepFailures = 1;
			double m_dZeroBranchImpedance = 0.1;
			double m_dAdamsDampingAlpha = 0.05;
			ptrdiff_t m_nAdamsDampingSteps = 10;
			Parameters() { }
			void UpdateSerializer(SerializerPtr& Serializer)
			{
				Serializer->SetClassName(_T("Parameters"));
				Serializer->AddProperty(_T("FrequencyTimeConstant"), m_dFrequencyTimeConstant, eVARUNITS::VARUNIT_SECONDS);
				Serializer->AddProperty(_T("LRCToShuntVmin"), m_dLRCToShuntVmin, eVARUNITS::VARUNIT_PU);
				Serializer->AddProperty(_T("ZeroCrossingTolerance"), m_dZeroCrossingTolerance);
				Serializer->AddProperty(_T("DontCheckTolOnMinStep"), m_bDontCheckTolOnMinStep);
				Serializer->AddProperty(_T("ConsiderDampingEquation"), m_bConsiderDampingEquation);
				Serializer->AddProperty(_T("OutStep"), m_dOutStep,eVARUNITS::VARUNIT_SECONDS);
				Serializer->AddProperty(_T("VarSearchStackDepth"), nVarSearchStackDepth);
				Serializer->AddProperty(_T("Atol"), m_dAtol);
				Serializer->AddProperty(_T("Rtol"), m_dRtol);
				Serializer->AddProperty(_T("RefactorByHRatio"), m_dRefactorByHRatio);
				Serializer->AddProperty(_T("LogToConsole"), m_bLogToConsole);
				Serializer->AddProperty(_T("LogToFile"), m_bLogToFile);
				Serializer->AddProperty(_T("MustangDerivativeTimeConstant"), m_dMustangDerivativeTimeConstant, eVARUNITS::VARUNIT_SECONDS);
				Serializer->AddProperty(_T("AdamsIndividualSuppressionCycles"), m_nAdamsIndividualSuppressionCycles, eVARUNITS::VARUNIT_PIECES);
				Serializer->AddProperty(_T("AdamsGlobalSuppressionStep"), m_nAdamsGlobalSuppressionStep, eVARUNITS::VARUNIT_PIECES);
				Serializer->AddProperty(_T("AdamsIndividualSuppressStepsRange"), m_nAdamsIndividualSuppressStepsRange, eVARUNITS::VARUNIT_PIECES);
				Serializer->AddProperty(_T("UseRefactor"), m_bUseRefactor);
				Serializer->AddProperty(_T("DisableResultsWriter"), m_bDisableResultsWriter);
				Serializer->AddProperty(_T("MinimumStepFailures"), m_nMinimumStepFailures, eVARUNITS::VARUNIT_PIECES);
				Serializer->AddProperty(_T("ZeroBranchImpedance"), m_dZeroBranchImpedance, eVARUNITS::VARUNIT_OHM);
				Serializer->AddProperty(_T("AdamsDampingAlpha"), m_dAdamsDampingAlpha);
				Serializer->AddProperty(_T("AdamsDampingSteps"), m_nAdamsDampingSteps);
			}
		} 
			m_Parameters;

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

		CDevice **m_ppVarSearchStackTop;
		std::unique_ptr<CDevice*[]> m_ppVarSearchStackBase;
		DEVICEPTRSET m_setVisitedDevices;

		ptrdiff_t m_nEstimatedMatrixSize;
		ptrdiff_t m_nTotalVariablesCount;
		double *pbRightHand;
		std::unique_ptr<double[]> pRightHandBackup;
		


		bool m_bEstimateBuild;
		bool m_bRebuildMatrixFlag;
		std::vector<CDevice*> ZeroCrossingDevices;
		void ConvertToCCSMatrix();
		void SolveLinearSystem();
		void SolveRcond();
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
		void RescaleNordsiek(double r);
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

		void WriteResultsHeader();
		void WriteResultsHeaderBinary();
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
		void DumpMatrix(bool bAnalyzeLinearDependenies = false);
		void DumpStateVector();
		FILE *fResult, *m_pLogFile;
		static bool ApproveContainerToWriteResults(CDeviceContainer *pDevCon);

		IResultWritePtr m_spResultWrite;
		double m_dTimeWritten;
		const _TCHAR *m_cszDampingName = nullptr;
		HANDLE m_hStopEvt;

		CDeviceContainer *m_pClosestZeroCrossingContainer = nullptr;

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

		void InitDevices();
		bool InitEquations();

		ptrdiff_t AddMatrixSize(ptrdiff_t nSizeIncrement);
		void SetElement(ptrdiff_t nRow, ptrdiff_t nCol, double dValue, bool bAddToPrevious);
		void SetElement(ptrdiff_t nRow, ptrdiff_t nCol, double dValue);
		void SetElement(const VariableIndex& Row, const VariableIndex& Col, double dValue);

		// Для теста с множителями
		//bool SetElement2(ptrdiff_t nRow, ptrdiff_t nCol, double dValue, bool bAddToPrevious = false);

		void SetFunction(ptrdiff_t nRow, double dValue);
		void SetFunctionDiff(ptrdiff_t nRow, double dValue);
		void SetDerivative(ptrdiff_t nRow, double dValue);
		void CorrectNordsiek(ptrdiff_t nRow, double dValue);
		double GetFunction(ptrdiff_t nRow);
		struct RightVector* GetRightVector(ptrdiff_t nRow);

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

		inline const _TCHAR* CDynaModel::GetDampingName() const
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
		inline DEVICE_EQUATION_TYPE CDynaModel::GetDiffEquationType() const
		{
			return m_Parameters.m_eDiffEquationType;
		}

		// возвращает напряжение перехода СХН на шунт
		double GetLRCToShuntVmin() const
		{
			return min(m_Parameters.m_dLRCToShuntVmin,1.0);
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
			return fabs(dValue) * GetRtol() * 0.01 + GetAtol() * 10.0;
		}

		void StopProcess();

		void ProcessTopologyRequest();
		void DiscontinuityRequest();
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

		void Log(CDFW2Messages::DFW2MessageStatus Status, ptrdiff_t nDBIndex, const _TCHAR* cszMessage);
		void Log(CDFW2Messages::DFW2MessageStatus Status, const _TCHAR* cszMessage, ...);

		double Methodl[4][4];
		double Methodlh[4];
		static const double MethodlDefault[4][4];

		static double FMA(double x, double y, double z)
		{
			//return x * y + z;
			return std::fma(x, y, z);
		}

		static double gs1(KLUWrapper<double>& klu, std::unique_ptr<double[]>& Imb, const double* Sol);

		void PushVarSearchStack(CDevice*pDevice);
		bool PopVarSearchStack(CDevice* &pDevice);
		void ResetStack();

		bool InitExternalVariable(PrimitiveVariableExternal& ExtVar, CDevice* pFromDevice, const _TCHAR* cszName);
		CDevice* GetDeviceBySymbolicLink(const _TCHAR* cszObject, const _TCHAR* cszKeys, const _TCHAR* cszSymLink);
		CDeviceContainer *GetContainerByAlias(const _TCHAR* cszAlias);
		CAutomatic& Automatic();

		void GetWorstEquations(ptrdiff_t nCount);
		void ReportKLUError(KLU_common& KLUCommon);

		void Serialize();
	};
}
