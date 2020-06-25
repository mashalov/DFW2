#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CAnd: public CDynaPrimitiveBinary
	{
	protected:
		PrimitiveVariableBase *m_Input2;
	public:
		CAnd(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, std::initializer_list<PrimitiveVariableBase*> Input);
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
		COr(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, std::initializer_list<PrimitiveVariableBase*> Input);
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
		CNot(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, std::initializer_list<PrimitiveVariableBase*> Input);
		virtual ~CNot() {}

		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;

		const _TCHAR* GetVerbalName() override { return _T("Логическое НЕ"); }
		static size_t PrimitiveSize() { return sizeof(CNot); }
		static long EquationsCount()  { return 1; }
	};
}
