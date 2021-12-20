#include "Plot.h"

CPlot::CPlot(size_t PointCount, const double* pt, const double* pv)
{
	const double* pte{ pt + PointCount };
	while (pt < pte)
	{
		data.insert( Point{*pt, *pv} );
		pt++; pv++;
	}
}
