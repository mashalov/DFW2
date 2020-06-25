#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CZCDetector : public CDynaPrimitiveBinaryOutput
	{
	protected:
		double OnStateOn(CDynaModel *pDynaModel) override;
		double OnStateOff(CDynaModel *pDynaModel) override;
	public:
		CZCDetector(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, std::initializer_list<PrimitiveVariableBase*> Input) : 
			CDynaPrimitiveBinaryOutput(pDevice, pOutput, nOutputIndex, Input) {}
		virtual ~CZCDetector() {}
		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		const _TCHAR* GetVerbalName() noexcept override { return _T("Пороговый элемент"); }
		static size_t PrimitiveSize() noexcept { return sizeof(CZCDetector); }
		static long EquationsCount() noexcept { return 1; }
	};
}
