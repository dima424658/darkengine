#pragma once

#include <types.h>

#pragma pack(push, 1)
typedef struct sDisplayMode
{
   int width;
   int height;
   int bit_depth;
} sDisplayMode;
#pragma pack(pop)

#pragma pack(push, 1)
typedef enum eDisplayScope
{
   kDisplayCurrentDisplay,
   kDisplayAllDisplays
} eDisplayScope;
#pragma pack(pop)

EXTERN int DisplayGetModes(eDisplayScope scope,
                                   sDisplayMode *modes,
                                   int capacity,
                                   int *native_width,
                                   int *native_height);
