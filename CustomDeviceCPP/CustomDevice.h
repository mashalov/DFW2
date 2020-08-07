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
		DOUBLEVECTOR		m_BlockParameters;
		VariableIndexRefVec m_StateVariablesRefs;
		VariableIndexExternalRefVec m_ExternalVariablesRefs;
	public:
		CCustomDevice();
		void BuildRightHand(CCustomDeviceData& CustomDeviceData) override;
		void BuildEquations(CCustomDeviceData& CustomDeviceData) override;
		void BuildDerivatives(CCustomDeviceData& CustomDeviceData) override;
		eDEVICEFUNCTIONSTATUS Init(CCustomDeviceData& CustomDeviceData) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CCustomDeviceData& CustomDeviceData) override;
		void GetDeviceProperties(CDeviceContainerPropertiesBase& DeviceProps) override;
		void SetConstsDefaultValues() override;
		bool SetSourceConstant(size_t nIndex, double Value);
		const VariableIndexRefVec& GetVariables() override;
		const VariableIndexExternalRefVec& GetExternalVariables() override;
		const PRIMITIVEVECTOR& GetPrimitives() override;
		const DOUBLEVECTOR& GetBlockParameters(ptrdiff_t nBlockIndex) override;
		void Destroy() override;
	};
}
