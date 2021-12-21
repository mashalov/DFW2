#pragma once
#include "Log.h"
#include <vector>


struct PointDifference
{
	double diff;
	double t;
	double v1;
	double v2;
	PointDifference(double Difference, double Time, double Value1, double Value2) : diff(Difference), 
		t(Time),
		v1(Value1),
		v2(Value2) {}
	PointDifference() : diff(-1.0), t(-1.0), v1(0.0), v2(0.0) {}
};

struct CompareRange
{
	double min = -1.0;
	double max = -1.0;
};


class CPlot
{
	struct Point 
	{
		double t;
		double v;

		bool CompareValue(const double& value, double Rtol, double Atol) const
		{
			return WeightedDifference(value, Rtol, Atol) < 1.0;
		}

		double WeightedDifference(const double& value, double Rtol, double Atol) const
		{
			return std::abs((v - value) / (Rtol * (std::min)(std::abs(v),std::abs(value)) + Atol));
		}

		static constexpr const double minstep = 1e-8;
	};

	struct PointCompare 
	{
		bool operator()(const Point& lhs, const Point& rhs) const 
		{
			return lhs.t < rhs.t;
		}
	};
	using PLOT = std::vector<Point>;
protected:
	PLOT data;
	double m_Rtol = 1E-4;
	double m_Atol = 1E-4;
public:
	CPlot(size_t PointCount, const double* pt, const double* pv, const CompareRange& range = {});
	CPlot(CPlot&& other)
	{
		data = std::move(other.data);
	}

	double tmin() const
	{
		if (data.empty())
			return 0.0;
		else
			return data.begin()->t;
	}

	double tmax() const
	{
		if (data.empty())
			return 0.0;
		else
			return data.rbegin()->t;
	}

	PointDifference Compare(const CPlot& plot);
};

