#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"
#include "DynaGeneratorMotion.h"

namespace DFW2
{
	class CDynaGeneratorDQBase : public CDynaGeneratorMotion
	{
	protected:
		cplx GetXofEqs() override { return cplx(0, xq); };
	public:
		enum CONSTVARS
		{
			C_EXCITERID = CDynaGeneratorMotion::CONSTVARS::C_LAST,		// номер возбудителя
			C_EQNOM,													// номинальная эдс возбуждения (рассчитывается внутри данной модели)
			C_SNOM,														// номинальная полная мощность
			C_QNOM,														// номинальная реактивная мощность
			C_INOM,														// номинальный ток
			C_EQE,														// каст ЭДС возбуждения к константе для расчета начальных условий
			C_LAST
		};

		double m_ExciterId, Eqnom, Snom, Qnom, Inom;

		enum VARS
		{
			V_VD = CDynaGeneratorMotion::V_LAST,
			V_VQ,
			V_ID,
			V_IQ,
			V_EQ,
			V_LAST
		};

		VariableIndex Vd, Vq, Id, Iq, Eq;							// составляющие напряжения и тока в осях 
		VariableIndexExternalOptional ExtEqe;						// внешняя переменная эдс возбуждения

		cplx m_Egen; // эквивалентная ЭДС генератора для учета явнополюсности в стартовом методе
					 // используется всеми dq генераторами

		using CDynaGeneratorMotion::CDynaGeneratorMotion;
		virtual ~CDynaGeneratorDQBase() = default;

		void IfromDQ();												// расчет токов в осях RI из токов в DQ
		double Xgen() override;
		cplx Igen(ptrdiff_t nIteration) override;
		virtual const cplx& CalculateEgen();

		// блок матрицы и правая часть уравнения для тока ir<-dq и напряжения dq-<ri
		bool BuildRIfromDQEquations(CDynaModel* pDynaModel);
		bool BuildRIfromDQRightHand(CDynaModel* pDynaModel);

		double* GetVariablePtr(ptrdiff_t nVarIndex) override;
		double* GetConstVariablePtr(ptrdiff_t nVarIndex) override;
		VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec) override;
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel* pDynaModel) override;

		void UpdateSerializer(CSerializerBase* Serializer) override;

		static constexpr const char* m_cszEqe = "Eqe";
		static constexpr const char* m_cszEq = "Eq";
		static constexpr const char* m_cszId = "Id";
		static constexpr const char* m_cszIq = "Iq";
		static constexpr const char* m_cszExciterId = "ExciterId";
		static constexpr const char* m_cszEqnom = "Eqnom";
		static constexpr const char* m_cszSnom = "Snom";
		static constexpr const char* m_cszInom = "Inom";
		static constexpr const char* m_cszQnom = "Qnom";

		static void DeviceProperties(CDeviceContainerProperties& properties);
	};
}

#pragma once
