#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CAbs : public CDynaPrimitiveState
	{
	protected:
		bool m_bPositive;		// текущее состояние
		bool m_bPositiveSaved;	// cохраненное состояние
	public:
		CAbs(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input);
		virtual ~CAbs() {}

		virtual bool Init(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);


		virtual bool BuildEquations(CDynaModel *pDynaModel);
		virtual bool BuildRightHand(CDynaModel *pDynaModel);
		virtual bool BuildDerivatives(CDynaModel *pDynaModel) { return true; }
		virtual double CheckZeroCrossing(CDynaModel *pDynaModel);

		virtual const _TCHAR* GetVerbalName() { return _T("Модуль"); }
		static size_t PrimitiveSize() { return sizeof(CAbs); }
		static long EquationsCount()  { return 1; }
		virtual void StoreState() override { m_bPositiveSaved = m_bPositive; }
		virtual void RestoreState() override { m_bPositive = m_bPositiveSaved; }
	};
}