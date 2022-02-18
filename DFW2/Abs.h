#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CAbs : public CDynaPrimitiveState
	{
	protected:
		bool Positive_ = true;			// текущее состояние
		bool PositiveSaved_ = true;		// cохраненное состояние
	public:

		CAbs(CDevice& Device, const ORange& Output, const IRange& Input) : 
			CDynaPrimitiveState(Device, Output, Input) {}
		CAbs(CDevice& Device, const OutputList& Output, const InputList& Input) : 
			CAbs(Device, ORange(Output), IRange(Input)) { }


		virtual ~CAbs() = default;

		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;


		void BuildEquations(CDynaModel *pDynaModel) override;
		void BuildRightHand(CDynaModel *pDynaModel) override;
		void BuildDerivatives(CDynaModel *pDynaModel) override { }
		double CheckZeroCrossing(CDynaModel *pDynaModel) override;

		const char* GetVerbalName() override { return "Модуль"; }
		static size_t PrimitiveSize() { return sizeof(CAbs); }
		static long EquationsCount()  { return 1; }
		void StoreState() override { PositiveSaved_ = Positive_; }
		void RestoreState() override { Positive_ = PositiveSaved_; }
	};
}