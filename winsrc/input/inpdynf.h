///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/winsrc/input/RCS/inpdynf.h $
// $Author: JON $
// $Date: 1997/10/06 20:43:08 $
// $Revision: 1.1 $
//
// Dynamic function loading for input

#ifndef __INPDYNF_H
#define __INPDYNF_H

#include <dynfunc.h>

DeclDynFunc_(HRESULT, WINAPI, DirectInputCreateA, (HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUT *lplpDirectInput, LPUNKNOWN punkOuter));
#define DynDirectInputCreate (DynFunc(DirectInputCreateA).GetProcAddress())

BOOL LoadDirectInput();

#endif /* !__INPDYNF_H */
