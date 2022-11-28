#pragma once
#include "DeviceContainer.h"
namespace DFW2
{
	class CTestDevice : public CDevice
	{
	public:
		enum VARS
		{
			V_X,			// координата
			V_V,			// скорость
			V_XREF,			// референс координаты
			V_LAST,
		};
		using CDevice::CDevice;
		static void DeviceProperties(CDeviceContainerProperties& properties);

		const double k = 1.4;
		const double m = 0.2;
		double omega = 0.0;
		double A = 0.0;
		double phi = 0.0;

		VariableIndex x,v, xref;

		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		void FinishStep(const CDynaModel& DynaModel) override;
	};
}