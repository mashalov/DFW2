#pragma once
#include "DynaPowerInjector.h"

namespace DFW2
{
	class CSVC : public CDynaPowerInjector
	{
	protected:
		//eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel) override;
		struct Output
		{
			double Bunlimited;
			double Blimited;
			double Qunlimited;
			double Qlimited;
			CDynaPrimitiveLimited::eLIMITEDSTATES State;
		} Output_;
	public:
		CSVC() : CDynaPowerInjector()
		{
			P = 0.0;
			Kgen = 1.0;
		}

		virtual eDEVICEFUNCTIONSTATUS PreInit(CDynaModel* pDynaModel);

		double Droop_ = 0.03;
		double Qnom_ = 100.0;
		double Vref_ = 220.0;
		double xsl_ = 0.01;
		double Bmin_ = -1;
		double Bmax_ = -1;

		const Output& B(const CDynaNodeBase& pNode);

		static void DeviceProperties(CDeviceContainerProperties& properties);
		void UpdateSerializer(CSerializerBase* Serializer) override;
		void UpdateValidator(CSerializerValidatorRules* Validator) override;
		static constexpr const char* m_cszr = "r";
		static constexpr const char* m_cszxd1 = "xd1";
		static constexpr const char* m_cszEqs = "Eqs";
		static constexpr const char* cszDroop_ = "Droop";
	};
}

