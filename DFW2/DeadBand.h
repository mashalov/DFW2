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
		CDeadBand(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input);
		virtual ~CDeadBand() {}

		virtual bool Init(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
	
		virtual bool BuildEquations(CDynaModel *pDynaModel);
		virtual bool BuildRightHand(CDynaModel *pDynaModel);
		virtual bool BuildDerivatives(CDynaModel *pDynaModel) { return true; }
		virtual double CheckZeroCrossing(CDynaModel *pDynaModel);
		virtual bool UnserializeParameters(CDynaModel *pDynaModel, double *pParameters, size_t nParametersCount);

		virtual const _TCHAR* GetVerbalName() { return _T("Мертвая зона"); }
		static size_t PrimitiveSize() { return sizeof(CDeadBand); }
		static long EquationsCount()  { return 1; }
		virtual void StoreState() override { m_eDbStateSaved = m_eDbState; }
		virtual void RestoreState() override { m_eDbState = m_eDbStateSaved; }
	};
}