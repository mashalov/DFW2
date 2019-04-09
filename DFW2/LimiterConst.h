#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CLimiterConst : public CDynaPrimitiveLimited
	{
	protected:
		virtual double OnStateMax(CDynaModel *pDynaModel);
		virtual double OnStateMin(CDynaModel *pDynaModel);
		virtual double OnStateMid(CDynaModel *pDynaModel);
	public:
		CLimiterConst(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : CDynaPrimitiveLimited(pDevice, pOutput, nOutputIndex, Input) {}
		virtual ~CLimiterConst() {}
		virtual bool Init(CDynaModel *pDynaModel);
		virtual bool BuildEquations(CDynaModel *pDynaModel);
		virtual bool BuildRightHand(CDynaModel *pDynaModel);
		virtual bool BuildDerivatives(CDynaModel *pDynaModel) { return true; }
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		virtual const _TCHAR* GetVerbalName() { return _T("Ограничитель"); }
		virtual bool UnserializeParameters(CDynaModel *pDynaModel, double *pParameters, size_t nParametersCount);
		static size_t PrimitiveSize() { return sizeof(CLimiterConst); }
		static long EquationsCount()  { return 1; }
	};
}
