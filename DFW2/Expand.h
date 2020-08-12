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
 
		virtual ~CExpand() {}

		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		bool NotifyDelay(CDynaModel *pDynaModel) override;
		bool UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters) override;

		const _TCHAR* GetVerbalName() override { return _T("Расширитель импульса"); }
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

		virtual ~CShrink() {}

		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		bool NotifyDelay(CDynaModel *pDynaModel) override;

		const _TCHAR* GetVerbalName() override { return _T("Ограничитель импульса"); }
		static size_t PrimitiveSize() { return sizeof(CShrink); }
		static long EquationsCount()  { return 1; }
	};
}
