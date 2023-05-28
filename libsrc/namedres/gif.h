#pragma once

#include <storeapi.h>
#include <resapi.h>
#include <grs.h>

grs_bitmap* ResGifReadImage(IStoreStream* pStream, IResMemOverride* pResMem);
uint8* ResGifReadPalette(IStoreStream* pStream, IResMemOverride* pResMem);