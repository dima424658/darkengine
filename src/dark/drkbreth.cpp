/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkbreth.cpp,v 1.6 1999/12/20 16:46:54 adurant Exp $
#include <drkbreth.h>
#include <brethprp.h>
#include <propert_.h>
#include <propdef.h>
#include <dataops_.h>
#include <propsprs.h>
#include <propface.h>

#include <dmgmodel.h>
#include <dmgbase.h>

#include <objpos.h>
#include <media.h>
#include <wr.h>
#include <iobjsys.h>

#include <sdesbase.h>
#include <sdestool.h>

#include <config.h>
#include <cfgdbg.h>

#include <playrobj.h>
#include <camera.h>
#include <objedit.h>
#include <physapi.h>
#include <phmoapi.h>

// Include these last 
#include <dbmem.h>
#include <initguid.h>
#include <brethiid.h>

////////////////////////////////////////////////////////////
// 
// AIR ECONOMY (BREATHING & SUFFOCATING) 
//
// In the future this will be all Act/Reacty
//
////////////////////////////////////////////////////////////


//------------------------------------------------------------
// BREATH CONFIG PROP
//

class cBreathOps : public cClassDataOps<sBreathConfig>
{
   
}; 

class cBreathStore: public cSparseHashPropertyStore<cBreathOps>
{
}; 

typedef cSpecificProperty<IBreathProperty,&IID_IBreathProperty,sBreathConfig*,cBreathStore> cBreathPropBase; 

class cBreathProp : public cBreathPropBase
{
public:
   cBreathProp(const sPropertyDesc* desc) : cBreathPropBase(desc) {}; 

   STANDARD_DESCRIBE_TYPE(sBreathConfig); 
}; 

//
// Descriptor 
//

static sPropertyConstraint breath_constraints[] = 
{
   { kPropertyAutoCreate, PROP_AIRSUPPLY_NAME },
   { kPropertyNullConstraint },
};


static sPropertyDesc breath_desc = 
{
   PROP_BREATH_NAME,
   0, // flags
   breath_constraints, // constraints
   0, 0, // version 
   {
      "Dark Gamesys", 
      "Breath Config", 
   },
   kPropertyChangeLocally
};

//
// Struct Desc 
//

static sFieldDesc breath_fields[] = 
{
   { "Max Air (ms)", kFieldTypeInt, FieldLocation(sBreathConfig,max_air), },
   { "Drown Freq (ms)", kFieldTypeInt, FieldLocation(sBreathConfig,drown_freq), },
   { "Drown Damage", kFieldTypeInt, FieldLocation(sBreathConfig,drown_damage), },
   { "Recover rate", kFieldTypeFloat, FieldLocation(sBreathConfig,recover_rate),}, 

};

static sStructDesc breath_sdesc = StructDescBuild(sBreathConfig,0,breath_fields); 


//
// Factory
//
static IBreathProperty* gpBreathProp = NULL; 

static void init_breath_prop()
{
   gpBreathProp = new cBreathProp(&breath_desc); 

   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Register(&breath_sdesc); 
}

const sBreathConfig* GetObjBreathConfig(ObjID obj)
{
   sBreathConfig* cfg = NULL; 
   gpBreathProp->Get(obj,&cfg); 
   return cfg; 
}

void SetObjBreathConfig(ObjID obj, const sBreathConfig* cfg)
{
   gpBreathProp->Set(obj,(sBreathConfig*)cfg); 
}

//------------------------------------------------------------
// AIR SUPPLY PROP
//

static sPropertyConstraint air_constraints[] = 
{
   { kPropertyRequires, PROP_BREATH_NAME },
   { kPropertyNullConstraint },
};

struct sPropertyDesc air_desc = 
{
   PROP_AIRSUPPLY_NAME,
   kPropertyInstantiate, // flags
   air_constraints, // constraints
   0, 0, // version
   { "Dark Gamesys", "Air Supply" },
   kPropertyChangeLocally
}; 



static IIntProperty* gpAirProp = NULL; 

void init_air_prop()
{
   gpAirProp = CreateIntProperty(&air_desc,kPropertyImplLlist); 
}

void SetObjAirSupply(ObjID obj, int air)
{
   gpAirProp->Set(obj,air); 
}

int GetObjAirSupply(ObjID obj)
{
   int air = 1; 
   gpAirProp->Get(obj,&air); 
   return air; 
}

//------------------------------------------------------------
// INIT/TERM
//

IDamageModel* gpDamageModel = NULL; 
IObjectSystem* gpObjSys = NULL; 

void BreathSimInit()
{  
   init_breath_prop();
   init_air_prop(); 
   gpDamageModel = AppGetObj(IDamageModel); 
   gpObjSys = AppGetObj(IObjectSystem); 

}

void BreathSimTerm()
{
   SafeRelease(gpBreathProp); 
   SafeRelease(gpAirProp); 
   SafeRelease(gpDamageModel); 
   SafeRelease(gpObjSys); 
}

//------------------------------------------------------------
// Run da sim 
//

void BreathSimUpdateFrame(int ms)
{
   ObjID waterstim = gpObjSys->GetObjectNamed("WaterStim"); 
   sPropertyObjIter iter; 
   ObjID obj; 
   int air; 

   gpObjSys->Lock();
   
   gpAirProp->IterStart(&iter);
   while (gpAirProp->IterNextValue(&iter,&obj,&air))
      if (OBJ_IS_CONCRETE(obj))
      {
         // look up config
         sBreathConfig* cfg;  
         if (!gpBreathProp->Get(obj,&cfg))
         {
            Warning(("%s in air prop iterator, but no breath prop!", ObjWarnName(obj)));
            continue;
         }

         if (cfg->drown_freq == 0)
         {
            Warning(("%s has zero drown frequency!\n", ObjWarnName(obj)));
            continue;
         }
         
         // find medium
         ObjPos* pos = ObjPosGet(obj); 
         if (!pos) 
            continue; 

         Location loc = pos->loc; 

         // @HACK: player shouldn't breathe from chest 
	 // player breathes from head submodel if he has physics.
	 // if he doesn't have physics, use the old style of looking
	 // for the player camera.
	 // @HACK: more hack.  Player breathes from chest when
	 // not in firstperson mode(and no physics).  Yes, if the 
	 // player is standing
	 // in chest high water and switches to a camera on dry land
	 // he starts to need to breathe... but this isn't so bad, and
	 // is better than drowning when he switches to a view under
	 // water.  And this was a cheap fix.  
	 if (IsPlayerObj(obj))
	   if (PhysObjHasPhysics(obj))
	     {
	       int cell = CellFromLoc(&loc);
	       CameraGetLocation(PlayerCamera(),&loc.vec,NULL);
	       PhysGetSubModLocation(obj,PLAYER_HEAD,&loc.vec);
	       mxs_vector eye_offset = PlayerGetEyeOffset();
	       mx_addeq_vec(&loc.vec,&eye_offset);
	       loc.cell = -1;
	       loc.hint = cell;
	     }
	   else
	     {
	       // force cell & hint to update
	       int cell = CellFromLoc(&loc); 
	       // fill the vector, 
	       if (CameraGetMode(PlayerCamera()) == FIRST_PERSON)
		 CameraGetLocation(PlayerCamera(),&loc.vec,NULL); 
	       // hint based on the player's obj 
	       loc.cell = -1; 
	       loc.hint = cell; 
	     }
	 
         int cell = CellFromLoc(&loc); 
         if (cell == -1)
            continue; 

         int medium = WR_CELL(cell)->medium;

         int oldair = air; 
         if (medium == MEDIA_WATER)
         {
            air -= ms; 

            while (air < 0)
            {
               // damage the object
               sDamage damage = { cfg->drown_damage, waterstim }; 
               gpDamageModel->DamageObject(obj,OBJ_NULL,&damage); 
               
               // pay us for the damage in air...
               air += cfg->drown_freq; 
            }
         }
         else if (medium == MEDIA_AIR)
         {
            air += (int)(ms * cfg->recover_rate); 
            if (air > cfg->max_air) 
               air = cfg->max_air; 
         }

         if (air != oldair)
         {
            ConfigSpew("drown_spew",("Obj %d had %d air, now has %d\n",obj,oldair,air)); 
            gpAirProp->Set(obj,air); 
         }
      
      } 
   gpAirProp->IterStop(&iter); 

   gpObjSys->Unlock();
}






