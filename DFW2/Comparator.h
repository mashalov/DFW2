#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CComparator : public CDynaPrimitiveBinaryOutput
	{
	protected:
		PrimitiveVariableBase *m_Input2;

		double OnStateOn(CDynaModel *pDynaModel) override;
		double OnStateOff(CDynaModel *pDynaModel) override;

	public:
		CComparator(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input1, PrimitiveVariableBase* Input2);
		virtual ~CComparator() {}
		bool Init(CDynaModel *pDynaModel) override;
//		virtual const _TCHAR* GetVerbalName() { return _T(""); }
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		static size_t PrimitiveSize() { return sizeof(CComparator); }
		static long EquationsCount()  { return 1; }
	};
}
