﻿#pragma once
#include "DeviceContainer.h"
#include "LimitedLag.h"
#include "LimiterConst.h"
#include "DerlagContinuous.h"
namespace DFW2
{
	// уравнение гармонического маятника
	// m * x''= -k * x
	// аналитическое решение
	// x(t) = A * cos(omega * t + phi)
	// omega = sqrt(k / m)
	// A = x(0)

	class CTestDevice : public CDevice
	{
	public:
		enum VARS
		{
			V_X,			// координата
			V_V,			// скорость
			V_LAG,			// выход лага
			V_DER,			// выход дерлага
			V_DER1,			// лаг дерлага
			V_XREF,			// референс координаты
			V_LAST,
		};
		CTestDevice();
		virtual ~CTestDevice() = default;

		const double k = 3.0;
		const double m = 0.3;
		double omega = 0.0;
		double A = 0.0;
		double phi = 0.0;

		CDerlagContinuous Der_;
		CLimiterConst OutputLag_;
		VariableIndex x, v, xref, LagOut, DerOut, DerOut1;

		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		void FinishStep(const CDynaModel& DynaModel) override;

		static void DeviceProperties(CDeviceContainerProperties& properties);
	};
}