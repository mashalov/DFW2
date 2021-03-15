#pragma once
#include "winbase.h"

class CHRTimer
{
public:
	CHRTimer(const char *szName = nullptr, bool bNoAuto = false)
	{
		QueryPerformanceFrequency(&liFreq);
		m_Seconds = -1;
		if (szName)
			m_szName = _strdup(szName);
		else
			m_szName = nullptr;

		if (!bNoAuto)
			StartTimer();
		else
			liCounterStart.QuadPart = 0;
	}

	double GetSeconds()
	{
		return m_Seconds;
	}

	void StartTimer()
	{
		liCounterStart.QuadPart = 0;
		m_Seconds = -1;
		QueryPerformanceCounter(&liCounterStart);
	}

	void StopTimer(bool bReport = true)
	{
		QueryPerformanceCounter(&liCounterEnd);
		m_Seconds = (double)((__int64)liCounterEnd.QuadPart - (_int64)liCounterStart.QuadPart);
	}


	~CHRTimer()
	{
		if (liCounterStart.QuadPart) StopTimer(false);
		if (m_szName)
			free(m_szName);
	}

protected:
	LARGE_INTEGER liCounterStart, liCounterEnd, liFreq;
	double m_Seconds;
	char *m_szName;
};
