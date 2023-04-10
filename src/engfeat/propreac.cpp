/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/propreac.cpp,v 1.12 1999/11/16 12:47:11 porges Exp $

#include <appagg.h>
#include <property.h>
#include <reaction.h>
#include <propreac.h>
#include <reacbase.h>
#include <stimbase.h>
#include <sensbase.h>

#include <iobjsys.h>
#include <osysbase.h>
#include <traitman.h>

#include <propname.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <wrtype.h>
#include <objpos.h>
#include <matrixs.h>
#include <physapi.h>
#include <gametool.h>

// @Hack (toml 11-10-98): for apparitions. just to get this thing done. fix after ship
#include <aiapi.h>
#include <speech.h>


// Must be last header 
#include <dbmem.h>


////////////////////////////////////////////////////////////
// PROPERTY SYSTEM ACT/REACT REACTIONS 
//

//------------------------------------------------------------
// Property Add 
//

struct sPropAddParam
{
   Label propname;
   char pad[sizeof(sReactParamData) - sizeof(Label)];
};

static eReactionResult LGAPI add_prop_func(sReactionEvent* event, 
                                           const sReactionParam* param, 
                                           tReactionFuncData )
{
   sPropAddParam* addp = (sPropAddParam*)&param->data;
   ObjID obj = param->obj[kReactDirectObj];
   ObjID example = param->obj[kReactIndirectObj];
   if (obj == OBJ_NULL) obj = event->sensor_obj;

   if (example != OBJ_NULL)
      PropName_Copy(addp->propname.text, obj, example);
   else
      PropName_Create(addp->propname.text, obj); 

   return kReactionNormal;
}

static sReactionDesc add_prop_desc =
{
   REACTION_PROP_ADD,
   "Set Property",
   REACTION_PARAM_TYPE(sPropAddParam), 
   kReactionHasDirectObj|kReactionHasIndirectObj,
};


static sFieldDesc prop_add_fields[] = 
{
   { "Prop Name", kFieldTypeString, FieldLocation(sPropAddParam,propname), },
};

static sStructDesc prop_add_sdesc = StructDescBuild(sPropAddParam,kStructFlagNone,prop_add_fields);


static ReactionID create_add_prop(IReactions* pReactions)
{
   StructDescRegister(&prop_add_sdesc);
   return pReactions->Add(&add_prop_desc,add_prop_func,NULL);
}

//------------------------------------------------------------
// Property remove 
//

struct sPropRemParam
{
   Label propname;
   char pad[sizeof(sReactParamData)-sizeof(Label)];
};

static eReactionResult LGAPI rem_prop_func(sReactionEvent* event, 
                                           const sReactionParam* param, 
                                           tReactionFuncData )
{
   sPropRemParam* remp = (sPropRemParam*)&param->data;
   ObjID obj = param->obj[kReactDirectObj];
   if (obj == OBJ_NULL) obj = event->sensor_obj;
   PropName_Delete(remp->propname.text,obj);
   return kReactionNormal;
}

static sReactionDesc rem_prop_desc =
{
   REACTION_PROP_REMOVE,
   "Remove Property",
   REACTION_PARAM_TYPE(sPropRemParam), 
   kReactionHasDirectObj, 
};


static sFieldDesc prop_rem_fields[] = 
{
   { "Prop Name", kFieldTypeString, FieldLocation(sPropAddParam,propname), },
};

static sStructDesc prop_rem_sdesc = StructDescBuild(sPropRemParam,kStructFlagNone,prop_rem_fields);


static ReactionID create_rem_prop(IReactions* pReactions)
{
   StructDescRegister(&prop_rem_sdesc);
   return pReactions->Add(&rem_prop_desc,rem_prop_func,NULL);
}

//------------------------------------------------------------
// MetaProperty Add 
//


static eReactionResult LGAPI add_metaprop_func(sReactionEvent* event, 
                                               const sReactionParam* param, 
                                               tReactionFuncData data)
{
   ObjID obj = param->obj[kReactDirectObj];
   ObjID meta = param->obj[kReactIndirectObj];

   if (obj == OBJ_NULL) obj = event->sensor_obj;

   if (meta == OBJ_NULL) // back compatible
   {
      AutoAppIPtr_(ObjectSystem,pObjSys);
      meta = pObjSys->GetObjectNamed((const char*)&param->data);
   }

   if (meta != OBJ_NULL)
   {
      AutoAppIPtr_(TraitManager,TraitMan);
      TraitMan->AddObjMetaProperty(event->sensor_obj,meta);
   }
   return kReactionNormal;
}



static sReactionDesc add_metaprop_desc =
{
   REACTION_METAPROP_ADD,
   "Add MetaProperty",
   NO_REACTION_PARAM,
   kReactionHasDirectObj|kReactionHasIndirectObj, 
};

static ReactionID create_add_metaprop(IReactions* pReactions)
{
   return pReactions->Add(&add_metaprop_desc,add_metaprop_func,NULL);
}


//------------------------------------------------------------
// MetaProperty Rem 
//


static eReactionResult LGAPI rem_metaprop_func(sReactionEvent* event, 
                                               const sReactionParam* param, 
                                               tReactionFuncData data)
{
   ObjID obj = param->obj[kReactDirectObj];
   ObjID meta = param->obj[kReactIndirectObj];

   if (obj == OBJ_NULL) obj = event->sensor_obj;
   if (meta == OBJ_NULL) // back compatibility
   {
      AutoAppIPtr_(ObjectSystem,pObjSys);
      meta = pObjSys->GetObjectNamed((const char*)&param->data);
   }

   if (meta != OBJ_NULL)
   {
      AutoAppIPtr_(TraitManager,TraitMan);
      if (TraitMan->ObjHasDonor(event->sensor_obj, meta)) {
         TraitMan->RemoveObjMetaProperty(event->sensor_obj,meta);
      }
   }
   return kReactionNormal;
}

static sReactionDesc rem_metaprop_desc =
{
   REACTION_METAPROP_REMOVE,
   "Remove MetaProperty",
   NO_REACTION_PARAM,
   kReactionHasDirectObj|kReactionHasIndirectObj, 
};


static ReactionID create_rem_metaprop(IReactions* pReactions)
{
   return pReactions->Add(&rem_metaprop_desc,rem_metaprop_func,NULL);
}

//------------------------------------------------------------
// Obj Create 
//

struct sObjCreateParam
{
   mxs_vector pos;
   float heading;
   float pitch;
   float bank; 
};

#define DEGREES2ANG(x) (mxs_ang)(((long)((x) * MX_ANG_PI))/180)

static eReactionResult LGAPI create_obj_func(sReactionEvent* event, 
                                           const sReactionParam* param, 
                                           tReactionFuncData )
{
   sObjCreateParam* createp = (sObjCreateParam*)&param->data;
   ObjID example = param->obj[kReactDirectObj];  
   ObjID ref_frame = param->obj[kReactIndirectObj];

   mxs_vector pos = createp->pos;
   mxs_angvec ang = { DEGREES2ANG(createp->bank), DEGREES2ANG(createp->pitch),DEGREES2ANG(createp->heading) } ;  


   if (ref_frame != OBJ_NULL)
   {
      ObjPos* ref_pos = ObjPosGet(ref_frame);
      
      // Compute orientation matrix
      mxs_matrix mat;
      mx_ang2mat(&mat,&ref_pos->fac);

      // transform pos into ref_frame
      mxs_vector oldpos = pos;
      mx_zero_vec(&pos);
      
      for (int i = 0; i < 3; i++)  
      {
         // Transform pos
         mx_scale_addeq_vec(&pos, &mat.vec[i], oldpos.el[i]);
         
         // transform ang
         ang.el[i] += ref_pos->fac.el[i];
      }

      // Now add in ref_frame origin
      mx_addeq_vec(&pos,&ref_pos->loc.vec);
   }

   // Ok.  Create the object.
   AutoAppIPtr_(ObjectSystem,pObjSys);
   ObjID obj = pObjSys->BeginCreate(example,kObjectConcrete);

   // Set its pos
   ObjPosUpdate(obj,&pos,&ang);

   // All Done!
   pObjSys->EndCreate(obj);

   return kReactionNormal;
}

static sReactionDesc create_obj_desc =
{
   REACTION_OBJ_CREATE,
   "Create Object",
   REACTION_PARAM_TYPE(sObjCreateParam), 
   kReactionHasDirectObj|kReactionHasIndirectObj,
};


static sFieldDesc obj_create_fields[] = 
{
   { "Position", kFieldTypeVector, FieldLocation(sObjCreateParam,pos), },
   { "Heading", kFieldTypeFloat, FieldLocation(sObjCreateParam,heading) },
   { "Pitch", kFieldTypeFloat, FieldLocation(sObjCreateParam,pitch) },
   { "Bank",  kFieldTypeFloat, FieldLocation(sObjCreateParam,bank) }, 
};

static sStructDesc obj_create_sdesc = StructDescBuild(sObjCreateParam,kStructFlagNone,obj_create_fields);

static ReactionID create_create_obj(IReactions* pReactions)
{
   StructDescRegister(&obj_create_sdesc);
   return pReactions->Add(&create_obj_desc,create_obj_func,NULL);
}


//------------------------------------------------------------
// Obj Destroy 
//


#define DEGREES2ANG(x) (mxs_ang)(((long)((x) * MX_ANG_PI))/180)

static eReactionResult LGAPI destroy_obj_func(sReactionEvent* event, 
                                           const sReactionParam* param, 
                                           tReactionFuncData )
{
   ObjID obj = param->obj[kReactDirectObj];  

   // @Hack (toml 11-10-98): for apparitions. just to get this thing done. fix after ship
   if (ObjIsAI(obj))
      SpeechHalt(obj);

   // Ok.  Destroy the object.
   AutoAppIPtr_(ObjectSystem,pObjSys);
   pObjSys->Destroy(obj);

   return (obj == event->sensor_obj) ? kReactionAbort : kReactionNormal;
}

static sReactionDesc destroy_obj_desc =
{
   REACTION_OBJ_DESTROY,
   "Destroy Object",
   NO_REACTION_PARAM,
   kReactionHasDirectObj,
};


static ReactionID create_destroy_obj(IReactions* pReactions)
{
   return pReactions->Add(&destroy_obj_desc,destroy_obj_func,NULL);
}

//------------------------------------------------------------
// Property Clone 
//


static eReactionResult LGAPI clone_prop_func(sReactionEvent* event, 
                                           const sReactionParam* param, 
                                           tReactionFuncData )
{
   ObjID obj = param->obj[kReactDirectObj];
   ObjID example = param->obj[kReactIndirectObj];

   AutoAppIPtr_(ObjectSystem,pObjSys);
   pObjSys->CloneObject(obj,example);

   return kReactionNormal;
}

static sReactionDesc clone_prop_desc =
{
   REACTION_PROP_CLONE,
   "Clone Properties",
   NO_REACTION_PARAM,
   kReactionHasDirectObj|kReactionHasIndirectObj,
};


static ReactionID create_clone_prop(IReactions* pReactions)
{
   return pReactions->Add(&clone_prop_desc,clone_prop_func,NULL);
}


//------------------------------------------------------------
// Obj Move 
//

struct sObjMoveParam
{
   mxs_vector pos;
   float heading;
   float pitch;
   float bank; 
};

#define DEGREES2ANG(x) (mxs_ang)(((long)((x) * MX_ANG_PI))/180)

static eReactionResult LGAPI move_obj_func(sReactionEvent* event, 
                                           const sReactionParam* param, 
                                           tReactionFuncData )
{
   sObjMoveParam* movep = (sObjMoveParam*)&param->data;
   ObjID obj = param->obj[kReactDirectObj];  
   ObjID ref_frame = param->obj[kReactIndirectObj];

   mxs_vector pos = movep->pos;
   mxs_angvec ang = { DEGREES2ANG(movep->bank), DEGREES2ANG(movep->pitch),DEGREES2ANG(movep->heading) } ;  

   AutoAppIPtr_(GameTools,pTools);
   pTools->TeleportObject(obj,&pos,&ang,ref_frame); 

   return kReactionNormal;
}

static sReactionDesc move_obj_desc =
{
   REACTION_OBJ_MOVE,
   "Teleport Object",
   REACTION_PARAM_TYPE(sObjMoveParam), 
   kReactionHasDirectObj|kReactionHasIndirectObj,
};


static sFieldDesc obj_move_fields[] = 
{
   { "Position", kFieldTypeVector, FieldLocation(sObjMoveParam,pos), },
   { "Heading", kFieldTypeFloat, FieldLocation(sObjMoveParam,heading) },
   { "Pitch", kFieldTypeFloat, FieldLocation(sObjMoveParam,pitch) },
   { "Bank",  kFieldTypeFloat, FieldLocation(sObjMoveParam,bank) }, 
};

static sStructDesc obj_move_sdesc = StructDescBuild(sObjMoveParam,kStructFlagNone,obj_move_fields);

static ReactionID create_move_obj(IReactions* pReactions)
{
   StructDescRegister(&obj_move_sdesc);
   return pReactions->Add(&move_obj_desc,move_obj_func,NULL);
}



//------------------------------------------------------------
// Make the effects
//

void InitPropReactions(void)
{
   AutoAppIPtr(Reactions);
   create_add_prop(pReactions);
   create_rem_prop(pReactions);
   create_add_metaprop(pReactions);
   create_rem_metaprop(pReactions);
   create_create_obj(pReactions);
   create_destroy_obj(pReactions);
   create_clone_prop(pReactions);
   create_move_obj(pReactions);
}





