#include "stdafx.h"
#ifdef _MSC_VER
#include "RastrImport.h"
#include "TaggedPath.h"

using namespace DFW2;

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) LONG __cdecl Run(IRastrPtr spRastr, unsigned long ThreadId)
{
	LONG RetCode{ 0 };
	try
	{
		CDynaModel::DynaModelParameters parameters;
		parameters.ThreadId_ = ThreadId;

		HMODULE module{ nullptr };
		wchar_t Path[MAX_PATH];
		if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
			GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPWSTR)&Run, &module))
			if (GetModuleFileName(module, Path, sizeof(Path)))
			{
				parameters.ModuleFilePath_ = Path;
				parameters.ModuleFilePath_.remove_filename();
			}


		CDynaModel Network(parameters);
		Network.SetLogger(std::make_unique<CLoggerRastrWin>(spRastr));
		Network.SetProgress(std::make_unique<CProgressRastrWin>(spRastr));

		try
		{
			CRastrImport ri(spRastr);
			ri.GetData(Network);
			RetCode = Network.RunTransient();
			ri.StoreResults(Network);
			// если задан шаблон имени для сохранения отладочного дампа
			if (const auto& ModelTemplateName{ Network.Parameters().DebugModelNameTemplate_ } ; !ModelTemplateName.empty())
			{
				auto ModelPath{ Network.Platform().ModelDebugFolder() };
				// создаем каталог для дампов
				Network.Platform().CheckPath(ModelPath);
				// формируем имя файла дампа по шаблону
				ModelPath.append(stringutils::utf8_decode(ModelTemplateName));
				TaggedPath path(ModelPath);
				// создаем файл
				path.Create().close();
				// сохраняем файл без шаблона
				spRastr->Save(stringutils::utf8_decode(path.PathString()).c_str(), L"");
			}
		}
		catch (_com_error& err)
		{
			Network.Log(DFW2MessageStatus::DFW2LOG_FATAL, fmt::format("Ошибка COM : {}", stringutils::utf8_encode(std::wstring(err.Description()))));
		}
		catch (const dfw2error& err)
		{
			Network.Log(DFW2MessageStatus::DFW2LOG_FATAL, fmt::format("Ошибка : {}", err.what()));
		}
}
	catch (const dfw2error& err)
	{
		std::cout << "Ошибка " << err.what() << std::endl;
	}
    return RetCode;
}
#else
#include "CompilerGCC.h"
extern "C" __attribute__((visibility("default"))) ICompiler * CompilerFactory()
{
    return new CCompilerGCC();
}

#endif
