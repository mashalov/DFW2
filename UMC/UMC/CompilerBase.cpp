#include "CompilerBase.h"
#include <shlobj_core.h>
#include <tchar.h>

bool CompilerBase::SetProperty(std::string_view PropertyName, std::string_view Value)
{
	if (auto it = Properties.find(PropertyName); it != Properties.end())
	{
		it->second = Value;
		return true;
	}
	return false;
}

std::string CompilerBase::GetProperty(std::string_view PropertyName)
{
	if (auto it = Properties.find(PropertyName); it != Properties.end())
		return it->second;
	else
		return "";
}

template<typename T>
struct UniqueHandleTraits
{
};

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


template<typename T, typename traits = UniqueHandleTraits<T> >
class UniqueHandle
{
protected:
	T handle = traits::invalid_value;
	T& exthandle;
public:
	UniqueHandle() : exthandle(handle) {};
	UniqueHandle(T& externalHandle) : exthandle(externalHandle) {}
	UniqueHandle(T&& externalHandle) : handle(externalHandle), exthandle(handle) {}
	constexpr operator T& () { return exthandle; }
	constexpr operator T* () { return &exthandle; }
	virtual ~UniqueHandle()
	{
		Close();
	}
	void Close()
	{
		if (exthandle != traits::invalid_value)
			traits::Close(exthandle);
		exthandle = traits::invalid_value;// ::INVALID_HANDLE_VALUE;
	}
};


DWORD RunWindowsConsole(std::wstring CommandLine, std::wstring WorkingFolder, std::list<std::wstring>& listConsole)
{

	std::string CommandLineUTF8Version(utf8_encode(CommandLine));
	DWORD dwCreationFlags = 0;
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
		throw std::system_error(std::error_code(GetLastError(),std::system_category()), 
			fmt::format("Невозможно создание канала получения данных для запуска {}", CommandLineUTF8Version));

	si.cb = sizeof(STARTUPINFO);
	si.hStdError = hStdWrite;
	si.hStdOutput = hStdWrite;
	si.dwFlags |= STARTF_USESTDHANDLES;

	UniqueHandle hProcess(pi.hProcess);
	UniqueHandle hThread(pi.hThread);

	auto wcmd = std::make_unique<wchar_t[]>(CommandLine.length() + 2);
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

	DWORD dwRead(0);
	const size_t BufSize = 500;
	auto chBuf = std::make_unique<char[]>(BufSize);
	BOOL bSuccess = FALSE;
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

	DWORD dwResult(0);
	if (!GetExitCodeProcess(pi.hProcess, &dwResult))
		throw std::system_error(std::error_code(GetLastError(), std::system_category()), 
			fmt::format("Ошибка при получении кода завершения работы процесса {}", CommandLineUTF8Version));

	return dwResult;
}

std::wstring CompilerBase::GetMSBuildPath()
{
	// используем vswhere из Visual Studio
	PWSTR ppszPath;
	if (FAILED(SHGetKnownFolderPath(FOLDERID_ProgramFilesX86, KF_FLAG_DEFAULT, NULL, &ppszPath)))
		throw std::system_error(std::error_code(GetLastError(), std::system_category()), "SHGetKnownFolderPath - отказ получения пути к Program Files");
	std::wstring MSBuildPath(ppszPath);
	CoTaskMemFree(ppszPath);
	MSBuildPath.append(L"\\Microsoft Visual Studio\\Installer\\vswhere.exe -latest -prerelease -products * -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe");

	std::list<std::wstring> output;
	DWORD dwResult(RunWindowsConsole(MSBuildPath,L"", output));

	if (dwResult != 0)
		throw std::system_error(std::error_code(GetLastError(), std::system_category()), "vswhere завершен с ошибкой");

	return output.size() ? output.front() : L"";// "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Professional\\MSBuild\\Current\\Bin\\msbuild.exe";
}


void CompilerBase::CompileWithMSBuild()
{
	// Pipeline
	// 1. Создаем каталог для сборки модели OurDir\ProjectName (уже сделано в генерации исходника)
	// 2. Копируем из референсного каталога ReferenceSources дополнительные исходники и vcxproj в каталог сборки

	// берем из параметров путь для вывода
	std::filesystem::path pathOutDir = pTree->GetPropertyPath(PropertyMap::szPropOutputPath);
	// добавляем к пути для вывода имя проекта
	pathOutDir.append(Properties[PropertyMap::szPropProjectName]);
	// формируем путь до CustomDevice.h, который должен быть сгенерирован в каталоге сборки
	std::filesystem::path pathCustomDeviceHeader = pathOutDir;
	pathCustomDeviceHeader.append("CustomDevice.h");
	// проверяем, есть ли CustomDevice.h в каталоге сборки
	if (!std::filesystem::exists(pathCustomDeviceHeader))
	{
		pTree->Error(fmt::format("В каталоге \"{}\" не найден файл скомпилированного пользовательского устройства \"{}\".",
			pathOutDir.string(), 
			CASTCodeGeneratorBase::CustomDeviceHeader));
		return;
	}
	// берем путь к референсному каталогу
	std::filesystem::path pathRefDir = pTree->GetPropertyPath(PropertyMap::szPropReferenceSources);
	// проверяем есть ли он
	if (!std::filesystem::exists(pathRefDir))
	{
		pTree->Error(fmt::format("Не найден каталог исходных файлов для сборки пользовательской модели \"{}\".",
			pathRefDir.string()));
		return;
	}
	// если каталог есть - копируем референсные файлы в каталог сборки (только уровень каталога, без рекурсии)
	std::filesystem::copy(pathRefDir, pathOutDir, std::filesystem::copy_options::overwrite_existing);
	
	// находим путь к msbuild
	std::wstring MSBuildPath(GetMSBuildPath());
	// задаем платформу сборки
	std::wstring Platform	= utf8_decode(Properties[PropertyMap::szPropPlatform]);
	// имя dll берем по имени проекта
	std::wstring TargetName  = utf8_decode(Properties[PropertyMap::szPropProjectName]);
	std::wstring Configuration = utf8_decode(Properties[PropertyMap::szPropConfiguration]);
	// формируем путь до vcxproj
	std::filesystem::path pathVcxproj = pathOutDir;
	pathVcxproj.append(CASTCodeGeneratorBase::CustomDeviceHeader).replace_extension("vcxproj");
	// формируем путь к результирующей dll модели
	std::filesystem::path pathDLLOutput = pathOutDir;
	pathDLLOutput.append("dll");
	// создаем ссылку на дополнительные include для сборки dll (они лежат в референсной папке)
	std::filesystem::path pathDFW2Include = pathRefDir;
	pathDFW2Include.append("DFW2");

	// формируем путь к pdb
	std::filesystem::path pathPDB = pathDLLOutput;
	pathPDB.append(Properties[PropertyMap::szPropProjectName]).replace_extension(".pdb");
	// удаляем pdb-файл чтобы избежать LNK1201 при сборке
	std::error_code ec;
	std::filesystem::remove(pathPDB,ec);

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

	DWORD dwResult(RunWindowsConsole(commandLine, WorkingFolder, listConsole));

	if (dwResult != 0)
	{
		for (auto& l : listConsole)
			std::cout << utf8_encode(l) << std::endl;
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

void CompilerBase::SaveSource(std::string_view SourceToCompile, std::filesystem::path& pathSourceOutput)
{
	std::filesystem::path OutputPath(pathSourceOutput);
	OutputPath.remove_filename();
	std::filesystem::create_directories(OutputPath);
	// UTF-8 filename ?
	std::ofstream OutputStream(pathSourceOutput);
	if (!OutputStream.is_open())
		throw std::system_error(std::error_code(GetLastError(), std::system_category()), fmt::format("Невозможно открыть файл \"{}\"", OutputPath.string()));
	OutputStream << SourceToCompile;
	OutputStream.close();
}

bool CompilerBase::NoRecompileNeeded(std::string_view SourceToCompile, std::filesystem::path& pathDLLOutput)
{
	bool bRes(false);
	if (!GetProperty(PropertyMap::szPropRebuild).empty())
		return bRes;
	// пытаемся найти dll, которую нужно собрать
	UniqueHandle dll(LoadLibrary(pathDLLOutput.generic_wstring().c_str()));
	if (static_cast<HMODULE>(dll) != nullptr)
	{
		using fnSourceType = const char* (__cdecl*)();
		fnSourceType fnSource = reinterpret_cast<fnSourceType>(::GetProcAddress(dll, "Source"));
		if (fnSource != nullptr)
		{
			std::string Source((fnSource)()), Gzip;
			CryptoPP::StringSource d64(Source, true, new CryptoPP::Base64Decoder(new CryptoPP::StringSink(Gzip)));
			Source.clear();
			CryptoPP::StringSource gzip(Gzip, true, new CryptoPP::Gunzip(new CryptoPP::StringSink(Source), 1));
			if (Source == SourceToCompile)
				bRes = true;
		}
	}

	if (bRes)
		pTree->Message(fmt::format("Модуль \"{}\" не нуждается в повторной компиляции", pathDLLOutput.string()));
	else
		pTree->Message(fmt::format("Необходима компиляция модуля \"{}\"", pathDLLOutput.string()));

	return bRes;
}