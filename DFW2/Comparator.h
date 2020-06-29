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
		CComparator(CDevice& Device, VariableIndex& OutputVariable, InputList Input, ExtraOutputList ExtraOutputVariables) :
			CDynaPrimitiveBinaryOutput(Device, OutputVariable, Input, ExtraOutputVariables), m_Input1(*(Input.begin() + 1)) { }
		virtual ~CComparator() {}
		bool Init(CDynaModel *pDynaModel) override;
//		virtual const _TCHAR* GetVerbalName() { return _T(""); }
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		static size_t PrimitiveSize() { return sizeof(CComparator); }
		static long EquationsCount()  { return 1; }
	};
}
