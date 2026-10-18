#include "volconv.h"

#include <lgassert.h>

#include <cmath>
#include <array>

namespace
{
constexpr int kVolumeTableLast = 31;
constexpr float kMinMillibels = -10000.0f;

const std::array<float, kVolumeTableLast + 1>& VolumeTable()
{
    static const std::array<float, kVolumeTableLast + 1> table = [] {
        std::array<float, kVolumeTableLast + 1> values{};
        values[0] = kMinMillibels;
        for (int i = 1; i <= kVolumeTableLast; ++i)
            values[i] = 1000.0f * std::log2(static_cast<float>(i) / kVolumeTableLast);
        return values;
    }();
    return table;
}
}

float VolLinearToMillibel(float linearVol)
{
    AssertMsg(linearVol >= 0.0f && linearVol <= 1.0f,
              "(linearVol >= 0.0F) && (linearVol <= 1.0F)");

    if (linearVol <= 0.0f)
        return kMinMillibels;
    if (linearVol >= 1.0f)
        return 0.0f;

    const auto& table = VolumeTable();
    const float scaled = linearVol * kVolumeTableLast;
    const int left = static_cast<int>(scaled);
    const float fraction = scaled - left;
    return table[left] + fraction * (table[left + 1] - table[left]);
}

float VolMillibelToLinear(float millibels)
{
    AssertMsg(millibels >= kMinMillibels && millibels <= 0.0f,
              "(millibels >= -10000.0F) && (millibels <= 0.0F)");

    if (millibels <= kMinMillibels)
        return 0.0f;
    if (millibels >= 0.0f)
        return 1.0f;

    const auto& table = VolumeTable();
    int left = 0;
    int right = kVolumeTableLast;
    while (right - left > 1)
    {
        const int middle = left + (right - left) / 2;
        if (table[middle] <= millibels)
            left = middle;
        else
            right = middle;
    }

    const float fraction = (millibels - table[left]) /
                           (table[right] - table[left]);
    return (left + fraction) / kVolumeTableLast;
}
