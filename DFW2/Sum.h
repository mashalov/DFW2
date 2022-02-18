#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	// примитив сумма двух переменных
	class CSum : public CDynaPrimitive
	{
		double K1_, K2_;
		InputVariable Input1_;
	public:
		// данный примитив пример примитива с несколькими входами
		// первый вход инициализируется в базовом классе, второй - в конструкторе 

		CSum(CDevice& Device, const ORange& Output, const IRange& Input) :
			CDynaPrimitive(Device, Output, Input), Input1_(Input[1]), K1_(1.0), K2_(1.0) {}
		CSum(CDevice& Device, const OutputList& Output, const InputList& Input) :
			CSum(Device, ORange(Output), IRange(Input)) { }


		virtual ~CSum() = default;
		void SetK(double K1, double K2) { K1_ = K1; K2_ = K2; }
		void BuildEquations(CDynaModel *pDynaModel) override;
		void BuildRightHand(CDynaModel *pDynaModel) override;
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
		const char* GetVerbalName() noexcept override { return "Сумматор"; }
	};
}
