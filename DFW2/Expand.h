#pragma once
#include "DynaPrimitive.h"
#include "Relay.h"

namespace DFW2
{
	class CExpand : public CDynaPrimitiveBinary, public CDiscreteDelay
	{
	protected:
		virtual void RequestZCDiscontinuity(CDynaModel* pDynaModel);
		virtual void SetCurrentState(CDynaModel *pDynaModel, eRELAYSTATES CurrentState);
		eRELAYSTATES GetInstantState();
	public:
 
		CExpand::CExpand(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) :  CDynaPrimitiveBinary(pDevice, pOutput, nOutputIndex, Input)  { }
		virtual ~CExpand() {}

		virtual bool Init(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		virtual bool NotifyDelay(CDynaModel *pDynaModel);
		virtual bool UnserializeParameters(CDynaModel *pDynaModel, double *pParameters, size_t nParametersCount);

		virtual const _TCHAR* GetVerbalName() { return _T("Расширитель импульса"); }
		static size_t PrimitiveSize() { return sizeof(CExpand); }
		static long EquationsCount()  { return 1; }
	};


	class CShrink : public CExpand
	{
	protected:
		virtual void SetCurrentState(CDynaModel *pDynaModel, eRELAYSTATES CurrentState);

	public:
		CShrink::CShrink(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : CExpand(pDevice, pOutput, nOutputIndex, Input)  { }
		virtual ~CShrink() {}

		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		virtual bool NotifyDelay(CDynaModel *pDynaModel);

		virtual const _TCHAR* GetVerbalName() { return _T("Ограничитель импульса"); }
		static size_t PrimitiveSize() { return sizeof(CShrink); }
		static long EquationsCount()  { return 1; }
	};
}
