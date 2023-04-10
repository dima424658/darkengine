/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/editbr_.h,v 1.7 2000/01/29 13:11:33 adurant Exp $
#pragma once

#ifndef ___EDITBR_H
#define ___EDITBR_H

// MUST UPDATE CSGBRUSH.H if this CHANGES

#include <matrixs.h>
#include <editbrs.h>
#include <gridsnap.h>
#include <fix.h>

// _editbr.h
//  internals for edit brush
//  really just the actual structure

// this is for scale tx_scale to the floats needed by CSG
//  (we use it for the grid scale too, just for fun)

// wacky support for zany pow2 integer to float scale stuff
#define scale_pow2int_to_float(pow2i) (((float)(1<<pow2i))*(4.0/(float)(1<<16)))

#define TEXINFO_HACK_ALIGN 1  // for now, how you specify rotate to brush

struct _TexInfo
{
   short tx_id;
   fixang tx_rot;

   short tx_scale;
   
   ushort tx_x;
   ushort tx_y;
};

extern struct _TexInfo defTexInf;

enum eEditBrushFlags
{
   kEBF_None          = 0x00,
   kEBF_BeingDeleted  = 0x01,
};

#define EDITBR_MAX_FACES 12

struct _editBrush
{
      // 8
   short br_id;         // the unique runtime brush id, for CSG->brush mapping
   short timestamp;     // timestamp, filled in at CSG/build time
   int primal_id;       // what shape this is, 0==cube

      // 4
   short tx_id;         // what texture to use by default
   char  media;         // what the magic media code is
   char  flags;         // reserved for expansion

      // 24
   mxs_vector pos;      // center of the brush, for now
   mxs_vector sz;       // extents of brush

      // 8
   mxs_angvec ang;      // angular locale of brush
   short cur_face;      // after the above for alignment

      // 24
   Grid grid;
   uchar num_faces;     // number of elements in the following data

      // 8
   char edge;           // used to be "int pad0, pad1" in version <= 1.0
   char point;
   char use_flg;
   char group_id;
   int pad0;            // should we set edge and point count here? or imply?

      // ???
   TexInfo txs[EDITBR_MAX_FACES]; // textures for each face of the brush
                             // this is actually only as long as it needs
                             // to be (according to num_faces),
};

#endif
