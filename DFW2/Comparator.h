#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CComparator : public CDynaPrimitiveBinaryOutput
	{
	protected:
		PrimitiveVariableBase *m_Input2;

		virtual double OnStateOn(CDynaModel *pDynaModel);
		virtual double OnStateOff(CDynaModel *pDynaModel);

	public:
		CComparator(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input1, PrimitiveVariableBase* Input2);
		virtual ~CComparator() {}
		virtual bool Init(CDynaModel *pDynaModel);
//		virtual const _TCHAR* GetVerbalName() { return _T(""); }
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		static size_t PrimitiveSize() { return sizeof(CComparator); }
		static long EquationsCount()  { return 1; }
	};
}
