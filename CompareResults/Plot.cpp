#include "Plot.h"

CPlot::CPlot(size_t PointCount, const double* pt, const double* pv, const CompareRange& range)
{
	if (PointCount)
	{
		data.reserve(PointCount);
		const double* ptOriginal{ pt };
		const double* pte{ pt + PointCount };
		
		// если задано минимальное ограничение диапазона
		// находим первую точку, попадающую в диапазон

		if(range.min >= 0.0)
			while (pt < pte)
			{
				if (*pt < range.min)
				{
					pt++;
				}
				else
					break;
			}

		pv = pv + (pt - ptOriginal);
		data.push_back({ *pt, *pv });
		pt++; pv++;

		while (pt < pte)
		{
			// если задано максимальное ограничение диапазона,
			// добавляем только точки, попадающие в диапазон
			if (range.max >= 0.0 && *pt > range.max)
				break;
			data.push_back({ *pt, *pv });
			pt++; pv++;
		}
	}
}

/* Идея сравнения: 
* Идем по первому графику, находим точки, которые отстоят друг от друга менее чем на минимальный шаг интегрирования.
* Получаем одну или несколько точек. Одна точка - обычная ситуация, несколько - разрыв в некоторой точке времени
* Во втором графике находим интервал, первая точка которого приближается к найденной точке первого графика слева , 
* вторая - справа, то есть интервал второго графика охватывает точку первого. В этом случае можно определить 
* точку в интервале второго графика с помощью интерполяции
*/
PointDifference CPlot::Compare(const CPlot& plot)
{
	PointDifference PointDiff;

	// если графики пустые - не сравниваем
	if (plot.data.empty() || data.empty())
		return PointDiff;

	auto p1{ data.begin() };
	auto p2{ plot.data.begin() };

	// идем по первому графику
	while (p1 != data.end())
	{
		// p1 - начальная точка интервала
		// находим range1 - диапазон, в котором 
		// время точек отличается от p1 меньше минимального

		// начинаем правую границу интервала первого графика
		// со следующей от начала
		auto range1 = std::next(p1);

		// ищем правую точку интервала со временем
		// превышающим время первой точки более чем
		// на минимальное значение
		while (range1 != data.end())
		{
			if (range1->t - p1->t <= Point::minstep)
				range1++;
			else
				break;
		}
		// здесь имеем интервал первого графика с одной или несколькими точками

		// ищем левую точку интервала второго графика
		while (p2 != plot.data.end())
		{
			// просматриваем второй график вправо
			auto p2next = std::next(p2);
			// если график закончился, интервал закрывается end()
			// если следующая точка имеет время больше, чем
			// точка первого интервала, то возвращаем предыдущую точку
			if (p2next != plot.data.end() && p2next->t > p1->t)
				break;
			p2 = p2next;
		}

		// находим точку во втором графике, время которой не меньше
		// чем в первом

		auto range2 = p2;
		while (range2 != plot.data.end())
		{
			if (range2->t <= p1->t)
				range2++;
			else
				break;
		}

		// здесь [p1;range1) множество точек первого графика близких к p1 на дистанции минимального шага
		// [p2;range2) - множество точек второго графика, включающее множество [p1;range1)

		auto rangesize1 = std::distance(p1, range1);
		auto rangesize2 = std::distance(p2, range2);

		_ASSERTE(rangesize1 != 0);

		if (rangesize2 != 0)
		{
			// второй график имеет диапазон
			// если диапазон пустой - искать нечего

			if (rangesize1 <= 1)
			{
				// в первом графике есть одна точка,
				// интерполируем по ней точку в интервале 
				// второго графика

				if (rangesize2 > 1)
				{
					_ASSERTE(0);
				}

				// во втором графике две точки по краям диапазона

				// если интервал второго графика заканчивается end(),
				// то для интерполяции используем предпоследнюю и последнюю точки
				if (range2 == plot.data.end())
				{
					range2 = p2;
					p2 = std::prev(p2);
				}
				// интерполируем точку в интервале второго графика по времени точки первого графика
				const double v2{ (range2->v - p2->v) / (range2->t - p2->t) * (p1->t - p2->t) + p2->v };
				const double diff{ p1->WeightedDifference(v2, m_Rtol, m_Atol) };
				if (PointDiff.diff < diff)
					PointDiff = PointDifference(diff, p1->t, p1->v, v2);
			}
			else
			{
				// в интервале первого графика несколько точек (почти точно - разрыв)
				// находим минимальную и максимальную точки по значению в первом и втором интервалах
				auto min1{ std::min_element(p1, range1, [](const Point& lhs, const Point& rhs) { return lhs.v < rhs.v; }) };
				const auto max1{ std::min_element(p1, range1, [](const Point& lhs, const Point& rhs) { return rhs.v < lhs.v; }) };
				auto min2{ std::min_element(p2, range2, [](const Point& lhs, const Point& rhs) { return lhs.v < rhs.v; }) };
				const auto max2{ std::min_element(p2, range2, [](const Point& lhs, const Point& rhs) { return rhs.v < lhs.v; }) };

				// сравниваем найденные точки - минимальную с минимальной, максимальную с максимальной
				double mindiff{ min1->WeightedDifference(min2->v, m_Rtol, m_Atol) };
				const double maxdiff{ max1->WeightedDifference(max2->v, m_Rtol, m_Atol) };

				// определяем минимальную разность 

				if (maxdiff < mindiff)
				{
					min1 = max1; min2 = max2;
					mindiff = maxdiff;
				}

				if (PointDiff.diff < mindiff)
					PointDiff = PointDifference(mindiff, p1->t, min1->v, min2->v);
			}
		}

		p1 = range1;
		p2 = range2;
	}

	return PointDiff;
}


CPlot CPlot::DenseOutput(double Step)
{
	CPlot outplot;
	double t{ 0.0 };

	auto p{ data.begin() };

	// идем по графику
	while (p != data.end())
	{
		// ищем ближайшую точку к заданному времени слева
		while (p != data.end())
		{
			auto pnext = std::next(p);
			if (pnext != data.end() && pnext->t > t)
				break;
			p = pnext;
		}

		// возвращаемся назад до тех пор, пока точки идут
		// с минимальным шагом

		if (p != data.end())
		{
			while (p != data.begin())
			{
				auto pprev = std::prev(p);
				if (p->t - pprev->t <= Point::minstep)
					p = pprev;
				else
					break;
			}
		}

		// ищем точку справа до тех пор, пока они
		// идут с минимальным шагом
		auto range = p;
		while (range != data.end())
		{
			if (range->t - p->t <= Point::minstep)
				range++;
			else
				break;
		}

		auto rangesize = std::distance(p, range);

		for (auto pr = p; pr != range; pr++)
		{
			auto pnext = std::next(pr);
			if (pnext->t - pr->t > Point::minstep)
			{
				const double v2{ (range->v - p->v) / (range->t - p->t) * (t - p->t) + p->v };
				outplot.data.push_back({ t, v2 });
			}
			else
				outplot.data.push_back({ t, pr->v });

		}

		t += Step;
	}

	return outplot;
}

void CPlot::WriteCSV(std::filesystem::path csvpath) const
{
	std::ofstream csvfile(csvpath);
	csvfile.imbue(std::locale(std::cout.getloc(), new DecimalSeparator));

	if (csvfile.is_open())
		for (const auto& point : data)
			csvfile << point.t << ";" << point.v << std::endl;
}