﻿// DFW2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "DynaModel.h"
#include "RastrImport.h"
#include "cli.h"

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

int _tmain(int argc, _TCHAR* argv[])
{
	int Ret{ 0 };
	_CrtSetDbgFlag(_CRTDBG_CHECK_ALWAYS_DF);
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
	SetConsoleOutputCP(CP_UTF8);
	try
	{
		if (HRESULT hr{ CoInitialize(NULL) }; FAILED(hr))
			throw dfw2error("Ошибка CoInitialize {:0x}", static_cast<unsigned long>(hr));

		const CLIParser<_TCHAR> cli(argc, argv);
		const auto& CLIOptions{ cli.Options() };

		if (const auto gtp{ CLIOptions.find("gt") }; gtp != CLIOptions.end())
			GenerateRastrWinTemplate(gtp->second);
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
		std::cout << "Исключение: " << err.what() << std::endl;
		Ret = 1;
	}
	SetConsoleCtrlHandler(NULL, TRUE);
	_CrtDumpMemoryLeaks();
	return Ret;
}

