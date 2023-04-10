/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkdmg.cpp,v 1.40 1999/11/19 14:56:16 adurant Exp $

#include <shkdmg.h>

#include <mprintf.h>
#include <appagg.h>

#include <linkbase.h>
#include <linkman.h>
#include <relation.h>
#include <lnkquery.h>
#include <osetlnkq.h>

#include <objedit.h>
#include <rand.h>
#include <schema.h>

#include <trait.h>
#include <traitman.h>
#include <traitbas.h>

#include <iobjsys.h>
#include <objdef.h>

#include <bintrait.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <dmgbase.h>
#include <playrobj.h>
#include <objpos.h>
#include <partprop.h>
#include <prjctile.h>
#include <particle.h>
#include <objhp.h>
#include <dmgprop.h>
#include <textarch.h>
#include <simtime.h>

#include <phclsn.h>
#include <phcollev.h>
#include <physapi.h>

#include <esnd.h>

#include <dyntex.h>

#include <shkblood.h>
#include <gunprop.h>
#include <shkprop.h>
#include <shkplayr.h>
#include <gunproj.h>

#include <netmsg.h>
#include <iobjnet.h>
#include <ghostapi.h>

// charm
#include <culpable.h>
#include <shkpsapi.h>
#include <scrptapi.h>

// must be last header
#include <dbmem.h>

#pragma off(unreferenced)

////////////////////////////////////////
//
// Miss-Spang Relation
//

IRelation *g_pMissSpangs;

static sRelationDesc missSpangRDesc = 
{
   "Miss Spang",
}; 

static sRelationDataDesc missSpangDDesc = {"Normal to terrain?", sizeof(BOOL),}; 

static sFieldDesc missSpangRawFields = 
   { "Normal to terrain?", kFieldTypeBool, sizeof(BOOL), 0, 0 };
static sStructDesc missSpangSDesc = 
   { "Normal to terrain?", sizeof(BOOL), kStructFlagNone, 1, &missSpangRawFields};

void CreateMissSpangRelation(void)
{
   g_pMissSpangs = CreateStandardRelation(&missSpangRDesc, &missSpangDDesc, kQCaseSetSourceKnown); 
   StructDescRegister(&missSpangSDesc);
}

ObjID ChooseMissSpang(ObjID obj, BOOL* pOrientToNormal)
{

   ILinkQuery* query = QueryInheritedLinksSingleUncached(g_pMissSpangs,obj,LINKOBJ_WILDCARD); 
   ObjID result = OBJ_NULL;

   if (!query->Done())
   {
      sLink link;
      query->Link(&link);
      result = link.dest; 
      // deal with old version data that doesn't have the field
      if (query->Data() != NULL)
         *pOrientToNormal = *((BOOL*)(query->Data()));
      else
         *pOrientToNormal = FALSE;
   }
   SafeRelease(query); 
   return result;
}

////////////////////////////////////////
//
// Hit-Spang Relation
//

// Place one of these links from an archetypal projectile object to the
// archetypal object that the hit spang is to occur on. The data on this
// link is the name of the hit spang object to be created. 

IRelation *g_pHitSpangs;

static sRelationDesc hitSpangRDesc = 
{
   "Hit Spang",
}; 

static sRelationDataDesc hitSpangDDesc = {"Hit Spang Obj", sizeof(int),}; 

static sFieldDesc hitSpangRawFields = 
   { "Hit Spang Obj", kFieldTypeInt, sizeof(int), 0, 0 };
static sStructDesc hitSpangSDesc = 
   { "Hit Spang Obj", sizeof(int), kStructFlagNone, 1, &hitSpangRawFields};

void CreateHitSpangRelation(void)
{
   g_pHitSpangs = CreateStandardRelation(&hitSpangRDesc, &hitSpangDDesc, kQCaseSetSourceKnown); 
   StructDescRegister(&hitSpangSDesc);
}

static ITrait* g_pHitSpangTrait;

ObjID ChooseHitSpang(ObjID bulletID, ObjID victimID)
{
   ObjID result = OBJ_NULL;

   ILinkQuery* query = QueryInheritedLinks(g_pHitSpangTrait,g_pHitSpangs,bulletID,victimID); 
   if (!query->Done())
      result = *((ObjID*)(query->Data()));
   SafeRelease(query); 
   return result;
}

////////////////////////////////////////
//
// NETWORKING CODE
//
// This little message just indicates that a creature has taken damage,
// so everyone should show the HUD for it.
//
static cNetMsg *g_pShowDmgMsg = NULL;

static void handleShowDmg(ObjID victim)
{
   gPropHUDTime->Set(victim, GetSimTime() + 5000);
}

static sNetMsgDesc sShowDmgDesc =
{
   kNMF_Broadcast,
   "ShowDmg",
   "Show Creature Damage",
   NULL,
   handleShowDmg,
   {{kNMPT_SenderObjID, kNMPF_IfHostedHere, "Victim"},
    {kNMPT_End}}
};

////////////////////////////////////////
//
// Damage Listener
//

const float kShockLandForce = 3600;
const float kShockFallDmgRatio = 1800;

#define MAX_TEX_CORPSES 16

// how far we backup from collision pt to place miss spang
// a pathetic attempt to stay in the world...
const float kMissSpangBackupAmt = 0.01;   


static void DoOrientToNormal(const sDamageMsg *pMsg, ObjID spang, float backup = kMissSpangBackupAmt)
{
   sPhysClsnEvent* pClsnEvent = (sPhysClsnEvent*)pMsg->Find(kEventKindCollision);
   cPhysClsn *pClsn = pClsnEvent->collision;
   mxs_angvec fac;
   mxs_matrix mat;
   mxs_vector loc;
   
   Assert_(pClsnEvent);
   mx_mk_move_x_mat(&mat, &pClsn->GetNormal());
   mx_mat2ang(&fac, &mat);
   loc = pClsn->GetClsnPt();
   // backup to try to stay in the world
   mx_scale_addeq_vec(&loc, &pClsn->GetNormal(), backup);
   ObjPosUpdate(spang, &loc, &fac);          
}

eDamageResult LGAPI ShockDamageListener(const sDamageMsg* pMsg, tDamageCallbackData data)
{
   switch(pMsg->kind)
   {
   case kDamageMsgDamage:
      {
         // Play an environmental sound 
         cTagSet Event("Event Damage");

         int damage = pMsg->data.damage->amount; 

         int hp = 100, maxhp = 100; 
         ObjGetHitPoints(pMsg->victim,&hp);
         ObjGetMaxHitPoints(pMsg->victim,&maxhp); 

         if (maxhp <= 0) 
            maxhp = 1; 
         if (damage > maxhp) 
            damage = maxhp; 
         if (hp < 0)
            hp = 0; 

         Event.Append(cTag("Damage",damage*100/maxhp));
         Event.Append(cTag("Health",hp*100/maxhp)); 

         // Add class tags for damage type
         ObjID dmgtype = (ObjID)pMsg->data.damage->kind; 
         sESndTagList* pDamageTags = NULL; 
         if (ObjGetESndClass(dmgtype,&pDamageTags))
            Event.Append(*pDamageTags->m_pTagSet); 

         ESndPlayObj(&Event,pMsg->victim,pMsg->culprit); // ,&ObjPosGet(pMsg->victim)->loc.vec); 

         // need something to filter this down to only monsters?
         BOOL drawhp = FALSE;
         gPropShowHP->Get(pMsg->victim,&drawhp);
         if (drawhp)
         {
            gPropHUDTime->Set(pMsg->victim, GetSimTime() + 5000);
            // Tell the other players to show the damage, too
            g_pShowDmgMsg->Send(OBJ_NULL, pMsg->victim);
         }
         ShockReleaseBlood(pMsg);

         // End charm if charmed
         if (IsCharmed(pMsg->victim) && IsAPlayer(GetRealCulprit(pMsg->culprit)))
         {
            AutoAppIPtr(ScriptMan);
            sScrMsg msg(pMsg->victim, "AbortCharm"); 
            pScriptMan->SendMessage(&msg); 
         }

         if (pMsg->victim==PlayerObject())
            GhostNotify(PlayerObject(),kGhostStWounded);
      }
      break;
   case kDamageMsgImpact:
      {
         int textureID = GetObjTextureIdx(pMsg->culprit);
  
         if (textureID>=0)
         {
            // terrain collision - generate Miss-spang
            BOOL orientToNormal;
            ObjID spang = ChooseMissSpang(pMsg->victim, &orientToNormal);
            AutoAppIPtr(ObjectNetworking);
            // Don't generate miss spangs for things we don't own:
            if ((spang != OBJ_NULL) && 
                !pObjectNetworking->ObjIsProxy(pMsg->victim))
            {
               mxs_angvec fac;
               AutoAppIPtr(ObjectSystem);
         
               spang = pObjectSystem->BeginCreate(spang, kObjectConcrete);
               Assert_(spang != OBJ_NULL);
               if (orientToNormal)
               {
                  // Place the spang normal to the terrain
                  DoOrientToNormal(pMsg, spang);
               }
               else
               {
                  // Place the spang at same location & orientation as colliding object
                  mxs_vector loc;
                  PhysGetModRotation(pMsg->victim, &fac);
                  PhysGetModLocation(pMsg->victim, &loc);
                  ObjPosUpdate(spang, &loc, &fac);          
               }
               pObjectSystem->EndCreate(spang); 
            }

            if (pMsg->victim != PlayerObject())
            {
               // switch the texture to a damaged form, potentially
               // does it have any corpse links?
               AutoAppIPtr(LinkManager);
               AutoAppIPtr(TraitManager);
               AutoAppIPtr(DynTexture);

               IRelation *pRel;
               ObjID texcand[MAX_TEX_CORPSES];
               ObjID texobj = GetTextureObj(textureID);
               ObjID texarch = pTraitManager->GetArchetype(texobj);
               ObjID newarch, newobj;
               char respath[255],resname[255], temp[255];
               const char *archname;
               sLink slink; 
               int count = 0;
               int id2, n, r;
               static ITrait *pTrait = NULL;

               pRel = pLinkManager->GetRelationNamed("Corpse");
               if (pTrait == NULL)
                  pTrait = MakeTraitFromRelation(pRel);
               ILinkQuery *query; 
               /*
               ILinkQuery *q2;
               query = QueryInheritedLinksSingle(pTrait,pRel,texarch, LINKOBJ_WILDCARD); 
               if (!query->Done())
               {
                  query->Link(&slink);
                  q2 = pRel->Query(slink.source, LINKOBJ_WILDCARD);
                  for (; !q2->Done(); q2->Next())
                  {
                     q2->Link(&slink);
                     texcand[count] = slink.dest;
                     count++;
                  }
                  SafeRelease(q2);
               }
               SafeRelease(query); 
               */

               query = QueryInheritedLinksSingle(pTrait,pRel,texarch, LINKOBJ_WILDCARD); 
               while (!query->Done())
               {
                  query->Link(&slink);
                  texcand[count] = slink.dest;
                  count++;
                  query->Next();
               }
               SafeRelease(query); 

               if (count > 0)
               {
                  r = Rand() % count;
                  newarch = texcand[r];
                  archname = ObjEditName(newarch);
                  sscanf(archname,"t_fam/%s",&temp);
                  strcpy(resname,strchr(temp,'/') + 1);
                  n = strlen(temp) - strlen(resname) - 1;
                  temp[n] = '\0';
                  sprintf(respath,"fam\\%s",temp);

                  newobj = GetTextureObjNamed(respath,resname);
                  if (!IsTextureObj(newobj))
                  {
                     Warning(("Obj %d is not a texture!\n",newobj));
                  }
                  else
                  {
                     sPhysClsnEvent* pClsnEvent = (sPhysClsnEvent*)pMsg->Find(kEventKindCollision);
                     cPhysClsn *pClsn = pClsnEvent->collision;
                     mxs_vector vec;
                     Location loc;
   
                     Assert_(pClsnEvent);
                     vec = pClsn->GetClsnPt();
                     memcpy(&loc.vec,&vec,sizeof(mxs_vector));

                     id2 = GetObjTextureIdx(newobj);
                     //pDynTexture->ChangeTexture(&loc, 0.1, textureID, id2);  
                     pDynTexture->ChangeTexture(pMsg->victim, textureID, id2);

                     // throw out some sparky crap
                     AutoAppIPtr(ObjectSystem);
         
                     // yeah, if I were cool I'd use a link
                     ObjID arch = pObjectSystem->GetObjectNamed("Anim Texture Break");
                     if (arch != OBJ_NULL)
                     {
                        ObjID spark = pObjectSystem->BeginCreate(arch, kObjectConcrete);
                        DoOrientToNormal(pMsg, spark, 0.6);
                        pObjectSystem->EndCreate(spark); 
                     }

                     // play a sound effect too
                     SchemaPlayLoc((Label *)"texbreak", &vec);
                  }
               }
            }
         }
         else
         {
            // object collission - generate hit-spang
            ObjID spang = ChooseHitSpang(pMsg->victim, pMsg->culprit);
            if (spang != OBJ_NULL)
            {
               mxs_vector loc;
               mxs_angvec fac;
               AutoAppIPtr(ObjectSystem);

               PhysGetModLocation(pMsg->victim, &loc);
               PhysGetModRotation(pMsg->victim, &fac);
               spang = pObjectSystem->BeginCreate(spang, kObjectConcrete); 
               ObjPosUpdate(spang, &loc, &fac);           // position it correctly 
               pObjectSystem->EndCreate(spang); 
            }
         }
         
         // projectiles hitting objects is handled inside simpdmg for the moment
         // we can move it into a ShockProj... fn if we need to
         // This just 
         if (PhysIsProjectile(pMsg->victim))
            if (pMsg->culprit == 0)
               return GunProjTerrImpactHandler(pMsg->victim);
 
      }
      break;
   case kDamageMsgSlay:
      {

         // experience points from AI kills
         // @TODO: detect whether the player was "responsible" for death

         /*
         int exp = ObjGetExp(pMsg->victim);
         if (exp != 0)
         {
            // hmm, need better algorithm for determining who gets the exp?
            AutoAppIPtr(ShockPlayer);
            pShockPlayer->AddExperience(PlayerObject(), exp);
         }
         */

         // Play an environmental sound 
         cTagSet Event("Event Death");
         ESndPlayLoc(&Event,pMsg->victim,(ObjID) pMsg->data.slay, &ObjPosGet(pMsg->victim)->loc.vec); 

         // destroy particles?
         if (ObjIsParticle(pMsg->victim))
         {
            ParticlesDeleteFromObjID(pMsg->victim);
            return kDamageDestroy;
         }
      }
   }
   return kDamageNoOpinion;
}

void ShockDamageInit(void)
{
   CreateMissSpangRelation();
   CreateHitSpangRelation();
   g_pHitSpangTrait = MakeTraitFromRelation(g_pHitSpangs);
   g_pShowDmgMsg = new cNetMsg(&sShowDmgDesc);
}

void ShockDamageShutDown(void)
{
   SafeRelease(g_pMissSpangs);
   delete g_pShowDmgMsg;
}

#pragma on(unreferenced)
