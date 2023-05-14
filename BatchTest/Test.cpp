#include <iostream>
#include <thread>
#include "Test.h"
#include "BS_thread_pool.hpp"
#include "ResultCompare.h"
#import "progid:Astra.Rastr.1" named_guids no_namespace
#include "nlohmann/json.hpp"

#include <atlbase.h>
#include <atlcom.h>

CComModule _Module;

_bstr_t bstrPath(const std::filesystem::path& Path)
{
	return _bstr_t(Path.c_str());
}

static const GUID CLSID_Handler = { 0x5219b44b, 0x874, 0x449e,{ 0x86, 0x11, 0xb7, 0x8, 0xd, 0xbf, 0xa6, 0xab } };


class ATL_NO_VTABLE CEventHandler :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CEventHandler, &CLSID_Handler>,
	public IDispatchImpl<_IRastrEvents, &DIID__IRastrEvents, &LIBID_ASTRALib>
{
public:
	BEGIN_COM_MAP(CEventHandler)
		COM_INTERFACE_ENTRY(_IRastrEvents)
		COM_INTERFACE_ENTRY(IDispatch)
	END_COM_MAP()
	//Custom functions

	STDMETHOD(Invoke)(
		_In_ DISPID dispidMember,
		_In_ REFIID riid,
		_In_ LCID lcid,
		_In_ WORD wFlags,
		_In_ DISPPARAMS* pdispparams,
		_Out_opt_ VARIANT* pvarResult,
		_Out_opt_ EXCEPINFO* pexcepinfo,
		_Out_opt_ UINT* puArgErr)
	{

		if (dispidMember == 3)
		{
			if (pdispparams->rgvarg[4].lVal == 24)
			{
				std::cout << stringutils::utf8_encode(pdispparams->rgvarg[2].bstrVal) << "\r";
			}
		}
		return S_OK;
	}

};

class  __declspec(uuid("5219b44b-0874-449e-8611-b7080dbfa6ab")) CEventHandler;

void CBatchTest::ReadParameters()
{
	if (!std::filesystem::exists(parametersPath_))
		throw dfw2error("Файл параметров {} не найден", parametersPath_.string());
	std::ifstream parametersFile(parametersPath_);
	using json = nlohmann::json;
	try
	{
		json parameters{ json::parse(parametersFile) };

		std::cout << "Используется файл параметров " << parametersPath_.u8string() << std::endl;

		const std::filesystem::path CaseFilesFolder{ parameters.at("CaseFilesFolder").get<std::string>()};
		const std::filesystem::path ScenarioFilesFolder{ parameters.at("ScenarioFilesFolder").get<std::string>()};
		GlobalOptions.Duration = parameters.at("Duration").get<double>();
		GlobalOptions.EmsMode = parameters.at("EMS").get<bool>();
		GlobalOptions.RUSTabHmin = parameters.at("RUSTabHmin").get<double>();
		GlobalOptions.RaidenAtol = parameters.at("RaidenAtol").get<double>();
		GlobalOptions.ResultPath = parameters.at("ResultPath").get<std::string>();
		GlobalOptions.SelectedRun = parameters.at("SelectedRun").get<long>();
		GlobalOptions.RaidenStopOnOOS = parameters.at("RaidenStopOnOOS").get<bool>();
		GlobalOptions.RUSTabAtol = parameters.at("RUSTabAtol").get<double>();
		GlobalOptions.RunRaiden = parameters.at("RunRaiden").get<bool>();
		GlobalOptions.RunRUSTab = parameters.at("RunRUSTab").get<bool>();
		
		constexpr const char* szRaidenRtol{ "RaidenRtol" };
		if (parameters.contains(szRaidenRtol))
			GlobalOptions.RaidenRtol = parameters[szRaidenRtol].get<double>();

		constexpr const char* szThreads{ "Threads" };
		if (parameters.contains(szThreads))
			GlobalOptions.Threads = parameters[szThreads].get<long>();

		constexpr const char* szRaidenZeroBranch{ "RaidenZeroBranch" };
		if (parameters.contains(szRaidenZeroBranch))
			GlobalOptions.RUSTabZeroBranch = parameters[szRaidenZeroBranch].get<double>();

		std::cout << "Поиск файлов моделей в каталоге " << CaseFilesFolder.string() << std::endl;

		for (const auto& entry : std::filesystem::directory_iterator(CaseFilesFolder))
		{
			const auto& path{ entry.path() };
			if (!path.has_extension() && !std::filesystem::is_directory(path))
			{
				AddCase(path);
				std::cout << "Добавлен файл модели " << path.u8string() << std::endl;
			}
		}

		std::cout << "Поиск файлов сценариев в каталоге " << ScenarioFilesFolder.string() << std::endl;

		for (const auto& entry : std::filesystem::directory_iterator(ScenarioFilesFolder))
		{
			const auto& path{ entry.path() };
			if (path.extension() == ".scn")
			{
				AddContingency(path);
				std::cout << "Добавлен файл сценария " << path.u8string() << std::endl;
			}
		}

		std::cout << "Максимальное количество расчетов " << ContingencyFiles_.size() * CaseFiles_.size() << std::endl;
		std::cout << "Длительность расчета: " << GlobalOptions.Duration << std::endl;
		std::cout << "Режим: " << (GlobalOptions.EmsMode ? "EMS" : "Инженерный") << std::endl;
		std::cout << "Raiden Atol: " << GlobalOptions.RaidenAtol << std::endl;
		std::cout << "Raiden Rtol: " << GlobalOptions.RaidenRtol << std::endl;
		std::cout << "RUSTab.Atol: " << GlobalOptions.RUSTabAtol << std::endl;
		std::cout << "RUSTab Hmin: " << GlobalOptions.RUSTabHmin << std::endl;
		std::cout << "RUSTab Zero branch: " << GlobalOptions.RUSTabZeroBranch << std::endl;
		std::cout << "Raiden останов по АР: " << (GlobalOptions.RaidenStopOnOOS ? "Да": "Нет") << std::endl;
		if(GlobalOptions.SelectedRun > 0)
			std::cout << "Выбран расчет №: " << GlobalOptions.SelectedRun << std::endl;
	}
	catch (const json::exception& jex)
	{
		throw dfw2error("json {}", jex.what());
	}
}

void CBatchTest::AddCase(std::filesystem::path CaseFile)
{
	CaseFiles_.emplace_back(CaseFile);
}
void CBatchTest::AddContingency(std::filesystem::path ContingencyFile)
{
	ContingencyFiles_.emplace_back(ContingencyFile);
}

void CBatchTest::Run()
{
	if (!parametersPath_.empty())
		ReadParameters();

	const auto fnFileName{ [this](const std::string filenamebase)
	{
		return fmt::format("{} Mode {} Atol {} Thds {}.csv", 
			filenamebase,
			GlobalOptions.EmsMode ? "ems" : "eng",
			GlobalOptions.RaidenAtol,
			GlobalOptions.Threads
		);
	} };

	briefreport.open((fnFileName)("briefreport"));
	briefreport.imbue(std::locale("fr_FR"));  // Set OUTPUT to use a COMMA
	constexpr const unsigned char bom[] = { 0xEF,0xBB,0xBF };
	briefreport.write((char*)bom, sizeof(bom));
	briefreport << fmt::format("Id;Модель;Возмущение;RUSTab;Raiden;T RUSTab;T Raiden;Макс|откл|;Макс|%|;tоткл;Переменная\n");

	fullreport.open((fnFileName)("fullreport"));
	fullreport.imbue(std::locale("fr_FR"));  // Set OUTPUT to use a COMMA
	fullreport.write((char*)bom, sizeof(bom));

	constexpr const char* NoRastrWinFoundInRegistry{ "Не найдено данных RastrWin3 в реестре" };
	HKEY handle{ NULL };
	try
	{
		std::filesystem::path templatePath, installPath;
		const auto cszUserFolder = L"UserFolder";
		const auto cszInstallPath = L"InstallPath";


		HKEY handle{ NULL };
		if (RegOpenKey(HKEY_CURRENT_USER, L"Software\\RastrWin3", &handle) != ERROR_SUCCESS)
			throw dfw2error(NoRastrWinFoundInRegistry);
		DWORD size(0);
		if (RegQueryValueEx(handle, cszUserFolder, NULL, NULL, NULL, &size) != ERROR_SUCCESS)
			throw dfw2error(NoRastrWinFoundInRegistry);
		auto buffer = std::make_unique<wchar_t[]>(size * sizeof(wchar_t) / sizeof(BYTE));
		if (RegQueryValueEx(handle, cszUserFolder, NULL, NULL, reinterpret_cast<BYTE*>(buffer.get()), &size) == ERROR_SUCCESS)
			templatePath = buffer.get();
		else
			throw dfw2error(NoRastrWinFoundInRegistry);
		size = 0;
		if (RegQueryValueEx(handle, cszInstallPath, NULL, NULL, NULL, &size) != ERROR_SUCCESS)
			throw dfw2error(NoRastrWinFoundInRegistry);
		buffer = std::make_unique<wchar_t[]>(size * sizeof(wchar_t) / sizeof(BYTE));
		if (RegQueryValueEx(handle, cszInstallPath, NULL, NULL, reinterpret_cast<BYTE*>(buffer.get()), &size) == ERROR_SUCCESS)
			installPath = buffer.get();
		else
			throw dfw2error(NoRastrWinFoundInRegistry);

		RegCloseKey(handle);
		handle = NULL;

		std::filesystem::path macroPath{ templatePath / L"macro" };
		templatePath.append(L"shablon");
		std::filesystem::path rstPath{ templatePath }, dfwPath{ templatePath }, scnPath{ templatePath };
		rstPath.append(L"динамика.rst");
		dfwPath.append(L"автоматика.dfw");
		scnPath.append(L"сценарий.scn");

		long CaseId{ 0 };

		const auto MaxThreads{ std::thread::hardware_concurrency() };

		struct InputOutput
		{
			Input Inp;
			Output Out;
		};

		std::list<InputOutput> Runs;

		for (const auto& contfile : ContingencyFiles_)
			for (const auto& casefile : CaseFiles_)
			{
				// если SelectedRun больше нуля - считаем только комбинацию с номером
				// равным SelectedRun
				++CaseId;
				if ((GlobalOptions.SelectedRun > 0 && GlobalOptions.SelectedRun == CaseId) ||
					GlobalOptions.SelectedRun <= 0)
				{
					Runs.emplace_back(InputOutput{ {
						   GlobalOptions,
						   casefile,
						   contfile,
						   rstPath,
						   scnPath,
						   dfwPath,
						   macroPath,
						   installPath,
						   std::filesystem::current_path(),
						   CaseId,
						   ContingencyFiles_.size() * CaseFiles_.size()
						},
						{} });
				}
			}
				
		std::cout << "Будет выполнено расчетов: " << Runs.size() << std::endl;

		std::vector<std::thread> threads;

		CTestTimer Timer;

		if (GlobalOptions.Threads == 0)
			GlobalOptions.Threads = std::thread::hardware_concurrency() / 2;

		BS::thread_pool pool(GlobalOptions.Threads);

		double TimeRaiden{ 0.0 }, TimeRustab{ 0.0 };

		if (GlobalOptions.Threads > 1)
		{
			for (auto&& run : Runs)
				pool.push_task(TestPair, std::ref(run.Inp), std::ref(run.Out));

			pool.wait_for_tasks();

			for (auto&& run : Runs)
			{
				TimeRaiden += run.Out.TimeRaiden;
				TimeRustab += run.Out.TimeRustab;
				fullreport << run.Out.Report.str();
				std::cout << run.Out.Report.str();
				briefreport << run.Out.BriefReport.str();
			}
		}
		else
			for (auto&& run : Runs)
			{
				TestPair(run.Inp, run.Out);
				TimeRaiden += run.Out.TimeRaiden;
				TimeRustab += run.Out.TimeRustab;
				fullreport << run.Out.Report.str();
				std::cout << run.Out.Report.str();
				briefreport << run.Out.BriefReport.str();
			}

		const std::string timeused{ fmt::format("Общая дительность расчетов {:.3f} RUSTab {:.3f}, Raiden {:.3f}\n",
			Timer.Duration(),
			TimeRustab,
			TimeRaiden
			) };
		fullreport << timeused;
		std::cout << timeused;
	}
	catch (const std::runtime_error& er)
	{
		if (handle)
			RegCloseKey(handle);

		auto s{ stringutils::utf8_decode(er.what()) };

		std::cout << er.what() << std::endl;
		fullreport << er.what() << std::endl;
	}
	fullreport.close();
	briefreport.close();
}

void CBatchTest::TestPair(const Input& Input, Output& Output)
{
	const auto& Opts{ Input.Opts };
	Output.BriefReport.imbue(std::locale("fr_FR"));  // Set OUTPUT to use a COMMA
	try
	{
		std::filesystem::current_path(Input.astraPath);
		if(Input.Opts.Threads > 1)
		{
			std::lock_guard<std::mutex> lock(Input.mtx_);
			std::cout << 
				fmt::format("Запущен расчет {}: {} + {}",
				Input.CaseId,
				Input.CaseFile.filename().u8string(),
				Input.ContingencyFile.filename().u8string()
				) 
				<< std::endl;
		}

		if (const HRESULT hr{ CoInitialize(NULL) }; FAILED(hr))
			throw dfw2error("Ошибка CoInitialize, scode {:0x}", static_cast<unsigned long>(hr));

		

		IRastrPtr Rastr;
		if (const HRESULT hr{ Rastr.CreateInstance(CLSID_Rastr) }; FAILED(hr))
			throw dfw2error("Ошибка создания Rastr, scode {:0x}", static_cast<unsigned long>(hr));

		_com_ptr_t<_com_IIID<IConnectionPoint, &__uuidof(IConnectionPoint)>> ConnectionPoint;
		DWORD dwCookie{ 0 };

		CComObject<CEventHandler>* handler{ nullptr };
		DWORD dwCookieReg{ 0 };

		if (Input.Opts.Threads == 1)
		{
			CEventHandler::CreateInstance(&handler);
			CComObject<CEventHandler>::CreateInstance(&handler);
			_com_ptr_t<_com_IIID<IConnectionPointContainer, &__uuidof(IConnectionPointContainer)>> ConnectionPoints;
			if (SUCCEEDED(Rastr->QueryInterface(&ConnectionPoints)))
				if (SUCCEEDED(ConnectionPoints->FindConnectionPoint(__uuidof(_IRastrEvents), &ConnectionPoint)))
					ConnectionPoint->Advise(handler, &dwCookie);
		}

		Rastr->Load(RG_REPL, bstrPath(Input.CaseFile), L"" /*bstrPath(Input.rstPath)*/);
		//Rastr->Load(RG_REPL, bstrPath(Input.ContingencyFile), bstrPath(Input.scnPath));
		auto FWDynamic{ Rastr->FWDynamic() };
		
		ITablePtr ComDynamic{ Rastr->Tables->Item("com_dynamics") };
		IColPtr DurationSet{ ComDynamic->Cols->Item("Tras")};
		IColPtr Hint{ ComDynamic->Cols->Item("Hint") };
		IColPtr Hmin{ ComDynamic->Cols->Item("Hmin") };
		IColPtr Hmax{ ComDynamic->Cols->Item("Hmax") };
		IColPtr Hout{ ComDynamic->Cols->Item("Hout") };
		IColPtr MInt{ ComDynamic->Cols->Item("Mint") };
		IColPtr LRCTol{ ComDynamic->Cols->Item("SXNTolerance") };
		IColPtr SalientTol{ ComDynamic->Cols->Item("dEf") };

		IColPtr SnapTemplate{ComDynamic->Cols->Item("SnapTemplate")};
		IColPtr SnapPath{ ComDynamic->Cols->Item("SnapPath") };
		IColPtr SnapAutoLoad{ ComDynamic->Cols->Item("SnapAutoLoad") };
		IColPtr RUSTabAtol{ ComDynamic->Cols->Item("IntEpsilon") };


		//MInt->PutZ(0, 6);
		Hint->PutZ(0, Opts.RUSTabHmin);
		Hmin->PutZ(0, Opts.RUSTabHmin);
		Hout->PutZ(0, Opts.RUSTabHmin);
		RUSTabAtol->PutZ(0, Opts.RUSTabAtol);
		LRCTol->PutZ(0, Opts.RUSTabAtol);
		SalientTol->PutZ(0, Opts.RUSTabAtol * 100.0);
		Hmax->PutZ(0, 5);
		SnapAutoLoad->PutZ(0, 0);

		DurationSet->PutZ(0, Opts.Duration);

		ITablePtr RaidenParameters{ Rastr->Tables->Item("RaidenParameters") };
		IColPtr GoRaiden{ RaidenParameters->Cols->Item("GoRaiden")};
		IColPtr Atol{ RaidenParameters->Cols->Item("Atol") };
		IColPtr Rtol{ RaidenParameters->Cols->Item("Rtol") };
		IColPtr ZeroBranchImpedance{ RaidenParameters->Cols->Item("ZeroBranchImpedance") };
		IColPtr ResultsFolder{ RaidenParameters->Cols->Item("ResultsFolder") };
		IColPtr OutStep{ RaidenParameters->Cols->Item("OutStep") };
		IColPtr DurationRaiden{ RaidenParameters->Cols->Item("Duration") };

		Rtol->PutZ(0, Opts.RaidenRtol);
		Atol->PutZ(0, Opts.RaidenAtol);
		OutStep->PutZ(0, Opts.RUSTabHmin);

		ZeroBranchImpedance->PutZ(0, Opts.RUSTabZeroBranch);
		DurationRaiden->PutZ(0, Opts.Duration);

		auto fnVerbalCode = [](const RastrRetCode& code) -> std::string
		{
			return code == AST_OK ? "OK" : "Отказ !";
		};

		auto fnVerbalSyncLossCause = [](const DFWSyncLossCause cause) -> std::string
		{
			switch (cause)
			{
			case SYNC_LOSS_NONE: return "Устойчивый";
			case SYNC_LOSS_BRANCHANGLE: return "Угол по линиии";
			case SYNC_LOSS_COA: return "COA";
			case SYNC_LOSS_OVERSPEED: return "Автомат скорости";
			case SYNC_LOSS_METHODFAILED: return "Отказ метода";
			default: return "Неизвестно";
			}
		};

		auto fnMessage = [](const _bstr_t& message)
		{
			return message.length() > 0 ? stringutils::utf8_encode(message) : "Нет";
		};

		IColPtr StopOnBranchOOS{ RaidenParameters->Cols->Item("StopOnBranchOOS") };
		IColPtr StopOnGeneratorOOS{ RaidenParameters->Cols->Item("StopOnGeneratorOOS") };
		IColPtr DisableResultWriter{ RaidenParameters->Cols->Item("DisableResultsWriter") };

		constexpr const char* cszThreadId{ "ThreadId" };
		if (RaidenParameters->Cols->GetFind(cszThreadId) < 0)
			RaidenParameters->Cols->Add(cszThreadId, PropTT::PR_INT);

		IColPtr ThreadId{ RaidenParameters->Cols->Item(cszThreadId) };

		ThreadId->PutZ(0, Input.CaseId);


		StopOnBranchOOS->PutZ(0, Opts.RaidenStopOnOOS ? 1 : 0);
		StopOnGeneratorOOS->PutZ(0, StopOnBranchOOS->GetZ(0));

		if (Opts.EmsMode)
		{
			StopOnBranchOOS->PutZ(0, 1);
			StopOnGeneratorOOS->PutZ(0, 1);
			DisableResultWriter->PutZ(0, 1);
		}
		else
			DisableResultWriter->PutZ(0, 0);


		const std::string ResultFileName{ fmt::format("{:05d}.sna", Input.CaseId) };
		std::filesystem::path ResultPath1(Opts.ResultPath), ResultPath2(Opts.ResultPath);
		ResultPath1.append("RUSTab");
		SnapPath->PutZ(0, bstrPath(ResultPath1));
		SnapTemplate->PutZ(0, stringutils::COM_encode(ResultFileName).c_str());
		ResultPath1.append(ResultFileName);
		ResultPath2.append("Raiden");
		ResultPath2.append(ResultFileName);

		ResultsFolder->PutZ(0, bstrPath(ResultPath2));

		Output.Report << fmt::format("Расчет {} из {}\nМодель: {}\nСценарий: {}\n",
			Input.CaseId,
			Input.TotalRuns,
			Input.CaseFile.filename().u8string(),
			Input.ContingencyFile.filename().u8string()
		);

		if (Input.Opts.Threads <= 1)
		{
			std::lock_guard<std::mutex> lock(Input.mtx_);
			std::cout << Output.Report.str() << std::endl;
			std::stringstream().swap(Output.Report);
		}

		double Duration[2];
		DFWSyncLossCause SyncLossCause[2];
		RastrRetCode RetCode[2];

		//Rastr->ExecMacroPath(bstrPath(Input.macroPath / "ModelCorrect.rbs"), L"");
		Rastr->ExecMacroPath(bstrPath(Input.currentPath / "ModelCorrect.rbs"), L"");

		
		for (int method{ 0 }; method < 2; method++)
		{
			if (method == 0 && !Opts.RunRUSTab) continue;
			if (method == 1 && !Opts.RunRaiden) continue;

			GoRaiden->PutZ(0, method);
			CTestTimer Timer;
			RetCode[method] = Opts.EmsMode ? FWDynamic->RunEMSmode() : FWDynamic->Run();
			Duration[method] = Timer.Duration();
			SyncLossCause[method] = FWDynamic->SyncLossCause;
			
			Output.Report << fmt::format("*\t{} {} режим "
									 "\n\tРезультат: {}"
									 "\n\tПросчитано: {:.3f}с"
									 "\n\tУстойчивость: {}"
									 "\n\tСообщение: {}"
									 "\n\tВремя расчета: {:.3f}с",

					method ? "Raiden" : "RUSTab",
					Opts.EmsMode ? "EMS" : "Инженерный",
					(fnVerbalCode)(RetCode[method]),
					FWDynamic->TimeReached,
					(fnVerbalSyncLossCause)(FWDynamic->SyncLossCause),
					(fnMessage)(FWDynamic->ResultMessage),
					Duration[method]) << std::endl;

			if (!Opts.EmsMode)
				Output.Report << fmt::format("\tФайл результатов : {}",
					method ? ResultPath2.u8string() : ResultPath1.u8string()) << std::endl;
		}

		double MaxDiff{ 0.0 };
		double MaxDiffRel{ 0.0 };
		double MaxDiffTime{ 0.0 };
		std::string MaxDiffVariable;

		if (!Opts.EmsMode)
		{
			ResultCompare compare;
			const auto CompareResults{ compare.Compare(ResultPath1, ResultPath2) };

			const auto fnRelativeDiff = [](const double& Value1, const double& Value2)
			{
				return std::abs(Value1 - Value2) / (1e-8 + (std::max)(std::abs(Value1), std::abs(Value2)));
			};

			for (const auto& var : CompareResults)
			{
				if(!var.Ordered.empty())
					Output.Report << fmt::format("*\tТоп {} отклонений переменной \"{}\" устройства типа \"{}\"\n",
						var.Ordered.size(),
						var.VariableName1,
						var.DeviceTypeVerbal);

				for (const auto& diff : var.Ordered)
				{
					ResultFileLib::IMinMaxDataPtr Max{ diff.second.CompareResult->Max };
					Output.Report << fmt::format("\t{} {} Max {:.5f}({:.5f}) {:.5f}% {:.5f}<=>{:.5f} Avg {:.5f}\n",
						diff.second.DeviceId,
						diff.second.DeviceName,
						Max->Metric,
						Max->Time,
						(fnRelativeDiff)(Max->Value1, Max->Value2) * 100.0,
						Max->Value1,
						Max->Value2,
						diff.second.CompareResult->Average
					);
				}

				if (!var.Ordered.empty())
				{
					const auto& diff{ *var.Ordered.begin() };
					ResultFileLib::IMinMaxDataPtr Max{ diff.second.CompareResult->Max };
					MaxDiff = Max->Metric;
					MaxDiffRel = (fnRelativeDiff)(Max->Value1, Max->Value2) * 100.0,
					MaxDiffTime = Max->Time;
					MaxDiffVariable = fmt::format("{}.{} {} [{}]",
						var.DeviceTypeVerbal, 
						var.VariableName1, 
						diff.second.DeviceId, 
						diff.second.DeviceName
					);
				}
			}
		}

		Output.BriefReport <<
			Input.CaseId << ";" <<
			Input.CaseFile.filename().u8string() << ";" <<
			Input.ContingencyFile.filename().u8string() << ";" <<
			(RetCode[0] != AST_OK ? (fnVerbalCode)(RetCode[0]) : (fnVerbalSyncLossCause)(SyncLossCause[0])) << ";" <<
			(RetCode[1] != AST_OK ? (fnVerbalCode)(RetCode[1]) : (fnVerbalSyncLossCause)(SyncLossCause[1])) << ";" <<
			Duration[0] << ";" <<
			Duration[1] << ";" <<
			MaxDiff << ";" <<
			MaxDiffRel << ";" <<
			MaxDiffTime << ";" <<
			MaxDiffVariable << ";" <<
			std::endl;

		Output.TimeRustab = Duration[0];
		Output.TimeRaiden = Duration[1];

		if (ConnectionPoint && handler && dwCookie > 0)
			ConnectionPoint->Unadvise(dwCookie);
	}
	
	catch (const _com_error& er)
	{
		if (Input.Opts.Threads > 1)
		{
			std::lock_guard<std::mutex> lock(Input.mtx_);
			Output.Report << "Исключение: " << stringutils::utf8_encode(er.Description()) << std::endl;
		}
		else
			throw dfw2error(stringutils::utf8_encode(er.Description()));
	}

	if (Input.Opts.Threads > 1)
	{
		std::lock_guard<std::mutex> lock(Input.mtx_);
		std::cout << "Завершен расчет " << Input.CaseId << std::endl;
	}
}