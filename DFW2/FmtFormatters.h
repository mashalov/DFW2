#pragma once

#include "fmt/core.h"
#include "fmt/format.h"
#include <complex>
#include "version.h"

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

template <> struct fmt::formatter<DFW2::VersionInfo> : fmt::formatter<size_t>
{
    template <typename FormatContext>
    auto format(const DFW2::VersionInfo& value, FormatContext& ctx)
    {
        const char* sep = "";
        for (const auto& pos : value)
        {
            format_to(ctx.out(), sep);
            sep = ".";
            fmt::formatter<size_t>::format(pos, ctx);
        }
        return ctx.out();
    }
};
