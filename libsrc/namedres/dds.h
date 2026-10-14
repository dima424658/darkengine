#pragma once

#include <resapi.h>

struct grs_bitmap;

grs_bitmap* ResDdsReadImage(IStoreStream* pStream,
                            IResMemOverride* pResMem);
uint8* ResDdsReadPalette(IStoreStream* pStream,
                         IResMemOverride* pResMem);
