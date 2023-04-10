/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/doorprop.h,v 1.9 2000/01/29 13:41:01 adurant Exp $
//
//
//
#pragma once

#ifndef __DOORPROP_H
#define __DOORPROP_H

#include <matrixs.h>

#include <property.h>

#define  ROTATING_DOOR     (0)
#define  TRANSLATING_DOOR  (1)


EXTERN  rot_door_lock;

#define ROT_DOOR_LOCK    rot_door_lock = TRUE
#define ROT_DOOR_UNLOCK  rot_door_lock = FALSE


#ifdef __cplusplus

struct sDoorProp
{
   int type;

   mxs_real closed;
   mxs_real open;

   mxs_real base_speed;

   int  axis;
   int  status;

   BOOL hard_limits;

   mxs_real sound_blocking;
   BOOL     vision_blocking;

   mxs_real push_mass;

   // These are hidden
   mxs_vector base_closed_location;
   mxs_vector base_open_location;

   mxs_vector base_location;
   mxs_angvec base_angle;

   mxs_real   base;

   int room1;
   int room2;
};

struct sRotDoorProp : public sDoorProp
{
   BOOL clockwise;

   mxs_angvec base_closed_facing;
   mxs_angvec base_open_facing;
};

struct sTransDoorProp : public sDoorProp
{
};

////////////////////////////////////////////////////////////////////////////////


#undef  INTERFACE
#define INTERFACE IRotDoorProperty
DECLARE_PROPERTY_INTERFACE(IRotDoorProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sRotDoorProp*); 
};

#define PROP_ROTDOOR_NAME "RotDoor"

////////////////////////////////////////////////////////////////////////////////

#undef  INTERFACE
#define INTERFACE ITransDoorProperty
DECLARE_PROPERTY_INTERFACE(ITransDoorProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sTransDoorProp*); 
};


#define PROP_TRANSDOOR_NAME "TransDoor"

////////////////////////////////////////////////////////////////////////////////

#else

typedef struct ITransDoorProperty ITransDoorProperty;
typedef struct IRotDoorProperty IRotDoorProperty;
typedef struct sDoorProp sDoorProp;

#endif

EXTERN ITransDoorProperty *TransDoorPropertyInit();
EXTERN void                TransDoorPropertyTerm();

EXTERN IRotDoorProperty *RotDoorPropertyInit();
EXTERN void              RotDoorPropertyTerm();

EXTERN sDoorProp *GetDoorProperty(int o_id);

EXTERN IRotDoorProperty   *g_pRotDoorProperty;
EXTERN ITransDoorProperty *g_pTransDoorProperty;

////////////////////////////////////////////////////////////////////////////////

#endif // __DOORPROP_H









