#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CDerlagContinuous : public CDynaPrimitive
	{
	protected:
		double m_K, m_T;
	public:
		double *m_Y2;
		CDerlagContinuous(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) :  
			CDynaPrimitive(pDevice, pOutput, nOutputIndex, Input), m_Y2(pOutput + 1) {}
		virtual ~CDerlagContinuous() {}
		bool Init(CDynaModel *pDynaModel) override;
		bool BuildEquations(CDynaModel *pDynaModel) override;
		bool BuildRightHand(CDynaModel *pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		void SetTK(double T, double K)  { m_K = K;  m_T = T; }
		const _TCHAR* GetVerbalName() override { return _T("РДЗ со сглаживанием"); }
		bool UnserializeParameters(CDynaModel *pDynaModel, double *pParameters, size_t nParametersCount) override;
		static size_t PrimitiveSize() { return sizeof(CDerlagContinuous); }
		static long EquationsCount()  { return 2; }
	};

	class CDerlagNordsieck : public CDynaPrimitive
	{
	protected:
		double m_K, m_T;
	public:
		double *m_Dummy1, *m_Dummy2;
		CDerlagNordsieck(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) :
			CDynaPrimitive(pDevice, pOutput, nOutputIndex, Input), m_Dummy1(pOutput + 1), m_Dummy2(pOutput + 2) {}
		virtual ~CDerlagNordsieck() {}
		bool Init(CDynaModel *pDynaModel) override;
		bool BuildEquations(CDynaModel *pDynaModel) override;
		bool BuildRightHand(CDynaModel *pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override;
		//virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		void SetTK(double T, double K) { m_K = K;  m_T = T; }
		const _TCHAR* GetVerbalName() override { return _T("ДЗ Nordsieck"); }
		bool UnserializeParameters(CDynaModel *pDynaModel, double *pParameters, size_t nParametersCount) override;
		static size_t PrimitiveSize() { return sizeof(CDerlagNordsieck); }
		static long EquationsCount() { return 3; }
	};


#define CDerlag CDerlagContinuous
//#define CDerlag CDerlagNordsieck

}

