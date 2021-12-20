#pragma once
#include "Log.h"
#include <set>

class CPlot
{
	struct Point 
	{
		double t;
		double v;
	};

	struct PointCompare 
	{
		bool operator()(const Point& lhs, const Point& rhs) const 
		{
			return lhs.t < rhs.t;
		}
	};
	using PLOT = std::multiset<Point, PointCompare>;
protected:
	PLOT data;
public:
	CPlot(size_t PointCount, const double *pt, const double *pv);
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
};

