#pragma once
#include "Header.h"

namespace MathUtils
{
	class CSquareSolver
	{
	public:
		static int Roots(double a, double b, double c, double& r1, double& r2)
		{
			if (a == 0.0)
			{
				if (b == 0.0)
					return 0;
				else
					r1 = -c / b;

				return 1;
			}

			double d = b * b - 4.0 * a * c;
			if (d >= 0)
			{
				d = sqrt(d);
				r1 = (-b + d) / 2.0 / a;
				r2 = (-b - d) / 2.0 / a;
				// use stable formulas to avoid
				// precision loss by numerical cancellation 
				// "What Every Computer Scientist Should Know About Floating-Point Arithmetic" by DAVID GOLDBERG p.10
				if ((b * b - a * c) > 1E3 && b > 0)
					r1 = 2.0 * c / (-b - d);
				else
					r2 = 2.0 * c / (-b + d);

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
