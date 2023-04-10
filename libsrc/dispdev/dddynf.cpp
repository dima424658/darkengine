///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/dddynf.cpp $
// $Author: KEVIN $
// $Date: 1997/10/10 15:09:44 $
// $Revision: 1.2 $
//

#include <windows.h>
#include <ddraw.h>

#include <dddynf.h>

//
// _DirectDrawCreate@12
//
ImplDynFunc(DirectDrawCreate, "ddraw.dll", "DirectDrawCreate", 0);
ImplDynFunc(DirectDrawEnumerateA, "ddraw.dll", "DirectDrawEnumerateA", 0);


BOOL LoadDirectDraw()
{
    return (DynFunc(DirectDrawCreate).Load() &&
            DynFunc(DirectDrawEnumerateA).Load());
}
