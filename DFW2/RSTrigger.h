#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	class CRSTrigger : public CDynaPrimitiveBinary
	{
	protected:
		PrimitiveVariableBase* m_Input1;
		bool m_bResetPriority;
	public:

		CRSTrigger::CRSTrigger(CDevice *pDevice, 
							   double* pOutput, 
							   ptrdiff_t nOutputIndex, 
							   std::initializer_list<PrimitiveVariableBase*> Input) : 
		  							CDynaPrimitiveBinary(pDevice, pOutput, nOutputIndex, Input)
									{ 
										InitializeInputs({&m_Input, &m_Input1}, Input);
									}
		virtual ~CRSTrigger() {}

		bool Init(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;

		const _TCHAR* GetVerbalName() noexcept  override { return _T("RS-�������"); }
		static size_t PrimitiveSize() noexcept { return sizeof(CRSTrigger); }
		static long EquationsCount()  noexcept { return 1; }
	};
}

