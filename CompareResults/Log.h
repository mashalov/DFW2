#pragma once
#include <windows.h>
#include <iostream>
#include "..\DFW2\dfw2exception.h"
class CLog
{
public:
	void Log(std::string_view log)
	{
		std::cout << log << std::endl;
	}
};

