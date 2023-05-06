#include "resapi.h"

#ifndef SHIP
BOOL g_fResPrintAccesses;
BOOL g_fResPrintDrops;
#endif

tResult LGAPI _Res2Create(REFIID, IResMan** ppResMan, IUnknown* pOuter)
{
    return kError; // TODO
}