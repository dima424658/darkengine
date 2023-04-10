/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Deep Cover specific game features

#include <string.h>
#include <math.h>

// LG Tech
#include <2d.h>
#include <rect.h>
#include <res.h>
#include <types.h>
#include <resapi.h>
#include <config.h>
#include <mprintf.h>
#include <matrix.h>
#include <gadbox.h>

// ActReact enhancments.
#include <dpcreact.h>
#include <dpcdrprp.h>
#include <dpcprop.h>

// AI
#include <dpcai.h>

// Combat:  Melee
#include <dpcmelee.h>
#include <dpcplcst.h>
#include <plycbllm.h>
#include <dpcmelpr.h>
#include <weapon.h>

// Damage
#include <dmgmodel.h>
#include <dmgbase.h>
#include <dpcdmg.h>
#include <dpcdmprp.h>

// File support
#include <filevar.h>

// Framework
#include <appname.h>
#include <command.h>
#include <gamestr.h>
#include <gametool.h>

// Input
#include <kbcook.h>
#include <kb.h>
#include <keydefs.h>
#include <mouse.h>
#include <dpckey.h>
#include <gen_bind.h>
#include <dpc_bind.h>
#include <dpcqbind.h>
#include <dpcctrl.h>

// Editor
#include <editobj.h>
#include <editor.h>

// Engine features
#include <contain.h>
#include <culpable.h>
#include <frobctrl.h>
#include <frobprop.h>
#include <pick.h>
#include <questapi.h>
#include <dpcfrob.h>
#include <dpcfsys.h>
#include <weapprop.h>
#include <gunvhot.h>

// Metagame stuff
#include <dpcmain.h>
#include <dpcmenu.h>
#include <init.h>

// Player
#include <player.h>
#include <playrobj.h>
#include <plyrmode.h>
#include <dpclooko.h>
#include <dpcparam.h>
#include <dpcpldmg.h>
#include <dpcplayr.h>
#include <dpcplprp.h>

// Loops
#include <loopapi.h>
#include <scrnloop.h>

// Motion
#include <headmove.h>
#include <dpccret.h>

// Multiplayer
#include <netsynch.h>
#include <netmsg.h>

// Objects
#include <objsys.h>
#include <osysbase.h>
#include <objquery.h>
#include <objpos.h>
#include <objhp.h>
#include <objtype.h>
#include <lnkquery.h>
#include <linkman.h>
#include <linkbase.h>
#include <relation.h>
#include <traitman.h>
#include <dpclinks.h>
#include <dpcobjst.h>
#include <objlpars.h>

// Physics
#include <physapi.h>
#include <phcore.h>
#include <phmod.h>
#include <phmods.h>
#include <phnet.h>
#include <phprop.h>
#include <physcast.h>

// Random numbers.
#include <r3d.h>
#include <rand.h>

// Render:  Camera
#include <camera.h>
#include <dpccam.h>

// Render:  Lights
#include <objlight.h>

// Render:  Particles
#include <particle.h>
#include <partprop.h>

// Render:  General
#include <rendobj.h>
#include <rendprop.h>
#include <mnamprop.h>
#include <dpccmapi.h>
#include <dpcrend.h> // for pick reset
#include <dpccobpr.h>

// Scripting
#include <scrnovls.h>
#include <scrnmode.h>
#include <anim_txt.h>
#include <creature.h>
#include <scrptapi.h>
#include <scrptmsg.h>
#include <dpcscapi.h>

// Simulation
#include <iobjnet.h>
#include <simbase.h>
#include <simtime.h>
#include <simstate.h>
#include <drkbreth.h>

// Sound
#include <appsfx.h>
#include <schema.h>
#include <dpcsndpr.h>
#include <dpcsound.h>

// Time
#include <fixtime.h>

// UI
#include <cmdterm.h>
#include <hud.h>
#include <dpccurm.h>        // For SCM modes
#include <dpcdlg.h>
#include <dpchud.h>
#include <dpcinv.h>
#include <dpcinvpr.h>
#include <dpciface.h>
#include <dpccurpr.h>
#include <dpclbox.h>
#include <dpcoptmn.h>
#include <dpclding.h>
#include <dpcmulti.h>
#include <dpcsavui.h>
#include <dpcovrly.h>
#include <dpcovcst.h>
#include <dpcemail.h>
#include <dpcpda.h>

// Weapons
#include <prjctile.h>
#include <gunproj.h>
#include <dpcammov.h>
#include <gunapi.h>
#include <gunprop.h>
#include <dpcpgapi.h>

// Dark tools & other loot
#include <drkdiff.h>
#include <drkbreth.h>

// Debugging
#include <bugterm.h>
#include <cfgdbg.h>

extern "C"
{
    #include <event.h>
}

// Other Deep Cover files
#include <dpchrm.h>
#include <dpcgame.h>
#include <dpcutils.h>
#include <dpcblood.h>
#include <dpcloot.h>
#include <dpcreprt.h>

#include <drkuires.h>
#include <globalid.h>
#include <transmod.h>
#include <dpcincst.h>

#ifndef SHIP
    #include <dpccmapi.h>
    #include <dpccs.h>
    #include <viewmgr.h>
#endif

#include <dbmem.h>

#define USE_DIST  100.0F

// is the user typing a message
static bool typingMode=FALSE;

static Point gLastpos = {-1,-1};

bool DPC_mouse = FALSE;
EXTERN bool g_lgd3d;
IRes *DPC_mouse_hnd;
IRes *gDefaultHnd;
int DPC_cursor_mode = SCM_NORMAL;

extern Rect gun_rect;
static Rect power_rect = {{276,254}, {447,390}};
static Rect subpower_rect = {{456,254}, {629,390}};
ObjID drag_obj = OBJ_NULL; // what is ON the cursor
bool DPC_mouse_loaded = FALSE; // have we loaded in cursor art yet?

// controllers in dark for focus/actions
BOOL active_focus=FALSE;
BOOL world_use_in_progress=FALSE;

#ifndef SHIP
void CameraPositionSpew(void);
#endif

void DPCDrawCursor(Point pt)
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
    AutoAppIPtr(DPCCamera);
    if (pDPCCamera->GetEditMode() != kCameraNormal)
        return;
#endif

    strcpy(temp,"");

    AutoAppIPtr(ContainSys);
    ObjID cont;

    switch (DPC_cursor_mode)
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

    w = gr_font_string_width(gDPCFont, temp);
    bmh = 16; // rather than compute
    dx = pt.x - (w / 2);

    sScrnMode smode;
    ScrnModeGet(&smode);
    if (dx + w + 10 > smode.w)
        dx = smode.w - w - 10;
    if (dx < 10)
        dx = 10;
    dy = pt.y + (bmh / 2) + 2;

    gr_set_fcolor(gDPCTextColor);
    gr_font_string(gDPCFontMono, temp, dx, dy);
}

static void doMouseFocusCheck(void)
{
    Point mpt;
    short mx, my;
    BOOL occlude;
    mouse_get_xy(&mx,&my);
    mpt.x = mx;  mpt.y = my;
    occlude = DPCOverlayMouseOcclude(mpt);

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
                DPCInterfaceMouseOver(g_PickCurrentObj);
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
    if (!mode && DPC_mouse && (drag_obj != OBJ_NULL))
        return(FALSE);

    DPC_mouse = mode;
    DPCOverlayMouseMode(mode);

    if (mode)
    {
        RemoveIBHandler();
        InstallIBHandler (HK_GAME2_MODE, UI_EVENT_KBD_RAW | UI_EVENT_MOUSE | UI_EVENT_MOUSE_MOVE | UI_EVENT_JOY, FALSE);

        //dont let input binding sys process mouse move events
        headmoveSetRelPosX(0);
        headmoveSetRelPosY(0);
        g_pInputBinder->SetValidEvents (ALL_EVENTS & (~UI_EVENT_MOUSE_MOVE)); //  & (~UI_EVENT_MOUSE));

        if (!DPC_mouse_loaded)
        {
            gDefaultHnd = DPC_mouse_hnd = LoadPCX("cursor"); // , INTERFACE_PATH, DPCLoadNoPalette);
            DPC_mouse_loaded = TRUE;
            // if ()   ??
            SetCursorByHandle(DPC_mouse_hnd);
        }
        if (gLastpos.x != -1)
            mouse_put_xy(gLastpos.x,gLastpos.y);
        if (clear)
            ClearCursor();
    }
    else
    {
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
        if (DPC_mouse_loaded)
        {
            SafeFreeHnd(&DPC_mouse_hnd);
            DPC_mouse_loaded = FALSE;
        }

    }

    // make sure nothing gets stuck down
    //g_pInputBinder->PollAllKeys();

    return(TRUE);
}

void DPC_check_keys(void)
{
    //static bool last_ctrl = FALSE;

    /*
    int kb_mods=hack_for_kbd_state();
    //bool old_mouse = DPC_mouse;
    bool ctrl_down = (kb_mods & CTRL_STATES);
    static bool ctrl_used = FALSE;
    if (ctrl_down) 
    {
       if (DPC_cursor_mode == SCM_NORMAL)
       {
          DPCLookCursor();
          ctrl_used = TRUE;
       }
    }
    else 
    {
       if (DPC_cursor_mode == SCM_LOOK)
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
bool DPC_key_parse(int keycode)
{
    int kc;

    // MFDs and other overlaid interface elements
    if (keycode&KB_FLAG_DOWN)
        if (DPCOverlayHandleKey(keycode))
            return(TRUE);

        // camera recording mode
    AutoAppIPtr(DPCCamera);
    if (pDPCCamera->GetEditMode() == kCameraRecord)
        return DPCControlKeyParse(keycode, pDPCCamera->GetAttachObject(), pDPCCamera->GetCameraSpeed());

    if (pDPCCamera->GetEditMode() == kCameraEdit)
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
            pDPCCamera->SwitchCamera(kc-'1');
            return TRUE;
        case '0':
            pDPCCamera->SwitchCamera(10);
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
    if (pNetManager->IsNetworkGame())
    {
        AutoAppIPtr(ObjectNetworking);
        pObjectNetworking->ObjTakeOver(obj);
    }

    // Try to deal with throwing out a melee weapon that is still
    // animating its close
    AutoAppIPtr(DPCPlayer);
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
    {
        DPCInvLoadCursor(OBJ_NULL);
    }
}

//////////

// player is interacting with the interface / frobbing
void DPCInterfaceUseItem()
{
    // don't allow anything if player dead
    if (GetPlayerMode() == kPM_Dead)
        return;

    if (DPC_cursor_mode == SCM_LOOK)
    {
        ClearCursor();
        return;
    }

    Point pos;
    mouse_get_xy(&pos.x, &pos.y);

    // kind of a misnomer now, since doubleclicks are gone
    DPCOverlayDoubleClick(pos);

    DPCInvRefresh();
}

// player wants to change mode with no other effects
void DPCToggleMode()
{
    // don't allow anything if player dead
    if (GetPlayerMode() == kPM_Dead)
        return;
    MouseMode(!DPC_mouse,TRUE);
}

// toggle mode, first frobbing what is under the cursor
void DPCFrobObject() 
{
    // don't allow anything if player dead
    if (GetPlayerMode() == kPM_Dead)
        return;
    DPCDoFrob(FALSE);
}

void DPCFrobObjectInv() 
{
    // don't allow anything if player dead
    if (GetPlayerMode() == kPM_Dead)
        return;
    // if we have a world object, frob it
    if (frobWorldSelectObj != OBJ_NULL)
    {
        DPCDoFrob(FALSE);
    }
    // otherwise, inv-frob 
    else
    {
        DPCInterfaceUseItem();
    }
}

void DPCFrobAndMaybeToggleMode() // BOOL start
{
    // don't allow anything if player dead
    if (GetPlayerMode() == kPM_Dead)
        return;
    // only do stuff on the downs
    //if (!start)
    //   return;

    if (frobWorldSelectObj == OBJ_NULL)
    {
        if (DPC_cursor_mode == SCM_NORMAL)
            MouseMode(!DPC_mouse, TRUE);
    }
    else
    {
        DPCDoFrob(FALSE);
        BOOL switchmode = FALSE;

        if (DPC_cursor_mode == SCM_DRAGOBJ)
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
            MouseMode(!DPC_mouse, TRUE);
        }
    }
}

void DPCHilightObject()
{
    AutoAppIPtr(NetManager);
    if (!pNetManager->Networking())
        // This feature only really makes sense in multiplayer
        return;

    if ((frobWorldSelectObj != OBJ_NULL) && ObjHasRefs(frobWorldSelectObj))
    {
        DPCBroadcastHilightObject(frobWorldSelectObj);
    }
}

//////////
//
// mouse parsing

BOOL DPCInterfaceClick()
{
    // Don't allow anything if player dead
    if (GetPlayerMode() == kPM_Dead)
    {
        return(TRUE);
    }

    BOOL retval = FALSE;
    Point pos;
    mouse_get_xy(&pos.x, &pos.y);

    if (DPCOverlayClick(pos)) // interface hacks
    {
        return(TRUE);
    }

    switch (DPC_cursor_mode)
    {
    case SCM_DRAGOBJ:
        {
            BOOL throw_me = FALSE;
            if (frobWorldSelectObj == OBJ_NULL)
            {
                throw_me = TRUE;
            }
            else
            {
                frobInvSelectObj = frobWorldSelectObj; // set the target of the tool frob
                DPCDoFrob(FALSE);
                //ClearCursor();
                if (DPCScriptAllowSwap())
                {
                    throw_me = TRUE;
                }
            }
            if (throw_me && (drag_obj != OBJ_NULL))
            {
                // throw into world
                ThrowObj(drag_obj,PlayerObject());
                retval = true;
            }
            break;
        }

    case SCM_LOOK:
        {
            //retval = TRUE;
            /*
            if (frobWorldSelectObj != OBJ_NULL)
            {
               DPCLookPopup(frobWorldSelectObj);
               retval = TRUE;
            }
            */
            //ClearCursor();
            break;
        }
    }
    return(retval);
}

// fire begin / stop
static void DPCFireWeapon(BOOL start)
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
    AutoAppIPtr(QuestData);
    BOOL slim_mode = pQuestData->Get("HideInterface");
    if (slim_mode)
    {
        return;
    }

    AutoAppIPtr(PlayerGun);
    AutoAppIPtr(DPCPlayer);

    if (start)
    {
        if (pPlayerGun->Get() != OBJ_NULL)
        {
            pPlayerGun->PullTrigger();
        }
        StartMeleeAttack(PlayerObject(), GetWeaponObjID(PlayerObject()));
    }
    else
    {
        if (pPlayerGun->Get() != OBJ_NULL)
        {
            pPlayerGun->ReleaseTrigger();
        }
        FinishMeleeAction(PlayerObject(), GetWeaponObjID(PlayerObject()));
    }
}

// query cursor
static void DPCQueryMode(BOOL start)
{
    // don't allow anything if player dead
    if (GetPlayerMode() == kPM_Dead)
        return;
    if (start)
    {
        if (DPC_cursor_mode == SCM_NORMAL)
            DPCLookCursor();
    }
    else
    {
        if (DPC_cursor_mode == SCM_LOOK)
            ClearCursor();
    }
}

static void DPCSplitMode(BOOL start)
{
    // don't allow anything if player dead
    if (GetPlayerMode() == kPM_Dead)
        return;
    if (start)
    {
        if (DPC_cursor_mode == SCM_NORMAL)
        {
            DPCSplitCursor();
        }
    }
    else
    {
        if (DPC_cursor_mode == SCM_SPLIT)
        {
            ClearCursor();
        }
    }
}

// drag & drop
#define DDNONE 0
#define DDFROB 1
#define DDMODE 2

static void DPCDragDropCore(BOOL start, int func)
{
    // don't allow anything if player dead
    if (GetPlayerMode() == kPM_Dead)
        return;
    Point pos;
    mouse_get_xy(&pos.x, &pos.y);

    if (start)
    {
        if (DPCOverlayDragDrop(pos, start))
            return;
    }
    else
    {
        int oldmode = DPC_cursor_mode;
        DPCInterfaceClick();
        if (DPCOverlayDragDrop(pos, start))
            return;

        if ((DPC_cursor_mode != SCM_NORMAL) || (oldmode != SCM_NORMAL))
            return;

        switch (func)
        {
        case DDFROB:
            DPCDoFrob(FALSE);
            break;
        case DDMODE:
            MouseMode(!DPC_mouse,TRUE);
            break;
        }
    }
}

static void DPCDragDropFrob(BOOL start)
{
    DPCDragDropCore(start, DDFROB);
}

static void DPCDragDropSimple(BOOL start)
{
    DPCDragDropCore(start, DDNONE);
}

static void DPCDragDropMode(BOOL start)
{
    DPCDragDropCore(start, DDMODE);
}

static void DPCToggleInv(void)
{
    DPCOverlayChange(kOverlayInv, kOverlayModeToggle);
}

static void DPCToggleCompass(void)
{
    AutoAppIPtr(DPCPlayer);
    AutoAppIPtr(ScriptMan);
    ObjID compass = pDPCPlayer->GetEquip(PlayerObject(),kEquipCompass);
    if (compass == OBJ_NULL)
    {
        return;
    }

    // don't allow the compass to be turned on until
    // we have the interface "installed"
    BOOL slim_mode;
    AutoAppIPtr(QuestData);
    slim_mode = pQuestData->Get("HideInterface");
    if (slim_mode)
    {
        return;
    }

    sScrMsg msg(compass,"Toggle"); 
    pScriptMan->SendMessage(&msg); 
}

static void DPCCycleAmmo(void)
{
    DPCAmmoChangeTypes();
}

static void DPCSettingToggle(ObjID gun)
{
    int wpntype;
    //AutoAppIPtr(PlayerGun);
    AutoAppIPtr(DPCPlayer);
    AutoAppIPtr(TraitManager);
    AutoAppIPtr(ObjectSystem);

    //ObjID gun = pPlayerGun->Get();
    if (gun == OBJ_NULL)
    {
        return;
    }

    g_pWeaponTypeProperty->Get(gun,&wpntype);

    // on a melee weapon, do nothing at all
    if (g_pMeleeTypeProperty->IsRelevant(gun))
    {

        AutoAppIPtr(ContainSys);
        // First unapply a meta prop if we have an applied one
        const char *metaName = GetWeaponModeUnchangeMetaProp(gun);
        if (metaName) 
        {
           ObjID metaProp = pObjectSystem->GetObjectNamed(metaName);
           if (metaProp) 
           {
              pTraitManager->RemoveObjMetaProperty(gun, metaProp);

              int newloc = pContainSys->IsHeld(PlayerObject(),gun);
              pDPCPlayer->Equip(PlayerObject(), kEquipWeapon, gun, FALSE);
              pContainSys->Add(PlayerObject(), gun, newloc, CTF_NONE);

              //pDPCPlayer->Equip(PlayerObject(), kEquipWeapon, gun, FALSE);
           }
        }
        else 
        {
           const char *metaName = GetWeaponModeChangeMetaProp(gun);
           if (metaName) 
           {
              ObjID metaProp = pObjectSystem->GetObjectNamed(metaName);
              if (metaProp) 
              {
                 pTraitManager->AddObjMetaProperty(gun, metaProp);

                 int newloc = pContainSys->IsHeld(PlayerObject(),gun);
                 pDPCPlayer->Equip(PlayerObject(), kEquipWeapon, gun, FALSE);
                 pContainSys->Add(PlayerObject(), gun, newloc, CTF_NONE);

                 //pDPCPlayer->Equip(PlayerObject(), kEquipWeapon, gun, FALSE);
              }
           }
        }
        return;
    }

    int curmode = GunGetSetting(gun);
    int numModes = BaseGunDescGetNumSettings(gun);
    if (numModes > 1) {
       curmode = (curmode+1) % numModes; 
       GunSetSetting(gun,curmode);
    
       float zoom = BaseGunDescGetZoom(gun);
       ZoomTarget(zoom, 2.0);

       // text feedback
       char temp[255], fmt[255];
       extern char *setting_headlines[2]; // hack
   
       AutoAppIPtr(GameStrings);
       cStr str = pGameStrings->FetchObjString(gun, setting_headlines[!curmode]);
       DPCStringFetch(fmt,sizeof(fmt), "SettingChange", "misc");
       sprintf(temp,fmt,str);
       DPCOverlayAddText(temp,DEFAULT_MSG_TIME);
   }
}

#define MAX_EQUIP_CANDIDATES  64

static int equip_val(ObjID o)
{
    // basically, return the condition
    int retval = 0 - (GunGetCondition(o) * 100);

    // out of ammo is bad
    if (GunStateGetAmmo(o) == 0)
    {
        retval = retval + 10000;
    }

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
    return(equip_val(*b) < equip_val(*a));
}

static void DPCUseObj(char *args)
{
    AutoAppIPtr(ContainSys);
    AutoAppIPtr(ObjectSystem);
    AutoAppIPtr(TraitManager);

    // no hotkey usage if dead
    if (GetPlayerMode() == kPM_Dead)
    {
        return;
    }

    // no hotkey usage if no interface
    AutoAppIPtr(QuestData);
    BOOL slim_mode = pQuestData->Get("HideInterface");
    if (slim_mode)
    {
        return;
    }

    ObjID invobj;
    ObjID arch = pObjectSystem->GetObjectNamed(args);

    sContainIter *iterp = pContainSys->IterStart(PlayerObject());
    while (!iterp->finished)
    {
        invobj = iterp->containee;

        // Use it as if double clicked in inventory
        if (pTraitManager->ObjHasDonor(invobj, arch))
        {
            frobInvSelectObj = invobj;
            DPCDoFrob(TRUE);
            pContainSys->IterEnd(iterp);

            DPCInvRefresh();
            return;
        }
        pContainSys->IterNext(iterp);
    }
    pContainSys->IterEnd(iterp);

    // give feedback
    char temp[255];
    char fmt[255];
    DPCStringFetch(fmt, sizeof(fmt), "NotInInventory", "misc");
    AutoAppIPtr(GameStrings);
    cStr str = pGameStrings->FetchObjString(arch, PROP_OBJSHORTNAME_NAME);
    sprintf(temp, fmt, str);
    DPCOverlayAddText(temp,DEFAULT_MSG_TIME);
}

static BOOL DPCWeaponEquipCore(char *args, BOOL spew)
{
    // search the player's inventory for a matching weapon
    // args can now be NULL meaning unequip only
    ObjID        invobj;
    ObjID        candidates[MAX_EQUIP_CANDIDATES];
    eContainType newloc;
    int          count = 0;
    int          i;
    BOOL         addme;

    AutoAppIPtr(PlayerGun);
    AutoAppIPtr(ContainSys);
    AutoAppIPtr(ObjectSystem);
    AutoAppIPtr(TraitManager);
    AutoAppIPtr(DPCPlayer);
    AutoAppIPtr(QuestData);

    // dont switch if no interface
    BOOL slim_mode = pQuestData->Get("HideInterface");
    if (slim_mode)
    {
        return(FALSE);
    }

    // don't switch if we are dead
    if (GetPlayerMode() == kPM_Dead)
    {
        return (FALSE);
    }

    // don't do jack if we are in the middle of reloading
    if (pPlayerGun->IsReloading())
    {
        return (FALSE);
    }

    // dont let you switch weapons mid firing
    ObjID gun = pPlayerGun->Get();
    if ((gun != OBJ_NULL) && pPlayerGun->IsTriggerPulled())
    {
        return(FALSE);
    }
    
    ObjID playerobj = PlayerObject();
    ObjID newobj    = OBJ_NULL;
    ObjID prevobj   = pDPCPlayer->GetEquip(playerobj, kEquipWeapon);

    if (!args) {
       if (prevobj) {
          // Just unequipping, thanks
          if (IsMelee(prevobj)) {
             pDPCPlayer->Equip(playerobj, kEquipWeapon, OBJ_NULL,spew);
             DPCInvAddObj(playerobj, prevobj, 1);
          } else {
             ZoomTarget(1.0, 0.0);

             // Lower gun then unequip in player gun code
             pPlayerGun->Unequip(prevobj, OBJ_NULL, spew, TRUE);
          }
          return TRUE;
       }
       else
       {
          return FALSE;
       }
    }
       
    ObjID weaparch = pObjectSystem->GetObjectNamed(args);
    // bail out if the request object doesn't exist
    if (weaparch == OBJ_NULL)
        return(FALSE);
   
    for (i=0; i < MAX_EQUIP_CANDIDATES; i++)
    {
        candidates[i] = OBJ_NULL;
    }

    BOOL currentIsThisKind = FALSE;
    sContainIter *iterp = pContainSys->IterStart(playerobj);
    while (args && !iterp->finished)
    {
        invobj = iterp->containee;
        addme = FALSE;

        if (pTraitManager->ObjHasDonor(invobj, weaparch))
        {
            if (invobj != prevobj)
            {
                // equip it, maybe
                addme = TRUE;
            } 
            else 
            {
                currentIsThisKind = TRUE;
            }
        }

        if (addme)
        {
            eObjState st = ObjGetObjState(invobj);
            if (st == kObjStateNormal)
            {
                candidates[count] = invobj;
                count++;
                // abort out if somehow we have more objects than we can contemplate
                if (count == MAX_EQUIP_CANDIDATES)
                {
                    break;
                }
            }
        }
        pContainSys->IterNext(iterp);
    }
    pContainSys->IterEnd(iterp);

    // if we have no canditates, punt
    if (count == 0 && args)
    {
        if (currentIsThisKind) {
           // User hit '2' when a weapon '2' is equipped (for instance)
           // which means -- toggle the setting!
           DPCSettingToggle(prevobj);
        } else {
           return(FALSE);
        }
    }

    // sort the candidates by quality
    qsort(candidates, count, sizeof(candidates[0]), equip_compare);

    // if we have no previous considerations, take the first (best)
    if ((prevobj == OBJ_NULL) || (!pTraitManager->ObjHasDonor(prevobj, weaparch)))
    {
        newobj = candidates[0];
    }
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
        {
            newobj = candidates[0];
        }
    }


    // okay, do the swap
    if (newobj != OBJ_NULL) 
    {
       if (pDPCPlayer->CheckRequirements(newobj, spew)) 
       {
          if (prevobj) 
          {
             if (IsMelee(prevobj)) 
             {
                // first unequip the old thing
                newloc = pContainSys->IsHeld(playerobj,newobj);
                pDPCPlayer->Equip(playerobj, kEquipWeapon, newobj, spew);
                pContainSys->Add(playerobj, prevobj, newloc, CTF_NONE);
             } 
             else 
             {
                // Clear any zoom
                ZoomTarget(1.0, 0.0);

                // Lower gun then unequip in player gun code
                pPlayerGun->Unequip(prevobj, newobj, spew, TRUE);
             }
          } 
          else 
          {
              pDPCPlayer->Equip(playerobj, kEquipWeapon, newobj, spew);
          }
          return TRUE;
       }
    }

    return(FALSE);
}

static void DPCWeaponEquip(char *args)
{
    DPCWeaponEquipCore(args, TRUE);
}

static void DPCWeaponUnequip(void)
{
    DPCWeaponEquipCore(NULL, TRUE);
}

static void DPCWeaponUnjam(void) 
{
    AutoAppIPtr(DPCPlayer);

    ObjID playerobj = PlayerObject();
    ObjID gunobj   = pDPCPlayer->GetEquip(playerobj, kEquipWeapon);

    int state = ObjGetObjState(gunobj);
    if (state == kObjStateBroken) {
        int skillval = pDPCPlayer->GetTechSkill(kTechRepair);
        // skillval goes 0 to ...6?...
        if (Rand() % 6 < skillval) {
           // success
           ObjSetObjState(gunobj, kObjStateNormal);
           SchemaPlay((Label *)"unjam",NULL);         
        } else {
           // failure
           SchemaPlay((Label *)"unjam_fail",NULL);         
        }
    }
}

static char *weapon_order[] = 
{
    "psi amp",       "wrench",        "pistol",        "shotgun",       "assault rifle",
    "laser pistol",  "EMP Rifle",     "Electro Shock", "Gren Launcher", "Stasis Field Generator",
    "Fusion Cannon", "Crystal Shard", "Viral Prolif",  "Worm Launcher",
};

#define WEAPON_CYCLE (sizeof(weapon_order) / sizeof(char *))
static void DPCWeaponCycle(int dir)
{
    ObjID weaparch;
    char archname[255];
    int i, count;
    int num = 0;

    AutoAppIPtr(DPCPlayer);
    AutoAppIPtr(TraitManager);
    AutoAppIPtr(ObjectSystem);

    // find the currently equipped weapon
    ObjID current = pDPCPlayer->GetEquip(PlayerObject(), kEquipWeapon);
    if (current == OBJ_NULL)
    {
        num = 0;
    }
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
        {
            num = 0;
        }
        if (num < 0)
        {
            num = WEAPON_CYCLE - 1;
        }
        if (DPCWeaponEquipCore(weapon_order[num], FALSE))
        {
            count = WEAPON_CYCLE;
        }
        num = num + dir;
    }
}

static void HackFOVCompute(void)
{
    mxs_vector vecs[4];
    mxs_matrix m1, m2;
    mxs_angvec av1, av2;
    r3_get_view_pyr_vecs(vecs);

    mxs_vector *v1 = &vecs[0];
    mxs_vector *v2 = &vecs[2];

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
void DPC_sim_update_frame(int ms)
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
    if (!DPC_mouse)
    {
        // set focus way out for non-cursor mode
        //PickSetFocus(fix_make(mpt.x, 0), fix_make(mpt.y, 0), 10000); 

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

    PickFrameUpdate();
    DPC_pick_reset();

    PlayerCbllmUpdate(ms);
    BreathSimUpdateFrame(ms); 
}

// post render frame updates
void DPC_rend_update_frame(void)
{
//   update_player_medium_sounds();
}

#pragma off(unreferenced)
static bool key_handler_func(uiEvent* ev, Region* r, void* data)
{
    uiCookedKeyEvent* kev = (uiCookedKeyEvent*)ev;
    return DPC_key_parse(kev->code);
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

void DPC_start_gamemode(void)
{
    LGadRoot* root = LGadCurrentRoot();
    uiInstallRegionHandler(LGadBoxRegion(root),UI_EVENT_KBD_COOKED,key_handler_func,NULL,&key_handler_id);
    //uiInstallRegionHandler(LGadBoxRegion(root),UI_EVENT_MOUSE,mouse_handler_func,NULL,&mouse_handler_id);
    g_pInputBinder->SetValidEvents (ALL_EVENTS); //  & (~UI_EVENT_MOUSE));

    DPCUtilInitColor();

    PickSetCanvas();

    //uiDoubleClickTime = 250;
    //uiDoubleClickDelay = 100;

    build_cmd_term(root);

    if (gScrnLoopSetModeFailed)
    {
        cStr str = FetchUIString("misc","set_mode_failed","strings"); 
        DPCOverlayAddText(str,DEFAULT_MSG_TIME); 
    }
}

void DPC_end_gamemode(void)
{
    if (DPC_mouse)
    {
        MouseMode(FALSE,TRUE);
    }

    //uiDoubleClickTime = 0;
    //uiDoubleClickDelay = 0;

    uiShowMouse(NULL);

    if (DPC_mouse_loaded)
    {
        //DPC_mouse_hnd->Unlock();
        SafeFreeHnd(&DPC_mouse_hnd);
        DPC_mouse_loaded = FALSE;
    }

    LGadRoot* root = LGadCurrentRoot();
    uiRemoveRegionHandler(LGadBoxRegion(root),key_handler_id);
    AutoAppIPtr(PlayerGun);
    pPlayerGun->Off();
    DestroyCommandTerminal();
    DestroyBugTerminal();

    AutoAppIPtr(DPCCamera);
    pDPCCamera->SetEditMode(kCameraNormal);
}

#ifndef SHIP
/*
void DPCSetViewRect(char* args)
{
   int x1 = 0;
   int y1 = 0;
   int x2 = 640;
   int y2 = 480;
   sscanf(args,"%d,%d,%d,%d",&x1,&y1,&x2,&y2);
   dpc_rend_rect.ul.x = x1;
   dpc_rend_rect.ul.y = y1;
   dpc_rend_rect.lr.x = x2;
   dpc_rend_rect.lr.y = y2;
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
    if (pNetManager->IsNetworkGame())
    {
        myPlayerNum = pNetManager->MyPlayerNum();
    }
    else
    {
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
        if (factoryPtr == NULL)
        {
            // It's an old factory with no player num; use it as the
            // default
            defaultFactory = pQuery->ID();
        }
        else
        {
            int factoryNum = *factoryPtr;
            if (factoryNum == myPlayerNum)
            {
                // Got the right one
                id = pQuery->ID();
            }
            else if (defaultFactory == LINKID_NULL)
            {
                // We don't have any default yet, so try this one
                defaultFactory = pQuery->ID();
            }
        }
    }

    SafeRelease(pQuery);

    if (id == LINKID_NULL)
    {
        // We didn't find an appropriate one, so fall back on a default
        if (defaultFactory == LINKID_NULL)
        {
            // There aren't *any* factories on this level!
            return OBJ_NULL;
        }
        else
        {
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
    switch (ev)
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
    { "CurWeapon", 0}, 
}; 

#define NUM_CULP_RELS (sizeof(culp_rels)/sizeof(culp_rels[0]))

// Set up culpability relations for dark
static void DPC_init_culpability()
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

///////////////////////////////////////////
// Skill related parameters
//

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gBindTableDesc = 
{
    kCampaignVar,              // Where do I get saved?
    "BINDTABLE",               // Tag file tag
    "Bind Table",              // friendly name
    FILEVAR_TYPE(sBindTable),  // Type (for editing)
    {1, 0},                    // version
    {1, 0},                    // last valid version 
    "deepc",                   // optional: what game am I in NULL means all 
}; 

////////////////////////////////////////
extern void DPCOpenMFD(int which);

static void DPCSummonObj(char *args)
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

static void DPCUbermensch(void)
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

    AutoAppIPtr(DPCPlayer);
    pDPCPlayer->RecalcData(plr);

}

void DPCVersion()
{
    DPCOverlayAddText(AppName(),DEFAULT_MSG_TIME);
}

extern void DPCEditSynchBrushTextures(void);
extern void DPCEditSynchBrushScales(void);
extern void DPCEditSynchBrushFull(void);
extern void DPCEditMakeDecalBrush(char *args);
//extern void DPCElevatorTest(char *args);
extern void DPCLoadFull(const char *args);
extern void DPCGainPool(int amt);
extern void DPCTrainMFD(int amt);
extern void DPCLoadGameAndLevel(char *args);

static Command DPCDebugKeys[] =
{

    //{ "elevator_test", FUNC_STRING, DPCElevatorTest, "Elevator-transport between two levels"},
    { "dpc_save_game", FUNC_INT, DPCSaveGame,        ""},
    { "dpc_load_game", FUNC_INT, TransModeLoadGame,  ""},
#ifndef SHIP
    { "dpc_load_full", FUNC_STRING, DPCLoadFull,     ""},
    //{ "dpc_load_level", FUNC_STRING, DPCLoadGameAndLevel, "[dpc_load_level <slot> <name>] Loads the slot, and goes into the named level"},

    { "compute_fov", FUNC_VOID, HackFOVCompute,      ""},
    { "letterbox",   FUNC_BOOL, DPCLetterboxSet,     "Set letterbox mode."},
#endif

    { "summon_obj",      FUNC_STRING, DPCSummonObj,  "Summon an object."},
    { "ubermensch",      FUNC_VOID, DPCUbermensch,   "Turns the player into homo superior."},
    { "add_pool",        FUNC_INT, DPCGainPool,      "Give player additional build pool points"},
    { "show_version",    FUNC_VOID, DPCVersion,      "Display version in game mode."},

    // editing commands
#ifdef EDITOR
    //{ "set_view_rect", FUNC_STRING, DPCSetViewRect, "Set the game view rect"},
    { "send_switch",       FUNC_STRING,    SendSwitch,               "Send a switch message to an object"},
    { "test_HUD",          FUNC_INT,       TestHUD,                  "Put HUD brackets around an object."},

    { "camera_pos",        FUNC_VOID,    CameraPositionSpew,         "spew the camera position"},
    { "camera_attach",     FUNC_STRING,  DPCCameraAttach,            "attach the camera position"},
    { "camera_rec",        FUNC_VOID,    SetCameraEditStateRecord,   "Set the camera edit state to record"},
    { "camera_edit",       FUNC_VOID,    SetCameraEditStateEdit,     "Set the camera edit state to edit"},
    { "camera_play",       FUNC_VOID,    SetCameraEditStatePlayback, "Set the camera edit state to playback"},
    { "set_camera_speed",  FUNC_FLOAT,   SetCameraEditSpeed,         "Set the edit camera speed"},
    { "make_scenes",       FUNC_VOID,    MakeScenes,                 "Make all the scene binaries"},
    { "cs_start",          FUNC_INT,     CutSceneStart,              "Start a conversation-based cut-scene (conversationID)"},
    { "cs_rec",            FUNC_INT,     CutSceneRecord,             "Record a new track for a conversation-based cut-scene (conversationID)"},
    { "cs_edit",           FUNC_INT,     CutSceneEdit,               "Edit a conversation-based cut-scene (conversationID)"},
    { "cs_play",           FUNC_INT,     CutScenePlayback,           "Playback a conversation-based cut-scene (conversationID)"},

    { "train_mfd",         FUNC_INT,    DPCTrainMFD,               "Open up a training MFD"},

    { "synch_brush_tx",    FUNC_VOID,   DPCEditSynchBrushTextures, "Set all textures to current face val"},
    { "synch_brush_scale", FUNC_VOID,   DPCEditSynchBrushScales,   "Synch all txt scales to current faces val"},
    { "synch_brush_full",  FUNC_VOID,   DPCEditSynchBrushFull,     "Synch textures & scales"},
    { "decal_brush",       FUNC_STRING, DPCEditMakeDecalBrush,     "[decal_brush x,y] Creates a decal brush off the selected face."},
    // physics update
//   { "update_creature_physics", FUNC_VOID, UpdateCreaturesPhysics,  "Update special flag on creature physics"},
#endif 

};

extern void DrawHistoryToggle(void);
extern void DPCPDAPlayUnreadLog();

    void
DPCUseCamera(void)
{
    AutoAppIPtr(DPCPlayer);
    AutoAppIPtr(ContainSys);
    AutoAppIPtr(ScriptMan);
    ObjID targetObj, camObj, player = PlayerObject();
    BOOL hasCamera = FALSE;
    BOOL anySuccess = FALSE;
    int ammoCount;
    
    sContainIter *piter = pContainSys->IterStart(player);

    while (!piter->finished)
    {
       if (piter->type < DPCCONTAIN_PDOLLBASE)
       {
          if (gPropIsPlayerCamera->IsRelevant(camObj = piter->containee)) {
             hasCamera = TRUE;
             break;
          }
       }
       pContainSys->IterNext(piter);
    }

    pContainSys->IterEnd(piter);
 
    if (!hasCamera) {
       // TDB: play complaint
       return;
    }

    if ((ammoCount = GunStateGetAmmo(camObj)) <= 0) {
       // TDB: play complaint
       return;
    }

    sPropertyObjIter iter;
    gPropIsCameraTarget->IterStart(&iter);
    while (gPropIsCameraTarget->IterNext(&iter,&targetObj))
    {
        // Camera targets have vhot 0 and vhot 1.  vhot 0 is the point
        // to be photographed; vhot 1 is a little out from that in a 
        // normal direction, so the angle of the photo can be enforced.
        BOOL success = TRUE;
        mxs_vector vHotLoc0, vHotLoc1, camPos;
        Location loc0, loc1, hit, camLoc;
        ObjPos *playPos = ObjPosGet(player);
        ObjID hitObj;
        mxs_angvec camAng;
        CameraGetLocation(PlayerCamera(), &camPos, &camAng);
        VHotGetLoc(&vHotLoc0, targetObj, 0);
        VHotGetLoc(&vHotLoc1, targetObj, 1);
        MakeLocationFromVector(&loc0, &vHotLoc0);
        MakeLocationFromVector(&loc1, &vHotLoc1);
        MakeLocationFromVector(&camLoc, &camPos);
        if (
            (!PhysRaycast(loc0, camLoc, &hit, &hitObj, 0.0) == 
                     kCollideSphere) && (hitObj == player)
           )
        {
            success = FALSE;
        }

        mxs_vector hot1Hot0Vec;
        mx_sub_vec(&hot1Hot0Vec, &vHotLoc1, &vHotLoc0);
        mxs_vector playerHot0Vec;
        mx_sub_vec(&playerHot0Vec, &camPos, &vHotLoc0);

        // Make sure not too far away
#define MIN_CAMERA_TARGET_DIST 25
        if (mx_mag_vec(&playerHot0Vec) > MIN_CAMERA_TARGET_DIST) {
           success = FALSE;
        }

        // Make sure position is not too far off from normal (vhot0, vhot1)
        float dot = mx_dot_vec(&hot1Hot0Vec, &playerHot0Vec);
        float angle = 
             acos(dot / 
                  (mx_mag_vec(&hot1Hot0Vec) * mx_mag_vec(&playerHot0Vec)));
#define MAX_CAMERA_TARGET_ANGLE 1.0 // 57 degrees
        if (fabs(angle) > MAX_CAMERA_TARGET_ANGLE) {
           success = FALSE;
        }

        // Make sure we are facing (near enough) the target by comparison
        // with normal
        // First figure out player facing direction
        mxs_matrix orientMat;
        mx_ang2mat(&orientMat, &camAng);
        // Flip direction of of playerHot0Vec for some reason (so following
        // angle computation faces right way)
        mx_negeq_vec(&playerHot0Vec);
        dot = mx_dot_vec(&playerHot0Vec, &orientMat.vec[0]);

        angle = 
             acos(dot / 
                  (mx_mag_vec(&playerHot0Vec) * mx_mag_vec(&orientMat.vec[0])));

#define MAX_SCREEN_ANGLE 0.6 // More than this and thing is probably off screen
        if (fabs(angle) > MAX_SCREEN_ANGLE) {
           success = FALSE;
        }

        if (success) {
            // TBD:  hook into quest
            anySuccess = TRUE;
            sScrMsg msg(player,"PictureTaken");
            msg.data = targetObj;
            pScriptMan->SendMessage(&msg); 
        }

    }
    gPropIsCameraTarget->IterStop(&iter);

    sScrMsg msg(player,"UseCamera");
    msg.data = anySuccess;
    pScriptMan->SendMessage(&msg); 

    GunStateSetAmmo(camObj, --ammoCount);
}

static Command DPCCommands[] =
{
    // hotkey accelerators
    { "toggle_inv",          FUNC_VOID,    DPCToggleInv,        "toggle inv panel"},
    { "cycle_ammo",          FUNC_VOID,    DPCCycleAmmo,        "cycle through available ammo types"},
    { "toggle_compass",      FUNC_VOID,    DPCToggleCompass,    "toggle compass state"},
    { "query",               FUNC_INT,     DPCQueryMode,        "query cursor 1 = on 0 = off"},
    { "split",               FUNC_INT,     DPCSplitMode,        "split cursor 1 = on 0 = off"},
    { "DPC_jump_player",     FUNC_VOID,    DPCPlayerJump,       "jump the player"},
    { "look_cursor",         FUNC_VOID,    DPCLookCursor,       "puts cursor into look mode"},
    { "reload_gun",          FUNC_VOID,    DPCAmmoReload,       "reloads weapon out of inventory"},
    { "swap_guns",           FUNC_VOID,    DPCSwapGuns,         "switches primary and secondary weapons"},
    { "wpn_setting_toggle",  FUNC_VOID,    DPCSettingToggle,    "toggles between weapon settings"},
    { "equip_weapon",        FUNC_STRING,  DPCWeaponEquip,      "searches your inv for a particular weapon and equips it"},
    { "cycle_weapon",        FUNC_INT,     DPCWeaponCycle,      "Cycles through next equippable weapon, 1 for fwd, -1 for back"},
    { "clear_weapon",        FUNC_VOID,    DPCWeaponUnequip,    "unequips current weapon"},
    { "unjam_weapon",        FUNC_VOID,    DPCWeaponUnjam,    "Attempts to unjam current weapon"},
    { "open_mfd",            FUNC_INT,     DPCOpenMFD,          "Open up an MFD by overlay constant"},
    { "stop_email",          FUNC_VOID,    DPCEmailStop,        "Stop any currently playing email/log"},
    { "use_obj",             FUNC_STRING,  DPCUseObj,           "Use an object by name"},
    { "msg_history",         FUNC_VOID,    DrawHistoryToggle,   "Toggle message history"},
    { "play_unread_log",     FUNC_VOID,    DPCPDAPlayUnreadLog, "Play an unread log"},
    { "use_camera",          FUNC_VOID,    DPCUseCamera, "Use camera"},

    // basic interface support
    { "toggle_mouse",        FUNC_VOID,   DPCToggleMode,             "switch between mouselook & cursor modes"},
    { "frob_toggle",         FUNC_VOID,   DPCFrobAndMaybeToggleMode, "switch between modes, also frobbing current selection"},
    { "frob_object",         FUNC_VOID,   DPCFrobObject,             "simple frob of selected object"},
    { "frob_object_inv",     FUNC_VOID,   DPCFrobObjectInv,          "simple frob of selected object, in world or in inv"},
    { "mp_hilight_obj",      FUNC_VOID,   DPCHilightObject,          "hilight an object in MP game"},
    //{ "interface_click", FUNC_VOID, DPCInterfaceClick, "interact with objects / interface" },
    { "interface_use",       FUNC_VOID,    DPCInterfaceUseItem,      "inv use an item"},
    { "fire_weapon",         FUNC_INT,     DPCFireWeapon,            "fire weapon 1 = start 0 = finish"},
    { "drag_and_drop",       FUNC_INT,     DPCDragDropSimple,        "drag & drop 1 = start 0 = finish"},
    { "drag_and_drop_frob",  FUNC_INT,     DPCDragDropFrob,          "drag & drop 1 = start 0 = finish"},
    { "drag_and_drop_mode",  FUNC_INT,     DPCDragDropMode,          "drag & drop 1 = start 0 = finish"},
    //{ "dragndrop", FUNC_VOID, DPCBeginDragDrop, "begin drag & drop operation" },
    //{ "end_dragndrop", FUNC_VOID, DPCBeginDragDrop, "begin drag & drop operation" },

    //{ "fire_at_cursor", FUNC_INT, DPCFireCursor, "fire weapon at cursor, 1=start 0=finish"},
    { "test_begin",          FUNC_VOID,      DPCBeginGame,           ""},
    { "quicksave",           FUNC_VOID,      DPCQuickSaveHack,       "Save to 'current' subdir."},
    { "quickload",           FUNC_VOID,      DPCQuickLoad,           "Load from 'current' subdir."},
};

// Init the Deep Cover-specific game tools.
void DPCToolsInit(void)
{
   IGameTools* pGameTools = AppGetObj(IGameTools);
   pGameTools->SetIsToGameModeGUIDCallback(DPCIsToGameModeGUID);
   SafeRelease(pGameTools);
}

void DPC_init_game(void)
{
    COMMANDS(DPCCommands,HK_ALL); // GAME_MODE | HK_GAME2_MODE);
    // note, debug keys are included in ship build as cheat keys
    COMMANDS(DPCDebugKeys,HK_ALL);

    // Install damage listener/filters
    IDamageModel* pDamageModel = AppGetObj(IDamageModel);
    pDamageModel->Listen(kDamageMsgImpact | kDamageMsgSlay | kDamageMsgDamage,   DPCDamageListener, NULL);
    pDamageModel->Listen(kDamageMsgTerminate,    DPCLootDamageListener,NULL);
    SafeRelease(pDamageModel);

    DPCReactionsInit();

    DPCUtilsInit();

    GunInit();
    setup_player_CB(); 
    setup_contain_CB(); 

    // man, we should clean this up some
    AutoAppIPtr(PlayerGun);
    pPlayerGun->GameInit();
    DPCPropertiesInit();
    CannisterPropertyInit();
    InvDimsPropertyInit();
    DoorOpenSoundPropertyInit();
    DoorCloseSoundPropertyInit();
    DoorTimerPropertyInit();
    DPCFrobsysInit();
    ObjSoundNamePropInit();
    DPCCreaturesInit();
    DPCDamageInit();
    DPCKeyInit();
    DPCFrobPropertiesInit();
    DPCPlayerDamageInit();
    DPCAIInit();
    DPCLinksInit();
    MeleeTypePropertyInit();
    DPCParamEditInit();
    DarkDifficultyInit(); 
    DPCMenusInit(); 
    DPCSoundInit();
    CameraObjPropertyInit();
    DPCMeleeInit();
    GunProjectileInit();
    DPCPDANetInit();
    DPCBloodInit();
    DPCSaveUIInit(); 

    DPCInvListenInit();
    DPCOverlayListenInit();   

    SetGameIBVarsFunc (InitDPCIBVars);

    DPCOptionsMenuInit(); 
    DPCMasterModeInit(); 
    DPCReportInit();

    DPC_init_culpability();
    DPCToolsInit();

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
    {
       gPrimordialMode = DescribeDPCMasterMode();
    }
    if (config_is_defined("quick_start"))
    {
       gPrimordialMode = DescribeDPCInitGameMode();
    }
}

void DPC_term_game(void)
{
    DPCDamageShutDown();
    DPCCreaturesTerm();
    DPCPlayerDamageTerm();
    AutoAppIPtr(PlayerGun);
    pPlayerGun->GameTerm();
    DPCLinksTerm();
    DPCPropertiesShutdown();
    MeleeTypePropertyShutdown();
    DPCUtilsTerm();
    DarkDifficultyTerm(); 
    DPCMenusTerm(); 
    DPCSoundTerm();
    CameraObjPropertyTerm();
    DPCOptionsMenuTerm(); 
    DPCKeyTerm();
    DPCMeleeTerm();
    DPCMasterModeTerm(); 
    GunProjectileTerm();
    DPCPDANetTerm();
    DPCFrobsysTerm();
    DPCReportTerm();
    DPCBloodTerm();
    DPCSaveUITerm(); 

    // Delete network message:
    delete g_pThrowObjMsg;
}

