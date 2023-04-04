#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CLeadLag : public CDynaPrimitive
	{
	protected:
		double T1_ = 1.0;
		double T2_ = 1.0;
		VariableIndex& Y2_;
	public:

		CLeadLag(CDevice& Device, const ORange& Output, const IRange& Input) :
			CDynaPrimitive(Device, Output, Input), Y2_(Output[1]) {}
		CLeadLag(CDevice& Device, const OutputList& Output, const InputList& Input) :
			CLeadLag(Device, ORange(Output), IRange(Input)) { }

		virtual ~CLeadLag() = default;
		bool ChangeTimeConstants(double T1,  double T2);
		bool Init(CDynaModel* pDynaModel) override;
		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		void BuildDerivatives(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		const char* GetVerbalName() override { return "Инцерционно-форсирующее звено"; }
		bool UnserializeParameters(CDynaModel* pDynaModel, const DOUBLEVECTOR& Parameters) override;

		static size_t PrimitiveSize() { return sizeof(CLeadLag); }
		static long EquationsCount() { return 2; }
	};
}
#pragma once
