#pragma once

#include <storeapi.h>
#include <resapi.h>
#include <grs.h>

grs_bitmap* ResBmpReadImage(IStoreStream* pStream, IResMemOverride* pResMem);
uint8* ResBmpReadPalette(IStoreStream* pStream, IResMemOverride* pResMem);