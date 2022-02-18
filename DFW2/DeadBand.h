#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CDeadBand : public CDynaPrimitiveState
	{
	protected:
		enum class eDFW2DEADBANDSTATES
		{
			DBS_ZERO,
			DBS_MAX,
			DBS_MIN,
			DBS_OFF
		};

		eDFW2DEADBANDSTATES	eDbState_ = eDFW2DEADBANDSTATES::DBS_ZERO;
		eDFW2DEADBANDSTATES eDbStateSaved_ = eDFW2DEADBANDSTATES::DBS_ZERO;

		double Db_ = 0.0;
		double DbMax_ = 0.0;
		double DbMin_ = 0.0;

		double OnStateMin(CDynaModel *pDynaModel);
		double OnStateMax(CDynaModel *pDynaModel);
		double OnStateZero(CDynaModel *pDynaModel);

		void SetCurrentState(eDFW2DEADBANDSTATES CurrentState);

	public:


		CDeadBand(CDevice& Device, const ORange& Output, const IRange& Input) : 
			CDynaPrimitiveState(Device, Output, Input) {}
		CDeadBand(CDevice& Device, const OutputList& Output, const InputList& Input) : 
			CDeadBand(Device, ORange(Output), IRange(Input)) { }

		virtual ~CDeadBand() = default;

		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
	
		void BuildEquations(CDynaModel *pDynaModel) override;
		void BuildRightHand(CDynaModel *pDynaModel) override;
		void BuildDerivatives(CDynaModel *pDynaModel) override { }
		double CheckZeroCrossing(CDynaModel *pDynaModel) override;
		bool UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters) override;

		const char* GetVerbalName() override { return "Мертвая зона"; }
		static size_t PrimitiveSize() { return sizeof(CDeadBand); }
		static long EquationsCount()  { return 1; }
		void StoreState() override { eDbStateSaved_ = eDbState_; }
		void RestoreState() override { eDbState_ = eDbStateSaved_; }
	};
}