/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/famedit.h,v 1.1 1998/04/12 17:16:08 mahk Exp $
#pragma once  
#ifndef __FAMEDIT_H
#define __FAMEDIT_H

#ifdef EDITOR
EXTERN BOOL _familyRemapBrushes(int *tex_swap, int cnt); 
EXTERN void family_edit_init(void); 
#else
#define _familyRemapBrushes(foo,bar) TRUE
#define family_edit_init()
#endif 


#endif // __FAMEDIT_H
