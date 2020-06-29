#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CRSTrigger : public CDynaPrimitiveBinary
	{
	protected:
		InputVariable m_Input1;
		bool m_bResetPriority;
	public:

		CRSTrigger::CRSTrigger(CDevice& Device, VariableIndex& OutputVariable, InputList Input, ExtraOutputList ExtraOutputVariables = {}) :
		  							CDynaPrimitiveBinary(Device, OutputVariable, Input, ExtraOutputVariables), m_Input1(*(Input.begin() + 1)) { }
		virtual ~CRSTrigger() {}

		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;

		const _TCHAR* GetVerbalName() noexcept  override { return _T("RS-Триггер"); }
		static size_t PrimitiveSize() noexcept { return sizeof(CRSTrigger); }
		static long EquationsCount()  noexcept { return 1; }
	};
}

