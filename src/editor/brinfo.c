/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/brinfo.c,v 1.45 2000/02/19 12:27:42 toml Exp $
// setup and create code for each brush type
// random maintenance for brush motion and so on

#include <mprintf.h>

#include <stdio.h>
#include <string.h>       // memcpy

#include <editbr.h>
#include <editbr_.h>
#include <brinfo.h>
#include <editobj.h>
#include <objsys.h>
#include <osysbase.h>
#include <portal.h>
#include <objscale.h>
#include <areapnp.h>

#include <traitman.h>

#include <ged_csg.h>
#include <ged_room.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//------------------------------------------------------------
// TERRAIN BRUSHES
//

#pragma off(unreferenced) 

static void _Terr_copy(editBrush *us, editBrush *old, BOOL clone)
{
   us->tx_id=old->tx_id;
   us->media=old->media;
   if (us->primal_id==DEF_PRIMAL)   // ?? - is this a good idea?
      us->primal_id=old->primal_id; // hmmm.. for now we want to keep ourselves
   memcpy(us->txs,old->txs,sizeof(TexInfo)*old->num_faces);
}

static void _Terr_new(editBrush *us)
{
  //if we weren't <=0 or >256, then we probably intended to be that way...
  // so let's keep on trucking with that.  AMSD (Fixes stair create problem)
   if ((us->tx_id <= 0) || (us->tx_id >256))
     us->tx_id=1;   // 0 is not really useful anymore
}

static void _Terr_setup(editBrush *us)
{
}

static void _Terr_delete(editBrush *us)
{
}

//------------------------------------------------------------
// LIGHT BRUSHES
//

static void _Light_copy(editBrush *us, editBrush *old, BOOL clone)
{
   brLight_Handle(us)=0;
   brLight_Type(us)=brLight_Type(old);
   brLight_Hue(us)=brLight_Hue(old);
   brLight_Script(us)=brLight_Script(old);
   brLight_Bright(us)=brLight_Bright(old);
   brLight_Saturation(us)=brLight_Saturation(old);
}

static void _Light_new(editBrush *us)
{
   brLight_Handle(us)=0;
   brLight_Type(us)=0;
   brLight_Hue(us)=0;
   brLight_Script(us)=0;
   brLight_Bright(us)=127.0;
   brLight_Saturation(us)=0.0;            
}

static void _Light_setup(editBrush *us)
{  // kind of an auto-auto-portalize, uses light handle internally to decide what to
   gedcsg_parse_light_brush(us, LIGHT_QUICK);
}

static void _Light_delete(editBrush *us)
{
}


//------------------------------------------------------------
// AREA BRUSHES
//

static void _Area_copy(editBrush *us, editBrush *old, BOOL clone)
{
}

static void _Area_new(editBrush *us)
{
}

static void _Area_setup(editBrush *us)
{
}

static void _Area_delete(editBrush *us)
{
   AreaPnP_Delete_Brush_Tags(us);
}

//------------------------------------------------------------
// OBJ BRUSHES
//

int brinfo_obj_type_default=0;

static void _Obj_copy(editBrush *us, editBrush *old, BOOL clone)
{
   ObjID arch = OBJ_NULL;
   ObjID oldobj = brObj_ID(old);
   if (clone)
   {
      arch = oldobj;
   }
   brObj_Type(us)= brinfo_obj_type_default;   // brObj_Type(old);
   editObjCreateObjFromBrush(arch, us);

}

static void _Obj_new(editBrush *us)
{
   brObj_Type(us) = brinfo_obj_type_default;
   editObjCreateObjFromBrush(OBJ_NULL, us);
}

static void _Obj_setup(editBrush *us)
{
   ObjID obj = brObj_ID(us);

   if (ObjectExists(obj))
   {  //@OPTIMIZE: this may be totally superfluous, add a BOOL new parm to figure it out. 
      editObjSetObjectBrush(brObj_ID(us),us);
      //      editobjUpdateBrushfromObj(brObj_ID(us));
      editobjUpdateObjfromBrush(us);
   }
   else 
      editObjCreateObjFromBrush(OBJ_NULL, us);
}

static void _Obj_delete(editBrush *us)
{  // should this be an editobj call?
   ObjID obj = brObj_ID(us); 
   if (obj != OBJ_NULL)
   {
      editObjSetObjectBrush(obj,NULL);
      DestroyObject(obj);
   }
}

// Flow
static void _Flow_copy(editBrush *us, editBrush *old, BOOL clone)
{
   brFlow_Index(us)=brFlow_Index(old);
}

static void _Flow_new(editBrush *us)
{
   brFlow_Index(us) = 1;
}

static void _Flow_setup(editBrush *us)
{
   // rewater level
}
   
static void _Flow_delete(editBrush *us)
{
   // rewater level
}

//------------------------------------------------------------
// ROOM BRUSHES
//

static mxs_vector zero = {0, 0, 0};

static void _Room_new(editBrush *us)
{
   brRoom_ObjID(us) = ged_room_get_default_arch();
   brRoom_InternalRoomID(us) = -1;   
 
   if (!ged_is_room(ged_room_get_default_arch()))
      brRoom_ObjID(us) = ged_room_get_default();
   else
      brRoom_ObjID(us) = ged_room_get_default_arch();

   if (!OBJ_IS_CONCRETE(brRoom_ObjID(us)))
   {
      Warning(("Attempt to create abstract room!\n"));
      brRoom_ObjID(us) = ged_room_get_default();
   }
}

static void _Room_copy(editBrush *us, editBrush *old, BOOL clone)
{
   if (clone)
   {
      brRoom_ObjID(us) = brRoom_ObjID(old);
      brRoom_InternalRoomID(us) = -1;
   }
   else
      _Room_new(us);
}

static void _Room_setup(editBrush *us)
{
   // update graph?
}
   
static void _Room_delete(editBrush *us)
{
   // update graph?
}

#pragma on(unreferenced)

// basically, this table takes each brush type and knows how to copy it to a new
// one of itself, or to create a new one

static void (*_same_type_copy_create[])(editBrush *us, editBrush *old, BOOL clone)=
 { _Terr_copy, _Light_copy, _Area_copy, _Obj_copy, _Flow_copy, _Room_copy };

static void (*_new_brush[])(editBrush *us)=
 { _Terr_new, _Light_new, _Area_new, _Obj_new, _Flow_new, _Room_new };

static void (*_setup_brush[])(editBrush *us)=
 { _Terr_setup, _Light_setup, _Area_setup, _Obj_setup, _Flow_setup, _Room_setup };

static void (*_delete_brush[])(editBrush *us)=
 { _Terr_delete, _Light_delete, _Area_delete, _Obj_delete, _Flow_delete, _Room_delete };

void brush_field_update(editBrush *us, editBrush *last, ulong flags)
{
   int idx=brushGetType(us);
   if (flags & brField_New)
      if (last && brushGetType(us)==brushGetType(last))
         (*_same_type_copy_create[idx])(us,last,flags & brField_Clone);
      else
         (*_new_brush[idx])(us);
   (*_setup_brush[idx])(us);
}

void brush_field_delete(editBrush *us)
{
   int idx=brushGetType(us);
   (*_delete_brush[idx])(us);
}
