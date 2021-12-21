#include "Plot.h"

CPlot::CPlot(size_t PointCount, const double* pt, const double* pv, const CompareRange& range)
{
	if (PointCount)
	{
		data.reserve(PointCount);
		const double* pte{ pt + PointCount };
		
		// если задано минимальное ограничение диапазона
		// находим первую точку, попадающую в диапазон

		if(range.min >= 0.0)
			while (pt < pte)
			{
				if (*pt < range.min)
					pt++;
				else
					break;
			}

		Point point{ *pt, *pv };
		// первую точку добавляем в сет
		data.push_back(point);
		pt++; pv++;

		while (pt < pte)
		{
			// если задано максимальное ограничение диапазона,
			// добавляем только точки, попадающие в диапазон
			if (range.max >= 0.0 && *pt > range.max)
				break;
			
			// проверяем, отличается ли следующая точка от предыдущей
			const bool bEqual{ point.CompareValue(*pv, m_Rtol, m_Atol) };
			point.t = *pt;
			// если не отличается, считаем
			// что изменилось только время
			if (!bEqual)
			{
				// если отличается
				//  проверяем, предыдущая точка отстоит от текущей 
				// по времени больше чем на минимальный шаг

				if (point.t - *(pt - 1) < 1E-8)
					data.push_back({ *(pt - 1), *(pv - 1) });
				
				// обновляем предыдущую точку
				// добавляем ее в сет
				point.v = *pv;
				data.push_back(point);
			}
			pt++; pv++;
		}
		// если время последней точки сета 
		// не равно времени последней точки графика
		// добавляем последнюю точку
		if (data.rbegin()->t != point.t)
		{
			point.v = *(pv - 1);
			data.push_back(point);
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
			if (p2next == plot.data.end())
				break;

			// если следующая точка имеет время больше, чем
			// точка первого интервала, то возвращаем предыдущую точку
			if (p2next->t < p1->t)
				p2 = p2next;
			else
				break;
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
