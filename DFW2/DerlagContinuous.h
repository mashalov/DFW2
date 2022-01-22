#pragma once
#include "DynaPrimitive.h"

namespace DFW2
{
	// реально-дифференцирующее звено
	class CDerlagContinuous : public CDynaPrimitive
	{
	protected:
		double m_K = 1.0;
		double m_T = 1.0;
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

	// Версия РДЗ с полным подавлением разрыва на выходе
	// имитирует поведение РДЗ в Мустанг, что совершенно неправильно,
	// но необходимо для совместимости с
	
	class CDerlagContinuousSmooth : public CDerlagContinuous
	{
	public:
		using CDerlagContinuous::CDerlagContinuous;
		virtual ~CDerlagContinuousSmooth() = default;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		const char* GetVerbalName() override { return "РДЗ со сглаживанием без разрыва"; }
	};



	class CDerlagNordsieck : public CDynaPrimitive
	{
	protected:
		double m_K, m_T;
	public:
		VariableIndex& m_Y2;
		CDerlagNordsieck(CDevice& Device, const ORange& Output, const IRange& Input) :
			CDynaPrimitive(Device, Output, Input), m_Y2(Output[1]) {}
		CDerlagNordsieck(CDevice& Device, const OutputList& Output, const InputList& Input) :
			CDerlagNordsieck(Device, ORange(Output), IRange(Input)) { }

		virtual ~CDerlagNordsieck() {}
		bool Init(CDynaModel *pDynaModel) override;
		bool BuildEquations(CDynaModel *pDynaModel) override;
		bool BuildRightHand(CDynaModel *pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		void SetTK(double T, double K) { m_K = K;  m_T = T; }
		const char* GetVerbalName() override { return "ДЗ Nordsieck"; }
		bool UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters) override;
		static size_t PrimitiveSize() { return sizeof(CDerlagNordsieck); }
		static long EquationsCount() { return 3; }
	};

#define DERLAG_CONTINUOUS
//#define DERLAG_CONTINUOUSMOOTH
//#define DERLAG_NORDSIEK

#if defined DERLAG_CONTINUOUS
#define CDerlag CDerlagContinuous
#elif defined DERLAG_CONTINUOUSMOOTH
#define CDerlag CDerlagContinuousSmooth
#elif defined DERLAG_NORDSIEK
#define CDerlag CDerlagNordsieck
#define CDerlagContinuousSmooth CDerlagNordsieck
#endif

}

