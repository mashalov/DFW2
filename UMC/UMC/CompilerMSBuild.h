#pragma once

#include "CompilerBase.h"

class CCompilerMSBuild : public CompilerBase
{
protected:
	void CompileWithMSBuild();
	void BuildWithCompiler() override;
	std::optional<ModelMetaData> GetMetaData(const std::filesystem::path& pathDLLOutput) override;
	std::wstring GetMSBuildPath();
};

