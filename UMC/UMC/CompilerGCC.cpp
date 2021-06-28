﻿#include "CompilerGCC.h"
#ifndef _MSC_VER
#include <dlfcn.h>

template<> struct UniqueHandleTraits<void*>
{
	static inline void* const invalid_value = NULL;
	static const void Close(void* h) { dlclose(h);}
};

std::pair<std::string, int> exec(const char* cmd) 
{
	std::array<char, 128> buffer;
	std::string result;
	int return_code = -1;
	auto pclose_wrapper = [&return_code](FILE* cmd) 
	{ 
		return_code = pclose(cmd); 
	};

	{ 
		const std::unique_ptr<FILE, decltype(pclose_wrapper)> pipe(popen(cmd, "r"), pclose_wrapper);
		if(!pipe)
			throw std::system_error(std::error_code(errno, std::system_category()),
				fmt::format("Ошибка запуска компилятора {}", cmd));

		size_t count(0);
		do 
		{
			if ((count = fread(buffer.data(), 1, buffer.size(), pipe.get())) > 0) 
				result.insert(result.end(), std::begin(buffer), std::next(std::begin(buffer), count));
		} 
		while (count > 0);
	}
	return make_pair(result, return_code);
}

void CCompilerGCC::CompileWithGCC()
{
	std::filesystem::path pathCustomDeviceCPP = pathOutDir;
	pathCustomDeviceCPP.append("CustomDevice").replace_extension(".cpp");

	// формируем путь к результирующей dll модели
	std::filesystem::path pathSOOutput(pTree->GetPropertyPath(PropertyMap::szPropDllLibraryPath));
	pathSOOutput.append(Properties[PropertyMap::szPropProjectName]).replace_extension(".so");
	const std::string CommandLine(fmt::format("g++ -std=c++17 -fPIC -shared -s -O2 -I'{}' -o '{}' '{}'",
		pathRefDir.string(),
		pathSOOutput.string(),
		pathCustomDeviceCPP.string()));

	const auto Result(exec(CommandLine.c_str()));
	if (Result.second != 0)
	{
		std::cout << Result.first << std::endl;
		pTree->Error("Ошибка компиляции MSBuild");
	}
}

void CCompilerGCC::BuildWithCompiler()
{
	// -s - strip symbol table and relocations
	//g++ -s -O2 -fPIC -shared -std=c++17 -o [outname] [*.cpp]
	CompileWithGCC();
}

std::optional<std::string> CCompilerGCC::GetSource(const std::filesystem::path& pathDLLOutput)
{
	// конвертируем путь в UNICODE
	UniqueHandle dll(dlopen(pathDLLOutput.string().c_str(), RTLD_LAZY));
	if (static_cast<void*>(dll) != NULL)
	{
		using fnSourceType = const char* (*)();
		fnSourceType fnSource = reinterpret_cast<fnSourceType>(dlsym(dll,"Source"));
		if (fnSource != nullptr)
			return std::string((fnSource)());
	}
	return {};
}
#endif
