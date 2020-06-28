#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CAnd: public CDynaPrimitiveBinary
	{
	protected:
		PrimitiveVariableBase *m_Input2;
	public:
		CAnd(CDevice *pDevice, VariableIndex& OutputVariable, InputList Input, ExtraOutputList ExtraOutputVariables) :
			CDynaPrimitiveBinary(pDevice, OutputVariable, Input, ExtraOutputVariables)
			{
				InitializeInputs({ &m_Input, &m_Input2 }, Input);
			}
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
		PrimitiveVariableBase *m_Input2;
	public:
		COr(CDevice *pDevice, VariableIndex& OutputVariable, InputList Input, ExtraOutputList ExtraOutputVariables) :
			CDynaPrimitiveBinary(pDevice, OutputVariable, Input, ExtraOutputVariables)
			{
				InitializeInputs({ &m_Input, &m_Input2 }, Input);
			}
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
		CNot(CDevice *pDevice, VariableIndex& OutputVariable, InputList Input, ExtraOutputList ExtraOutputVariables):
			CDynaPrimitiveBinary(pDevice, OutputVariable, Input, ExtraOutputVariables) { }
		virtual ~CNot() {}

		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;

		const _TCHAR* GetVerbalName() override { return _T("Логическое НЕ"); }
		static size_t PrimitiveSize() { return sizeof(CNot); }
		static long EquationsCount()  { return 1; }
	};
}
