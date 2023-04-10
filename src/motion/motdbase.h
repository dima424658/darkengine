/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/motdbase.h,v 1.6 2000/01/31 09:50:57 adurant Exp $
#pragma once

// declaration of motdatabase structs that are forward declared in motdbtyp.h
#ifndef __MOTDBASE_H
#define __MOTDBASE_H

#include <label.h>
#include <motdbtyp.h>
#include <matrixs.h>

// blending types
#define BLEND_NONE 0
#define BLEND_DEFAULT 1

#define kMSF_NECK_IS_FIXED 0x1
#define kMSF_IS_TURN       0x2
#define kMSF_WANT_NO_XLAT  0x4
#define kMSF_IS_LOCO       0x8

struct sMotStuff
{
   ulong flags;
   ushort blendLength;
   mxs_ang endDirAction;
   mxs_vector xlat; // actual xlat of motion
   float duration;
};

struct sMotDesc
{
   Label name;
   sMotStuff stuff;
};

struct sTagDesc
{
   Label type;
   int value; /* could support value ranges, but don't think that's
                   needed for motion schemas. will add if needed */
};

struct sMotSchemaDesc
{
   Label name;
   BOOL archIsString;
   Label archName;
   int archInt;
   int actor;
   int nTags;
   sTagDesc *pTags;
   int nMots;
   sMotDesc *pMots;
   float duration;
   float timeWarp;
   float stretch;
   float distance;
};


#endif // motdbase.h

