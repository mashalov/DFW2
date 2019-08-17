#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CRelay : public CDynaPrimitiveBinaryOutput
	{
	protected:
		double m_dUpper;
		double m_dLower;
		double m_dUpperH;
		double m_dLowerH;
		bool m_bMaxRelay;

	protected:
		inline eRELAYSTATES GetCurrentState() { return eCurrentState; }
		virtual double OnStateOn(CDynaModel *pDynaModel);
		virtual double OnStateOff(CDynaModel *pDynaModel);
		eRELAYSTATES GetInstantState();
		double ChangeState(CDynaModel *pDynaModel, double Check, double OnBound, eRELAYSTATES SetState);
	public:
		CRelay(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : CDynaPrimitiveBinaryOutput(pDevice, pOutput, nOutputIndex, Input) {}
		virtual ~CRelay() {}
		void SetRefs(CDynaModel *pDynaModel, double dUpper, double dLower, bool MaxRelay);
		virtual bool Init(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		virtual const _TCHAR* GetVerbalName() { return _T("Πελε"); }
		virtual bool UnserializeParameters(CDynaModel *pDynaModel, double *pParameters, size_t nParametersCount);
		static size_t PrimitiveSize() { return sizeof(CRelay); }
		static long EquationsCount()  { return 1; }
	};

	class CDiscreteDelay
	{
	protected:
		double m_dDelay;
		ptrdiff_t m_nDiscontinuityId;
	public:
		CDiscreteDelay() : m_nDiscontinuityId(0) {}
		virtual bool NotifyDelay(CDynaModel *pDynaModel) = 0;
		bool Init(CDynaModel *pDynaModel);
		void SetDiscontinuityId(ptrdiff_t nDiscontinuityId) { m_nDiscontinuityId = nDiscontinuityId; }
		ptrdiff_t GetDiscontinuityId() const { return m_nDiscontinuityId; }
	};

	class CRelayDelay : public CRelay, public CDiscreteDelay
	{
	protected:
		virtual void SetCurrentState(CDynaModel *pDynaModel, eRELAYSTATES CurrentState);
		virtual void RequestZCDiscontinuity(CDynaModel* pDynaModel);
	public:
		CRelayDelay(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : CRelay(pDevice, pOutput, nOutputIndex, Input), CDiscreteDelay() {}
		virtual bool Init(CDynaModel *pDynaModel);
		void SetRefs(CDynaModel *pDynaModel, double dUpper, double dLower, bool MaxRelay, double dDelay);
		virtual bool NotifyDelay(CDynaModel *pDynaModel);
		virtual bool UnserializeParameters(CDynaModel *pDynaModel, double *pParameters, size_t nParametersCount);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		static size_t PrimitiveSize() { return sizeof(CRelayDelay); }
		static long EquationsCount()  { return 1; }
	};

	class CRelayDelayLogic : public CRelayDelay
	{
	public:
		CRelayDelayLogic(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : CRelayDelay(pDevice, pOutput, nOutputIndex, Input) {}
		virtual bool Init(CDynaModel *pDynaModel);
		virtual bool NotifyDelay(CDynaModel *pDynaModel);
		static size_t PrimitiveSize() { return sizeof(CRelayDelayLogic); }
	};
}

