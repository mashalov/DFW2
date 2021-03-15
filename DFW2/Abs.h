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

		CAbs(CDevice& Device, const ORange& Output, const IRange& Input) : 
			CDynaPrimitiveState(Device, Output, Input) {}
		CAbs(CDevice& Device, const OutputList& Output, const InputList& Input) : 
			CAbs(Device, ORange(Output), IRange(Input)) { }


		virtual ~CAbs() {}

		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;


		bool BuildEquations(CDynaModel *pDynaModel) override;
		bool BuildRightHand(CDynaModel *pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override { return true; }
		double CheckZeroCrossing(CDynaModel *pDynaModel) override;

		const char* GetVerbalName() override { return "Модуль"; }
		static size_t PrimitiveSize() { return sizeof(CAbs); }
		static long EquationsCount()  { return 1; }
		void StoreState() override { m_bPositiveSaved = m_bPositive; }
		void RestoreState() override { m_bPositive = m_bPositiveSaved; }
	};
}