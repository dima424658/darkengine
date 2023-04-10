/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/sndgen.cpp,v 1.31 2000/03/05 18:37:46 adurant Exp $

//#define PROFILE_ON

#include <appagg.h>

#include <portal.h>
#include <wrdbrend.h>

#include <propguid.h>

#include <sndgen.h>
#include <sndprop.h>

#include <iobjsys.h>

#include <propman.h>
#include <property.h>
#include <propface.h>
#include <propbase.h>
#include <objpos.h>

#include <schbase.h>
#include <schema.h>

#include <collprop.h>
#include <objmedia.h>
#include <physapi.h>
#include <texprop.h>
#include <locobase.h>
#include <timings.h>
#include <esnd.h>
#include <textarch.h>

// Must be last header 
#include <dbmem.h>


#define COLLISION_SOUND_PROP_NAME_MAX 20

DECLARE_TIMER(FFALL_PCALL, Average);
DECLARE_TIMER(FFALL_PCALL_SOUND, Average);
DECLARE_TIMER(WSWING_PCALL, Average);
DECLARE_TIMER(WCHARGE_PCALL, Average);
DECLARE_TIMER(WSEARCH_PCALL, Average);


static IObjectSystem *pObjSys = NULL;
static IPropertyManager *pPropMan = NULL;

// play sound for object-object collision
void CollisionSoundObjects(ObjID obj1ID, ObjID obj2ID, int collisionResult, float mag)
{
   int coll_type_1;
   int coll_type_2;

   ObjGetCollisionType(obj1ID, &coll_type_1);
   ObjGetCollisionType(obj2ID, &coll_type_2);

   sSchemaCallParams schema_params;
   memset(&schema_params, 0, sizeof(sSchemaCallParams));

   schema_params.flags = SCH_ADD_VOLUME;

   if (!(coll_type_1 & COLLISION_FULL_SOUND) &&
       !(coll_type_2 & COLLISION_FULL_SOUND))
   {
      if (mag > 3000)
         schema_params.volume = 0;
      else
      {
         schema_params.volume = -(3000 - mag);

         if (schema_params.volume < -2500)
            schema_params.volume = -2500;
      }
   }
   else
      schema_params.volume = 0;

   cTagSet Event("Event Collision"); 
   ESndPlayLoc(&Event,obj1ID,obj2ID,&ObjPosGet(obj1ID)->loc.vec,&schema_params); 
}      // @TODO: shouldnt we have a way to override these too? (dc)

// hey, this is all a hack and will change
// play a footstep appropriate to the AI and the texture he's on

// ideally this would use feet_editor too, but that needs
//   a better way to know if in editor or not, so we will wait
//   for real simstate control
EXTERN bool foot_sounds;

//
// @HACK: This has been moved from physics.c so I can get rid of that file.  
//
static int GetTxtIdNearAndUnder(const mxs_vector *pos, const Location *old_loc, mxs_vector *snd_loc)
{
   mxs_vector ray_start, ray_end, delta;
   Location   ray_source, ray_dest, ray_hit;
   int  poly_id, txt_id;

   txt_id = -1;

   mx_copy_vec(&ray_start, (mxs_vector *)pos);
   mx_mk_vec(&delta, 0, 0, -10);
   mx_add_vec(&ray_end, &ray_start, &delta);

   MakeHintedLocationFromVector(&ray_source, &ray_start, (Location *)old_loc);
   MakeHintedLocationFromVector(&ray_dest, &ray_end, (Location *) old_loc);

   if (!PortalRaycast(&ray_source, &ray_dest, &ray_hit, 0))
   {
      if (ray_source.vec.z != ray_hit.vec.z)
      {
         poly_id = PortalRaycastFindPolygon();
         txt_id = WR_CELL(PortalRaycastCell)->render_list[poly_id].texture_id;
      }
      else
         Warning(("GetTxtIdNearAndUnder: out of level?\n"));

      *snd_loc = ray_hit.vec;
      snd_loc->z += 0.2;
   }

   return txt_id;
}

void FootfallSound(const ObjID objID, const mxs_vector *footPos)
{
   AUTO_TIMER(FFALL_PCALL);
   int textureType;

   mxs_vector snd_loc;

   // @TODO: make AIs have better media states
   if (PhysObjInWater(objID))
      return;
   if ((textureType = GetTxtIdNearAndUnder(footPos, &(ObjPosGet(objID)->loc), &snd_loc)) != -1)
   {
      cTagSet Event("Event Footstep");
      ESndPlayLoc(&Event,objID,GetTextureObj(textureType),&snd_loc);
   }
   else
      Warning(("Can't find texture type for AI (%d) footfall at (%g, %g, %g)\n", objID, footPos->x, footPos->y, footPos->z));
}

void WeaponSwingSound(const ObjID objID, const mxs_vector *weaponPos)
{
   AUTO_TIMER(WSWING_PCALL);

   if (PhysObjInWater(objID))
      return;
   cTagSet Event("Event WeaponSwing");
   ESndPlayLoc(&Event, objID, OBJ_NULL, weaponPos);
}

void WeaponChargeSound(const ObjID objID, const mxs_vector *weaponPos)
{
   AUTO_TIMER(WCHARGE_PCALL);

   if (PhysObjInWater(objID))
      return;
   cTagSet Event("Event WeaponCharge");
   ESndPlayLoc(&Event, objID, OBJ_NULL, weaponPos);
}

void SearchSound(const ObjID objID, const mxs_vector *searchPos)
{
   AUTO_TIMER(SEARCH_PCALL);

   if (PhysObjInWater(objID))
      return;
   cTagSet Event("Event Search");
   ESndPlayLoc(&Event, objID, OBJ_NULL, searchPos);
}

void SoundGenInit(void)
{
   if (pPropMan == NULL)
      pPropMan = AppGetObj(IPropertyManager);
   if (pObjSys == NULL)
      pObjSys = AppGetObj(IObjectSystem);
}

void SoundGenShutdown(void)
{
   SafeRelease(pPropMan);
   SafeRelease(pObjSys);
}
