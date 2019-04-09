#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CLimitedLag : public CDynaPrimitiveLimited
	{
	protected:
		double m_T;
		double m_K;
		virtual double OnStateMax(CDynaModel *pDynaModel);
		virtual double OnStateMin(CDynaModel *pDynaModel);
		virtual double OnStateMid(CDynaModel *pDynaModel);
	public:
		CLimitedLag(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : CDynaPrimitiveLimited(pDevice, pOutput, nOutputIndex, Input) {}
		virtual ~CLimitedLag() {}
		void SetMinMaxTK(CDynaModel *pDynaModel, double dMin, double dMax, double T, double K);
		void ChangeMinMaxTK(CDynaModel *pDynaModel, double dMin, double dMax, double T, double K);
		void ChangeTimeConstant(double TexcNew);
		virtual bool Init(CDynaModel *pDynaModel);
		virtual bool BuildEquations(CDynaModel *pDynaModel);
		virtual bool BuildRightHand(CDynaModel *pDynaModel);
		virtual bool BuildDerivatives(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		virtual const _TCHAR* GetVerbalName() { return _T("Апериодическое звено с ограничениями"); }
		virtual bool UnserializeParameters(CDynaModel *pDynaModel, double *pParameters, size_t nParametersCount);

		static size_t PrimitiveSize() { return sizeof(CLimitedLag); }
		static long EquationsCount()  { return 1; }
	};
}
