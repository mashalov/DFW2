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

		virtual ~CAnd() = default;

		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;

		const char* GetVerbalName() override { return "Логическое И"; }
		static size_t PrimitiveSize() { return sizeof(CAnd); }
		static long EquationsCount()  { return 1; }
	};

	// OR может иметь любое количество входов, поэтому
	// наследуем его от MultiInput
	class COr : public CDynaPrimitiveBinary, CDynaPrimitiveMultiInput
	{
	public:

		COr(CDevice& Device, const ORange& Output, const IRange& Input) :
			CDynaPrimitiveBinary(Device, Output, Input), CDynaPrimitiveMultiInput(Input) {}
		COr(CDevice& Device, const OutputList& Output, const InputList& Input) : 
			COr(Device, ORange(Output), IRange(Input)) { }
		virtual ~COr() = default;

		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;

		const char* GetVerbalName() override { return "Логическое ИЛИ"; }
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

		virtual ~CNot() = default;

		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;

		const char* GetVerbalName() override { return "Логическое НЕ"; }
		static size_t PrimitiveSize() { return sizeof(CNot); }
		static long EquationsCount()  { return 1; }
	};
}
