#pragma once

#include <storeapi.h>
#include <resapi.h>
#include <grs.h>

grs_bitmap* ResCelReadImage(IStoreStream* pStream, IResMemOverride* pResMem);
uint8* ResCelReadPalette(IStoreStream* pStream, IResMemOverride* pResMem);