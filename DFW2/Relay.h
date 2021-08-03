#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CRelay : public CDynaPrimitiveBinaryOutput
	{
	protected:
		double m_dUpper = 0.0;
		double m_dLower = 0.0;
		double m_dUpperH = 0.0;
		double m_dLowerH = 0.0;
		bool m_bMaxRelay = true;

	protected:
		inline eRELAYSTATES GetCurrentState() override { return eCurrentState; }
		double OnStateOn(CDynaModel *pDynaModel) override;
		double OnStateOff(CDynaModel *pDynaModel) override;
		eRELAYSTATES GetInstantState();
	public:
		CRelay(CDevice& Device, ORange Output, IRange Input) : 
			CDynaPrimitiveBinaryOutput(Device, Output, Input) {}
		CRelay(CDevice& Device, const OutputList& Output, const InputList& Input) : CRelay(Device, ORange(Output), IRange(Input)) { }

		virtual ~CRelay() = default;
		void SetRefs(CDynaModel *pDynaModel, double dUpper, double dLower, bool MaxRelay);
		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		const char* GetVerbalName() noexcept override { return "Реле"; }
		bool UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters) override;
		static size_t PrimitiveSize() noexcept { return sizeof(CRelay); }
		static long EquationsCount()  noexcept { return 1; }
	};

	class CDiscreteDelay
	{
	protected:
		double m_dDelay = 0.0;
		ptrdiff_t m_nDiscontinuityId;
	public:
		CDiscreteDelay() : m_nDiscontinuityId(0) {}
		virtual bool NotifyDelay(CDynaModel *pDynaModel) = 0;
		virtual CDevice* GetDevice() { return nullptr; };
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

		CRelayDelay(CDevice& Device, ORange Output, IRange Input) : CRelay(Device, Output, Input), CDiscreteDelay() {}
		CRelayDelay(CDevice& Device, const OutputList& Output, const InputList& Input) : CRelayDelay(Device, ORange(Output), IRange(Input)) { }

		bool Init(CDynaModel *pDynaModel) override;
		void SetRefs(CDynaModel *pDynaModel, double dUpper, double dLower, bool MaxRelay, double dDelay);
		bool NotifyDelay(CDynaModel *pDynaModel) override;
		CDevice* GetDevice() override { return &m_Device; };
		bool UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		static size_t PrimitiveSize() noexcept { return sizeof(CRelayDelay); }
		static long EquationsCount()  noexcept { return 1; }
	};

	class CRelayDelayLogic : public CRelayDelay
	{
	public:

		CRelayDelayLogic(CDevice& Device, const ORange& Output, const IRange& Input) : 
			CRelayDelay(Device, Output, Input){}
		CRelayDelayLogic(CDevice& Device, const OutputList& Output, const InputList& Input) : 
			CRelayDelayLogic(Device, ORange(Output), IRange(Input)) { }

		bool Init(CDynaModel *pDynaModel) override;
		bool NotifyDelay(CDynaModel *pDynaModel) override;
		static size_t PrimitiveSize() noexcept { return sizeof(CRelayDelayLogic); }
	};
}

