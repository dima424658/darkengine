/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/editobj.c,v 1.106 1998/10/12 00:59:01 dc Exp $
// editor object code
// specifically for render/creation

#include <stdlib.h>
#include <string.h>

#include <md.h>
#include <mlight.h>
#include <2d.h>

#include <comtools.h>
#include <appagg.h>
#include <traitman.h>

#include <mprintf.h>

#include <editbr.h>
#include <editbr_.h>
#include <brinfo.h>
#include <brlist.h>
#include <gedit.h>
#include <eosapp.h>

#include <lresname.h>
#include <lress.h>

#include <refsys.h>
#include <wrobj.h>
#include <wr.h>
#include <objlight.h>

#include <physapi.h>
#include <gamespec.h>
#include <status.h>
#include <editobj.h>
#include <rendobj.h>
#include <objsys.h>
#include <osysbase.h>
#include <propbase.h>
#include <brprop.h>
#include <creatext.h>

#include <propid.h>
#include <objscale.h>
#include <scalprop.h>
#include <mnamprop.h>
#include <mnumprop.h>
#include <objshape.h>
#include <objmodel.h>
#include <propname.h>
#include <vbrush.h>
#include <linkdraw.h>

#include <traitman.h>
#include <traitbas.h>
#include <objquery.h>

#ifdef NETWORK_ENABLED
//include <netobj.h>
#endif
#include <objpos.h>
#include <litprop.h>

#include <config.h>
#include <cfgdbg.h>

#include <ged_room.h>

#include <dbmem.h>

static PropertyID brushprop_id = PROPID_NULL;    // the brush property id

static ObjID creation_archetype = ROOT_ARCHETYPE;
static IBoolProperty* hasbrush_prop = NULL;

// so that viewmgr can keep us from doing this...
BOOL dont_redraw_on_update_brushes=FALSE;

void LGAPI editObjBrushListener(sPropertyListenMsg* msg, PropListenerData data);
void LGAPI editObjModelListener(sPropertyListenMsg* msg, PropListenerData data);


void editobjRescaleObjFromBrush(ObjID obj);
void editobjRescaleBrushFromObj(ObjID obj);

////////////////////////////////////////

#define get_brush_property(pp)  GetPropertyInterface(brushprop_id,IBrushProperty,pp)

////////////////////////////////////////

#pragma off(unreferenced)
static void obj_pos_cb(ObjID obj, const ObjPos* pos, void* data)
{
   // @TODO: write a version of this call that takes pos 
   if (pos)
      editobjUpdateBrushfromObj(obj); 
   else
      editObjDeleteObjBrush(obj); 
}
#pragma on(unreferenced) 

////////////////////////////////////////

void editObjInit(void)
{
   IProperty* prop = GetPropertyNamed(PROP_BRUSH_NAME);
   if (prop != NULL)
   {
      brushprop_id = IProperty_GetID(prop);
      IProperty_Listen(prop,kListenPropUnset ,editObjBrushListener,NULL);
   }
   SafeRelease(prop);
   PropName_Listen(PROP_MODELNUMBER_NAME,kListenPropModify,editObjModelListener,NULL);
   PropName_Listen(PROP_SCALE_NAME,kListenPropModify,editObjModelListener,NULL);
   ObjPosListen(obj_pos_cb,NULL);

   hasbrush_prop = HasBrushPropInit(); 
}

////////////////////////////////////////

void editObjTerm(void)
{
   brushprop_id = PROPID_NULL;
   SafeRelease(hasbrush_prop); 
}

////////////////////////////////////////

void editObjSetDefaultArchetype(ObjID archetype)
{
#if 0
   // Hey, this was removed because room brushes need to inherit from concretes
   if (OBJ_IS_ABSTRACT(type))
#endif

   creation_archetype = archetype;
}

////////////////////////////////////////

ObjID editObjGetDefaultArchetype(void)
{
   return creation_archetype;
}

////////////////////////////////////////

editBrush *editObjGetBrushFromObj(ObjID usID)
{
   // @OPTIMIZE: 
   //  May need to speed this up, expose back door to property.  Not yet though. 
   IBrushProperty* prop;
   
   if (get_brush_property(&prop))
   {
      editBrush* ptr = NULL;
      PROPERTY_GET(prop,usID,&ptr);
      SafeRelease(prop);
      return ptr;
   }
   return NULL;
}

// @TODO person moving stuff should be done by callback once property system is in.
bool editobjSetObjfromBrush(editBrush *us)
{
   ObjID id = brObj_ID(us);
   if (!ObjExists(id))
   {
      editObjCreateObjFromBrush(OBJ_NULL,us);
   }
   else
   {
      editobjUpdateObjfromBrush(us);
   }
   return TRUE;
}

// call this when you have a brush, and need to have an object built for it
BOOL editObjCreateObjFromBrush(ObjID archetype, editBrush *us)
{
   ObjID type = (archetype == OBJ_NULL) ? creation_archetype : archetype;
   ObjID id = BeginObjectCreate(type,kObjectConcrete);
   if (id == OBJ_NULL) return FALSE;
   brObj_ID(us)= id;
   editObjSetObjectBrush(id,us);
#ifdef NETWORK_ENABLED
   NetObjInitObj(id);
#endif

   editobjRescaleBrushFromObj(id);
   editobjSetObjfromBrush(us);
   EndObjectCreate(id);

   // If either of these is relevent, this will shine the light in
   // the world.
   ObjLightShine(id);
   ObjAnimLightShine(id);

   return TRUE;
}

BOOL editObjSetObjectBrush(ObjID id, editBrush* us)
{
   IBrushProperty* brushprop;
   if (get_brush_property(&brushprop))
   {
      if (us)
         PROPERTY_SET(brushprop,id,us);
      else
         IProperty_Delete(brushprop,id); 
      return TRUE;
   }
   return FALSE;
}



// @TODO: EVERYTHING BELOW HERE HAS NOT BEEN FULLY PROPERTIZED

// this moves an object to be in synch with a moved brush (ie. if user has moved
//  the brush with GFH or something, call this to put the obj there)
// XXX person moving stuff should be done by callback once property system is in.
bool editobjUpdateObjfromBrush(editBrush *us)
{
   ObjID obj_id = brObj_ID(us);
   BOOL isCreature=CreatureExists(obj_id);

   ObjPosUpdate(obj_id,&us->pos,&us->ang);

   editobjRescaleObjFromBrush(obj_id);

   if (ObjHasDrawnLinks(obj_id))
      gedit_full_redraw();

   return TRUE;
}

// this takes an obj that has an associated brush and puts the brush where the obj is
bool editobjUpdateBrushfromObj(int usID)
{
   editBrush* us;
   us=editObjGetBrushFromObj(usID);
   if (us==NULL) 
   {
      ConfigSpew("update_brush_spew",("obj %d has no brush\n",usID));
      return FALSE; 
   }
   else
   {
      mxs_vector oldpos=us->pos;
      mxs_angvec oldang=us->ang;
      BOOL delta=FALSE;
      ObjPos* pos = ObjPosGetUnsafe(usID); 
      Assert_(pos != NULL); 
      us->pos = pos->loc.vec;
      us->ang = pos->fac;

      mx_subeq_vec(&oldpos,&us->pos);
      delta=mx_mag2_vec(&oldpos)>0.02;
      delta|=((abs(us->ang.el[0]-oldang.el[0])>0x10)||
              (abs(us->ang.el[1]-oldang.el[1])>0x10)||
              (abs(us->ang.el[2]-oldang.el[2])>0x10));
      if (delta)
         if (vBrush_GetSel()==us)
            gedit_redraw_selection();
         else
            gedit_full_redraw();
   }

   // Update the brush size for object.
   editobjRescaleBrushFromObj(usID);

   return TRUE;
}

// Set the brush size based on the object's model and scale.
void editobjUpdateBrushFromObjScale(int brID, mxs_vector* scale)
{
   editBrush *us = editObjGetBrushFromObj(brID);
   mxs_vector dims, oldsz;

   if (us == NULL) return;

   ObjGetUnscaledDims(brID, &dims);
   if (scale!=NULL)
      mx_elmuleq_vec(&dims,scale);

   oldsz=us->sz;
   if (mx_mag_vec(&dims) == 0.0)
      us->sz.x = us->sz.y = us->sz.z = 0.5;
   else
      mx_scale_vec(&us->sz,&dims,0.5);  // brush dims are halved
   mx_subeq_vec(&oldsz,&us->sz);
   if (mx_mag2_vec(&oldsz)>0.02)
      if (vBrush_GetSel()==us)
         gedit_redraw_selection();
      else
         gedit_full_redraw();
}

void editobjRescaleBrushFromObj(ObjID obj)
{
   mxs_vector scale;
   mx_mk_vec(&scale,1.0,1.0,1.0);
   ObjGetScale(obj,&scale);
   editobjUpdateBrushFromObjScale(obj,&scale);
}

#define SCALE_TOLERANCE 0.01

void editobjRescaleObjFromBrush(ObjID obj)
{
   editBrush *br = editObjGetBrushFromObj(obj);
   mxs_vector scale,v1,dims;

   if (br == NULL) return;
   
   // don't do anything for creatures, since their bbox changes shape
   // when rotated since they only have world coord bbox info, and 
   // this cares about inherent obj-rel dimensions.
   if(CreatureExists(obj)) return;

   ObjGetUnscaledDims(obj, &dims);         // Get the model size from v2.

   scale.x = br->sz.x*2/dims.x;
   scale.y = br->sz.y*2/dims.y;
   scale.z = br->sz.z*2/dims.z;   

   // are we close enough to 1?
   mx_mk_vec(&v1,1.0,1.0,1.0);
   if (mx_dist2_vec(&v1,&scale) < SCALE_TOLERANCE*SCALE_TOLERANCE)
   {
      ObjResetScale(obj);
   }   
   else
      ObjSetScale(obj,&scale);
}

BOOL gCreateEditBrush = TRUE;

bool editobjWantsEditBrush(int usID)
{

   if (OBJ_IS_ABSTRACT(usID) || !gCreateEditBrush)
      return FALSE;

   if (ObjPosGet(usID) == NULL)
      return FALSE; 

   return ObjCanHaveBrush(usID); 
}


BOOL ObjCanHaveBrush(ObjID obj)
{
   BOOL result = TRUE; 
   PROPERTY_GET(hasbrush_prop,obj,&result);
   return result;
}

void ObjSetCanHaveBrush(ObjID obj, BOOL can_have)
{
   PROPERTY_SET(hasbrush_prop,obj,can_have); 
}

void ObjUnsetCanHaveBrush(ObjID obj)
{
   IProperty_Delete(hasbrush_prop,obj); 
}

// given a newly generate Obj this will create a brush and put it there
bool editobjCreateBrushfromObj(int usID)
{
   if (!editobjWantsEditBrush(usID))
      return FALSE;

   if (editObjGetBrushFromObj(usID) == NULL)
   {
      int type = -1;
      editBrush *us=brushInstantiate(0);
      brushSetType(us,brType_OBJECT);
      ObjGetModelNumber(usID, &type);
      brObj_Type(us)=type;

      // this is the wrong call anyway, but something better than the below would be nice, eh?
      //   gedit_place_brush(us,FALSE,FALSE);

      // now actually put the thing there
      brObj_ID(us)=usID;
      us->sz.el[0]=us->sz.el[1]=us->sz.el[2]=1.0;
      blistSeek(0,SEEK_END);     // for now, always put at the end
      blistInsert(us);           // put us in the list
      editObjSetObjectBrush(usID,us);
      vBrush_SelectBrush(us);      
      return editobjUpdateBrushfromObj(usID);
   }
   return FALSE; 
}

void editObjDeleteObjBrush(ObjID obj)
{
   PropID_Delete(brushprop_id,obj);
}

// if we were more robust, we could just scan through obj_to_brush here
// faster and easier

// fully remove refs to a given object
void editobjFullDeref(void)
{
   ObjDeleteAllRefs();
}

// add objects to the world 
void editobjFullReref(void)
{
   ObjBuildAllRefs();
}


////////////////////////////////////////////////////////////

#pragma off(unreferenced)
void LGAPI editObjBrushListener(sPropertyListenMsg* msg, PropListenerData data)
{
   if (msg->property != brushprop_id)  
      return; 

   if (msg->type & kListenPropUnset)
   {
      editBrush* tbr = (editBrush*)msg->value.ptrval;
      brObj_ID(tbr) = OBJ_NULL; 
      vBrush_DeletePtr(tbr);
   }
}

void LGAPI editObjModelListener(sPropertyListenMsg* msg, PropListenerData data)
{
   if (msg->type & kListenPropModify)
   {
      ITraitManager* traitman;
      IObjectQuery* query;

      if (OBJ_IS_CONCRETE(msg->obj))
         editobjRescaleBrushFromObj(msg->obj);            

      traitman = AppGetObj(ITraitManager);
      query = ITraitManager_Query(traitman,msg->obj,kTraitQueryAllDescendents); 

      for (; !IObjectQuery_Done(query); IObjectQuery_Next(query))
      {
         ObjID obj = IObjectQuery_Object(query);
         if (OBJ_IS_CONCRETE(obj))
            editobjRescaleBrushFromObj(obj);            
      }
      SafeRelease(query);
      SafeRelease(traitman);

   }
}
#pragma on(unreferenced)


