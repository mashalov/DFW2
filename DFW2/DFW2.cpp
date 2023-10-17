// DFW2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "DynaModel.h"
#include "RastrImport.h"
#include <CLI11.hpp>

using namespace DFW2;
/*
struct CrtBreakAllocSetter {
	CrtBreakAllocSetter() {
		_crtBreakAlloc = 211;
	}
};

CrtBreakAllocSetter g_crtBreakAllocSetter;

*/


std::list<CDynaModel*> networks;

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
	for (auto&& nw : networks)
		nw->StopProcess();
	return TRUE;
}

void GenerateRastrWinTemplate(std::filesystem::path Path = {})
{
	CDynaModel::DynaModelParameters parameters;
	parameters.m_eConsoleLogLevel = DFW2MessageStatus::DFW2LOG_INFO;
	CDynaModel Network(parameters);
	try
	{
		{	
			CRastrImport ri;
			ri.GenerateRastrWinTemplate(Network, Path);
		}
	}
	catch (const _com_error& err)
	{
		const std::string Description{ CRastrImport::COMErrorDescription(err) };
		Network.Log(DFW2MessageStatus::DFW2LOG_FATAL, fmt::format(CDFW2Messages::m_cszCOMError, Description));
		throw dfw2error(Description);
	}
	catch (const dfw2error& err)
	{
		Network.Log(DFW2MessageStatus::DFW2LOG_FATAL, fmt::format(CDFW2Messages::m_cszError, err.what()));
		throw;
	}
}

void RunTransient()
{
	CDynaModel::DynaModelParameters parameters;
	CDynaModel Network(parameters);
	try
	{
		{	// работаем с RastrWin в scope
			CRastrImport ri;
			networks.push_back(&Network);
			//Network.DeSerialize(Network.Platform().ResultFile("serialization.json"));
			ri.GetFileData(Network);
		}
		//Network.Serialize(Network.Platform().ResultFile("lf_1500.json"));
		//Network.Serialize(Network.Platform().ResultFile("lf_7ku.json")); 
		//Network.RunLoadFlow();
		//Network.Serialize(Network.Platform().ResultFile("siberia.json")); 
		Network.RunTransient();
	}
	catch (const _com_error& err)
	{
		const std::string Description{ CRastrImport::COMErrorDescription(err) };
		Network.Log(DFW2MessageStatus::DFW2LOG_FATAL, fmt::format(CDFW2Messages::m_cszCOMError, Description));
		throw dfw2error(Description);
	}
	catch (const dfw2error& err)
	{
		Network.Log(DFW2MessageStatus::DFW2LOG_FATAL, fmt::format(CDFW2Messages::m_cszDFW2Error, err.what()));
		throw;
	}
	//Network.Serialize("c:\\tmp\\lf.json");
}

void RunTest()
{
	CDynaModel::DynaModelParameters parameters;
	CDynaModel Network(parameters);
	networks.push_back(&Network);
	Network.RunTest();
}

int main(int argc, char* argv[])
{
	int Ret{ 0 };
	_CrtSetDbgFlag(_CRTDBG_CHECK_ALWAYS_DF);
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
	SetConsoleOutputCP(CP_UTF8);
	HANDLE hCon{ GetStdHandle(STD_OUTPUT_HANDLE) };
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hCon, &csbi);
	try
	{
		if (HRESULT hr{ CoInitialize(NULL) }; FAILED(hr))
			throw dfw2error("Ошибка CoInitialize {:0x}", static_cast<unsigned long>(hr));

		CLI::App app{ fmt::format("{} {} built {} for {} {}",
			CDFW2Messages::m_cszProjectName,
			CDynaModel::version,
			__DATE__,
			CDFW2Messages::m_cszOS,
			CDFW2Messages::m_cszCopyright)};

		std::string cli_templatetogenerate;
		bool cli_showversion;
		constexpr const char* szPath{ "PATH" };
		app.add_option("--gt", cli_templatetogenerate, "Update RastrWin3 template specified at the given path")->option_text(szPath);
		app.add_flag("--ver", cli_showversion, "Show version info");

		try
		{
			app.parse(argc, argv);
		}
		catch (const CLI::ParseError& e)
		{
			return app.exit(e);
		}

		if (cli_showversion)
			std::cout << app.get_description() << std::endl;
		else if (!cli_templatetogenerate.empty())
			GenerateRastrWinTemplate(cli_templatetogenerate);
		else
		{
			SetConsoleCtrlHandler(HandlerRoutine, TRUE);
			RunTransient();
			//RunTest();
			networks.clear();
			CoUninitialize();
		}
	}
	catch (const dfw2error& err)
	{
		SetConsoleTextAttribute(hCon, csbi.wAttributes);
		std::cout << "Исключение: " << err.what() << std::endl;
		Ret = 1;
	}
	SetConsoleCtrlHandler(NULL, TRUE);
	_CrtDumpMemoryLeaks();
	return Ret;
}

