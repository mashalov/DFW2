#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CComparator : public CDynaPrimitiveBinaryOutput
	{
	protected:
		InputVariable Input1_;

		double OnStateOn(CDynaModel *pDynaModel) override;
		double OnStateOff(CDynaModel *pDynaModel) override;

	public:

		CComparator(CDevice& Device, const ORange& Output, const IRange& Input) : 
			CDynaPrimitiveBinaryOutput(Device, Output, Input), Input1_(Input[1]) {}
		CComparator(CDevice& Device, const OutputList& Output, const InputList& Input) :
			CComparator(Device, ORange(Output), IRange(Input)) { }

		virtual ~CComparator() = default;
		bool Init(CDynaModel *pDynaModel) override;
//		virtual const char* GetVerbalName() { return ""; }
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		static size_t PrimitiveSize() { return sizeof(CComparator); }
		static long EquationsCount()  { return 1; }
	};
}
