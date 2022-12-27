#pragma once
#include "DeviceContainer.h"
#include "DynaNode.h"
#include "DynaGeneratorMotion.h"

namespace DFW2
{

// дефайн который определяет какие уравнения движения используются
// для уравнений Парка
//#define USE_VOLTAGE_FREQ_DAMPING

	class CDynaGeneratorDQBase : public CDynaGeneratorMotion
	{
	protected:
		double GetXofEqs() const override { return xq; };
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel) override;
		void CalculateDerivatives(CDynaModel* pDynaModel, CDevice::fnDerivative fn) override;
		void CompareParksParameterCalculation();
	public:
		enum CONSTVARS
		{
			C_EXCITERID = CDynaGeneratorMotion::CONSTVARS::C_LAST,		// номер возбудителя
			C_EQNOM,													// номинальная эдс возбуждения (рассчитывается внутри данной модели)
			C_EQE,														// каст ЭДС возбуждения к константе для расчета начальных условий
			C_LAST
		};

		double ExciterId, Eqnom, xd;
		double xd2, xq1, xq2, xl, Tdo1, Tqo1, Tdo2, Tqo2;

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

		cplx Egen_; // эквивалентная ЭДС генератора для учета явнополюсности в стартовом методе
					 // используется всеми dq генераторами

		using CDynaGeneratorMotion::CDynaGeneratorMotion;
		virtual ~CDynaGeneratorDQBase() = default;

		void IfromDQ();												// расчет токов в осях RI из токов в DQ
		cplx Igen(ptrdiff_t nIteration) override;
		virtual const cplx& CalculateEgen();

		// блок матрицы и правая часть уравнения для тока ir<-dq и напряжения dq-<ri
		void BuildRIfromDQEquations(CDynaModel* pDynaModel);
		void BuildRIfromDQRightHand(CDynaModel* pDynaModel);

		// блок матрицы уравнения движения
		void BuildMotionEquationBlock(CDynaModel* pDynaModel);
		eDEVICEFUNCTIONSTATUS PreInit(CDynaModel* pDynaModel) override;

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
		static constexpr const char* m_cszVd = "Vd";
		static constexpr const char* m_cszVq = "Vq";
		static constexpr const char* m_cszExciterId = "ExciterId";
		static constexpr const char* m_cszEqnom = "Eqnom";
		static constexpr const char* m_cszxd = "xd";
		static constexpr const char* m_csztdo1 = "td01";
		static constexpr const char* m_csztdo2 = "td02";
		static constexpr const char* m_csztqo1 = "tq01";
		static constexpr const char* m_csztqo2 = "tq02";
		static constexpr const char* m_csztd1 = "Td1";
		static constexpr const char* m_csztd2 = "Td2";
		static constexpr const char* m_csztq1 = "Tq1";
		static constexpr const char* m_csztq2 = "Tq2";
		static constexpr const char* m_cszxd2 = "xd2";
		static constexpr const char* m_cszxq1 = "xq1";
		static constexpr const char* m_cszxq2 = "xq2";

		static constexpr const char* m_cszBadCoeficients = "(lad + lrc)^2 - (lad + lrc + lfd) * (lad + lrc + l1d)";

		static void DeviceProperties(CDeviceContainerProperties& properties);

		bool GetAxisParametersNiipt(const double& x, double xl, double x1, double x2, double To1, double To2, double& r1, double& l1, double& r2, double& l2, PARK_PARAMETERS_DETERMINATION_METHOD Method);
		bool GetAxisParametersNiipt(double x, double xl, double x2, double To2, double& r1, double& l1);
		bool GetAxisParametersCanay(const double& x, double xl, double x1, double x2, double To1, double To2, double& r1, double& l1, double& r2, double& l2);
		bool GetAxisParametersCanay(double x, double xl, double x2, double To2, double& r1, double& l1);
		bool GetShortCircuitTimeConstants(const double& X, double xl, double x1, double x2, double To1, double To2, double& T1, double& T2);
		bool GetCanayTimeConstants(const double& x, double xl, double x1, double x2, double xrc, double& To1, double& To2, double& T1, double& T2);
		bool CheckTimeConstants(const char* cszTo1, const char* cszT1, const char* cszTo2, const char* cszT2, double To1, double T1, double To2, double T2) const;

		static inline CValidationRuleBiggerT<CDynaGeneratorDQBase, &CDynaGeneratorDQBase::Tdo2> ValidatorTdo1 = { CDynaGeneratorDQBase::m_csztdo2 };
		static inline CValidationRuleBiggerT<CDynaGeneratorDQBase, &CDynaGeneratorDQBase::Tqo2> ValidatorTqo1 = { CDynaGeneratorDQBase::m_csztqo2 };
		static inline CValidationRuleBiggerOrEqualT<CDynaGeneratorInfBusBase, &CDynaGeneratorInfBusBase::xd1> ValidatorXd = { CDynaGeneratorInfBusBase::m_cszxd1 };
		static inline CValidationRuleBiggerT<CDynaGeneratorDQBase, &CDynaGeneratorDQBase::xd2> ValidatorXd1 = { CDynaGeneratorDQBase::m_cszxd2 };
		static inline CValidationRuleBiggerT<CDynaGeneratorDQBase, &CDynaGeneratorDQBase::xq1> ValidatorXq = { CDynaGeneratorDQBase::m_cszxq1 };
		static inline CValidationRuleBiggerOrEqualT<CDynaGeneratorDQBase, &CDynaGeneratorDQBase::xq2> ValidatorXq1 = { CDynaGeneratorDQBase::m_cszxq2 };
		static inline CValidationRuleLessOrEqualT<CDynaGeneratorMotion, &CDynaGeneratorMotion::xq> ValidatorXlXq = { CDynaGeneratorMotion::m_cszxq };
		static inline CValidationRuleLessOrEqualT<CDynaGeneratorDQBase, &CDynaGeneratorDQBase::xd> ValidatorXlXd = { CDynaGeneratorDQBase::m_cszxd };
		static inline CValidationRuleLess<CDynaGeneratorDQBase, &CDynaGeneratorDQBase::xd2> ValidatorXlXd2 = { CDynaGeneratorDQBase::m_cszxd2 };
		static inline CValidationRuleLess<CDynaGeneratorDQBase, &CDynaGeneratorDQBase::xq2> ValidatorXlXq2 = { CDynaGeneratorDQBase::m_cszxq2 };
		
	};
}

#pragma once
