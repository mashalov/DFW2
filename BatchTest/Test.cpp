#include <iostream>
#include "Test.h"
#import "progid:Astra.Rastr.1" named_guids no_namespace

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

		templatePath.append(L"shablon");

		rstPath_ = templatePath;
		dfwPath_ = templatePath;
		scnPath_ = templatePath;

		rstPath_.append(L"динамика.rst");
		dfwPath_.append(L"автоматика.dfw");
		scnPath_.append(L"сценарий.scn");

		Options opts;

		for (const auto& casefile : CaseFiles_)
		{
			for (const auto& contfile : ContingencyFiles_)
			{
				TestPair(casefile, contfile, opts);
			}
		}
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
		if (const HRESULT hr{ Rastr.CreateInstance("Astra.Rastr.1") }; FAILED(hr))
			throw dfw2error("Rastr CoCreate failed with scode {}", hr);
		Rastr->Load(RG_REPL, bstrPath(CaseFile), bstrPath(rstPath_));
		Rastr->Load(RG_REPL, bstrPath(ContingencyFile), bstrPath(dfwPath_));
		Rastr->NewFile(bstrPath(scnPath_));
		CTestTimer RustabTimer;
		auto FWDynamic{ Rastr->FWDynamic() };
		const auto RustabRetCode { Opts.EmsMode_ ? FWDynamic->RunEMSmode() : FWDynamic->Run() };
		const auto RustabDuration{ RustabTimer.Duration() };

		ITablePtr ComDynamic{ Rastr->Tables->Item("com_dynamics") };
		IColPtr DurationSet{ ComDynamic->Cols->Item("Tras")};

		report << fmt::format("Модель: {} Возмущение: {} Режим: {} Длительность ЭМПП: {:.3f}",
			CaseFile.filename().u8string(),
			ContingencyFile.filename().u8string(),
			Opts.EmsMode_ ? "EMS" : "Инженерный",
			DurationSet->GetZ(0).dblVal
			) << std::endl;

		auto fnVerbalCode = [](const RastrRetCode& code) -> std::string
		{
			return code == AST_OK ? "Ok" : "Failed !";
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

		report << fmt::format("Rustab Результат: {} Просчитано: {:.3f} Устойчивость: {} Сообщение: {} Время расчета: {:.3f}",
			(fnVerbalCode)(RustabRetCode),
			FWDynamic->TimeReached,
			(fnVerbalSyncLossCause)(FWDynamic->SyncLossCause),
			stringutils::utf8_encode(FWDynamic->ResultMessage),
			RustabDuration
			) << std::endl;

	}
	catch (const _com_error& er)
	{
		throw dfw2error(stringutils::utf8_encode(er.Description()));
	}
}