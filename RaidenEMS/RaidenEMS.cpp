﻿// RaidenEMS.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <array>
#include "../DFW2/stringutils.h"
#include "../DFW2/dfw2exception.h"
#include <CLI11.hpp>
#include "../DFW2/DLLWrapper.h"

void Log(std::string_view Message)
{
	SetConsoleOutputCP(CP_ACP);
	std::cout << stringutils::console_encode(Message) << std::endl;
}

int main(int argc, char* argv[])
{
	int Ret{ 1 };
	try
	{
		if (HRESULT hr{ CoInitialize(NULL) }; FAILED(hr))
			throw dfw2error("Ошибка CoInitialize {:0x}", static_cast<unsigned long>(hr));

		CLI::App app{ "Raiden EMS utility"};
		std::string dllpath, templatespath;
		app.add_option("--dll", dllpath, "dfw2.dll path");
		app.add_option("--templates", templatespath, "RastrWin3 templates path");

		try 
		{
			app.parse(argc, argv);
		}
		catch (const CLI::ParseError& e) 
		{
			SetConsoleOutputCP(CP_UTF8);
			return app.exit(e);
		}

		constexpr const char* cszGenerateRastrTemplate = "GenerateRastrTemplate";
		if (!dllpath.empty() && !templatespath.empty())
		{
			DFW2::CDLLInstance Raiden(dllpath);
			auto fnGenerateTemplates{ static_cast<long(__cdecl*)(const wchar_t*)>(Raiden.GetProcAddress(cszGenerateRastrTemplate)) };
			if (fnGenerateTemplates)
			{
				int SuccessCount{ 0 };
				std::filesystem::path BasePath{ templatespath };
				std::array<const wchar_t*, 2> TemplateFiles = {L"динамика.rst", L"poisk.os"};

				for (const auto& TemplateFile : TemplateFiles)
				{
					std::filesystem::path Template{ BasePath };
					Template.append(TemplateFile);

					auto Result{ (*fnGenerateTemplates)(Template.c_str()) };
					if (Result)
						SuccessCount++;
					Log(fmt::format("{}Обновление {}", 
						Result ? "" : "Отказ ! : ",
						stringutils::utf8_encode(Template.c_str())));
				}
				if (SuccessCount == TemplateFiles.size())
					Ret = 0;
			}
			else
				throw dfw2error("Не найдена функция {}", cszGenerateRastrTemplate);
		}
		CoUninitialize();
	}
	catch (const dfw2error& err)
	{
		Log(err.what());
	}
	return Ret;
}

