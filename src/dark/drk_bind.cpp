/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drk_bind.cpp,v 1.14 2000/02/19 13:09:06 toml Exp $

//dark binds processing
#include <stdlib.h>
#include <comtools.h>
#include <appagg.h>

#include <camera.h>
#include <command.h>
#include <config.h>
#include <contexts.h>
#include <gen_bind.h>
#include <headmove.h>
#include <phclimb.h>
#include <physapi.h>
#include <playtest.h>
#include <plyrmode.h>
#include <plyrspd.h>
#include <playrobj.h>
#include <weapon.h>
#include <drkwswd.h>

#include <drkmislp.h>
#include <drkplinv.h>
#include <drkwswd.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static BOOL g_on_ground;
static BOOL g_in_water;
static BOOL g_climbing;
static BOOL g_mantling;

EXTERN BOOL gNoMoveKeys; //shock movement lockout.  here hacked in for 
                         //jumpblock

#define MINEYEZOOM 1.0
#define MAXEYEZOOM 3.0
#define EYEZOOMFACTOR 1.1
#define DRUNKEYEZOOM 0.1

static void GetStates ()
{
   g_on_ground = PhysObjOnGround (PlayerObject ());
   g_in_water = PhysObjInWater (PlayerObject ());
   g_climbing = PhysObjIsClimbing (PlayerObject ());
   g_mantling = PhysObjIsMantling (PlayerObject ());
}

static BOOL disallow_action_and_unwind(void)
{
   if (GetPlayerMode() == kPM_Dead)
   {
      UnwindToMissionLoop();
      return TRUE;
   }
   return FALSE;
}


static char *BlockProc (char *, char *val, BOOL)
{
   if (gNoMoveKeys)
     return NULL;

   if (disallow_action_and_unwind())
      return NULL;

   double dval = atof (val);
   if (dval == 1.0)
      StartWeaponBlock (PlayerObject (), OBJ_NULL);
   else
      FinishWeaponAction (PlayerObject (), OBJ_NULL);
   return NULL;
}             


static char *UseWeaponOrItemProc (char *cmd, char *val, BOOL)
{  // this is NOT TOO COOL
   if (strnicmp(cmd,"use_weapon",strlen("use_weapon"))==0)
   {
      if (GetPlayerMode() == kPM_Dead)
         return NULL;
   }
   else
      if (disallow_action_and_unwind())
         return NULL;

   //if not in FIRST_PERSON mode, frobbing should return you.
   // and do nothing.
   Camera* playercam = PlayerCamera();
   if (CameraGetMode(playercam) != FIRST_PERSON)
     {
       CameraAttach(playercam,PlayerObject());
       return NULL;
     }

   double dval = atof (val);
   //for these, start is 0 and end is 1
   char str[16];
   strcpy (str, cmd);
   strcat (str, (dval == 0.0) ? " 1" : " 0");
   CommandExecute (str);

   return NULL;
}             


static char *InvSelectProc (char *cmd, char *val, BOOL)
{
   if (disallow_action_and_unwind())
      return NULL;

   char str[64];
   strcpy (str, cmd);
   strcat (str, " ");
   strcat (str, val);
   *val = '\0'; 

   CommandExecute (str);
   return NULL;
}             

static void
zoomIn( void )
{
  Camera* player_cam = PlayerCamera();

  //disallow zoom while in remote mode
  if (CameraIsRemote(player_cam))
    return;

  float curzoom = player_cam->zoom;

  curzoom *= EYEZOOMFACTOR;

  if (curzoom > MAXEYEZOOM)
    curzoom = MAXEYEZOOM;
  
  player_cam->zoom = curzoom;
}

static void
zoomOut( void )
{
  Camera* player_cam = PlayerCamera();

  //disallow zoom while in remote mode
  if (CameraIsRemote(player_cam))
    return;

  float curzoom = player_cam->zoom;

  curzoom /= EYEZOOMFACTOR;

  if (curzoom < MINEYEZOOM)
    curzoom = MINEYEZOOM;
  
  player_cam->zoom = curzoom;
}

static char *EyeZoomInProc (char *, char *val, BOOL)
{
   int i, count;
   BOOL bZoomIn;

   if (disallow_action_and_unwind())
      return NULL;

   count = atoi( val );
   if ( count == 0 ) {
      count = 1;
   }
   bZoomIn = (count > 0);
   count = abs( count );
   for ( i = 0; i < count; i++ ) {
      if ( bZoomIn ) {
         zoomIn();
      } else {
         zoomOut();
      }
   }

   return NULL;
}

static char *EyeZoomOutProc (char *, char *val, BOOL)
{
   int i, count;
   BOOL bZoomIn;

   if (disallow_action_and_unwind())
      return NULL;

   count = atoi( val );
   if ( count == 0 ) {
      count = 1;
   }
   bZoomIn = (count < 0);
   count = abs( count );
   for ( i = 0; i < count; i++ ) {
      if ( bZoomIn ) {
         zoomIn();
      } else {
         zoomOut();
      }
   }

  return NULL;
}

static char *EyeNoZoomProc (char *, char *val, BOOL)
{
  if (disallow_action_and_unwind())
    return NULL;

  Camera* player_cam = PlayerCamera();

  //disallow zoom while in remote mode
  if (CameraIsRemote(player_cam))
    return NULL;

  float curzoom = player_cam->zoom;

  curzoom = MINEYEZOOM;
  
  player_cam->zoom = curzoom;
  return NULL;
}

static char *EyeDrunkZoomProc (char *, char *val, BOOL)
{
  if (disallow_action_and_unwind())
    return NULL;

  Camera* player_cam = PlayerCamera();

  //disallow zoom while in remote mode
  if (CameraIsRemote(player_cam))
    return NULL;

  float curzoom = player_cam->zoom;

  curzoom /= EYEZOOMFACTOR;

  if (curzoom < DRUNKEYEZOOM)
    curzoom = DRUNKEYEZOOM;
  
  player_cam->zoom = curzoom;
  return NULL;
}

static char *JumpBlockProc (char *, char *val, BOOL)
{
   if (gNoMoveKeys)
      return NULL;

   if (disallow_action_and_unwind())
      return NULL;

   GetStates();

   double dval = atof (val);

   AutoAppIPtr_(Inventory, pInv);

   ObjID weapon = pInv->Selection(kCurrentWeapon);
   ObjID owner;

   if (weapon)
      owner = GetWeaponOwnerObjID(weapon);

   if (weapon && WeaponIsSword(owner, weapon))
   {
      if (dval == 1.0)
         StartWeaponBlock (PlayerObject(), OBJ_NULL);
      else
         FinishWeaponAction (PlayerObject(), OBJ_NULL);
   }
   else
   {
      if (dval == 1.0) {
         SetJumpState (TRUE);
         if ((g_on_ground || g_in_water || g_climbing) && !g_mantling)
            PhysPlayerJump (PlayerObject (), JUMP_SPEED);
      }
      else {
         SetJumpState (FALSE);
         if (g_mantling)
            BreakMantle (PlayerObject ());
      }
   }

   return NULL;
}

static char *DummyProc (char *, char *, BOOL)
{
  return NULL;
}

IB_var g_drk_ib_vars[] = {
//{var name, starting var val, flags, game callback to be called, agg callback to resolve conflicts, always NULL},
   {"block", "0", 0, BlockProc, NULL, NULL},
   {"use_weapon", "0", 0, UseWeaponOrItemProc, NULL, NULL},
   {"use_item", "0", 0, UseWeaponOrItemProc, NULL, NULL},
   {"inv_select", "0", 0, InvSelectProc, NULL, NULL},

   {"jumpblock", "0",  0, JumpBlockProc, NULL, NULL},
   {"zoominsome", "0", 0, EyeZoomInProc, NULL, NULL},
   {"zoomoutsome", "0", 0, EyeZoomOutProc, NULL, NULL},
   {"zoomoutfull", "0", 0, EyeNoZoomProc, NULL, NULL},
   {"drunkzoom", "0", 0, EyeDrunkZoomProc, NULL, NULL},
   
   {"auto_equip", "0", 0, DummyProc, NULL, NULL},
   {"auto_search", "0", 0, DummyProc, NULL, NULL},
   {"goal_notify", "0", 0, DummyProc, NULL, NULL},
   {"climb_touch", "0", 0, DummyProc, NULL, NULL},

   NULL
};



void InitDarkIBVars ()
{
   //input binding stuff
   g_pInputBinder->VarSet (g_drk_ib_vars);
}
