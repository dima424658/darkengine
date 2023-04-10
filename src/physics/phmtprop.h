/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmtprop.h,v 1.5 2000/01/31 09:52:00 adurant Exp $
//
// Physics moving terrain properties/links
//
#pragma once

#ifndef __PHMTPROP_H
#define __PHMTPROP_H

#include <matrixs.h>

#include <property.h>
#include <propface.h>

////////////////////////////////////////

#define TERRAIN_PATH_NAME       "TPath"
#define TERRAIN_PATH_INIT_NAME  "TPathInit"
#define TERRAIN_PATH_NEXT_NAME  "TPathNext"

////////////////////////////////////////

EXTERN void InitTerrainPathLinks();

////////////////////////////////////////

typedef struct
{
   mxs_real speed;
   int      pause;
   BOOL     path_limit;

   int      cur_paused;
} sTerrainPath;

////////////////////////////////////////////////////////////////////////////////

#define MOVING_TERRAIN_PROP_NAME "MovingTerrain"

class cMovingTerrainProp
{
public:

   cMovingTerrainProp()
   {
      active = -1;
      prev_state = -1;
   };
   
   BOOL active;
   BOOL prev_state;
};

#undef  INTERFACE
#define INTERFACE IMovingTerrainProperty
DECLARE_PROPERTY_INTERFACE(IMovingTerrainProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(cMovingTerrainProp *); 
}; 

////////////////////////////////////////

EXTERN void InitMovingTerrainProps();
EXTERN void TermMovingTerrainProps();

EXTERN void SetMovingTerrainActive(BOOL status);
EXTERN BOOL IsMovingTerrainActive(ObjID objID);
EXTERN BOOL IsMovingTerrainMoving(ObjID objID);

EXTERN IMovingTerrainProperty *g_pMovingTerrainProperty;

#endif







