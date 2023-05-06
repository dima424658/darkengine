#include <loopapi.h>
       
ILoopClient * LGAPI
CreateSimpleLoopClient(tLoopClientReceiveMessageFunc pCallback,
                       void *                        pContext,
                       const sLoopClientDesc *       pClientDesc)
{
    return nullptr; // TODO
}

ILoopClientFactory* LGAPI CreateLoopFactory(const sLoopClientDesc** descs)
{
    return nullptr; // TODO
}

tResult LGAPI _LoopManagerCreate(REFIID, ILoopManager** ppLoopManager, IUnknown* pOuter, unsigned nMaxModes)
{
    return kError; // TODO
}
