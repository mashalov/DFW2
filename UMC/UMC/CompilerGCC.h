#pragma once
#include "CompilerBase.h"

class CCompilerGCC : public CompilerBase
{
protected:
	void BuildWithCompiler() override;
	void CompileWithGCC();
	std::optional<ModelMetaData> GetMetaData(const std::filesystem::path& pathDLLOutput) override;
};
