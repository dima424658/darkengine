/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/



// $Header: r:/t2repos/thief2/src/render/ctrast.h,v 1.2 1998/02/17 17:16:57 JAEMZ Exp $
#pragma once  
#ifndef __CTRAST_H
#define __CTRAST_H

////////////////////////////////////////////////////////////
// CONTRAST ENHANCEMENT

// Initialize the Contrast system
EXTERN void ContrastInit(void);

// Terminate the Contrast System
EXTERN void ContrastTerm(void);

// Update the contrast.  Do this right before the blit.
EXTERN void ContrastUpdate();

// Call this after you do the flash effect.
// Makes it look like you've been blinded by something
// super bright.
EXTERN void ContrastFlash(void);



#endif // __CTRAST_H



