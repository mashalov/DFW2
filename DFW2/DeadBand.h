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

		eDFW2DEADBANDSTATES	m_eDbState = eDFW2DEADBANDSTATES::DBS_ZERO;
		eDFW2DEADBANDSTATES m_eDbStateSaved = eDFW2DEADBANDSTATES::DBS_ZERO;

		double m_Db = 0.0;
		double m_DbMax = 0.0;
		double m_DbMin = 0.0;

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
		void StoreState() override { m_eDbStateSaved = m_eDbState; }
		void RestoreState() override { m_eDbState = m_eDbStateSaved; }
	};
}