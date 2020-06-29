#pragma once
#include "DeviceContainerPropertiesBase.h"

namespace DFW2
{
	class CDFWModelData
	{
	public:
		void* pModel = nullptr;
		void* pDevice = nullptr;
	};

	using FNCDSETELEMENT		= void(*)(CDFWModelData&, const VariableIndexBase&, const VariableIndexBase&, double);
	using FNCDSETFUNCTION		= void(*)(CDFWModelData&, const VariableIndexBase&, double);
	using FNCDSETFUNCTIONDIFF	= void(*)(CDFWModelData&, const VariableIndexBase&, double);
	using FNCDSETDERIVATIVE		= void(*)(CDFWModelData&, const VariableIndexBase&, double);
	using FNCDINITPRIMITIVE		= eDEVICEFUNCTIONSTATUS(*)(CDFWModelData&, ptrdiff_t nPrimitiveIndex);
	using FNCDPROCPRIMDISCO		= eDEVICEFUNCTIONSTATUS(*)(CDFWModelData&, ptrdiff_t nPrimitiveIndex);

	class CCustomDeviceData : public CDFWModelData
	{
	public:
		FNCDSETELEMENT			pFnSetElement;
		FNCDSETFUNCTION			pFnSetFunction;
		FNCDSETFUNCTIONDIFF		pFnSetFunctionDiff;
		FNCDSETDERIVATIVE		pFnSetDerivative;
		FNCDINITPRIMITIVE		pFnInitPrimitive;
		FNCDPROCPRIMDISCO		pFnProcPrimDisco;
	};

	class ICustomDevice
	{
	public:
		virtual void Destroy() = 0;
		virtual void BuildRightHand(CCustomDeviceData& CustomDeviceData) = 0;
		virtual void BuildEquations(CCustomDeviceData& CustomDeviceData) = 0;
		virtual void BuildDerivatives(CCustomDeviceData& CustomDeviceData) = 0;
		virtual eDEVICEFUNCTIONSTATUS Init(CCustomDeviceData& CustomDeviceData) = 0;
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CCustomDeviceData& CustomDeviceData) = 0;

		virtual void GetDeviceProperties(CDeviceContainerPropertiesBase& DeviceProps) = 0;
		virtual DOUBLEVECTOR& GetConstantData() = 0;
		virtual VARIABLEVECTOR& GetVariables() = 0;
		virtual EXTVARIABLEVECTOR& GetExternalVariables() = 0;
		virtual PRIMITIVEVECTOR& GetPrimitives() = 0;
		virtual void SetConstsDefaultValues() = 0;
	};

	typedef ICustomDevice* (__cdecl* CustomDeviceFactory)();
}
