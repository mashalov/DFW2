#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CDeadBand : public CDynaPrimitiveState
	{
	protected:
		enum DFW2DEADBANDSTATES
		{
			DBS_ZERO,
			DBS_MAX,
			DBS_MIN,
			DBS_OFF
		}
			m_eDbState, m_eDbStateSaved;

		double m_Db;

		double m_DbMax;
		double m_DbMin;

		double OnStateMin(CDynaModel *pDynaModel);
		double OnStateMax(CDynaModel *pDynaModel);
		double OnStateZero(CDynaModel *pDynaModel);

		void SetCurrentState(DFW2DEADBANDSTATES CurrentState);

	public:


		CDeadBand(CDevice& Device, const ORange& Output, const IRange& Input) : 
			CDynaPrimitiveState(Device, Output, Input) {}
		CDeadBand(CDevice& Device, const OutputList& Output, const InputList& Input) : 
			CDeadBand(Device, ORange(Output), IRange(Input)) { }

		virtual ~CDeadBand() {}

		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
	
		bool BuildEquations(CDynaModel *pDynaModel) override;
		bool BuildRightHand(CDynaModel *pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override { return true; }
		double CheckZeroCrossing(CDynaModel *pDynaModel) override;
		bool UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters) override;

		const _TCHAR* GetVerbalName() override { return _T("Мертвая зона"); }
		static size_t PrimitiveSize() { return sizeof(CDeadBand); }
		static long EquationsCount()  { return 1; }
		void StoreState() override { m_eDbStateSaved = m_eDbState; }
		void RestoreState() override { m_eDbState = m_eDbStateSaved; }
	};
}