#pragma once

#include "fmt/core.h"
#include "fmt/format.h"
#include <complex>

template <> struct fmt::formatter<std::complex<double>> : fmt::formatter<double>
{
	template <typename FormatContext>
	auto format(const std::complex<double>& value, FormatContext& ctx) 
	{
		fmt::formatter<double>::format(value.real(), ctx);
		format_to(ctx.out(), "{}j", value.imag() > 0.0 ? '+' : '-');
		fmt::formatter<double>::format(std::abs(value.imag()), ctx);
		return ctx.out();
	}
};