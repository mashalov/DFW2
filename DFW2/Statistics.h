#pragma once
#include "Device.h"
namespace DFW2
{
	class StatisticsMaxFinder
	{
	protected:
		const CDevice* m_pDevice = nullptr;
		double m_dTime = 0.0;
		double m_dValue = 0.0;
	public:
		void UpdateAbs(double Value, double Time, const CDevice* pDevice)
		{
			if (!m_pDevice || std::abs(m_dValue) < std::abs(Value))
			{
				m_dTime = Time;
				m_dValue = Value;
				m_pDevice = pDevice;
			}
		}
		void Reset()
		{
			*this = {};
		}

		const CDevice* Device() const
		{
			return m_pDevice;
		}

		double Time() const
		{
			return m_dTime;
		}

		double Value() const
		{
			return m_dValue;
		}
	};
}
