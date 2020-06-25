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
		inline eRELAYSTATES GetCurrentState() override { return eCurrentState; }
		double OnStateOn(CDynaModel *pDynaModel) override;
		double OnStateOff(CDynaModel *pDynaModel) override;
		eRELAYSTATES GetInstantState();
	public:
		CRelay(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, std::initializer_list<PrimitiveVariableBase*> Input) : 
			CDynaPrimitiveBinaryOutput(pDevice, pOutput, nOutputIndex, Input) {}
		virtual ~CRelay() {}
		void SetRefs(CDynaModel *pDynaModel, double dUpper, double dLower, bool MaxRelay);
		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		const _TCHAR* GetVerbalName() noexcept override { return _T("����"); }
		bool UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters) override;
		static size_t PrimitiveSize() noexcept { return sizeof(CRelay); }
		static long EquationsCount()  noexcept { return 1; }
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
		void SetDiscontinuityId(ptrdiff_t nDiscontinuityId) noexcept { m_nDiscontinuityId = nDiscontinuityId; }
		ptrdiff_t GetDiscontinuityId() const noexcept { return m_nDiscontinuityId; }
	};

	class CRelayDelay : public CRelay, public CDiscreteDelay
	{
	protected:
		void SetCurrentState(CDynaModel *pDynaModel, eRELAYSTATES CurrentState) override;
		void RequestZCDiscontinuity(CDynaModel* pDynaModel) override;
	public:
		CRelayDelay(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, std::initializer_list<PrimitiveVariableBase*> Input) : 
			CRelay(pDevice, pOutput, nOutputIndex, Input), CDiscreteDelay() {}
		bool Init(CDynaModel *pDynaModel) override;
		void SetRefs(CDynaModel *pDynaModel, double dUpper, double dLower, bool MaxRelay, double dDelay);
		bool NotifyDelay(CDynaModel *pDynaModel) override;
		bool UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		static size_t PrimitiveSize() noexcept { return sizeof(CRelayDelay); }
		static long EquationsCount()  noexcept { return 1; }
	};

	class CRelayDelayLogic : public CRelayDelay
	{
	public:
		CRelayDelayLogic(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, std::initializer_list<PrimitiveVariableBase*> Input) : 
			CRelayDelay(pDevice, pOutput, nOutputIndex, Input) {}
		bool Init(CDynaModel *pDynaModel) override;
		bool NotifyDelay(CDynaModel *pDynaModel) override;
		static size_t PrimitiveSize() noexcept { return sizeof(CRelayDelayLogic); }
	};
}

