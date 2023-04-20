/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
//$Header: r:/t2repos/thief2/src/sim/prjctile.cpp,v 1.94 2000/03/18 15:24:51 ccarollo Exp $
//
// Projectile-launching system
//

#include <lg.h>
#include <appagg.h>

#include <matrix.h>
#include <camera.h>

#include <prjctile.h>
#include <playrobj.h>
#include <timer.h>
#include <objsys.h>
#include <objpos.h>
#include <osysbase.h>
#include <physapi.h>
#include <phprop.h>
#include <phoprop.h>
#include <phcore.h>
#include <phmod.h>
#include <physcast.h>
#include <propface.h>
#include <objmodel.h>
#include <objshape.h>
#include <property.h>
#include <collprop.h>
#include <objslit.h>
#include <wr.h>
#include <sphrcst.h>
#include <sphrcsts.h>
#include <simtime.h>
#include <cfgdbg.h>
#include <portal.h>
#include <mprintf.h>

#include <aiapi.h>

// sound stuff
#include <config.h>
#include <schprop.h>
#include <schema.h>
#include <esnd.h>
#include <ctagset.h>

// For player-specific case
#include <playrobj.h>

#include <plycbllm.h> // for mode switching

// For isprojectile
#include <comtools.h>
#include <traitman.h>
#include <traitbas.h>
#include <objquery.h>

// for commands
#include <ctype.h>
#include <command.h>

// For firer relation
#include <proplink.h>
#include <linkqdb.h>
#include <propbase.h>
#include <linkbase.h>

#include <rooms.h>
#include <roomsys.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// PROJECTILE CODE.
//
// I MOVED THIS OUT OF GAMEMODE.C BECAUSE IT OFFENDED MY DELICATE
// SENSIBILITIES AS A PROGRAMMER.
//
//    - MAHK
// awwww, you're so delicate (must be said in appropriate aunt or uncle sort of voice)


#define FIRE_RATE 250

static ObjID gun_type = OBJ_NULL;

#define fireBullet(owner) launchProjectileDefault(owner,gun_type,1.0)

ObjID projArch = ROOT_ARCHETYPE;

BOOL has_projectile_type(ObjID obj);

static IFloatProperty* gpLaunchMassProp = NULL; 

static void compute_firer_pos(ObjID firer, mxs_vector *pos, mxs_angvec *ang, const mxs_vector *dir)
{
   Assert_(pos != NULL && ang != NULL);

   if (firer == PlayerObject() && PlayerCamera()->mode == FIRST_PERSON)
   {
      Camera *c = PlayerCamera();
      CameraGetLocation(c, pos, ang);
   }
   else
   {
      ObjPos *objpos = ObjPosGet(firer);
      *pos = objpos->loc.vec;
      *ang = objpos->fac;
   }

   if (dir)
   {
      mxs_matrix orien;
      mxs_vector z_vec;
      
      // forward
      mx_norm_vec(&orien.vec[0], dir);

      // build other vectors
      mx_mk_vec(&z_vec, 0, 0, 1);
      mx_cross_vec(&orien.vec[1], &z_vec, &orien.vec[0]);
      mx_cross_vec(&orien.vec[2], &orien.vec[0], &orien.vec[1]);

      mx_mat2ang(ang, &orien);
   }
}

ObjID launchProjectile(ObjID launcher, ObjID proj_arch, float power, int flags, const mxs_vector *add_vel, const mxs_vector *dir, const mxs_vector *loc)
{
   AutoAppIPtr_(ObjectSystem, pObjSys);

   BOOL  new_object = proj_arch==OBJ_NULL ? TRUE : !OBJ_IS_CONCRETE(proj_arch);
   BOOL  has_physics;
   ObjID proj_id = OBJ_NULL;

   mxs_vector *p_init_vel;
   mxs_vector default_vel;
   mxs_vector add_velocity;
   mxs_vector velocity;

   mxs_vector pos;
   mxs_angvec ang;
   mxs_matrix mat;

   // Create projectile object
   if (new_object)
   {
      // default
      if (proj_arch == OBJ_NULL)
         proj_arch = projArch;

      proj_id = pObjSys->BeginCreate(proj_arch, kObjectConcrete);
   }
   else
      proj_id = proj_arch;

   // Bail if unsuccessful
   if (proj_id == OBJ_NULL)
      return OBJ_NULL;

   // Determine initial position
   if (loc == NULL)
   {
      if (!new_object && (flags & PRJ_FLG_FROMPOS))
      {
         Position *pPos = ObjPosGet(proj_id);
         if (pPos)
         {
            mx_copy_vec(&pos, &pPos->loc.vec);
            pos = pPos->loc.vec;
            ang = pPos->fac;
         }
         else
         {
            Warning(("launchProjectile: FROMPOS flag set but obj has no position\n"));
            compute_firer_pos(launcher, &pos, &ang, dir);
         }
      }
      else
         compute_firer_pos(launcher, &pos, &ang, dir);
   }
   else
   {
      compute_firer_pos(launcher, &pos, &ang, dir);
      pos = *loc;
   }
   
   mx_ang2mat(&mat, &ang);

   ObjPosUpdate(proj_id, &pos, &ang);
   cRoom* pLauncherRoom = g_pRooms->GetObjRoom(launcher);
   if (pLauncherRoom != NULL)
   {
      g_pRooms->SetObjRoom(proj_id, NULL);
      g_pRooms->SetObjRoom(proj_id, pLauncherRoom);
      g_pRooms->UpdateRoomData(proj_id, pos);
   }
   if (g_pRooms->GetObjRoom(proj_id) == NULL)
   {
      cRoom* pRoom = g_pRooms->RoomFromPoint(pos);
      g_pRooms->SetObjRoom(proj_id, pRoom);
   }

   // Set up physics
   has_physics = TRUE;

   if (!PhysObjHasPhysics(proj_id))
   {
      if (flags & PRJ_FLG_NOPHYS) 
      {
         has_physics = FALSE;
      } 
      else 
      {
         g_pPhysTypeProp->Create(proj_id);

         PhysSetFlag(proj_id, kPMF_Projectile, TRUE);

         if (new_object)
         {
            if (flags & PRJ_FLG_GRAVITY)
               PhysSetGravity(proj_id, 1.0);
            else
               PhysSetGravity(proj_id, 0.0);
         }
      }
   }

   // Push away from launcher (must be physical)
   if ((flags & PRJ_FLG_PUSHOUT) && (PhysObjHasPhysics(launcher)))
   {
      mxs_real push_dist = ObjGetRadius(launcher) + ObjGetRadius(proj_id);
      Location start;
      Location end;
      Location hit;

      MakeLocationFromVector(&start, &pos);
      MakeLocationFromVector(&end, &pos);

      Verify(CellFromLoc(&start) != CELL_INVALID);

      mx_scale_addeq_vec(&end.vec, &mat.vec[0], push_dist);

      cPhysDimsProp *pDimsProp;
      g_pPhysDimsProp->Get(proj_id, &pDimsProp);

      ObjID hit_obj;

      if (pDimsProp->radius[0] == 0.0)
      {
         if (PhysRaycast(start, end, &hit, &hit_obj, 0.0) != kCollideNone)
            mx_interpolate_vec(&pos, &start.vec, &hit.vec, 0.95);
         else
            mx_copy_vec(&pos, &end.vec);
      }
      else
      {
         mxs_real hit_time = -1;

         PhysSetModLocation(proj_id, &ObjPosGet(launcher)->loc.vec);

         // Try spherecasting first
         if (PhysObjValidPos(proj_id, NULL))
         {
            int contactCount;
            if ((contactCount = SphrSpherecastStatic(&start, &end, pDimsProp->radius[0], 0)) > 0)
            {
               // Find the collision with the lowest timestamp
               sSphrContact *pContact = &gaSphrContact[0];
               for (int j=1; j<contactCount; j++)
               {
                  if (gaSphrContact[j].time < pContact->time)
                     pContact = &gaSphrContact[j];
               }

               hit_time = pContact->time * 0.95;
            }
         }
         else
            hit_time = 0.0;

         if (!PhysSphereIntersectsDoor(proj_id))
         {
            if (PhysRaycast(start, end, &hit, &hit_obj, pDimsProp->radius[0], kCollideAllPhys) != kCollideNone)
            {
               mxs_vector full, partial;   
               mxs_real our_hit_time;

               mx_sub_vec(&full, &end.vec, &start.vec);
               mx_sub_vec(&partial, &hit.vec, &start.vec);

               our_hit_time = 0.95 * sqrt(mx_mag2_vec(&partial) / mx_mag2_vec(&full));

               if ((hit_time < 0) || (our_hit_time < hit_time))
                  hit_time = our_hit_time;
            }
         }
         else
            hit_time = 0.0;

         if (hit_time > 0.0)
            mx_interpolate_vec(&pos, &start.vec, &end.vec, hit_time);
         else
         if (hit_time < 0.0)
            mx_copy_vec(&pos, &end.vec);
      }
   }

   if (has_physics) 
   {
      PhysSetModLocation(proj_id, &pos);
      PhysSetModRotation(proj_id, &ang);

      if (!PhysObjValidPos(proj_id, NULL) || PhysSphereIntersectsDoor(proj_id))
      {
         if (new_object)
         {
            pObjSys->EndCreate(proj_id);
            pObjSys->Destroy(proj_id);
         }
         else
            PhysDeregisterModel(proj_id);

         return OBJ_NULL;
      }

      if (g_pPhysInitVelProp->Get(proj_id, &p_init_vel))
         default_vel = *p_init_vel;
      else
         mx_mk_vec(&default_vel, 1, 0, 0);

      // Give owner's velocity or zero it out
      if (flags & PRJ_FLG_ZEROVEL)
         mx_zero_vec(&add_velocity);
      else
      {
         if (PhysObjHasPhysics(launcher))
            PhysGetVelocity(launcher, &add_velocity);
         else
            mx_zero_vec(&add_velocity);
      }

      mx_zero_vec(&velocity);

      // Add in object-relative default velocity
      for (int i=0; i<3; i++)
         mx_scale_addeq_vec(&velocity, &mat.vec[i], default_vel.el[i]);

      // Scale result to specified power
      mx_scaleeq_vec(&velocity, power);

      // Add in specified add-in velocity
      if (add_vel)
         mx_addeq_vec(&velocity, add_vel);
      mx_addeq_vec(&velocity, &add_velocity);

      // scale back velocity based on mass 
      float launcher_mass; 
      if ((flags & PRJ_FLG_MASSIVE) 
          && gpLaunchMassProp->Get(launcher,&launcher_mass))
      {
         // Basically, we're computing the momentum of the launcher
         // (based on velocity and launcher_mass)
         // And then we're "weighing" down the launcher with the 
         // additional mass of the projectile.  
         float mass = PhysGetMass(proj_id); 
         float coeff = launcher_mass/(mass+launcher_mass); 

         mx_scaleeq_vec(&velocity,coeff); 
      }

      // Set velocity
      PhysSetVelocity(proj_id, &velocity);
   }

   // should only happen on "real" objects
   if ((flags & PRJ_FLG_TELLAI) && IsPlayerObj(launcher))
   {
      AutoAppIPtr(AIManager);
      if (!!pAIManager)
         pAIManager->NotifyFastObj(launcher, proj_id, velocity);
   }

   // Inform relation
   if (!(flags&PRJ_FLG_NO_FIRER))
      SetProjectileFirer(proj_id, launcher);

   // If we're really launching a "projectile" then inform physics
   if (has_projectile_type(proj_id))
      PhysSetProjectile(proj_id);

   if (new_object)
      pObjSys->EndCreate(proj_id);

   // Play launch sound
   cTagSet SoundEvent("Event Launch"); 
   SoundEvent.Add(cTag("LaunchVel", (int) power));
   ESndPlayLoc(&SoundEvent, launcher, proj_id, &pos);

   // Attach projectile sound to object
   const char* pSchemaName;
   if (g_pProjSound->Get(proj_id, &pSchemaName))
      SchemaPlayObj((const Label*)pSchemaName, proj_id);

   return proj_id;
}

//////////////////////////////////////////////////

#define PROJECTILE_COLLISION  (COLLISION_NORESULT | COLLISION_SLAY)

static const char name[] = "Projectile";

void InitProjectileArchetype(void)
{
   ObjID arch = ROOT_ARCHETYPE;
   AutoAppIPtr_(TraitManager,TraitMan);
   HRESULT result = TraitMan->CreateBaseArchetype(name,&arch);

   if (SUCCEEDED(result) && result != S_FALSE)
   {
      // set the collision property
      ObjSetCollisionType(arch, PROJECTILE_COLLISION);
   }
   projArch = arch;
}
 
BOOL has_projectile_type(ObjID obj)
{
   if (obj == projArch) return TRUE;

   AutoAppIPtr_(TraitManager,TraitMan);
   return TraitMan->ObjHasDonor(obj,projArch);
}

// Now we just pass this through to physics
BOOL ObjIsProjectile(ObjID obj)
{
   return (PhysIsProjectile(obj));
}

////////////////////////////////////////


void shoot_gun_type(char* arg)
{
   // eliminate trailing whitespace
   for (char* s = arg +strlen(arg) - 1; s >= arg && isspace(*s); s--)
      *s = '\0'; 

   AutoAppIPtr_(ObjectSystem,ObjSys);
   ObjID arch = ObjSys->GetObjectNamed(arg);
   if (has_projectile_type(arch))
      gun_type = arch;

}

static Command commands[] =
{
   { "select_gun", FUNC_STRING, shoot_gun_type, "select_gun <archetype>  Shoot a bullet out of the player", HK_ALL },
};


////////////////////////////////////////

// Note that the Firer property is local-only. Networking ObjID properties
// is problematic at best, so we don't try; we let the relation take
// care of it.
static sPropertyDesc firerPropDesc =
{
   RELATION_FIRER_NAME,
   kPropertyNoInherit,
   NULL, // constraints
   0, 0, // Version
   {NULL, NULL,}, // ui strings
   kPropertyChangeLocally,
};

static sRelationDesc firerRelDesc =
{
   RELATION_FIRER_NAME,
   0,

};

static sRelationDataDesc firerDataDesc = { "None", 0 };


#define FIRER_IMPL kPropertyImplSparseHash

static IIntProperty* firer_prop = NULL;
static IRelation* firer_rel; 

static void init_firer_prop(void)
{
   ILinkQueryDatabase* db = CreateKnownDestLinkQueryDatabase();
   IProperty* prop = CreateLinkedProperty(&firerPropDesc,&firerRelDesc,&firerDataDesc,db,FIRER_IMPL);
   Verify(SUCCEEDED(prop->QueryInterface(IID_IIntProperty,(void**)&firer_prop)));
   Verify(SUCCEEDED(prop->QueryInterface(IID_IRelation,(void**)&firer_rel)));

   SafeRelease(prop);
   SafeRelease(db);
}

ObjID GetProjectileFirer(ObjID obj)
{
   ObjID firer = OBJ_NULL;
   if (firer_prop)
      firer_prop->Get(obj,&firer);
   return firer;
}

void SetProjectileFirer(ObjID bullet, ObjID firer)
{
   // go through the relation, so that listeners get called
   firer_prop->Delete(bullet); 
   if (firer != OBJ_NULL)
      firer_rel->Add(bullet,firer); 
}


////////////////////////////////////////

static sPropertyDesc launch_mass_property_desc =
{
   "LauncherMass",
   0, // flags,   
   NULL, // constraints
   1, 1, // version
   { "Physics: Projectile", "Launcher Mass" }, 
}; 

static void create_launch_mass_prop()
{
   gpLaunchMassProp = CreateFloatProperty(&launch_mass_property_desc,kPropertyImplSparseHash); 
}


/////////////////////////////////////////////////////////////
// Projectile sound property

IStringProperty* g_pProjSound;

static sPropertyDesc _g_ProjSoundPDesc =
{
   PROP_PROJ_SND_NAME, 0, 
   NULL, 0, 0, // constraints, versions
   { "Sound", "Projectile Sound" },
};

//////////////////////////////////////////////////////////

void InitProjectiles(void)
{
   COMMANDS(commands,HK_ALL);
   init_firer_prop();
   create_launch_mass_prop(); 
   g_pProjSound = CreateStringProperty(&_g_ProjSoundPDesc, kPropertyImplSparseHash);
}


