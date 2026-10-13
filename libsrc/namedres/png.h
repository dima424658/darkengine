#pragma once

#include <resapi.h>

struct grs_bitmap;

grs_bitmap* ResPngReadImage(IStoreStream* pStream,
                            IResMemOverride* pResMem);
uint8* ResPngReadPalette(IStoreStream* pStream,
                         IResMemOverride* pResMem);
