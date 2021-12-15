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
}
