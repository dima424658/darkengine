/*
 * $Source: x:/prj/tech/libsrc/sound/RCS/dsnddynf.h $
 * $Log: dsnddynf.h $
 * Revision 1.3  1996/07/30  17:51:20  PATMAC
 * anarkmd mc5 header comment style
 * 
 * Revision 1.2  1996/07/30  17:35:50  PATMAC
 * Minor cleanups
 * 
 * Revision 1.1  1996/07/30  11:27:39  PATMAC
 * Initial revision
 */

// Dynamic function loading

#ifndef __DSNDDYNF_H
#define __DSNDDYNF_H

#include <dsound.h>
#include <dynfunc.h>

DeclDynFunc_( HRESULT, WINAPI, DirectSoundCreate8, (LPCGUID lpGUID, LPDIRECTSOUND8 *ppDS8, LPUNKNOWN pUnkOuter) );
#define DynDirectSoundCreate8 (DynFunc(DirectSoundCreate8).GetProcAddress())

BOOL LoadDirectSound();

#endif /* !__DSNDDYNF_H */
