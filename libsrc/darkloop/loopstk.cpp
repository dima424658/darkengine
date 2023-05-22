#include "loopstk.h"

cLoopModeStack::cLoopModeStack()
{
}

void cLoopModeStack::Push(sModeData data)
{
    Prepend(new cLoopModeElem{ &data });
}

sModeData* cLoopModeStack::Pop(sModeData* result)
{
    auto first = GetFirst();
    if (first)
    {
        *result = first->modedata;
        Remove(first);
        delete first;
    }
    else
    {
        *result = {};
        result->flags = 128;
    }

    return result;
}

sModeData* cLoopModeStack::Top()
{
    return &GetFirst()->modedata;
}