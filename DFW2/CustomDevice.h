#pragma once
#include "Device.h"
#include "CustomDeviceContainer.h"

namespace DFW2
{
	// прокси-устройство для включения пользовательского устройства из dll в расчетную модель
	class CCustomDevice : public CDevice
	{
	protected:
		PrimitiveVariable  *m_pPrimitiveVars;
		PrimitiveVariableExternal *m_pPrimitiveExtVars;

		VariableIndex *m_pVars;
		double *m_pConstVars;
		double *m_pSetPoints;
		ExternalVariable *m_pExternals;

		BuildEquationsArgs m_DLLArgs;

		bool ConstructDLLParameters(CDynaModel *pDynaModel);

		static long DLLEntrySetElement(BuildEquationsObjects *pBEObjs, ptrdiff_t nRow, ptrdiff_t nCol, double dValue, int bAddToPreviois);
		static long DLLEntrySetFunction(BuildEquationsObjects *pBEObjs, ptrdiff_t nRow, double dValue);
		static long DLLEntrySetFunctionDiff(BuildEquationsObjects *pBEObjs, ptrdiff_t nRow, double dValue);
		static long DLLEntrySetDerivative(BuildEquationsObjects *pBEObjs, ptrdiff_t nRow, double dValue);
		static long DLLProcessBlockDiscontinuity(BuildEquationsObjects *pBEObjs, long nBlockIndex);
		static long DLLInitBlock(BuildEquationsObjects *pBEObjs, long nBlockIndex);

		CCustomDeviceContainer *Container() { return static_cast<CCustomDeviceContainer*>(m_pContainer); }
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;

	public:
		eDEVICEFUNCTIONSTATUS m_ExternalStatus;

		CCustomDevice();
		virtual ~CCustomDevice();
		bool BuildStructure();
		bool SetConstDefaultValues();
		bool SetConstValue(size_t nIndex, double dValue);

		double* GetConstVariablePtr(ptrdiff_t nVarIndex) override;

		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		bool BuildEquations(CDynaModel *pDynaModel) override;
		bool BuildRightHand(CDynaModel *pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override;
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		double CheckZeroCrossing(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		CDynaPrimitive* GetPrimitiveForNamedOutput(const _TCHAR* cszOutputName);
	};

		
	class CCustomDeviceCPP : public CDevice
	{
	protected:
		std::shared_ptr<CCustomDeviceCPPDLL> m_pDLL;
		ICustomDevice* m_pDevice = nullptr;
		CCustomDeviceData CustomDeviceData;
		static CDynaModel*			GetModel(CDFWModelData& ModelData)	{ return static_cast<CDynaModel*>(ModelData.pModel); }
		static CCustomDeviceCPP*	GetDevice(CDFWModelData& ModelData)	{ return static_cast<CCustomDeviceCPP*>(ModelData.pDevice); }
		void PrepareCustomDeviceData(CDynaModel* pDynaModel);
	public:
		CCustomDeviceCPP();
		void CreateDLLDeviceInstance(CCustomDeviceCPPContainer& Container);
		void SetConstsDefaultValues();
		DOUBLEVECTOR& GetConstantData();
		void Connect(CDynaModel* pDynaModel);
		CCustomDeviceCPPContainer* GetContainer();
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel* pDynaModel) override;
		bool BuildRightHand(CDynaModel* pDynaModel) override;
		bool BuildEquations(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		virtual ~CCustomDeviceCPP();
		static void DLLSetElement(CDFWModelData& DFWModelData, const VariableIndexBase& Row, const VariableIndexBase& Col, double dValue);
		static void DLLSetFunction(CDFWModelData& DFWModelData, const VariableIndexBase& Row, double dValue);
		static void DLLSetFunctionDiff(CDFWModelData& DFWModelData, const VariableIndexBase& Row, double dValue);
		static void DLLSetDerivative(CDFWModelData& DFWModelData, const VariableIndexBase& Row, double dValue);
		static eDEVICEFUNCTIONSTATUS DLLInitPrimitive(CDFWModelData& DFWModelData, ptrdiff_t nPrimitiveIndex);
		static eDEVICEFUNCTIONSTATUS DLLProcPrimDisco(CDFWModelData& DFWModelData, ptrdiff_t nPrimitiveIndex);
		static const _TCHAR* m_cszNoDeviceDLL;
	};
}

