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
		data.emplace_back(*pt, *pv) ;
		pt++; pv++;

		while (pt < pte)
		{
			// если задано максимальное ограничение диапазона,
			// добавляем только точки, попадающие в диапазон
			if (range.max >= 0.0 && *pt > range.max)
				break;
			data.emplace_back(*pt, *pv);
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
				const double diff{ p1->WeightedDifference(v2, Rtol_, Atol_) };
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
				double mindiff{ min1->WeightedDifference(min2->v, Rtol_, Atol_) };
				const double maxdiff{ max1->WeightedDifference(max2->v, Rtol_, Atol_) };

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

// возвращает график с заданным шагом. В точках дискретных изменений
// добавляются дополнительные точки времени для воспроизведения изломов

CPlot CPlot::DenseOutput(double Step)
{
	CPlot outplot;
	auto p{ data.begin() };

	if (Step <= 0.0)
		throw std::runtime_error("CPlot::DenseOutput - step must be positive");

	if (!data.empty())
		outplot.data.reserve(static_cast<size_t>(1.2 * data.back().t / Step));
	
	// отсчитываем время как целое чтобы не
	// прибегать к компенсации сложения
	ptrdiff_t IntegerT{ 0 };	

	// идем по графику
	while (p != data.end())
	{
		double t{ IntegerT * Step };
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

		// добавляем точку в выходной вектор если 
		// она отличается от последней
		auto push = [this, &outplot](const Point& pt)
		{
			if (outplot.data.empty() || !outplot.data.back().CompareValue(pt, Rtol_, Atol_))
				outplot.data.push_back(pt);
		};

		// здесь диапазон точек, которые нужно записать
		for (auto pr = p; pr != range; pr++)
		{
			// смотрим следующую точку от текущей
			auto pnext = std::next(pr);
			// если это конец исходных данных - просто записываем текущую точку
			if (pnext != data.end())
			{
				// если следующая точка есть - проверяем интервал времени
				// если он больше минимального шага - интерполируем
				if (pnext->t - pr->t > Point::minstep)
					push({ t, (pnext->v - pr->v) / (pnext->t - pr->t) * (t - pr->t) + pr->v });
				else
				{
					// если интервал меньше минимального шага - записываем две точки - это излом
					push({ t, pr->v });
					push({ t, pnext->v });
				}
			}
			else
				push({ t, pr->v });
		}
		IntegerT++;
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