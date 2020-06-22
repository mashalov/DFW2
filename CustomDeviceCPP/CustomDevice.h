#pragma once
#include "..\DFW2\ICustomDevice.h"

namespace DFW2
{
	class CCustomDevice : public ICustomDevice
	{
	protected:
		DOUBLEVECTOR m_Consts;
		VARIABLEVECTOR m_Variables;
	public:
		CCustomDevice();
		void BuildRightHand(CustomDeviceData* pData) override;
		void GetDeviceProperties(CDeviceContainerPropertiesBase& DeviceProps) override;
		void SetConstsDefaultValues() override;
		DOUBLEVECTOR& GetConstantData() override;
		VariableIndexVec GetVariables() override;
		void Destroy() override;
	};
}
