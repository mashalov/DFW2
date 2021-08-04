#pragma once
#include <cmath>
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
};
