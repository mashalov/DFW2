#pragma once
#include <string>
#include <algorithm>
#include <fmt/core.h>
#include <fmt/format.h>
#define NOMINMAX
#ifdef _MSC_VER
#include <Windows.h>
#endif
#include "../../DFW2/stringutils.h"

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename T>
std::string to_string(const T& t)
{
    std::string str{ fmt::format("{}", t) };
    int offset{ 1 };
    if (str.find_last_not_of('0') == str.find('.'))
    {
        offset = 0;
    }
    str.erase(str.find_last_not_of('0') + offset, std::string::npos);
    return str;
}

#ifdef _MSC_VER
#define EXCEPTIONMSG(x) { _ASSERTE(!(x)); throw std::runtime_error(fmt::format("{} {} in {} {}", __FUNCSIG__, (x), __FILE__, __LINE__));}
#else
#define EXCEPTIONMSG(x) { throw std::runtime_error(fmt::format("{} {} in {} {}", __PRETTY_FUNCTION__, (x), __FILE__, __LINE__));}
#endif
#define EXCEPTION  EXCEPTIONMSG("");

#ifndef _MSC_VER
#define _ASSERTE(x)
#endif
