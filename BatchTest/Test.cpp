﻿#include <iostream>
#include "Test.h"
#include "ResultCompare.h"
#import "progid:Astra.Rastr.1" named_guids no_namespace
#include "nlohmann/json.hpp"

_bstr_t bstrPath(const std::filesystem::path& Path)
{
	return _bstr_t(Path.c_str());
}

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


		std::cout << "Поиск файлов моделей в каталоге " << CaseFilesFolder.string() << std::endl;

		for (const auto& entry : std::filesystem::directory_iterator(CaseFilesFolder))
		{
			const auto& path{ entry.path() };
			if (!path.has_extension())
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

		std::cout << "Будет выполнено " << ContingencyFiles_.size() * CaseFiles_.size() << " расчетов" << std::endl;
		std::cout << "Длительность расчета: " << GlobalOptions.Duration << std::endl;
		std::cout << "Режим: " << (GlobalOptions.EmsMode ? "EMS" : "Инженерный") << std::endl;
		std::cout << "Точность Raiden: " << GlobalOptions.RaidenAtol << std::endl;
		std::cout << "Hmin RUSTab: " << GlobalOptions.RUSTabHmin << std::endl;
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

	report.open("c:\\tmp\\rep.txt");


	constexpr const char* NoRastrWinFoundInRegistry{ "Не найдено данных RastrWin3 в реестре" };
	HKEY handle{ NULL };
	try
	{
		if (const HRESULT hr{ CoInitialize(NULL) }; FAILED(hr))
			throw dfw2error("Ошибка CoInitialize scode {}", hr);
	

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

		long CaseId{ 0 };

		for (const auto& contfile : ContingencyFiles_)
			for (const auto& casefile : CaseFiles_)
			{
				
				Options opts{GlobalOptions};
				opts.CaseId = ++CaseId;
				TestPair(casefile, contfile, opts);
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
		IColPtr SnapTemplate{ComDynamic->Cols->Item("SnapTemplate")};
		IColPtr SnapPath{ ComDynamic->Cols->Item("SnapPath") };

		Hint->PutZ(0, Opts.RUSTabHmin);
		Hmin->PutZ(0, Opts.RUSTabHmin);

		DurationSet->PutZ(0, Opts.Duration);

		ITablePtr RaidenParameters{ Rastr->Tables->Item("RaidenParameters") };
		IColPtr GoRaiden{ RaidenParameters->Cols->Item("GoRaiden")};
		IColPtr Atol{ RaidenParameters->Cols->Item("Atol") };
		IColPtr ResultsFolder{ RaidenParameters->Cols->Item("ResultsFolder") };

		Atol->PutZ(0, Opts.RaidenAtol);

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

		auto fnMessage = [](const _bstr_t& message)
		{
			return message.length() > 0 ? stringutils::utf8_encode(message) : "Нет";
		};

		IColPtr StopOnBranchOOS{ RaidenParameters->Cols->Item("StopOnBranchOOS") };
		IColPtr StopOnGeneratorOOS{ RaidenParameters->Cols->Item("StopOnGeneratorOOS") };
		IColPtr DisableResultWriter{ RaidenParameters->Cols->Item("DisableResultsWriter") };

		StopOnBranchOOS->PutZ(0, 1);
		StopOnGeneratorOOS->PutZ(0, 1);

		if (Opts.EmsMode)
		{
			StopOnBranchOOS->PutZ(0, 1);
			StopOnGeneratorOOS->PutZ(0, 1);
			DisableResultWriter->PutZ(0, 1);
		}

		report << CaseFile.filename().u8string() << ";" << ContingencyFile.filename().u8string() << ";";
		const std::string ResultFileName{ fmt::format("{:05d}.sna", Opts.CaseId) };
		std::filesystem::path ResultPath1(Opts.ResultPath), ResultPath2(Opts.ResultPath);
		ResultPath1.append("RUSTab");
		SnapPath->PutZ(0, bstrPath(ResultPath1));
		SnapTemplate->PutZ(0, stringutils::COM_encode(ResultFileName).c_str());
		ResultPath1.append(ResultFileName);
		ResultPath2.append("Raiden");
		ResultPath2.append(ResultFileName);

		ResultsFolder->PutZ(0, bstrPath(ResultPath2));

		std::cout << fmt::format("Расчет {} из {}\nМодель: {}\nСценарий: {}\n",
			Opts.CaseId,
			ContingencyFiles_.size() * CaseFiles_.size(),
			CaseFile.u8string(),
			ContingencyFile.u8string()
		);
		
		for (int method{ 0 }; method < 2; method++)
		{
			CTestTimer Timer;
			GoRaiden->PutZ(0, method);
			const auto RustabRetCode{ Opts.EmsMode ? FWDynamic->RunEMSmode() : FWDynamic->Run() };
			const auto Duration{ Timer.Duration() };

			report << Duration << ";";
			report << (fnVerbalSyncLossCause)(FWDynamic->SyncLossCause) << ";";

			
			std::cout << fmt::format("*\t{} {} режим "
									 "\n\tРезультат: {}"
									 "\n\tПросчитано: {:.3f}с"
									 "\n\tУстойчивость: {}"
									 "\n\tСообщение: {}"
									 "\n\tВремя расчета: {:.3f}с",

					method ? "Raiden" : "RUSTab",
					Opts.EmsMode ? "EMS" : "Инженерный",
					(fnVerbalCode)(RustabRetCode),
					FWDynamic->TimeReached,
					(fnVerbalSyncLossCause)(FWDynamic->SyncLossCause),
					(fnMessage)(FWDynamic->ResultMessage),
					Duration) << std::endl;

			if (!Opts.EmsMode)
				std::cout << fmt::format("\tФайл результатов : {}",
					method ? ResultPath2.u8string() : ResultPath1.u8string()) << std::endl;
		}

		if (!Opts.EmsMode)
		{
			ResultCompare compare;
			const auto CompareResults{ compare.Compare(ResultPath1, ResultPath2) };
			for (const auto& var : CompareResults)
			{
				if(!var.Ordered.empty())
					std::cout << fmt::format("*\tТоп {} отклонений переменной \"{}\" устройства типа \"{}\"\n",
						var.Ordered.size(),
						var.VariableName,
						var.DeviceTypeVerbal);

				for (const auto& diff : var.Ordered)
				{
					ResultFileLib::IMinMaxDataPtr Max{ diff.second.CompareResult->Max };
					std::cout << fmt::format("\t{} {} Max {:.5f}({:.5f}) {:.5f}<=>{:.5f} Avg {:.5f}\n",
						diff.second.DeviceId,
						diff.second.DeviceName,
						Max->Metric,
						Max->Time,
						Max->Value1,
						Max->Value2,
						diff.second.CompareResult->Average
					);
				}
			}
		}
	}

	catch (const _com_error& er)
	{
		throw dfw2error(stringutils::utf8_encode(er.Description()));
	}
}