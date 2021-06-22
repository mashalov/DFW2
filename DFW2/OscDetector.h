#pragma once
#include <vector>
#include <list>
#include <stdexcept>
#include <algorithm>
#include <functional>
#include <iostream>

class COscDetectorBase
{
	enum class eState
	{
		wait_min_or_max,
		wait_min,
		wait_max,
	};

	struct point_time_type
	{
		double time = 0.0;

		point_time_type(double t) : time(t) {}
		point_time_type() : time(0.0) {}

		[[nodiscard]] bool check(const point_time_type& pointtime)
		{
			return pointtime.time > time;
			if (pointtime.time < time)
				throw std::runtime_error("point_time - time to be added is less than previous time");
		}
	};

	using value_type = double;
	
	struct extreme_point_type : point_time_type 
	{
		value_type value;
		extreme_point_type(double time, const value_type& val) : point_time_type(time), value(val) {}
	};

	struct value_state_type
	{
		value_type value;
		eState state = eState::wait_min_or_max;
		std::list<extreme_point_type> mins, maxs;
		
		value_state_type(value_type val) : value(val) {}
		value_state_type() : value(0.0) {}
		value_state_type& operator = (const value_type& val) 
		{ 
			value = val;
			return *this;
		}

		void clear_min_max()
		{
			mins.clear();
			maxs.clear();
		}
	};


	using value_pointers_type = std::list<const double*>;
	value_pointers_type pointers;

	struct values_type : std::vector<value_type>
	{
		using std::vector<value_type>::vector;
	};

	struct values_pointers_type : value_pointers_type
	{
		using value_pointers_type::value_pointers_type;
	};

	struct values_state_type : std::vector<value_state_type>
	{
		using std::vector<value_type>::vector;

		void transform(const values_type& values, std::function<void(COscDetectorBase::value_state_type& vs, const  COscDetectorBase::value_type& v)> func)
		{
			if (size() != values.size())
				throw std::runtime_error("values_state_type::transform - values sizes mismatch");

			auto dst = values.begin();
			for (auto& v : *this)
			{
				func(v, *dst);
				dst++;
			}
		}

		values_state_type& operator = (const values_type& value)
		{
			if (empty())
				resize(value.size());
			transform(value, [](COscDetectorBase::value_state_type& vs, const COscDetectorBase::value_type& v)
				{
					vs = v;
				}
			);
			return *this;
		}
	};


	struct time_point_type : point_time_type
	{
		values_type value;
		time_point_type(const double time, const values_type& values) : point_time_type(time), value(values) {}
	};

	struct time_point_state : point_time_type
	{
		values_state_type value;

		time_point_state& operator = (const time_point_type& timepoint)
		{
			if (check(timepoint))
			{
				time = timepoint.time;
				value = timepoint.value;
			}
			return *this;
		}

		void add(const time_point_type& timepoint, const double rtol, const double atol)
		{
			if (!check(timepoint))
				return;

			double newtime(timepoint.time), oldtime(time);
			value.transform(timepoint.value, [&oldtime, &newtime, rtol, atol](COscDetectorBase::value_state_type& vs, const COscDetectorBase::value_type& v)
				{
					double diff(vs.value - v);
					switch (vs.state)
					{
					case eState::wait_min_or_max:
						if (diff < 0)
							vs.state = eState::wait_max;
						else if (diff > 0)
							vs.state = eState::wait_min;
						break;
					case eState::wait_max:
						if (diff > 0)
						{
							vs.state = eState::wait_min;
							// если уже есть набранные точки, проверяем, чтобы
 							// найденный максимум не превышал сохраненный более чем на точность расчета
							if (!vs.maxs.empty())
							{
								double d = (vs.maxs.back().value - vs.value) / (std::abs(vs.value) * rtol + atol);
								if (d < -1.0)
								{
									// если отклонение более чем на точность расчета,
									// сбрасываем накопленные максимумы и добавляем текущий
									vs.clear_min_max();
									vs.maxs.push_back({ oldtime, vs.value });
								}
								else if(d < 0.0) // если в пределах точности, просто обновляем сохраненный максимум
									vs.maxs.back() = { oldtime, vs.value };
								else
									vs.maxs.push_back({ oldtime, vs.value });		// если макимум меньше, накапливаем его
							}
							else
								vs.maxs.push_back({ oldtime, vs.value });		// добавляем максимум если максимумы пустые
						}
						break;
					case eState::wait_min:
						if (diff < 0)
						{
							vs.state = eState::wait_max;
							// если уже есть набранные точки, проверяем, чтобы
// 							// найденный минимум не был меньше сохраненного на точность расчета
							if (!vs.mins.empty())
							{
								double d = (vs.mins.back().value - vs.value) / (std::abs(vs.value) * rtol + atol);
								if (d > 1.0)
								{
									// если отклонение более чем на точность, сбрасываем минимумы
									vs.clear_min_max();
									vs.mins.push_back({ oldtime, vs.value });
								}
								else if(d > 0.0)	// если в пределах точности - обновляем минимум
									vs.mins.back() = { oldtime, vs.value };		
								else
									vs.mins.push_back({ oldtime, vs.value });	// если минимум больше - накапливаем его
							}
							else
								vs.mins.push_back({ oldtime, vs.value });	// добавляем минимум если минимумы пусты
						}
						break;
					}
					vs.value = v;
				});
			time = timepoint.time;
		}
	};

	time_point_state old_time_point;

public:
	void add(const time_point_type& timepoint, const double rtol, const double atol)
	{
		if (old_time_point.value.empty())
			old_time_point = timepoint;
		else
			old_time_point.add(timepoint, rtol, atol);
	}

	void add_value_pointer(const double* value_ptr)
	{
		pointers.push_back(value_ptr);
	}

	const values_state_type& channels() const
	{
		return old_time_point.value;
	}

	bool check_pointed_values(double time, const double rtol, const double atol)
	{
		// TODO сделать прямой доступ к значениям на указателях без копирования
		values_type vals;
		std::transform(pointers.begin(), pointers.end(), std::back_inserter(vals), [](const auto& val) { return *val; });
		const time_point_type timepoint(time,  vals);
		// TODO сделать прямой доступ к значениям на указателях без копирования
		add(timepoint, rtol, atol);
		return true;
	}

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

