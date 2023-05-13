#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CRelay : public CDynaPrimitiveBinaryOutput
	{
	protected:
		double RefOn_ = 0.0;		// уставка на срабатывание
		double RefOff_ = 0.0;		// уставка на возврат
		bool MaxRelay_ = true;		// true - максимальное реле, false - минимальное

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
		// задать уставки и режим работы
		void SetRefs(CDynaModel *pDynaModel, double RefOn, double RefOff, bool MaxRelay);	
		// задать уставки и режим работы реле с максимальным коэффициентом возврата
		void SetRefs(CDynaModel* pDynaModel, double RefOn, bool MaxRelay);					
		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		const char* GetVerbalName() noexcept override { return "Реле"; }
		bool UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters) override;	// десериализация параметров
		static size_t PrimitiveSize() noexcept { return sizeof(CRelay); }
		static long EquationsCount()  noexcept { return 1; }
	};

	class CRelayMin : public CRelay
	{
	public:
		using CRelay::CRelay;
		bool UnserializeParameters(CDynaModel* pDynaModel, const DOUBLEVECTOR& Parameters) override;	// десериализация параметров
		static size_t PrimitiveSize() noexcept { return sizeof(CRelayMin); }
	};

	class CDiscreteDelay
	{
	protected:
		double Delay_ = 0.0;
		ptrdiff_t DiscontinuityId_;
		CDynaPrimitive& Primitive_;	// примитив, который работает с выдержкой времени
	public:
		// для конструктора CDiscreteDelay при использовании множественного
		// наследования с примитивом надо использовать static_cast<CDynaPrimitive*>(this)
		CDiscreteDelay(CDynaPrimitive& Primitive) : Primitive_(Primitive), DiscontinuityId_(0) {}
		virtual bool NotifyDelay(CDynaModel *pDynaModel) = 0;
		bool Init(CDynaModel *pDynaModel);
		void SetDiscontinuityId(ptrdiff_t DiscontinuityId) noexcept { DiscontinuityId_ = DiscontinuityId; }
		ptrdiff_t GetDiscontinuityId() const noexcept { return DiscontinuityId_; }
		CDynaPrimitive& Primitive() { return Primitive_; }
	};

	class CRelayDelay : public CRelay, public CDiscreteDelay
	{
	protected:
		void SetCurrentState(CDynaModel *pDynaModel, eRELAYSTATES CurrentState) override;
		void RequestZCDiscontinuity(CDynaModel* pDynaModel) override;
		// разрешает "мгновенное" переключение выхода для реле с нулевой задержкой
		virtual bool EnableInstantSwitch(CDynaModel* pDynaModel) const;
	public:

		CRelayDelay(CDevice& Device, ORange Output, IRange Input) : 
			CDiscreteDelay(*static_cast<CDynaPrimitive*>(this)), 
			CRelay(Device, Output, Input) {}
		CRelayDelay(CDevice& Device, const OutputList& Output, const InputList& Input) : CRelayDelay(Device, ORange(Output), IRange(Input)) { }

		bool Init(CDynaModel *pDynaModel) override;
		void SetRefs(CDynaModel* pDynaModel, double RefOn, bool RefOff, double Delay);
		void SetRefs(CDynaModel *pDynaModel, double RefOn, double RefOff, bool MaxRelay, double Delay);
		bool NotifyDelay(CDynaModel *pDynaModel) override;
		bool UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		static size_t PrimitiveSize() noexcept { return sizeof(CRelayDelay); }
		static long EquationsCount()  noexcept { return 1; }
	};

	class CRelayMinDelay : public CRelayDelay
	{
	public:
		using CRelayDelay::CRelayDelay;
		bool UnserializeParameters(CDynaModel* pDynaModel, const DOUBLEVECTOR& Parameters) override;
		static size_t PrimitiveSize() noexcept { return sizeof(CRelayMinDelay); }
	};

	class CRelayDelayLogic : public CRelayDelay
	{
	protected:
		bool EnableInstantSwitch(CDynaModel* pDynaModel) const override;
	public:

		CRelayDelayLogic(CDevice& Device, const ORange& Output, const IRange& Input) : 
			CRelayDelay(Device, Output, Input){}
		CRelayDelayLogic(CDevice& Device, const OutputList& Output, const InputList& Input) : 
			CRelayDelayLogic(Device, ORange(Output), IRange(Input)) { }

		bool Init(CDynaModel *pDynaModel) override;
		bool NotifyDelay(CDynaModel *pDynaModel) override;
		static size_t PrimitiveSize() noexcept { return sizeof(CRelayDelayLogic); }
	};

	// минимальное реле логики сценария и автоматики
	class CRelayMinDelayLogic : public CRelayDelayLogic
	{
	public:
		using CRelayDelayLogic::CRelayDelayLogic;
		// для минимального реле после десериализации уставок меняем режим реле
		bool UnserializeParameters(CDynaModel* pDynaModel, const DOUBLEVECTOR& Parameters) override
		{
			const bool bRes{ CRelayDelayLogic::UnserializeParameters(pDynaModel, Parameters) };
			MaxRelay_ = false;
			return bRes;
		}
		static size_t PrimitiveSize() noexcept { return sizeof(CRelayMinDelayLogic); }
	};
}

