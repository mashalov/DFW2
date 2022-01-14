#pragma once
#include "DynaNode.h"

namespace DFW2
{
	// Блок расчета параметров узла, не участвующих в расчетах. Добавляется в Якоби для узла,
	// по которому необходимы эти параметры
	class CDynaNodeMeasure : public CDevice
	{
	protected:
		CDynaNode* m_pNode;				// ветвь, для которой выполняеются расчеты потоков
		eDEVICEFUNCTIONSTATUS  ProcessDiscontinuityImpl(CDynaModel* pDynaModel);
	public:
		enum VARS
		{
			V_PLOAD,					// Мощность нагрузки узла
			V_QLOAD,
			V_LAST
		};

		VariableIndex Pload, Qload;
		CDynaNodeMeasure(CDynaNode* pNode) : CDevice(), m_pNode(pNode) {}

		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		bool BuildEquations(CDynaModel* pDynaModel) override;
		bool BuildRightHand(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;

		static void DeviceProperties(CDeviceContainerProperties& properties);

		static constexpr const char* m_cszPload = "Pn";
		static constexpr const char* m_cszQload = "Qn";
	};

	class CDynaNodeZeroLoadFlow : public CDevice
	{
		struct MatrixRow
		{
			CDynaNodeBase* pNode;
			VariableIndex Vre, Vim;
		};

	protected:
		std::unique_ptr<MatrixRow[]> m_MatrixRows;
		VariableIndexRefVec m_Vars;
		ptrdiff_t m_nSize;
	public:
		static void DeviceProperties(CDeviceContainerProperties& properties);
		CDynaNodeZeroLoadFlow(const NodeSet& ZeroLFNodes) : CDevice()  { UpdateSuperNodeSet(ZeroLFNodes); };
		void UpdateSuperNodeSet(const NodeSet& ZeroLFNodes);
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		bool BuildEquations(CDynaModel* pDynaModel) override;
		bool BuildRightHand(CDynaModel* pDynaModel) override;
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
			if (m_DevVec.size() != 1)
				throw dfw2error("CDynaNodeZeroLoadFlowContainer::EquationsCount - container must contain exactly 1 device");
			return static_cast<const CDynaNodeZeroLoadFlow*>(m_DevVec.front())->EquationsCount();
		}
	};
}
