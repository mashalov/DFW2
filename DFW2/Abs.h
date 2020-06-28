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
		CAbs(CDevice *pDevice, VariableIndex& OutputVariable, InputList Input, ExtraOutputList ExtraOutputVariables) :
			CDynaPrimitiveState(pDevice, OutputVariable, Input, ExtraOutputVariables) {}
		virtual ~CAbs() {}

		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;


		bool BuildEquations(CDynaModel *pDynaModel) override;
		bool BuildRightHand(CDynaModel *pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override { return true; }
		double CheckZeroCrossing(CDynaModel *pDynaModel) override;

		const _TCHAR* GetVerbalName() override { return _T("Модуль"); }
		static size_t PrimitiveSize() { return sizeof(CAbs); }
		static long EquationsCount()  { return 1; }
		void StoreState() override { m_bPositiveSaved = m_bPositive; }
		void RestoreState() override { m_bPositive = m_bPositiveSaved; }
	};
}