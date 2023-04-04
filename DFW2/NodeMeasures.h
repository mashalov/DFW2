#pragma once
#include "DynaNode.h"

namespace DFW2
{
	// Блок расчета параметров узла, не участвующих в расчетах. Добавляется в Якоби для узла,
	// по которому необходимы эти параметры
	class CDynaNodeMeasure : public CDevice
	{
	protected:
		CDynaNode* pNode_;				// узел, для которой выполняеются расчеты потоков
		eDEVICEFUNCTIONSTATUS  ProcessDiscontinuityImpl(CDynaModel* pDynaModel);
	public:
		enum VARS
		{
			V_PLOAD,					// Мощность нагрузки узла
			V_QLOAD,
			V_LAST
		};

		VariableIndex Pload, Qload;
		CDynaNodeMeasure(CDynaNode* pNode) : CDevice(), pNode_(pNode) {}

		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		static void DeviceProperties(CDeviceContainerProperties& properties);

		static constexpr const char* m_cszPload = "Pn";
		static constexpr const char* m_cszQload = "Qn";
	};

	class CDynaNodeZeroLoadFlow : public CDevice
	{
	protected:
		std::unique_ptr<CDynaNodeBase*[]> m_MatrixRows;
		VariableIndexRefVec m_Vars;
		ptrdiff_t m_nSize;
		const NodeSet& m_ZeroSuperNodes;
	public:
		static void DeviceProperties(CDeviceContainerProperties& properties);
		CDynaNodeZeroLoadFlow(const NodeSet& ZeroLFNodes) : CDevice(), m_ZeroSuperNodes(ZeroLFNodes)
		{
			UpdateSuperNodeSet();
		};
		void UpdateSuperNodeSet();
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		ptrdiff_t EquationsCount() const { return 2 * m_nSize; }
	};

	class CDynaNodeZeroLoadFlowContainer : public CDeviceContainer
	{
	public:
		using CDeviceContainer::CDeviceContainer;
		ptrdiff_t EquationsCount() const override
		{
			if (DevVec.size() != 1)
				throw dfw2error("CDynaNodeZeroLoadFlowContainer::EquationsCount - container must contain exactly 1 device");
			return static_cast<const CDynaNodeZeroLoadFlow*>(DevVec.front())->EquationsCount();
		}
	};
}
