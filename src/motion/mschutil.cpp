/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mschutil.cpp,v 1.5 1998/06/04 16:04:30 kate Exp $

#include <motprop.h>
#include <mschutil.h>
#include <mschbase.h>
#include <motschem.h>
#include <drkcret.h>
#include <plycbllm.h>

#include <dbmem.h> // must be last included header

BOOL MSchUGetGaitData(const cMotionSchema *pSchema, sMGaitSkillData **pData)
{
   ObjID obj;

   Assert_(pSchema);

   Assert_(pSchema->GetFlags()&kMSchFlag_ArchObjSwizzle);

   obj=pSchema->GetArchetype();
   return ObjGetGaitData(obj,pData);
}

BOOL MSchUGetSwordActionType(const cMotionSchema *pSchema, int *pType)
{
   ObjID obj;

   Assert_(pSchema);

   Assert_(pSchema->GetFlags()&kMSchFlag_ArchObjSwizzle);

   obj=pSchema->GetArchetype();
   return ObjGetSwordActionType(obj,pType);
}


BOOL MSchUGetMotControllerID(const cMotionSchema *pSchema, int *pID)
{
   ObjID obj;

   Assert_(pSchema);

   if(pSchema->GetFlags()&kMSchFlag_ArchObjSwizzle)
   {
      obj=pSchema->GetArchetype();
      return ObjGetMotControllerID(obj,pID);
   } else // is player schema
   {
      // @NOTE: this only works if schema is the currently set up one for
      // player cerebellum.  However, this is always the case currently.
      //                                              KJ 4/98
      return PlayerGetMotControllerID(pID);
   }
}

// @TODO: cache last used archetype, and then make a property listener or something,
// so that this doesn't need to check the property every time, since it almost never
// changes, and same archetype will likely get used over and over for same player ability
// (sword idling etc)
BOOL MSchUGetPlayerPosOffset(const cMotionSchema *pSchema, mxs_vector *pPosOff, mxs_angvec *pAngOff)
{
   ObjID obj;

   Assert_(pSchema);

   if(pSchema->GetFlags()&kMSchFlag_ArchObjSwizzle)
   {
      obj=pSchema->GetArchetype();
      return ObjGetPlayerPosOffsets(obj,pPosOff,pAngOff);
   } else
   {
      Warning(("Don't know how to get arm offset\n"));
      return FALSE;
   }
}

BOOL MSchUGetPlayerSkillData(const cMotionSchema *pSchema, sMPlayerSkillData *pSkillData)
{
   // @NOTE: this only works if schema is the currently set up one for
   // player cerebellum.  However, this is always the case currently.
   //                                              KJ 4/98
   return PlayerGetSkillData(pSkillData);
}

