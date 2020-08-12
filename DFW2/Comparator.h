#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CComparator : public CDynaPrimitiveBinaryOutput
	{
	protected:
		InputVariable m_Input1;

		double OnStateOn(CDynaModel *pDynaModel) override;
		double OnStateOff(CDynaModel *pDynaModel) override;

	public:

		CComparator(CDevice& Device, const ORange& Output, const IRange& Input) : 
			CDynaPrimitiveBinaryOutput(Device, Output, Input), m_Input1(Input[1]) {}
		CComparator(CDevice& Device, const OutputList& Output, const InputList& Input) :
			CComparator(Device, ORange(Output), IRange(Input)) { }

		virtual ~CComparator() {}
		bool Init(CDynaModel *pDynaModel) override;
//		virtual const _TCHAR* GetVerbalName() { return _T(""); }
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		static size_t PrimitiveSize() { return sizeof(CComparator); }
		static long EquationsCount()  { return 1; }
	};
}
