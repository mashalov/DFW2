#pragma once
#include "Device.h"
#include "CustomDeviceContainer.h"
using namespace std;

namespace DFW2
{
	typedef vector<CDynaPrimitive*> PRIMITIVEBLOCKS;
	typedef PRIMITIVEBLOCKS::iterator PRIMITIVEBLOCKITR;

	class CCustomDevice : public CDevice
	{
	protected:
		PRIMITIVEBLOCKS		m_Primitives;
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

		virtual double* GetConstVariablePtr(ptrdiff_t nVarIndex);

		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		virtual bool BuildEquations(CDynaModel *pDynaModel);
		virtual bool BuildRightHand(CDynaModel *pDynaModel);
		virtual bool BuildDerivatives(CDynaModel *pDynaModel);
		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);
		virtual double CheckZeroCrossing(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		CDynaPrimitive* GetPrimitiveForNamedOutput(const _TCHAR* cszOutputName);
	};
}

