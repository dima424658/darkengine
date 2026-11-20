///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/nonover.cpp $
// $Author: TOML $
// $Date: 1998/06/10 13:58:12 $
// $Revision: 1.2 $
//
// Default, non override versions of functions

#include <stdlib.h>

#pragma code_seg("lgalloc")

EXTERN BOOL LGAllocOverride()
{
   return FALSE;
}
