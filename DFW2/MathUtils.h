#pragma once
#include "Header.h"

namespace MathUtils
{
	class CSquareSolver
	{
	public:
		static int Roots(double a, double b, double c, double& r1, double& r2)
		{
			const double cs[3] = { c,b,a };
			double rs[2];
			const int nRoots{ Roots(cs,rs) };
			r1 = rs[0];
			r2 = rs[1];
			return nRoots;
		}

		static int Roots(const double(&c)[3], double(&r)[2])
		{
			if (c[2] == 0.0)
			{
				if (c[1] == 0.0)
					return 0;
				else
					r[0] = -c[2] / c[1];
				return 1;
			}

			double d{ c[1] * c[1] - 4.0 * c[2] * c[0]};
			if (d >= 0)
			{
				d = std::sqrt(d);
				r[0] = (-c[1] + d) / 2.0 / c[2];
				r[1] = (-c[1] - d) / 2.0 / c[2];
				// use stable formulas to avoid
				// precision loss by numerical cancellation 
				// "What Every Computer Scientist Should Know About Floating-Point Arithmetic" by DAVID GOLDBERG p.10
				if ((c[1] * c[1] - c[2] * c[0]) > 1E3 && c[1] > 0)
					r[0] = 2.0 * c[0] / (-c[1] - d);
				else
					r[1] = 2.0 * c[0] / (-c[1] + d);

				//_ASSERTE(DFW2::Equal(a * r1 * r1 + b * r1 + c, 0.0));
				//_ASSERTE(DFW2::Equal(a * r2 * r2 + b * r2 + c, 0.0));

				return (d == 0) ? 1 : 2;
			}
			else
				return 0;
		}

		static int RootsSortedByAbs(double a, double b, double c, double& r1, double& r2)
		{
			int nRoots(Roots(a, b, c, r1, r2));
			if (nRoots && std::abs(r1) > std::abs(r2))
				std::swap(r1, r2);
			return nRoots;
		}
	};

	// https://github.com/erich666/GraphicsGems/blob/master/gems/Roots3And4.c

	class CCubicSolver
	{
	public:
		static int Roots(const double(&c)[4], double(&r)[4])
		{
			int num{ 0 };
			// normal form: x^3 + Ax^2 + Bx + C = 0 

			const double A{ c[2] / c[3] };
			const double B{ c[1] / c[3] };
			const double C{ c[0] / c[3] };

			//  substitute x = y - A/3 to eliminate quadric term:
			// x^3 +px + q = 0

			const double sq_A{ A * A };
			const double p{ 1.0 / 3 * (-1.0 / 3 * sq_A + B) };
			const double q{ 1.0 / 2 * (2.0 / 27 * A * sq_A - 1.0 / 3 * A * B + C) };

			// use Cardano's formula 

			const double cb_p{ p * p * p };
			const double D{ q * q + cb_p };

			if (D == 0.0)
			{
				if (q == 0.0) // one triple solution 
				{
					r[0] = 0;
					num = 1;
				}
				else // one single and one double solution 
				{
					const double u{ std::cbrt(-q) };
					r[0] = 2 * u;
					r[1] = -u;
					num = 2;
				}
			}
			else if (D < 0) // three real solutions
			{
				const double phi{ 1.0 / 3 * std::acos(-q / std::sqrt(-cb_p)) };
				const double t{ 2 * std::sqrt(-p) };

				r[0] = t * std::cos(phi);
				r[1] = -t * std::cos(phi + M_PI / 3);
				r[2] = -t * std::cos(phi - M_PI / 3);
				num = 3;
			}
			else // one real solution
			{
				const double sqrt_D{ std::sqrt(D) };
				const double u{ std::cbrt(sqrt_D - q) };
				const double v{ -std::cbrt(sqrt_D + q) };
				r[0] = u + v;
				num = 1;
			}

			// resubstitute

			const double sub{ 1.0 / 3 * A };

			for (int i{ 0 }; i < num; ++i)
				r[i] -= sub;

			return num;
		}
	};

	class AngleRoutines
	{
	public:
		// возвращает минимальный угол в радианах между углами y и x со знаком
		static double GetAbsoluteDiff2Angles(const double x, const double y)
		{
			// https://stackoverflow.com/questions/1878907/how-can-i-find-the-difference-between-two-angles

			/*
			def f(x,y):
			import math
			return min(y-x, y-x+2*math.pi, y-x-2*math.pi, key=abs)
			*/

			std::array<double, 3> args{ y - x , y - x + 2.0 * M_PI , y - x - 2 * M_PI };
			return *std::min_element(args.begin(), args.end(), [](const auto& lhs, const auto& rhs) { return std::abs(lhs) < std::abs(rhs); });
		}

		
		// возврщает угол приведенный к диапазону [-pi;pi) (удаление периодов)
		inline static double WrapPosNegPI(double fAng)
		{
			return AngleRoutines::Mod(fAng + M_PI, 2 * M_PI) - M_PI;
		}

		// кастомное деление по модулю для периодизации угла
		template<typename T> 
		static T Mod(T x, T y)
		{
			// https://stackoverflow.com/questions/4633177/c-how-to-wrap-a-float-to-the-interval-pi-pi

			static_assert(!std::numeric_limits<T>::is_exact, "Mod: floating-point type expected");

			if (0. == y)
				return x;

			const double m{ x - y * floor(x / y) };

			// handle boundary cases resulted from floating-point cut off:

			if (y > 0)              // modulo range: [0..y)
			{
				if (m >= y)           // Mod(-1e-16             , 360.    ): m= 360.
					return 0;

				if (m < 0)
				{
					if (y + m == y)
						return 0; // just in case...
					else
						return y + m; // Mod(106.81415022205296 , _TWO_PI ): m= -1.421e-14 
				}
			}
			else                    // modulo range: (y..0]
			{
				if (m <= y)           // Mod(1e-16              , -360.   ): m= -360.
					return 0;

				if (m > 0)
				{
					if (y + m == y)
						return 0; // just in case...
					else
						return y + m; // Mod(-106.81415022205296, -_TWO_PI): m= 1.421e-14 
				}
			}

			return m;
		}
	};

	class StraightSummation
	{
	protected:
		double sum_ = 0.0;
	public:
		void Reset()
		{
			sum_ = 0.0;
		}
		void Add(double value) noexcept
		{
			sum_ += value;
		}
		double Finalize() noexcept
		{
			return sum_;
		}
	};

	class KahanSummation : public StraightSummation
	{
	protected:
		volatile double kahan_ = 0.0;
	public:
		void Reset() noexcept
		{
			StraightSummation::Reset();
			kahan_ = 0.0;
		}
		void Add(double value) noexcept
		{
			volatile double y{ value - kahan_ };
			volatile double t{ sum_ + y };
			kahan_ = (t - sum_) - y;
			sum_ = t;
		}
	};

	class NeumaierSummation : public StraightSummation
	{
	protected:
		volatile double neumaier_ = 0.0;
	public:
		void Reset() noexcept
		{
			StraightSummation::Reset();
			neumaier_ = 0.0;
		}
		void Add(double value) noexcept
		{
			volatile double t{ sum_ + value };
			neumaier_ += (std::abs(sum_) >= std::abs(value)) ? ((sum_ - t) + value) : ((value - t) + sum_);
			sum_ = t;
		}
		double Finalize() noexcept
		{
			sum_ += neumaier_;
			neumaier_ = 0.0;
			return sum_;
		}
	};
};

