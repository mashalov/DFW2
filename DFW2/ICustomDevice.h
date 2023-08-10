#pragma once
#include "DeviceContainerPropertiesBase.h"
namespace DFW2
{
	class CDFWModelData
	{
	public:
		void* pModel = nullptr;
		void* pDevice = nullptr;
		double t;
	};

	using FNCDSETELEMENT		= void(*)(CDFWModelData&, const VariableIndexBase&, const VariableIndexBase&, double);
	using FNCDSETFUNCTION		= void(*)(CDFWModelData&, const VariableIndexBase&, double);
	using FNCDSETFUNCTIONDIFF	= void(*)(CDFWModelData&, const VariableIndexBase&, double);
	using FNCDSETDERIVATIVE		= void(*)(CDFWModelData&, const VariableIndexBase&, double);
	using FNCDINITPRIMITIVE		= eDEVICEFUNCTIONSTATUS(*)(CDFWModelData&, ptrdiff_t nPrimitiveIndex);
	using FNCDPROCPRIMDISCO		= eDEVICEFUNCTIONSTATUS(*)(CDFWModelData&, ptrdiff_t nPrimitiveIndex);
	using FNCDINDEXED			= bool(*)(CDFWModelData&, const VariableIndexBase&);

	class CCustomDeviceData : protected CDFWModelData
	{
	protected:
		FNCDSETELEMENT			pFnSetElement;
		FNCDSETFUNCTION			pFnSetFunction;
		FNCDSETFUNCTIONDIFF		pFnSetFunctionDiff;
		FNCDSETDERIVATIVE		pFnSetDerivative;
		FNCDINITPRIMITIVE		pFnInitPrimitive;
		FNCDPROCPRIMDISCO		pFnProcPrimDisco;
		FNCDINDEXED				pFnIndexed;
	public:
		inline void ProcessPrimitiveDisco(ptrdiff_t nPrimitiveIndex)
		{
			(*pFnProcPrimDisco)(*this, nPrimitiveIndex);
		}
		inline void InitPrimitive(ptrdiff_t nPrimitiveIndex)
		{
			(*pFnInitPrimitive)(*this, nPrimitiveIndex);
		}
		inline void SetFunction(const VariableIndexBase& Row, double Value)
		{
			(*pFnSetFunction)(*this, Row, Value);
		}
		inline void SetElement(const VariableIndexBase& Row, const VariableIndexBase& Col, double Value)
		{
			(*pFnSetElement)(*this, Row, Col, Value);
		}
		inline bool IndexedVariable(const VariableIndexBase& Variable)
		{
			return (*pFnIndexed)(*this, Variable);
		}
		inline double time() const
		{
			return t;
		}
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
		virtual bool SetSourceConstant(size_t Index, double Value) = 0;
		virtual const VariableIndexRefVec& GetVariables() = 0;
		virtual const  VariableIndexExternalRefVec& GetExternalVariables() = 0;
		virtual const PRIMITIVEVECTOR& GetPrimitives() = 0;
		virtual void SetConstsDefaultValues() = 0;
		virtual const DOUBLEVECTOR& GetBlockParameters(ptrdiff_t BlockIndex) = 0;
	};

#ifdef _MSC_VER
	typedef ICustomDevice* (__cdecl* CustomDeviceFactory)();
#else
	typedef ICustomDevice* (*CustomDeviceFactory)();
#endif 
}
