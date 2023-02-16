/*
* Raiden - программа для моделирования длительных электромеханических переходных процессов в энергосистеме
* (С) 2018 - 2021 Евгений Машалов
* Raiden - long-term power system transient stability simulation module
* (C) 2018 - 2021 Eugene Mashalov
*/

#include "stdafx.h"
#include "DynaModel.h"
#include "DynaGeneratorMustang.h"
#include "DynaGeneratorPark3C.h"
#include "DynaGeneratorPark4C.h"
#include "DynaGeneratorInfBus.h"
#include "DynaExciterMustang.h"
#include "DynaDECMustang.h"
#include "DynaExcConMustang.h"
#include "DynaExcConMustangNW.h"
#include "DynaBranch.h"
#include "BranchMeasures.h"
#include "NodeMeasures.h"
#include "TestDevice.h"

#define _LFINFO_

#include "klu.h"
#include "cs.h"
using namespace DFW2;

CDynaModel::CDynaModel(const DynaModelParameters& ExternalParameters) : 
						   SSE2Available_(IsSSE2Available()),
						   m_Discontinuities(this),
						   Automatic_(this),
						   Scenario_(this),
						   Nodes(this),
						   Branches(this),
						   Generators1C(this),
						   Generators3C(this),
						   GeneratorsPark3C(this),
						   GeneratorsPark4C(this),
						   GeneratorsMustang(this),
						   GeneratorsMotion(this),
						   GeneratorsInfBus(this),
						   GeneratorsPowerInjector(this),
						   LRCs(this),
						   Reactors(this),
						   SynchroZones(this),
						   ExcitersMustang(this),
						   DECsMustang(this),
						   ExcConMustang(this),
						   CustomDevice(this),
						   BranchMeasures(this),
						   NodeMeasures(this),
						   ZeroLoadFlow(this),
						   AutomaticDevice(this),
						   ScenarioDevice(this),
						   TestDevices(this),
						   CustomDeviceCPP(this),
						   m_ResultsWriter(*this),
						   m_Platform(*this)
{
	static_cast<DynaModelParameters&>(m_Parameters) = ExternalParameters;
	CDynaNode::DeviceProperties(Nodes.ContainerProps());
	CSynchroZone::DeviceProperties(SynchroZones.ContainerProps());
	CDynaBranch::DeviceProperties(Branches.ContainerProps());
	CDynaLRC::DeviceProperties(LRCs.ContainerProps());
	CDynaReactor::DeviceProperties(Reactors.ContainerProps());
	CDynaPowerInjector::DeviceProperties(GeneratorsPowerInjector.ContainerProps());
	CDynaGeneratorInfBus::DeviceProperties(GeneratorsInfBus.ContainerProps());
	CDynaGeneratorMotion::DeviceProperties(GeneratorsMotion.ContainerProps());
	CDynaGenerator1C::DeviceProperties(Generators1C.ContainerProps());
	CDynaGenerator3C::DeviceProperties(Generators3C.ContainerProps());
	CDynaGeneratorMustang::DeviceProperties(GeneratorsMustang.ContainerProps());
	CDynaGeneratorPark3C::DeviceProperties(GeneratorsPark3C.ContainerProps());
	CDynaGeneratorPark4C::DeviceProperties(GeneratorsPark4C.ContainerProps());
	CDynaDECMustang::DeviceProperties(DECsMustang.ContainerProps());
	CDynaExcConMustang::DeviceProperties(ExcConMustang.ContainerProps());
	//CDynaExcConMustangNonWindup::DeviceProperties(ExcConMustang.ContainerProps());
	CDynaExciterMustang::DeviceProperties(ExcitersMustang.ContainerProps());
	CDynaBranchMeasure::DeviceProperties(BranchMeasures.ContainerProps());
	CDynaNodeMeasure::DeviceProperties(NodeMeasures.ContainerProps());
	CDynaNodeZeroLoadFlow::DeviceProperties(ZeroLoadFlow.ContainerProps());
	CTestDevice::DeviceProperties(TestDevices.ContainerProps());

	// указываем фабрику устройства здесь - для автоматики свойства не заполняются
	AutomaticDevice.ContainerProps().DeviceFactory = std::make_unique<CDeviceFactory<CCustomDeviceCPP>>();
	ScenarioDevice.ContainerProps().DeviceFactory = std::make_unique<CDeviceFactory<CCustomDeviceCPP>>();
	CustomDevice.ContainerProps().DeviceFactory = std::make_unique<CDeviceFactory<CCustomDevice>>();
	CustomDeviceCPP.ContainerProps().DeviceFactory = std::make_unique<CDeviceFactory<CCustomDeviceCPP>>();

	m_DeviceContainers.push_back(&Nodes);
	m_DeviceContainers.push_back(&LRCs);
	m_DeviceContainers.push_back(&Reactors);
	m_DeviceContainers.push_back(&ExcitersMustang);
	m_DeviceContainers.push_back(&DECsMustang);
	m_DeviceContainers.push_back(&ExcConMustang);
	m_DeviceContainers.push_back(&Branches);
	m_DeviceContainers.push_back(&Generators3C);
	m_DeviceContainers.push_back(&GeneratorsMustang);
	m_DeviceContainers.push_back(&GeneratorsPark3C);
	m_DeviceContainers.push_back(&GeneratorsPark4C);
	m_DeviceContainers.push_back(&Generators1C);
	m_DeviceContainers.push_back(&GeneratorsMotion);
	m_DeviceContainers.push_back(&GeneratorsInfBus);
	m_DeviceContainers.push_back(&GeneratorsPowerInjector);
	//m_DeviceContainers.push_back(&CustomDevice);
	//m_DeviceContainers.push_back(&CustomDeviceCPP);
	m_DeviceContainers.push_back(&BranchMeasures);
	m_DeviceContainers.push_back(&NodeMeasures);
	m_DeviceContainers.push_back(&AutomaticDevice);
	m_DeviceContainers.push_back(&ScenarioDevice);
	m_DeviceContainers.push_back(&ZeroLoadFlow);
	m_DeviceContainers.push_back(&TestDevices);
	m_DeviceContainers.push_back(&SynchroZones);		// синхрозоны должны идти последними

	CheckFolderStructure();

	if (m_Parameters.m_eFileLogLevel != DFW2MessageStatus::DFW2LOG_NONE)
	{
		const auto logPath{ std::filesystem::path(Platform().Logs()).append("dfw2.log") };
		const auto debugLogPath{ std::filesystem::path(Platform().Logs()).append("debug.log") };
		LogFile.open(logPath, std::ios::out);
		DebugLogFile.open(debugLogPath, std::ios::out);
		if (LogFile.is_open())
		{
			LogFile << fmt::format(CDFW2Messages::m_cszLogStarted, 
				CDFW2Messages::m_cszProjectName,
				version,
				__DATE__,
				CDFW2Messages::m_cszOS,
				stringutils::enum_text(m_Parameters.m_eFileLogLevel, m_Parameters.m_cszLogLevelNames)) << std::endl;
		}
		else
			throw dfw2errorGLE(fmt::format(CDFW2Messages::m_cszStdFileStreamError, stringutils::utf8_encode(logPath.c_str())));
	}

	if (!SSE2Available_)
		throw dfw2error(CDFW2Messages::m_cszNoSSE2Support);
}



CDynaModel::~CDynaModel()
{
	if(LogFile.is_open())
		LogFile.close();
	if (DebugLogFile.is_open())
		DebugLogFile.close();
}

bool CDynaModel::RunTransient()
{
	Automatic().CompileModels();

	AutomaticDevice.ConnectDLL(Automatic().GetModulePath());
	AutomaticDevice.CreateDevices(1);
	AutomaticDevice.BuildStructure();
	AutomaticDevice.GetDeviceByIndex(0)->SetName(CDFW2Messages::m_cszAutomaticName);

	Scenario().CompileModels();
	ScenarioDevice.ConnectDLL(Scenario().GetModulePath());
	ScenarioDevice.CreateDevices(1);
	ScenarioDevice.BuildStructure();
	ScenarioDevice.GetDeviceByIndex(0)->SetName(CDFW2Messages::m_cszScenarioName);

		
	bool bRes{ true };
#ifdef _WIN64
	_set_FMA3_enable(1);
#endif

	/*
	unsigned int u;
	unsigned int control_word;
	_controlfp_s(&control_word, 0, 0);
	u = control_word & ~(_EM_INVALID
		| _EM_DENORMAL
		| _EM_ZERODIVIDE
		| _EM_OVERFLOW
		| _EM_UNDERFLOW
		| _EM_INEXACT);
	_controlfp_s(&control_word, u, _MCW_EM);
	*/

	try
	{
		//m_Parameters.m_bConsiderDampingEquation = false;
		//m_Parameters.m_eParkParametersDetermination = PARK_PARAMETERS_DETERMINATION_METHOD::Canay;
		//m_Parameters.m_bUseRefactor = false;
		//m_Parameters.m_eGeneratorLessLRC = GeneratorLessLRC::Iconst;
		//m_Parameters.m_dLRCToShuntVmin = 0.5;
		//m_Parameters.m_dZeroBranchImpedance = 4.0E-6;
		//m_Parameters.m_dProcessDuration = 15;
		//m_Parameters.m_dFrequencyTimeConstant = 0.04;
		//m_Parameters.eFreqDampingType = ACTIVE_POWER_DAMPING_TYPE::APDT_NODE;
		//m_Parameters.eFreqDampingType = APDT_ISLAND;
		//m_Parameters.m_eDiffEquationType = DET_ALGEBRAIC;
		//m_Parameters.m_eAdamsRingingSuppressionMode = ADAMS_RINGING_SUPPRESSION_MODE::ARSM_GLOBAL;
		//m_Parameters.m_eAdamsRingingSuppressionMode = ADAMS_RINGING_SUPPRESSION_MODE::ARSM_NONE;
		//m_Parameters.m_eAdamsRingingSuppressionMode = ADAMS_RINGING_SUPPRESSION_MODE::ARSM_INDIVIDUAL;
		//m_Parameters.m_eAdamsRingingSuppressionMode = ADAMS_RINGING_SUPPRESSION_MODE::ARSM_DAMPALPHA;
		//m_Parameters.m_nAdamsGlobalSuppressionStep = 15;
		//m_Parameters.m_nAdamsIndividualSuppressStepsRange = 150;
		//m_Parameters.m_dAtol = 1E-4;
		//m_Parameters.m_dRtol = 1E-4 ;
	//m_Parameters.m_bStopOnBranchOOS = m_Parameters.m_bStopOnGeneratorOOS = false;
		//m_Parameters.m_eParkPar metersDetermination = PARK_PARAMETERS_DETERMINATION_METHOD::Canay;
		//m_Parameters.m_bDisableResultsWriter = false;
		//m_Parameters.Flat = true;
		//m_Parameters.Startup = CLoadFlow::eLoadFlowStartupMethod::Seidell;
		//m_Parameters.SeidellIterations = 5;
		//m_Parameters.EnableSwitchIteration = 5;
		//m_Parameters.SeidellStep = 1.02;
		//m_Parameters.m_eFileLogLevel = DFW2MessageStatus::DFW2LOG_DEBUG;
		m_Parameters.Imb = 0.05 * Atol();
		//m_Parameters.DerLagTolerance_ = 10.0;

		PrecomputeConstants();

		// если в параметрах задан BDF для дифуров, отключаем
		// подавление рингинга
		if(m_Parameters.m_eDiffEquationType == DET_ALGEBRAIC)
			m_Parameters.m_eAdamsRingingSuppressionMode = ADAMS_RINGING_SUPPRESSION_MODE::ARSM_NONE;

		//m_Parameters.m_dOutStep = 1E-5;
		//bRes = bRes && (LRCs.Init(this) == eDEVICEFUNCTIONSTATUS::DFS_OK);
		bRes = bRes && (Reactors.Init(this) == eDEVICEFUNCTIONSTATUS::DFS_OK);

		if (!bRes)
			throw dfw2error(CDFW2Messages::m_cszWrongSourceData);

		if(!Link())
			throw dfw2error(CDFW2Messages::m_cszWrongSourceData);


		TurnOffDevicesByOffMasters();

		// записывать заголовок нужно сразу после линковки и отключения устройств без ведущих навсегда
		// иначе устройства могут изменить родителей после топологического анализа
		WriteResultsHeader();

		PrepareNetworkElements();

		LoadFlow();
		// выполняем предварительную инициализацию устройств
		// (расчет констант и валидация , не связанные с другими устройствами
		PreInitDevices();
		// Здесь вызываем особый вариант ProcessTopology, который проверяет
		// наличие созданных суперузлов, и если они уже есть - то не создает их
		// но создает все остальное (синхронные зоны и все что еще понадобится)
		Nodes.ProcessTopologyInitial();
		InitDevices();
		EstimateMatrix();
		bRes = bRes && InitEquations();

		//pLRC->TestDump("c:\\tmp\\lrctest2.csv");

	#define SMZU

		// Расчет может завалиться внутри цикла, например из-за
		// сингулярной матрицы, поэтому контролируем
		// была ли начата запись результатов. Если что-то записали - нужно завершить
		bool bResultsNeedToBeFinished = false;

		if (bRes)
		{
			m_Discontinuities.AddEvent(m_Parameters.m_dProcessDuration, new CModelActionStop());

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

	

			Automatic().Init();
			Scenario().Init();
			m_Discontinuities.Init();
			SetH(0.01);
			// сохраняем начальные условия
			// в истории Нордсика с тем чтобы иметь 
			// возможность восстановить их при сбое Ньютона
			// на первом шаге
			SaveNordsiek();

			//Serialize("c:\\tmp\\serialization.json");

			try
			{
				StartProgress();
				while (!CancelProcessing() && bRes)
				{
					bRes = bRes && Step();
					// внутри UpdateProgress проверяется команда останова
					// пользователем и на нее выдается StopProcess
					UpdateProgress();
					// проверка StopProcess
					if (!CancelProcessing())
					{
						/*
						if (GetCurrentTime() > 0.01)
							static_cast<CDynaGeneratorPark3C*>(*GeneratorsPark.begin())->Pt = 1650;
							//static_cast<CDynaGeneratorMustang*>(*GeneratorsMustang.begin())->Pt = 1650;*/
						
						if (bRes)
						{
							// если ошибок не было, пишем результаты
							WriteResults();
							bResultsNeedToBeFinished = true;  // если записали - то фиксируем признак завершения
						}
					}
					// обрабатываем автоматику, которая
					// может работать между шагами
					// и не требует точности определения пороговых
					// параметров
					bRes = bRes && ProcessOffStep();
					if (StabilityLost())
						break;
					if (OscillationsDecayed())
						break;
				}
			}
			catch (const std::exception& err)
			{
				if (bResultsNeedToBeFinished)
				{
					bResultsNeedToBeFinished = false;
					FinishWriteResults();
				}
				Log(DFW2MessageStatus::DFW2LOG_FATAL, FinalMessage_ = fmt::format("Ошибка в цикле расчета : {}", err.what()));
				bRes = false;
			}
			EndProgress();
		}

		// вне зависимости от результата завершаем запись результатов
		// по признаку завершения
		if (bResultsNeedToBeFinished)
			FinishWriteResults();

#ifdef _MSC_VER
		if (!bRes)
			MessageBox(NULL, L"Failed", L"Failed", MB_OK);
#endif

		DumpStatistics();
	}

	catch (const std::exception& err)
	{
		Log(DFW2MessageStatus::DFW2LOG_FATAL, FinalMessage_ = fmt::format("Исключение : {}", err.what()));
		bRes = false;
	}
	return bRes;
}

void CDynaModel::PreInitDevices()
{
	// выполняем валидацию параметров расчета
	CSerializerValidator Validator(this, m_Parameters.GetSerializer(), m_Parameters.GetValidator());
	eDEVICEFUNCTIONSTATUS Status{ Validator.Validate() };

	for (auto&& container : m_DeviceContainers)
		Status = CDevice::DeviceFunctionResult(Status, container->PreInit(this));

	if (!CDevice::IsFunctionStatusOK(Status))
		throw dfw2error(CDFW2Messages::m_cszWrongSourceData);
}

void CDynaModel::InitDevices()
{
	eDEVICEFUNCTIONSTATUS Status{ eDEVICEFUNCTIONSTATUS::DFS_NOTREADY };

	m_cszDampingName = (GetFreqDampingType() == ACTIVE_POWER_DAMPING_TYPE::APDT_ISLAND) ? CDynaNode::m_cszSz : CDynaNode::m_cszS;

	// Вызываем обновление внешних переменных чтобы получить значения внешних устройств. Индексов до построения матрицы пока нет
	if (!UpdateExternalVariables())
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	
	// создаем блок уравнений потокораспределения
	// в суперузлах
	CreateZeroLoadFlow();

	klu.Common()->ordering = 0; // используем amd (0) или colamd (1). 0 - лучше для userefactor = true, 1 - для userefactor = false

	ptrdiff_t nTotalOKInits = -1;
	
	while (Status == eDEVICEFUNCTIONSTATUS::DFS_NOTREADY && Status != eDEVICEFUNCTIONSTATUS::DFS_FAILED)
	{
		ptrdiff_t nOKInits = 0;

		for (auto&& it : m_DeviceContainers)
		{
			if (Status == eDEVICEFUNCTIONSTATUS::DFS_FAILED)
				break;

			switch (it->Init(this))
			{
			case eDEVICEFUNCTIONSTATUS::DFS_OK:
			case eDEVICEFUNCTIONSTATUS::DFS_DONTNEED:
				nOKInits++; // count how many inits succeeded
				break;
			case eDEVICEFUNCTIONSTATUS::DFS_FAILED:
				Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
				break;
			case eDEVICEFUNCTIONSTATUS::DFS_NOTREADY:
				Status = eDEVICEFUNCTIONSTATUS::DFS_NOTREADY;
				break;
			}
		}
		if (nOKInits == m_DeviceContainers.size())
		{
			Status = eDEVICEFUNCTIONSTATUS::DFS_OK;
			break;
		}
		else
		{
			if (nTotalOKInits < 0)
				nTotalOKInits = nOKInits;
			else
				if (nTotalOKInits == nOKInits)
				{
					Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
					Log(DFW2MessageStatus::DFW2LOG_ERROR, DFW2::CDFW2Messages::m_cszInitLoopedInfinitely);
					break;
				}
		}

		if (!CDevice::IsFunctionStatusOK(Status) && Status != eDEVICEFUNCTIONSTATUS::DFS_FAILED)
		{
			// инициализируем контейнеры
			for (auto&& it : m_DeviceContainers)
			{
				// для каждого контейнера получаем статус
				eDEVICEFUNCTIONSTATUS ContainerStatus  = it->Init(this);
				// если статус - failed - выводим сообщение о контейнере
				if (!CDevice::IsFunctionStatusOK(ContainerStatus))
				{
					Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(DFW2::CDFW2Messages::m_cszDeviceContainerFailedToInit, it->GetTypeName(), ContainerStatus));
					// и обновляем общий статус инициализации на failed
					Status = ContainerStatus;
				}
			}
		}
	}

	// Здесь делаем расчет шунтовой части нагрузки суперузлов,
	// так как СХН генераторов становятся доступны после завершения 
	// CDynaNodeBase::Init()
	if (!CDevice::IsFunctionStatusOK(Status))
		throw dfw2error(CDFW2Messages::m_cszWrongSourceData);

	Nodes.CalculateShuntParts();
}


bool CDynaModel::InitEquations()
{
	InitNordsiek();
	// Второй раз вызываем обновление внешних переменных чтобы получить реальные индексы элементов из матрицы
	bool bRes = UpdateExternalVariables(); 
	if (bRes)
	{
		const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };

		for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
		{
			//pVectorBegin->Nordsiek[0] = *pVectorBegin->pValue;
			pVectorBegin->Nordsiek[0] = pVectorBegin->SavedNordsiek[0] = *pVectorBegin->pValue;
			PrepareNordsiekElement(pVectorBegin);
		}

		double dCurrentH{ H() };
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

bool CDynaModel::NewtonUpdate()
{
	bool bRes{ true };

	sc.m_bNewtonConverged = false;
	sc.m_bNewtonDisconverging = false;
	sc.m_bNewtonStepControl = false;

	const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };

	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::Reset);

	// original Hindmarsh (2.99) suggests ConvCheck = 0.5 / (sc.q + 2), but i'm using tolerance 5 times lower
	const double ConvCheck{ 0.1 / (sc.q + 2.0) };

	double dOldMaxAbsError = sc.Newton.Absolute.dMaxError;

	// first check Newton convergence
	sc.Newton.Reset();

	// константы метода выделяем в локальный массив, определяя порядок метода для всех переменных один раз
	const double Methodl0[2] { Methodl[sc.q - 1 + DET_ALGEBRAIC * 2][0],  Methodl[sc.q - 1 + DET_DIFFERENTIAL * 2][0] };

	const double* const pB{ klu.B() };

	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
	{
		const double& db = *(pB + (pVectorBegin - pRightVector));
		pVectorBegin->Error += db;
		pVectorBegin->b = db;

		sc.Newton.Absolute.Update(pVectorBegin, std::abs(db));

#ifdef USE_FMA
		const double dNewValue{ std::fma(Methodl0[pVectorBegin->EquationType], pVectorBegin->Error, pVectorBegin->Nordsiek[0]) };
#else
		double l0{ pVectorBegin->Error };
		l0 *= Methodl0[pVectorBegin->EquationType];
		const double dNewValue{ pVectorBegin->Nordsiek[0] + l0 };
#endif

		const double dOldValue{ *pVectorBegin->pValue };

		*pVectorBegin->pValue = dNewValue;

		if (pVectorBegin->Atol > 0)
		{
			const double dError{ pVectorBegin->GetWeightedError(db, dOldValue) };
			sc.Newton.Weighted.Update(pVectorBegin, dError);
			_CheckNumber(dError);
			ConvergenceTest* pCt{ ConvTest + pVectorBegin->EquationType };
#ifdef _DEBUG
			// breakpoint place for nans
			_ASSERTE(!std::isnan(dError));
#endif
			pCt->AddError(dError);
		}
	}

	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::FinalizeSum);
	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::GetConvergenceRatio);

	bool bConvCheckConverged = ConvTest[DET_ALGEBRAIC].dErrorSums < Methodl[sc.q - 1][3] * ConvCheck &&
							   ConvTest[DET_DIFFERENTIAL].dErrorSums < Methodl[sc.q + 1][3] * ConvCheck &&
							   sc.Newton.Weighted.dMaxError < m_Parameters.m_dNewtonMaxNorm;

	if ( bConvCheckConverged )
	{
		sc.m_bNewtonConverged = true;
	}
	else
	{
		if (ConvTest[DET_ALGEBRAIC].dCms > 1.0 || 
			ConvTest[DET_DIFFERENTIAL].dCms > 1.0)
		{
			sc.RefactorMatrix();

			double lambdamin{ 0.1 };
			bool bLineSearch{ false };

			
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
			else if (sc.Hmin / H() > 0.95)
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
				else if(sc.nNewtonIteration > 2 && sc.Newton.Absolute.dMaxError > 1.0)
					sc.m_bNewtonDisconverging = true;
			}

			if (!sc.m_bNewtonDisconverging)
			{
				if (bLineSearch)
				{
					std::unique_ptr<double[]> pRh = std::make_unique<double[]>(klu.MatrixSize());			 // невязки до итерации	
					std::unique_ptr<double[]> pRb = std::make_unique<double[]>(klu.MatrixSize());			 // невязки после итерации
					std::copy(pRightHandBackup.get(), pRightHandBackup.get() + klu.MatrixSize(), pRh.get()); // копируем невязки до итерации
					std::copy(klu.B(), klu.B() + klu.MatrixSize(), pRb.get());								 // копируем невязки после итерации
					double g0 = sc.dRightHandNorm;															 // норма небаланса до итерации
					BuildRightHand();																		 // рассчитываем невязки после итерации
					double g1 = sc.dRightHandNorm;															 // норма небаланса после итерации

					if (g0 < g1)
					{
						
						// если небаланс увеличился
						double gs1v = gs1(klu, pRh, pRb.get());
						
						// считаем множитель
						double lambda = -0.5 * gs1v / (g1 - g0 - gs1v);

						if (lambda > lambdamin && lambda < 1.0)
						{
							for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
							{
								double& db = pRb[pVectorBegin - pRightVector];
								pVectorBegin->Error -= db;
								pVectorBegin->Error += db * lambda;
								double l0 = pVectorBegin->Error;
								l0 *= Methodl0[pVectorBegin->EquationType];
								*pVectorBegin->pValue = pVectorBegin->Nordsiek[0] + l0;
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
		bRes = bRes && Nodes.LULF();
		bRes = bRes && ProcessDiscontinuity();
		sc.m_bProcessTopology = false;
		_ASSERTE(sc.m_bDiscontinuityMode);
		sc.m_bDiscontinuityMode = true;
	}

	if (!bRes)
		return bRes;

	bRes = false;

	/*
	if (sc.m_bDiscontinuityMode)
	{
		sc.RefactorMatrix();
	}
	*/

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
				_tcprintf("\n%30s - %6.2f %6.2f %6.2f %6.2f", pNode->GetVerbalName(), pNode->V, pNode->Delta * 180 / 3.14159, pNode->Pnr, pNode->Qnr);
			}
			for (DEVICEVECTORITR it = Generators1C.begin(); it != Generators1C.end(); it++)
			{
				CDynaGenerator1C *pGen = static_cast<CDynaGenerator1C*>(*it);
				_tcprintf("\n%30s - %6.2f %6.2f", pGen->GetVerbalName(), pGen->P, pGen->Q);
			}
			for (DEVICEVECTORITR it = GeneratorsMotion.begin(); it != GeneratorsMotion.end(); it++)
			{
				CDynaGeneratorMotion *pGen = static_cast<CDynaGeneratorMotion*>(*it);
				_tcprintf("\n%30s - %6.2f %6.2f", pGen->GetVerbalName(), pGen->P, pGen->Q);
			}
			for (DEVICEVECTORITR it = GeneratorsInfBus.begin(); it != GeneratorsInfBus.end(); it++)
			{
				CDynaGeneratorInfBus *pGen = static_cast<CDynaGeneratorInfBus*>(*it);
				_tcprintf("\n%30s - %6.2f %6.2f", pGen->GetVerbalName(), pGen->P, pGen->Q);
			}
		}
#endif

		sc.nNewtonIterationsCount++;

		ptrdiff_t imax(0);
		double bmax = klu.FindMaxB(imax);

		double *bwatch = klu.B();

//		Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_DEBUG, "%g %d", bmax, imax);

		SolveLinearSystem();

		/*
		cs Aj;
		Aj.i = klu.Ap();
		Aj.p = klu.Ai();
		Aj.x = klu.Ax();
		Aj.m = Aj.n = klu.MatrixSize();
		Aj.nz = -1;

		double *y = new double[klu.MatrixSize()]();
		memcpy(y, bwatch, sizeof(double) * klu.MatrixSize());
		ZeroMemory(bwatch, sizeof(double) * klu.MatrixSize());

		cs_gatxpy(&Aj, y, bwatch);
		bmax = klu.FindMaxB(imax);


		memcpy(bwatch, y, sizeof(double) * klu.MatrixSize());
		*/

		
		/*
		if (sc.nStepsCount == 398)
		{
			DumpStateVector();
			klu.DumpMatrix(true);
		}
		*/
  	    

		bmax = klu.FindMaxB(imax);
//		Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_DEBUG, "%g %d", bmax, imax);

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
				Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("t={:15.012f} {} Converged{:>3} iteration {} MaxWeight {} Saving {:.2} Rcond {}", GetCurrentTime(),
																				sc.nStepsCount,
																				sc.nNewtonIteration,
																				sc.Newton.Absolute.Info(),
																				sc.Newton.Weighted.dMaxError,
																				1.0 - static_cast<double>(klu.FactorizationsCount() + klu.RefactorizationsCount()) / sc.nNewtonIterationsCount,
																				sc.dLastConditionNumber));
						
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
					Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("t={:15.012f} {} Continue {:>3} iteration {} MaxWeight {} Rcond {}", GetCurrentTime(),
						sc.nStepsCount,
						sc.nNewtonIteration,
						sc.Newton.Absolute.Info(),
						sc.Newton.Weighted.dMaxError,
						sc.dLastConditionNumber));
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
		if (sc.Newton.Absolute.pVector)
			sc.Newton.Absolute.pVector->ErrorHits++;
	}

	return bRes;
}

bool CDynaModel::ApplyChangesToModel()
{
	bool bRes{ true };
	// проверяем, не возникла ли при обработке разрыва необходимость обработки топологии
	if (sc.m_bProcessTopology)
		Nodes.ProcessTopology();

	// если возникла необходимость перестроения Якоби
	if (m_bRebuildMatrixFlag)
	{
		// строим ее заново
		// создаем новый вариант систем уравнений 
		// потокораспределения суперузлов
		CreateZeroLoadFlow();
		EstimateMatrix();
		bRes = UpdateExternalVariables();
	}
	// проверяем, не возникло ли новых запросов на обработку разрыва при обработке разрыва
	if (sc.m_eDiscontinuityLevel == DiscontinuityLevel::None)
		sc.m_bBeforeDiscontinuityWritten = false;		// если запросов нет - больше записывать до разрыва нечего

	return bRes;
}

bool CDynaModel::Step()
{
	bool bRes{ true };

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
			double rHit = (dTimeNextEvent - GetCurrentTime()) / H() * (1.0 - 1E-12);
			//sc.t0 = dTimeNextEvent;
			// если при этом настроенный коэффициент изменения шага больше погрешности
			if (rHit > DFW2_EPSILON)
			{
				SetH(H() * rHit); 							// меняем шаг
				RescaleNordsiek();							// пересчитываем Nordsieck на новый шаг
				Log(DFW2MessageStatus::DFW2LOG_DEBUG, 
					fmt::format(CDFW2Messages::m_cszStepAdjustedToDiscontinuity, 
						GetCurrentTime(), 
						GetIntegrationStepNumber(), 
						H()));
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
						bRes = m_Discontinuities.ProcessStaticEvents() != eDFW2_ACTION_STATE::AS_ERROR;
						bRes = bRes && ApplyChangesToModel();
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

	if (sc.m_eDiscontinuityLevel != DiscontinuityLevel::None)
	{
		// если был запрос на обработку разрыва
		if (sc.m_bBeforeDiscontinuityWritten)
		{
			// и мы уже записали состояние модели до разрыва
			ServeDiscontinuityRequest();	// обрабатываем разрыв
			bRes = bRes && ApplyChangesToModel();
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
	while (sc.m_bRetryStep && bRes && !CancelProcessing())
	{

		Predict();		// делаем прогноз Nordsieck для следуюшего времени

		/*if (GetIntegrationStepNumber() == 1354)
			SnapshotRightVector();
		if(GetIntegrationStepNumber() == 1356)
			CompareRightVector();
		*/

		bRes = bRes && SolveNewton(sc.m_bDiscontinuityMode ? 20 : 10);	// делаем корректор

		/*if (GetIntegrationStepNumber() == 3514)
		{
			SnapshotRightVector();
			bRes = bRes && SolveNewton(sc.m_bDiscontinuityMode ? 20 : 10);	// делаем корректор
			CompareRightVector();
			SnapshotRightVector();
			bRes = bRes && SolveNewton(sc.m_bDiscontinuityMode ? 20 : 10);	// делаем корректор
			CompareRightVector();
		}*/

		/*if (GetIntegrationStepNumber() == 3516)
			CompareRightVector();*/

		if (bRes)
		{
			// если корректор отработал без сбоев
			if (sc.m_bNewtonConverged)
			{
				// если Ньютон сошелся
				FinishStep();	// рассчитываем независимые переменные, которые не входят в систему уравнения

				if (!sc.m_bDiscontinuityMode)
				{
					DetectAdamsRinging();

					// и мы не находились в режиме обработки разрыва
					double rSame{ 0.0 };
					// рассчитываем возможный шаг для текущего порядка метода
					// если сделанный шаг не был сделан без предиктора
					if (sc.m_bNordsiekReset)
					{
						// если нордсик был сброшен не конторолируем предиктор
						Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("t={:15.012f} {} startup step",
							GetCurrentTime(),
							GetIntegrationStepNumber()));
					}
					else
						rSame = GetRatioForCurrentOrder(); // если нордиск не был сброшен выбираем шаг по соответствию предиктору

					// шаг выполнен успешно если это был стартап-шаг
					// после сброса предиктора или корректор и предиктор
					// дали возможность не уменьшать шаг
					if (rSame > 1.0 || sc.m_bNordsiekReset)
					{

						// если шаг можно увеличить
						// проверяем нет ли зерокроссинга
						double rZeroCrossing{ CheckZeroCrossing() };

						if (rZeroCrossing < 0)
							Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format("Negative ZC ratio rH={} at t={}",
								rZeroCrossing,
								GetCurrentTime()));

						if (sc.m_bZeroCrossingMode)
						{
							// если были в зерокроссинге проверяем точность попадания времени при поиске зерокроссинга
							if (ZeroCrossingStepReached(rZeroCrossing))
							{
								// если нашли время зерокроссинга выходим из режима зерокроссинга
								sc.m_bZeroCrossingMode = false;

								if (sc.m_eDiscontinuityLevel == DiscontinuityLevel::None)
								{
									// если не возникло запросов на обработку разрыва
									// обнуляем коэффициент шага, чтобы он не изменился
									rSame = 0.0;
									// и признаем шаг успешным
									GoodStep(rSame);
									sc.ZeroCrossingMisses++;
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
								RepeatZeroCrossing(rZeroCrossing);
								sc.ZeroCrossingMisses++;
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

								if (sc.m_eDiscontinuityLevel == DiscontinuityLevel::None)
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
								RepeatZeroCrossing(rZeroCrossing);
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
					sc.OrderStatistics[sc.q - 1].nSteps++;
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
	double r{ 0.0 };

	const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };
	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::Reset);

	sc.Integrator.Reset();

	const double Methodl0[2] { Methodl[sc.q - 1 + DET_ALGEBRAIC * 2][0],  Methodl[sc.q - 1 + DET_DIFFERENTIAL * 2][0] };

	for(RightVector* pVectorBegin = pRightVector ; pVectorBegin < pVectorEnd; pVectorBegin++)
	{
		if (pVectorBegin->Atol > 0)
		{
			// compute again to not asking device via pointer
#ifdef USE_FMA
			double dNewValue = std::fma(pVectorBegin->Error, Methodl0[pVectorBegin->EquationType], pVectorBegin->Nordsiek[0]);
#else
			const double dNewValue{ pVectorBegin->Nordsiek[0] + pVectorBegin->Error * Methodl0[pVectorBegin->EquationType] };
#endif
			const double dError{ pVectorBegin->GetWeightedError(dNewValue) };
			sc.Integrator.Weighted.Update(pVectorBegin, dError);
			struct ConvergenceTest* const pCt{ ConvTest + pVectorBegin->EquationType };
			pCt->AddError(dError);
		}
	}


	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::FinalizeSum);
	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::GetRMS);

	const double maxnorm{ 0.2 };

	// гибридная норма
	//ConvTest[DET_ALGEBRAIC].dErrorSum *= (1.0 - maxnorm);
	//ConvTest[DET_ALGEBRAIC].dErrorSum += maxnorm * sc.Integrator.Weighted.dMaxError;
	//ConvTest[DET_DIFFERENTIAL].dErrorSum *= (1.0 - maxnorm);
	//ConvTest[DET_DIFFERENTIAL].dErrorSum += maxnorm * sc.Integrator.Weighted.dMaxError;

	const double DqSame0{ ConvTest[DET_ALGEBRAIC].dErrorSum / Methodl[sc.q - 1][3] };
	const double DqSame1{ ConvTest[DET_DIFFERENTIAL].dErrorSum / Methodl[sc.q + 1][3] };
	const double rSame0{ pow(DqSame0, -1.0 / (sc.q + 1)) };
	const double rSame1{ pow(DqSame1, -1.0 / (sc.q + 1)) };

	r = (std::min)(rSame0, rSame1);

	if (Equal(H() / sc.Hmin, 1.0) && m_Parameters.m_bDontCheckTolOnMinStep)
		r = (std::max)(1.01, r);

	Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("t={:15.012f} {} {} rSame {} RateLimit {} for {} steps", 
		GetCurrentTime(), 
		GetIntegrationStepNumber(),
		sc.Integrator.Weighted.Info(),
		r,
		sc.dRateGrowLimit < (std::numeric_limits<double>::max)() ? sc.dRateGrowLimit : 0.0,
		sc.nStepsToEndRateGrow - sc.nStepsCount));

	// считаем ошибку в уравнении если шаг придется уменьшить
	if (r <= 1.0 && sc.Integrator.Weighted.pVector)
		sc.Integrator.Weighted.pVector->ErrorHits++;

	return r;
}

double CDynaModel::GetRatioForHigherOrder()
{
	double rUp{ 0.0 };
	_ASSERTE(sc.q == 1);

	const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };

	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::Reset);
	
	const double Methodl1[2] = { Methodl[sc.q - 1 + DET_ALGEBRAIC * 2][1],  Methodl[sc.q - 1 + DET_DIFFERENTIAL * 2][1] };

	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
	{
		if (pVectorBegin->Atol > 0) 
		{
			struct ConvergenceTest *pCt = ConvTest + pVectorBegin->EquationType;
			double dNewValue = *pVectorBegin->pValue;
			// method consts lq can be 1 only
			double dError = pVectorBegin->GetWeightedError(pVectorBegin->Error - pVectorBegin->SavedError, dNewValue) * Methodl1[pVectorBegin->EquationType];
			pCt->AddError(dError);
		}
	}

	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::FinalizeSum);
	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::GetRMS);

	const double DqUp0{ ConvTest[DET_ALGEBRAIC].dErrorSum / Methodl[1][3] };		// 4.5 gives better result than 3.0, calculated by formulas in Hindmarsh
	const double DqUp1{ ConvTest[DET_DIFFERENTIAL].dErrorSum / Methodl[3][3] };		// also 4.5 is LTE of BDF-2. 12 is LTE of ADAMS-2, so 4.5 seems correct

	const double rUp0{ pow(DqUp0, -1.0 / (sc.q + 2)) };
	const double rUp1{ pow(DqUp1, -1.0 / (sc.q + 2)) };

	rUp = (std::min)(rUp0, rUp1);

	return rUp;
}

double CDynaModel::GetRatioForLowerOrder()
{
	double rDown{ 0.0 };
	_ASSERTE(sc.q == 2);
	const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };
	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::Reset);

	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
	{
		if (pVectorBegin->Atol > 0)
		{
			struct ConvergenceTest* const pCt{ ConvTest + pVectorBegin->EquationType };
			const double dNewValue{ *pVectorBegin->pValue };
			// method consts lq can be 1 only
			const double dError{ pVectorBegin->GetWeightedError(pVectorBegin->Nordsiek[2], dNewValue) };
			pCt->AddError(dError);
		}
	}

	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::FinalizeSum);
	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::GetRMS);

	const double DqDown0{ ConvTest[DET_ALGEBRAIC].dErrorSum };
	const double DqDown1{ ConvTest[DET_DIFFERENTIAL].dErrorSum };

	const double rDown0{ pow(DqDown0, -1.0 / sc.q) };
	const double rDown1{ pow(DqDown1, -1.0 / sc.q) };

	rDown = (std::min)(rDown0, rDown1);
	return rDown;
}

void CDynaModel::EnterDiscontinuityMode()
{
	if (!sc.m_bDiscontinuityMode)
	{
		sc.m_bDiscontinuityMode = true;
		sc.m_bZeroCrossingMode = false;
		ChangeOrder(1);
		SetH(0.0);
		RescaleNordsiek();
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

	eDEVICEFUNCTIONSTATUS Status = eDEVICEFUNCTIONSTATUS::DFS_NOTREADY;
		
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
			sc.m_eDiscontinuityLevel = DiscontinuityLevel::None;
			sc.m_pDiscontinuityDevice = nullptr;

			ChangeOrder(1);
			ptrdiff_t nTotalOKPds = -1;

			Status = eDEVICEFUNCTIONSTATUS::DFS_NOTREADY;

			while (Status == eDEVICEFUNCTIONSTATUS::DFS_NOTREADY)
			{
				// пока статус "неготово"
				ptrdiff_t nOKPds = 0;
				// обрабатываем разрывы для всех устройств во всех контейнерах
				for (auto&& it : m_DeviceContainers)
				{
					if (Status == eDEVICEFUNCTIONSTATUS::DFS_FAILED)
						break;

					switch (it->ProcessDiscontinuity(this))
					{
					case eDEVICEFUNCTIONSTATUS::DFS_OK:
					case eDEVICEFUNCTIONSTATUS::DFS_DONTNEED:
						nOKPds++; // если контейнер обработал разрыв успешно, считаем количество успехов
						break;
					case eDEVICEFUNCTIONSTATUS::DFS_FAILED:
						Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;	// если контейнер завалился - выходим из цикла обработки
						break;
					case eDEVICEFUNCTIONSTATUS::DFS_NOTREADY:
						Status = eDEVICEFUNCTIONSTATUS::DFS_NOTREADY;	// если контейнер не готов - повторяем
						break;
					}
				}
				// если количество успехов равно количеству конейнеров - модель обработала разрыв успешно, выходим из цикла
				if (nOKPds == m_DeviceContainers.size())
				{
					Status = eDEVICEFUNCTIONSTATUS::DFS_OK;
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
							Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
							Log(DFW2MessageStatus::DFW2LOG_ERROR, DFW2::CDFW2Messages::m_cszProcessDiscontinuityLoopedInfinitely);
							break;
						}
				}
			}

			// если все ОК, но в процессе обработки разрыва был(и) запрос(ы) на обработку разрывов - повторяем цикл обработки
			if (sc.m_eDiscontinuityLevel == DiscontinuityLevel::None)
				break;
		}
		// инициализируем Нордсик
		ResetNordsiek();
	}
	else
		Status = eDEVICEFUNCTIONSTATUS::DFS_OK;

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
	double Kh{ 1.0 };
	for (auto&& it : m_DeviceContainers)
	{
		double Khi{ it->CheckZeroCrossing(this) };
		if (Khi < Kh)
		{
			Kh = Khi;
			m_pClosestZeroCrossingContainer = it;
			if (Kh < 0.0)
				it->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format("Negative ZC ratio rH={} at container \"{}\" at t={}",
					Kh,
					it->GetTypeName(),
					GetCurrentTime()
				));
		}
	}
	return Kh;
}

void CDynaModel::AddZeroCrossingDevice(CDevice *pDevice)
{
	ZeroCrossingDevices.push_back(pDevice);
	if (ZeroCrossingDevices.size() >= static_cast<size_t>(klu.MatrixSize()))
		throw dfw2error("CDynaModel::AddZeroCrossingDevice - matrix size overrun");
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
			const double rHigher{ GetRatioForHigherOrder() / 1.4 };
			// call before step change
			UpdateNordsiek();

			// если второй порядок лучше чем первый
			if (rHigher > rSame)
			{
				// пытаемся перейти на второй порядок
				if (sc.FilterOrder(rHigher))
				{
					ConstructNordsiekOrder();
					ChangeOrder(2);
					SetH(H() * sc.dFilteredOrder);
					RescaleNordsiek();
					Log(DFW2MessageStatus::DFW2LOG_DEBUG, 
						fmt::format(CDFW2Messages::m_cszStepAndOrderChanged, GetCurrentTime(), GetIntegrationStepNumber(), sc.q, H()));
				}
			}
			else
				sc.OrderChanged();	// сбрасываем фильтр изменения порядка, если перейти на второй неэффективно
		}
		break;

		case 2:
		{
			// если были на втором порядке, пробуем шаг для первого порядка
			const double rLower{ GetRatioForLowerOrder() / 1.3 / 5.0}; // 5.0 - чтобы уменьшить использование демпфирующего метода
			// call before step change
			UpdateNordsiek();

			// если первый порядок лучше чем второй
			if (rLower > rSame)
			{
				// пытаемся перейти на первый порядок
				if (sc.FilterOrder(rLower))
				{
					SetH(H() * sc.dFilteredOrder);
					ChangeOrder(1);
					RescaleNordsiek();
					Log(DFW2MessageStatus::DFW2LOG_DEBUG, 
						fmt::format(CDFW2Messages::m_cszStepAndOrderChanged, GetCurrentTime(), GetIntegrationStepNumber(), sc.q, H()));
				}
			}
			else
				sc.OrderChanged();	// сбрасываем фильтр изменения порядка, если перейти на первый неэффективно
		}
		break;
		}

		// запрашиваем возможность изменения (увеличения шага)
		// тут можно регулировать частоту увеличения шага 
		// rSame уже поделили выше для безопасности на 1.2
		if (sc.FilterStep(rSame) && rSame > 1.1)
		{
			// если фильтр дает разрешение на увеличение
			_ASSERTE(Equal(H(), sc.m_dOldH));
			// запоминаем коэффициент увеличения только для репорта
			// потому что sc.dFilteredStep изменится в последующем 
			// RescaleNordsiek
			const double k{ sc.dFilteredStep };
			// рассчитываем новый шаг
			// пересчитываем Nordsieck на новый шаг
			SetH(H() * sc.dFilteredStep);
			RescaleNordsiek();
			Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(CDFW2Messages::m_cszStepChanged, GetCurrentTime(), GetIntegrationStepNumber(), H(), k, sc.q));
			sc.StepChanged();
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
void CDynaModel::BadStep()
{
	double newH{ 0.5 * H() };
	double newEffectiveH{ (std::max)(newH, sc.Hmin) };

	sc.RefactorMatrix(true);	// принудительно рефакторизуем матрицу
	sc.m_bEnforceOut = false;	// отказываемся от вывода данных на данном заваленном шаге

	Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(CDFW2Messages::m_cszStepChangedOnError, GetCurrentTime(), GetIntegrationStepNumber(),
		newEffectiveH, sc.Integrator.Weighted.Info()));

	// считаем статистику по заваленным шагам для текущего порядка метода интегрирования
	sc.OrderStatistics[sc.q - 1].nFailures++;

	// если шаг снизился до минимума
	if (newH <= sc.Hmin && Equal(H(), sc.Hmin))
	{
		if (++sc.nMinimumStepFailures > m_Parameters.m_nMinimumStepFailures)
			throw dfw2error(fmt::format(CDFW2Messages::m_cszFailureAtMinimalStep, GetCurrentTime(), GetIntegrationStepNumber(), sc.q, H()));

		ChangeOrder(1);

		// проверяем количество последовательно
		// заваленных шагов
		if (sc.StepFailLimit())
		{
			// если можно еще делать шаги,
			// обновляем Nordsieck и пересчитываем 
			// производные
			UpdateNordsiek();
			SetH(newEffectiveH);
			RescaleNordsiek();
		}
		else
			ReInitializeNordsiek(); // если заваленных шагов слишком много, делаем новый Nordsieck

		// шагаем на следующее время без возможности возврата, так как шаг уже не снизить
		sc.Advance_t0();
		sc.Assign_t0();
	}
	else
	{
		// если шаг еще можно снизить
		sc.nMinimumStepFailures = 0;
		RestoreNordsiek();					// восстанавливаем Nordsieck c предыдущего шага
		SetH(newEffectiveH);
		RescaleNordsiek();					// масштабируем Nordsieck на новый (половинный см. выше) шаг
		sc.CheckAdvance_t0();
	}
}

void CDynaModel::NewtonFailed()
{
	// обновляем подсчет ошибок Ньютона
	if (!sc.m_bDiscontinuityMode)
	{
		// если вне разрыва - считаем для порядка шага
		sc.OrderStatistics[sc.q - 1].nNewtonFailures++;
	}
	else // если в разрыве - считаем количество завалов на разрывах
		sc.nDiscontinuityNewtonFailures++;

	if (sc.nSuccessfullStepsOfNewton > 10)
	{
		if (sc.m_dOldH / H() >= 0.8)
		{
			SetH(H() * 0.87);
			sc.SetRateGrowLimit(1.0);
		}
		else
		{
			SetH(0.8 * sc.m_dOldH + 0.2 * H());
			sc.SetRateGrowLimit(1.18);
		}
	}
	else
	if (sc.nSuccessfullStepsOfNewton >= 1)
	{
		SetH(H() * 0.87);
		sc.SetRateGrowLimit(1.0);
	}
	else
	if (sc.nSuccessfullStepsOfNewton == 0)
	{
		SetH(H() * 0.25);
		sc.SetRateGrowLimit(10.0);
	}

	sc.nSuccessfullStepsOfNewton = 0;

	ChangeOrder(1);

	if (H() < sc.Hmin)
	{
		SetH(sc.Hmin);
		if (++sc.nMinimumStepFailures > m_Parameters.m_nMinimumStepFailures)
			throw dfw2error(fmt::format(CDFW2Messages::m_cszFailureAtMinimalStep, GetCurrentTime(), GetIntegrationStepNumber(), sc.q, H()));
		sc.Advance_t0();
		sc.Assign_t0();
	}
	else
	{
		sc.nMinimumStepFailures = 0;
		sc.CheckAdvance_t0();
	}

	if (sc.Hmin / H() > 0.99)
		ResetNordsiek();
	else
		ReInitializeNordsiek();
	
	sc.RefactorMatrix();
	Log(DFW2MessageStatus::DFW2LOG_DEBUG, 
		fmt::format(CDFW2Messages::m_cszStepAndOrderChangedOnNewton, GetCurrentTime(), GetIntegrationStepNumber(), sc.q, H()));
}

// функция подготовки к повтору шага
// для поиска зерокроссинга
void CDynaModel::RepeatZeroCrossing(double rH)
{
	double rHstep{ H() * rH };
	// восстанавливаем Nordsieck с предыдущего шага
	RestoreNordsiek();
	// ограничиваем шаг до минимального
	if (rHstep < sc.Hmin)
	{
		rHstep = sc.Hmin;
		// переходим на первый порядок, так
		// как снижение шага может быть очень
		// значительным
		ChangeOrder(1);
	}
	sc.OrderStatistics[sc.q - 1].nZeroCrossingsSteps++;
	SetH(rHstep);
	RescaleNordsiek();
	sc.CheckAdvance_t0();
	Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(CDFW2Messages::m_cszZeroCrossingStep, GetCurrentTime(),
																				GetIntegrationStepNumber(), 
																				H(), 
																				m_pClosestZeroCrossingContainer->GetZeroCrossingDevice()->GetVerbalName(),
																				rH));
}

bool CDynaModel::InitExternalVariable(VariableIndexExternal& ExtVar, CDevice* pFromDevice, std::string_view Name)
{
	bool bRes{ false };
	const size_t nSourceLength{ Name.size() };
	const char cszSpace{ ' ' };
	std::string Object(nSourceLength, cszSpace),
				 Keys(nSourceLength, cszSpace),
				 Prop(nSourceLength, cszSpace);
#ifdef _MSC_VER
	int nFieldCount = sscanf_s(std::string(Name).c_str(), "%[^[][%[^]]].%s", &Object[0], static_cast<unsigned int>(nSourceLength),
																			 &Keys[0],   static_cast<unsigned int>(nSourceLength),
																			 &Prop[0],   static_cast<unsigned int>(nSourceLength));
#else
	int nFieldCount = sscanf(std::string(Name).c_str(), "%[^[][%[^]]].%s", &Object[0],&Keys[0],&Prop[0]);
#endif 
	// обрезаем длину строк до нуль-терминатора
	Object = Object.c_str();
	Keys   = Keys.c_str();
	Prop   = Prop.c_str();

	if (nFieldCount == 3)
	{
		if (CDevice* pFoundDevice{ GetDeviceBySymbolicLink(Object, Keys, Name) }; pFoundDevice)
		{
			// Сначала ищем переменную состояния, со значением и с индексом
			ExtVar = pFoundDevice->GetExternalVariable(Prop);
			if (ExtVar.pValue)
			{
				// смещение больше не нужно - работаем в абсолютных индексах
				//ExtVar.Index -= pFromDevice->A(0);

				// если работаем с узлами, учитываем что может быть
				// найдет slave-узел и подменяем его на суперузел
				if (pFoundDevice->GetType() == DEVTYPE_NODE)
					ExtVar = static_cast<CDynaNodeBase*>(pFoundDevice)->GetSuperNode()->GetExternalVariable(Prop);

				bRes = true;
			}
			else
			{
				// если в девайсе такой нет, ищем константу, со значением, но без
				// индекса
				ExtVar.pValue = pFoundDevice->GetConstVariablePtr(Prop);
				if (ExtVar.pValue)
				{
					// если нашли константу, объявляем ей индекс DFW2_NON_STATE_INDEX,
					// чтобы при формировании матрицы понимать, что это константа
					// и не ставить производную в индекс этой переменной
					ExtVar.Index = DFW2_NON_STATE_INDEX;
					bRes = true;
				}
				else if (pFoundDevice->GetType() == DEVTYPE_BRANCH)
				{
					// если не нашли параметр и тип объекта - "ветвь"
					// создаем для ветви блок измерений и пытаемся забрать параметр из блока
					CDynaBranch* pBranch{ static_cast<CDynaBranch*>(pFoundDevice) };
					if (!pBranch->pMeasure_)
					{
						CDynaBranchMeasure* pBranchMeasure{ new CDynaBranchMeasure() };
						pBranchMeasure->SetBranch(pBranch);
						pBranchMeasure->Init(this);
						BranchMeasures.AddDevice(pBranchMeasure);
						pBranch->pMeasure_ = pBranchMeasure;
					}
					//else
					//	pBranch->pMeasure_->Init(this);	/// ???????

					ExtVar = pBranch->pMeasure_->GetExternalVariable(Prop);

					if (ExtVar.pValue)
					{
						// смещение больше не нужно - работаем в абсолютных индексах
						//ExtVar.Index -= pFromDevice->A(0);
						bRes = true;
					}
					else
						Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszObjectHasNoPropBySymbolicLink, Prop, Name));
				}
				else if (pFoundDevice->GetType() == DEVTYPE_NODE)
				{
					// если не нашли параметр и тип объекта - "узел"
					// создаем блок измерений для узла и пытаемся забрать параметр из блока
					CDynaNode* pNode{ static_cast<CDynaNode*>(pFoundDevice) };
					if (!pNode->pMeasure)
					{
						CDynaNodeMeasure* pNodeMeasure{ new CDynaNodeMeasure(pNode) };
						pNodeMeasure->SetId(NodeMeasures.Count() + 1);
						pNodeMeasure->SetName(pNode->GetVerbalName());
						pNodeMeasure->Init(this);
						NodeMeasures.AddDevice(pNodeMeasure);
						pNode->pMeasure = pNodeMeasure;
					}

					ExtVar = pNode->pMeasure->GetExternalVariable(Prop);

					if (ExtVar.pValue)
					{
						// смещение больше не нужно - работаем в абсолютных индексах
						//ExtVar.Index -= pFromDevice->A(0);
						bRes = true;
					}
					else
						Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszObjectHasNoPropBySymbolicLink, Prop, Name));
				}
				else
					Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszObjectHasNoPropBySymbolicLink, Prop, Name));
			}
		}
		else
			Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszObjectNotFoundBySymbolicLink, Name));
	}
	else
		Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszWrongSymbolicLink, Name));

	return bRes;
}

// расчет градиента функции оптимального шага g(f(x+lambda*dx))
double CDynaModel::gs1(KLUWrapper<double>& klu, std::unique_ptr<double[]>& Imb, const double *pSol)
{
	// вектор результата умножения матрицы Якоби на вектор невязок
	std::unique_ptr<double[]> yv = std::make_unique<double[]>(klu.MatrixSize());

	// считаем градиент до итерации - умножаем матрицу якоби на вектор невязок до итерации
	klu.Multiply(Imb.get(), yv.get());

	// умножаем градиент на решение
	double gs1v(0.0);
	for (ptrdiff_t s = klu.MatrixSize() - 1; s >= 0; s--)
		gs1v += yv[s] * pSol[s];

	return gs1v;
}

/*
double CDynaModel::GradientNorm(KLUWrapper<double>& klu, const double* Sol)
{
	// формируем матрицу в виде пригодном для умножения на вектор
	cs Aj;
	Aj.i = klu.Ap();
	Aj.p = klu.Ai();
	Aj.x = klu.Ax();
	Aj.m = Aj.n = klu.MatrixSize();
	Aj.nz = -1;

	// считаем градиент до итерации - умножаем матрицу якоби на вектор невязок до итерации
	double* pb{ pNewtonGradient.get() };
	double* pe{ pNewtonGradient.get() + klu.MatrixSize() };
	std::fill(pb, pe, 0.0);
	cs_gatxpy(&Aj, Sol, pb);
	double Norm{ 0.0 };
	RightVector* pMaxGradient{ nullptr };
	while (pb < pe)
	{
		RightVector* pRv{ pRightVector + (pb - pNewtonGradient.get()) };
		if (pRv->Atol > 0.0)
		{
			const double WeightedNorm{ pRv->GetWeightedError(*pb, *pRv->pValue) };
			if (Norm < WeightedNorm)
			{
				Norm = WeightedNorm;
				pMaxGradient = pRv;
			}
		}

		//Norm += (*pb) * (*pb);
		pb++;
	}

	if (pMaxGradient)
	{
		Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("Max grad {} at {} \"{}\"",
			Norm,
			pMaxGradient->pDevice->GetVerbalName(),
			pMaxGradient->pDevice->VariableNameByPtr(pMaxGradient->pValue)));
	}

	return Norm;
}
*/

// отключает ведомое устройство если ведущего нет или оно отключено
bool CDynaModel::SetDeviceStateByMaster(CDevice *pDev, const CDevice *pMaster)
{
	if (pMaster)
	{
		// ведущее устройство есть и отключено
		if (!pMaster->IsStateOn())
		{
			// если проверяемое устройство не отключено
			if (pDev->IsStateOn())
			{
				// отключаем его и учитываем в количестве отключений
				pDev->SetState(eDEVICESTATE::DS_OFF, pMaster->GetStateCause());
				Log(DFW2MessageStatus::DFW2LOG_MESSAGE, fmt::format(CDFW2Messages::m_cszTurningOffDeviceByMasterDevice, pDev->GetVerbalName(), pMaster->GetVerbalName()));
				return true;
			}
			else
			{
				// проверяемое устройство отключено, но его статус может измениться на DSC_INTERNAL_PERMANENT
				if (pMaster->IsPermanentOff() && !pDev->IsPermanentOff())
				{
					// если ведущее отключено навсегда, а ведомое просто отключено, отключаем ведомое навсегда
					pDev->SetState(eDEVICESTATE::DS_OFF, pMaster->GetStateCause());
					return true;
				}
			}
		}
	}
	else
	{
		// ведущего нет, если ведомое устройство не отключено навсегда - отключаем его
		if (!pDev->IsPermanentOff())
		{
			pDev->SetState(eDEVICESTATE::DS_OFF, eDEVICESTATECAUSE::DSC_INTERNAL_PERMANENT);
			Log(DFW2MessageStatus::DFW2LOG_MESSAGE, fmt::format(CDFW2Messages::m_cszTurningOffDeviceDueToNoMasterDevice, pDev->GetVerbalName()));
			return true;
		}
	}
	return false;
}


// Отключает все устройства, у которых отключены или отсутствуют ведущие. Предназначено для однократного вызова при инициализации
// в процессе интегрирования нужно использовать CDevice::ChangeState
// Если в модели есть устройства, которые не получилось слинковать, их нужно отключать, причем отключать так, чтобы было понятно,
// что их дальнейшая работа невозможна. По идее можно было бы их просто удалить, но тогда потребовалась бы повторная линковка.
// Поэтому работает такой алгоритм
// 1. Если ведущее устройство отсутствует - отключаем ведомое навсегда
// 2. Если ведущее устройство есть и оно отключено навсегда - отключаем ведомое навсегда
// 3. Если ведущее устройство есть и оно просто отключено, отключаем ведомое
// 4. Повторяем рекурсивно по всем устройствам до тех пор, пока не будет зафиксировано отключений
// Отключенные навсегда не инициализируются и не обрабатываются на разрывах
// При записи результатов отключенные навсегда устройства также не учитываются
void CDynaModel::TurnOffDevicesByOffMasters()
{
	size_t nOffCount(1);
	while (nOffCount)
	{
		nOffCount = 0;
		for (auto&& it : m_DeviceContainers)
		{
			const CDeviceContainerProperties& Props{ it->ContainerProps() };
			if (Props.eDeviceType == eDFW2DEVICETYPE::DEVTYPE_BRANCH)
				continue;
					   
			for (auto&& dit : *it)
			{
				// если устройство уже отключено навсегда, обходим его
				if (dit->IsPermanentOff())
					continue;

				// идем по всем ссылкам на ведущие устройства
				for (auto&& masterdevice : Props.Masters)
				{
					// нужно проверить, есть ли вообще ведущие устройства у данного устройства,
					// если нет - нужное его перевести в DS_OFF DSC_INTERNAL_PERMANENT
					if (masterdevice->eLinkMode == DLM_MULTI)
					{
						// просматриваем мультиссылки на ведущие
						const CLinkPtrCount* const pLink{ dit->GetLink(masterdevice->nLinkIndex) };
						LinkWalker<CDevice> pDevice;
						while (pLink->In(pDevice))
						{
							if(SetDeviceStateByMaster(dit, pDevice))
								nOffCount++;
						}
						// здесь надо принять решение - отключать навсегда при условии что хотя бы одно ведущее отсутствует или должны отсутствовать все
					}
					else
					{
						// проверяем ссылку на ведущее
						if (SetDeviceStateByMaster(dit, dit->GetSingleLink(masterdevice->nLinkIndex)))
							nOffCount++;
					}
				}
			}
		}
	}
}


bool CDynaModel::RunLoadFlow() 
{
	// внутри линка делается обработка СХН
	if(!Link())
		throw dfw2error(CDFW2Messages::m_cszWrongSourceData);

	// Поэтому Init СХН с pNext/pPrev делаем после линка

	if (LRCs.Init(this) != eDEVICEFUNCTIONSTATUS::DFS_OK)
		throw dfw2error(CDFW2Messages::m_cszWrongSourceData);

	if (Reactors.Init(this) != eDEVICEFUNCTIONSTATUS::DFS_OK)
		throw dfw2error(CDFW2Messages::m_cszWrongSourceData);

	TurnOffDevicesByOffMasters();
	PrepareNetworkElements();
	return LoadFlow();
}
