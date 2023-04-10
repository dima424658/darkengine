/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/anim_txt.h,v 1.8 1999/08/05 17:28:33 Justin Exp $
//

#ifndef __ANIM_TXT_H
#pragma once
#define __ANIM_TXT_H

#include <comtools.h>

F_DECLARE_INTERFACE(IRes); 

// from anim_txt itself
EXTERN int   ectsAnimTxtTime;
EXTERN BOOL  ectsAnimLoading;

EXTERN BOOL  ectsAnimTxtIgnore(const char *name);
EXTERN void  ectsAnimTxtCheckLoad(IRes *me, BOOL portal_tmap, char *path, int txt_id);
EXTERN void *ectsAnimTxtGetFrame(IRes *me);
EXTERN void  ectsAnimTxtCheckFree(IRes *me);
EXTERN void  ectsAnimTxtUpdateAll(void);

// refresh this anim texture data for this resource
EXTERN BOOL  ectsAnimRefresh(IRes *pRes, struct sAnimTexProp *newvals);

// if you remap txt_ids, remap the anim secret data too
EXTERN void  ectsAnimTxtRemapAll(int *mapping, int cnt);

// actually in texmem itself, to track loading operations
EXTERN int   ectsSecretLoadIdx;
EXTERN int   ectsSecretLoadCopy[];

EXTERN void  ectsAnimTxtInit();
EXTERN void  ectsAnimTxtTerm();

#ifndef SHIP
// if you want to know overall size, frame count sort of things
EXTERN int  ectsAnimTxtGetRawSize(IRes *me, int *frames);
#endif

#endif  // __ANIM_TXT_H
