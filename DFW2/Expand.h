#pragma once
#include "DynaPrimitive.h"
#include "Relay.h"

namespace DFW2
{
	class CExpand : public CDynaPrimitiveBinary, public CDiscreteDelay
	{
	protected:
		void RequestZCDiscontinuity(CDynaModel* pDynaModel)  override;
		void SetCurrentState(CDynaModel *pDynaModel, eRELAYSTATES CurrentState)  override;
		eRELAYSTATES GetInstantState();
	public:

		CExpand(CDevice& Device, const ORange& Output, const IRange& Input) : 
			CDynaPrimitiveBinary(Device, Output, Input) {}
		CExpand(CDevice& Device, const OutputList& Output, const InputList& Input) : 
			CExpand(Device, ORange(Output), IRange(Input)) { }
 
		virtual ~CExpand() = default;

		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		bool NotifyDelay(CDynaModel *pDynaModel) override;
		bool UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters) override;

		const char* GetVerbalName() override { return "Расширитель импульса"; }
		static size_t PrimitiveSize() { return sizeof(CExpand); }
		static long EquationsCount()  { return 1; }
	};


	class CShrink : public CExpand
	{
	protected:
		void SetCurrentState(CDynaModel *pDynaModel, eRELAYSTATES CurrentState) override;

	public:

		CShrink(CDevice& Device, ORange Output, IRange Input) : CExpand(Device, Output, Input) {}
		CShrink(CDevice& Device, const OutputList& Output, const InputList& Input) : CShrink(Device, ORange(Output), IRange(Input))  { }

		virtual ~CShrink() = default;

		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		bool NotifyDelay(CDynaModel *pDynaModel) override;

		const char* GetVerbalName() override { return "Ограничитель импульса"; }
		static size_t PrimitiveSize() { return sizeof(CShrink); }
		static long EquationsCount()  { return 1; }
	};
}
