#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"
#include "DynaGenerator1C.h"

namespace DFW2
{
	class CDynaGenerator3C : public CDynaGenerator1C
	{
	protected:
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel) override;
	public:

		enum VARS
		{
			V_EQSS = CDynaGenerator1C::V_LAST,
			V_EDSS,
			V_LAST
		};

		VariableIndex Eqss, Edss;

		double Td02, Tq02;
		double xd2, xq1, xq2;

		using CDynaGenerator1C::CDynaGenerator1C;
		virtual ~CDynaGenerator3C() = default;

		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		double* GetConstVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		bool BuildEquations(CDynaModel* pDynaModel) override;
		bool BuildRightHand(CDynaModel* pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS PreInit(CDynaModel* pDynaModel) override;
		cplx GetEMF() override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		bool CalculatePower() override;
		const cplx& CalculateEgen() override;
		void UpdateSerializer(CSerializerBase* Serializer) override;
		void UpdateValidator(CSerializerValidatorRules* Validator) override;
		static void DeviceProperties(CDeviceContainerProperties& properties);

		static constexpr const char* m_csztd02 = "td02";
		static constexpr const char* m_csztq02 = "tq02";
		static constexpr const char* m_cszxd2  = "xd2";
		static constexpr const char* m_cszxq1  = "xq1";
		static constexpr const char* m_cszxq2  = "xq2";
	};
}
