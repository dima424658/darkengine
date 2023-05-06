#include "rng.h"

#include <cmath>
#include <random>
#include <lgassert.h>

class RNGCongruential : public RNG
{
private:
    static constexpr int CongruentialMagic = 23479589;
    struct State
    {
        int Magic, Seed;
    };

public:
    RNGCongruential();
    ~RNGCongruential() = default;

    void* GetState(long* sz) override;
    void SetState(void*) override;
    void Seed(long seed) override;
    long GetLong() override;

private:
    int m_seed;
};

RNGCongruential::RNGCongruential()
    : m_seed{} { }

void* RNGCongruential::GetState(long* sz)
{
    auto result = new State();

    result->Magic = CongruentialMagic;
    result->Seed = m_seed;

    *sz = sizeof(State);

    return result;
}

void RNGCongruential::SetState(void* rawState)
{
    auto state = reinterpret_cast<State*>(rawState);

    if (state->Magic != CongruentialMagic)
        CriticalMsg("Invalid state for RNGCongruential::SetState", aXPrjTechLibsrc_1593, 33);

    m_seed = state->Seed;
}

void RNGCongruential::Seed(long seed)
{
    m_seed = seed;
}

long RNGCongruential::GetLong()
{
    constexpr auto a = 1664525;
    constexpr auto c = 12345;
    m_seed = a * m_seed + c;
    return m_seed >> 1;
}

RNG* CreateRNGCongruential()
{
    return new RNGCongruential();
}