#pragma once

#include <algorithm>
#include <iterator>
#include <utility>

// Macro for constexpr
#if __cplusplus >= 201103L
#define LG_CONSTEXPR constexpr
#else
#define LG_CONSTEXPR
#endif

namespace relg
{
#if __cplusplus >= 201103L
    template <typename T>
    using min = std::min<T>;

    template <typename T>
    using max = std::max<T>;
#else
    template <typename T>
    inline const T &min(const T &lhs, const T &rhs)
    {
        if (rhs < lhs)
            return rhs;
        return lhs;
    }

    inline const T &min(const T &lhs, const T &rhs)
    {
        if (lhs < rhs)
            return lhs;
        return rhs;
    }
#endif

#if __cplusplus >= 201703L
    template <typename T>
    using clamp = std::clamp<T>;

    template <typename T>
    using size = std::size<T>;
#else
    template <typename T>
    LG_CONSTEXPR inline const T &clamp(const T &value, const T &low, const T &high)
    {
        assert(!(high < low));
        return min(max(value, low), high);
    }

    template <typename T, size_t N>
    LG_CONSTEXPR inline size_t size(const T (&)[N]) LG_NOEXCEPT
    {
        return N;
    }
#endif
};