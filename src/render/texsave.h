/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/texsave.h,v 1.2 1998/04/13 20:17:48 mahk Exp $
#pragma once  
#ifndef __TEXSAVE_H
#define __TEXSAVE_H

EXTERN BOOL texture_Save(ITagFile *file); 
EXTERN BOOL texture_Load(ITagFile *file); 

// enable texture save/loading (defaults true)
// returns old state
EXTERN BOOL texture_EnableSaveLoad(BOOL enabled); 
#endif // __TEXSAVE_H
