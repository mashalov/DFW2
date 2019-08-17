#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CZCDetector : public CDynaPrimitiveBinaryOutput
	{
	protected:
		virtual double OnStateOn(CDynaModel *pDynaModel);
		virtual double OnStateOff(CDynaModel *pDynaModel);
		double ChangeState(CDynaModel *pDynaModel);
	public:
		CZCDetector(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : CDynaPrimitiveBinaryOutput(pDevice, pOutput, nOutputIndex, Input) {}
		virtual ~CZCDetector() {}
		virtual bool Init(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		virtual const _TCHAR* GetVerbalName() { return _T("Пороговый элемент"); }
		static size_t PrimitiveSize() { return sizeof(CZCDetector); }
		static long EquationsCount()  { return 1; }
	};
}
