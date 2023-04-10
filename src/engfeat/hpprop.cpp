/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/hpprop.cpp,v 1.8 1998/10/19 13:03:31 mahk Exp $
#include <propert_.h>
#include <propface.h>
#include <objhp.h>
#include <hpprop.h>

// must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// HIT POINTS PROP API 
//


////////////////////////////////////////////////////////////
// HIT POINTS PROPERTY CREATION 
//

#define HPPROP_IMPL kPropertyImplDense

static PropertyID hpprop_id = PROPID_NULL;
static PropertyID hpmaxprop_id = PROPID_NULL;

static sPropertyDesc hpprop_desc  = 
{
   PROP_HP_NAME,
   0, // flags 
   NULL, // constraints 
   0, 0, // Version
   { "Game: Damage Model", "Hit Points" }, 
};

static sPropertyDesc hpmaxprop_desc = 
{
   PROP_HPMAX_NAME,
   0,  // flags 
   NULL, // constraints 
   0, 0, // Version
   { "Game: Damage Model", "Max Hit Points" }, 
};

void HitPointsPropInit(void)
{
   IIntProperty* prop;
   prop = CreateIntProperty(&hpprop_desc,HPPROP_IMPL);
   hpprop_id = prop->GetID();
   SafeRelease(prop);
   prop = CreateIntProperty(&hpmaxprop_desc,HPPROP_IMPL);
   hpmaxprop_id = prop->GetID();
   SafeRelease(prop);
}

#define get_hp_prop(prop) GetPropertyInterface(hpprop_id,IIntProperty,prop)
#define get_hpmax_prop(prop) GetPropertyInterface(hpmaxprop_id,IIntProperty,prop)

BOOL ObjGetHitPoints(ObjID obj, int *hp)
{
   IIntProperty* prop;
   if (get_hp_prop(&prop))
   {  // Probably don't want to get simple here
      BOOL retval = prop->Get(obj,hp);
      SafeRelease(prop);
      return retval;
   }
   return FALSE;
}

BOOL ObjSetHitPoints(ObjID obj, int hp)
{
   IIntProperty* prop;
   if (get_hp_prop(&prop))
   {
      prop->Set(obj,hp);
      SafeRelease(prop);
      return TRUE;
   }
   return FALSE;
}



BOOL ObjGetMaxHitPoints(ObjID obj, int *hp)
{
   IIntProperty* prop;
   if (get_hpmax_prop(&prop))
   {  // Probably don't want to get simple here
      BOOL retval = prop->Get(obj,hp);
      SafeRelease(prop);
      return retval;
   }
   return FALSE;
}

BOOL ObjSetMaxHitPoints(ObjID obj, int hp)
{
   IIntProperty* prop;
   if (get_hpmax_prop(&prop))
   {
      prop->Set(obj,hp);
      SafeRelease(prop);
      return TRUE;
   }
   return FALSE;
}






