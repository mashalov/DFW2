#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CLimiterConst : public CDynaPrimitiveLimited
	{
	protected:
		double OnStateMax(CDynaModel *pDynaModel) override;
		double OnStateMin(CDynaModel *pDynaModel) override;
		double OnStateMid(CDynaModel *pDynaModel) override;
	public:

		CLimiterConst(CDevice& Device, const ORange& Output, const IRange& Input) :
			CDynaPrimitiveLimited(Device, Output, Input) {}
		CLimiterConst(CDevice& Device, const OutputList& Output, const InputList& Input) : 
			CLimiterConst(Device, ORange(Output), IRange(Input)) { }

		virtual ~CLimiterConst() = default;
		bool Init(CDynaModel *pDynaModel) override;
		void BuildEquations(CDynaModel *pDynaModel) override;
		void BuildRightHand(CDynaModel *pDynaModel) override;
		void BuildDerivatives(CDynaModel *pDynaModel) override { }
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		const char* GetVerbalName() override { return "Ограничитель"; }
		bool UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters) override;
		static size_t PrimitiveSize() { return sizeof(CLimiterConst); }
		static long EquationsCount()  { return 1; }
	};
}
