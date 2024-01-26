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
		// предусмотрен ограничитель на выходе (windup-режим)
		CLimiterConst OutputLimit, EqLimit;
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


		static constexpr const char* cszTexc_ = "Texc";
		static constexpr const char* cszUf_min_ = "Uf_min";
		static constexpr const char* cszUf_max_ = "Uf_max";
		static constexpr const char* cszIf_min_ = "If_min";
		static constexpr const char* cszIf_max_ = "If_max";
		static constexpr const char* cszKif_ = "Kif";
		static constexpr const char* cszKig_ = "Kig";
		static constexpr const char* cszDECId_ = "ForcerId";
		static constexpr const char* cszExcControlId_ = "ExcControlId";
		static constexpr const char* cszType_rg_ = "Type_rg";

		static constexpr const char* cszEqeV_ = "EqeV";
		static constexpr const char* cszEqLimit_ = "EqLimit";
	};
}
