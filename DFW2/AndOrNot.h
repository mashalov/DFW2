#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CAnd: public CDynaPrimitiveBinary
	{
	protected:
		PrimitiveVariableBase *m_Input2;
	public:
		CAnd(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input1, PrimitiveVariableBase* Input2);
		virtual ~CAnd() {}

		virtual bool Init(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);

		virtual const _TCHAR* GetVerbalName() { return _T("���������� �"); }
		static size_t PrimitiveSize() { return sizeof(CAnd); }
		static long EquationsCount()  { return 1; }
	};

	class COr : public CDynaPrimitiveBinary
	{
	protected:
		PrimitiveVariableBase *m_Input2;
	public:
		COr(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input1, PrimitiveVariableBase* Input2);
		virtual ~COr() {}

		virtual bool Init(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);

		virtual const _TCHAR* GetVerbalName() { return _T("���������� ���"); }
		static size_t PrimitiveSize() { return sizeof(COr); }
		static long EquationsCount()  { return 1; }
	};

	class CNot : public CDynaPrimitiveBinary
	{
	public:
		CNot(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input);
		virtual ~CNot() {}

		virtual bool Init(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);

		virtual const _TCHAR* GetVerbalName() { return _T("���������� ��"); }
		static size_t PrimitiveSize() { return sizeof(CNot); }
		static long EquationsCount()  { return 1; }
	};
}
