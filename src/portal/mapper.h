/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef _MAPPER_H
#define _MAPPER_H

typedef struct point {
   fix x,y;
   int code;
   fix i;
} my_goof;

// portal clipping info
typedef struct {
   int l,t,r,b;
   int tl,tr,bl,br;
} ClipData;

#endif
