/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/brobjpro.cpp,v 1.3 2000/02/19 12:27:45 toml Exp $
// expose brush property info to brush/editor system
// mostly for objects which have props which make them behave like other brush types

#include <brrend.h>
#include <brinfo.h>
#include <brobjpro.h>
#include <litprop.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


// You can think of this as an enumeration of those properties which
// light the world.
static BOOL get_brightness(ObjID obj, float *brightness)
{
   if (ObjLightGetBrightness(obj, brightness))
      return TRUE;

   if (ObjAnimLightGetBrightness(obj, brightness))
      return TRUE;

   return FALSE;
}


// Lights take a brightness of [0, 1024].
float brObjProp_getLightLevel(ObjID o_id)
{
   float brightness;

   if (get_brightness(o_id, &brightness))
      return brightness;

   return -1.0;
}

// for now, only knows about the light prop
int brObjProp_getFilterType(ObjID o_id)
{
   if (brObjProp_getLightLevel(o_id)>=0.0)
      return brFlag_FILTER_OBJ|brFlag_FILTER_LIGHT;
   return brFlag_FILTER_OBJ;
}
