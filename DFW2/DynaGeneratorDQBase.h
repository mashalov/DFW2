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
		double DampMechanicalPower_ = 1.0;
		double GetXofEqs() const override { return xq; };
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel) override;
		eDEVICEFUNCTIONSTATUS InitModel(CDynaModel* pDynaModel) override;
		void CalculateDerivatives(CDynaModel* pDynaModel, CDevice::fnDerivative fn) override;
		void CompareParksParameterCalculation();
		void GetVdVq();
		void GetPQ();
		inline cplx ToRI(const cplx& Value) const
		{
			return Value * std::polar(1.0, static_cast<double>(Delta));
		}
		inline cplx ToDQ(const cplx& Value) const
		{
			return Value * std::polar(1.0, -static_cast<double>(Delta));
		}
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

		double ExciterId, Eqnom, Snom, Qnom, Inom, xd;
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

		static constexpr const char* cszEqe_ = "Eqe";
		static constexpr const char* cszEq_ = "Eq";
		static constexpr const char* cszId_ = "Id";
		static constexpr const char* cszIq_ = "Iq";
		static constexpr const char* cszVd_ = "Vd";
		static constexpr const char* cszVq_ = "Vq";
		static constexpr const char* cszExciterId_ = "ExciterId";
		static constexpr const char* cszEqnom_ = "Eqnom";
		static constexpr const char* cszInom_ = "Inom";
		static constexpr const char* cszxd_ = "xd";
		static constexpr const char* csztdo1_ = "td01";
		static constexpr const char* csztdo2_ = "td02";
		static constexpr const char* csztqo1_ = "tq01";
		static constexpr const char* csztqo2_ = "tq02";
		static constexpr const char* csztd1_ = "Td1";
		static constexpr const char* csztd2_ = "Td2";
		static constexpr const char* csztq1_ = "Tq1";
		static constexpr const char* csztq2_ = "Tq2";
		static constexpr const char* cszxd2_ = "xd2";
		static constexpr const char* cszxq1_ = "xq1";
		static constexpr const char* cszxq2_ = "xq2";
		static constexpr const char* cszxl_ = "xl";
		static constexpr const char* csztq01_ = "tq01";
		static constexpr const char* cszPsifd_ = "Psifd";
		static constexpr const char* cszPsi1d_ = "Psi1d";
		static constexpr const char* cszPsi1q_ = "Psi1q";
		static constexpr const char* cszPsi2q_ = "Psi2q";

		static constexpr const char* cszBadCoeficients_ = "(lad + lrc)^2 - (lad + lrc + lfd) * (lad + lrc + l1d)";

		static void DeviceProperties(CDeviceContainerProperties& properties);

		bool GetAxisParametersNiipt(const double& x, double xl, double x1, double x2, double To1, double To2, double& r1, double& l1, double& r2, double& l2, PARK_PARAMETERS_DETERMINATION_METHOD Method);
		bool GetAxisParametersNiipt(double x, double xl, double x2, double To2, double& r1, double& l1);
		bool GetAxisParametersCanay(const double& x, double xl, double x1, double x2, double To1, double To2, double& r1, double& l1, double& r2, double& l2);
		bool GetAxisParametersCanay(double x, double xl, double x2, double To2, double& r1, double& l1);
		bool GetShortCircuitTimeConstants(const double& X, double xl, double x1, double x2, double To1, double To2, double& T1, double& T2);
		bool GetCanayTimeConstants(const double& x, double xl, double x1, double x2, double xrc, double& To1, double& To2, double& T1, double& T2);
		bool CheckTimeConstants(const char* cszTo1, const char* cszT1, const char* cszTo2, const char* cszT2, double To1, double T1, double To2, double T2) const;

		static inline CValidationRuleBiggerT<CDynaGeneratorDQBase, &CDynaGeneratorDQBase::Tdo2> ValidatorTdo1 = { CDynaGeneratorDQBase::csztdo2_ };
		static inline CValidationRuleBiggerT<CDynaGeneratorDQBase, &CDynaGeneratorDQBase::Tqo2> ValidatorTqo1 = { CDynaGeneratorDQBase::csztqo2_ };
		static inline CValidationRuleBiggerOrEqualT<CDynaGeneratorInfBusBase, &CDynaGeneratorInfBusBase::xd1> ValidatorXd = { CDynaGeneratorInfBusBase::cszxd1_ };
		static inline CValidationRuleBiggerT<CDynaGeneratorDQBase, &CDynaGeneratorDQBase::xd2> ValidatorXd1 = { CDynaGeneratorDQBase::cszxd2_ };
		static inline CValidationRuleBiggerT<CDynaGeneratorDQBase, &CDynaGeneratorDQBase::xq1> ValidatorXq = { CDynaGeneratorDQBase::cszxq1_ };
		static inline CValidationRuleBiggerOrEqualT<CDynaGeneratorDQBase, &CDynaGeneratorDQBase::xq2> ValidatorXq1 = { CDynaGeneratorDQBase::cszxq2_ };
		static inline CValidationRuleLessOrEqualT<CDynaGeneratorMotion, &CDynaGeneratorMotion::xq> ValidatorXlXq = { CDynaGeneratorMotion::cszxq_ };
		static inline CValidationRuleLessOrEqualT<CDynaGeneratorDQBase, &CDynaGeneratorDQBase::xd> ValidatorXlXd = { CDynaGeneratorDQBase::cszxd_ };
		static inline CValidationRuleLess<CDynaGeneratorDQBase, &CDynaGeneratorDQBase::xd2> ValidatorXlXd2 = { CDynaGeneratorDQBase::cszxd2_ };
		static inline CValidationRuleLess<CDynaGeneratorDQBase, &CDynaGeneratorDQBase::xq2> ValidatorXlXq2 = { CDynaGeneratorDQBase::cszxq2_ };
		
	};
}

#pragma once
