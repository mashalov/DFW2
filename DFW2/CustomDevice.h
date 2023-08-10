#pragma once
#include "Device.h"
#include "CustomDeviceContainer.h"

namespace DFW2
{
	using CCustomDeviceDLLWrapper = CDLLInstanceWrapper<CCustomDeviceCPPDLL>;

	class CCustomDeviceDataHost : public CCustomDeviceData
	{
	public:
		void SetFnSetElement(FNCDSETELEMENT pFn) { pFnSetElement = pFn; }
		void SetFnSetFunction(FNCDSETFUNCTION pFn) { pFnSetFunction = pFn; }
		void SetFnSetFunctionDiff(FNCDSETFUNCTIONDIFF pFn) { pFnSetFunctionDiff = pFn; }
		void SetFnSetDerivative(FNCDSETDERIVATIVE pFn) { pFnSetDerivative = pFn; }
		void SetFnInitPrimitive(FNCDINITPRIMITIVE pFn) { pFnInitPrimitive = pFn; }
		void SetFnProcessPrimitiveDisco(FNCDPROCPRIMDISCO pFn) { pFnProcPrimDisco = pFn; }
		void SetFnIndexedVariable(FNCDINDEXED pFn) { pFnIndexed = pFn; }
		void SetModelData(CDynaModel* Model, CDevice* Device);
	};

		
	class CCustomDeviceCPP : public CDevice
	{
	protected:
		CCustomDeviceDLLWrapper m_pDevice;
		CCustomDeviceDataHost CustomDeviceData;
		static CDynaModel*			GetModel(CDFWModelData& ModelData)	{ return static_cast<CDynaModel*>(ModelData.pModel); }
		static CCustomDeviceCPP*	GetDevice(CDFWModelData& ModelData)	{ return static_cast<CCustomDeviceCPP*>(ModelData.pDevice); }
		void PrepareCustomDeviceData(CDynaModel* pDynaModel);
		DOUBLEVECTOR m_PrimitiveVars;
	public:
		CCustomDeviceCPP();
		void CreateDLLDeviceInstance(CCustomDeviceCPPContainer& Container);
		void SetConstsDefaultValues();
		void SetSourceConstant(size_t nIndex, double Value);
		CCustomDeviceCPPContainer* GetContainer();
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		void BuildEquations(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		static void DLLSetElement(CDFWModelData& DFWModelData, const VariableIndexBase& Row, const VariableIndexBase& Col, double dValue);
		static void DLLSetFunction(CDFWModelData& DFWModelData, const VariableIndexBase& Row, double dValue);
		static bool DLLIndexedVariable(CDFWModelData& DFWModelData, const VariableIndexBase& Variable);
		static void DLLSetFunctionDiff(CDFWModelData& DFWModelData, const VariableIndexBase& Row, double dValue);
		static void DLLSetDerivative(CDFWModelData& DFWModelData, const VariableIndexBase& Row, double dValue);
		static eDEVICEFUNCTIONSTATUS DLLInitPrimitive(CDFWModelData& DFWModelData, ptrdiff_t nPrimitiveIndex);
		static eDEVICEFUNCTIONSTATUS DLLProcPrimDisco(CDFWModelData& DFWModelData, ptrdiff_t nPrimitiveIndex);
		CDynaPrimitive* GetPrimitiveForNamedOutput(std::string_view OutputName);
		static constexpr const char* m_cszNoDeviceDLL = "CCustomDeviceCPP - no DLL device initialized";
	};
}

