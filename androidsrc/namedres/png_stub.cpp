#include <png.h>

#include <allocapi.h>

// The first Android bring-up deliberately omits libpng. Stock TDP, Gold, and
// Thief 2 resources use the legacy image formats, so returning no decoder here
// preserves retail gameplay while making the missing FM replacement-image
// capability explicit and contained.
grs_bitmap *ResPngReadImage(IStoreStream *, IResMemOverride *)
{
   return nullptr;
}

uint8 *ResPngReadPalette(IStoreStream *, IResMemOverride *)
{
   return nullptr;
}
