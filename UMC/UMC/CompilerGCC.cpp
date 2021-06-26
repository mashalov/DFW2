#include "CompilerGCC.h"

void CCompilerGCC::BuildWithCompiler()
{
	// -s - strip symbol table and relocations
	//g++ -s -O2 -fPIC -shared -std=c++17 -o [outname] [*.cpp]
}

std::optional<std::string> CCompilerGCC::GetSource(const std::filesystem::path& pathDLLOutput)
{
	return {};
}
