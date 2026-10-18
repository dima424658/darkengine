#include "rng.h"
#include "random.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdint>
#include <cstring>

float RNG::GetFloat()
{
    const std::uint32_t bits
        = (static_cast<std::uint32_t>(GetLong()) & 0x007FFFFFu) | 0x3F800000u;
    float result;
    static_assert(sizeof(result) == sizeof(bits), "RNG float bit conversion requires 32-bit floats");
    std::memcpy(&result, &bits, sizeof(result));

    return result - 1.0f;
}

float RNG::GetNorm()
{
    return static_cast<float>(std::cos(GetFloat() * 2.0 * M_PI)
        * std::sqrt(std::log(GetFloat() * -2.0)));
}

long RNG::GetRange(long max)
{
    return GetLong() % max;
}

RNG* gRNGCongruential = nullptr;
RNG* gRNGFibonacci = nullptr;

EXTERN void RandInit(long seed)
{
    gRNGCongruential = CreateRNGCongruential();
    gRNGCongruential->Seed(seed);

    gRNGFibonacci = CreateRNGFibonacci();
    gRNGFibonacci->Seed(seed);
}
