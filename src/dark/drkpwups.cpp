/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkpwups.cpp,v 1.21 1999/12/21 10:52:44 adurant Exp $
// Dark Specific Powerup Code and Script Services

#include <lg.h>
#include <mprintf.h>

// property stuff
#include <property.h>
#include <propface.h>
#include <propbase.h>
#include <propert_.h>
#include <dataops_.h>

// link stuff
#include <linkbase.h>
#include <lnkquery.h>
#include <relation.h>
// trait stuff
#include <traitman.h>
#include <trait.h>
#include <traitbas.h>

// misc
#include <drkpwups.h>
#include <ctrast.h>
#include <playrobj.h>
#include <objpos.h>
#include <math.h>
#include <rndflash.h>
#include <esnd.h>
#include <dumbprox.h>
#include <textarch.h>
#include <physapi.h>

// snd
#include <esnd.h>
#include <ctagset.h>

// obj create and stuff
#include <objsys.h>
#include <osysbase.h>
#include <iobjsys.h>

// attempt to get an objpos and math ops
#include <portal.h>
#include <wrtype.h>
#include <matrix.h>
#include <dmgmodel.h>
#include <objedit.h>

// for ai interactions
#include <aiapi.h>
#include <aistnapi.h>

// Must be last header
#include <dbmem.h>

////////////////
// Common utilities

// return TRUE if i fit, and create a deploy obj, else return FALSE
BOOL DrkObjTryDeploy(ObjID src_obj, ObjID deploy_arch)
{
   // ok, really, this should see if deploy_arch can fit
   // where src_obj is, and if so, create it there
   {
      ObjID new_obj=BeginObjectCreate(deploy_arch,kObjectConcrete);
      Position *pPos=ObjPosGet(src_obj);
      ObjPosCopyUpdate(new_obj,pPos);
      EndObjectCreate(new_obj);
   }
   return TRUE;
}

////////////////
// Flash Trigger effect

static sPropertyDesc InvulnFlashPropDesc = { "NoFlash", 0, NULL, 0, 0, { "Dark GameSys", "FlashInvuln" }};
static IBoolProperty *pInvulnFlashProp=NULL;

// power is basically feet of full effect
// return is 0.0 for no effect, 1.0 for full effect (multiplier to time disabled)
static float check_flash_effect(ObjID flash, ObjPos *flash_pos, ObjID target, float power)
{
   ObjPos *target_pos=ObjPosGet(target);
   mxs_vector pos_diff;
   
   mx_sub_vec(&pos_diff,&flash_pos->loc.vec,&target_pos->loc.vec);
   if (mx_mag2_vec(&pos_diff)<power*power*4*4)  // up to 4 times distance can be affected
   {
      mxs_matrix target_mat;
      mxs_vector pos_vec;
      mxs_angvec player_pitch;
      mxs_angvec player_view;
      float dot_res;
      
      if (target == PlayerObject())
	{
	  PhysGetSubModRotation(target,PLAYER_HEAD,&player_pitch);
	  player_view.ty=player_pitch.ty;
	  player_view.tz=target_pos->fac.tz;
	  player_view.tx=0;
	  mx_ang2mat(&target_mat,&player_view);
	}
      else
	mx_ang2mat(&target_mat,&target_pos->fac);
      mx_norm_vec(&pos_vec,&pos_diff);
      dot_res=mx_dot_vec(&pos_vec,&target_mat.vec[0]);
#ifdef TEST_TALK
      char buf[200];
      sprintf(buf,"Got %g from %g %g %g and %g %g %g\n",dot_res,
              pos_vec.el[0],pos_vec.el[1],pos_vec.el[2],
              target_mat.vec[0].el[0],target_mat.vec[0].el[1],target_mat.vec[0].el[2]);
      mprintf(buf);
#endif      
      if (dot_res>0)
      {
         Location hit;
         if (PortalRaycast(&flash_pos->loc,&target_pos->loc,&hit,0))
         {
            float dist=mx_mag_vec(&pos_diff), effect;
            effect=(4.0-(dist/power))*dot_res;  // 4 means great, 0 means sadness
            if (target==PlayerObject())
	      {
                 //effect*=0.5;
		 effect*=dot_res;  //make it easier to look away.
	      }
//            else
//            {
               // in close is much more effective - attempt to fix the "at feet" issue
               //            if (dist*2<power) effect*=4;

	       //implement a more gradual scaling.  End result is a wee bit
	       //above multiply by 4.
	       if (dist*5<power) effect *=1.32;
	       if (dist*4<power) effect *=1.32;
	       if (dist*3<power) effect *=1.32;
	       if (dist*2<power) effect *=1.32;
	       if (dist<power) effect *=1.32;
//               if (dist<power) effect*=4;
//               if (dist*2<power) effect+=0.5;
//            }
            return effect/4.0;
         }
      }
   }
   return 0.0;
}

static BOOL has_linker(ObjID obj, void *data)
{
   ILinkQuery *query = ((IRelation *) data)->Query(obj, LINKOBJ_WILDCARD);
   BOOL result = !(query->Done());
   SafeRelease(query);
   return result;
}
   
static IRelation *render_flash_rel;
static sTraitPredicate flash_pred = { has_linker, NULL };
static sTraitDesc flash_trait_desc = { "RenderFlash", 0 };
static ITrait *flash_trait;

static ObjID GetFlashArchetype(ObjID obj)
{
   ObjID donor = flash_trait->GetDonor(obj);

   if (donor == OBJ_NULL) return OBJ_NULL;

   ILinkQuery *query = render_flash_rel->Query(donor, LINKOBJ_WILDCARD);
   ObjID result = OBJ_NULL;

   for(; !query->Done(); query->Next())
   {
      sLink link;
      query->Link(&link);
      result = link.dest;
   }
   SafeRelease(query);
   return result;
}

// @TODO: someone has to do non-sucky special effects here
void DrkTriggerWorldFlash(ObjID obj)
{
   ObjPos *FlashPos =ObjPosGet(obj);
   tAIIter iAIIter;
   
   // go through and blind AIs   
   AutoAppIPtr(AIManager);
   IAI *pAI=pAIManager->GetFirst(&iAIIter);
   while (pAI)
   {
      ObjID us=pAI->GetObjID();
      BOOL is_invuln=FALSE;
      pInvulnFlashProp->Get(us,&is_invuln);
      if (!is_invuln)
      {
         float flash=check_flash_effect(obj,FlashPos,us,12.0);
         if (flash>0.2)   // for now, no time
         {
            int time;
            if (flash>4.0)
               flash=4.0;
            time=4000+(flash*2000);
            DoAISetStun(us,NULL,"Blinded 0, Stalled 0",time);
//            mprintf("Stun an AI\n");
         }
      }
      pAI->Release();
      pAI=pAIManager->GetNext(&iAIIter);
   }
   pAIManager->GetDone(&iAIIter);

   float flash = check_flash_effect(obj,FlashPos,PlayerObject(),15.0);
   if (flash > 0.1)
   {
      // mprintf("Flash Hit the player\n");
      // make flash only a big deal if player is looking right at it
      //flash *= flash;
      //yeah, but if the flash was over 1, it now gets really bad.
   }
   else
      flash = 0;

   IObjectSystem *osys = AppGetObj(IObjectSystem);      
   ObjID our_arch=GetFlashArchetype(obj);
   if (our_arch != OBJ_NULL)
   {
      ObjID flash_obj = osys->Create(our_arch,kObjectConcrete);
      ObjPosCopyUpdate(flash_obj, ObjPosGet(obj));
      // must set the location before calling the startFR function
      startFlashbombRender(flash_obj, flash);
   }
   SafeRelease(osys);

   cTagSet SoundEvent("Activate");
   ESndPlayLoc(&SoundEvent,obj,OBJ_NULL,&FlashPos->loc.vec);
}

////////////////
// Fungus Carpet current count property

#define BASE_FUNGUS_RAD 9.0
#define PLAY_FUNGUS_MUL 0.85
#define PLAY_FUNGUS_RAD (BASE_FUNGUS_RAD*PLAY_FUNGUS_MUL)

static IBoolProperty *pFungusProxProp=NULL;

BOOL FungusProxCB(ObjID obj, float d_2)
{  // if you found a fungus carpet, go ahead
   return TRUE;
}

BOOL ObjInFungus(ObjID obj)
{  // really, i want the position of my feet, n'est ce pas?
   // @TODO: fix this!
   if (IsPlayerObj(obj))
   {
      mxs_vector foot_pos;
      PhysGetSubModLocation(obj,PLAYER_FOOT,&foot_pos);
      return ProxCheckLoc(pFungusProxProp,&foot_pos,PLAY_FUNGUS_RAD,NULL,FungusProxCB);
   }
   else
      return ProxCheckObj(pFungusProxProp,obj,BASE_FUNGUS_RAD,NULL,FungusProxCB);      
}

void FungusizeSound(sESndEvent* ev)
{
   BOOL in_fungus = ObjInFungus(ev->obj1);

   if (!in_fungus)
   {
      // don't use texture objects
      if (GetObjTextureIdx(ev->obj2) < 0)
         in_fungus = ObjInFungus(ev->obj2); 

      if (!in_fungus)
         return; 
   }
   
   cTagSet* app = ev->tagsets[kAppTags];
   app->Append(cTagSet("Fungus true"));
}

// why is this in drkpwups?  Well, because it goes with the other flash
// stuff, and because it is intended for use when switching cameras
// which is sort of a pwup thing, right?  Since remote cameras are
// Thief only at the moment, this seems fine.
void FlashOnlyPlayer(float intensity)
{
  IObjectSystem *osys = AppGetObj(IObjectSystem);
  ObjID playerobj = PlayerObject();
  ObjID flash_arch = GetFlashArchetype(playerobj);

  if (flash_arch != NULL)
    {
      ObjID flash_obj = osys->Create(flash_arch,kObjectConcrete);
      ObjPosCopyUpdate(flash_obj,ObjPosGet(playerobj));
      startFlashbombRender(flash_obj,intensity);
    }
  SafeRelease(osys);
}


////////////////
// flash bomb link/trait setup

static sRelationDesc flash_desc =
{
   LINK_RENDER_FLASH_NAME,
   0,
};

static sRelationDataDesc flash_ddesc = LINK_NO_DATA;

#define FLASH_QUERY_CASES  (kQCaseSetSourceKnown)

////////////////
// blood

static IBoolProperty * pBloodProxProp = NULL;

void ObjAddBlood(ObjID obj)
{
   pBloodProxProp->Create(obj);
}

void ObjRemBlood(ObjID obj)
{
   pBloodProxProp->Delete(obj);
}

BOOL CleanBloodCB(ObjID obj, float d_2)
{
   IDamageModel *pDamageModel=AppGetObj(IDamageModel);
   pDamageModel->SlayObject(obj,NULL);  // dont think we need to know
   return FALSE;  // keep going, we want to kill all of it
}

// look for nearby blood prox, go get it killed
void TryCleanBlood(ObjID water_src, float rad)
{
   if (rad==0.0) rad=6.0;
   ProxCheckObj(pBloodProxProp,water_src,rad,NULL,CleanBloodCB);
}

//
// NEW BLOOD CLEANING PROP HERE!!!
// 

// search for nearby blood

////////////////
// overall setup

void DrkPowerupInit(void)
{
   ITraitManager *trm = AppGetObj(ITraitManager);
   pInvulnFlashProp=CreateBoolProperty(&InvulnFlashPropDesc,kPropertyImplVerySparse);
   render_flash_rel = CreateStandardRelation(&flash_desc,&flash_ddesc,FLASH_QUERY_CASES);
   flash_pred.data = render_flash_rel;
   flash_trait=trm->CreateTrait(&flash_trait_desc, &flash_pred);
   pFungusProxProp = ProxBuildProp("Fungus");
   pBloodProxProp = ProxBuildProp("Blood");
}

void DrkPowerupTerm(void)
{
   SafeRelease(pInvulnFlashProp);
   SafeRelease(render_flash_rel);
   SafeRelease(flash_trait);
   SafeRelease(pFungusProxProp);
   SafeRelease(pBloodProxProp);
}
