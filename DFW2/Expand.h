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
 
		CExpand::CExpand(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, std::initializer_list<PrimitiveVariableBase*> Input) :  
			CDynaPrimitiveBinary(pDevice, pOutput, nOutputIndex, Input)  { }
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
		CShrink::CShrink(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, std::initializer_list<PrimitiveVariableBase*> Input) : 
			CExpand(pDevice, pOutput, nOutputIndex, Input)  { }
		virtual ~CShrink() {}

		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		bool NotifyDelay(CDynaModel *pDynaModel) override;

		const _TCHAR* GetVerbalName() override { return _T("Ограничитель импульса"); }
		static size_t PrimitiveSize() { return sizeof(CShrink); }
		static long EquationsCount()  { return 1; }
	};
}
