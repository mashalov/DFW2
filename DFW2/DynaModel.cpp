/*
* Raiden - программа для моделирования длительных электромеханических переходных процессов в энергосистеме
* (С) 2018 - 2021 Евгений Машалов
* Raiden - long-term power system transient stability simulation module
* (C) 2018 - 2021 Eugene Mashalov
*/

#include "stdafx.h"
#include "DynaModel.h"
#include "DynaGeneratorMustang.h"
#include "DynaGenerator2C.h"
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
#include "TaggedPath.h"
#include "FolderClean.h"
#include "Rodas.h"
#include "MixedAdamsBDF.h"
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
						   Generators2C(this),
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
	CDynaGenerator2C::DeviceProperties(Generators2C.ContainerProps());
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

	DeviceContainers_.push_back(&Nodes);
	DeviceContainers_.push_back(&LRCs);
	DeviceContainers_.push_back(&Reactors);
	DeviceContainers_.push_back(&ExcitersMustang);
	DeviceContainers_.push_back(&DECsMustang);
	DeviceContainers_.push_back(&ExcConMustang);
	DeviceContainers_.push_back(&Branches);
	DeviceContainers_.push_back(&Generators2C);
	DeviceContainers_.push_back(&Generators3C);
	DeviceContainers_.push_back(&GeneratorsMustang);
	DeviceContainers_.push_back(&GeneratorsPark3C);
	DeviceContainers_.push_back(&GeneratorsPark4C);
	DeviceContainers_.push_back(&Generators1C);
	DeviceContainers_.push_back(&GeneratorsMotion);
	DeviceContainers_.push_back(&GeneratorsInfBus);
	DeviceContainers_.push_back(&GeneratorsPowerInjector);
	//m_DeviceContainers.push_back(&CustomDevice);
	//m_DeviceContainers.push_back(&CustomDeviceCPP);
	DeviceContainers_.push_back(&BranchMeasures);
	DeviceContainers_.push_back(&NodeMeasures);
	DeviceContainers_.push_back(&AutomaticDevice);
	DeviceContainers_.push_back(&ScenarioDevice);
	DeviceContainers_.push_back(&ZeroLoadFlow);
	DeviceContainers_.push_back(&TestDevices);
	DeviceContainers_.push_back(&SynchroZones);		// синхрозоны должны идти последними

	CheckFolderStructure();

	if (m_Parameters.m_eFileLogLevel != DFW2MessageStatus::DFW2LOG_NONE)
	{
		const auto LogPath{ Platform().Logs() };
		TaggedPath MainLogPath(std::filesystem::path(LogPath).append("dfw2.log"));
		TaggedPath DebugLogPath(std::filesystem::path(LogPath).append("debug.log"));
		LogFile = MainLogPath.Create();
		DebugLogFile = DebugLogPath.Create();

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
			throw dfw2errorGLE(fmt::format(CDFW2Messages::m_cszStdFileStreamError, stringutils::utf8_encode(MainLogPath.Path().c_str())));

		CFolderClean FolderClean(LogPath, m_Parameters.MaxLogFilesCount_, m_Parameters.MaxLogFilesSize_);
		FolderClean.Clean();
	}

	if (!SSE2Available_)
		throw dfw2error(CDFW2Messages::m_cszNoSSE2Support);

	Integrator_ = std::make_unique<Rodas4>(*this);
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
	DeserializeParameters(Platform().Root() / "config.json");
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
		InitEquations();

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

#if defined(_MSC_VER) && !defined(_WINDLL)
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

	for (auto&& container : DeviceContainers_)
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

		for (auto&& it : DeviceContainers_)
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
		if (nOKInits == DeviceContainers_.size())
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
			for (auto&& it : DeviceContainers_)
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


void CDynaModel::InitEquations()
{
	InitNordsiek();
	// Второй раз вызываем обновление внешних переменных чтобы получить реальные индексы элементов из матрицы
	if(!UpdateExternalVariables())
		throw dfw2error(CDFW2Messages::m_cszWrongSourceData);

	const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };

	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
	{
		pVectorBegin->Nordsiek[0] = pVectorBegin->SavedNordsiek[0] = *pVectorBegin->pValue;
		PrepareNordsiekElement(pVectorBegin);
	}

	double dCurrentH{ H() };
	sc.SetNordsiekScaledForH(1.0);
	SetH(0.0);
	sc.m_bDiscontinuityMode = true;
	SolveNewton(100);
	sc.m_bDiscontinuityMode = false;
	SetH(dCurrentH);
	sc.nStepsCount = 0;

	for (auto&& cit : DeviceContainersStoreStates_)
		for (auto&& dit : *cit)
			dit->StoreStates();
}

void CDynaModel::NewtonUpdate()
{
	sc.m_bNewtonConverged = false;
	sc.m_bNewtonDisconverging = false;
	sc.m_bNewtonStepControl = false;
	ConvergenceTest::ProcessRange(Integrator_->ConvTest(), ConvergenceTest::Reset);
	Integrator_->NewtonUpdateIteration();
	ConvergenceTest::ProcessRange(Integrator_->ConvTest(), ConvergenceTest::NextIteration);
}

void CDynaModel::NewtonUpdateDevices()
{
	for (auto&& it : DeviceContainersNewtonUpdate_)
		it->NewtonUpdateBlock(this);
}

void CDynaModel::SolveNewton(ptrdiff_t nMaxIts)
{
	sc.nNewtonIteration = 1;

	if (sc.m_bProcessTopology)
	{
		Nodes.LULF();
		ProcessDiscontinuity();
		sc.m_bProcessTopology = false;
		_ASSERTE(sc.m_bDiscontinuityMode);
		sc.m_bDiscontinuityMode = true;
	}
		
	for (; sc.nNewtonIteration < nMaxIts; sc.nNewtonIteration++)
	{
		if (IsInDiscontinuityMode())
			sc.RefactorMatrix();

		BuildMatrix();

		sc.nNewtonIterationsCount++;

		ptrdiff_t imax(0);

		double bmax{ klu.FindMaxB(imax) };
		double* bwatch{ klu.B() };

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

		NewtonUpdate();
		// если Ньютон сошелся, выдаем репорт 
		// обрабатываем разрыв если есть и выходим
		if (sc.m_bNewtonConverged)
		{
			NewtonUpdateDevices();
			LogTime(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("Converged{:>3} iteration {} MaxWeight {} Saving {:.2} Rcond {}", 
																			sc.nNewtonIteration,
																			sc.Newton.Absolute.Info(),
																			sc.Newton.Weighted.dMaxError,
																			1.0 - static_cast<double>(klu.FactorizationsCount() + klu.RefactorizationsCount()) / sc.nNewtonIterationsCount,
																			sc.dLastConditionNumber));
			// ProcessDiscontinuity checks Discontinuity mode internally and can be called
			// regardless of this mode
			ProcessDiscontinuity();
			break;
		}
		else if (sc.m_bNewtonDisconverging)
		{
			// Ньютон расходится

			sc.RefactorMatrix();
			double lambdamin{ 0.1 };
			if (IsInDiscontinuityMode())
			{
				// для обработки разрыва шаг линейного поиска делаем больше чем минимальный шаг
				// (хотя может стоит его сделать таким же, как и для минимального шага,
				// но будет сходиться очень медленно)
				lambdamin = 1E-4;
				sc.m_bNewtonStepControl = true;
			}
			else if (sc.Hmin / H() > 0.95)
			{
				// если шаг снижается до минимального 
				// переходим на Ньютон по параметру
				lambdamin = sc.Hmin;
				sc.m_bNewtonStepControl = true;
			}
			else
			{
				// если шаг относительно большой
				// прерываем итерации Ньютона: проще снизить шаг чем пытаться пройти Ньютоном
				// при этом не вызываем NewtonUpdateDevices
				if (sc.nNewtonIteration > 5)
					break;
				else if (sc.nNewtonIteration > 2 && sc.Newton.Absolute.dMaxError > 1.0)
					break;
			}

			if (sc.m_bNewtonStepControl)
			{
				// Бэктрэк Ньютона
				std::unique_ptr<double[]> pRh = std::make_unique<double[]>(klu.MatrixSize());			 // невязки до итерации	
				std::unique_ptr<double[]> pRb = std::make_unique<double[]>(klu.MatrixSize());			 // невязки после итерации
				std::copy(pRightHandBackup.get(), pRightHandBackup.get() + klu.MatrixSize(), pRh.get()); // копируем невязки до итерации
				std::copy(klu.B(), klu.B() + klu.MatrixSize(), pRb.get());								 // копируем невязки после итерации
				const double g0{ sc.dRightHandNorm };													 // норма небаланса до итерации
				NewtonUpdateDevices();																     // обновляем комплексные VreVim в узлах
				BuildRightHand();																		 // рассчитываем невязки после итерации
				const double g1{ sc.dRightHandNorm };													// норма небаланса после итерации

				if (g0 < g1)
				{
					// если небаланс увеличился
					const double gs1v{ gs1(klu, pRh, pRb.get()) };

					// считаем множитель
					double lambda{ -0.5 * gs1v / (g1 - g0 - gs1v) };

					if (lambda > lambdamin && lambda < 1.0)
						Integrator_->NewtonBacktrack(pRb.get(), lambda);
				}
			}
		}
		NewtonUpdateDevices();
		LogTime(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("Continue {:>3} iteration {} MaxWeight {} Rcond {}", 
				sc.nNewtonIteration,
				sc.Newton.Absolute.Info(),
				sc.Newton.Weighted.dMaxError,
				sc.dLastConditionNumber));
	}

	if (sc.nNewtonIteration == nMaxIts && IsInDiscontinuityMode())
		throw dfw2error(CDFW2Messages::m_cszNewtonSolverDoesNotConvergedInIterations, nMaxIts);

	if (!sc.m_bNewtonConverged)
	{
		if (sc.Newton.Absolute.pVector)
			sc.Newton.Absolute.pVector->ErrorHits++;
	}
}

bool CDynaModel::ApplyChangesToModel()
{
	bool bRes{ true };
	// проверяем, не возникла ли при обработке разрыва необходимость обработки топологии
	if (sc.m_bProcessTopology)
		Nodes.ProcessTopology();

	// если возникла необходимость перестроения Якоби
	if (RebuildMatrixFlag_)
	{
		// строим ее заново
		// создаем новый вариант систем уравнений 
		// потокораспределения суперузлов
		CreateZeroLoadFlow();
		EstimateMatrix();
		bRes = UpdateExternalVariables();
	}
	// проверяем, не возникло ли новых запросов на обработку разрыва при обработке разрыва
	if (sc.DiscontinuityLevel_ == DiscontinuityLevel::None)
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
				LogTime(DFW2MessageStatus::DFW2LOG_DEBUG, 
					fmt::format(CDFW2Messages::m_cszStepAdjustedToDiscontinuity, 
						H()));
				sc.m_bBeforeDiscontinuityWritten = false;		// готовимся к обработке разрыва
			}
			else
			{
				// если время найденного события "точно" попадает во время текущего шага
				if (std::abs(GetCurrentTime() - dTimeNextEvent) < Hmin())
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

	if (sc.DiscontinuityLevel_ != DiscontinuityLevel::None)
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
		Integrator_->Step();

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

		// если корректор отработал без сбоев
		if (sc.m_bNewtonConverged)
		{
			// если Ньютон сошелся
			FinishStep();	// рассчитываем независимые переменные, которые не входят в систему уравнения

			if (!sc.m_bDiscontinuityMode)
			{
				// и мы не находились в режиме обработки разрыва
				bool StepConverged{ false };
					
				// рассчитываем возможный шаг для текущего порядка метода
				// если сделанный шаг не был сделан без предиктора
				if (StepFromReset())
				{
					// если нордсик был сброшен не конторолируем предиктор
					LogTime(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("startup step"));
				}
				else
					StepConverged = Integrator_->StepConverged();

				// шаг выполнен успешно если это был стартап-шаг
				// после сброса предиктора или корректор и предиктор
				// дали возможность не уменьшать шаг
				if (StepConverged || StepFromReset())
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

							if (sc.DiscontinuityLevel_ == DiscontinuityLevel::None)
							{
								// если не возникло запросов на обработку разрыва
								// и признаем шаг успешным, блокируя управление шагом
								Integrator_->AcceptStep(true);
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

							if (sc.DiscontinuityLevel_ == DiscontinuityLevel::None)
							{
								// если не было запросов обработки разрыва признаем шаг успешным
								Integrator_->AcceptStep(StepFromReset());
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
					Integrator_->RejectStep();
				}
			}
			else
			{
				// если находились в режиме обработки разрыва, выходим из него (Ньютон сошелся, все ОК)
				LeaveDiscontinuityMode();
				// удаляем события, который уже отработали по времени
				m_Discontinuities.PassTime(GetCurrentTime() + 0.9 * Hmin());
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
		sc.nStepsCount++;
	}
	
	//_ASSERTE(_CrtCheckMemory());
	return bRes;

}

void CDynaModel::EnterDiscontinuityMode()
{
	if (!sc.m_bDiscontinuityMode)
	{
		sc.m_bDiscontinuityMode = true;
		sc.m_bZeroCrossingMode = false;
		ChangeOrder(1);
		StoreUsedH();
		SetH(0.0);
	}
}

// для устройств во всех контейнерах сбрасывает статус готовности функции
void CDynaModel::UnprocessDiscontinuity()
{
	for (auto&& it : DeviceContainers_)
		it->UnprocessDiscontinuity();
}

void CDynaModel::ProcessDiscontinuity()
{
	eDEVICEFUNCTIONSTATUS Status{ eDEVICEFUNCTIONSTATUS::DFS_NOTREADY };
		
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
			sc.DiscontinuityLevel_ = DiscontinuityLevel::None;
			sc.pDiscontinuityDevice_ = nullptr;

			ChangeOrder(1);
			ptrdiff_t nTotalOKPds = -1;

			Status = eDEVICEFUNCTIONSTATUS::DFS_NOTREADY;

			while (Status == eDEVICEFUNCTIONSTATUS::DFS_NOTREADY)
			{
				// пока статус "неготово"
				ptrdiff_t nOKPds = 0;
				// обрабатываем разрывы для всех устройств во всех контейнерах
				for (auto&& it : DeviceContainers_)
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
				if (nOKPds == DeviceContainers_.size())
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
			if (sc.DiscontinuityLevel_ == DiscontinuityLevel::None)
				break;
		}
		// инициализируем Нордсик
		sc.DiscontinuitiesProcessed_++;
		Integrator_->Restart();
	}
	else
		Status = eDEVICEFUNCTIONSTATUS::DFS_OK;

	if (!CDevice::IsFunctionStatusOK(Status))
		throw dfw2error(CDFW2Messages::m_cszDiscontinuityProcessingFailed);
}

void CDynaModel::LeaveDiscontinuityMode()
{
	if (sc.m_bDiscontinuityMode)
	{
		sc.m_bDiscontinuityMode = false;
		for (auto&& it : DeviceContainers_)
			it->LeaveDiscontinuityMode(this);
		SetRestartH();
		Integrator_->Restart();
	}
}

double CDynaModel::CheckZeroCrossing()
{
	double Kh{ 1.0 };
	for (auto&& it : DeviceContainers_)
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
		if (sc.UsedH() / H() >= 0.8)
		{
			SetH(H() * 0.87);
			sc.SetRateGrowLimit(1.0);
		}
		else
		{
			SetH(0.8 * UsedH() + 0.2 * H());
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
		Integrator_->Restart();
	else
		ReInitializeNordsiek();
	
	sc.RefactorMatrix();
	LogTime(DFW2MessageStatus::DFW2LOG_DEBUG, 
		fmt::format(CDFW2Messages::m_cszStepAndOrderChangedOnNewton,
			sc.q, 
			H()));
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
	sc.CheckAdvance_t0();
	LogTime(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(CDFW2Messages::m_cszZeroCrossingStep,
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
		for (auto&& it : DeviceContainers_)
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
