#include "CompilerGCC.h"
#ifndef _MSC_VER
#include <dlfcn.h>

template<> struct UniqueHandleTraits<void*>
{
	static inline  void* const invalid_value = NULL;
	static const void Close(void* h) { dlclose(h); }
};

void CCompilerGCC::BuildWithCompiler()
{
	// -s - strip symbol table and relocations
	//g++ -s -O2 -fPIC -shared -std=c++17 -o [outname] [*.cpp]
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
