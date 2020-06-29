#pragma once
#include "..\DFW2\ICustomDevice.h"

namespace DFW2
{
	class CCustomDevice : public ICustomDevice
	{
	protected:
		DOUBLEVECTOR		m_Consts;
		VARIABLEVECTOR		m_Variables;
		EXTVARIABLEVECTOR	m_ExtVariables;
		PRIMITIVEVECTOR		m_Primitives;
	public:
		CCustomDevice();
		void BuildRightHand(CCustomDeviceData& CustomDeviceData) override;
		void BuildEquations(CCustomDeviceData& CustomDeviceData) override;
		void BuildDerivatives(CCustomDeviceData& CustomDeviceData) override;
		eDEVICEFUNCTIONSTATUS Init(CCustomDeviceData& CustomDeviceData) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CCustomDeviceData& CustomDeviceData) override;
		void GetDeviceProperties(CDeviceContainerPropertiesBase& DeviceProps) override;
		void SetConstsDefaultValues() override;
		DOUBLEVECTOR& GetConstantData() override;
		VARIABLEVECTOR& GetVariables() override;
		EXTVARIABLEVECTOR& GetExternalVariables() override;
		PRIMITIVEVECTOR& GetPrimitives() override;
		void Destroy() override;
	};
}
