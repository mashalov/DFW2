#pragma once
#include "Device.h"
#include "CustomDeviceContainer.h"
using namespace std;

namespace DFW2
{
	class CCustomDevice : public CDevice
	{
	protected:
		PrimitiveVariable  *m_pPrimitiveVars;
		PrimitiveVariableExternal *m_pPrimitiveExtVars;

		double *m_pVars;
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
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel);

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
}

