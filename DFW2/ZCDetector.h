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
		CZCDetector(CDevice *pDevice, VariableIndex& OutputVariable, InputList Input, ExtraOutputList ExtraOutputVariables) :
			CDynaPrimitiveBinaryOutput(pDevice, OutputVariable, Input, ExtraOutputVariables) {}
		virtual ~CZCDetector() {}
		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		const _TCHAR* GetVerbalName() noexcept override { return _T("Пороговый элемент"); }
		static size_t PrimitiveSize() noexcept { return sizeof(CZCDetector); }
		static long EquationsCount() noexcept { return 1; }
	};
}
