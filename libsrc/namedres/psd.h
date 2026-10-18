#pragma once

#include <storeapi.h>
#include <resapi.h>

struct grs_bitmap;

grs_bitmap* ResPsdReadImage(IStoreStream* pStream,
                            IResMemOverride* pResMem);
