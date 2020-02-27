#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CLimitedLag : public CDynaPrimitiveLimited
	{
	protected:
		double m_T;
		double m_K;
		double OnStateMax(CDynaModel *pDynaModel) override;
		double OnStateMin(CDynaModel *pDynaModel) override;
		double OnStateMid(CDynaModel *pDynaModel) override;
	public:
		CLimitedLag(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : CDynaPrimitiveLimited(pDevice, pOutput, nOutputIndex, Input) {}
		virtual ~CLimitedLag() {}
		void SetMinMaxTK(CDynaModel *pDynaModel, double dMin, double dMax, double T, double K);
		void ChangeMinMaxTK(CDynaModel *pDynaModel, double dMin, double dMax, double T, double K);
		void ChangeTimeConstant(double TexcNew);
		bool Init(CDynaModel *pDynaModel) override;
		bool BuildEquations(CDynaModel *pDynaModel) override;
		bool BuildRightHand(CDynaModel *pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		const _TCHAR* GetVerbalName() override { return _T("Апериодическое звено с ограничениями"); }
		bool UnserializeParameters(CDynaModel *pDynaModel, double *pParameters, size_t nParametersCount) override;

		static size_t PrimitiveSize() { return sizeof(CLimitedLag); }
		static long EquationsCount()  { return 1; }
	};
}
