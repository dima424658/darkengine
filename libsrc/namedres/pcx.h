#pragma once

#include <storeapi.h>
#include <resapi.h>
#include <grs.h>

grs_bitmap* ResPcxReadImage(IStoreStream* pStream, IResMemOverride* pResMem);
uint8* ResPcxReadPalette(IStoreStream* pStream, IResMemOverride* pResMem);