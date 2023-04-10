/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkgame.cpp,v 1.282 2000/02/19 13:20:22 toml Exp $
// shock specific game features

#include <string.h>
#include <math.h>

#include <2d.h>
#include <rect.h>
#include <kbcook.h>
#include <kb.h>
#include <keydefs.h>
#include <res.h>
#include <types.h>
#include <resapi.h>
#include <config.h>
#include <mprintf.h>
#include <matrix.h>
#include <r3d.h>
#include <filevar.h>
#include <rand.h>

#include <appname.h>
#include <objtype.h>
#include <globalid.h>
#include <rendobj.h>
#include <hud.h>
#include <editor.h>
#include <particle.h>
#include <fixtime.h>
#include <mouse.h>
#include <camera.h>
#include <config.h>
#include <headmove.h>
#include <gen_bind.h>
#include <appsfx.h>
#include <player.h>
#include <playrobj.h>
#include <editobj.h>
#include <objhp.h>
#include <frobctrl.h>
#include <frobprop.h>
#include <simtime.h>
#include <simstate.h>
#include <command.h>
#include <rendprop.h>
#include <cmdterm.h>
#include <bugterm.h>
#include <physapi.h>
#include <gamestr.h>
#include <schema.h>
#include <contain.h>
#include <gen_bind.h>
#include <lnkquery.h>
#include <traitman.h>
#include <transmod.h>
#include <plyrmode.h>
#include <questapi.h>
#include <phnet.h>

#include <objsys.h>
#include <osysbase.h>
#include <objquery.h>
#include <objpos.h>
#include <objlight.h>
#include <pick.h>
#include <shkrend.h> // for pick reset

#include <scrnovls.h>
#include <scrnmode.h>
#include <partprop.h>
#include <anim_txt.h>
#include <creature.h>

#include <cfgdbg.h>
#include <iobjnet.h>
#include <netmsg.h>

#include <netsynch.h>

#include <culpable.h>

#include <scrnloop.h>
#include <drkuires.h>

// for player stuff
#include <linkman.h>
#include <linkbase.h>
#include <relation.h>
#include <mnamprop.h>

// player physics
#include <phcore.h>
#include <phmod.h>
#include <phmods.h>
#include <phprop.h>

#include <shkutils.h>
#include <gunprop.h>
#include <shkpgapi.h>
#include <gunapi.h>
#include <shkctrl.h>
#include <shkpsapi.h>
#include <shkdmprp.h>
#include <shkprop.h>
#include <shkfrob.h>
#include <shkdrprp.h>
#include <gunproj.h>
#include <shkrep.h>
#include <shkradar.h>
#include <shkrlup2.h>
#include <shkreact.h>
#include <shksndpr.h>
#include <shkctrl.h>
#include <shkinvpr.h>
#include <shkcret.h>
#include <shkcmapi.h>
#include <shklooko.h>
#include <shkai.h>
#include <shkmulti.h>
#include <shkkey.h>
#include <shkcyber.h>
#include <shkhud.h>
#include <shkammov.h>
#include <shklinks.h>
#include <shkiface.h>
#include <shkrsrch.h>
#include <shk_bind.h>
#include <shkcurpr.h>
#include <shkhrm.h>
#include <shkscapi.h>
#include <shkparam.h>
#include <shksound.h>
#include <shklbox.h>
#include <shkoptmn.h>
#include <shkcobpr.h>
#include <shkpldmg.h>
#include <shkhome.h>
#include <shkemail.h>
#include <shkexp.h>
#include <shkpda.h>
#include <shkpsipw.h>
#include <shkpsiif.h>
#include <shkqbind.h>
#include <shkreprt.h>
#include <shkobjst.h>
#include <shkrndst.h>
#include <shkblood.h>
#include <shklding.h>
#include <shksavui.h>

// Metagame stuff
#include <shkmain.h>
#include <shkmenu.h>
#include <shkdebrf.h>
#include <init.h>

// Dark tools & other loot
#include <drkdiff.h>
#include <drkbreth.h>


// melee
#include <shkmelee.h>
#include <shkplcst.h>
#include <plycbllm.h>
#include <shkmelpr.h>
#include <weapon.h>

#include <scrptapi.h>
#include <scrptmsg.h>

extern "C"
{
#include <event.h>
}
#include <gadbox.h>

#include <prjctile.h>

//#include <pktalk.h>
//#include <pkreport.h>

#include <dmgmodel.h>
#include <dmgbase.h>

#include <loopapi.h>
#include <shkdlg.h>

#include <shkovrly.h>
#include <shkovcst.h>
#include <shkhacks.h>

#include <shkcharg.h>
#include <shkplayr.h>
#include <shkplprp.h>
#include <shkprop.h>
#include <shkinv.h>

#include <shkgame.h>
#include <shkdmg.h>
#include <shkloot.h>
#include <shkfsys.h>
#include <shkcurm.h>

#include <shkmusic.h>

#ifndef SHIP
#include <shkcmapi.h>
#include <shkcs.h>
#include <viewmgr.h>
#endif

#include <simbase.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define USE_DIST  100.0F

// fwd declaration
void ShockPsiPowerLevel(int which);

// is the user typing a message
static bool typingMode=FALSE;

static Point gLastpos = {-1,-1};

bool shock_mouse = FALSE;
EXTERN bool g_lgd3d;
IRes *shock_mouse_hnd;
IRes *gDefaultHnd;
int shock_cursor_mode = SCM_NORMAL;
 
extern Rect gun_rect;
extern Rect psi_rect;
static Rect power_rect = {{276,254}, {447,390}};
static Rect subpower_rect = {{456,254}, {629,390}};
ObjID drag_obj = OBJ_NULL; // what is ON the cursor
bool shock_mouse_loaded = FALSE; // have we loaded in cursor art yet?

// controllers in dark for focus/actions
BOOL active_focus=FALSE;
BOOL world_use_in_progress=FALSE;

#ifndef SHIP
void CameraPositionSpew(void);
#endif

void ShockDrawCursor(Point pt)
{
   // don't allow anything if player dead
   if (GetPlayerMode() == kPM_Dead)
      return;

   // now, depending on cursor mode, draw some help text
   char temp[255];
   cStr str;
   ObjID useobj;
   short dx, dy, w, bmh;

   // the easy part: draw the art
   if (gCursorHnd == gDefaultHnd)
      DrawCursorByHandle(gCursorHnd,pt); // we should probably decide this a better way....
   else
      DrawCursorByHandleCenter(gCursorHnd,pt);

   AutoAppIPtr(GameStrings);
   useobj = frobWorldSelectObj;
   if (useobj == OBJ_NULL)
      useobj = g_ifaceFocusObj;
   if (useobj == OBJ_NULL)
      return;

#ifdef EDITOR
   AutoAppIPtr(ShockCamera);
   if (pShockCamera->GetEditMode() != kCameraNormal)
      return;
#endif

   strcpy(temp,"");

   AutoAppIPtr(ContainSys);
   ObjID cont;

   switch (shock_cursor_mode)
   {
   case SCM_NORMAL:
      // okay, if the object we are over is a world object, use it's world text.
      if (useobj == frobWorldSelectObj)
      {
         str = pGameStrings->FetchObjString(useobj, PROP_WORLDCURSOR_NAME);
      }
      // otherwise, use its inventory text
      else
      {
         str = pGameStrings->FetchObjString(useobj, PROP_INVCURSOR_NAME);
      }
      cont = pContainSys->GetContainer(useobj);
      if ((cont == OBJ_NULL) || (cont == PlayerObject()))
         strcpy(temp,str);
      
      break;
   default:
      return;
      break;
   }

   if (strlen(temp) == 0)
      return;

   w = gr_font_string_width(gShockFont, temp);
   bmh = 16; // rather than compute
   dx = pt.x - (w / 2);

   sScrnMode smode;
   ScrnModeGet(&smode);
   if (dx + w + 10 > smode.w)
      dx = smode.w - w - 10;
   if (dx < 10)
      dx = 10;
   dy = pt.y + (bmh / 2) + 2;

   gr_set_fcolor(gShockTextColor);
   gr_font_string(gShockFontMono, temp, dx, dy);
}

static void doMouseFocusCheck(void)
{
   Point mpt;
   short mx, my;
   BOOL occlude;
   mouse_get_xy(&mx,&my);
   mpt.x = mx;  mpt.y = my;
   occlude = ShockOverlayMouseOcclude(mpt);

   // timeout any "stale" focus items
   if (!gFocusTouched)
      g_ifaceFocusObj = OBJ_NULL;
   gFocusTouched = FALSE;

   if ((frobWorldSelectObj == OBJ_NULL) || (frobWorldSelectObj != g_PickCurrentObj) || occlude)
   {
      int t;
      if (gPropHUDTime->Get(frobWorldSelectObj,&t))
      {
         if (t == 0)
            gPropHUDTime->Delete(frobWorldSelectObj);
      }
   }
   if ((g_PickCurrentObj != OBJ_NULL) && (!occlude))
   {
      if (frobWorldSelectObj != g_PickCurrentObj)
      {
         BOOL showhud;
         if ((gPropAllowHUDSelect->Get(g_PickCurrentObj,&showhud)) && showhud)
         {
            gPropHUDTime->Set(g_PickCurrentObj, 0);
            ShockInterfaceMouseOver(g_PickCurrentObj);
         }
      }
      frobWorldSelectObj = g_PickCurrentObj;
   }
   else
      // highlit_obj = 
      frobWorldSelectObj = OBJ_NULL;
}

// bonus space for cursors and such
#define TYPE_MAX 40
static char typebuf[TYPE_MAX+3];


// parse the key
EXTERN int hack_for_kbd_state(void);
#define CTRL_STATES  (KBM_LCTRL|KBM_RCTRL)

bool MouseMode(bool mode, bool clear)
{
   BOOL slim_mode;
   AutoAppIPtr(QuestData);
   slim_mode = pQuestData->Get("HideInterface");

   // don't allow into into cursor mode if we are pre-HUD
   if (mode && slim_mode)
      return(FALSE);

   // don't allow out of mouse mode if object on cursor
   if (!mode && shock_mouse && (drag_obj != OBJ_NULL))
      return(FALSE);

   shock_mouse = mode;
   ShockOverlayMouseMode(mode);

   if (mode)
   {
      RemoveIBHandler();
      InstallIBHandler (HK_GAME2_MODE, UI_EVENT_KBD_RAW | UI_EVENT_MOUSE | UI_EVENT_MOUSE_MOVE | UI_EVENT_JOY, FALSE);

      //dont let input binding sys process mouse move events
      headmoveSetRelPosX(0);
      headmoveSetRelPosY(0);
      g_pInputBinder->SetValidEvents (ALL_EVENTS & (~UI_EVENT_MOUSE_MOVE)); //  & (~UI_EVENT_MOUSE));

      if (!shock_mouse_loaded)
      {
         gDefaultHnd = shock_mouse_hnd = LoadPCX("cursor"); // , INTERFACE_PATH, ShockLoadNoPalette);
         shock_mouse_loaded = TRUE;
         // if ()   ??
         SetCursorByHandle(shock_mouse_hnd);
      }
      if (gLastpos.x != -1)
         mouse_put_xy(gLastpos.x,gLastpos.y);
      if (clear)
         ClearCursor();
   }
   else
   {
      // if we are in psi cursor mode, generate a OBJ_NULL usage
      if (shock_cursor_mode == SCM_PSI)
      {
         AutoAppIPtr(PlayerPsi);
         pPlayerPsi->PsiTarget(OBJ_NULL);
      }

      RemoveIBHandler();
      InstallIBHandler (HK_GAME_MODE, UI_EVENT_KBD_RAW | UI_EVENT_MOUSE | UI_EVENT_MOUSE_MOVE | UI_EVENT_JOY, FALSE);
      //turn input binder's mouse move events back on
      g_pInputBinder->SetValidEvents (ALL_EVENTS); //  & (~UI_EVENT_MOUSE));

      int centerx,centery;
      mouse_get_xy(&gLastpos.x,&gLastpos.y);
      sScrnMode smode;
      ScrnModeGet(&smode);
      centerx = smode.w / 2; //grd_visible_canvas->bm.w / 2;
      centery = smode.h / 2; //grd_visible_canvas->bm.h / 2;
      //mprintf("center = %d, %d\n",centerx,centery);

      // What the heck was this doing?  this is certain to cause whiplash on inv mode changes!
      // removed, 6/9/99 Xemu
      //centerx = 640 / 2;
      //centery = 480 / 2;
      mouse_put_xy(centerx, centery);
      // clear out HUD selection rectangle
      if (frobWorldSelectObj != OBJ_NULL)
      {
         int t;
         if (gPropHUDTime->Get(frobWorldSelectObj, &t) && (t == 0))
            gPropHUDTime->Delete(frobWorldSelectObj);
      }

      frobWorldSelectObj = OBJ_NULL;
      if (shock_mouse_loaded)
      {
         SafeFreeHnd(&shock_mouse_hnd);
         shock_mouse_loaded = FALSE;
      }

   }

   // make sure nothing gets stuck down
   //g_pInputBinder->PollAllKeys();

   return(TRUE);
}

void shock_check_keys(void)
{
   //static bool last_ctrl = FALSE;

   /*
   int kb_mods=hack_for_kbd_state();
   //bool old_mouse = shock_mouse;
   bool ctrl_down = (kb_mods & CTRL_STATES);
   static bool ctrl_used = FALSE;
   if (ctrl_down) 
   {
      if (shock_cursor_mode == SCM_NORMAL)
      {
         ShockLookCursor();
         ctrl_used = TRUE;
      }
   }
   else 
   {
      if (shock_cursor_mode == SCM_LOOK)
      {
         if (ctrl_used)
            ClearCursor();
      }
      else
         ctrl_used = FALSE;
   }
   */
}


#pragma off(unreferened)
bool shock_key_parse(int keycode)
{
   int kc;

   // MFDs and other overlaid interface elements
   if (keycode&KB_FLAG_DOWN)
      if (ShockOverlayHandleKey(keycode)) 
         return(TRUE);

   // camera recording mode
   AutoAppIPtr(ShockCamera);
   if (pShockCamera->GetEditMode() == kCameraRecord)
      return ShockControlKeyParse(keycode, pShockCamera->GetAttachObject(), pShockCamera->GetCameraSpeed());

   if (pShockCamera->GetEditMode() == kCameraEdit)
   {
      if (!(keycode&KB_FLAG_DOWN))
         return FALSE;

      kc=keycode&~KB_FLAG_DOWN;
      switch (kc)
      {
      case ' ':
         SetSimTimePassing(!IsSimTimePassing());
         return TRUE;
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
         pShockCamera->SwitchCamera(kc-'1');
         return TRUE;
      case '0':
         pShockCamera->SwitchCamera(10);
         return TRUE;
      }
      return FALSE;
   }

   if (keycode&KB_FLAG_DOWN)
   {
      kc=keycode&~KB_FLAG_DOWN;
      switch (kc)
      {
      //case 'w': g_playerMoveMode = kPlayerRun; return FALSE;
      //case 's': g_playerMoveMode = kPlayerWalk; return FALSE;
      //case 'x': g_playerMoveMode = kPlayerBackup; return FALSE;

      case KEY_BS:
         strcpy(typebuf,"");
         typingMode=TRUE;

      case KEY_F1:
         break;
        
      default:
         return FALSE;
      }
   }
   else 
   {
      /*
      switch(keycode)
      {
      case 'w': g_playerMoveMode = kPlayerMoveNone; return FALSE;
      case 's': g_playerMoveMode = kPlayerMoveNone; return FALSE;
      case 'x': g_playerMoveMode = kPlayerMoveNone; return FALSE;
      default:
         return FALSE;
      }
      */
   }
   return TRUE;
}

//////////
//
// Throwing an object back into the world. This is a little complex,
// because if this is a multiuser game, then we have to hand the object
// back to the world host.
//

static cNetMsg *g_pThrowObjMsg = NULL;

//
// This code should only be run on the world host, and deals with actually
// putting the object back into the world:
//
static void handleThrowObj(ObjID obj, ObjID src)
{
   // Take the object back from the thrower:
   AutoAppIPtr(NetManager);
   if (pNetManager->IsNetworkGame()) {
      AutoAppIPtr(ObjectNetworking);
      pObjectNetworking->ObjTakeOver(obj);
   }

   // Try to deal with throwing out a melee weapon that is still
   // animating its close
   AutoAppIPtr(ShockPlayer);
   if (IsMelee(obj) && IsCurrentPlayerArm(obj))
      PlayerSwitchMode(kPlayerModeInvalid);

   ObjPos *pos = ObjPosGet(src);
   mxs_vector plrpos, dir;  
   mxs_angvec plrang;
   mxs_matrix mat; // There is no spoon, only the matrix

   mx_copy_vec(&plrpos,&pos->loc.vec);

   // Place the object at the player's location, so networking has a more
   // or less rational start position for the thing:
   ObjPosCopyUpdate(obj, pos);

   ObjSetHasRefs(obj,TRUE);
   PhysRegisterSphereDefault(obj);

   sGameParams *params = GetGameParams();
   int randval = Rand() % 100;
   float power;

   if (config_is_defined("disable_random_throw"))
      power = params->throwpower;
   else
      power = params->throwpower * (0.9 + (0.2 * (float(randval) / 100)));

   // get player angle, and disturb it a touch
   memcpy(&plrang,&pos->fac,sizeof(mxs_angvec));
   //plrang = pos->fac;
   if (!config_is_defined("disable_random_throw"))
      plrang.tz += (Rand() % 0x0A00) - 0x0500;
   mx_ang2mat(&mat,&plrang);

   // set our final throw direction in world coords
   mx_copy_vec(&dir,&mat.vec[0]);

   if (launchProjectile(src,obj,power,PRJ_FLG_PUSHOUT|PRJ_FLG_MASSIVE|PRJ_FLG_NO_FIRER,NULL,&dir,NULL) == OBJ_NULL)
   {
      // Head too close to some surface, launch from body instead
      mxs_vector loc;
      mx_copy_vec(&loc, &ObjPosGet(src)->loc.vec);

      launchProjectile(src,obj,power,PRJ_FLG_PUSHOUT|PRJ_FLG_MASSIVE|PRJ_FLG_NO_FIRER,NULL,&dir,&loc);
   }
   PhysNetBroadcastObjPosByObj(obj);
}

static sNetMsgDesc sThrowObjDesc =
{
   kNMF_SendToHost,
   "ThrowObj",
   "Throw Object Back to World",
   NULL,
   handleThrowObj,
   {{kNMPT_SenderObjID, kNMPF_None, "Obj"},
    {kNMPT_SenderObjID, kNMPF_None, "From"},
    {kNMPT_End}}
};

//
// Tell the default host that we're throwing the object back to the world
// Client code
//
void ThrowObj(ObjID o, ObjID src)
{
   // If the host is throwing the thing, then this will just loop back
   // and land in handleThrowObj:
   g_pThrowObjMsg->Send(OBJ_NULL, o, src);

   // And now, the object is no longer on our cursor.
   if (o == drag_obj)
      ShockInvLoadCursor(OBJ_NULL);
}

//////////

// player is interacting with the interface / frobbing
void ShockInterfaceUseItem()
{
   // don't allow anything if player dead
   if (GetPlayerMode() == kPM_Dead)
      return;

   if (shock_cursor_mode == SCM_LOOK)
   {
      ClearCursor();
      return;
   }

   Point pos;
   mouse_get_xy(&pos.x, &pos.y);

   // kind of a misnomer now, since doubleclicks are gone
   ShockOverlayDoubleClick(pos);

   ShockInvRefresh();
}

// player wants to change mode with no other effects
void ShockToggleMode()
{
   // don't allow anything if player dead
   if (GetPlayerMode() == kPM_Dead)
      return;
   MouseMode(!shock_mouse,TRUE);
}

// toggle mode, first frobbing what is under the cursor
void ShockFrobObject() 
{
   // don't allow anything if player dead
   if (GetPlayerMode() == kPM_Dead)
      return;
   ShockDoFrob(FALSE);
}

void ShockFrobObjectInv() 
{
   // don't allow anything if player dead
   if (GetPlayerMode() == kPM_Dead)
      return;
   // if we have a world object, frob it
   if (frobWorldSelectObj != OBJ_NULL)
   {
      ShockDoFrob(FALSE);
   }
   // otherwise, inv-frob 
   else
   {
      ShockInterfaceUseItem();
   }
}

void ShockFrobAndMaybeToggleMode() // BOOL start
{
   // don't allow anything if player dead
   if (GetPlayerMode() == kPM_Dead)
      return;
   // only do stuff on the downs
   //if (!start)
   //   return;

   if (frobWorldSelectObj == OBJ_NULL)
   {
      if (shock_cursor_mode == SCM_NORMAL)
         MouseMode(!shock_mouse, TRUE);
   }
   else
   {
      ShockDoFrob(FALSE);
      BOOL switchmode = FALSE;

      if (shock_cursor_mode == SCM_DRAGOBJ)
      {
         switchmode = TRUE;
      }
      else
      {
         // if the object we are trying to frob has no relevant
         // frob engine properties, then allow a switch
         AutoAppIPtr(ObjectSystem);
         if (pObjectSystem->Exists(frobWorldSelectObj))
         {
            sFrobInfo *frobinfo;
            if (!pFrobInfoProp->Get(frobWorldSelectObj, &frobinfo))
               switchmode = TRUE;
            else if (frobinfo->actions[kFrobLocWorld] == 0)
               switchmode = TRUE;
         }
      }
      if (switchmode)
      {
         gLastpos.x = -1;
         MouseMode(!shock_mouse, TRUE);
      }
   }
}

void ShockHilightObject()
{
   AutoAppIPtr(NetManager);
   if (!pNetManager->Networking())
      // This feature only really makes sense in multiplayer
      return;

   if ((frobWorldSelectObj != OBJ_NULL) && ObjHasRefs(frobWorldSelectObj))
   {
      ShockBroadcastHilightObject(frobWorldSelectObj);
   }
}

//////////
//
// mouse parsing

BOOL ShockInterfaceClick()
{
   // don't allow anything if player dead
   if (GetPlayerMode() == kPM_Dead)
      return(TRUE);

   Point pos;
   BOOL retval = FALSE;
   mouse_get_xy(&pos.x, &pos.y);

   if (ShockOverlayClick(pos)) // interface hacks
      return(TRUE);

   switch (shock_cursor_mode)
   {
   case SCM_DRAGOBJ:
      {
         BOOL throw_me = FALSE;
         if (frobWorldSelectObj == OBJ_NULL)
            throw_me = TRUE;
         else
         {
            frobInvSelectObj = frobWorldSelectObj; // set the target of the tool frob
            ShockDoFrob(FALSE);
            //ClearCursor();
            if (ShockScriptAllowSwap())
               throw_me = TRUE;
         }
         if (throw_me && (drag_obj != OBJ_NULL))
         {
            // check to break researching
            ShockResearchCheckObj(drag_obj);
            // throw into world
            ThrowObj(drag_obj,PlayerObject());
            retval = true;
         }
      }
      break;
   case SCM_LOOK:
      //retval = TRUE;
      /*
      if (frobWorldSelectObj != OBJ_NULL)
      {
         ShockLookPopup(frobWorldSelectObj);
         retval = TRUE;
      }
      */
      //ClearCursor();
      break;
   case SCM_PSI:
      // hmm, perhaps this should just be disallowed?
      AutoAppIPtr(PlayerPsi);
      pPlayerPsi->PsiTarget(frobWorldSelectObj);
      ClearCursor();
      retval = TRUE;
      break;
   }
   return(retval);
}

// fire begin / stop
static void ShockFireWeapon(BOOL start)
{
   if (GetPlayerMode() == kPM_Dead)
   {
      // removed 5/31/99: Xemu
      /*
      // finish dying, that is to say, unwind or resurrect
      AutoAppIPtr(ScriptMan);
      sScrMsg msg(PlayerObject(),"FinishDying"); 
      msg.flags |= kSMF_MsgPostToOwner;
      pScriptMan->SendMessage(&msg); 
      */
      return;
   }
   //mprintf("firing weapon!\n");

   // no firing if no interface
   BOOL slim_mode;
   AutoAppIPtr(QuestData);
   slim_mode = pQuestData->Get("HideInterface");
   if (slim_mode)
      return;

   AutoAppIPtr(PlayerGun);
   AutoAppIPtr(ShockPlayer);

   if (start)
   {
      if (pPlayerGun->Get() != OBJ_NULL)
         pPlayerGun->PullTrigger();
      StartMeleeAttack(PlayerObject(), GetWeaponObjID(PlayerObject()));
   }
   else
   {
      if (pPlayerGun->Get() != OBJ_NULL)
         pPlayerGun->ReleaseTrigger();
      FinishMeleeAction(PlayerObject(), GetWeaponObjID(PlayerObject()));
   }
}

// query cursor
static void ShockQueryMode(BOOL start)
{
   // don't allow anything if player dead
   if (GetPlayerMode() == kPM_Dead)
      return;
   if (start)
   {
      if (shock_cursor_mode == SCM_NORMAL)
         ShockLookCursor();
   }
   else
   {
      if (shock_cursor_mode == SCM_LOOK)
         ClearCursor();
   }
}

static void ShockSplitMode(BOOL start)
{
   // don't allow anything if player dead
   if (GetPlayerMode() == kPM_Dead)
      return;
   if (start)
   {
      if (shock_cursor_mode == SCM_NORMAL)
         ShockSplitCursor();
   }
   else
   {
      if (shock_cursor_mode == SCM_SPLIT)
         ClearCursor();
   }
}

// drag & drop
#define DDNONE 0
#define DDFROB 1
#define DDMODE 2

static void ShockDragDropCore(BOOL start, int func)
{
   // don't allow anything if player dead
   if (GetPlayerMode() == kPM_Dead)
      return;
   Point pos;
   mouse_get_xy(&pos.x, &pos.y);

   if (start)
   {
      if (ShockOverlayDragDrop(pos, start))
         return;
   }
   else
   {
      int oldmode = shock_cursor_mode;
      ShockInterfaceClick();
      if (ShockOverlayDragDrop(pos, start))
         return;

      if ((shock_cursor_mode != SCM_NORMAL) || (oldmode != SCM_NORMAL))
         return;

      switch (func)
      {
      case DDFROB:
         ShockDoFrob(FALSE);
         break;
      case DDMODE:
         MouseMode(!shock_mouse,TRUE);
         break;
      }
   }
}

static void ShockDragDropFrob(BOOL start)
{
   ShockDragDropCore(start, DDFROB);
}

static void ShockDragDropSimple(BOOL start)
{
   ShockDragDropCore(start, DDNONE);
}

static void ShockDragDropMode(BOOL start)
{
   ShockDragDropCore(start, DDMODE);
}

static void ShockToggleInv(void)
{
   ShockOverlayChange(kOverlayInv, kOverlayModeToggle);
}

static void ShockToggleCompass(void)
{
   ObjID compass;
   AutoAppIPtr(ShockPlayer);
   AutoAppIPtr(ScriptMan);
   compass = pShockPlayer->GetEquip(PlayerObject(),kEquipCompass);
   if (compass == OBJ_NULL)
      return;

   // don't allow the compass to be turned on until
   // we have the interface "installed"
   BOOL slim_mode;
   AutoAppIPtr(QuestData);
   slim_mode = pQuestData->Get("HideInterface");
   if (slim_mode)
      return;

   sScrMsg msg(compass,"Toggle"); 
   pScriptMan->SendMessage(&msg); 
}

static void ShockCycleAmmo(void)
{
   ShockAmmoChangeTypes();
}

static void ShockSettingToggle(void)
{
   int curmode;
   ObjID gun;
   int wpntype;

   AutoAppIPtr(PlayerGun);
   AutoAppIPtr(PlayerPsi);
   AutoAppIPtr(ShockPlayer);

   gun = pPlayerGun->Get();
   if (gun == OBJ_NULL)
      return;

   g_pWeaponTypeProperty->Get(gun,&wpntype);

   // for psi amp, treat it as a cycle on the current level
   if (wpntype == kWeaponPsiAmp)
   {
      ePsiPowers power = pPlayerPsi->GetSelectedPower();
      if (power != kPsiNone)
      {
         int level = pShockPlayer->FindPsiLevel(power);
         ShockPsiPowerLevel(level);
      }
      return;
   }

   // on a melee weapon, do nothing at all
   if (g_pMeleeTypeProperty->IsRelevant(gun))
      return;

   curmode = GunGetSetting(gun);
   GunSetSetting(gun,!curmode);

   // text feedback
   char temp[255], fmt[255];
   cStr str;
   extern char *setting_headlines[2]; // hack

   AutoAppIPtr(GameStrings);
   str = pGameStrings->FetchObjString(gun,setting_headlines[!curmode]);
   ShockStringFetch(fmt,sizeof(fmt), "SettingChange", "misc");
   sprintf(temp,fmt,str);
   ShockOverlayAddText(temp,DEFAULT_MSG_TIME);
}

static void ShockPsiPowerSelect(void)
{
   // should this just be always on?
   // should it play an activation sound effect?
   ShockOverlayChange(kOverlayPsi, kOverlayModeToggle);
}

#define MAX_EQUIP_CANDIDATES  64

static int equip_val(ObjID o)
{
   int retval = 0;
   // basically, return the condition
   retval = retval - (GunGetCondition(o) * 100);

   // out of ammo is bad
   if (GunStateGetAmmo(o) == 0)
      retval = retval + 10000;

   // introduce a minor objID element so that there are fewer/no ties
   retval = retval + (o % 100);

   return(retval);
}

static int equip_compare(const void *p, const void *q)
{
   ObjID *a = (ObjID *) p;
   ObjID *b = (ObjID *) q;

   if (equip_val(*b) > equip_val(*a))
      return -1;  
   return (equip_val(*b) < equip_val(*a));
}

static void ShockUseObj(char *args)
{
   AutoAppIPtr(ContainSys);
   AutoAppIPtr(ObjectSystem);
   AutoAppIPtr(TraitManager);

   // no hotkey usage if dead
   if (GetPlayerMode() == kPM_Dead)
      return;

   // no hotkey usage if no interface
   BOOL slim_mode;
   AutoAppIPtr(QuestData);
   slim_mode = pQuestData->Get("HideInterface");
   if (slim_mode)
      return;

   sContainIter *iterp;
   ObjID arch, invobj;

   arch = pObjectSystem->GetObjectNamed(args);

   iterp = pContainSys->IterStart(PlayerObject());
   while (!iterp->finished)
   {
      invobj = iterp->containee;

      // Use it as if double clicked in inventory
      if (pTraitManager->ObjHasDonor(invobj, arch))
      {
         frobInvSelectObj = invobj;
         ShockDoFrob(TRUE);
         pContainSys->IterEnd(iterp);

         ShockInvRefresh();
         return;
      }
      pContainSys->IterNext(iterp);
   }
   pContainSys->IterEnd(iterp);

   // give feedback
   char temp[255],fmt[255];
   cStr str;
   ShockStringFetch(fmt,sizeof(fmt),"NotInInventory","misc");
   AutoAppIPtr(GameStrings);
   str = pGameStrings->FetchObjString(arch, PROP_OBJSHORTNAME_NAME);
   sprintf(temp,fmt,str);
   ShockOverlayAddText(temp,DEFAULT_MSG_TIME);
}

static BOOL ShockWeaponEquipCore(char *args, BOOL spew)
{
   // search the player's inventory for a matching weapon
   sContainIter *iterp;
   ObjID weaparch, playerobj, prevobj, newobj, invobj;
   ObjID candidates[MAX_EQUIP_CANDIDATES];
   eContainType newloc;
   int count = 0;
   int i;
   BOOL horrible_hack = FALSE;
   BOOL addme;
   ObjID hack_weaparch;

   AutoAppIPtr(PlayerGun);
   AutoAppIPtr(ContainSys);
   AutoAppIPtr(ObjectSystem);
   AutoAppIPtr(TraitManager);
   AutoAppIPtr(ShockPlayer);
   AutoAppIPtr(QuestData);

   // dont switch if no interface
   BOOL slim_mode;
   slim_mode = pQuestData->Get("HideInterface");
   if (slim_mode)
      return(FALSE);
  
   // don't switch if we are dead
   if (GetPlayerMode() == kPM_Dead)
      return(FALSE);

   // don't do jack if we are in the middle of reloading
   if (pPlayerGun->IsReloading())
      return(FALSE);

   // dont let you switch weapons mid firing
   ObjID gun = pPlayerGun->Get();
   if ((gun != OBJ_NULL) && pPlayerGun->IsTriggerPulled())
      return(FALSE);

   weaparch = pObjectSystem->GetObjectNamed(args);
   // bail out if the request object doesn't exist
   if (weaparch == OBJ_NULL)
      return(FALSE);

   for (i=0; i < MAX_EQUIP_CANDIDATES; i++)
      candidates[i] = OBJ_NULL;

   playerobj = PlayerObject();
   newobj = OBJ_NULL;
   prevobj = pShockPlayer->GetEquip(playerobj, kEquipWeapon);

   // GRUESOME HACK ALERT!  woop woop woop
   // We have this other types of weapons, "hybrid_shotgun", that wants to be
   // equippable in the same set as normal shotguns.  Gross, eh?
   // Mahk rightly points out that we could have used a metaproperty that both
   // of these inherit from.  Good idea, too bad I already implemented this without it.
   if (stricmp(args,"shotgun") == 0)
   {
      horrible_hack = TRUE;
      hack_weaparch = pObjectSystem->GetObjectNamed("hybrid_shotgun");
   }

   iterp = pContainSys->IterStart(playerobj);
   while (!iterp->finished)
   {
      invobj = iterp->containee;
      addme = FALSE;

      // equip it, maybe
      if (invobj != prevobj) 
      {
         if (pTraitManager->ObjHasDonor(invobj, weaparch))
            addme = TRUE;
         else if (horrible_hack)
         {
            // okay, we are a shotgun (ie, horrible_hack is true)
            // so add all the horrible hybrid_shotgun-s to our list as well
            if (pTraitManager->ObjHasDonor(invobj, hack_weaparch))
               addme = TRUE;
         }
      }

      if (addme)
      {
         eObjState st;
         st = ObjGetObjState(invobj);
         if (st == kObjStateNormal)
         {
            candidates[count] = invobj;
            count++;
            // abort out if somehow we have more objects than we can contemplate
            if (count == MAX_EQUIP_CANDIDATES)
               break;
         }
      }
      pContainSys->IterNext(iterp);
   }
   pContainSys->IterEnd(iterp);

   // if we have no canditates, punt
   if (count == 0)
      return(FALSE);

   // sort the candidates by quality
   qsort(candidates, count, sizeof(candidates[0]), equip_compare);

   // if we have no previous considerations, take the first (best)
   if ((prevobj == OBJ_NULL) || (!pTraitManager->ObjHasDonor(prevobj, weaparch)))
      newobj = candidates[0];
   else
   {
      // we want the candidate which is closest above our current object 
      for (i=0; i < count; i++)
      {
         if (equip_val(candidates[i]) > equip_val(prevobj))
         {
            newobj = candidates[i];
            break;
         }
      }
      // if there are none, take the first
      if (newobj == OBJ_NULL)
         newobj = candidates[0];
   }


   // okay, do the swap
   if (newobj != OBJ_NULL)
   {
      if (prevobj != OBJ_NULL)
      {
         // first unequip the old thing
         newloc = pContainSys->IsHeld(playerobj,newobj);
         pShockPlayer->Equip(playerobj, kEquipWeapon, OBJ_NULL,spew);
      }
      // equip the new thing
      BOOL equipped;
      equipped = pShockPlayer->Equip(playerobj, kEquipWeapon, newobj, spew);

      if (prevobj != OBJ_NULL)
      {
         if (equipped)
         {
            // put the old thing back in the same spot as the original
            pContainSys->Add(playerobj, prevobj, newloc, CTF_NONE);
            return(TRUE);
         }
         else
         {
            // we failed to equip a new object, so put the old one back in hand
            pShockPlayer->Equip(playerobj, kEquipWeapon, prevobj, FALSE); // spew);
            return(FALSE);
         }
      }
   }
   return(FALSE);
}

static void ShockWeaponEquip(char *args)
{
   ShockWeaponEquipCore(args, TRUE);
}


static char *weapon_order[] = {
   "psi amp","wrench","pistol","shotgun","assault rifle",
   "laser pistol","EMP Rifle","Electro Shock","Gren Launcher","Stasis Field Generator",
   "Fusion Cannon","Crystal Shard","Viral Prolif","Worm Launcher",
};

#define WEAPON_CYCLE (sizeof(weapon_order) / sizeof(char *))
static void ShockWeaponCycle(int dir)
{
   ObjID current, weaparch;
   char archname[255];
   int i, count;
   int num = 0;

   AutoAppIPtr(ShockPlayer);
   AutoAppIPtr(TraitManager);
   AutoAppIPtr(ObjectSystem);

   // find the currently equipped weapon
   current = pShockPlayer->GetEquip(PlayerObject(), kEquipWeapon);
   if (current == OBJ_NULL)
      num = 0;
   else
   {
      weaparch = pTraitManager->GetArchetype(current);
      strcpy(archname, pObjectSystem->GetName(weaparch));

      for (i=0; i < WEAPON_CYCLE; i++)
      {
         //mprintf("comparing %s to %s\n",archname, weapon_order[i]);
         if (stricmp(archname,weapon_order[i]) == 0)
         {
            // a match!
            num = i + dir;
         }
      }
   }

   count = 0;
   while (count < WEAPON_CYCLE)
   {
      count++;
      // try to equip this type of weapon
      if (num == WEAPON_CYCLE)
         num = 0;
      if (num < 0)
         num = WEAPON_CYCLE - 1;
      if (ShockWeaponEquipCore(weapon_order[num], FALSE))
         count = WEAPON_CYCLE;
      num = num + dir;
   }
}

static void HackFOVCompute(void)
{
   mxs_vector vecs[4];
   mxs_vector *v1, *v2;
   mxs_matrix m1, m2;
   mxs_angvec av1, av2;
   r3_get_view_pyr_vecs(vecs);

   v1 = &vecs[0];
   v2 = &vecs[2];

   // take the view pyramid and turn it into angvecs
   // by first converting into a rotation matrix
   mx_mk_move_x_mat(&m1, v1);
   mx_mk_move_x_mat(&m2, v2);

   // then deconstructing that 
   mx_mat2ang(&av1, &m1);
   mx_mat2ang(&av2, &m2);

   //mprintf ("v1 = %g, %g, %g  v2 = %g, %g, %g \n",av1.el[0],av1.el[1],av1.el[2],av2.el[0],av2.el[1],av2.el[2]);
   //mprintf ("delta = %g %g %g\n",av1.el[0] - av2.el[0],av1.el[1] - av2.el[1],av1.el[2] - av2.el[2]);
}

#pragma on(unreferenced)

// per frame updates, for now just spells
// #define PICK_DIST 35
void shock_sim_update_frame(int ms)
{
   ectsAnimTxtTime=GetSimTime();
   ectsAnimTxtUpdateAll();

   Point mpt;
   mouse_get_xy(&mpt.x,&mpt.y);

   headmoveCheck(PlayerCamera(),ms);
   playerHeadControl();
   // highlit_obj = 
   doMouseFocusCheck();

   sGameParams *params = GetGameParams();
   if (!shock_mouse)
   {
      // set focus way out for non-cursor mode
      //PickSetFocus(fix_make(mpt.x, 0), fix_make(mpt.y, 0), 10000); 

      // need support for farther projection for psi powers?
      sScrnMode smode;
      ScrnModeGet(&smode);
      
      PickSetFocus(fix_make(smode.w / 2,0),fix_make(smode.h / 2,0),params->frobdist);
      //PickSetFocus(fix_make(smode.w / 2,0),fix_make(smode.h / 2,0),64);

      //doHeadFocusCheck();
   }
   else
   {
      // pull focus in
      PickSetFocus(fix_make(mpt.x, 0), fix_make(mpt.y, 0), params->frobdist); 
   }

   /*
   // Jon: what is this doing?
   // Xemu:  I think this is intended for checking object consistency per frame and whatnot

   IObjectQuery* pQuery;
   IObjectSystem* pOS = AppGetObj(IObjectSystem);

   pQuery = IObjectSystem_Iter(pOS, kObjectConcrete);
   for (; !IObjectQuery_Done(pQuery); IObjectQuery_Next(pQuery))
   {
      ObjID id = IObjectQuery_Object(pQuery);

   }
   SafeRelease(pQuery);
   SafeRelease(pOS);
   */

   ShockResearchLoop();
   PickFrameUpdate();
   shock_pick_reset();

   PlayerCbllmUpdate(ms);
   BreathSimUpdateFrame(ms); 
}

// post render frame updates
void shock_rend_update_frame(void)
{
//   update_player_medium_sounds();
}

#pragma off(unreferenced)
static bool key_handler_func(uiEvent* ev, Region* r, void* data)
{
   uiCookedKeyEvent* kev = (uiCookedKeyEvent*)ev;
   return shock_key_parse(kev->code);
}
#pragma on(unreferenced)

//
// In-game command terminal
//

#define NUM_CMD_TERM_LINES 2
#define NUM_BUG_TERM_LINES 10
#define CMD_Y_MARGIN 2

static void build_cmd_term(LGadRoot* root)
{
   Rect r = *LGadBoxRect(root);
   short w,h;

   guiStyleSetupFont(NULL,StyleFontNormal);
   gr_string_size("X",&w,&h);
   guiStyleCleanupFont(NULL,StyleFontNormal);

   r.lr.y = h * NUM_CMD_TERM_LINES + CMD_Y_MARGIN;
   CreateCommandTerminal(root,&r,kCmdTermHideUnfocused);

   r.lr.y = h * NUM_BUG_TERM_LINES + CMD_Y_MARGIN;
   CreateBugTerminal(root,&r,kCmdTermHideUnfocused);
}

static int key_handler_id;
static int mouse_handler_id;

void shock_start_gamemode(void)
{
   LGadRoot* root = LGadCurrentRoot();
   uiInstallRegionHandler(LGadBoxRegion(root),UI_EVENT_KBD_COOKED,key_handler_func,NULL,&key_handler_id);
   //uiInstallRegionHandler(LGadBoxRegion(root),UI_EVENT_MOUSE,mouse_handler_func,NULL,&mouse_handler_id);
   g_pInputBinder->SetValidEvents (ALL_EVENTS); //  & (~UI_EVENT_MOUSE));

   ShockUtilInitColor();

   PickSetCanvas();

   //uiDoubleClickTime = 250;
   //uiDoubleClickDelay = 100;

   build_cmd_term(root);

   if (gScrnLoopSetModeFailed)
   {
      cStr str = FetchUIString("misc","set_mode_failed","strings"); 
      ShockOverlayAddText(str,DEFAULT_MSG_TIME); 
   }
}

void shock_end_gamemode(void)
{
   if (shock_mouse)
   {
      MouseMode(FALSE,TRUE);
   }

   //uiDoubleClickTime = 0;
   //uiDoubleClickDelay = 0;

   uiShowMouse(NULL);

   if (shock_mouse_loaded)
   {
      //shock_mouse_hnd->Unlock();
      SafeFreeHnd(&shock_mouse_hnd);
      shock_mouse_loaded = FALSE;
   }
   
   LGadRoot* root = LGadCurrentRoot();
   uiRemoveRegionHandler(LGadBoxRegion(root),key_handler_id);
   AutoAppIPtr(PlayerGun);
   pPlayerGun->Off();
   DestroyCommandTerminal();
   DestroyBugTerminal();

   AutoAppIPtr(ShockCamera);
   pShockCamera->SetEditMode(kCameraNormal);
}

#ifndef SHIP
/*
void ShockSetViewRect(char* args)
{
   int x1 = 0;
   int y1 = 0;
   int x2 = 640;
   int y2 = 480;
   sscanf(args,"%d,%d,%d,%d",&x1,&y1,&x2,&y2);
   shock_rend_rect.ul.x = x1;
   shock_rend_rect.ul.y = y1;
   shock_rend_rect.lr.x = x2;
   shock_rend_rect.lr.y = y2;
}
*/

void SendSwitch(char *pObjName)
{
   AutoAppIPtr(ScriptMan);
   AutoAppIPtr(ObjectSystem);

   ObjID objID = pObjectSystem->GetObjectNamed(pObjName);
   if (objID != OBJ_NULL)
   {
      //mprintf("Sending fake TurnOn to obj %d (%s)\n",objID,pObjName);
      sScrMsg msg(objID, "TurnOn");
      pScriptMan->SendMessage(&msg);
   }
}

void TestHUD(int objID)
{
   if (objID != OBJ_NULL)
   {
      gPropHUDTime->Set(objID, GetSimTime() + 5000);
   }
}

#endif

////////////////////////////////////////////////////////////
// PLAYER OBJECT CALLBACKS 
//



static void player_create_CB(ePlayerEvent event, ObjID player)
{
   switch (event)
   {
      case kPlayerCreate:
      {
         char buf[80];
         // @TODO: get rid of this once the player archetype has the 
         // correct model 
         if (config_get_raw("player_model",buf,sizeof(buf)))
         {
            buf[sizeof(buf)-1] = '\0';
            ObjSetModelName(player,buf);
         }

         PhysCreateDefaultPlayer(player);

         // make the player "special"
         cPhysModel *pModel = g_PhysModels.Get(player);
         Assert_(pModel != NULL);
         pModel->SetFlagState(kPMF_Special, TRUE);
         UpdatePhysProperty(player, PHYS_TYPE);

         AutoAppIPtr(NetManager);
         if (pNetManager->IsNetworkGame())
         {
            // Register the player object in the proxy tables. This used
            // to happen at synch time, but now needs to be available
            // before we start processing the player's scripts.
            AutoAppIPtr(ObjectNetworking);
            pObjectNetworking->ObjRegisterProxy(player, player, player);
         }

         PlayerCbllmCreate(); // creates "lower brain" and body
      }
      break;

      case kPlayerLoad:
         PlayerCbllmCreate(); // creates "lower brain" and body
         /*
         {
            // @HACK: get the players connected to each other. This will
            // go away once we have the new joinup major mode.
            AutoAppIPtr(NetManager);
            if (pNetManager->IsNetworkGame()) {
               JoinUp();
            }
         }
         */
         break; 

      case kPlayerDestroy:
         PlayerCbllmDestroy(); 
         break;
   }
}


// Look through the playerfactory links on the level. Ideally, we find
// a link whose data is this player's playernum. If not, we use some other
// link as a default.
static ObjID player_factory_CB(void)
{
   AutoAppIPtr_(LinkManager,pLinkMan);
   cAutoIPtr<IRelation> pRel ( pLinkMan->GetRelationNamed("PlayerFactory") );

   ulong myPlayerNum;
   AutoAppIPtr(NetManager);
   if (pNetManager->IsNetworkGame()) {
      myPlayerNum = pNetManager->MyPlayerNum();
   } else {
      myPlayerNum = 0;
   }
   
   // LinkID id = pRel->GetSingleLink(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD); 
   LinkID defaultFactory = LINKID_NULL;
   LinkID id = LINKID_NULL;
   ILinkQuery *pQuery = pRel->Query(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD);
   if (pQuery == NULL)
      return OBJ_NULL;

   // Run through the PlayerFactory links, and see if any of them work
   for ( ; (!pQuery->Done()) && (id == LINKID_NULL); pQuery->Next()) 
   {
      int *factoryPtr = (int *) pQuery->Data();
      if (factoryPtr == NULL) {
         // It's an old factory with no player num; use it as the
         // default
         defaultFactory = pQuery->ID();
      } else {
         int factoryNum = *factoryPtr;
         if (factoryNum == myPlayerNum) {
            // Got the right one
            id = pQuery->ID();
         } else if (defaultFactory == LINKID_NULL) {
            // We don't have any default yet, so try this one
            defaultFactory = pQuery->ID();
         }
      }
   }

   SafeRelease(pQuery);

   if (id == LINKID_NULL) {
      // We didn't find an appropriate one, so fall back on a default
      if (defaultFactory == LINKID_NULL) {
         // There aren't *any* factories on this level!
         return OBJ_NULL;
      } else {
         id = defaultFactory;
      }
   }

   sLink link; 
   pRel->Get(id,&link); 
   return link.source; 
}

static void setup_player_CB()
{
   HookPlayerCreate(player_create_CB);
   HookPlayerFactory(player_factory_CB); 
}

////////////////////////////////////////////////////////////
// CONTAINS LISTENER 
//

static BOOL contain_CB(eContainsEvent ev, ObjID outer, ObjID inner, eContainType , ContainCBData )
{
   switch(ev)
   {
      case kContainRemove:
         //ObjSetHasRefs(inner,TRUE); 
         break;
      case kContainAdd:
      {
         AutoAppIPtr(ObjectNetworking);
         if (!pObjectNetworking->ObjIsProxy(inner))
         {
            ObjSetHasRefs(inner,FALSE); 
            // @HACK: Need to make sure this dereg gets broadcast, even
            // though the object's now in the container:
            PhysNetForceContainedMsgs(TRUE);
            PhysDeregisterModel(inner);
            PhysNetForceContainedMsgs(FALSE);
            ObjForceReref(inner);
         }
         break; 
      }
   }
   return TRUE; 
}

static void setup_contain_CB(void)
{
   AutoAppIPtr(ContainSys);
   pContainSys->Listen(OBJ_NULL,contain_CB,NULL); 
}

////////////////////////////////////////
// Init culpability
struct sCulpRelations
{
   const char* name; 
   ulong flags; 
}; 

static sCulpRelations culp_rels[] = 
{
   { "~Firer", kCulpTransitive},
   { "CurWeapon", 0 }, 
}; 

#define NUM_CULP_RELS (sizeof(culp_rels)/sizeof(culp_rels[0]))

// Set up culpability relations for dark
static void shock_init_culpability()
{
   // set up culpability listeners
   AutoAppIPtr_(LinkManager,pLinkMan); 
   for (int i = 0; i < NUM_CULP_RELS; i++)
   {
      sCulpRelations& rel = culp_rels[i]; 
      cAutoIPtr<IRelation> pRel = pLinkMan->GetRelationNamed(rel.name); 
      AddCulpabilityRelation(pRel,rel.flags); 
   }
}

////////////////////////////////////////

void ShockPsiPowerLevel(int which)
{
   ShockAmmoPsiLevel(which-1);
}

////////////////////////////////////////
// hm, we should probably save this?
int gQuickbindTable[MAX_BIND];

///////////////////////////////////////////
// Skill related parameters
//

struct sBindTable
{
   ePsiPowers m_table[MAX_BIND];
};

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gBindTableDesc = 
{
   kCampaignVar,         // Where do I get saved?
   "BINDTABLE",          // Tag file tag
   "Bind Table",     // friendly name
   FILEVAR_TYPE(sBindTable),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "shock",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
//cFileVar<sBindTable,&gBindTableDesc> gBindTable; 

// The global machine state
static sBindTable def_state = { 
   kPsiLevel1, kPsiLevel2, kPsiLevel3, kPsiLevel4, kPsiLevel5, kPsiNone,
   kPsiNone, kPsiNone, kPsiNone, kPsiNone, kPsiNone, kPsiNone,
};

class cBindTable : public cFileVar<sBindTable,&gBindTableDesc>
{
   void Reset()
   {
      *(sBindTable*)this = def_state; 
   }
} gBindTable; 

// sdescs
/*
#ifdef EDITOR
static sFieldDesc BindTableFields[] =
{
   { "F1",    kFieldTypeInt,  FieldLocation(sBindTable, m_table[0]),  kFieldFlagNone},
   { "F2",    kFieldTypeInt,  FieldLocation(sBindTable, m_table[1]),  kFieldFlagNone},
   { "F3",    kFieldTypeInt,  FieldLocation(sBindTable, m_table[2]),  kFieldFlagNone},
   { "F4",    kFieldTypeInt,  FieldLocation(sBindTable, m_table[3]),  kFieldFlagNone},
   { "F5",    kFieldTypeInt,  FieldLocation(sBindTable, m_table[4]),  kFieldFlagNone},
   { "F6",    kFieldTypeInt,  FieldLocation(sBindTable, m_table[5]),  kFieldFlagNone},
   { "F7",    kFieldTypeInt,  FieldLocation(sBindTable, m_table[6]),  kFieldFlagNone},
   { "F8",    kFieldTypeInt,  FieldLocation(sBindTable, m_table[7]),  kFieldFlagNone},
   { "F9",    kFieldTypeInt,  FieldLocation(sBindTable, m_table[8]),  kFieldFlagNone},
   { "F10",    kFieldTypeInt,  FieldLocation(sBindTable, m_table[9]),  kFieldFlagNone},
   { "F11",    kFieldTypeInt,  FieldLocation(sBindTable, m_table[10]),  kFieldFlagNone},
   { "F12",    kFieldTypeInt,  FieldLocation(sBindTable, m_table[11]),  kFieldFlagNone},
};

static sStructDesc BindTableDesc = StructDescBuild(sBindTable, kStructFlagNone, BindTableFields);

#endif
*/

////////////////////////////////////////
// finds the "F key" that this psipower is bound to
// or zero for no match.
int ShockBindVal(ePsiPowers power)
{
   int i;
   ePsiPowers match;

   for (i = 0; i < MAX_BIND; i++)
   {
      match = gBindTable.m_table[i];
      if (match == power)
         return(i+1);
   }
   return(0);
}
////////////////////////////////////////
void ShockQuickbind(int which)
{
   ePsiPowers power = kPsiNone;
   if ((which < 1) || (which > MAX_BIND))
   {
      Warning(("ShockQuickbind: which is %d!\n",which));
      return;
   }

   AutoAppIPtr(PlayerPsi);

   if (ShockOverlayCheck(kOverlayPsi))
      power = ShockPsiQuickbind();

   if (power == kPsiNone)
      power = pPlayerPsi->GetSelectedPower();

   gBindTable.m_table[which-1] = power;
}

////////////////////////////////////////

int CountTierPsiPowers(ePsiPowers tierPower)
{
   int count = 0;
   int level = -1;
   int power;
   AutoAppIPtr(ShockPlayer);
   int i;

   for (i=0; (i < NUM_PSI_LEVELS) && (level==-1); i++)
   {
      if (psi_levels[i] == tierPower)
         level = i;
   }
   if ((level<0) || (level>NUM_PSI_LEVELS))
   {
      Warning(("PsiPowerCount: power %d not a tier power\n", tierPower));
      return 0;
   }
   power = psi_levels[level]+1;
   while ((power != psi_levels[level+1]) && (power<kPsiMax))
   {
      if (pShockPlayer->PsiPower(PlayerObject(), (ePsiPowers)power))
         count++;
      power++;
   }
   return count;
}

////////////////////////////////////////

void ShockQuickuse(int which)
{
   if ((which < 1) || (which > MAX_BIND))
   {
      Warning(("ShockQuickuse: which is %d!\n",which));
      return;
   }

   ePsiPowers power;
   power = gBindTable.m_table[which-1];
   if (power == kPsiNone)
      return;

   AutoAppIPtr(ShockPlayer);
   AutoAppIPtr(PlayerPsi);
   if (pShockPlayer->PsiPower(PlayerObject(),(ePsiPowers)power))
   {
      if (pShockPlayer->IsPsiTier(power))
      {
         if (CountTierPsiPowers(power)>0)
         {
            // cycle through
            int level = pShockPlayer->FindPsiLevel(power);
            ShockPsiPowerLevel(level);
         }
      }
      else
      {
         // set it
         pPlayerPsi->Select((ePsiPowers)power);
      }
   }
}
////////////////////////////////////////
void ShockQuickbindInit(void)
{
   /*
   int i;
   for (i=0; i < 5; i++)
      gBindTable.m_table[i] = psi_levels[i];

   for (i=5; i < MAX_BIND; i++)
      gBindTable.m_table[i] = kPsiNone;
   */

}
////////////////////////////////////////
extern void ShockOpenMFD(int which);

static void ShockSummonObj(char *args)
{
   BOOL slim_mode;
   AutoAppIPtr(QuestData);
   slim_mode = pQuestData->Get("HideInterface");

   // don't allow summoning (like nav markers) if we are pre-HUD
   if (slim_mode)
      return;

   AutoAppIPtr(ObjectSystem);

   ObjID arch, newobj;
   ObjPos *pos;
   mxs_vector throwvec = {1.0, 0, 0};

   arch = pObjectSystem->GetObjectNamed(args);
   if (arch == OBJ_NULL)
      return;

   if (!OBJ_IS_CONCRETE(arch))
   {
      newobj = pObjectSystem->BeginCreate(arch,kObjectConcrete);
      pos = ObjPosGet(PlayerObject());
      ObjPosSetLocation(newobj, &pos->loc);
      pObjectSystem->EndCreate(newobj);
      //pGameTools->TeleportObject(newobj, pos->loc.vec, fac);
      PhysSetVelocity(newobj, &throwvec);
   }
}

static void ShockPsiFull(void)
{
   int max;
   AutoAppIPtr(PlayerPsi);
   max = pPlayerPsi->GetMaxPoints();
   pPlayerPsi->SetPoints(max);
}

static void ShockUbermensch(void)
{
   ObjID plr = PlayerObject();
   if (plr == OBJ_NULL)
      return;

   // give the player max stats & skills
   int i;
   sStatsDesc stats;
   for (i=0; i < 5; i++)
      stats.m_stats[i] = 6;
   g_BaseStatsProperty->Set(plr,&stats);

   sWeaponSkills wpnskill;
   for (i=0; i < 4; i++)
      wpnskill.m_wpn[i] = 6;
   g_BaseWeaponProperty->Set(plr,&wpnskill);

   sTechSkills techskill;
   for (i=0; i < 5; i++)
      techskill.m_tech[i] = 6;
   g_BaseTechProperty->Set(plr,&techskill);

   AutoAppIPtr(ShockPlayer);
   pShockPlayer->RecalcData(plr);

   CommandExecute("psi_all");
}

void ShockVersion()
{
   ShockOverlayAddText(AppName(),DEFAULT_MSG_TIME);
}

extern void ShockEditSynchBrushTextures(void);
extern void ShockEditSynchBrushScales(void);
extern void ShockEditSynchBrushFull(void);
extern void ShockEditMakeDecalBrush(char *args);
//extern void ShockElevatorTest(char *args);
extern void ShockLoadFull(const char *args);
extern void ShockGainPool(int amt);
extern void ShockTrainMFD(int amt);
extern void ShockLoadGameAndLevel(char *args);

static Command ShkDebugKeys[] =
{

   //{ "elevator_test", FUNC_STRING, ShockElevatorTest, "Elevator-transport between two levels"},
   { "shk_save_game", FUNC_INT, ShockSaveGame, ""},
   { "shk_load_game", FUNC_INT, TransModeLoadGame, ""},
#ifndef SHIP
   { "shk_load_full", FUNC_STRING, ShockLoadFull, ""},
   //{ "shk_load_level", FUNC_STRING, ShockLoadGameAndLevel, "[shk_load_level <slot> <name>] Loads the slot, and goes into the named level"},

   { "compute_fov", FUNC_VOID, HackFOVCompute, ""},
   { "letterbox", FUNC_BOOL, ShockLetterboxSet, "Set letterbox mode."},
#endif

   { "summon_obj", FUNC_STRING, ShockSummonObj, "Summon an object."},
   { "psi_full", FUNC_VOID, ShockPsiFull, "Give the player full psi points"},
   { "ubermensch", FUNC_VOID, ShockUbermensch, "Turns the player into homo superior."},
   { "add_pool", FUNC_INT, ShockGainPool, "Give player additional build pool points"},
   { "show_version",  FUNC_VOID, ShockVersion, "Display version in game mode." },


   // editing commands
#ifdef EDITOR
   //{ "set_view_rect", FUNC_STRING, ShockSetViewRect, "Set the game view rect"},
   { "send_switch", FUNC_STRING, SendSwitch, "Send a switch message to an object"},
   { "test_HUD", FUNC_INT, TestHUD, "Put HUD brackets around an object." },

   { "camera_pos", FUNC_VOID, CameraPositionSpew, "spew the camera position"},
   { "camera_attach", FUNC_STRING, ShockCameraAttach, "attach the camera position"},
   { "camera_rec", FUNC_VOID, SetCameraEditStateRecord, "Set the camera edit state to record"},
   { "camera_edit", FUNC_VOID, SetCameraEditStateEdit, "Set the camera edit state to edit"},
   { "camera_play", FUNC_VOID, SetCameraEditStatePlayback, "Set the camera edit state to playback"},
   { "set_camera_speed", FUNC_FLOAT, SetCameraEditSpeed, "Set the edit camera speed"},
   { "make_scenes", FUNC_VOID, MakeScenes, "Make all the scene binaries"},
   { "cs_start", FUNC_INT, CutSceneStart, "Start a conversation-based cut-scene (conversationID)" },
   { "cs_rec", FUNC_INT, CutSceneRecord, "Record a new track for a conversation-based cut-scene (conversationID)" },
   { "cs_edit", FUNC_INT, CutSceneEdit, "Edit a conversation-based cut-scene (conversationID)" },
   { "cs_play", FUNC_INT, CutScenePlayback, "Playback a conversation-based cut-scene (conversationID)" },

   { "train_mfd", FUNC_INT, ShockTrainMFD, "Open up a training MFD"},

   { "synch_brush_tx", FUNC_VOID, ShockEditSynchBrushTextures, "Set all textures to current face val"},
   { "synch_brush_scale", FUNC_VOID, ShockEditSynchBrushScales, "Synch all txt scales to current faces val"},
   { "synch_brush_full", FUNC_VOID, ShockEditSynchBrushFull, "Synch textures & scales" },
   { "decal_brush", FUNC_STRING, ShockEditMakeDecalBrush, "[decal_brush x,y] Creates a decal brush off the selected face."},
   // physics update
   { "update_creature_physics", FUNC_VOID, UpdateCreaturesPhysics, "Update special flag on creature physics"},

#endif 

};

extern void DrawHistoryToggle(void);
extern void ShockPDAPlayUnreadLog();

static Command ShkCommands[] =
{
   // hotkey accelerators
   { "toggle_inv", FUNC_VOID, ShockToggleInv, "toggle inv panel" },
   { "cycle_ammo", FUNC_VOID, ShockCycleAmmo, "cycle through available ammo types" },
   { "toggle_compass", FUNC_VOID, ShockToggleCompass, "toggle compass state" },
   { "query", FUNC_INT, ShockQueryMode, "query cursor 1 = on 0 = off" },
   { "split", FUNC_INT, ShockSplitMode, "split cursor 1 = on 0 = off" },
   { "shock_jump_player",FUNC_VOID, ShockPlayerJump, "jump the player" },
   { "look_cursor", FUNC_VOID, ShockLookCursor, "puts cursor into look mode" },
   { "reload_gun", FUNC_VOID, ShockAmmoReload, "reloads weapon out of inventory" },
   { "swap_guns", FUNC_VOID, ShockSwapGuns, "switches primary and secondary weapons" },
   { "wpn_setting_toggle", FUNC_VOID, ShockSettingToggle, "toggles between weapon settings" },
   { "select_psipower", FUNC_VOID, ShockPsiPowerSelect, "brings up the psi power selection MFD" },
   { "equip_weapon", FUNC_STRING, ShockWeaponEquip, "searches your inv for a particular weapon and equips it" },
   { "cycle_weapon", FUNC_INT, ShockWeaponCycle, "Cycles through next equippable weapon, 1 for fwd, -1 for back" },
   { "psi_power", FUNC_INT, ShockPsiPowerLevel, "acts like the 1-5 level buttons" },
   { "open_mfd", FUNC_INT, ShockOpenMFD, "Open up an MFD by overlay constant" },
   { "stop_email", FUNC_VOID, ShockEmailStop, "Stop any currently playing email/log" },
   { "clear_teleport", FUNC_VOID, ShockTeleportClear, "Clear any existing teleport marker."},
   { "quickbind", FUNC_INT, ShockQuickbind, "Bind a quick slot" },
   { "quickuse", FUNC_INT, ShockQuickuse, "Activate a quick slot" },
   { "use_obj", FUNC_STRING, ShockUseObj, "Use an object by name" },
   { "msg_history", FUNC_VOID, DrawHistoryToggle, "Toggle message history" },
   { "play_unread_log", FUNC_VOID, ShockPDAPlayUnreadLog, "Play an unread log" },

   // basic interface support
   { "toggle_mouse", FUNC_VOID, ShockToggleMode, "switch between mouselook & cursor modes" },
   { "frob_toggle", FUNC_VOID, ShockFrobAndMaybeToggleMode, "switch between modes, also frobbing current selection" },
   { "frob_object", FUNC_VOID, ShockFrobObject, "simple frob of selected object" },
   { "frob_object_inv", FUNC_VOID, ShockFrobObjectInv, "simple frob of selected object, in world or in inv" },
   { "mp_hilight_obj", FUNC_VOID, ShockHilightObject, "hilight an object in MP game" },
   //{ "interface_click", FUNC_VOID, ShockInterfaceClick, "interact with objects / interface" },
   { "interface_use", FUNC_VOID, ShockInterfaceUseItem, "inv use an item"},
   { "fire_weapon", FUNC_INT, ShockFireWeapon, "fire weapon 1 = start 0 = finish" },
   { "drag_and_drop", FUNC_INT, ShockDragDropSimple, "drag & drop 1 = start 0 = finish" },
   { "drag_and_drop_frob", FUNC_INT, ShockDragDropFrob, "drag & drop 1 = start 0 = finish" },
   { "drag_and_drop_mode", FUNC_INT, ShockDragDropMode, "drag & drop 1 = start 0 = finish" },
   //{ "dragndrop", FUNC_VOID, ShockBeginDragDrop, "begin drag & drop operation" },
   //{ "end_dragndrop", FUNC_VOID, ShockBeginDragDrop, "begin drag & drop operation" },

   //{ "fire_at_cursor", FUNC_INT, ShockFireCursor, "fire weapon at cursor, 1=start 0=finish"},
   { "test_begin", FUNC_VOID, ShockBeginGame, ""},
   { "quicksave", FUNC_VOID, ShockQuickSaveHack, "Save to 'current' subdir." },
   { "quickload", FUNC_VOID, ShockQuickLoad, "Load from 'current' subdir." },
};

void shock_init_game(void)
{
   COMMANDS(ShkCommands,HK_ALL); // GAME_MODE|HK_GAME2_MODE);
   // note, debug keys are included in ship build as cheat keys
   COMMANDS(ShkDebugKeys,HK_ALL);

   // Install damage listener/filters
   IDamageModel* pDamageModel = AppGetObj(IDamageModel);
   pDamageModel->Listen(kDamageMsgImpact|kDamageMsgSlay|kDamageMsgDamage,ShockDamageListener,NULL);
   pDamageModel->Listen(kDamageMsgTerminate,ShockLootDamageListener,NULL);
   SafeRelease(pDamageModel);

   ShockReactionsInit();

   ShockUtilsInit();

   GunInit();
   setup_player_CB(); 
   setup_contain_CB(); 

   // man, we should clean this up some
   AutoAppIPtr(PlayerGun);
   pPlayerGun->GameInit();
   ShockPropertiesInit();
   CannisterPropertyInit();
   InvDimsPropertyInit();
   DoorOpenSoundPropertyInit();
   DoorCloseSoundPropertyInit();
   DoorTimerPropertyInit();
   ShockFrobsysInit();
   ObjSoundNamePropInit();
   RepContentsPropertyInit();
   ShockCreaturesInit();
   ShockMultiLevelInit();
   ShockDamageInit();
   ShockKeyInit();
   ShockFrobPropertiesInit();
   ShockPlayerDamageInit();
   ShockAIInit();
   ShockLinksInit();
   MeleeTypePropertyInit();
   PsiInit();
   ShockParamEditInit();
   DarkDifficultyInit(); 
   ShockMenusInit(); 
   ShockDebriefPanelInit();
   ShockSoundInit();
   CameraObjPropertyInit();
   ProjectileTargetInit();
   ShockMeleeInit();
   GunProjectileInit();
   ShockExpInit();
   ShockPDANetInit();
   ShockSourceRandomizeInit();
   ShockBloodInit();
   ShockSaveUIInit(); 

   ShockQuickbindInit();

   ShockInvListenInit();
   ShockOverlayListenInit();   

   SetGameIBVarsFunc (InitShockIBVars);

   ShockOptionsMenuInit(); 
   ShockMasterModeInit(); 
   ShockReportInit();

   shock_init_culpability();

   // Network messages for the game level:
   g_pThrowObjMsg = new cNetMsg(&sThrowObjDesc);

   // stolen from dark
   BreathSimInit();

   // Choose game mode 
#ifdef EDITOR
   BOOL start_game = config_is_defined("play"); 
#else
   BOOL start_game = TRUE; 
#endif

   // hack to set initial difficulty level
   int diffval = 0;
   config_get_int("difficulty",&diffval);
   AutoAppIPtr(QuestData);
   pQuestData->Create("Difficulty",diffval,kQuestDataCampaign);
 
   if (start_game)
      gPrimordialMode = DescribeShockMasterMode(); 
   if (config_is_defined("quick_start"))
      gPrimordialMode = DescribeShockInitGameMode(); 

   

}

void shock_term_game(void)
{
   ShockDamageShutDown();
   ShockCreaturesTerm();
   ShockPlayerDamageTerm();
   AutoAppIPtr(PlayerGun);
   pPlayerGun->GameTerm();
   ShockLinksTerm();
   g_pMeleeTypeProperty->Release();
   PsiTerm();
   ShockUtilsTerm();
   DarkDifficultyTerm(); 
   ShockMenusTerm(); 
   ShockDebriefPanelTerm();
   ShockSoundTerm();
   CameraObjPropertyTerm();
   ShockMultiLevelTerm();
   ShockOptionsMenuTerm(); 
   ShockKeyTerm();
   ProjectileTargetTerm();
   ShockMeleeTerm();
   ShockMasterModeTerm(); 
   GunProjectileTerm();
   ShockExpTerm();
   ShockPDANetTerm();
   ShockFrobsysTerm();
   ShockReportTerm();
   ShockSourceRandomizeTerm();
   ShockBloodTerm();
   ShockSaveUITerm(); 

   // Delete network message:
   delete g_pThrowObjMsg;
}
