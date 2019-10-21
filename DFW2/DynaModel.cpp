#include "stdafx.h"
#include "DynaModel.h"
#include "Automatic.h"

#define _LFINFO_

#include "klu.h"
#include "cs.h"
using namespace DFW2;

CDynaModel::CDynaModel() : m_Discontinuities(this),
						   m_Automatic(this),
						   Nodes(this),
						   Branches(this),
						   Generators1C(this),
						   Generators3C(this),
						   GeneratorsMustang(this),
						   GeneratorsMotion(this),
						   GeneratorsInfBus(this),
						   LRCs(this),
						   SynchroZones(this),
						   ExcitersMustang(this),
						   DECsMustang(this),
						   ExcConMustang(this),
						   CustomDevice(this),
						   BranchMeasures(this),
						   AutomaticDevice(this),
						   m_pLogFile(NULL)
{
	m_hStopEvt = CreateEvent(NULL, TRUE, FALSE, _T("DFW2STOP"));
	// копируем дефолтные константы методов интегрирования в константы экземпляра модели
	// константы могут изменяться, например для демпфирования
	std::copy(&MethodlDefault[0][0], &MethodlDefault[0][0] + sizeof(MethodlDefault) / sizeof(MethodlDefault[0][0]), &Methodl[0][0]);

	Nodes.m_ContainerProps				= CDynaNode::DeviceProperties();
	Branches.m_ContainerProps			= CDynaBranch::DeviceProperties();
	LRCs.m_ContainerProps				= CDynaLRC::DeviceProperties();
	GeneratorsInfBus.m_ContainerProps	= CDynaGeneratorInfBus::DeviceProperties();
	GeneratorsMotion.m_ContainerProps	= CDynaGeneratorMotion::DeviceProperties();
	Generators1C.m_ContainerProps		= CDynaGenerator1C::DeviceProperties();
	Generators3C.m_ContainerProps		= CDynaGenerator3C::DeviceProperties();
	GeneratorsMustang.m_ContainerProps	= CDynaGeneratorMustang::DeviceProperties();
	DECsMustang.m_ContainerProps		= CDynaDECMustang::DeviceProperties();
	ExcConMustang.m_ContainerProps		= CDynaExcConMustang::DeviceProperties();
	ExcitersMustang.m_ContainerProps	= CDynaExciterMustang::DeviceProperties();
	SynchroZones.m_ContainerProps		= CSynchroZone::DeviceProperties();
	BranchMeasures.m_ContainerProps		= CDynaBranchMeasure::DeviceProperties();

	m_DeviceContainers.push_back(&Nodes);
	m_DeviceContainers.push_back(&ExcitersMustang);
	m_DeviceContainers.push_back(&DECsMustang);
	m_DeviceContainers.push_back(&ExcConMustang);
	m_DeviceContainers.push_back(&Branches);
	m_DeviceContainers.push_back(&Generators3C);
	m_DeviceContainers.push_back(&GeneratorsMustang);
	m_DeviceContainers.push_back(&Generators1C);
	m_DeviceContainers.push_back(&GeneratorsMotion);
	m_DeviceContainers.push_back(&GeneratorsInfBus);
	//m_DeviceContainers.push_back(&CustomDevice);
	m_DeviceContainers.push_back(&AutomaticDevice);
	m_DeviceContainers.push_back(&BranchMeasures);
	m_DeviceContainers.push_back(&SynchroZones);
	_tfopen_s(&m_pLogFile, _T("c:\\tmp\\dfw2.log"), _T("w+, ccs=UTF-8"));
}


CDynaModel::~CDynaModel()
{
	if (m_hStopEvt)
		CloseHandle(m_hStopEvt);
	if (m_ppVarSearchStackBase)
		delete m_ppVarSearchStackBase;
}

bool CDynaModel::Run()
{
	bool bRes = true;
#ifdef _WIN64
	_set_FMA3_enable(1);
#endif

	try
	{
		m_Parameters.m_dZeroBranchImpedance = -0.1;

		//m_Parameters.m_dFrequencyTimeConstant = 1E-3;
		m_Parameters.eFreqDampingType = APDT_NODE;
		m_Parameters.m_dOutStep = 1E-10;
		//m_Parameters.eFreqDampingType = APDT_ISLAND;
		//m_Parameters.m_eDiffEquationType = DET_ALGEBRAIC;

		//m_Parameters.m_eAdamsRingingSuppressionMode = ADAMS_RINGING_SUPPRESSION_MODE::ARSM_GLOBAL;
		//m_Parameters.m_eAdamsRingingSuppressionMode = ADAMS_RINGING_SUPPRESSION_MODE::ARSM_NONE;
		//m_Parameters.m_eAdamsRingingSuppressionMode = ADAMS_RINGING_SUPPRESSION_MODE::ARSM_INDIVIDUAL;
		m_Parameters.m_eAdamsRingingSuppressionMode = ADAMS_RINGING_SUPPRESSION_MODE::ARSM_DAMPALPHA;
		m_Parameters.m_nAdamsGlobalSuppressionStep = 15;
		m_Parameters.m_nAdamsIndividualSuppressStepsRange = 150;

		m_Parameters.m_bUseRefactor = true;
		m_Parameters.m_dAtol = 1E-4;
		m_Parameters.m_dMustangDerivativeTimeConstant = 1E-4;
		m_Parameters.m_bLogToConsole = false;
		m_Parameters.m_bLogToFile = true;

		m_Parameters.m_bDisableResultsWriter = true;

		// если в параметрах задан BDF для дифуров, отключаем
		// подавление рингинга
		if(m_Parameters.m_eDiffEquationType == DET_ALGEBRAIC)
			m_Parameters.m_eAdamsRingingSuppressionMode = ADAMS_RINGING_SUPPRESSION_MODE::ARSM_NONE;

		//m_Parameters.m_dOutStep = 1E-5;
		bRes = bRes && (LRCs.Init(this) == DFS_OK);

		bRes = bRes && Link();
		bRes = bRes && PrepareGraph();
		bRes = bRes && PrepareYs();
		bRes = bRes && Nodes.ProcessTopology();
		LoadFlow();
		bRes = bRes && InitDevices();
		EstimateMatrix();
		bRes = bRes && InitEquations();

		//CDynaLRC *pLRC = static_cast<CDynaLRC*>(LRCs.GetDevice(-1));
		//pLRC->TestDump();

	#define SMZU

		// Расчет может завалиться внутри цикла, например из-за
		// сингулярной матрицы, поэтому контролируем
		// была ли начата запись результатов. Если что-то записали - нужно завершить
		bool bResultsNeedToBeFinished = false;

		if (bRes)
		{
			m_Discontinuities.AddEvent(150.0, new CModelActionStop());

	#ifdef SMZU

	/*		CDynaBranch *pBranch = static_cast<CDynaBranch*>(*(Branches.begin() + 520));
			m_Discontinuities.AddEvent(0.55, new CModelActionChangeBranchState(pBranch, CDynaBranch::BRANCH_OFF));
			m_Discontinuities.AddEvent(0.7, new CModelActionChangeBranchState(pBranch, CDynaBranch::BRANCH_ON));

			CDynaNode *pNode392 = static_cast<CDynaNode*>(Nodes.GetDevice(1114));
			m_Discontinuities.AddEvent(0.5, new CModelActionChangeNodeShunt(pNode392, cplx(0.0, 1E-7)));
			m_Discontinuities.AddEvent(0.6, new CModelActionRemoveNodeShunt(pNode392));*/

	#else
			//CDynaGenerator1C *pGen = static_cast<CDynaGenerator1C*>(*Generators1C.begin());

			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*(Branches.begin() + 5));

			CDynaNode *pNode392 = static_cast<CDynaNode*>(Nodes.GetDevice(392));
			CDynaNode *pNode319 = static_cast<CDynaNode*>(Nodes.GetDevice(319));

			//CDynaExciterMustang *pExciter = static_cast<CDynaExciterMustang>(ExcitersMustang.GetDevice(1319));


		
			//m_Discontinuities.AddEvent(1.0, new CModelActionChangeVariable(&pGen->Pt, 1749.6));
			//m_Discontinuities.AddEvent(5.0, new CModelActionChangeVariable(&pGen->Pt, 1700.0));
			//m_Discontinuities.AddEvent(1.0, new CModelActionChangeBranchState(pBranch, CDynaBranch::BRANCH_OFF));

			// /*
			m_Discontinuities.AddEvent(0.5, new CModelActionChangeNodeShunt(pNode392, cplx(0.1, 0.1)));
			m_Discontinuities.AddEvent(0.6, new CModelActionRemoveNodeShunt(pNode392));
			m_Discontinuities.AddEvent(0.6, new CModelActionChangeBranchState(pBranch, CDynaBranch::BRANCH_OFF));

			m_Discontinuities.AddEvent(7.8, new CModelActionChangeNodeLoad(pNode319, cplx(pNode319->Pn * 0.8, pNode319->Qn * 0.8)));

			m_Discontinuities.AddEvent(.7, new CModelActionChangeBranchState(pBranch, CDynaBranch::BRANCH_ON));
			// */

			//m_Discontinuities.AddEvent(5.0, new CModelActionChangeNodeShunt(pNode392, cplx(0.0, 0.0)));
			//m_Discontinuities.AddEvent(5.5, new CModelActionRemoveNodeShunt(pNode392));

			//m_Discontinuities.AddEvent(1.0, new CModelActionChangeVariable(&pExciter->Uexc, pExciter->Uexc * 4.1));
			//m_Discontinuities.AddEvent(15.0, new CModelActionChangeVariable(&pExciter->Uexc, pExciter->Uexc * 0.5 ));
			//m_Discontinuities.AddEvent(23.0, new CModelActionChangeVariable(&pExciter->Uexc, pExciter->Uexc * 4.1));
			//m_Discontinuities.AddEvent(5.0, new CModelActionChangeVariable(&pExciter->Uexc, pExciter->Uexc));
	#endif

	

			bRes = bRes && m_Automatic.Init();
			bRes = bRes && m_Discontinuities.Init();
			bRes = bRes && WriteResultsHeader();
			SetH(0.01);
			while (!sc.m_bStopCommandReceived && bRes)
			{
				bRes = bRes && Step();
				if (!sc.m_bStopCommandReceived)
				{
					if (bRes)
					{
						// если ошибок не было, пишем результаты
						if (WriteResults())
							bResultsNeedToBeFinished = true;  // если записали - то фиксируем признак завершения
						else
							bResultsNeedToBeFinished = bRes = false; // если не записали - сбрасываем признак завершения
					}
				}
				if (WaitForSingleObject(m_hStopEvt, 0) == WAIT_OBJECT_0)
					break;
			}
		}

		if (!bRes)
			MessageBox(NULL, _T("Failed"), _T("Failed"), MB_OK);


		// вне зависимости от результата завершаем запись результатов
		// по признаку завершения
		if (bResultsNeedToBeFinished)
			if (!FinishWriteResults())
				bRes = false;	// если завершить не получилось - портим результат

		Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("Steps count %d"), sc.nStepsCount);
		Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("Steps by 1st order count %d, failures %d Newton failures %d zc %d Time passed %f"),
																		sc.OrderStatistics[0].nSteps, 
																		sc.OrderStatistics[0].nFailures,
																		sc.OrderStatistics[0].nNewtonFailures,
																		sc.OrderStatistics[0].nZeroCrossingsSteps,
																		sc.OrderStatistics[0].dTimePassed);

		Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("Steps by 2nd order count %d, failures %d Newton failures %d zc %d Time passed %f"),
																		sc.OrderStatistics[1].nSteps,
																		sc.OrderStatistics[1].nFailures,
																		sc.OrderStatistics[1].nNewtonFailures,
																		sc.OrderStatistics[1].nZeroCrossingsSteps,
																		sc.OrderStatistics[1].dTimePassed);

		Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("Factors count %d / (%d + %d failures) Analyzings count %d"), klu.FactorizationsCount(), 
																															 klu.RefactorizationsCount(), 
																															 klu.RefactorizationFailuresCount(),
																															 klu.AnalyzingsCount());
		Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("Newtons count %d %f per step, failures at step %d failures at discontinuity %d"),
																	 sc.nNewtonIterationsCount, 
																	 static_cast<double>(sc.nNewtonIterationsCount) / sc.nStepsCount, 
																	 sc.OrderStatistics[0].nNewtonFailures + sc.OrderStatistics[1].nNewtonFailures,
																	 sc.nDiscontinuityNewtonFailures);
		Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("Max condition number %g at time %g"),
																	 sc.dMaxConditionNumber,
																	 sc.dMaxConditionNumberTime);

		GetWorstEquations(10);
		chrono::milliseconds CalcDuration = chrono::duration_cast<std::chrono::milliseconds>(chrono::high_resolution_clock::now() - sc.m_ClockStart);
		Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("Duration %g"), static_cast<double>(CalcDuration.count()) / 1E3);
	}
	catch (dfw2error& err)
	{
		Log(CDFW2Messages::DFW2LOG_FATAL, Cex(_T("Исключение : %s"), err.uwhat()));
	}

	return bRes;
}

bool CDynaModel::InitDevices()
{
	eDEVICEFUNCTIONSTATUS Status = DFS_NOTREADY;

	m_cszDampingName = (GetFreqDampingType() == APDT_ISLAND) ? CDynaNode::m_cszSz : CDynaNode::m_cszS;

	// Вызываем обновление внешних переменных чтобы получить значения внешних устройств. Индексов до построения матрицы пока нет
	if (!UpdateExternalVariables())
		Status = DFS_FAILED;

	klu.Common()->ordering = 0; // используем amd (0) или colamd (1). 0 - лучше для userefactor = true, 1 - для userefactor = false

	ptrdiff_t nTotalOKInits = -1;
	
	while (Status == DFS_NOTREADY && Status != DFS_FAILED)
	{
		ptrdiff_t nOKInits = 0;

		for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end() && Status != DFS_FAILED; it++)
		{
			switch ((*it)->Init(this))
			{
			case DFS_OK:
			case DFS_DONTNEED:
				nOKInits++; // count how many inits succeeded
				break;
			case DFS_FAILED:
				Status = DFS_FAILED;
				break;
			case DFS_NOTREADY:
				Status = DFS_NOTREADY;
				break;
			}
		}
		if (nOKInits == m_DeviceContainers.size())
		{
			Status = DFS_OK;
			break;
		}
		else
		{
			if (nTotalOKInits < 0)
				nTotalOKInits = nOKInits;
			else
				if (nTotalOKInits == nOKInits)
				{
					Status = DFS_FAILED;
					Log(CDFW2Messages::DFW2LOG_ERROR, DFW2::CDFW2Messages::m_cszInitLoopedInfinitely);
					break;
				}
		}

		if (!CDevice::IsFunctionStatusOK(Status) && Status != DFS_FAILED)
		{
			for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
			{
				Status = (*it)->Init(this);
				if (!CDevice::IsFunctionStatusOK(Status))
				{
					Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(DFW2::CDFW2Messages::m_cszDeviceContainerFailedToInit, (*it)->GetType(), Status));
				}
			}
		}
	}
	return CDevice::IsFunctionStatusOK(Status);
}


bool CDynaModel::InitEquations()
{
	InitNordsiek();
	// Второй раз вызываем обновление внешних переменных чтобы получить реальные индексы элементов из матрицы
	bool bRes = UpdateExternalVariables(); 
	if (bRes)
	{
		struct RightVector *pVectorBegin = pRightVector;
		struct RightVector *pVectorEnd = pRightVector + klu.MatrixSize();

		while (pVectorBegin < pVectorEnd)
		{
			//pVectorBegin->Nordsiek[0] = *pVectorBegin->pValue;
			pVectorBegin->Nordsiek[0] = pVectorBegin->SavedNordsiek[0] = *pVectorBegin->pValue;
			PrepareNordsiekElement(pVectorBegin);
			pVectorBegin++;
		}

		double dCurrentH = sc.m_dCurrentH;
		SetH(0.0);
		sc.m_bDiscontinuityMode = true;

		if (!SolveNewton(100))
			bRes = false;
		if (!sc.m_bNewtonConverged)
			bRes = false;

		sc.m_bDiscontinuityMode = false;
		SetH(dCurrentH);
		sc.nStepsCount = 0;

		for (auto&& cit : m_DeviceContainers)
			for (auto&& dit : *cit)
				dit->StoreStates();
	}
	return bRes;
}


csi cs_gatxpy(const cs *A, const double *x, double *y);

bool CDynaModel::NewtonUpdate()
{
	bool bRes = true;

	sc.m_bNewtonConverged = false;
	sc.m_bNewtonDisconverging = false;
	sc.m_bNewtonStepControl = false;

	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + klu.MatrixSize();

	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::Reset);

	// original Hindmarsh (2.99) suggests ConvCheck = 0.5 / (sc.q + 2), but i'm using tolerance 5 times lower
	double ConvCheck = 0.1 / (sc.q + 2);

	double dOldMaxAbsError = sc.Newton.dMaxErrorVariable;

	// first check Newton convergence
	sc.Newton.Reset();

	// констатны метода выделяем в локальный массив, определяя порядок метода для всех переменных один раз
	const double Methodl0[2] = { Methodl[sc.q - 1 + DET_ALGEBRAIC * 2][0],  Methodl[sc.q - 1 + DET_DIFFERENTIAL * 2][0] };

	double *pB = klu.B();
	while (pVectorBegin < pVectorEnd)
	{
		double& db = *(pB + (pVectorBegin - pRightVector));
		pVectorBegin->Error += db;
		pVectorBegin->b = db;

		double dMaxErrorDevice = fabs(db);

		if (sc.Newton.nMaxErrorVariableEquation < 0 || sc.Newton.dMaxErrorVariable < dMaxErrorDevice)
		{
			sc.Newton.pMaxErrorDevice = pVectorBegin->pDevice;
			sc.Newton.pMaxErrorVariable = pVectorBegin->pValue;
			sc.Newton.dMaxErrorVariable = dMaxErrorDevice;
			sc.Newton.nMaxErrorVariableEquation = pVectorBegin - pRightVector;
		}

#ifdef USE_FMA
		double dNewValue = FMA(Methodl0[pVectorBegin->EquationType], pVectorBegin->Error, pVectorBegin->Nordsiek[0]);
#else
		double l0 = pVectorBegin->Error;
		l0 *= Methodl0[pVectorBegin->EquationType];
		double dNewValue = pVectorBegin->Nordsiek[0] + l0;
#endif


		double dOldValue = *pVectorBegin->pValue;
		*pVectorBegin->pValue = dNewValue;

		if (pVectorBegin->Atol > 0)
		{
			double dError = pVectorBegin->GetWeightedError(db, dOldValue);
			_CheckNumber(dError);
			struct ConvergenceTest *pCt = ConvTest + pVectorBegin->EquationType;
			pCt->AddError(dError * dError);
		}
		pVectorBegin++;
	}

	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::FinalizeSum);
	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::GetConvergenceRatio);

	if (ConvTest[DET_ALGEBRAIC].dErrorSums < Methodl[sc.q - 1][3] * ConvCheck &&
		ConvTest[DET_DIFFERENTIAL].dErrorSums < Methodl[sc.q + 1][3] * ConvCheck)
	{
		sc.m_bNewtonConverged = true;
	}
	else
	{
		if (ConvTest[DET_ALGEBRAIC].dCms > 1.0 || 
			ConvTest[DET_DIFFERENTIAL].dCms > 1.0)
		{
			sc.RefactorMatrix();

			double lambdamin = 0.1;
			bool bLineSearch = false;

			
			/*

			Старый вариант определения расходимости 

			if ((sc.nNewtonIteration > 5 && sc.Hmin / sc.m_dCurrentH < 0.98) ||
				(sc.nNewtonIteration > 2 && sc.Newton.dMaxErrorVariable > 1.0))
					sc.m_bNewtonDisconverging = true;
			*/

			if (sc.m_bDiscontinuityMode)
			{
				// для обработки разрыва шаг линейного поиска делаем больше чем минимальный шаг
				// (хотя может стоит его сделать таким же, как и для минимального шага,
				// но будет сходиться очень медленно)
				lambdamin = 1E-4;
				bLineSearch = true;
			}
			else if (sc.Hmin / sc.m_dCurrentH > 0.95)
			{
				// если шаг снижается до минимального 
				// переходим на Ньютон по параметру
				lambdamin = sc.Hmin;
 				bLineSearch = true;
			}
			else
			{
				// если шаг относительно большой
				// прерываем итерации Ньютона: проще снизить шаг чем пытаться пройти Ньютоном
				if(sc.nNewtonIteration > 5)
					sc.m_bNewtonDisconverging = true;
				else if(sc.nNewtonIteration > 2 && sc.Newton.dMaxErrorVariable > 1.0)
					sc.m_bNewtonDisconverging = true;
			}

			if (!sc.m_bNewtonDisconverging)
			{
				if (bLineSearch)
				{
					unique_ptr<double[]> pRh = make_unique<double[]>(klu.MatrixSize());
					unique_ptr<double[]> pRb = make_unique<double[]>(klu.MatrixSize());
					std::copy(pRightHandBackup.get(), pRightHandBackup.get() + klu.MatrixSize(), pRh.get()); // невязки до итерации
					std::copy(klu.B(), klu.B() + klu.MatrixSize(), pRb.get());								 // решение на данной итерации
					double g0 = sc.dRightHandNorm;															 // норма небаланса до итерации
					BuildRightHand();																		 // невязки после итерации
					double g1 = sc.dRightHandNorm;															 // норма небаланса после итерации

					if (g0 < g1)
					{
						
						// если небаланс увеличился
						double gs1v = gs1(klu, pRh, pRb.get());
						
						// считаем множитель
						double lambda = -0.5 * gs1v / (g1 - g0 - gs1v);

						if (lambda > lambdamin && lambda < 1.0)
						{
							pVectorBegin = pRightVector;
							while (pVectorBegin < pVectorEnd)
							{
								double& db = pRb[pVectorBegin - pRightVector];
								pVectorBegin->Error -= db;
								pVectorBegin->Error += db * lambda;
								double l0 = pVectorBegin->Error;
								l0 *= Methodl0[pVectorBegin->EquationType];
								*pVectorBegin->pValue = pVectorBegin->Nordsiek[0] + l0;
								pVectorBegin++;
							}
						}
						else
							sc.m_bNewtonDisconverging = true;
					}
					sc.RefactorMatrix();
				}
			}
		}
		else
			sc.RefactorMatrix(false);
	}

	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::NextIteration);

	NewtonUpdateDevices();
	return bRes;
}

void CDynaModel::NewtonUpdateDevices()
{
	for (auto&& it : m_DeviceContainersNewtonUpdate)
		it->NewtonUpdateBlock(this);
}

bool CDynaModel::SolveNewton(ptrdiff_t nMaxIts)
{
	bool bRes = true;
	sc.nNewtonIteration = 1;

	if (sc.m_bProcessTopology)
	{
		bRes = bRes && Nodes.Seidell();
		bRes = bRes && ProcessDiscontinuity();
		sc.m_bProcessTopology = false;
		_ASSERTE(sc.m_bDiscontinuityMode);
		sc.m_bDiscontinuityMode = true;
	}

	if (!bRes)
		return bRes;

	bRes = false;

	if (sc.m_bDiscontinuityMode)
	{
		sc.RefactorMatrix();
	}

	for (; sc.nNewtonIteration < nMaxIts; sc.nNewtonIteration++)
	{
		bool IterationOK = false;

		if (sc.m_bDiscontinuityMode)
			sc.RefactorMatrix();

		BuildMatrix();
#ifdef _LFINFO2_
		if (sc.m_bDiscontinuityMode || sc.m_dCurrentH <= sc.Hmin)
		{
			for (DEVICEVECTORITR it = Nodes.begin(); it != Nodes.end(); it++)
			{
				CDynaNode *pNode = static_cast<CDynaNode*>(*it);
				_tcprintf(_T("\n%30s - %6.2f %6.2f %6.2f %6.2f"), pNode->GetVerbalName(), pNode->V, pNode->Delta * 180 / 3.14159, pNode->Pnr, pNode->Qnr);
			}
			for (DEVICEVECTORITR it = Generators1C.begin(); it != Generators1C.end(); it++)
			{
				CDynaGenerator1C *pGen = static_cast<CDynaGenerator1C*>(*it);
				_tcprintf(_T("\n%30s - %6.2f %6.2f"), pGen->GetVerbalName(), pGen->P, pGen->Q);
			}
			for (DEVICEVECTORITR it = GeneratorsMotion.begin(); it != GeneratorsMotion.end(); it++)
			{
				CDynaGeneratorMotion *pGen = static_cast<CDynaGeneratorMotion*>(*it);
				_tcprintf(_T("\n%30s - %6.2f %6.2f"), pGen->GetVerbalName(), pGen->P, pGen->Q);
			}
			for (DEVICEVECTORITR it = GeneratorsInfBus.begin(); it != GeneratorsInfBus.end(); it++)
			{
				CDynaGeneratorInfBus *pGen = static_cast<CDynaGeneratorInfBus*>(*it);
				_tcprintf(_T("\n%30s - %6.2f %6.2f"), pGen->GetVerbalName(), pGen->P, pGen->Q);
			}
		}
#endif

		sc.nNewtonIterationsCount++;

		ptrdiff_t imax(0);
		double bmax = klu.FindMaxB(imax);
	
//		Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_DEBUG, _T("%g %d"), bmax, imax);
		SolveLinearSystem();

//			DumpMatrix();
		bmax = klu.FindMaxB(imax);
//		Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_DEBUG, _T("%g %d"), bmax, imax);

		if (NewtonUpdate())
		{
			/*if (GetIntegrationStepNumber() == 2031 && GetNewtonIterationNumber() == 5)
			{
				DumpStateVector();
				DumpMatrix();
			}
			*/
			IterationOK = true;

			if (sc.m_bNewtonConverged)
			{
#ifdef _LFINFO_
				Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("t=%.12g (%d) Converged in %3d iterations %g %s %g %g %s Saving %g"), GetCurrentTime(),
																				sc.nStepsCount,
																				sc.nNewtonIteration,
																				sc.Newton.dMaxErrorVariable, 
																				sc.Newton.pMaxErrorDevice->GetVerbalName(),
																				*sc.Newton.pMaxErrorVariable, 
																				pRightVector[sc.Newton.nMaxErrorVariableEquation].Nordsiek[0],
																				sc.Newton.pMaxErrorDevice->VariableNameByPtr(sc.Newton.pMaxErrorVariable),
																				1.0 - static_cast<double>(klu.FactorizationsCount()) / sc.nNewtonIterationsCount);
						
#endif

				// ProcessDiscontinuity checks Discontinuity mode internally and can be called
				// regardless of this mode
				bRes = ProcessDiscontinuity();
				break;
			}
#ifdef _LFINFO_
			else
			{

				if (!sc.m_bNewtonStepControl)
				{
					Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("t=%.12g (%d) Continue %3d iteration %g %s %g %g %s"), GetCurrentTime(),
						sc.nStepsCount,
						sc.nNewtonIteration,
						sc.Newton.dMaxErrorVariable,
						sc.Newton.pMaxErrorDevice->GetVerbalName(),
						*sc.Newton.pMaxErrorVariable,
						pRightVector[sc.Newton.nMaxErrorVariableEquation].Nordsiek[0],
						sc.Newton.pMaxErrorDevice->VariableNameByPtr(sc.Newton.pMaxErrorVariable));
				}
			}

			if (sc.m_bNewtonDisconverging && !sc.m_bDiscontinuityMode)
			{
				bRes = true;
				break;
			}
#endif
		}
		if (!IterationOK) 
			break;
	}

	if (sc.nNewtonIteration == nMaxIts)
	{
		bRes = true;
	}

	if (!sc.m_bNewtonConverged)
	{
		if (sc.Newton.nMaxErrorVariableEquation >= 0)
			(pRightVector + sc.Newton.nMaxErrorVariableEquation)->nErrorHits++;
	}

	return bRes;
}

bool CDynaModel::Step()
{
	bool bRes = true;

	// запоминаем текущее значение времени
	sc.Assign_t0();

	if (!sc.m_bDiscontinuityMode)
	{
		// если мы были вне режима обработки разрыва
		double dTimeNextEvent = m_Discontinuities.NextEventTime();
		// проверяем, нет ли внутри шага событий
		if(sc.CurrentTimePlusStep() > dTimeNextEvent)
		{
			// если внутри шага есть события, настраиваем шаг на момент события
			double rHit = (dTimeNextEvent - GetCurrentTime()) / GetH();
			// если при этом настроенный коэффициент изменения шага больше погрешности
			if (rHit > DFW2_EPSILON)
			{
				SetH(sc.m_dCurrentH * rHit); 					// меняем шаг
				RescaleNordsiek(rHit);							// пересчитываем Nordsieck на новый шаг
				Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(CDFW2Messages::m_cszStepAdjustedToDiscontinuity, GetCurrentTime(), GetIntegrationStepNumber(), sc.m_dCurrentH));
				sc.m_bBeforeDiscontinuityWritten = false;		// готовимся к обработке разрыва
			}
			else
			{
				// если время найденного события "точно" попадает во время текущего шага
				if (Equal(GetCurrentTime(), dTimeNextEvent))
				{
					if (sc.m_bBeforeDiscontinuityWritten)
					{
						// если момент до разрыва уже был записан
						// входим в режим обработки разрывов
						EnterDiscontinuityMode();			
						// обрабатываем разрыв
						bRes = m_Discontinuities.ProcessStaticEvents() != DFW2_ACTION_STATE::AS_ERROR;
						// проверяем, не возникла ли при обработке разрыва необходимость обработки топологии
						if (sc.m_bProcessTopology)
							bRes = bRes && Nodes.ProcessTopology();

						// если возникла необходимость перестроения Якоби
						if (m_bRebuildMatrixFlag)
						{
							// строим ее заново
							EstimateMatrix();
							bRes = bRes && UpdateExternalVariables();
						}
						// проверяем, не возникло ли новых запросов на обработку разрыва при обработке разрыва
						if (!sc.m_bDiscontinuityRequest)
							sc.m_bBeforeDiscontinuityWritten = false;		// если запросов нет - больше записывать до разрыва нечего
					}
					else
					{
						// если мы еще не обработали момент до разрыва, включаем ожидание разрыва
						sc.m_bBeforeDiscontinuityWritten = true;
						// готовим запись результата до разрыва
						sc.m_bEnforceOut = true;
						// прерываем шаг
						return bRes;
					}
				}
			}
		}
	}

	if (sc.m_bDiscontinuityRequest)
	{
		// если был запрос на обработку разрыва
		if (sc.m_bBeforeDiscontinuityWritten)
		{
			// и мы уже записали состояние модели до разрыва
			ServeDiscontinuityRequest();	// обрабатываем разрыв
			sc.m_bBeforeDiscontinuityWritten = false;
		}
		else
		{
			// если состояние еще не записали - требуем записи текущего времени
			sc.m_bBeforeDiscontinuityWritten = true;
			sc.m_bEnforceOut = true;
			return bRes;
		}
	}

	// по умолчанию считаем, что шаг придется повторить
	sc.m_bRetryStep = true;

	// передвигаем время на следующий шаг с возможностью отката (см. CheckAdvance_t0)
	sc.CheckAdvance_t0();

	// пока шаг нужно повторять, все в порядке и не получена команда останова
	while (sc.m_bRetryStep && bRes && !sc.m_bStopCommandReceived)
	{
		if (WaitForSingleObject(m_hStopEvt, 0) == WAIT_OBJECT_0)
			break;

		Predict();		// делаем прогноз Nordsieck для следуюшего времени
		bRes = bRes && SolveNewton(sc.m_bDiscontinuityMode ? 20 : 10);	// делаем корректор

		if (bRes)
		{
			// если корректор отработал без сбоев
			if (sc.m_bNewtonConverged)
			{
				// если Ньютон сошелся
				if (!sc.m_bDiscontinuityMode)
				{
					DetectAdamsRinging();

					// и мы не находились в режиме обработки разрыва
					double rSame = GetRatioForCurrentOrder();	// рассчитываем возможный шаг для текущего порядка метода

					// this call before accuracy makes limits
					// work correct

					if (rSame > 1.0)
					{
						// если шаг можно увеличить
						// проверяем нет ли зерокроссинга
						double rZeroCrossing = CheckZeroCrossing();

						_ASSERTE(rZeroCrossing >= 0.0);

						if (sc.m_bZeroCrossingMode)
						{
							// если были в зерокроссинге проверяем точность попадания времени при поиске зерокроссинга
							if (ZeroCrossingStepReached(rZeroCrossing))
							{
								// если нашли время зерокроссинга выходим из режима зерокроссинга
								sc.m_bZeroCrossingMode = false;

								if (!sc.m_bDiscontinuityRequest)
								{
									// если не возникло запросов на обработку разрыва
									// обнуляем коэффициент шага, чтобы он не изменился
									rSame = 0.0;
									// и признаем шаг успешным
									GoodStep(rSame);
								}
								else
								{
									sc.m_bRetryStep = false; // если были запросы на обработку разрыва отменяем повтор шага, чтобы обработать разрыв
									sc.OrderStatistics[sc.q - 1].nZeroCrossingsSteps++;
									sc.Advance_t0();	// делаем Advance чтобы правильно рассчитать время пройденное каждым методом интегрирования
								}
							}
							else
							{
								// если время зерокроссинга не достаточно точно определено подбираем новый шаг
								SetH(sc.m_dCurrentH * rZeroCrossing);
								RepeatZeroCrossing();
							}
						}
						else
						{
							// если мы не были в режиме зерокроссинга
							if (rZeroCrossing >= 1.0)
							{
								// и найденное время зерокроссинга не находится в пределах текущего шага
								// режим зерокроссинга отменяем
								sc.m_bZeroCrossingMode = false;

								if (!sc.m_bDiscontinuityRequest)
								{
									// если не было запросов обработки разрыва признаем шаг успешным
									GoodStep(rSame);
								}
								else
								{
									sc.m_bRetryStep = false; // отменяем повтор шага, чтобы обработать разрыв
									sc.OrderStatistics[sc.q - 1].nZeroCrossingsSteps++;
									sc.Advance_t0();	// делаем Advance чтобы правильно рассчитать время пройденное каждым методом интегрирования
								}
							}
							else
							{
								// если время зерокроссинга внутри шага, входим в режим зерокроссинга
								sc.m_bZeroCrossingMode = true;
								// и пытаемся подобрать шаг до времени зерокроссинга
								SetH(sc.m_dCurrentH * rZeroCrossing);
								RepeatZeroCrossing();
							}
						}
					}
					else
					{
						// шаг нужно уменьшить
						// отбрасываем шаг
						bRes = BadStep() && bRes;
					}
				}
				else
				{
					// если находились в режиме обработки разрыва, выходим из него (Ньютон сошелся, все ОК)
					LeaveDiscontinuityMode();
					// удаляем события, который уже отработали по времени
					m_Discontinuities.PassTime(GetCurrentTime() + 2.0 * DFW2_EPSILON);
					sc.m_bRetryStep = false;		// отказываемся от повтора шага, все хорошо
					sc.m_bEnforceOut = true;		// требуем записи результатов после обработки разрыва
					sc.m_bBeforeDiscontinuityWritten = false;
					sc.OrderStatistics[sc.q - 1].nSteps++;
				}
			}
			else
			{
				// Ньютон не сошелся
				// отбрасываем шаг
				bRes = NewtonFailed() && bRes;
			}
		}
		sc.nStepsCount++;
	}
	
	//_ASSERTE(_CrtCheckMemory());
	return bRes;

}

void CDynaModel::ConvergenceTest::AddError(double dError)
{
	// Три варианта расчета суммы погрешностей
	nVarsCount++;
	AddErrorStraight(dError);
	//AddErrorKahan(dError);
	//AddErrorNeumaier(dError);		// если используется Neumaier - в  FinalizeSum должна быть раскомментирована сумма ошибки с корректором
}

void CDynaModel::ConvergenceTest::FinalizeSum()
{
	//dErrorSum += dKahanC;
}



double CDynaModel::GetRatioForCurrentOrder()
{
	double r = 0.0;

	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + klu.MatrixSize();

	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::Reset);

	sc.Integrator.Reset();

	const double Methodl0[2] = { Methodl[sc.q - 1 + DET_ALGEBRAIC * 2][0],  Methodl[sc.q - 1 + DET_DIFFERENTIAL * 2][0] };

	while (pVectorBegin < pVectorEnd)
	{
		if (pVectorBegin->Atol > 0)
		{
			// compute again to not asking device via pointer
#ifdef USE_FMA
			double dNewValue = FMA(pVectorBegin->Error, Methodl0[pVectorBegin->EquationType], pVectorBegin->Nordsiek[0]);
#else
			double dNewValue = pVectorBegin->Nordsiek[0] + pVectorBegin->Error * Methodl0[pVectorBegin->EquationType];
#endif

			double dError = pVectorBegin->GetWeightedError(dNewValue);
			double dMaxError = fabs(dError);

			if (sc.Integrator.nMaxErrorVariableEquation < 0 || sc.Integrator.dMaxErrorVariable < dMaxError)
			{
				sc.Integrator.dMaxErrorVariable = dMaxError;
				sc.Integrator.pMaxErrorDevice = pVectorBegin->pDevice;
				sc.Integrator.pMaxErrorVariable = pVectorBegin->pValue;
				sc.Integrator.nMaxErrorVariableEquation = pVectorBegin - pRightVector;
			}

			struct ConvergenceTest *pCt = ConvTest + pVectorBegin->EquationType;
			pCt->AddError(dError * dError);
		}
		pVectorBegin++;
	}
	// Neumaier addition final phase

	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::FinalizeSum);
	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::GetRMS);
		
	double DqSame0 = ConvTest[DET_ALGEBRAIC].dErrorSum / Methodl[sc.q - 1][3];
	double DqSame1 = ConvTest[DET_DIFFERENTIAL].dErrorSum / Methodl[sc.q + 1][3];
	double rSame0 = pow(DqSame0, -1.0 / (sc.q + 1));
	double rSame1 = pow(DqSame1, -1.0 / (sc.q + 1));

	r = min(rSame0, rSame1);

	if (Equal(sc.m_dCurrentH / sc.Hmin, 1.0) && m_Parameters.m_bDontCheckTolOnMinStep)
		r = max(1.01, r);

	Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("t=%.12g (%d) %s[%s] %g rSame %g RateLimit %g for %d steps"), 
		GetCurrentTime(), 
		GetIntegrationStepNumber(),
		sc.Integrator.pMaxErrorDevice->GetVerbalName(),
		sc.Integrator.pMaxErrorDevice->VariableNameByPtr(sc.Integrator.pMaxErrorVariable),
		sc.Integrator.dMaxErrorVariable, r,
		sc.dRateGrowLimit < FLT_MAX ? sc.dRateGrowLimit : 0.0,
		sc.nStepsToEndRateGrow - sc.nStepsCount);

	if (sc.Integrator.nMaxErrorVariableEquation >= 0)
		(pRightVector + sc.Integrator.nMaxErrorVariableEquation)->nErrorHits++;

	return r;
}

double CDynaModel::GetRatioForHigherOrder()
{
	double rUp = 0.0;
	_ASSERTE(sc.q == 1);

	RightVector *pVectorBegin = pRightVector;
	RightVector *pVectorEnd = pRightVector + klu.MatrixSize();

	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::Reset);
	
	const double Methodl1[2] = { Methodl[sc.q - 1 + DET_ALGEBRAIC * 2][1],  Methodl[sc.q - 1 + DET_DIFFERENTIAL * 2][1] };

	while (pVectorBegin < pVectorEnd)
	{
		if (pVectorBegin->Atol > 0) 
		{
			struct ConvergenceTest *pCt = ConvTest + pVectorBegin->EquationType;
			double dNewValue = *pVectorBegin->pValue;
			// method consts lq can be 1 only
			double dError = pVectorBegin->GetWeightedError(pVectorBegin->Error - pVectorBegin->SavedError, dNewValue) * Methodl1[pVectorBegin->EquationType];
			pCt->AddError(dError * dError);
		}
		pVectorBegin++;
	}

	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::FinalizeSum);
	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::GetRMS);

	double DqUp0 = ConvTest[DET_ALGEBRAIC].dErrorSum    / Methodl[1][3];		// 4.5 gives better result than 3.0, calculated by formulas in Hindmarsh
	double DqUp1 = ConvTest[DET_DIFFERENTIAL].dErrorSum / Methodl[3][3];		// also 4.5 is LTE of BDF-2. 12 is LTE of ADAMS-2, so 4.5 seems correct

	double rUp0 = pow(DqUp0, -1.0 / (sc.q + 2));
	double rUp1 = pow(DqUp1, -1.0 / (sc.q + 2));

	rUp = min(rUp0, rUp1);

	return rUp;
}

double CDynaModel::GetRatioForLowerOrder()
{
	double rDown = 0.0;
	_ASSERTE(sc.q == 2);
	RightVector *pVectorBegin = pRightVector;
	RightVector *pVectorEnd = pRightVector + klu.MatrixSize();

	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::Reset);

	while (pVectorBegin < pVectorEnd)
	{
		if (pVectorBegin->Atol > 0)
		{
			struct ConvergenceTest *pCt = ConvTest + pVectorBegin->EquationType;
			double dNewValue = *pVectorBegin->pValue;
			// method consts lq can be 1 only
			double dError = pVectorBegin->GetWeightedError(pVectorBegin->Nordsiek[2], dNewValue);
			pCt->AddError(dError * dError);
		}
		pVectorBegin++;
	}

	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::FinalizeSum);
	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::GetRMS);

	double DqDown0 = ConvTest[DET_ALGEBRAIC].dErrorSum;
	double DqDown1 = ConvTest[DET_DIFFERENTIAL].dErrorSum;

	double rDown0 = pow(DqDown0, -1.0 / sc.q);
	double rDown1 = pow(DqDown1, -1.0 / sc.q);

	rDown = min(rDown0, rDown1);
	return rDown;
}

void CDynaModel::EnterDiscontinuityMode()
{
	if (!sc.m_bDiscontinuityMode)
	{
		sc.m_bDiscontinuityMode = true;
		sc.m_bZeroCrossingMode = false;
		ChangeOrder(1);
		RescaleNordsiek(sc.Hmin / sc.m_dCurrentH);
		SetH(0.0);
		//sc.m_bEnforceOut = true;
	}
}

// для устройств во всех контейнерах сбрасывает статус готовности функции
void CDynaModel::UnprocessDiscontinuity()
{
	for (auto&& it : m_DeviceContainers)
		it->UnprocessDiscontinuity();
}

bool CDynaModel::ProcessDiscontinuity()
{
	bool bRes = true;

	eDEVICEFUNCTIONSTATUS Status = DFS_NOTREADY;
		
	// функция работает только в режиме обработки разрыва
	if (sc.m_bDiscontinuityMode)
	{
		// работаем в двух вложенных циклах
		// первый работает до тех пор, пока есть запросы на обработку разрыва sc.m_bDiscontinuityRequest в процессе обработки разрыва
		// второй работает до тех пор, пока все устройства не обработают разрыв успешно
		while (1)
		{
			// сбрасываем все статусы готовности. На каждой итерации обработки разрыва все устройства
			// начинают обработку заново
			UnprocessDiscontinuity();
			// запрос на обработку разрыва сбрасываем
			sc.m_bDiscontinuityRequest = false;

			ChangeOrder(1);
			ptrdiff_t nTotalOKPds = -1;

			Status = DFS_NOTREADY;

			while (Status == DFS_NOTREADY)
			{
				// пока статус "неготово"
				ptrdiff_t nOKPds = 0;
				// обрабатываем разрывы для всех устройств во всех контейнерах
				for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end() && Status != DFS_FAILED; it++)
				{
					switch ((*it)->ProcessDiscontinuity(this))
					{
					case DFS_OK:
					case DFS_DONTNEED:
						nOKPds++; // если контейнер обработал разрыв успешно, считаем количество успехов
						break;
					case DFS_FAILED:
						Status = DFS_FAILED;	// если контейнер завалился - выходим из цикла обработки
						break;
					case DFS_NOTREADY:
						Status = DFS_NOTREADY;	// если контейнер не готов - повторяем
						break;
					}
				}
				// если количество успехов равно количеству конейнеров - модель обработала разрыв успешно, выходим из цикла
				if (nOKPds == m_DeviceContainers.size())
				{
					Status = DFS_OK;
					break;
				}
				else
				{
					// иначе считаем сколько было успехов на предыдущей итерации
					if (nTotalOKPds < 0)
						nTotalOKPds = nOKPds;	// если итерация первая, запоминаем сколько было на ней успехов
					else
						if (nTotalOKPds == nOKPds)	// если итерация вторая и далее, и количество успехов равно предыдущем количеству успехов
						{
							// это бесконечный цикл
							Status = DFS_FAILED;
							Log(CDFW2Messages::DFW2LOG_ERROR, DFW2::CDFW2Messages::m_cszProcessDiscontinuityLoopedInfinitely);
							break;
						}
				}
			}

			// если все ОК, но в процессе обработки разрыва был(и) запрос(ы) на обработку разрывов - повторяем цикл обработки
			if (!sc.m_bDiscontinuityRequest)
				break;
		}
		// инициализируем Нордсик
		ResetNordsiek();
	}
	else
		Status = DFS_OK;

	return CDevice::IsFunctionStatusOK(Status);
}

void CDynaModel::LeaveDiscontinuityMode()
{
	if (sc.m_bDiscontinuityMode)
	{
		sc.m_bDiscontinuityMode = false;
		for (auto&& it : m_DeviceContainers)
			it->LeaveDiscontinuityMode(this);
		SetH(sc.Hmin);
		ResetNordsiek();
	}
}

double CDynaModel::CheckZeroCrossing()
{
	double Kh = 1.0;
	for (auto&& it : m_DeviceContainers)
	{
		double Khi = it->CheckZeroCrossing(this);
		if (Khi < Kh)
			Kh = Khi;
	}
	return Kh;
}

void CDynaModel::AddZeroCrossingDevice(CDevice *pDevice)
{
	ZeroCrossingDevices.push_back(pDevice);
	if (ZeroCrossingDevices.size() >= static_cast<size_t>(klu.MatrixSize()))
		throw dfw2error(_T("CDynaModel::AddZeroCrossingDevice - matrix size overrun"));
}

void CDynaModel::GoodStep(double rSame)
{
	sc.m_bRetryStep = false;		// отказываемся от повтора шага, все хорошо
	sc.RefactorMatrix(false);		// отказываемся от рефакторизации Якоби
	sc.nSuccessfullStepsOfNewton++;
	sc.nMinimumStepFailures = 0;

	// рассчитываем количество успешных шагов и пройденного времени для каждого порядка
	sc.OrderStatistics[sc.q - 1].nSteps++;
	// переходим к новому рассчитанному времени с обновлением суммы Кэхэна
	sc.Advance_t0();

	if (rSame > 1.2)
	{
		// если шаг можно хорошо увеличить
		rSame /= 1.2;

		switch (sc.q)
		{
		case 1:
		{
			// если были на первом порядке, пробуем шаг для второго порядка
			double rHigher = GetRatioForHigherOrder() / 1.4;

			// call before step change
			UpdateNordsiek();

			// если второй порядок лучше чем первый
			if (rHigher > rSame)
			{
				// пытаемся перейти на второй порядок
				if (sc.FilterOrder(rHigher))
				{
					ConstructNordsiekOrder();
					SetH(sc.m_dCurrentH * sc.dFilteredOrder);
					ChangeOrder(2);
					RescaleNordsiek(sc.dFilteredOrder);
					Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(CDFW2Messages::m_cszStepAndOrderChanged, GetCurrentTime(), GetIntegrationStepNumber(), sc.q, GetH()));
				}
			}
			else
				sc.OrderChanged();	// сбрасываем фильтр изменения порядка, если перейти на второй неэффективно
		}
		break;

		case 2:
		{
			// если были на втором порядке, пробуем шаг для первого порядка
			double rLower = GetRatioForLowerOrder() / 1.3;
			// call before step change
			UpdateNordsiek();

			// если первый порядок лучше чем второй
			if (rLower > rSame)
			{
				// пытаемся перейти на первый порядок
				if (sc.FilterOrder(rLower))
				{
					SetH(sc.m_dCurrentH * sc.dFilteredOrder);
					ChangeOrder(1);
					RescaleNordsiek(sc.dFilteredOrder);
					Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(CDFW2Messages::m_cszStepAndOrderChanged, GetCurrentTime(), GetIntegrationStepNumber(), sc.q, GetH()));
				}
			}
			else
				sc.OrderChanged();	// сбрасываем фильтр изменения порядка, если перейти на первый неэффективно
		}
		break;
		}

		// запрашиваем возможность изменения (увеличения шага)
		if (sc.FilterStep(rSame))
		{
			// если фильтр дает разрешение на увеличение
			_ASSERTE(Equal(sc.m_dCurrentH, sc.m_dOldH));
			// запоминаем коэффициент увеличения только для репорта
			// потому что sc.dFilteredStep изменится в последующем 
			// RescaleNordsiek
			double k = sc.dFilteredStep;
			// рассчитываем новый шаг
			SetH(sc.m_dCurrentH * sc.dFilteredStep);
			// пересчитываем Nordsieck на новый шаг
			RescaleNordsiek(sc.dFilteredStep);
			Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(CDFW2Messages::m_cszStepChanged, GetCurrentTime(), GetIntegrationStepNumber(), GetH(), k, sc.q));
		}
	}
	else
	{
		// если шаг хорошо увеличить нельзя, обновляем Nordsieck, сбрасываем фильтр шага, если его увеличивать неэффективно
		UpdateNordsiek(true);
		// если мы находимся здесь - можно включить BDF по дифурам, на случай, если шаг не растет из-за рингинга
		// но нужно посчитать сколько раз мы не смогли увеличить шаг и перейти на BDF, скажем, на десятом шагу
		// причем делать это нужно после вызова UpdateNordsiek(), так как в нем BDF отменяется
		sc.StepChanged();
	}
}

// обработка шага с недопустимой погрешностью
bool CDynaModel::BadStep()
{
	bool bRes(true);

	SetH(sc.m_dCurrentH * 0.5);		// делим шаг пополам
	sc.RefactorMatrix(true);	// принудительно рефакторизуем матрицу
	sc.m_bEnforceOut = false;	// отказываемся от вывода данных на данном заваленном шаге

	Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(CDFW2Messages::m_cszStepChangedOnError, GetCurrentTime(), GetIntegrationStepNumber(),
		GetH() < sc.Hmin ? sc.Hmin : GetH(), 
		sc.Integrator.dMaxErrorVariable,
		sc.Integrator.pMaxErrorDevice->GetVerbalName(),
		*sc.Integrator.pMaxErrorVariable,
		sc.Integrator.pMaxErrorDevice->VariableNameByPtr(sc.Integrator.pMaxErrorVariable),
		pRightVector[sc.Integrator.nMaxErrorVariableEquation].Nordsiek[0],
		pRightVector[sc.Integrator.nMaxErrorVariableEquation].Nordsiek[1]));

	// считаем статистику по заваленным шагам для текущего порядка метода интегрирования
	sc.OrderStatistics[sc.q - 1].nFailures++;

	// если шаг снизился до минимума
	if (sc.m_dCurrentH < sc.Hmin)
	{
		if (++sc.nMinimumStepFailures > m_Parameters.m_nMinimumStepFailures)
		{
			Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszFailureAtMinimalStep, GetCurrentTime(), GetIntegrationStepNumber(), sc.q, GetH()));
			bRes = false;
		}
		ChangeOrder(1);				// шаг не изменяем
		SetH(sc.Hmin);

		// проверяем количество последовательно
		// заваленных шагов
		if (sc.StepFailLimit())
		{
			// если можно еще делать шаги,
			// обновляем Nordsieck и пересчитываем 
			// производные
			UpdateNordsiek();
			BuildDerivatives();
		}
		else
			ReInitializeNordsiek(); // если заваленных шагов слишком много, делаем новый Nordsieck

		// шагаем на следующее время без возможности возврата, так как шаг уже не снизить
		sc.Advance_t0();
	}
	else
	{
		// если шаг еще можно снизить
		sc.nMinimumStepFailures = 0;
		RestoreNordsiek();									// восстанавливаем Nordsieck c предыдущего шага
		RescaleNordsiek(sc.m_dCurrentH / sc.m_dOldH);		// масштабируем Nordsieck на новый (половинный см. выше) шаг
		sc.CheckAdvance_t0();
	}

	return bRes;
}

bool CDynaModel::NewtonFailed()
{
	bool bRes(true);

	if (GetIntegrationStepNumber() == 2052)
		DumpStateVector();


	if (!sc.m_bDiscontinuityMode)
	{
		sc.OrderStatistics[sc.q - 1].nNewtonFailures++;
	}
	else
		sc.nDiscontinuityNewtonFailures++;

	if (sc.nSuccessfullStepsOfNewton > 10)
	{
		if (sc.m_dOldH / sc.m_dCurrentH >= 0.8)
		{
			SetH(sc.m_dCurrentH * 0.87);
			sc.SetRateGrowLimit(1.0);
		}
		else
		{
			SetH(0.8 * sc.m_dOldH + 0.2 * sc.m_dCurrentH);
			sc.SetRateGrowLimit(1.18);
		}
	}
	else
	if (sc.nSuccessfullStepsOfNewton > 1)
	{
		SetH(sc.m_dCurrentH * 0.87);
		sc.SetRateGrowLimit(1.0);
	}
	else
	if (sc.nSuccessfullStepsOfNewton == 0)
	{
		SetH(sc.m_dCurrentH * 0.25);
		sc.SetRateGrowLimit(10.0);
	}

	sc.nSuccessfullStepsOfNewton = 0;

	ChangeOrder(1);

	if (sc.m_dCurrentH < sc.Hmin)
	{
		SetH(sc.Hmin);
		if (++sc.nMinimumStepFailures > m_Parameters.m_nMinimumStepFailures)
		{
			Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszFailureAtMinimalStep, GetCurrentTime(), GetIntegrationStepNumber(), sc.q, GetH()));
			bRes = false;
		}
	}
	else
		sc.nMinimumStepFailures = 0;

	if (bRes)
	{
		sc.CheckAdvance_t0();

		if (sc.Hmin / sc.m_dCurrentH > 0.99)
			ResetNordsiek();
		else
			ReInitializeNordsiek();

		sc.RefactorMatrix();
		Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(CDFW2Messages::m_cszStepAndOrderChangedOnNewton, GetCurrentTime(), GetIntegrationStepNumber(), sc.q, GetH()));
	}

	return bRes;
}

// функция подготовки к повтору шага
// для поиска зерокроссинга
void CDynaModel::RepeatZeroCrossing()
{
	if (sc.m_dCurrentH < sc.Hmin)
	{
		// если шаг снижен до минимального,
		// отменяем зерокроссинг, так как его невозможно выполнить
		SetH(sc.Hmin);
		sc.m_bZeroCrossingMode = false;
	}
	sc.OrderStatistics[sc.q - 1].nZeroCrossingsSteps++;
	// восстанавливаем Nordsieck с предыдущего шага
	RestoreNordsiek();
	// масштабируем на шаг зерокроссинга (m_dCurrentH уже должен быть настроен)
	// старый шаг m_dOldH еще не успели изменить
	RescaleNordsiek(sc.m_dCurrentH / sc.m_dOldH);
	sc.CheckAdvance_t0();
	Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(CDFW2Messages::m_cszZeroCrossingStep, GetCurrentTime(), GetIntegrationStepNumber(), GetH()));
}


CDevice* CDynaModel::GetDeviceBySymbolicLink(const _TCHAR* cszObject, const _TCHAR* cszKeys, const _TCHAR* cszSymLink)
{
	CDevice *pFoundDevice(nullptr);

	CDeviceContainer *pContainer = GetContainerByAlias(cszObject);
	if (pContainer)
	{
		if (pContainer->GetType() == DEVTYPE_BRANCH)
		{
			ptrdiff_t nIp(0), nIq(0), nNp(0);
			bool bReverse = false;
			ptrdiff_t nKeysCount = _stscanf_s(cszKeys, _T("%td,%td,%td"), &nIp, &nIq, &nNp);
			if (nKeysCount > 1)
			{
				for (DEVICEVECTORITR it = pContainer->begin(); it != pContainer->end(); it++)
				{
					CDynaBranch *pDevice = static_cast<CDynaBranch*>(*it);
					if ((nKeysCount == 3 && pDevice->Np == nNp) || nKeysCount == 2)
					{
						if ( (pDevice->Ip == nIp && pDevice->Iq == nIq) || (pDevice->Ip == nIq && pDevice->Iq == nIp))
						{
							pFoundDevice = pDevice;
							break;
						}
					}
				}
			}
			else
				Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszWrongKeyForSymbolicLink, cszKeys, cszSymLink));
		}
		else
		{
			ptrdiff_t nId(0);

			if (_stscanf_s(cszKeys, _T("%td"), &nId) == 1)
				pFoundDevice = pContainer->GetDevice(nId);
			else
				Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszWrongKeyForSymbolicLink, cszKeys, cszSymLink));
		}
	}
	else
		Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszObjectNotFoundByAlias, cszObject, cszSymLink));

	return pFoundDevice;
}

bool CDynaModel::InitExternalVariable(PrimitiveVariableExternal& ExtVar, CDevice* pFromDevice, const _TCHAR* cszName)
{
	bool bRes = false;
	unsigned int nSourceLength = static_cast<unsigned int>(_tcslen(cszName));

	_TCHAR* szObject = new _TCHAR[nSourceLength];
	_TCHAR* szKeys = new _TCHAR[nSourceLength];
	_TCHAR* szProp = new _TCHAR[nSourceLength];

	int nFieldCount = _stscanf_s(cszName, _T("%[^[][%[^]]].%s"), szObject, nSourceLength, szKeys, nSourceLength, szProp, nSourceLength);

	if (nFieldCount == 3)
	{
		CDevice *pFoundDevice = GetDeviceBySymbolicLink(szObject, szKeys, cszName);
		if (pFoundDevice)
		{
			// Сначала ищем переменную состояния, со значением и с индексом
			ExternalVariable extVar = pFoundDevice->GetExternalVariable(szProp);
			if (extVar.pValue)
			{
				ExtVar.IndexAndValue(extVar.nIndex - pFromDevice->A(0), extVar.pValue);
				bRes = true;
			}
			else
			{
				// если в девайсе такой нет, ищем константу, со значением, но без
				// индекса
				extVar.pValue = pFoundDevice->GetConstVariablePtr(szProp);
				if (extVar.pValue)
				{
					// если нашли константу, объявляем ей индекс DFW2_NON_STATE_INDEX,
					// чтобы при формировании матрицы понимать, что это константа
					// и не ставить производную в индекс этой переменной
					ExtVar.IndexAndValue(DFW2_NON_STATE_INDEX, extVar.pValue);
					bRes = true;
				}
				else
				if (pFoundDevice->GetType() == DEVTYPE_BRANCH)
				{
					ptrdiff_t nIp;
					_stscanf_s(szKeys, _T("%td"), &nIp);
					CDynaBranch *pBranch = static_cast<CDynaBranch*>(pFoundDevice);

					if (!pBranch->m_pMeasure)
					{
						CDynaBranchMeasure *pBranchMeasure = new CDynaBranchMeasure(pBranch);
						pBranchMeasure->SetId(BranchMeasures.Count() + 1);
						pBranchMeasure->SetName(pBranch->GetVerbalName());
						pBranchMeasure->Init(this);
						BranchMeasures.AddDevice(pBranchMeasure);
						pBranch->m_pMeasure = pBranchMeasure;
					}

					ExternalVariable extVar = pBranch->m_pMeasure->GetExternalVariable(szProp);

					if (extVar.pValue)
					{
						ExtVar.IndexAndValue(extVar.nIndex - pFromDevice->A(0), extVar.pValue);
						bRes = true;
					}
					else
						Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszObjectHasNoPropBySymbolicLink, szProp, cszName));
				}
				else
					Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszObjectHasNoPropBySymbolicLink, szProp, cszName));
			}
		}
		else
			Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszObjectNotFoundBySymbolicLink, cszName));
	}
	else
		Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszWrongSymbolicLink, cszName));

	delete szObject;
	delete szKeys;
	delete szProp;

	return bRes;
}


double CDynaModel::gs1(KLUWrapper<double>& klu, unique_ptr<double[]>& Imb, const double *pSol)
{
	// если небаланс увеличился
	unique_ptr<double[]> yv = make_unique<double[]>(klu.MatrixSize());
	cs Aj;
	Aj.i = klu.Ap();
	Aj.p = klu.Ai();
	Aj.x = klu.Ax();
	Aj.m = Aj.n = klu.MatrixSize();
	Aj.nz = -1;

	// считаем градиент до итерации - умножаем матрицу якоби на вектор невязок до итерации
	cs_gatxpy(&Aj, Imb.get(), yv.get());

	// умножаем градиент на решение
	double gs1v(0.0);
	for (ptrdiff_t s = klu.MatrixSize() - 1; s >= 0; s--)
		gs1v += yv[s] * pSol[s];

	return gs1v;
}