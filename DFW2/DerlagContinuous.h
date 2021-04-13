#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	// реально-дифференцирующее звено
	class CDerlagContinuous : public CDynaPrimitive
	{
	protected:
		double m_K, m_T;
		VariableIndex& m_Y2;
	public:
		// этот примитив пример примитива с двумя выходами. Второй выход соответсвует дополнительному второму алгебраическому уравнению

		CDerlagContinuous(CDevice& Device, const ORange& Output, const IRange& Input) :
			CDynaPrimitive(Device, Output, Input), m_Y2(Output[1]) {}
		CDerlagContinuous(CDevice& Device, const OutputList& Output, const InputList& Input) : 
			CDerlagContinuous(Device, ORange(Output), IRange(Input)) { }

		virtual ~CDerlagContinuous() = default;
		bool Init(CDynaModel *pDynaModel) override;
		bool BuildEquations(CDynaModel *pDynaModel) override;
		bool BuildRightHand(CDynaModel *pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		void SetTK(double T, double K)  { m_K = K;  m_T = T; }
		const char* GetVerbalName() override { return "РДЗ со сглаживанием"; }
		bool UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters) override;
		static size_t PrimitiveSize() { return sizeof(CDerlagContinuous); }
		static long EquationsCount()  { return 2; }
	};

	/*
	class CDerlagNordsieck : public CDynaPrimitive
	{
	protected:
		double m_K, m_T;
	public:
		VariableIndex 
		CDerlagNordsieck(CDevice *pDevice, VariableIndex& OutputVariable, std::initializer_list<PrimitiveVariableBase*> Input) :
			CDynaPrimitive(pDevice, OutputVariable, Input), m_Dummy1(pOutput + 1), m_Dummy2(pOutput + 2) {}
		virtual ~CDerlagNordsieck() {}
		bool Init(CDynaModel *pDynaModel) override;
		bool BuildEquations(CDynaModel *pDynaModel) override;
		bool BuildRightHand(CDynaModel *pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override;
		//virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		void SetTK(double T, double K) { m_K = K;  m_T = T; }
		const char* GetVerbalName() override { return "ДЗ Nordsieck"; }
		bool UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters) override;
		static size_t PrimitiveSize() { return sizeof(CDerlagNordsieck); }
		static long EquationsCount() { return 3; }
	};
	*/


#define CDerlag CDerlagContinuous
//#define CDerlag CDerlagNordsieck

}

