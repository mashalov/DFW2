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

	class CAngleRoutines
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
	};

	class StraightSummation
	{
	protected:
		double m_sum = 0.0;
	public:
		void Reset()
		{
			m_sum = 0.0;
		}
		void Add(double value)
		{
			m_sum += value;
		}
		double Finalize() 
		{
			return m_sum;
		}
	};

	class KahanSummation : public StraightSummation
	{
	protected:
		volatile double m_kahan = 0.0;
	public:
		void Reset()
		{
			StraightSummation::Reset();
			m_kahan = 0.0;
		}
		void Add(double value)
		{
			volatile double y{ value - m_kahan };
			volatile double t{ m_sum + y };
			m_kahan = (t - m_sum) - y;
			m_sum = t;
		}
	};

	class NeumaierSummation : public StraightSummation
	{
	protected:
		volatile double m_neumaier = 0.0;
	public:
		void Reset()
		{
			StraightSummation::Reset();
			m_neumaier = 0.0;
		}
		void Add(double value)
		{
			volatile double t{ m_sum + value };
			m_neumaier += (std::abs(m_sum) >= std::abs(value)) ? ((m_sum - t) + value) : ((value - t) + m_sum);
			m_sum = t;
		}
		double Finalize()
		{
			m_sum += m_neumaier;
			m_neumaier = 0.0;
			return m_sum;
		}
	};
};
