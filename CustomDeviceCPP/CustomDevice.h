#pragma once
#include "..\DFW2\ICustomDevice.h"

namespace DFW2
{
	class CCustomDevice : public ICustomDevice
	{
	protected:
		DOUBLEVECTOR m_Consts;
	public:
		CCustomDevice();
		void BuildRightHand(CustomDeviceData* pData) override;
		void GetDeviceProperties(CDeviceContainerPropertiesBase& DeviceProps) override;
		void SetConstsDefaultValues() override;
		DOUBLEVECTOR& GetConstantData() override;
		void Destroy() override;
	};
}
