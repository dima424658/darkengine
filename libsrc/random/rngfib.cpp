#include "rng.h"

#include <cstring>

#include <lgassert.h>

class RNGFibonacci : public RNG
{
private:
    static constexpr int StateMagic = 21796;
    static constexpr int StateSize = 55;
    static constexpr int Lag = 31;
    struct State
    {
        int Magic, Position, Seeds[StateSize];
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
    int m_seeds[StateSize];
};

RNGFibonacci::RNGFibonacci()
    : m_seed1{0}, m_seed2{Lag}, m_seeds{} { }

void* RNGFibonacci::GetState(long* sz)
{
    if (sz)
        *sz = sizeof(State);
    auto state = new State();
    
    state->Magic = StateMagic;
    state->Position = m_seed2;
  
    std::memcpy(state->Seeds, m_seeds, sizeof(m_seeds));

    return state;
}

void RNGFibonacci::SetState(void* rawState)
{
    auto state = reinterpret_cast<State*>(rawState);
    
    if (state->Magic != StateMagic || state->Position < 0 || state->Position >= StateSize)
        CriticalMsg("Invalid state for RNGFibonacci::SetState");

    m_seed2 = state->Position;
    m_seed1 = (m_seed2 + StateSize - Lag) % StateSize;

    std::memcpy(m_seeds, state->Seeds, sizeof(m_seeds));
}

void RNGFibonacci::Seed(long seed)
{
    static auto RNGCongruential = CreateRNGCongruential();
    RNGCongruential->Seed(seed);
    
    m_seeds[0] = 0x7FFFFFFF;
    for (int i = 1; i < StateSize; ++i)
        m_seeds[i] = RNGCongruential->GetLong();
    
    for (int j = 0; j < 1000; ++j)
        GetLong();
}

long RNGFibonacci::GetLong()
{
    m_seed1 = (m_seed1 + 1) % StateSize;
    m_seed2 = (m_seed2 + 1) % StateSize;
    m_seeds[m_seed1] ^= m_seeds[m_seed2];

    return m_seeds[m_seed1];
}

RNG* CreateRNGFibonacci()
{
    return new RNGFibonacci();
}
