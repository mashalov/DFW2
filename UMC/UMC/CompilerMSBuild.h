#pragma once

#include "CompilerBase.h"

class CCompilerMSBuild : public CompilerBase
{
protected:
	void CompileWithMSBuild();
	void BuildWithCompiler() override;
	std::optional<ModelMetaData> GetMetaData(const std::filesystem::path& pathDLLOutput) override;
	std::wstring GetMSBuildPath();
	DFW2::VersionInfo GetMSBuildVersion(const std::filesystem::path& MSBuildPath);
	DFW2::VersionInfo GetVSVersion();
	static DFW2::VersionInfo Version(const std::wstring& strVersion);
};

