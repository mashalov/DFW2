#include "stdafx.h"
#include "DynaModel.h"
#include "Automatic.h"

#define _LFINFO_

#include "klu.h"
#include "cs.h"
using namespace DFW2;

CDynaModel::CDynaModel() : Symbolic(NULL),
						   pRightVector(NULL),
						   m_ppVarSearchStackBase(NULL),
						   m_cszDampingName(NULL),
						   m_Discontinuities(this),
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
						   m_pLogFile(nullptr)
{
	Ax = NULL;
	Ai = Ap = NULL;
	pRightVector = NULL;

	m_hStopEvt = CreateEvent(NULL, TRUE, FALSE, _T("DFW2STOP"));

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
	_tfopen_s(&m_pLogFile, _T("c:\\tmp\\dfw2.log"), _T("w+"));
}


CDynaModel::~CDynaModel()
{
	CleanUpMatrix();

	if (m_hStopEvt)
		CloseHandle(m_hStopEvt);

	if (m_ppVarSearchStackBase)
		delete m_ppVarSearchStackBase;
}

bool CDynaModel::Run()
{
	bool bRes = true;

	//m_Parameters.m_dFrequencyTimeConstant = 1E-3;
	m_Parameters.eFreqDampingType = APDT_NODE;
	//m_Parameters.m_dOutStep = 1E-10;
	//m_Parameters.eFreqDampingType = APDT_ISLAND;
	//m_Parameters.m_eDiffEquationType = DET_ALGEBRAIC;
	//m_Parameters.bAllowRingingSuppression = true;
	

	m_Parameters.m_dAtol = 1E-4;
	m_Parameters.m_dMustangDerivativeTimeConstant = 1E-4;
	m_Parameters.m_bLogToConsole = false;
	m_Parameters.m_bLogToFile = true;

	//m_Parameters.m_dOutStep = 1E-5;
	m_Parameters.m_dRefactorByHRatio = 1.5;

	bRes = bRes && (LRCs.Init(this) == DFS_OK);

	bRes = bRes && Link();
	bRes = bRes && PrepareGraph();
	bRes = bRes && PrepareYs();
	bRes = bRes && Nodes.ProcessTopology();
	bRes = bRes && InitDevices();
	bRes = bRes && EstimateMatrix();
	bRes = bRes && InitEquations();

	/*
	CDynaLRC *pLRC = static_cast<CDynaLRC*>(LRCs.GetDevice(2));


	if (pLRC)
	{
		FILE *f = NULL;
		setlocale(LC_ALL, "RU-ru");
		_tfopen_s(&f, _T("c:\\tmp\\lrc.csv"), _T("wb+"));
		for (double dv = 0; dv < 1.5; dv += 0.01)
		{
			double dPl = 0.0;
			double Pl = pLRC->GetQdQ(dv, dPl, 0.0);
			_ftprintf_s(f, _T("%g;%g;%g\n"), dv, Pl, dPl);
		}
		fclose(f);
	}
	*/

#define SMZU

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
		sc.m_dCurrentH = 0.01;

		while (!sc.m_bStopCommandReceived && bRes)
		{
			bRes = bRes && Step();
			if (!sc.m_bStopCommandReceived)
				bRes = bRes && WriteResults();
			if (WaitForSingleObject(m_hStopEvt, 0) == WAIT_OBJECT_0)
				break;
		}
	}

	if (!bRes)
		MessageBox(NULL, _T("Failed"), _T("Failed"), MB_OK);

	bRes = bRes && FinishWriteResults(); 

	Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("Steps count %d"), sc.nStepsCount);
	Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("Steps by 1st order count %d, failures %d Newton failures %d Time passed %f"),
																	sc.OrderStatistics[0].nSteps, 
																	sc.OrderStatistics[0].nFailures,
																	sc.OrderStatistics[0].nNewtonFailures,
																	sc.OrderStatistics[0].dTimePassed);

	Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("Steps by 2nd order count %d, failures %d Newton failures %d Time passed %f"),
																	sc.OrderStatistics[1].nSteps,
																	sc.OrderStatistics[1].nFailures,
																	sc.OrderStatistics[1].nNewtonFailures,
																	sc.OrderStatistics[1].dTimePassed);

	Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("Factors count %d Analyzings count %d"), sc.nFactorizationsCount, sc.nAnalyzingsCount);
	Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("Newtons count %d %f per step, failures at step %d failures at discontinuity %d"),
																 sc.nNewtonIterationsCount, 
																 static_cast<double>(sc.nNewtonIterationsCount) / sc.nStepsCount, 
																 sc.OrderStatistics[0].nNewtonFailures + sc.OrderStatistics[1].nNewtonFailures,
																 sc.nDiscontinuityNewtonFailures);

	GetWorstEquations(10);
	chrono::milliseconds CalcDuration = chrono::duration_cast<std::chrono::milliseconds>(chrono::high_resolution_clock::now() - sc.m_ClockStart);
	Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("Duration %g"), static_cast<double>(CalcDuration.count()) / 1E3);
	return bRes;
}

bool CDynaModel::InitDevices()
{
	eDEVICEFUNCTIONSTATUS Status = DFS_NOTREADY;

	if (!InitExternalVariables())
		Status = DFS_FAILED;

	KLU_defaults(&Common);

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
	bool bRes = InitExternalVariables();
	if (bRes)
	{
		struct RightVector *pVectorBegin = pRightVector;
		struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

		while (pVectorBegin < pVectorEnd)
		{
			//pVectorBegin->Nordsiek[0] = *pVectorBegin->pValue;
			pVectorBegin->Nordsiek[0] = pVectorBegin->SavedNordsiek[0] = *pVectorBegin->pValue;
			PrepareNordsiekElement(pVectorBegin);
			pVectorBegin++;
		}

		double dCurrentH = sc.m_dCurrentH;
		sc.m_dCurrentH = 0.0;
		sc.m_bDiscontinuityMode = true;

		if (!SolveNewton(100))
			bRes = false;
		if (!sc.m_bNewtonConverged)
			bRes = false;

		sc.m_bDiscontinuityMode = false;
		sc.m_dCurrentH = dCurrentH;
		sc.nStepsCount = 0;
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
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	ConvTest[0].Reset();
	ConvTest[1].Reset();


	// original Hindmarsh (2.99) suggests ConvCheck = 0.5 / (sc.q + 2), but i'm using tolerance 5 times lower
	double ConvCheck = 0.1 / (sc.q + 2);

	double dOldMaxAbsError = sc.Newton.dMaxErrorVariable;

	sc.Newton.Reset();

	// first check Newton convergence

	while (pVectorBegin < pVectorEnd)
	{
		double& db = b[pVectorBegin - pRightVector];
		pVectorBegin->Error += db;

		double dMaxErrorDevice = fabs(db);

		if (sc.Newton.nMaxErrorVariableEquation < 0 || sc.Newton.dMaxErrorVariable < dMaxErrorDevice)
		{
			sc.Newton.pMaxErrorDevice = pVectorBegin->pDevice;
			sc.Newton.pMaxErrorVariable = pVectorBegin->pValue;
			sc.Newton.dMaxErrorVariable = dMaxErrorDevice;
			sc.Newton.nMaxErrorVariableEquation = pVectorBegin - pRightVector;
		}

		double l0 = pVectorBegin->Error;
		l0 *= l[pVectorBegin->EquationType * 2 + (sc.q - 1)][0];

		double dNewValue = pVectorBegin->Nordsiek[0] + l0;

		double dOldValue = *pVectorBegin->pValue;
		*pVectorBegin->pValue = dNewValue;

		if (pVectorBegin->Atol > 0)
		{
			double dError = pVectorBegin->GetWeightedError(db, dOldValue);
			struct ConvergenceTest *pCt = ConvTest + pVectorBegin->EquationType;
			pCt->AddError(dError * dError);
		}
		pVectorBegin++;
	}

	ConvTest[0].GetConvergenceRatio();
	ConvTest[1].GetConvergenceRatio();


	if (ConvTest[0].dErrorSums < l[sc.q - 1][3] * ConvCheck &&
		ConvTest[1].dErrorSums < l[sc.q + 1][3] * ConvCheck)
	{
		sc.m_bNewtonConverged = true;
	}
	else
	{
		if (ConvTest[0].dCms > 1.0 || ConvTest[1].dCms > 1.0)
		{
			sc.RefactorMatrix();
			if ((sc.nNewtonIteration > 5 && sc.Hmin / sc.m_dCurrentH < 0.98 ) || 
				(sc.nNewtonIteration > 2 && sc.Newton.dMaxErrorVariable > 1.0) )
			{
				sc.m_bNewtonDisconverging = true;
			}

			if (!sc.m_bNewtonDisconverging)
			{
				double *pRh = new double[m_nMatrixSize];
				double *pRb = new double[m_nMatrixSize];
				memcpy(pRh, pRightHandBackup, sizeof(double) * m_nMatrixSize);
				memcpy(pRb, b, sizeof(double) * m_nMatrixSize);
				double g0 = sc.dRightHandNorm;
				BuildRightHand();
				double g1 = sc.dRightHandNorm;

				if (false && g0 < g1)
				{
					double *yv = new double[m_nMatrixSize];
					ZeroMemory(yv, sizeof(double)*m_nMatrixSize);
					cs Aj;
					Aj.i = Ap;
					Aj.p = Ai;
					Aj.x = Ax;
					Aj.m = Aj.n = m_nMatrixSize;
					Aj.nz = -1;

					cs_gatxpy(&Aj, pRh, yv);

					double gs1 = 0.0;
					for (ptrdiff_t s = 0; s < m_nMatrixSize; s++)
						gs1 += yv[s] * pRb[s];
					delete yv;
					double lambda = -0.5 * gs1 / (g1 - g0 - gs1);

					double lambdamin = 0.1;
					if (sc.m_bDiscontinuityMode)
						lambdamin = 1E-4;


					if (lambda > lambdamin && lambda < 1.0)
					{
						pVectorBegin = pRightVector;
						while (pVectorBegin < pVectorEnd)
						{
							double& db = pRb[pVectorBegin - pRightVector];
							pVectorBegin->Error -= db;
							pVectorBegin->Error += db * lambda;
							double l0 = pVectorBegin->Error;
							l0 *= l[pVectorBegin->EquationType * 2 + (sc.q - 1)][0];
							*pVectorBegin->pValue = pVectorBegin->Nordsiek[0] + l0;
							pVectorBegin++;
						}
					}
					else
						sc.m_bNewtonDisconverging = true;
				}

				delete pRh;
				delete pRb;
				sc.RefactorMatrix();
			}
		}
		else
			sc.RefactorMatrix(false);
	}

	ConvTest[0].NextIteration();
	ConvTest[1].NextIteration();


	bRes = bRes && NewtonUpdateDevices();

	return bRes;
}

bool CDynaModel::NewtonUpdateDevices()
{
	bool bRes = true;
	for (DEVICECONTAINERITR it = m_DeviceContainersNewtonUpdate.begin(); it != m_DeviceContainersNewtonUpdate.end() && bRes; it++)
	{
		bRes = bRes && (*it)->NewtonUpdateBlock(this);
	}
	return bRes;
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

		if (BuildMatrix())
		{
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

			double bmax = 0.0;
			ptrdiff_t imax = 0;
			for (int r = 0; r < m_nMatrixSize; r++)
			{

				_CheckNumber(b[r]);

				if (bmax < abs(b[r]))
				{
					imax = r;
					bmax = abs(b[r]);
				}
			}

			if (SolveLinearSystem())
			{
				bmax = 0.0;

				for (int r = 0; r < m_nMatrixSize; r++)
				{
					_CheckNumber(b[r]);

					if (bmax < abs(b[r]))
					{
						imax = r;
						bmax = abs(b[r]);
					}
				}

				if (NewtonUpdate())
				{
					IterationOK = true;

					if (sc.m_bNewtonConverged)
					{
#ifdef _LFINFO_
						Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("t=%g Converged in %3d iterations %g %s %g %g %s Saving %g"), GetCurrentTime(),
																						sc.nNewtonIteration,
																						sc.Newton.dMaxErrorVariable, 
																						sc.Newton.pMaxErrorDevice->GetVerbalName(),
																						*sc.Newton.pMaxErrorVariable, 
																						pRightVector[sc.Newton.nMaxErrorVariableEquation].Nordsiek[0],
																						sc.Newton.pMaxErrorDevice->VariableNameByPtr(sc.Newton.pMaxErrorVariable),
																						1.0 - static_cast<double>(sc.nFactorizationsCount) / sc.nNewtonIterationsCount);
						
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
							Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("t=%g Continue %3d iteration %g %s %g %g %s"), GetCurrentTime(),
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
			}
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
				sc.m_dCurrentH *= rHit; 						// меняем шаг
				RescaleNordsiek(rHit);							// пересчитываем Nordsieck на новый шаг
				Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(CDFW2Messages::m_cszStepAdjustedToDiscontinuity, GetCurrentTime(), sc.m_dCurrentH));
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
							bRes = bRes && EstimateMatrix();
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
			bRes = bRes && ServeDiscontinuityRequest();	// обрабатываем разрыв
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
									rSame = 0.0;
									// признаем шаг успешным
									GoodStep(rSame);
								}
								else
									sc.m_bRetryStep = false; // отменяем повтор шага, чтобы обработать разрыв
							}
							else
							{
								// если время зерокроссинга не достаточно точно определено подбираем новый шаг
								sc.m_dCurrentH *= rZeroCrossing;
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
									sc.m_bRetryStep = false; // отменяем повтор шага, чтобы обработать разрыв
							}
							else
							{
								// если время зерокроссинга внутри шага, входим в режим зерокроссинга
								sc.m_bZeroCrossingMode = true;
								// и пытаемся подобрать шаг до времени зерокроссинга
								sc.m_dCurrentH *= rZeroCrossing;
								RepeatZeroCrossing();
							}
						}
					}
					else
					{
						// шаг нужно уменьшить
						// отбрасываем шаг
						BadStep();
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
				}
			}
			else
			{
				// Ньютон не сошелся
				// отбрасываем шаг
				NewtonFailed();
			}
		}
		sc.nStepsCount++;
	}
	
	//_ASSERTE(_CrtCheckMemory());
	return bRes;

}

double CDynaModel::GetRatioForCurrentOrder()
{
	double r = 0.0;

	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	ConvTest[0].Reset();
	ConvTest[1].Reset();
	
	sc.Integrator.Reset();

	while (pVectorBegin < pVectorEnd)
	{
		if (pVectorBegin->Atol > 0)
		{
			const double *MethodConsts = l[pVectorBegin->EquationType * 2 + (sc.q - 1)];
			// compute again to not asking device via pointer
			double dNewValue = pVectorBegin->Nordsiek[0] + pVectorBegin->Error * MethodConsts[0];

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

	ConvTest[0].GetRMS();
	ConvTest[1].GetRMS();

	double DqSame0 = ConvTest[0].dErrorSum / l[sc.q - 1][3];
	double DqSame1 = ConvTest[1].dErrorSum / l[sc.q + 1][3];
	double rSame0 = pow(DqSame0, -1.0 / (sc.q + 1));
	double rSame1 = pow(DqSame1, -1.0 / (sc.q + 1));

	r = min(rSame0, rSame1);

	if (Equal(sc.m_dCurrentH / sc.Hmin, 1.0) && m_Parameters.m_bDontCheckTolOnMinStep)
		r = max(1.01, r);

	Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("t=%g %s[%s] %g rSame %g RateLimit %g for %d steps"), GetCurrentTime(), sc.Integrator.pMaxErrorDevice->GetVerbalName(),
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
	RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	ConvTest[0].Reset();
	ConvTest[1].Reset();

	while (pVectorBegin < pVectorEnd)
	{
		if (pVectorBegin->Atol > 0) 
		{
			const double *MethodConsts = l[pVectorBegin->EquationType * 2 + (sc.q - 1)];
			struct ConvergenceTest *pCt = ConvTest + pVectorBegin->EquationType;
			double dNewValue = *pVectorBegin->pValue;
			// method consts lq can be 1 only
			double dError = pVectorBegin->GetWeightedError(pVectorBegin->Error - pVectorBegin->SavedError, dNewValue) * MethodConsts[1];
			pCt->AddError(dError * dError);
		}
		pVectorBegin++;
	}

	ConvTest[0].GetRMS();
	ConvTest[1].GetRMS();

	double DqUp0 = ConvTest[0].dErrorSum / 4.5;		// 4.5 gives better result than 3.0, calculated by formulas in Hindmarsh
	double DqUp1 = ConvTest[1].dErrorSum / 12.0;	// also 4.5 is LTE of BDF-2. 12 is LTE of ADAMS-2, so 4.5 seems correct

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
	RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	ConvTest[0].Reset();
	ConvTest[1].Reset();

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

	ConvTest[0].GetRMS();
	ConvTest[1].GetRMS();

	double DqDown0 = ConvTest[0].dErrorSum;
	double DqDown1 = ConvTest[1].dErrorSum;

	double rDown0 = pow(DqDown0, -1.0 / sc.q);
	double rDown1 = pow(DqDown1, -1.0 / sc.q);

	rDown = min(rDown0, rDown1);
	return rDown;
}

bool CDynaModel::EnterDiscontinuityMode()
{
	if (!sc.m_bDiscontinuityMode)
	{
		sc.m_bDiscontinuityMode = true;
		sc.m_bZeroCrossingMode = false;
		ChangeOrder(1);
		RescaleNordsiek(sc.Hmin / sc.m_dCurrentH);
		sc.m_dCurrentH = 0.0;
		//sc.m_bEnforceOut = true;
	}
	return true;
}

void CDynaModel::UnprocessDiscontinuity()
{
	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
		(*it)->UnprocessDiscontinuity();
}

bool CDynaModel::ProcessDiscontinuity()
{
	bool bRes = true;

	eDEVICEFUNCTIONSTATUS Status = DFS_NOTREADY;
		
	if (sc.m_bDiscontinuityMode)
	{
		while (1)
		{

			UnprocessDiscontinuity();
			sc.m_bDiscontinuityRequest = false;

			ChangeOrder(1);
			ptrdiff_t nTotalOKPds = -1;

			Status = DFS_NOTREADY;

			while (Status == DFS_NOTREADY)
			{
				ptrdiff_t nOKPds = 0;

				for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end() && Status != DFS_FAILED; it++)
				{
					switch ((*it)->ProcessDiscontinuity(this))
					{
					case DFS_OK:
					case DFS_DONTNEED:
						nOKPds++; // count how many inits succeeded
						break;
					case DFS_FAILED:
						Status = DFS_FAILED;
						break;
					case DFS_NOTREADY:
						Status = DFS_NOTREADY;
						break;
					}
				}
				if (nOKPds == m_DeviceContainers.size())
				{
					Status = DFS_OK;
					break;
				}
				else
				{
					if (nTotalOKPds < 0)
						nTotalOKPds = nOKPds;
					else
						if (nTotalOKPds == nOKPds)
						{
							Status = DFS_FAILED;
							Log(CDFW2Messages::DFW2LOG_ERROR, DFW2::CDFW2Messages::m_cszProcessDiscontinuityLoopedInfinitely);
							break;
						}
				}
			}

			if (!sc.m_bDiscontinuityRequest)
				break;
		}
		ResetNordsiek();
	}
	else
		Status = DFS_OK;

	return CDevice::IsFunctionStatusOK(Status);
}

bool CDynaModel::LeaveDiscontinuityMode()
{
	bool bRes = true;
	if (sc.m_bDiscontinuityMode)
	{
		sc.m_bDiscontinuityMode = false;
		for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end() && bRes; it++)
		{
			bRes = bRes && (*it)->LeaveDiscontinuityMode(this);
		}
		sc.m_dCurrentH = sc.Hmin;
		ResetNordsiek();
	}
	return bRes;
}

double CDynaModel::CheckZeroCrossing()
{
	double Kh = 1.0;
	m_nZeroCrossingDevicesCount = 0;
	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end() ; it++)
	{
		double Khi = (*it)->CheckZeroCrossing(this);
		if (Khi < Kh)
			Kh = Khi;
	}
	return Kh;
}

bool CDynaModel::AddZeroCrossingDevice(CDevice *pDevice)
{
	bool bRes = true;
	if (m_nZeroCrossingDevicesCount < m_nMatrixSize)
		m_pZeroCrossingDevices[m_nZeroCrossingDevicesCount++] = pDevice;
	return bRes;
}

void CDynaModel::GoodStep(double rSame)
{
	sc.m_bRetryStep = false;		// отказываемся от повтора шага, все хорошо
	sc.RefactorMatrix(false);		// отказываемся от рефакторизации Якоби
	sc.nSuccessfullStepsOfNewton++;

	// рассчитываем количество успешных шагов и пройденного времени для каждого порядка
	sc.OrderStatistics[sc.q - 1].nSteps++;
	sc.OrderStatistics[sc.q - 1].dTimePassed += GetH();

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
					sc.m_dCurrentH *= sc.dFilteredOrder;
					ChangeOrder(2);
					RescaleNordsiek(sc.dFilteredOrder);
					Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(CDFW2Messages::m_cszStepAndOrderChanged, GetCurrentTime(), sc.q, GetH()));
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
					sc.m_dCurrentH *= sc.dFilteredOrder;
					ChangeOrder(1);
					RescaleNordsiek(sc.dFilteredOrder);
					Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(CDFW2Messages::m_cszStepAndOrderChanged, GetCurrentTime(), sc.q, GetH()));
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
			sc.m_dCurrentH *= sc.dFilteredStep;
			// пересчитываем Nordsieck на новый шаг
			RescaleNordsiek(sc.dFilteredStep);
			Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(CDFW2Messages::m_cszStepChanged, GetCurrentTime(), GetH(), k, sc.q));
		}
	}
	else
	{
		// если шаг хорошо увеличить нельзя, обновляем Nordsieck, сбрасываем фильтр шага, если его увеличивать неэффективно
		UpdateNordsiek();
		// если мы находимся здесь - можно включить BDF по дифурам, на случай, если шаг не растет из-за рингинга
		// но нужно посчитать сколько раз мы не смогли увеличить шаг и перейти на BDF, скажем, на десятом шагу
		// причем делать это нужно после вызова UpdateNordsiek(), так как в нем BDF отменяется
		//m_Parameters.m_eDiffEquationType = DET_ALGEBRAIC;
		sc.StepChanged();
	}
}

// обработка шага с недопустимой погрешностью
void CDynaModel::BadStep()
{
	sc.m_dCurrentH *= 0.5;		// делим шаг пополам
	sc.RefactorMatrix(true);	// принудительно рефакторизуем матрицу
	sc.m_bEnforceOut = false;	// отказываемся от вывода данных на данном заваленном шаге

	Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(CDFW2Messages::m_cszStepChangedOnError, GetCurrentTime(),
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
		ChangeOrder(1);				// шаг не изменяем
		sc.m_dCurrentH = sc.Hmin;

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
		RestoreNordsiek();									// восстанавливаем Nordsieck c предыдущего шага
		RescaleNordsiek(sc.m_dCurrentH / sc.m_dOldH);		// масштабируем Nordsieck на новый (половинный см. выше) шаг
		sc.CheckAdvance_t0();
	}
}

void CDynaModel::NewtonFailed()
{
	if (!sc.m_bDiscontinuityMode)
	{
		sc.OrderStatistics[sc.q - 1].nNewtonFailures++;
	}
	else
		sc.nDiscontinuityNewtonFailures++;

	//if (sc.q == 2)
	//	sc.q++;

	if (sc.nSuccessfullStepsOfNewton > 10)
	{
		if (sc.m_dOldH / sc.m_dCurrentH >= 0.8)
		{
			sc.m_dCurrentH *= 0.87;
			sc.SetRateGrowLimit(1.0);
		}
		else
		{
			sc.m_dCurrentH = 0.8 * sc.m_dOldH + 0.2 * sc.m_dCurrentH;
			sc.SetRateGrowLimit(1.18);
		}
	}
	else
	if (sc.nSuccessfullStepsOfNewton > 1)
	{
		sc.m_dCurrentH *= 0.87;
		sc.SetRateGrowLimit(1.0);
	}
	else
	if (sc.nSuccessfullStepsOfNewton == 0)
	{
		sc.m_dCurrentH *= 0.25;
		sc.SetRateGrowLimit(10.0);
	}

	sc.nSuccessfullStepsOfNewton = 0;

	ChangeOrder(1);

	if (sc.m_dCurrentH < sc.Hmin)
		sc.m_dCurrentH = sc.Hmin;

	sc.CheckAdvance_t0();

	ReInitializeNordsiek();
	sc.RefactorMatrix();
	Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(CDFW2Messages::m_cszStepAndOrderChangedOnNewton, GetCurrentTime(), sc.q, GetH()));
}

void CDynaModel::RepeatZeroCrossing()
{
	if (sc.m_dCurrentH < sc.Hmin)
	{
		sc.m_dCurrentH = sc.Hmin;
		sc.m_bZeroCrossingMode = false;
	}

	//ChangeOrder(1);

	RestoreNordsiek();
	RescaleNordsiek(sc.m_dCurrentH / sc.m_dOldH);
	sc.CheckAdvance_t0();
	Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(CDFW2Messages::m_cszZeroCrossingStep, GetCurrentTime(), GetH()));
}


CDevice* CDynaModel::GetDeviceBySymbolicLink(const _TCHAR* cszObject, const _TCHAR* cszKeys, const _TCHAR* cszSymLink)
{
	CDevice *pFoundDevice = NULL;

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