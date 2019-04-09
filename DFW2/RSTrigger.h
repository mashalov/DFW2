#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CRSTrigger : public CDynaPrimitiveBinary
	{
	protected:
		PrimitiveVariableBase* m_Input1;
		bool m_bResetPriority;
	public:

		CRSTrigger::CRSTrigger(CDevice *pDevice, 
							   double* pOutput, 
							   ptrdiff_t nOutputIndex, 
							   PrimitiveVariableBase* R, 
							   PrimitiveVariableBase* S, 
							   bool bResetPriority) : 
															CDynaPrimitiveBinary(pDevice, pOutput, nOutputIndex, R),
															m_Input1(S),
															m_bResetPriority(bResetPriority) 
															{ }
		virtual ~CRSTrigger() {}

		virtual bool Init(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);

		virtual const _TCHAR* GetVerbalName() { return _T("RS-Триггер"); }
		static size_t PrimitiveSize() { return sizeof(CRSTrigger); }
		static long EquationsCount()  { return 1; }
	};
}

