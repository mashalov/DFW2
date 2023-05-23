#pragma once
#include "IntegratorBase.h"
namespace DFW2
{
	class Rosenbrock23 : public IntegratorMultiStageBase
	{
	public:
		Rosenbrock23(CDynaModel& DynaModel);
		void Step() override;
		int Order() const override { return 2; }
	protected:
		static inline const double c32 = 6.0 + std::sqrt(2.0);
		IntegratorBase::vecType f0, f1, f2, k1, k2;
	};

	class Rodas4 : public IntegratorMultiStageBase
	{
	public:
		Rodas4(CDynaModel& DynaModel);
		void Step() override;
		int Order() const override { return 4; }

	protected:
        IntegratorBase::vecType du, k1, k2, k3, k4, k5, k6;

        static inline const double a21 = 1.544000000000000;
        static inline const double a31 = 0.9466785280815826;
        static inline const double a32 = 0.2557011698983284;
        static inline const double a41 = 3.314825187068521;
        static inline const double a42 = 2.896124015972201;
        static inline const double a43 = 0.9986419139977817;
        static inline const double a51 = 1.221224509226641;
        static inline const double a52 = 6.019134481288629;
        static inline const double a53 = 12.53708332932087;
        static inline const double a54 = -0.6878860361058950;
        static inline const double C21 = -5.668800000000000;
        static inline const double C31 = -2.430093356833875;
        static inline const double C32 = -0.2063599157091915;
        static inline const double C41 = -0.1073529058151375;
        static inline const double C42 = -9.594562251023355;
        static inline const double C43 = -20.47028614809616;
        static inline const double C51 = 7.496443313967647;
        static inline const double C52 = -10.24680431464352;
        static inline const double C53 = -33.99990352819905;
        static inline const double C54 = 11.70890893206160;
        static inline const double C61 = 8.083246795921522;
        static inline const double C62 = -7.981132988064893;
        static inline const double C63 = -31.52159432874371;
        static inline const double C64 = 16.31930543123136;
        static inline const double C65 = -6.058818238834054;

        static inline const double c2 = 0.386;
        static inline const double c3 = 0.21;
        static inline const double c4 = 0.63;

        static inline const double d1 = 0.2500000000000000;
        static inline const double d2 = -0.1043000000000000;
        static inline const double d3 = 0.1035000000000000;
        static inline const double d4 = -0.03620000000000023;

        static inline const double h21 = 10.12623508344586;
        static inline const double h22 = -7.487995877610167;
        static inline const double h23 = -34.80091861555747;
        static inline const double h24 = -7.992771707568823;
        static inline const double h25 = 1.025137723295662;
        static inline const double h31 = -0.6762803392801253;
        static inline const double h32 = 6.087714651680015;
        static inline const double h33 = 16.43084320892478;
        static inline const double h34 = 24.76722511418386;
        static inline const double h35 = -6.594389125716872;
	};
}
