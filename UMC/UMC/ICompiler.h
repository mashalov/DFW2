﻿#pragma once
#include <iostream>
#include <string>
#include <variant>
#include "MessageCallbacks.h"


class ICompiler
{
public:
	virtual bool SetProperty(std::string_view PropertyName, std::string_view Value) = 0;
	virtual std::string GetProperty(std::string_view PropertyName) = 0;
	virtual bool Compile(std::istream& stream) = 0;
	virtual bool Compile(std::string_view FilePath) = 0;
	virtual void SetMessageCallBacks(const MessageCallBacks& MessageCallBackFunctions) = 0;
	virtual void Destroy() = 0;
};


typedef ICompiler* (__cdecl* fnCompilerFactory)();