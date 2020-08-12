#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CAnd: public CDynaPrimitiveBinary
	{
	protected:
		InputVariable m_Input1;
	public:
		CAnd(CDevice& Device, const ORange& Output, const IRange& Input) : 
			CDynaPrimitiveBinary(Device, Output, Input), m_Input1(Input[1]) {}
		CAnd(CDevice& Device, const OutputList& Output, const InputList& Input) : 
			CAnd(Device, ORange(Output), IRange(Input)) { }

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

		COr(CDevice& Device, const ORange& Output, const IRange& Input) : 
			CDynaPrimitiveBinary(Device, Output, Input), m_Input1(Input[1]) {}
		COr(CDevice& Device, const OutputList& Output, const InputList& Input) : 
			COr(Device, ORange(Output), IRange(Input)) { }
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
		CNot(CDevice& Device, const ORange& Output, const IRange& Input) : 
			CDynaPrimitiveBinary(Device, Output, Input) {}
		CNot(CDevice& Device, const OutputList& Output, const InputList& Input) : 
			CNot(Device, ORange(Output), IRange(Input)) { }

		virtual ~CNot() {}

		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;

		const _TCHAR* GetVerbalName() override { return _T("Логическое НЕ"); }
		static size_t PrimitiveSize() { return sizeof(CNot); }
		static long EquationsCount()  { return 1; }
	};
}
