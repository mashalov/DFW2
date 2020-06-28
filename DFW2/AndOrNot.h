#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CAnd: public CDynaPrimitiveBinary
	{
	protected:
		InputVariable m_Input1;
	public:
		CAnd(CDevice& Device, VariableIndex& OutputVariable, InputList Input, ExtraOutputList ExtraOutputVariables) :
			CDynaPrimitiveBinary(Device, OutputVariable, Input, ExtraOutputVariables), m_Input1(*(Input.begin() + 1)) {}
		virtual ~CAnd() {}

		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;

		const _TCHAR* GetVerbalName() override { return _T("Логическое И"); }
		static size_t PrimitiveSize() { return sizeof(CAnd); }
		static long EquationsCount()  { return 1; }
	};

	class COr : public CDynaPrimitiveBinary
	{
	protected:
		InputVariable m_Input1;
	public:
		COr(CDevice& Device, VariableIndex& OutputVariable, InputList Input, ExtraOutputList ExtraOutputVariables) :
			CDynaPrimitiveBinary(Device, OutputVariable, Input, ExtraOutputVariables), m_Input1(*(Input.begin() + 1)) {}
		virtual ~COr() {}

		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;

		const _TCHAR* GetVerbalName() override { return _T("Логическое ИЛИ"); }
		static size_t PrimitiveSize() { return sizeof(COr); }
		static long EquationsCount()  { return 1; }
	};

	class CNot : public CDynaPrimitiveBinary
	{
	public:
		CNot(CDevice& Device, VariableIndex& OutputVariable, InputList Input, ExtraOutputList ExtraOutputVariables):
			CDynaPrimitiveBinary(Device, OutputVariable, Input, ExtraOutputVariables) { }
		virtual ~CNot() {}

		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;

		const _TCHAR* GetVerbalName() override { return _T("Логическое НЕ"); }
		static size_t PrimitiveSize() { return sizeof(CNot); }
		static long EquationsCount()  { return 1; }
	};
}
