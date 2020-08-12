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

		CZCDetector(CDevice& Device, const ORange& Output, const IRange& Input) : 
			CDynaPrimitiveBinaryOutput(Device, Output, Input) {}
		CZCDetector(CDevice& Device, const OutputList& Output, const InputList& Input) : 
			CZCDetector(Device, ORange(Output), IRange(Input)) { }

		virtual ~CZCDetector() {}
		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		const _TCHAR* GetVerbalName() noexcept override { return _T("Пороговый элемент"); }
		static size_t PrimitiveSize() noexcept { return sizeof(CZCDetector); }
		static long EquationsCount() noexcept { return 1; }
	};
}
