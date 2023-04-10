/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/hdskprop.h,v 1.3 2000/01/29 13:38:59 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   hdskprop.h

   heat disk special effect property

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _HDSKPROP_H_
#define _HDSKPROP_H_

#include <property.h>
#include <hdskbase.h>


#undef INTERFACE
#define INTERFACE IHeatDiskProperty
DECLARE_PROPERTY_INTERFACE(IHeatDiskProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sHeatDiskCluster*); 
}; 
#undef INTERFACE

#define PROP_HEAT_DISK_DATA_NAME "HeatDisks"

EXTERN BOOL HeatDiskPropInit(void);

// for getting, not forgetting...
EXTERN PropertyID heat_disk_id;

// @TODO: What else do we want in this interface?
EXTERN BOOL ObjHeatDiskGet(ObjID obj, sHeatDiskCluster **cluster);
EXTERN BOOL ObjHeatDiskSet(ObjID obj, sHeatDiskCluster *cluster);
EXTERN void ObjHeatDiskRender(ObjID obj, sHeatDiskCluster *cluster);

#endif  // ~_HDSKPROP_H_
