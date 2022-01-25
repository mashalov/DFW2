#pragma once
#include "DynaGenerator1C.h"
#include "DynaExciterBase.h"
#include "LimiterConst.h"

namespace DFW2
{
	class CDynaExciterMustang : public CDynaExciterBase
	{
	protected:
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		VariableIndex EqOutputValue;
		CLimiterConst EqLimit;
		bool SetUpEqLimits(CDynaModel *pDynaModel, CDynaPrimitiveLimited::eLIMITEDSTATES EqLimitStateInitial, CDynaPrimitiveLimited::eLIMITEDSTATES EqLimitStateResulting);
	public:

		double Kig, Kif, Imin, Imax;
		CDynaExciterMustang();
		virtual ~CDynaExciterMustang() = default;

		void UpdateSerializer(CSerializerBase* Serializer) override;
		void UpdateValidator(CSerializerValidatorRules* Validator) override;
		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		void BuildEquations(CDynaModel* pDynaModel) override;
		void BuildRightHand(CDynaModel* pDynaModel) override;
		void BuildDerivatives(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		double CheckZeroCrossing(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		static void DeviceProperties(CDeviceContainerProperties& properties);


		static constexpr const char* m_cszTexc = "Texc";
		static constexpr const char* m_cszUf_min = "Uf_min";
		static constexpr const char* m_cszUf_max = "Uf_max";
		static constexpr const char* m_cszIf_min = "If_min";
		static constexpr const char* m_cszIf_max = "If_max";
		static constexpr const char* m_cszKif = "Kif";
		static constexpr const char* m_cszKig = "Kig";
		static constexpr const char* m_cszDECId = "ForcerId";
		static constexpr const char* m_cszExcControlId = "ExcControlId";
		static constexpr const char* m_cszType_rg = "Type_rg";
	};
}
