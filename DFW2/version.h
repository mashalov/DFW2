#pragma once

#include "fmt/core.h"
#include "fmt/format.h"

namespace DFW2
{
    using VersionInfo = std::array<size_t, 4>;
}

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
