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

		void add(const time_point_type& timepoint)
		{
			if (!check(timepoint))
				return;

			double newtime(timepoint.time), oldtime(time);
			value.transform(timepoint.value, [&oldtime, &newtime](COscDetectorBase::value_state_type& vs, const COscDetectorBase::value_type& v)
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
// 							// найденный максимум был меньше сохраненного
							// Если нет - то очищаем набранные точки
							if (!vs.maxs.empty() && vs.maxs.back().value < vs.value)
								vs.clear_min_max();
							// добавляем в набранные точки текущую
							vs.maxs.push_back({ oldtime, vs.value });
						}
						break;
					case eState::wait_min:
						if (diff < 0)
						{
							vs.state = eState::wait_max;
							// если уже есть набранные точки, проверяем, чтобы
// 							// найденный минимум был больше сохраненного
							// Если нет - то очищаем набранные точки
							if (!vs.mins.empty() && vs.mins.back().value > vs.value)
								vs.clear_min_max();
							// добавляем в набранные точки текущую
							vs.mins.push_back({ oldtime, vs.value });
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
	void add(const time_point_type& timepoint)
	{
		if (old_time_point.value.empty())
			old_time_point = timepoint;
		else
			old_time_point.add(timepoint);
	}

	void add_value_pointer(const double* value_ptr)
	{
		pointers.push_back(value_ptr);
	}

	const values_state_type& channels() const
	{
		return old_time_point.value;
	}

	bool check_pointed_values(double time)
	{
		values_type vals;
		std::transform(pointers.begin(), pointers.end(), std::back_inserter(vals), [](const auto& val) { return *val; });
		const time_point_type timepoint(time,  vals);
		add(timepoint);
		return true;
	}
};

