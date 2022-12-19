#include "CompilerMSBuild.h"
#ifdef _MSC_VER
#include <shlobj_core.h>
#include <tchar.h>
#endif

void CCompilerMSBuild::BuildWithCompiler()
{
	CompileWithMSBuild();
}

template<> struct UniqueHandleTraits<HANDLE>
{
	static inline const HANDLE invalid_value = INVALID_HANDLE_VALUE;
	static const void Close(HANDLE h) { CloseHandle(h); }
};

template<> struct UniqueHandleTraits<HMODULE>
{
	static inline const HMODULE invalid_value = NULL;
	static const void Close(HMODULE h) { FreeLibrary(h); }
};

DWORD RunWindowsConsole(std::wstring CommandLine, std::wstring WorkingFolder, std::list<std::wstring>& listConsole)
{

	std::string CommandLineUTF8Version(utf8_encode(CommandLine));
	const DWORD dwCreationFlags{ 0 };
	UniqueHandle<HANDLE> hStdWrite, hStdRead;
	SECURITY_ATTRIBUTES stdSA;
	ZeroMemory(&stdSA, sizeof(SECURITY_ATTRIBUTES));
	stdSA.nLength = sizeof(SECURITY_ATTRIBUTES);
	stdSA.bInheritHandle = TRUE;
	stdSA.lpSecurityDescriptor = NULL;

	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(STARTUPINFO));
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

	if (!CreatePipe(hStdRead, hStdWrite, &stdSA, 0))
		throw std::system_error(std::error_code(GetLastError(), std::system_category()),
			fmt::format("Невозможно создание канала получения данных для запуска {}", CommandLineUTF8Version));

	si.cb = sizeof(STARTUPINFO);
	si.hStdError = hStdWrite;
	si.hStdOutput = hStdWrite;
	si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;

	UniqueHandle hProcess(pi.hProcess);
	UniqueHandle hThread(pi.hThread);

	auto wcmd{ std::make_unique<wchar_t[]>(CommandLine.length() + 2) };
	_tcscpy_s(wcmd.get(), CommandLine.length() + 2, CommandLine.c_str());

	if (!SetHandleInformation(hStdRead, HANDLE_FLAG_INHERIT, 0))
		throw std::system_error(std::error_code(GetLastError(), std::system_category()),
			fmt::format("Невозможно подключение канала получения данных {}", CommandLineUTF8Version));

	if (!CreateProcess(NULL,
		wcmd.get(),
		NULL,
		NULL,
		TRUE,
		dwCreationFlags,
		NULL,
		WorkingFolder.length() ? WorkingFolder.c_str() : NULL,
		&si,
		&pi))
		throw std::system_error(std::error_code(GetLastError(), std::system_category()),
			fmt::format("Ошибка запуска процесса {}", CommandLineUTF8Version));

	// не нужно ждать завершения, если мы используем перенаправление вывода
	// https://stackoverflow.com/questions/35969730/how-to-read-output-from-cmd-exe-using-createprocess-and-createpipe
	// закрываем handle вывода в консоль, иначе ReadFile может зависнуть
	hStdWrite.Close();

	DWORD dwRead{ 0 };
	const size_t BufSize{ 500 };
	auto chBuf{ std::make_unique<char[]>(BufSize) };
	BOOL bSuccess{ FALSE };
	std::string strOut;
	for (;;)
	{
		bSuccess = ReadFile(hStdRead, chBuf.get(), BufSize, &dwRead, NULL);
		// процесс закроет хэндл hStdRead у себя когда закончит вывод
		// так мы узнаем что процесс завершен
		if (!bSuccess || dwRead == 0) break;
		std::string strMsgLine(static_cast<const char*>(chBuf.get()), dwRead);

		// ищем в строке переводы строки и разбиваем ее на отдельные строки
		size_t nFind = std::string::npos;
		while ((nFind = strMsgLine.find("\r\n")) != std::string::npos)
		{
			// добавляем то, что прочитали до перевода строки 
			// в текущую строку
			strOut += strMsgLine.substr(0, nFind);
			// и добавляем получившуюся строку в список
			listConsole.push_back(utf8_decode(strOut));
			// строку очищаем и ждем нового перевода строки
			strOut.clear();
			// из исходной строки удаляем перевод
			strMsgLine.erase(0, nFind + 2);
		}
		// если после чтения в исходной строке что-то осталось
		// это значит что строка из процессе еще не закончилась.
		// добавляем остаток к текущей строке
		if (strMsgLine.length() > 0)
			strOut += strMsgLine.substr(0, nFind);
	}

	// если после обработки вывода что-то осталось в текущей
	// строке - добавляем ее в список
	if (!strOut.empty())
		listConsole.push_back(utf8_decode(strOut));

	DWORD dwResult{ 0 };
	if (!GetExitCodeProcess(pi.hProcess, &dwResult))
		throw std::system_error(std::error_code(GetLastError(), std::system_category()),
			fmt::format("Ошибка при получении кода завершения работы процесса {}", CommandLineUTF8Version));

	return dwResult;
}

std::wstring CCompilerMSBuild::GetMSBuildPath()
{
	// используем vswhere из Visual Studio
	PWSTR ppszPath;
	if (FAILED(SHGetKnownFolderPath(FOLDERID_ProgramFilesX86, KF_FLAG_DEFAULT, NULL, &ppszPath)))
		throw std::system_error(std::error_code(GetLastError(), std::system_category()), "SHGetKnownFolderPath - отказ получения пути к Program Files");
	std::wstring MSBuildPath(ppszPath);
	CoTaskMemFree(ppszPath);
	MSBuildPath.append(L"\\Microsoft Visual Studio\\Installer\\vswhere.exe -latest -prerelease -products * -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe");

	std::list<std::wstring> output;
	DWORD dwResult(RunWindowsConsole(MSBuildPath, L"", output));

	if (dwResult != 0)
		throw std::system_error(std::error_code(GetLastError(), std::system_category()), "vswhere завершен с ошибкой");

	return output.size() ? output.front() : L"";// "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Professional\\MSBuild\\Current\\Bin\\msbuild.exe";
}


void CCompilerMSBuild::CompileWithMSBuild()
{
	// Pipeline
	// 1. Создаем каталог для сборки модели OurDir\ProjectName (уже сделано в генерации исходника)
	// 2. Копируем из референсного каталога ReferenceSources дополнительные исходники и vcxproj в каталог сборки

	// находим путь к msbuild
	std::wstring MSBuildPath(GetMSBuildPath());
	// задаем платформу сборки
	std::wstring Platform = utf8_decode(Properties[PropertyMap::szPropPlatform]);
	// имя dll берем по имени проекта
	std::wstring TargetName = utf8_decode(Properties[PropertyMap::szPropProjectName]);
	std::wstring Configuration = utf8_decode(Properties[PropertyMap::szPropConfiguration]);
	// формируем путь до vcxproj
	std::filesystem::path pathVcxproj = pathOutDir;
	pathVcxproj.append(CASTCodeGeneratorBase::CustomDeviceHeader).replace_extension("vcxproj");
	// формируем путь к результирующей dll модели
	std::filesystem::path pathDLLOutput = pathOutDir;
	pathDLLOutput.append("dll");
	// создаем ссылку на дополнительные include для сборки dll (они лежат в референсной папке)
	std::filesystem::path pathDFW2Include = pathRefDir;
	// формируем путь к pdb
	std::filesystem::path pathPDB = pathDLLOutput;
	pathPDB.append(Properties[PropertyMap::szPropProjectName]).replace_extension(".pdb");
	// удаляем pdb-файл чтобы избежать LNK1201 при сборке
	std::error_code ec;
	std::filesystem::remove(pathPDB, ec);

	// если есть слэши в конце пути, убираем их
	// иначе msbuild неправильно понимает последовательность \"

	if (!pathDFW2Include.has_filename())
		pathDFW2Include = pathDFW2Include.parent_path();
	if (!pathDLLOutput.has_filename())
		pathDLLOutput = pathDLLOutput.parent_path();

	// генерируем командную строку msbuild
	std::wstring commandLine = fmt::format(L"\"{}\"", MSBuildPath);

	std::wstring commandLineArgs = fmt::format(L" /p:Platform={} /p:Configuration={} /p:TargetName={} /p:OutDir=\"{}\\\\\" /p:DFW2Include=\"{}\\\\\"  \"{}\"",
		Platform,
		Configuration,
		TargetName,
		pathDLLOutput.wstring(),
		pathDFW2Include.wstring(),
		pathVcxproj.wstring());

	commandLine += commandLineArgs;

	// конвертируем пути для CreateProcess в wchar_t
	std::wstring WorkingFolder = pathOutDir.generic_wstring();

	std::list<std::wstring> listConsole;

	if (messageCallBacks.Debug)
		messageCallBacks.Debug(fmt::format("MSBuild using : {}", utf8_encode(commandLine)));

	DWORD dwResult(RunWindowsConsole(commandLine, WorkingFolder, listConsole));

	if (dwResult != 0)
	{
		for (auto& l : listConsole)
		{
			const std::string utf8message{ utf8_encode(l) };
			//std::cout << utf8message << std::endl;
			if (messageCallBacks.Debug)
				messageCallBacks.Debug(utf8message);
		}
		pTree->Error("Ошибка компиляции MSBuild");
	}
	else
	{
		std::filesystem::path pathResultingDLL = pathDLLOutput;
		pathResultingDLL.append(TargetName).replace_extension(".dll");
		std::filesystem::path pathDLLLibrary = pTree->GetPropertyPath(PropertyMap::szPropDllLibraryPath);
		std::filesystem::create_directories(pathDLLLibrary);
		std::filesystem::copy(pathResultingDLL, pathDLLLibrary, std::filesystem::copy_options::overwrite_existing);
	}
}

std::optional<CompilerBase::ModelMetaData> CCompilerMSBuild::GetMetaData(const std::filesystem::path& pathDLLOutput)
{
	// конвертируем путь в UNICODE
	UniqueHandle dll(LoadLibrary(pathDLLOutput.c_str()));
	if (static_cast<HMODULE>(dll) != nullptr)
	{
		using fnSourceType = const char* (__cdecl*)();
		using fnVersionType = const DFW2::VersionInfo& (__cdecl*)();
		fnSourceType fnSource = reinterpret_cast<fnSourceType>(::GetProcAddress(dll, "Source"));
		fnVersionType fnModelVersion = reinterpret_cast<fnVersionType>(::GetProcAddress(dll, "ModelVersion"));
		fnVersionType fnCompilerVersion = reinterpret_cast<fnVersionType>(::GetProcAddress(dll, "CompilerVersion"));
		if (fnSource != nullptr && fnModelVersion != nullptr && fnCompilerVersion != nullptr)
			return CompilerBase::ModelMetaData{ fnSource(), fnModelVersion(), fnCompilerVersion() };
	}
	return {};
}

