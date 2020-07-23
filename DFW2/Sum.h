#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	// примитив сумма двух переменных
	class CSum : public CDynaPrimitive
	{
		double m_K1, m_K2;
		InputVariable m_Input1;
	public:
		// данный примитив пример примитива с несколькими входами
		// первый вход инициализируется в базовом классе, второй - в конструкторе 
		CSum(CDevice& Device, VariableIndex& OutputVariable, InputList Input, ExtraOutputList ExtraOutputVariables = {}) :
			CDynaPrimitive(Device, OutputVariable, Input), m_Input1(*(Input.begin() + 1)), m_K1(1.0), m_K2(1.0) { }
		virtual ~CSum() {}
		void SetK(double K1, double K2) { m_K1 = K1; m_K2 = K2; }
		bool BuildEquations(CDynaModel *pDynaModel) override;
		bool BuildRightHand(CDynaModel *pDynaModel) override;
		/*
		PrimitiveVariableBase& Input(ptrdiff_t nIndex) override
		{ 
			switch (nIndex)
			{
			case 1:
				return *m_Input1;
			}
			return CDynaPrimitive::Input(0);
		}
		*/
		const _TCHAR* GetVerbalName() noexcept override { return _T("Сумматор"); }
	};
}
