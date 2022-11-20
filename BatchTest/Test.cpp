#include <iostream>
#include "Test.h"
#import "progid:Astra.Rastr.1" named_guids no_namespace
#import "progid:ResultFile.Raiden.1" named_guids no_namespace

_bstr_t bstrPath(const std::filesystem::path& Path)
{
	return _bstr_t(Path.c_str());
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
	SetConsoleOutputCP(65001);
	report.open("c:\\tmp\\rep.txt");


	constexpr const char* NoRastrWinFoundInRegistry{ "No RastrWin3 found in registry" };
	HKEY handle{ NULL };
	try
	{
		if (const HRESULT hr{ CoInitialize(NULL) }; FAILED(hr))
			throw dfw2error("CoInitialize failed with scode {}", hr);
	

		std::filesystem::path templatePath;
		const auto cszUserFolder = L"UserFolder";


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

		RegCloseKey(handle);
		handle = NULL;

		macroPath_ = templatePath / L"macro";
		

		templatePath.append(L"shablon");

		rstPath_ = templatePath;
		dfwPath_ = templatePath;
		scnPath_ = templatePath;

		rstPath_.append(L"динамика.rst");
		dfwPath_.append(L"автоматика.dfw");
		scnPath_.append(L"сценарий.scn");

		Options opts;
		opts.EmsMode_ = false;

		for (const auto& contfile : ContingencyFiles_)
			for (const auto& casefile : CaseFiles_)
				TestPair(casefile, contfile, opts);
	}
	catch (const std::runtime_error& er)
	{
		if (handle)
			RegCloseKey(handle);

		auto s{ stringutils::utf8_decode(er.what()) };

		std::cout << er.what();
	}
	report.close();
}

void CBatchTest::TestPair(const std::filesystem::path& CaseFile, const std::filesystem::path& ContingencyFile, const Options& Opts)
{
	try
	{
		IRastrPtr Rastr;
		if (const HRESULT hr{ Rastr.CreateInstance(CLSID_Rastr) }; FAILED(hr))
			throw dfw2error("Rastr CoCreate failed with scode {}", hr);
		Rastr->Load(RG_REPL, bstrPath(CaseFile), bstrPath(rstPath_));
		Rastr->Load(RG_REPL, bstrPath(ContingencyFile), bstrPath(scnPath_));
		Rastr->NewFile(bstrPath(dfwPath_));
		Rastr->ExecMacroPath(bstrPath(macroPath_ / "Scn2Dfw.rbs"), L"");
		Rastr->NewFile(bstrPath(scnPath_));
		auto FWDynamic{ Rastr->FWDynamic() };
		
		ITablePtr ComDynamic{ Rastr->Tables->Item("com_dynamics") };
		IColPtr DurationSet{ ComDynamic->Cols->Item("Tras")};
		IColPtr Hint{ ComDynamic->Cols->Item("Hint") };
		IColPtr Hmin{ ComDynamic->Cols->Item("Hmin") };
		IColPtr Hmax{ ComDynamic->Cols->Item("Hmax") };

		//Hint->PutZ(0, 5e-3);
		//Hmin->PutZ(0, 5e-3);
		//Hmax->PutZ(0, 5.0);

		DurationSet->PutZ(0, 15.0);

		ITablePtr RaidenParameters{ Rastr->Tables->Item("RaidenParameters") };
		IColPtr GoRaiden{ RaidenParameters->Cols->Item("GoRaiden")};
		IColPtr Atol{ RaidenParameters->Cols->Item("Atol") };
		Atol->PutZ(0, 1e-4);

		/*report << fmt::format("-- Модель: {}\n-- Возмущение: {}\n-- Режим: {}\n-- Длительность ЭМПП: {:.3f}",
			CaseFile.filename().u8string(),
			ContingencyFile.filename().u8string(),
			Opts.EmsMode_ ? "EMS" : "Инженерный",
			DurationSet->GetZ(0).dblVal
			) << std::endl;*/

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

		IColPtr StopOnBranchOOS{ RaidenParameters->Cols->Item("StopOnBranchOOS") };
		IColPtr StopOnGeneratorOOS{ RaidenParameters->Cols->Item("StopOnGeneratorOOS") };
		IColPtr DisableResultWriter{ RaidenParameters->Cols->Item("DisableResultsWriter") };

		StopOnBranchOOS->PutZ(0, 1);
		StopOnGeneratorOOS->PutZ(0, 1);

		if (Opts.EmsMode_)
		{
			StopOnBranchOOS->PutZ(0, 1);
			StopOnGeneratorOOS->PutZ(0, 1);
			DisableResultWriter->PutZ(0, 1);
		}

		report << CaseFile.filename().u8string() << ";" << ContingencyFile.filename().u8string() << ";";

		IResultPtr Result;
		if(const HRESULT hr{ Result.CreateInstance(CLSID_Result) }; FAILED(hr))
			throw dfw2error("Result CoCreate failed with scode {}", hr);
		IResultReadPtr ResultRead{ Result->Load(L"c:\\tmp\\000000.sna") };
		const auto plot1{ ResultRead->GetPlot(1000006, 248, L"modV") };
		const auto plot2{ ResultRead->GetPlot(1000006, 240, L"modV") };
		ICompareResultPtr CompareResult{ ResultRead->Compare(plot1,plot2) };

		IMinMaxDataPtr Max{ CompareResult->Max };
		IMinMaxDataPtr Min{ CompareResult->Min };
		IMinMaxDataPtr Left{ CompareResult->Left };
		IMinMaxDataPtr Right{ CompareResult->Right };

		auto strrep{
			fmt::format("Max {:.5f}({:.05f}) {:.5f}:{:.5f}, Min {:.5f}({:.05f}) {:.5f}:{:.5f}, Avg {:.5f}, Sqr {:.5f}",
			Max->Metric,
			Max->Time,
			Max->Value1,
			Max->Value2,
			Min->Metric,
			Min->Time,
			Min->Value1,
			Min->Value2,
			CompareResult->Average,
			CompareResult->SquaredDiff
			)
		};
		
		for (int method{ 0 }; method < 2; method++)
		{
			CTestTimer Timer;
			GoRaiden->PutZ(0, method);
			const auto RustabRetCode{ Opts.EmsMode_ ? FWDynamic->RunEMSmode() : FWDynamic->Run() };
			const auto Duration{ Timer.Duration() };

			report << Duration << ";";
			report << (fnVerbalSyncLossCause)(FWDynamic->SyncLossCause) << ";";

			/*
				report << fmt::format("{} Результат: {} Просчитано: {:.3f} Устойчивость: {} Сообщение: {} Время расчета: {:.3f}",
				method ? "Raiden" : "RUSTab",
				(fnVerbalCode)(RustabRetCode),
				FWDynamic->TimeReached,
				(fnVerbalSyncLossCause)(FWDynamic->SyncLossCause),
				stringutils::utf8_encode(FWDynamic->ResultMessage),
				Duration
			) << std::endl;
			*/
		}

		report << std::endl;
	}

	catch (const _com_error& er)
	{
		throw dfw2error(stringutils::utf8_encode(er.Description()));
	}
}