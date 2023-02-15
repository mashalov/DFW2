#pragma once
#include "DynaBranch.h"

namespace DFW2 
{
	// Блок расчета параметров потоков по ветви. Добавляется в Якоби для ветви,
	// по которой необходимы эти параметры
	class CDynaBranchMeasure : public CDevice
	{
	protected:
		CDynaBranch* pBranch_ = nullptr;			// ветвь, для которой выполняеются расчеты потоков
		CDynaNodeBase* pZeroLFNode_ = nullptr;		// суперузел, в который входит ветвь с нулевым сопротивлением
	public:
		enum VARS
		{
			V_IBRE,						// Ток в прямоугольных координатах в начале
			V_IBIM,
			V_IERE,						// Ток в прямоугольных координатах в конце
			V_IEIM,
			V_IB,						// Модуль тока в начале
			V_IE,						// Модуль тока в конце
			V_PB,						// Активная и реактивная мощности в начале
			V_QB,
			V_PBR,						// Активная и реактивная мощности в начале 
			V_QBR,						// с положительным направлением РЗА
			V_PE,						// Активная и реактивная мощности в конце
			V_QE,
			V_SB,						// Полные мощности в начале и в конце 
			V_SE,
			V_LAST
		};

		VariableIndex Ibre, Ibim, Iere, Ieim, Ib, Ie, Pb, Qb, Pe, Qe, Sb, Se;
		// перетоки в положительном направлении РЗА
		VariableIndex Pbr, Qbr; 
		CDynaBranchMeasure() : CDevice() {}
		void SetBranch(CDynaBranch* pBranch);
		void TopologyUpdated();


		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;

		static void DeviceProperties(CDeviceContainerProperties& properties);
		static void CalculateFlows(const CDynaBranch* pBranch, cplx& cIb, cplx& cIe, cplx& cSb, cplx& cSe);

		static constexpr const char* m_cszIbre = "Ibre";
		static constexpr const char* m_cszIbim = "Ibim";
		static constexpr const char* m_cszIere = "Iere";
		static constexpr const char* m_cszIeim = "Ieim";
		static constexpr const char* m_cszIb   = "Ib";
		static constexpr const char* m_cszIe   = "Ie";
		static constexpr const char* m_cszPb   = "Pb";
		static constexpr const char* m_cszQb   = "Qb";
		static constexpr const char* m_cszPbr  = "Pbr";
		static constexpr const char* m_cszQbr  = "Qbr";
		static constexpr const char* m_cszPe   = "Pe";
		static constexpr const char* m_cszQe   = "Qe";
		static constexpr const char* m_cszSb   = "Sb";
		static constexpr const char* m_cszSe   = "Se";
	};
}
