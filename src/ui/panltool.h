/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/panltool.h,v 1.3 1999/10/11 16:21:51 adurant Exp $
#pragma once  
#ifndef __PANLTOOL_H
#define __PANLTOOL_H

////////////////////////////////////////////////////////////
// Simple panel mode tools
//

EXTERN void InitPanelTools(void); 
EXTERN void TermPanelTools(void); 

// Switch to a static image, centered on on the existing background
EXTERN void StaticImagePanel(const char* respath, const char* image); 

// play a movie, using movie_path
// returns FALSE iff movie not found
EXTERN BOOL MoviePanel(const char* filename); 




#endif // __PANLTOOL_H
