#pragma once

#include <types.h>

/* Modern platforms do not require the engine to rewrite executable pages. */
#ifdef _WIN32
EXTERN BOOL LGAPI MakeAllCodeWritable(void);
EXTERN BOOL LGAPI MakeFunctionWritable(void *function, unsigned size);
#else
#define MakeAllCodeWritable() ((void)0)
#define MakeFunctionWritable(function, size) ((void)0)
#endif
