// $Header: x:/prj/tech/libsrc/lgd3d/RCS/d3dpoint.h 1.1 1997/10/09 15:31:05 KEVIN Exp $

#ifndef __D3DPOINT_H
#define __D3DPOINT_H

typedef struct lgd3ds_point
{
   float sx;
   float sy;
   float sz;
   float rhw;
   uchar r;
   uchar g;
   uchar b;
   uchar a;
   uchar flags;
   uchar sg;
   uchar sb;
   uchar sa;
   float u;
   float v;
} lgd3ds_point;

#endif
