///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/nonover.cpp $
// $Author: TOML $
// $Date: 1998/06/10 13:58:12 $
// $Revision: 1.2 $
//
// Default, non override versions of functions

#include <malloc.h>
#include <mallocdb.h>

#pragma code_seg("lgalloc")

EXTERN BOOL LGAllocOverride()
{
   return FALSE;
}

#ifndef SHIP
void * malloc_db(size_t size, const char *, int)
{
    return malloc(size);
}

void free_db(void * p, const char *, int)
{
    free(p);
}

void * realloc_db(void * p, size_t size, const char *, int)
{
    return realloc(p, size);
}
#endif
