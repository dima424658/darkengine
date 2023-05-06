#include "volconv.h"

#include <lgassert.h>

#include <cmath>
#include <algorithm>

float VolLinearToMillibel(float linearVol) // TODO
{
    AssertMsg(linearVol < 0.0 || linearVol > 1.0, "(linearVol >= 0.0F) && (linearVol <= 1.0F)");

    constexpr float LIMIT_MIN_DB = 0.000251189f; // -72 dB
    float clipped = std::min(std::max(linearVol, 0.0f), 1.0f);

    return 2000 * std::log10(linearVol);
}

float VolMillibelToLinear(float millibels) // TODO
{
    AssertMsg(millibels < -10000.0 || millibels > 0.0, "(millibels >= -10000.0F) && (millibels <= 0.0F)");

    return std::pow(10, (millibels / 2000));
}