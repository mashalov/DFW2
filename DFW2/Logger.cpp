#include "stdafx.h"
#include <iostream>
#include "Logger.h"

using namespace DFW2;

void CLoggerConsole::Log(DFW2MessageStatus Status, std::string_view Message, ptrdiff_t nDbIndex) const
{
	if (Status <= DFW2MessageStatus::DFW2LOG_ERROR)
	{
#ifdef _MSC_VER
		// для Windows делаем разные цвета в консоли
		HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(hCon, &csbi);

		switch (Status)
		{
		case DFW2MessageStatus::DFW2LOG_FATAL:
			SetConsoleTextAttribute(hCon, BACKGROUND_RED | FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN);
			break;
		case DFW2MessageStatus::DFW2LOG_ERROR:
			SetConsoleTextAttribute(hCon, FOREGROUND_RED | FOREGROUND_INTENSITY);
			break;
		case DFW2MessageStatus::DFW2LOG_WARNING:
			SetConsoleTextAttribute(hCon, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			break;
		case DFW2MessageStatus::DFW2LOG_MESSAGE:
		case DFW2MessageStatus::DFW2LOG_INFO:
			SetConsoleTextAttribute(hCon, FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN);
			break;
		case DFW2MessageStatus::DFW2LOG_DEBUG:
			SetConsoleTextAttribute(hCon, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			break;
		}

		SetConsoleOutputCP(CP_UTF8);
#endif
		std::cout << Message << std::endl;

#ifdef _MSC_VER
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY | FOREGROUND_RED);
#endif
	}
}
