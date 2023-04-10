/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/scrnedit.h,v 1.1 1998/10/07 14:30:12 mahk Exp $
#pragma once  
#ifndef __SCRNEDIT_H
#define __SCRNEDIT_H

//------------------------------------------------------------
// Editing tools for sScrnMode
//
EXTERN BOOL EditScreenMode(const char* title, struct sScrnMode* mode); 

EXTERN void ScrnEditInit(void); 
EXTERN void ScrnEditTerm(void); 


#endif // __SCRNEDIT_H
