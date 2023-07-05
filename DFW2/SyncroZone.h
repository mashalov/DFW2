#pragma once
#include "Device.h"
#include "DeviceContainerProperties.h"

namespace DFW2
{
	class CSynchroZone : public CDevice
	{
	protected:
		double CalculateS() const;
	public:

		using CDevice::CDevice;

		enum VARS
		{
			V_S,			// скольжение в синхронной зоне
			V_DELTA,		// угол синхронной зоны		
			V_LAST
		};

		DEVICEVECTOR LinkedGenerators;
		VariableIndex S, Delta;					// переменная состояния - скольжение

		double Mj = 0.0;						// суммарный момент инерции
		bool InfPower = false;					// признак наличия ШБМ

		virtual ~CSynchroZone() = default;
		bool Energized = false;					// признак наличия источника напряжения

		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel)  override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel)  override;
		static void DeviceProperties(CDeviceContainerProperties& properties);
		void AnglesToSyncReference();
	};
}

