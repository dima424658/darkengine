/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/mediag8r.h,v 1.1 1998/08/21 00:00:43 mahk Exp $
#pragma once  
#ifndef __MEDIAG8R_H
#define __MEDIAG8R_H

////////////////////////////////////////////////////////////
// ACT/REACT MEDIA PROPAGATOR API
//
// This is the propagator that allows media (well, flow groups, really)
// to stimulate objects.
// 

//
// Just initialize it, and it hooks into physics
//
EXTERN void MediaPropagatorInit(void); 
EXTERN void MediaPropagatorTerm(void); 


#endif // __MEDIAG8R_H
