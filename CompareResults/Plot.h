#pragma once
#include "Log.h"
#include <vector>
#include <locale>
#include <filesystem>
#include <fstream>


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


class DecimalSeparator : public std::numpunct<char>
{
public:
	using numpunct<char>::numpunct;
protected:
	char do_decimal_point() const override
	{
		return ',';
	}
};


class CPlot
{
	struct Point 
	{
		double t;
		double v;

		Point(double time, double value) : t(time), v(value) {}

		bool CompareValue(const double& value, double Rtol, double Atol) const
		{
			return WeightedDifference(value, Rtol, Atol) < 1.0;
		}

		bool CompareValue(const Point& point) const
		{
			return point.t == t && point.v == v;
		}

		bool CompareValue(const Point& point, double Rtol, double Atol) const
		{
			return point.t == t && CompareValue(point.v, Rtol, Atol);
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
	double Rtol_ = 1E-4;
	double Atol_ = 1E-4;
public:
	CPlot(size_t PointCount, const double* pt, const double* pv, const CompareRange& range = {});
	CPlot(CPlot&& other)
	{
		data = std::move(other.data);
	}
	CPlot() {};

	CPlot& operator= (CPlot&& other)
	{
		data = std::move(other.data);
		return *this;
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

	void WriteCSV(std::filesystem::path csvpath) const;

	PointDifference Compare(const CPlot& plot);
	CPlot DenseOutput(double Step);
};

