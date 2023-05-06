#include "rng.h"
#include "random.h"

#define _USE_MATH_DEFINES
#include <cmath>

float RNG::GetFloat()
{
    auto result = 0x7FFFFF & GetLong() | 0x3F800000;

    return *reinterpret_cast<float*>(result) - 1.0;
}

float RNG::GetNorm()
{
    return std::cos(GetFloat() * 2.0 * M_PI) * std::sqrt(std::log(GetFloat() * -2.0));
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