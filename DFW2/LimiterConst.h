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
		CLimiterConst(CDevice& Device, VariableIndex& OutputVariable, InputList Input, ExtraOutputList ExtraOutputVariables = {}) :
			CDynaPrimitiveLimited(Device, OutputVariable, Input, ExtraOutputVariables) {}
		virtual ~CLimiterConst() {}
		bool Init(CDynaModel *pDynaModel) override;
		bool BuildEquations(CDynaModel *pDynaModel) override;
		bool BuildRightHand(CDynaModel *pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override { return true; }
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		const _TCHAR* GetVerbalName() override { return _T("Ограничитель"); }
		bool UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters) override;
		static size_t PrimitiveSize() { return sizeof(CLimiterConst); }
		static long EquationsCount()  { return 1; }
	};
}
