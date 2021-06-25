﻿#pragma once
#include "CompilerBase.h"

class CCompilerGCC : public CompilerBase
{
protected:
	void BuildWithCompiler() override;
	std::optional<std::string> GetSource(const std::filesystem::path& pathDLLOutput) override;
};
