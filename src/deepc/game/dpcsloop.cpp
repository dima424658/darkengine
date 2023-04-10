/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/////////////////////////////////////////////////////////////
// DEEP COVER SIMULATION LOOP CLIENT
////////////////////////////////////////////////////////////

#include <lg.h>
#include <loopapi.h>
#include <config.h>
#include <res.h>

#include <dispbase.h>  
#include <loopmsg.h>
#include <dbasemsg.h>

#include <netman.h>

#include <ailoop.h>
#include <biploop.h>
#include <simloop.h>
#include <dpcloop.h>
#include <physloop.h>
#include <plyrloop.h>
#include <objloop.h>
#include <gen_bind.h>
#include <scrnmode.h>

#include <uiloop.h>
#include <netloop.h>
#include <gamemode.h>
#include <transmod.h>

#include <headmove.h>
#include <dpcgame.h>
#include <playrobj.h>
#include <simstate.h>
#include <simflags.h>
#include <prjctile.h>
#include <gamemode.h>
#include <dpcmulti.h>
#include <dpcnet.h>
#include <frobctrl.h>
#include <dpcammov.h>

#include <prjctile.h>
#include <particle.h>

#include <label.h>
#include <hpprop.h>
#include <dmgprop.h>
#include <slitprop.h>
#include <doorprop.h>

#include <dpcplayr.h>
#include <gunprop.h>
#include <dpcctrl.h>
#include <dpcprop.h>
#include <dpcinv.h>
#include <scrptprp.h>
#include <dpcpgapi.h>
#include <dpcreact.h>
#include <dpccmapi.h>
#include <dpcplcst.h>
#include <dpccmobj.h>
#include <dpciface.h>
#include <dpcifstr.h>
#include <dpcutils.h>
#include <dpcplprp.h> // For DPCHACK below
#include <dpcinvpr.h> // For DPCHACK below
#include <engfprop.h> // For DPCHACK below
#include <aivision.h> // For DPCHACK below
#include <aiprcore.h> // For DPCHACK below
#include <dpccam.h> 

// Must be last header
#include <dbmem.h>

static bool gMode;

//////////////////
// CONSTANTS
//
// These are just here to separate out boiler-plate code and leave it untouched
//

#define MY_FACTORY DPCSimLoopFactory
#define MY_GUID   LOOPID_DPCSim

// My context data
typedef void Context;

// My state
typedef struct _StateRecord
{
    Context* context; // a pointer to the context data I got.
    BOOL in_game_mode;
    // State fields go here
} StateRecord;

static IPlayerGun *g_pPlayerGun = NULL;
static IDPCCamera *g_pDPCCamera = NULL;

////////////////////////////////////////
//
// LOOP/DISPATCH callback
// Here's where we do the dirty work.
//

static void db_message(DispatchData* msg)
{
    msgDatabaseData data;
    data.raw = msg->data;

    switch (DB_MSG(msg->subtype))
    {
        case kDatabaseReset:
            {
                AutoAppIPtr(PlayerGun);
                pPlayerGun->Off();
            }
            break;
        case kDatabasePostLoad:
        case kDatabaseDefault:
            InitProjectileArchetype();
            InitParticleArchetype();
            DPCInvReset();
            //gChar->ClearEquip();
            DPCReactionsPostLoad();

            // Clear some variables that will get awkward if we begin running
            // frames without the sim on (which will happen in multiplayer).
            frobWorldSelectObj = OBJ_NULL;
            g_ifaceFocusObj = OBJ_NULL;
            gHelpString[0] = '\0';
            break;
    }
}

void EquipWeapon(void)
{
    if (PlayerObjectExists())
    {
        ObjID gunobj;
        //AutoAppIPtr(PlayerGun);
        AutoAppIPtr(DPCPlayer);
        gunobj = pDPCPlayer->GetEquip(PlayerObject(),kEquipWeapon);
        //pPlayerGun->Set(gunobj);
        pDPCPlayer->SetWeaponModel(gunobj,FALSE);
        DPCAmmoRefreshButtons();
    }
}

void UnequipWeapon(void)
{
    if (PlayerObjectExists())
    {
        AutoAppIPtr(DPCPlayer);
        pDPCPlayer->SetWeaponModel(OBJ_NULL,FALSE);
    }
}

static BOOL g_endingGameMode;

    BOOL
isEndingGameMode(
    void
)
{
    return g_endingGameMode;
}

#pragma off(unreferenced)
static eLoopMessageResult LGAPI _LoopFunc(void* data, eLoopMessage msg, tLoopMessageData hdata)
{
    // useful stuff for most clients
    eLoopMessageResult result = kLoopDispatchContinue; 
    StateRecord* state = (StateRecord*)data;
    LoopMsg info;

    info.raw = hdata; 

    AutoAppIPtr(DPCPlayer);

    switch (msg)
    {
        case kMsgAppInit:
            {
                DPC_init_game();
                //ResOpenFile("dpcres.res");
                g_pPlayerGun = AppGetObj(IPlayerGun);
                g_pDPCCamera = AppGetObj(IDPCCamera);
                DPCNetInit();
            }
            break;

        case kMsgAppTerm:
            // gee, maybe we should terminate some stuff here
            // well, now we do!
            DPC_term_game();
            SafeRelease(g_pPlayerGun);
            SafeRelease(g_pDPCCamera);
            DPCNetTerm();
            break;

        case kMsgEnterMode:
            state->in_game_mode = IsEqualGUID(*info.mode->to.pID,LOOPID_GameMode);
            if (state->in_game_mode)
            {
                // @HACK: start player with pistol and shotgun
                /*
                gChar->AddGun(0, (Label*)"Pistol");
                gChar->AddGun(1, (Label*)"Shotgun");
                gChar->SelectGun(0);
                */
                // center so mouse look around will work
                headmoveInit();
                EquipWeapon();
                DPC_start_gamemode();
                {
                    // DPCHACK: temp holder for initial stats
                    ObjID playerObj = PlayerObject();
                    sStatsDesc Stats = {1,1,1,1,1};
                    sContainDims Dims = {3,3}; 
                    sBashParamsProp BashProps = {0,0};
                    sAIVisibilityMods VisMods = {0,0,0,0,0,0};
                    sWeaponSkills WeaponSkills = {1,1,1,1};
                    sTechSkills TechSkills = {2,2,2,2,2};
                    g_BaseStatsProperty->Set(playerObj,&Stats);
                    g_ContainDimsProperty->Set(playerObj, &Dims);
                    g_pBashParamsProp->Set(playerObj, &BashProps);
                    g_pAIVisibilityModProperty->Set(playerObj, &VisMods);
                    g_BaseWeaponProperty->Set(playerObj, &WeaponSkills);
                    g_BaseTechProperty->Set(playerObj, &TechSkills);
                }
                // since we may have updated the difficulty level
                pDPCPlayer->RecalcData(PlayerObject());
            }
            break;
        case kMsgExitMode:
            if (state->in_game_mode)
            {
                g_endingGameMode = TRUE;
                DPC_end_gamemode();
                UnequipWeapon();
                ZoomTarget(1.0, 0.0);
                g_endingGameMode = FALSE;
            }
            break;

        case kMsgNormalFrame:
            if (SimStateCheckFlags(kSimGameSpec))
            {
                DPC_check_keys();
                g_pDPCCamera->Frame();
                g_pPlayerGun->Frame(info.frame->dTicks);
                AutoAppIPtr(CameraObjects);
                pCameraObjects->Frame(info.frame->dTicks);
                // Don't move this next line; it needs to be be before
                // DPC_sim_update_frame() because it exists only to handle
                // the wield-on-next-frame hack in dpcplayr.cpp.
                pDPCPlayer->Frame();

                DPC_sim_update_frame(info.frame->dTicks);

                // The player started a quicksave, and we've delayed a frame
                // in order to display a message:
                if (gQuickSaveHack > 0)
                {
                    gQuickSaveHack--;
                    if (gQuickSaveHack == 0)
                        DPCQuickSave();
                }

                // We've changed levels, so let's do a save now:
                if (gTransSaveHack)
                {
                    gTransSaveHack = FALSE;

                    // If we're networked, then only the host decides to save.
                    // As a side-effect, this will tell everyone else to save
                    // as well.
                    AutoAppIPtr(NetManager);
                    if (!pNetManager->IsNetworkGame() ||
                        pNetManager->AmDefaultHost())
                    {
                        char temp[255];
                        DPCStringFetch(temp,sizeof(temp),"QuickSaveName","misc");
                        DPCSaveGame(-1,temp);
                    }
                }
            }

            // Some other player initiated a save, and we've delayed a frame
            // in order to display a message. This will *often* happen when
            // the sim is paused...
            if (gRemoteSaveHack > 0)
            {
                gRemoteSaveHack--;
                if (gRemoteSaveHack == 0)
                {
                    DoDPCSaveGame(gRemoteSaveSlot, gpRemoteSaveDesc);
                    if (gpRemoteSaveDesc)
                    {
                        free(gpRemoteSaveDesc);
                        gpRemoteSaveDesc = NULL;
                    }
                }
            }
            break;

        case kMsgDatabase:
            db_message(info.dispatch);
            break;

            // MAHK 7/2/99  Why is suspend/resume totally divergent from exit/enter?
            // I am fixing this so that it does nothing in non-game modes,
            // since our db_message implies that we are in the base mode.
            // I bet there are other suspend/resume bugs, but fuck, we
            // were supposed to ship this yesterday.     
        case kMsgSuspendMode:
            if (state->in_game_mode)
            {
                if (DPC_mouse)
                {
                    gMode = TRUE;
                    MouseMode(FALSE,FALSE);
                }
                else
                    gMode = FALSE;
                UnequipWeapon();
            }
            break;
        case kMsgResumeMode:
            if (state->in_game_mode)
            {
                if (gMode)
                {
                    MouseMode(TRUE,FALSE);
                    gMode = FALSE;
                }
                else
                {
                    sScrnMode smode;
                    int centerx,centery;
                    ScrnModeGet(&smode);
                    centerx = smode.w / 2; //grd_visible_canvas->bm.w / 2;
                    centery = smode.h / 2; //grd_visible_canvas->bm.h / 2;
                    mouse_put_xy(centerx, centery);
                }
                EquipWeapon();
                // since we may have updated the difficulty level
                pDPCPlayer->RecalcData(PlayerObject());
            }
            break;
        case kMsgEnd:
            Free(state);
            break;   
    }
    return result;
}

////////////////////////////////////////////////////////////
//
// Loop client factory function.
//

#pragma off(unreferenced)
static ILoopClient* LGAPI _CreateClient(const sLoopClientDesc* desc, tLoopClientData data)
{
    StateRecord* state;
    // allocate space for our state, and fill out the fields
    state = (StateRecord*)Malloc(sizeof(StateRecord));
    state->context = (Context*)data;

    return CreateSimpleLoopClient(_LoopFunc,state,desc);
}
#pragma on(unreferenced)

///////////////
// DESCRIPTOR
//

sLoopClientDesc DPCSimLoopClientDesc =
{
    &MY_GUID,                           // GUID
    "DPC Simulation",                    // NAME        
    kPriorityNormal,                    // PRIORITY
    kMsgEnd | kMsgsMode | kMsgsFrameMid | kMsgsAppOuter | kMsgDatabase,   // INTERESTS

    kLCF_Callback,
    _CreateClient,

    NO_LC_DATA,

    {
        {kConstrainAfter, &LOOPID_Physics, kMsgsFrame},
        {kConstrainBefore, &LOOPID_SimFinish, kMsgsFrame},
        {kConstrainAfter, &LOOPID_UI, kMsgsMode},
        {kConstrainAfter, &LOOPID_Game, kMsgsMode},
        {kConstrainAfter, &LOOPID_Biped, kMsgsAppOuter},
        {kConstrainBefore, &LOOPID_Biped, kMsgsFrame},
        {kConstrainAfter, &LOOPID_Player, kMsgsFrame},
        {kConstrainBefore, &LOOPID_AI, kMsgsAppOuter},
        //{kConstrainAfter, &LOOPID_Render, kMsgsFrame},
// @Note (toml 04-13-98) dark has the following constraint for the equivalent client: 
        {kConstrainAfter, &LOOPID_ObjSys, kMsgDatabase},
        {kNullConstraint} // terminator
    }
};


