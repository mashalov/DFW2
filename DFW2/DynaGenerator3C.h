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
		cplx GetXofEqs() override { return cplx(0, xq); };
	public:

		enum VARS
		{
			V_EQSS = CDynaGenerator1C::V_LAST,
			V_EDSS,
			V_LAST
		};

		double Eqss, Edss;
		double Td0ss, Tq0ss;
		double xd2, xq1, xq2;

		CDynaGenerator3C();
		virtual ~CDynaGenerator3C() {}

		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		double* GetConstVariablePtr(ptrdiff_t nVarIndex) override;
		bool BuildEquations(CDynaModel* pDynaModel) override;
		bool BuildRightHand(CDynaModel* pDynaModel) override;
		bool BuildDerivatives(CDynaModel *pDynaModel) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		cplx GetEMF() override { return cplx(Eqss, Edss) * std::polar(1.0, Delta); }
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel) override;
		bool CalculatePower() override;
		double Xgen();
		const cplx& CalculateEgen() override;
		void UpdateSerializer(SerializerPtr& Serializer) override;
		static const CDeviceContainerProperties DeviceProperties();
	};
}
