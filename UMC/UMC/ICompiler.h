#pragma once
#include <iostream>
#include <string>
#include <variant>
#include "MessageCallbacks.h"
#include "../../DFW2/version.h"

class ICompiler
{
public:
	virtual bool SetProperty(std::string_view PropertyName, std::string_view Value) = 0;
	virtual std::string GetProperty(std::string_view PropertyName) = 0;
	virtual bool Compile(std::istream& stream) = 0;
	virtual bool Compile(std::filesystem::path FilePath) = 0;
	virtual void SetMessageCallBacks(const MessageCallBacks& MessageCallBackFunctions) = 0;
	virtual const std::filesystem::path& CompiledModulePath() const = 0;
	virtual const DFW2::VersionInfo& Version() const = 0;
	virtual void Destroy() = 0;
};

#ifdef _MSC_VER
typedef ICompiler* (__cdecl* fnCompilerFactory)();
#else
typedef ICompiler* (*fnCompilerFactory)();
#endif