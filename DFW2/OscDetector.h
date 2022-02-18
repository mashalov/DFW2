#pragma once
#include <vector>
#include <list>
#include <stdexcept>
#include <algorithm>
#include <functional>
#include <iostream>

class COscDetectorBase
{
	// состояния детектора
	enum class eState
	{
		wait_min_or_max,	// неопределено - ждем отклонения вверх или вниз
		wait_min,			// ждем минимальное значение
		wait_max,			// ждем максимальное значение
	};

	// описание времени
	struct point_time_type
	{
		double time = 0.0;	// время в секундах

		point_time_type(double t) : time(t) {}
		point_time_type() : time(0.0) {}

		// возвращает true, если время в параметре больше чем время (проверка возрастания времени)
		[[nodiscard]] bool check(const point_time_type& pointtime)
		{
			return pointtime.time > time;
			if (pointtime.time < time)
				throw std::runtime_error("point_time - time to be added is less than previous time");
		}
	};

	// тип значения в детекторе - double
	using value_type = double;

	// конверсия значения из некоторого типа в value_type
	template<typename T>
	static inline value_type Value(T);
	
	// точка экстремума (минимума или максимума) с привязкой ко времени
	struct extreme_point_type : point_time_type 
	{
		value_type value;	// значение экстремума
		extreme_point_type(double time, const value_type& val) : point_time_type(time), value(val) {}
	};

	// описание состояния детектора
	struct value_state_type
	{
		value_type value;							// последнее известное значение
		eState state = eState::wait_min_or_max;		// состояние детектора
		std::list<extreme_point_type> mins, maxs;	// найденные минимумы и максимумы
		
		value_state_type(value_type val) : value(val) {}
		value_state_type() : value(0.0) {}

		// оператор присвоения из value_type
		value_state_type& operator = (const value_type& val) 
		{ 
			value = val;
			return *this;
		}

		// очистка экстремумов (shortcut для одновременной очистки минимумов и максимумов)
		void clear_min_max()
		{
			mins.clear();
			maxs.clear();
		}
	};

	// описание вектора значений
	template<typename T>
	struct values_type : std::vector<T>
	{
		using std::vector<T>::vector;
	};

	// вектор состояний детекторов
	struct values_state_type : std::vector<value_state_type>
	{
		using std::vector<value_type>::vector;

		// функция преобразования состояний по заданным значениям
		template<typename T>
		void transform(const values_type<T>& values, std::function<void(COscDetectorBase::value_state_type& vs, const  COscDetectorBase::value_type& v)> func)
		{
			// проверяем, чтобы размерность вектора детекторов была равна размерности вектора заданных значений
			if (size() != values.size())
				throw std::runtime_error("values_state_type::transform - values sizes mismatch");

			// параллельно проходим состояния детекторов и заданные значения
			auto dst{ values.begin() };
			for (auto& v : *this)
			{
				// для доступа к значению используем шаблон - можно работать и с double, и с const double*
				func(v, COscDetectorBase::Value(*dst));
				dst++;
			}
		}

		// присвоение состояний детекторов из вектора заданных значений
		template<typename T>
		values_state_type& operator = (const values_type<T>& value)
		{
			// если состояния не были заданы, задаем размерность по размерности заданных значений
			if (empty())
				resize(value.size());

			// копируем значения в состояния с помощью transform
			transform(value, [](COscDetectorBase::value_state_type& vs, const COscDetectorBase::value_type& v)
				{
					vs = v;
				}
			);
			return *this;
		}
	};

	// описание точки времени с вектором значений
	template<typename T>
	struct time_point_type : point_time_type
	{
		using point_time_type::point_time_type;
		values_type<T> value;
		time_point_type(const double time, const values_type<T>& values) : point_time_type(time), value(values) {}
	};

	// вектор указателей на внешние значения
	time_point_type <const value_type*> pointers;

	// описание точки времени состояний детекторов
	struct time_point_state : point_time_type
	{
		// вектор состояний детекторов
		values_state_type value;

		// присвоение значений состояний детекторов из заданной точки времени
		template<typename U>
		time_point_state& operator = (const time_point_type<U>& timepoint)
		{
			if (check(timepoint))
			{
				time = timepoint.time;
				value = timepoint.value;
			}
			return *this;
		}
		
		// добавление новой точки времени для контроля в детекторы
		template<typename U>
		void add(const time_point_type<U>& timepoint, const double rtol, const double atol)
		{
			// проверяем новую точку на возврастание относительно текущей
			if (!check(timepoint))
				return;

			double newtime(timepoint.time), oldtime(time);
			// с помощью transform добавляем точку в каждый детектор и обновляем их состояния
			value.transform(timepoint.value, [&oldtime, &newtime, rtol, atol](COscDetectorBase::value_state_type& vs, const COscDetectorBase::value_type& v)
				{
					// определяем разность между текущим значением и заданным
					double diff{ vs.value - v };
					// в зависимости от состояния детектора
					switch (vs.state)
					{
					case eState::wait_min_or_max:
						// если состояние не определено, 
						// определяем его по знаку отклонения текущего значения
						if (diff < 0)
							vs.state = eState::wait_max;
						else if (diff > 0)
							vs.state = eState::wait_min;
						break;
					case eState::wait_max:
						// если ждем максимум, то нужно чтобы новая точка была меньше текущей
						if (diff > 0)
						{
							// если новая точка меньше текущей переключаемся на поиск минимума
							vs.state = eState::wait_min;
							// если уже есть набранные точки, проверяем, чтобы
 							// найденный максимум не превышал сохраненный более чем на точность расчета
							COscDetectorBase::extreme_point_type newmax{ oldtime, vs.value };
							if (!vs.maxs.empty())
							{
								// проверяем отличие сохраненного максимума от найденного с учетом допустимой точности расчета
								const double d{ (vs.maxs.back().value - vs.value) / (std::abs(vs.value) * rtol + atol) };
								if (d < -1.0)
								{
									// если отклонение более чем на точность расчета,
									// сбрасываем накопленные максимумы и добавляем текущий
									vs.clear_min_max();
									vs.maxs.push_back(newmax);
								}
								else if(d < 0.0) // если в пределах точности, просто обновляем сохраненный максимум
									vs.maxs.back() = newmax;
								else
									vs.maxs.push_back(newmax);		// если макимум меньше, накапливаем его
							}
							else
								vs.maxs.push_back(newmax);			// добавляем максимум если максимумы пустые
						}
						break;
					case eState::wait_min:
						// если ждем минимум, то нужно чтобы новая точка была больше текущей
						if (diff < 0)
						{
							// переключаемся в режим поиска максимума
							vs.state = eState::wait_max;
							// если уже есть набранные точки, проверяем, чтобы
 							// найденный минимум не был меньше сохраненного на точность расчета
							COscDetectorBase::extreme_point_type newmin{ oldtime, vs.value };
							if (!vs.mins.empty())
							{
								// проверяем отличие сохраненного максимума от найденного с учетом допустимой точности расчета
								double d = (vs.mins.back().value - vs.value) / (std::abs(vs.value) * rtol + atol);
								if (d > 1.0)
								{
									// если отклонение более чем на точность, сбрасываем минимумы
									vs.clear_min_max();
									vs.mins.push_back(newmin);
								}
								else if(d > 0.0)	// если в пределах точности - обновляем минимум
									vs.mins.back() = newmin;
								else
									vs.mins.push_back(newmin);	// если минимум больше - накапливаем его
							}
							else
								vs.mins.push_back(newmin);		// добавляем минимум если минимумы пусты
						}
						break;
					}
					// сохраняем заданное значение в качестве предыдущего
					vs.value = v;
				});
			// обновляем время среза детекторов
			time = timepoint.time;
		}
	};

	// состояние вектора детекторов
	time_point_state old_time_point;

public:
	// добавляет значения в детекторы
	template<typename T>
	void add(const time_point_type<T>& timepoint, const double rtol, const double atol)
	{
		if (old_time_point.value.empty())
			old_time_point = timepoint;						// если не было сохранено значений, копируем новые
		else
			old_time_point.add(timepoint, rtol, atol);		// если значения уже были сохранены, добавляем новую точку времени
	}

	// добавляет указатель на значение, которое нужно будет контролировать
	void add_value_pointer(const value_type* value_ptr)
	{
		pointers.value.push_back(value_ptr);
	}

	// возвращает состояния детекторов
	const values_state_type& channels() const
	{
		return old_time_point.value;
	}

	// выполняет проверку значений от сохраненных указателей
	bool check_pointed_values(double time, const double rtol, const double atol)
	{
		// ставим заданное время
		pointers.time = time;
		// добавляем точку времени с указателей
		add(pointers, rtol, atol);
		return true;
	}

	// возвращает true, если по всем контролируемым значениям зафиксировано затухание
	bool has_decay(size_t decay_check_cycles)
	{
		bool bHasDecay(false);
		size_t Decayed(0);
		for (const auto& channel : old_time_point.value)
		{
			// если есть канал с обнаруженным отклонением
			if(channel.state != eState::wait_min_or_max)
				if (channel.mins.size() > decay_check_cycles  && channel.maxs.size() > decay_check_cycles)
				{
					// и в канале накоплено более чем заданное количество минимумов и максимумов
					// определяем затухание
					bHasDecay = true;
					Decayed++;
				}
				else
				{
					// если в канале нет нужного количества минимумов и максимумов, затухания нет
					// и дальше проверять смысла нет
					bHasDecay = false;
					break;
				}
		}
		return bHasDecay;
	}
};

// прямая конверсия из value_type
template<> inline COscDetectorBase::value_type COscDetectorBase::Value<COscDetectorBase::value_type>(COscDetectorBase::value_type value) { return value; }
// конверсия из указателя value_type
template<> inline COscDetectorBase::value_type COscDetectorBase::Value<const COscDetectorBase::value_type*>(const COscDetectorBase::value_type* value) { return *value; }


