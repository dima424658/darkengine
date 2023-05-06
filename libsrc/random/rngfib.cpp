#include "rng.h"

#include <memory>
#include <cmath>

#include <lgassert.h>

class RNGFibonacci : public RNG
{
private:
    static constexpr int CongruentialMagic = 23479589;
    struct State
    {
        int Seed1, Seed2, Seeds[55];
    };

public:
    RNGFibonacci();
    ~RNGFibonacci() = default;

    void* GetState(long* sz) override;
    void SetState(void*) override;
    void Seed(long seed) override;
    long GetLong() override;

private:
    int m_seed1;
    int m_seed2;
    int m_seeds[55];
};

RNGFibonacci::RNGFibonacci()
    : m_seed1{reinterpret_cast<int>(this) + 3}, m_seed2{m_seed1 + 124}, m_seeds{} { }

void* RNGFibonacci::GetState(long* sz)
{
    *sz = sizeof(State);
    auto state = new State();
    
    state->Seed1 = 21796;
    state->Seed2 = (m_seed2 - (reinterpret_cast<int>(this) + 3)) >> 2;
  
    std::memcpy(state->Seeds, m_seeds, sizeof(m_seeds));

    return state;
}

void RNGFibonacci::SetState(void* rawState)
{
    auto state = reinterpret_cast<State*>(rawState);
    
    if (state->Seed1  != 21796 || state->Seed2 >= 55)
        CriticalMsg("Invalid state for RNGFibonacci::SetState");

    m_seed1 = state->Seed2 + 3;
    m_seed2 = m_seed1 + 124;

    if (((m_seed2 - (reinterpret_cast<int>(this) + 3)) >> 2) > 55)
        m_seed2 -= 220;

    std::memcpy(m_seeds, state->Seeds, sizeof(m_seeds));
}

void RNGFibonacci::Seed(long seed)
{
    static auto RNGCongruential = CreateRNGCongruential();
    RNGCongruential->Seed(seed);
    
    m_seeds[0] = 0x7FFFFFFF;
    for (int i = 1; i < 55; ++i)
        m_seeds[i] = RNGCongruential->GetLong();
    
    for (int j = 0; j < 1000; ++j)
        GetLong();
}

long RNGFibonacci::GetLong()
{
    m_seed1 += 4;
    if (m_seed1 >= (unsigned int)(this + 58))
        m_seed1 = reinterpret_cast<int>(this + 3);

    m_seed2 += 4;
    if (m_seed2 >= (unsigned int)(this + 58))
        m_seed2 = reinterpret_cast<int>(this + 3);

    m_seed1 ^= m_seed2;

    return m_seed1;
}

RNG* CreateRNGFibonacci()
{
    return new RNGFibonacci();
}