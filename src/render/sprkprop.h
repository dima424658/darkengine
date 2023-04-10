/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/sprkprop.h,v 1.4 2000/01/31 09:53:27 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   sprkprop.h

   little triangular spark thingbob special effect property

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _SPRKPROP_H_
#define _SPRKPROP_H_

#include <property.h>
#include <sprkt.h>

#undef INTERFACE
#define INTERFACE ISparkProperty
DECLARE_PROPERTY_INTERFACE(ISparkProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sSpark*); 
}; 

#define PROP_SPARK_DATA_NAME "Spark"

EXTERN BOOL SparkPropInit(void);

// for getting, not forgetting...
EXTERN PropertyID spark_id;

EXTERN BOOL ObjSparkGet(ObjID obj, sSpark **spark);
EXTERN BOOL ObjSparkSet(ObjID obj, sSpark *spark);
EXTERN void ObjSparkRender(ObjID obj, sSpark *spark, struct grs_bitmap *bitmap);

#endif  // ~_SPRKPROP_H_
