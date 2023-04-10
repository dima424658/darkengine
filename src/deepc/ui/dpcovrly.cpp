/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <string.h>
#include <math.h>
#include <2d.h>
#include <rect.h>
#include <config.h>
#include <mprintf.h>
#include <lg.h>

#include <res.h>
#include <guistyle.h>
#include <matrix.h>
#include <schema.h>
#include <filevar.h>
#include <contain.h>

#include <wrtype.h>
#include <objsys.h>
#include <osysbase.h>
#include <objpos.h>
#include <playrobj.h>
#include <rendprop.h>
#include <questapi.h>
#include <scrnmode.h>

#include <resapi.h>
#include <lgd3d_o.h>

#include <dpcovrly.h>
#include <dpcovcst.h>
#include <dpciface.h>
#include <dpcutils.h>
#include <dpcgame.h>

#include <dpcbooko.h>
#include <dpclbox.h>
#include <dpcyorn.h>
#include <dpcammov.h>
#include <dpcmeter.h>
#include <dpchud.h>
#include <dpciftul.h>
#include <dpcpda.h>
#include <dpcsecur.h>
#include <dpchplug.h>
#include <dpcscomp.h>
#include <dpchcomp.h>
#include <dpcelev.h>
#include <dpcparam.h>

#include <simtime.h>

//-------------
// Prototypes
//-------------
void DrawOverlayText(unsigned long inDeltaTicks);

//-------------
// Globals
//-------------
int     gOverlayOn[kNumOverlays];
Rect    gOverlayRects[kNumOverlays];
ulong   gOverlayFlags[kNumOverlays];
bool    gHires = TRUE;
ObjID   gOverlayObj;
int     gOverlayObjWhich;
static BOOL gOverlaysInitted = FALSE;

// Here's the type of my global 
struct sRestoreInfo
{
    int restore[kNumOverlays];
    BOOL restoring;
};

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gRestoreInfoDesc = 
{
    kCampaignVar,                // Where do I get saved?
    "OVERLAY",                   // Tag file tag
    "Overlay State",             // friendly name
    FILEVAR_TYPE(sRestoreInfo),  // Type (for editing)
    { 1, 0},                     // version
    { 1, 0},                     // last valid version 
    "deepc",                     // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sRestoreInfo,&gRestoreInfoDesc> gRestoreInfo; 


/*
sOverlayFunc OverlayInv = 
{ 
   // draw
   // init
   // term
   // mouse
   // dclick (really, use)
   // dragdrop
   // key
   // bitmap
   // upschema
   // downschema
   // state
   // transparency
   // distance
   // needmouse
   // alpha
   // update func
   // check contains?
};
*/

// hmm, this is all getting big enough I wonder whether we want a faster accessing method than
// a big ol' array?
OverlayFunc gOverlayDrawFuncs[kNumOverlays] =
{ 
    NULL,            DPCInterfaceDraw, DrawOverlayText,  NULL,            DPCBookDraw,
    NULL,            NULL,             NULL,             NULL,            DPCLetterboxDraw, 
    NULL,            NULL,             DPCYorNDraw,      NULL,            NULL, 
    DPCAmmoDraw,     DPCMetersDraw,    DPCHUDDraw,       NULL,            NULL,
//  DPCAmmoDraw,     DPCMetersDraw,    DPCHUDDraw,       DPCStatsDraw,    DPCSkillsDraw
    NULL,            NULL,             NULL,             NULL,            DPCPDADraw,
//  DPCTraitDraw,    NULL,             NULL,             DPCResearchDraw, DPCPDADraw,
    NULL,            NULL,             NULL,             NULL,            NULL,
    NULL,            NULL,             DPCMiniFrameDraw, DPCSecurityDraw, NULL,
    NULL,            NULL,             NULL,             NULL,            NULL,
//  DPCTrainingDraw, DPCTrainingDraw,  DPCTrainingDraw,  DPCBuyPsiDraw,   DPCTechSkillDraw,
    NULL,            NULL,             DPCSecCompDraw,   DPCComputerDraw, DPCHRMPlugDraw,
    NULL,            DPCElevDraw,      NULL,             NULL,
//  NULL,            DPCElevDraw,      NULL,             DPCTurretDraw,
};
OverlayWhichFunc gOverlayInitFuncs[kNumOverlays] =
{ 
    NULL,            DPCInterfaceInit, NULL,             NULL,            DPCBookInit,
    NULL,            NULL,             NULL,             NULL,            DPCLetterboxInit, 
    NULL,            NULL,             DPCYorNInit,      NULL,            NULL, 
    DPCAmmoInit,     DPCMetersInit,    DPCHUDInit,       NULL,            NULL, 
//  DPCAmmoInit,     DPCMetersInit,    DPCHUDInit,       DPCStatsInit,    DPCSkillsInit, 
    NULL,            NULL,             NULL,             NULL,            DPCPDAInit,
//  DPCTraitInit,    NULL,             NULL,             DPCResearchInit, DPCPDAInit,
    NULL,            NULL,             NULL,             NULL,            NULL,
    NULL,            NULL,             DPCMiniFrameInit, DPCSecurityInit, NULL,
    NULL,            NULL,             NULL,             NULL,            NULL,
//  DPCBuyStatsInit, DPCBuyTechInit,   DPCBuyWeaponInit, DPCBuyPsiInit,   DPCTechSkillInit,
    NULL,            NULL,             DPCSecCompInit,   DPCComputerInit, DPCHRMPlugInit,
    NULL,            DPCElevInit,      NULL,             NULL,
//    NULL,            DPCElevInit,      NULL,             DPCTurretInit,
};
OverlayTermFunc gOverlayTermFuncs[kNumOverlays] =
{
    NULL,            DPCInterfaceTerm, NULL,             NULL,            DPCBookTerm,
    NULL,            NULL,             NULL,             NULL,            DPCLetterboxTerm, 
    NULL,            NULL,             DPCYorNTerm,      NULL,            NULL, 
    DPCAmmoTerm,     DPCMetersTerm,    DPCHUDTerm,       NULL,            NULL, 
//  DPCAmmoTerm,     DPCMetersTerm,    DPCHUDTerm,       DPCStatsTerm,    DPCSkillsTerm, 
    NULL,            NULL,             NULL,             NULL,            DPCPDATerm,
//  DPCTraitTerm,    NULL,             NULL,             DPCResearchTerm, DPCPDATerm,
    NULL,            NULL,             NULL,             NULL,            NULL,
    NULL,            NULL,             DPCMiniFrameTerm, DPCSecurityTerm, NULL,
    NULL,            NULL,             NULL,             NULL,            NULL,
//  DPCTrainingTerm, NULL,             NULL,             DPCBuyPsiTerm,   DPCTechSkillTerm,
    NULL,            NULL,             DPCSecCompTerm,   DPCComputerTerm, DPCHRMPlugTerm,
    NULL,            DPCElevTerm,      NULL,             NULL,
//    NULL,          DPCElevTerm,      NULL,             DPCTurretTerm,
};
// inv & main interface are in this weird inter-related thing that is handled elsewhere
OverlayPointFunc gOverlayMouseFuncs[kNumOverlays] =
{ 
    NULL,                DPCInterfaceHandleMouse, NULL, NULL,                   DPCBookHandleMouse,
    NULL,                NULL,                    NULL, NULL,                   NULL, 
    NULL,                NULL,                    NULL, NULL,                   NULL, 
    DPCAmmoHandleMouse,  DPCMetersHandleMouse,    NULL, NULL,                   NULL, 
//  DPCAmmoHandleMouse,  DPCMetersHandleMouse,    NULL, DPCStatsHandleMouse,    DPCSkillsHandleMouse, 
    NULL,                NULL,                    NULL, NULL, DPCPDAHandleMouse,
//  DPCTraitHandleMouse, NULL,                    NULL, DPCResearchHandleMouse, DPCPDAHandleMouse,
    NULL,                NULL,                    NULL, NULL,                   NULL, 
    NULL,                NULL,                    NULL, DPCSecurityHandleMouse, NULL, 
    NULL,                NULL,                    NULL, NULL,                   NULL, 
//  NULL,                NULL,                    NULL, DPCBuyPsiHandleMouse,   NULL, 
    NULL,                NULL,                    NULL, NULL,                   NULL,
    NULL,                NULL,                    NULL, NULL,                   
};                   

OverlayPointFunc gOverlayDoubleClickFuncs[kNumOverlays];
OverlayPointBoolFunc gOverlayDragDropFuncs[kNumOverlays] = 
{
    NULL, NULL,              NULL, NULL, NULL,
    NULL, NULL,              NULL, NULL, NULL, 
    NULL, NULL,              NULL, NULL, NULL, 
    NULL, DPCMetersDragDrop, NULL, NULL, NULL, 
    NULL, NULL,              NULL, NULL, NULL,
    NULL, NULL,              NULL, NULL, NULL, 
    NULL, NULL,              NULL, NULL, NULL, 
    NULL, NULL,              NULL, NULL, NULL, 
    NULL, NULL,              NULL, NULL, NULL,
    NULL, NULL,              NULL, NULL, 
};
OverlayKeyFunc gOverlayKeyFuncs[kNumOverlays];
OverlayBitmapFunc gOverlayBitmapFuncs[kNumOverlays];

Label gOverlayUpSchema[kNumOverlays];
Label gOverlayDownSchema[kNumOverlays];

OverlayWhichFunc gOverlayStateFuncs[kNumOverlays] =
{ 
    NULL,                   DPCInterfaceStateChange, NULL,                    NULL,                   DPCBookStateChange,
    NULL,                   NULL,                    NULL,                    NULL,                   NULL,
    NULL,                   NULL,                    DPCYorNStateChange,      NULL,                   NULL, 
    DPCAmmoStateChange,     NULL,                    NULL,                    NULL,                   NULL,
//  DPCAmmoStateChange,     NULL,                    NULL,                    DPCStatsStateChange,    DPCSkillsStateChange,
    NULL,                   NULL,                    NULL,                    NULL,                   DPCPDAStateChange,
//  DPCTraitStateChange,    NULL,                    NULL,                    DPCResearchStateChange, DPCPDAStateChange,
    NULL,                   NULL,                    NULL,                    NULL,                   NULL, 
    NULL,                   NULL,                    NULL,                    DPCSecurityStateChange, NULL,
    NULL,                   NULL,                    NULL,                    NULL,                   NULL,
//  DPCBuyStatsStateChange, DPCBuyTechStateChange,   DPCBuyWeaponStateChange, DPCBuyPsiStateChange,   DPCTechSkillStateChange,
    NULL,                   NULL,                    DPCSecCompStateChange,   DPCComputerStateChange, DPCHRMPlugStateChange, 
    NULL,                   DPCElevStateChange,      NULL,                    NULL,
//  NULL,                   DPCElevStateChange,      NULL,                    DPCTurretStateChange,
};
OverlayPointFunc gOverlayTranspFuncs[kNumOverlays] =
{ 
    NULL,                NULL,                 NULL, NULL,                   NULL,
    NULL,                NULL,                 NULL, NULL,                   NULL,
    NULL,                NULL,                 NULL, NULL,                   NULL, 
    DPCAmmoCheckTransp,  DPCMetersCheckTransp, NULL, NULL,                   NULL,
//  DPCAmmoCheckTransp,  DPCMetersCheckTransp, NULL, DPCStatsCheckTransp,    DPCSkillsCheckTransp,
    NULL,                NULL,                 NULL, NULL,                   NULL,
//  DPCTraitCheckTransp, NULL,                 NULL, NULL,                   NULL,
    NULL,                NULL,                 NULL, NULL,                   DPCPDACheckTransp, 
    NULL,                NULL,                 NULL, DPCSecurityCheckTransp, NULL, 
    NULL,                NULL,                 NULL, NULL,                   NULL, 
    NULL,                NULL,                 NULL, NULL,                   DPCHRMPlugCheckTransp,
    NULL,                DPCElevCheckTransp,   NULL, NULL,                   
};

BOOL gOverlayDistance[kNumOverlays] = 
{
    // 0      
    FALSE, FALSE, FALSE, TRUE,  TRUE,
    // 5
    FALSE, FALSE, TRUE,  FALSE, FALSE, 
    // 10
    FALSE, FALSE, TRUE,  TRUE,  FALSE,  //TRUE,
    // 15
    FALSE, FALSE, FALSE, FALSE, FALSE,
    // 20
    TRUE,  FALSE, FALSE, FALSE, FALSE,
    // 25
    FALSE, FALSE, FALSE, FALSE, FALSE, 
    // 30
    FALSE, FALSE, FALSE, FALSE, FALSE, 
    // 35
    TRUE,  TRUE,  TRUE,  TRUE,  FALSE,
    // 40
    FALSE, FALSE, TRUE,  TRUE,  FALSE, 
    // 45
    FALSE, TRUE,  FALSE, TRUE,  
};

// zeros indicate being set by an sOverlayFunc
bool gOverlayNeedMouse[kNumOverlays] =
{ 
    // 0
    0,     TRUE,  FALSE, TRUE,  TRUE,   
    // 5
    FALSE, 0,     TRUE,  0,     FALSE,
    // 10
    0,     TRUE,  TRUE,  TRUE,  TRUE,
    // 15
    FALSE, FALSE, FALSE, TRUE,  TRUE,
    // 20
    TRUE,  TRUE,  0,     TRUE,  TRUE,
    // 25
    TRUE,  TRUE,  0,     FALSE, 0, 
    // 30
    FALSE, FALSE, FALSE, TRUE,  FALSE, 
    // 35
    TRUE,  TRUE,  TRUE,  TRUE,  TRUE,
    // 40
    TRUE,  TRUE,  TRUE,  TRUE,  TRUE, 
    // 45
    FALSE, TRUE,  0,     TRUE,  
};

int gOverlayAlpha[kNumOverlays];

HLGD3DOVERLAY gOverlayTlucHandles[kNumOverlays];
grs_bitmap *gOverlayBitmaps[kNumOverlays];
OverlayBoolFunc gOverlayUpdateFuncs[kNumOverlays];
bool gOverlayCheckContains[kNumOverlays];

// THESE NEXT FIELDS ARE FILLED OUT EVEN IF YOU PROVIDE AN SOVERLAYFUNC!

int gOverlayOrder[kNumOverlays] = 
{ 
    kOverlayCrosshair, kOverlayAlarm,   kOverlayPsiIcons,  kOverlayHackIcon,  kOverlayRadiation, kOverlayPoison,
    kOverlayHUD,       kOverlayVersion, kOverlayTlucText,  

    kOverlayLook,      kOverlayYorN,

    kOverlayLetterbox, kOverlayText,

    kOverlayMeters,    kOverlayAmmo,    kOverlayFrame,     kOverlayInv,       kOverlayMiniFrame, kOverlayTicker,
    kOverlayMouseMode, 

    kOverlayRep,       kOverlayBook,    kOverlayComm,      kOverlayContainer, kOverlayHRM,       kOverlayPsi,
    kOverlayStats,     kOverlaySkills,  kOverlayBuyTraits, kOverlayKeypad,    kOverlayElevator,

    kOverlayBuyStats,  kOverlayBuyTech, kOverlayBuyWeapon, kOverlayBuyPsi,

    kOverlayRadar,     

    kOverlayResearch,  kOverlayPDA,     kOverlayEmail,     kOverlaySetting,   kOverlaySecurity,  

    kOverlayTechSkill, kOverlayMFDGame, kOverlaySecurComp, kOverlayHackComp,  kOverlayMap,
    kOverlayMiniMap,   kOverlayTurret,  kOverlayOverload,

    kOverlayHRMPlug,

};

// need one of these for left and for right
static int exclude_list_left[] =
{
    kOverlayContainer, kOverlayComm,      kOverlayHRM,       kOverlayRep,    
    kOverlayBook,      kOverlayKeypad,    kOverlayEmail,     kOverlayPDA,    kOverlayResearch, 
    kOverlaySetting,   kOverlaySecurity,  
    kOverlayBuyStats,  kOverlayBuyTech,   kOverlayBuyWeapon, kOverlayBuyPsi, 
    kOverlayMFDGame,   
    kOverlaySecurComp, kOverlayHackComp,  
    kOverlayMap,       kOverlayElevator,  kOverlayTurret,
    kOverlayLook,      kOverlayBuyTraits, 
};                 
static int exclude_list_right[] =
{
    kOverlayStats, kOverlaySkills, kOverlayPsi, kOverlayTechSkill, kOverlayMap 
};

#define MAX_OVERLAY_LINES  6
char DPC_overlay_text[MAX_OVERLAY_LINES][255];
ulong DPC_overlay_times[MAX_OVERLAY_LINES];
int DPC_overlay_colors[MAX_OVERLAY_LINES];

//--------------------------------------------------------------------------------------
void DPCOverlayAddFuncs(sOverlayFunc *pFunc, int which)
{
    gOverlayDrawFuncs[which]        = pFunc->m_drawfunc;
    gOverlayInitFuncs[which]        = pFunc->m_initfunc;
    gOverlayTermFuncs[which]        = pFunc->m_termfunc;
    gOverlayMouseFuncs[which]       = pFunc->m_mousefunc;
    gOverlayDoubleClickFuncs[which] = pFunc->m_dclickfunc;
    gOverlayDragDropFuncs[which]    = pFunc->m_dragdropfunc;
    gOverlayKeyFuncs[which]         = pFunc->m_keyfunc;
    gOverlayBitmapFuncs[which]      = pFunc->m_bitmapfunc;
    gOverlayUpdateFuncs[which]      = pFunc->m_updatefunc;

    strcpy(gOverlayUpSchema[which].text,   pFunc->m_upschema.text);
    strcpy(gOverlayDownSchema[which].text, pFunc->m_downschema.text);

    gOverlayStateFuncs[which]    = pFunc->m_statefunc;
    gOverlayTranspFuncs[which]   = pFunc->m_transpfunc;
    gOverlayDistance[which]      = pFunc->m_distance;
    gOverlayNeedMouse[which]     = pFunc->m_needmouse;
    gOverlayAlpha[which]         = pFunc->m_alpha;
    gOverlayCheckContains[which] = pFunc->m_checkcontains;
}

//--------------------------------------------------------------------------------------
extern sOverlayFunc OverlayInv;
extern sOverlayFunc OverlayVersion;
extern sOverlayFunc OverlayContainer;
extern sOverlayFunc OverlayTicker;
extern sOverlayFunc OverlayAlarm;
extern sOverlayFunc OverlayHackIcon;
extern sOverlayFunc OverlayCrosshair;
extern sOverlayFunc OverlayRad;
extern sOverlayFunc OverlayMap;
extern sOverlayFunc OverlayMiniMap;
extern sOverlayFunc OverlayEmail;
extern sOverlayFunc OverlayHRM;
extern sOverlayFunc OverlayKeypad;
extern sOverlayFunc OverlayLook;
extern sOverlayFunc OverlayTlucText;
extern sOverlayFunc OverlayMouseMode;
extern sOverlayFunc OverlaySetting;

void DPCOverlayAddAll()
{
    DPCOverlayAddFuncs(&OverlayInv,       kOverlayInv);
    DPCOverlayAddFuncs(&OverlayVersion,   kOverlayVersion);
    DPCOverlayAddFuncs(&OverlayContainer, kOverlayContainer);
    DPCOverlayAddFuncs(&OverlayTicker,    kOverlayTicker);
    DPCOverlayAddFuncs(&OverlayAlarm,     kOverlayAlarm);
    DPCOverlayAddFuncs(&OverlayHackIcon,  kOverlayHackIcon);
    DPCOverlayAddFuncs(&OverlayCrosshair, kOverlayCrosshair);
    DPCOverlayAddFuncs(&OverlayMap,       kOverlayMap);
    DPCOverlayAddFuncs(&OverlayMiniMap,   kOverlayMiniMap);
    DPCOverlayAddFuncs(&OverlayEmail,     kOverlayEmail);
    DPCOverlayAddFuncs(&OverlayHRM,       kOverlayHRM);
    DPCOverlayAddFuncs(&OverlayKeypad,    kOverlayKeypad);
    DPCOverlayAddFuncs(&OverlayLook,      kOverlayLook);
    DPCOverlayAddFuncs(&OverlayTlucText,  kOverlayTlucText);
    DPCOverlayAddFuncs(&OverlayMouseMode, kOverlayMouseMode);
    DPCOverlayAddFuncs(&OverlaySetting,   kOverlaySetting);
}
//--------------------------------------------------------------------------------------
// Load overlay settings from config file, load in overlay bitmaps.
//--------------------------------------------------------------------------------------
void DPCOverlayInit(void)
{
    int i;

    gOverlaysInitted = TRUE;

    DPCOverlayAddAll(); // set up the big arrays of funcs

    for (i=0; i < kNumOverlays; i++)
    {
        gOverlayOn[i] = FALSE;
        gOverlayFlags[i] = kOverlayFlagNone;
        RectSetNull(&gOverlayRects[i]);
        gOverlayTlucHandles[i] = -1;
        if (gOverlayInitFuncs[i] != NULL)
        {
            gOverlayInitFuncs[i](i);
        }
    }
    DPCMFDNavButtonsInit();

    //gHires = (grd_canvas->bm.w >= 640);
    gOverlayObj = OBJ_NULL;
    gOverlayObjWhich = -1;

    for (i=0; i < MAX_OVERLAY_LINES; i++)
    {
        strcpy(DPC_overlay_text[i],"");
        DPC_overlay_times[i] = 0;
    }

    BOOL slim_mode;
    AutoAppIPtr(QuestData);
    slim_mode = pQuestData->Get("HideInterface");

    if (gRestoreInfo.restoring && !slim_mode)
    {
        int i;
        for (i=0; i < kNumOverlays; i++)
        {
            if (gRestoreInfo.restore[i] == kOverlayModeOn)
            {
                DPCOverlayChange(i, kOverlayModeOn);
            }
        }
    }
    else
    {
        //DPCOverlayChange(kOverlayVersion, kOverlayModeOn);
        DPCOverlayChange(kOverlayText, kOverlayModeOn);

        if (!slim_mode)
        {
            DPCOverlayChange(kOverlayMeters,    kOverlayModeOn);
            DPCOverlayChange(kOverlayAmmo,      kOverlayModeOn);
            DPCOverlayChange(kOverlayCrosshair, kOverlayModeOn);
            DPCOverlayChange(kOverlayHUD,       kOverlayModeOn);
            DPCOverlayChange(kOverlayMiniFrame, kOverlayModeOn);
        }
    }
}

//--------------------------------------------------------------------------------------
// Store off our backup information about which overlays are up and which are down
//--------------------------------------------------------------------------------------
void DPCOverlayComputeRestore()
{
    if (!gOverlaysInitted)
    {
        // Don't try to save out the overlay state if it isn't real. This
        // can happen in multiplayer.
        return;
    }

    int i;
    gRestoreInfo.restoring = TRUE;
    for (i=0; i < kNumOverlays; i++)
    {
        gRestoreInfo.restore[i] = gOverlayOn[i];
    }
}

//--------------------------------------------------------------------------------------
// Free overlay bitmaps.
//--------------------------------------------------------------------------------------
void DPCOverlayTerm(void)
{
    int i;

    DPCOverlayComputeRestore();

    for (i=0; i < kNumOverlays; i++)
    {
        if (gOverlayOn[i])
        {
            DPCOverlayChange(i,kOverlayModeOff);
        }
        if (gOverlayTermFuncs[i] != NULL)
        {
            gOverlayTermFuncs[i]();
        }
    }
    DPCMFDNavButtonsTerm();

    gOverlaysInitted = FALSE;
}

//--------------------------------------------------------------------------------------
// Set the rectangle for a given overlay
// intended use is for the init funcs of overlay subsystems to install their
// dimensions at startup
//--------------------------------------------------------------------------------------
void DPCOverlaySetRect(int which, Rect r)
{
    if ((which < 0) || (which >= kNumOverlays))
    {
        Warning(("DPCOverlaySetRect: invalid overlay %d!\n",which));
        return;
    }

    gOverlayRects[which] = r;
}

//--------------------------------------------------------------------------------------
// Returns the installed rectangle
//--------------------------------------------------------------------------------------
Rect DPCOverlayGetRect(int which)
{
    if ((which < 0) || (which >= kNumOverlays))
    {
        Warning(("DPCOverlayGetRect: invalid overlay %d!\n",which));
        return(gOverlayRects[0]);
    }

    return(gOverlayRects[which]);
}

// finds the topmost overlay underneath the mouse
int FindOverlay(Point pt)
{
    int i,j;

    for (j=0; j < kNumOverlays; j++)
    {
        i = gOverlayOrder[j];
        if (gOverlayOn[i] || (gOverlayFlags[i] & kOverlayFlagAlwaysMouse))
        {
            Rect r = gOverlayRects[i];
            if (RectCheckNull(&r))
                continue;

            if (RectTestPt(&r,pt))
                return(i);
        }
    }
    return(-1);
}


// general overlay-management function array re-vectoring system
bool HandleMouseFunc(Point pt, OverlayPointFunc funcarray[])
{
// only triggered in cursor mode
    bool retval = FALSE;
    int i,j;
    Point usepos;

    for (j=0; j < kNumOverlays; j++)
    {
        i = gOverlayOrder[j];
        if (gOverlayOn[i] || (gOverlayFlags[i] & kOverlayFlagAlwaysMouse))
        {
            if (funcarray[i] != NULL)
            {
                Rect r = gOverlayRects[i];
                if (RectCheckNull(&r))
                {
                    usepos.x = pt.x;
                    usepos.y = pt.y;
                }
                else
                {
                    usepos.x = pt.x - r.ul.x;
                    usepos.y = pt.y - r.ul.y;
                }

                //if (i == kOverlayInv)
                //   mprintf("comparing pt %d, %d to rect %d,%d %d,%d\n",pt.x,pt.y,r.ul.x,r.ul.y,r.lr.x,r.lr.y);

                if (!RectCheckNull(&r) && RectTestPt(&r,pt))
                {
                    // mprintf("overlay click: pt %d, %d (%d, %d) for overlay %d\n",pt.x,pt.y,usepos.x,usepos.y,i);
                    if (funcarray[i](usepos))
                        retval = TRUE;
                }
                // if modal, always capture mouse events no matter what, while up  
                if (gOverlayFlags[i] & kOverlayFlagModal)
                    retval = TRUE;
            }
        }
    }

    return(retval);
}

// damn, this is stupid.  Find a way to integrate with the standard HandleMouseFunc 
// or otherwise generalize the system
bool HandleMouseFuncBool(Point pt, OverlayPointBoolFunc funcarray[], BOOL arg)
{
// only triggered in cursor mode
    bool retval = FALSE;
    int i,j;
    Point usepos;

    for (j=0; j < kNumOverlays; j++)
    {
        i = gOverlayOrder[j];
        if (gOverlayOn[i] || (gOverlayFlags[i] & kOverlayFlagAlwaysMouse))
        {
            if (funcarray[i] != NULL)
            {
                Rect r = gOverlayRects[i];
                if (RectCheckNull(&r))
                {
                    usepos.x = pt.x;
                    usepos.y = pt.y;
                }
                else
                {
                    usepos.x = pt.x - r.ul.x;
                    usepos.y = pt.y - r.ul.y;
                }

                //if (i == kOverlayInv)
                //   mprintf("comparing pt %d, %d to rect %d,%d %d,%d\n",pt.x,pt.y,r.ul.x,r.ul.y,r.lr.x,r.lr.y);

                if (RectCheckNull(&r) || RectTestPt(&r,pt))
                {
                    // mprintf("overlay click: pt %d, %d (%d, %d) for overlay %d\n",pt.x,pt.y,usepos.x,usepos.y,i);
                    if (funcarray[i](usepos, arg))
                        retval = TRUE;
                }
                // if modal, always capture mouse events no matter what, while up  
                if (gOverlayFlags[i] & kOverlayFlagModal)
                    retval = TRUE;
            }
        }
    }

    return(retval);
}

//--------------------------------------------------------------------------------------
// Is the overlay in the way of mouse selection and the like?
//--------------------------------------------------------------------------------------
bool DPCOverlayMouseOcclude(Point pt)
{
    bool retval;
    int which;
    Point loc;
    Rect r;

    which = FindOverlay(pt);
    if (which == -1)
        return(FALSE);

    if (gOverlayTranspFuncs[which] == NULL)
        return(TRUE);

    r = gOverlayRects[which];
    loc.x = pt.x - r.ul.x;
    loc.y = pt.y - r.ul.y;

    retval = !gOverlayTranspFuncs[which](loc);

    return(retval);
}
//--------------------------------------------------------------------------------------
// Vector inputs off to correct subsystem (or just handle, if trivial)
//--------------------------------------------------------------------------------------
bool DPCOverlayClick(Point pt)
{
    bool retval;

    retval = HandleMouseFunc(pt, gOverlayMouseFuncs);

    return(retval);
}

bool DPCOverlayDoubleClick(Point pt)
{
    bool retval;

    retval = HandleMouseFunc(pt, gOverlayDoubleClickFuncs);

    return(retval);
}

bool DPCOverlayDragDrop(Point pt, BOOL start)
{
    bool retval;

    retval = HandleMouseFuncBool(pt, gOverlayDragDropFuncs, start);

    // don't let through anything spurious
    if (!retval)
        if (DPCOverlayMouseOcclude(pt))
            retval = TRUE;

    return(retval);
}

//--------------------------------------------------------------------------------------
bool DPCOverlayHandleKey(int keycode)
{
// only triggered in cursor mode
    bool retval = FALSE;
    int i,j;

    for (j=0; j < kNumOverlays; j++)
    {
        i = gOverlayOrder[j];
        if (gOverlayOn[i] || (gOverlayFlags[i] & kOverlayFlagAlwaysMouse))
        {
            if (gOverlayKeyFuncs[i] != NULL)
            {
                if (gOverlayKeyFuncs[i](keycode))
                    retval = TRUE;

                // if modal, always capture events no matter what, while up  
                if (gOverlayFlags[i] & kOverlayFlagModal)
                    retval = TRUE;
            }
        }
    }

    return(retval);
}
//--------------------------------------------------------------------------------------
// Actually changes overlay state and calls appropriate callbacks
//--------------------------------------------------------------------------------------
void SetOverlay(int which, int mode)
{
    grs_bitmap *bmp;
    IRes *hnd;

    if ((mode != kOverlayModeOn) && (mode != kOverlayModeOff))
    {
        Warning(("SetOverlay for %d, to mode %d\n",which,mode));
        return;
    }

    // drop out if we are already in that mode
    if (DPCOverlayCheck(which) == mode)
        return;

    gOverlayOn[which] = mode;

    // Some modes auto turn on mouse when you enter
    if ((mode == kOverlayModeOn) && gOverlayNeedMouse[which])
    {
        if (!DPC_mouse)
            MouseMode(TRUE,TRUE);
    }

    // Note that state funcs get called AFTER the new state is in place!
    if (gOverlayStateFuncs[which] != NULL)
        gOverlayStateFuncs[which](which);

    if (mode == kOverlayModeOn)
    {
        if (strlen(gOverlayUpSchema[which].text) > 0)
        {
            SchemaPlay(&gOverlayUpSchema[which],NULL);
        }
        if (gOverlayFlags[which] & (kOverlayFlagTranslucent|kOverlayFlagBufferTranslucent))
        {
            if (gOverlayBitmapFuncs[which] != NULL)
            {
                HLGD3DOVERLAY handle;
                sLGD3DOverlayInfo overlay;
                memset(&overlay,0,sizeof(sLGD3DOverlayInfo));
                Rect r = gOverlayRects[which];

                // prep the overlay structure
                overlay.fX0 = r.ul.x;
                overlay.fY0 = r.ul.y;
                overlay.fX1 = r.lr.x;
                overlay.fY1 = r.lr.y;
                overlay.dwFlags = LGD3DOI_ALPHA|LGD3DOI_BITMAP;
                overlay.nAlpha = gOverlayAlpha[which];

                // get the art, or a handle for it at least
                hnd = gOverlayBitmapFuncs[which]();
                if (hnd != NULL)
                {
                    bmp = (grs_bitmap *) hnd->Lock();
                    if (bmp != NULL)
                    {
                        HRESULT retval;
                        grs_bitmap *newbitmap;
                        grs_canvas newcanv;
                        newbitmap = gr_alloc_bitmap(bmp->type,bmp->flags, bmp->w, bmp->h);
                        newbitmap->align = bmp->align;
                        gr_make_canvas(newbitmap, &newcanv);
                        gr_push_canvas(&newcanv);
                        gr_clear(0);
                        gr_bitmap(bmp,0,0);

                        // also draw in initial contents if need be
                        if (gOverlayFlags[which] & kOverlayFlagBufferTranslucent)
                        {
                            // @HACK:  Unless the draw gets reset, this could be a problem
                            gOverlayDrawFuncs[which](0);
                        }

                        // mprintf("clear & draw\n");
                        gr_pop_canvas();
                        hnd->Unlock();
                        //if (gOverlayDrawFuncs[which] != NULL)
                        //gOverlayDrawFuncs[which]();

                        gOverlayBitmaps[which] = newbitmap;

                        overlay.pBitmap = newbitmap;
                        // finally add it to the system
                        retval = lgd3d_aol_add(&overlay, &handle);
                        // mprintf("just added\n");
                        gOverlayTlucHandles[which] = handle;
                    }
                }
            }
        }
    }
    else
    {
        if (strlen(gOverlayDownSchema[which].text) > 0)
        {
            SchemaPlay(&gOverlayDownSchema[which],NULL);
        }
        if (gOverlayFlags[which] & (kOverlayFlagTranslucent|kOverlayFlagBufferTranslucent))
        {
            if (gOverlayTlucHandles[which] != -1)
            {
                lgd3d_aol_remove(gOverlayTlucHandles[which]);
                gOverlayTlucHandles[which] = -1;

                // mprintf("closing bitmap\n");
                gr_close_bitmap(gOverlayBitmaps[which]);
                gr_free(gOverlayBitmaps[which]);
                gOverlayBitmaps[which] = NULL;
                //hnd = gOverlayBitmapFuncs[which]();
                //hnd->Unlock();
            }
        }
    }
}
//--------------------------------------------------------------------------------------
// Turn off old overlays that are contradictory to current overlay
//--------------------------------------------------------------------------------------
void CheckExcludeList(int which, int *exclude_list, int num)
{
    bool do_exclude = FALSE;
    int i;

    // is this in the set of mutual excluders?
    for (i=0; i < num; i++)
    {
        if (which == exclude_list[i])
        {
            do_exclude = TRUE;
            break;
        }
    }

    if (do_exclude)
    {
        // unset all the others in this set
        for (i=0; i < num; i++)
        {
            if (exclude_list[i] != which)
            {
                SetOverlay(exclude_list[i],kOverlayModeOff);
                if (exclude_list[i] == gOverlayObjWhich)
                {
                    gOverlayObj = OBJ_NULL;
                }
            }
        }
    }
}
//--------------------------------------------------------------------------------------
void CheckExclusion(int which)
{
    CheckExcludeList(which, exclude_list_left, sizeof(exclude_list_left) / sizeof(int));
    CheckExcludeList(which, exclude_list_right, sizeof(exclude_list_right) / sizeof(int));
}

//--------------------------------------------------------------------------------------
// Turn individual overlays on or off, or toggle them
//--------------------------------------------------------------------------------------
void DPCOverlayChangeObj(int which, int mode, ObjID obj)
{
    BOOL newmode;
    // turn off any others that want same "real estate"
    if ((mode == kOverlayModeOn) 
        || ((mode == kOverlayModeToggle) && (!gOverlayOn[which])))
    {
        CheckExclusion(which);
    }

    // bring up/down the actual thing
    if (mode == kOverlayModeToggle)
        newmode = !gOverlayOn[which];
    else
        newmode = mode;

    if ((newmode == 0) && (which == gOverlayObjWhich) && (gOverlayObj != OBJ_NULL))
        DPCOverlaySetObj(which, OBJ_NULL);
    if (obj != OBJ_NULL)
        DPCOverlaySetObj(which,obj);

    SetOverlay(which, newmode);
}

//--------------------------------------------------------------------------------------
void DPCOverlayChange(int which, int mode)
{
    DPCOverlayChangeObj(which, mode, OBJ_NULL);
}

//--------------------------------------------------------------------------------------
// Check the state of a particular overlay
//--------------------------------------------------------------------------------------
bool DPCOverlayCheck(int which)
{
    return(gOverlayOn[which]);
}

//--------------------------------------------------------------------------------------
// Draw the overlay bitmaps.
//--------------------------------------------------------------------------------------
void DPCOverlayDoFrame(unsigned long inDeltaTicks)
{
    int i,j;
    // check to see if we've moved too far away from the reference obj
    if ((gOverlayObj != OBJ_NULL) && (gOverlayDistance[gOverlayObjWhich]))
    {
        ObjPos *p1, *p2;
        mxs_real dist;

        // verify that mouseobj is close enough to focus 
        if (!ObjHasRefs(gOverlayObj))
            dist = 0;
        else
        {
            p1 = ObjPosGet(gOverlayObj);
            p2 = ObjPosGet(PlayerObject());
            if ((p1 == NULL) || (p2 == NULL))
            {
                // okay, this is not really warning worthy, we should just treat it like a giant distance
                //Warning(("DPCOverlayDoFrame: couldn't get object position for %d, %d\n",gOverlayObj,PlayerObject()));
                dist = 10000;
            }
            else
                dist = mx_dist2_vec(&p1->loc.vec, &p2->loc.vec);
        }
        float maxdist;
        sGameParams *params = GetGameParams();
        maxdist = params->overlaydist;
        if (dist > maxdist) // OVERLAY_DIST
        {
            if (!gOverlayOn[gOverlayObjWhich])
                Warning(("DPCOverlayDoFrame: overlay %d is already off!\n",gOverlayObjWhich));
            DPCOverlayChange(gOverlayObjWhich,kOverlayModeOff);
        }
    }

    // now call the draw functions for every "on" overlay
    for (j=0; j < kNumOverlays; j++)
    {
        i = gOverlayOrder[j];
        if (gOverlayOn[i] || (gOverlayFlags[i] & kOverlayFlagAlwaysDraw))
        {
            if ((gOverlayDrawFuncs[i] != NULL) && !(gOverlayFlags[i] & kOverlayFlagBufferTranslucent))
            {
                // nice, simple case
                gOverlayDrawFuncs[i](inDeltaTicks);
            }
        }
    }
}

//--------------------------------------------------------------------------------------
void DPCOverlayDrawBuffers()
{
    int i,j;
    // now call the draw functions for every "on" overlay
    for (j=0; j < kNumOverlays; j++)
    {
        i = gOverlayOrder[j];
        if (gOverlayOn[i] || (gOverlayFlags[i] & kOverlayFlagAlwaysDraw))
        {
            if (gOverlayFlags[i] & kOverlayFlagBufferTranslucent)
            {
                BOOL need_update = TRUE;
                if ((gOverlayUpdateFuncs[i] != NULL) && (!gOverlayUpdateFuncs[i]()))
                    need_update = FALSE;
                if (need_update && (gOverlayDrawFuncs[i] != NULL))
                {
                    //mprintf("handle %d = %d\n",i,gOverlayTlucHandles[i]);
                    // okay, we have to update the tluc buffer
                    grs_bitmap *pBmp,*back;
                    grs_canvas canv;
                    IRes *hnd;
                    HRESULT locked;

                    hnd = gOverlayBitmapFuncs[i]();
                    locked = lgd3d_aol_lock_bitmap_data(gOverlayTlucHandles[i],&pBmp);
                    if (locked >= 0)
                    {
                        //mprintf("B. bmp->bits = %x\n",pBmp->bits);
                        gr_make_canvas(pBmp, &canv);
                        gr_push_canvas(&canv);

                        // redraw the background
                        if (hnd != NULL)
                        {
                            //gr_clear(0);
                            back = (grs_bitmap *) hnd->Lock();
                            gr_bitmap(back,0,0);
                            hnd->Unlock();
                            //mprintf("drawing background\n");
                        }

                        // @HACK:  Unless the draw gets reset, this could be a problem.
                        gOverlayDrawFuncs[i](0);
                        gr_pop_canvas();
                        gr_close_canvas(&canv);
                        lgd3d_aol_unlock_bitmap_data(gOverlayTlucHandles[i]);
                    }
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------
// should move all the overlay text out to it's own module as well

#define HISTORY_LENGTH  18  

struct sMsgHistory 
{
    char m_text[HISTORY_LENGTH][255];
    int m_colors[HISTORY_LENGTH];
};

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gMsgHistoryDesc = 
{
    kCampaignVar,                // Where do I get saved?
    "MSGHISTORY",                // Tag file tag
    "Message History",           // friendly name
    FILEVAR_TYPE(sMsgHistory),   // Type (for editing)
    { 1, 0},                     // version
    { 1, 0},                     // last valid version 
    "deepc",                     // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sMsgHistory,&gMsgHistoryDesc> gMsgHistory; 

// also be filevar?
static BOOL gDrawHistory = FALSE;

void DrawHistoryToggle(void)
{
    gDrawHistory = !gDrawHistory;
}
//--------------------------------------------------------------------------------------
// Set the refence obj and a connected overlay 
// So that we can drop the overlay if you get too far away
//--------------------------------------------------------------------------------------
void DPCOverlaySetObj(int which, ObjID obj)
{
    if ((gOverlayObj != OBJ_NULL) && (obj != OBJ_NULL))
        Warning(("DPCOverlaySetObj: setting to obj %d (%d), but already set!\n",obj,which));
    gOverlayObj = obj;
    gOverlayObjWhich = which;
}

//--------------------------------------------------------------------------------------
// Retrieve the reference obj
//--------------------------------------------------------------------------------------
ObjID DPCOverlayGetObj(void)
{
    return(gOverlayObj);
}

//--------------------------------------------------------------------------------------
// Set flag state
//--------------------------------------------------------------------------------------
void DPCOverlaySetFlags(int which, ulong flags)
{
    gOverlayFlags[which] |= flags;
}
//--------------------------------------------------------------------------------------
// utility for transparency checks
//--------------------------------------------------------------------------------------
bool DPCOverlayCheckTransp(Point p, int, IRes *art)
{
    bool retval = FALSE;
    int pix;

    pix = HandleGetPix(art,p);

    if (pix == 0)
        retval = TRUE;
    return(retval);
}
//--------------------------------------------------------------------------------------
// Draw the 'onscreen' text
//--------------------------------------------------------------------------------------
#define OVTEXT_X  192
#define OVTEXT_Y1 18
#define OVTEXT_Y2 130

#define OVTEXT_WIDTH 446
#define OVTEXT_SPACING  5
static void DrawOverlayText(unsigned long inDeltaTicks)
{
    char str[255];
    int i, cleanup;

    sScrnMode smode;
    ScrnModeGet(&smode);

    int drawx = OVTEXT_X;
    // in high resolution modes, shift the text to the right so that we
    // don't conflict with the hacking tab.  The 70 is a bit more than
    // the width of said tab.
    if (smode.w >= 800)
    {
        drawx += 70;
    }

    int drawy = DPCOverlayCheck(kOverlayInv) ? OVTEXT_Y2 : OVTEXT_Y1;
    
    if (gDrawHistory)
    {
        for (i=HISTORY_LENGTH - 1; i >= 0; i--)
        {
            strcpy(str,gMsgHistory.m_text[i]);
            if (strlen(str) > 0)
            {
                //mprintf("%d: using color %d\n",i,gMsgHistory.m_colors[i]);
                gr_set_fcolor(gMsgHistory.m_colors[i]);
                gr_font_string_wrap(gDPCFontMono, str, OVTEXT_WIDTH);
                gr_font_string(gDPCFontMono, str, drawx, drawy);
                drawy = drawy + gr_font_string_height(gDPCFontMono, str) + OVTEXT_SPACING;
            }
        }
    }
    else
    {
        for (i=0; i < MAX_OVERLAY_LINES; i++)
        {
            strcpy(str,DPC_overlay_text[i]);
            if (strlen(str) > 0)
            {
                gr_set_fcolor(DPC_overlay_colors[i]);
                gr_font_string_wrap(gDPCFontMono, str, OVTEXT_WIDTH);
                gr_font_string(gDPCFontMono, str, drawx, drawy);
                drawy = drawy + gr_font_string_height(gDPCFontMono, str) + OVTEXT_SPACING;
            }
        }
    }

    // now check for timed-out text messages
    cleanup = -1;
    for (i=0; i < MAX_OVERLAY_LINES; i++)
    {
        if ((DPC_overlay_times[i] != 0) && (GetSimTime() > DPC_overlay_times[i]))
            cleanup = i;
    }
    if (cleanup != -1)
    {
        for (i=0; i < MAX_OVERLAY_LINES - (cleanup + 1); i++)
        {
            strcpy(DPC_overlay_text[i],DPC_overlay_text[i+1]);
            DPC_overlay_times[i] = DPC_overlay_times[i+1];
            DPC_overlay_colors[i] = DPC_overlay_colors[i+1];
        }
        for (i = MAX_OVERLAY_LINES - (cleanup + 1); i < MAX_OVERLAY_LINES; i++)
        {
            strcpy(DPC_overlay_text[i],"");
            DPC_overlay_times[i] = 0;
            DPC_overlay_colors[i] = gDPCTextColor;
        }
    }
}
//--------------------------------------------------------------------------------------
// Add some overlay text to the list in the next available slot
//--------------------------------------------------------------------------------------

static void safeStrcpy(char *target, const char *text)
{
    strncpy(target, text, 254);
    target[254] = '\0';
}

//
// Here's the list of which files we know how to save into
//

void DPCOverlayAddColoredText(const char *text, ulong time, int color)
{
    if (DPCOverlayCheck(kOverlayText))
        SchemaPlay((Label *)"linebeep", NULL);

    int i,j;
    for (i = 0; i < MAX_OVERLAY_LINES; i++)
    {
        if (strlen(DPC_overlay_text[i]) == 0)
            break;
    }

    // Not enough space? Scroll off the top line...
    if (i == MAX_OVERLAY_LINES)
    {
        for (j=0; j < MAX_OVERLAY_LINES - 1; j++)
        {
            strcpy(DPC_overlay_text[j],DPC_overlay_text[j+1]);
            DPC_overlay_times[j] = DPC_overlay_times[j+1];
            DPC_overlay_colors[j] = DPC_overlay_colors[j+1];
        }
        i = MAX_OVERLAY_LINES - 1;
    }

    // Now add the new line...
    safeStrcpy(DPC_overlay_text[i], text);
    DPC_overlay_times[i] = GetSimTime() + time;
    DPC_overlay_colors[i] = color;

    // add to history
    // slow, but stupid
    for (i=HISTORY_LENGTH-1; i >= 1 ; i--)
    {
        strncpy(gMsgHistory.m_text[i],gMsgHistory.m_text[i-1],254);
        gMsgHistory.m_colors[i] = gMsgHistory.m_colors[i-1];
    }

    // store it
    //mprintf("setting color 0 to %d\n",color);
    gMsgHistory.m_colors[0] = color;
    safeStrcpy(gMsgHistory.m_text[0],text);
}
void DPCOverlayAddText(const char *text, ulong time)
{
    // Default color is currently pure green...
    DPCOverlayAddColoredText(text, time, gDPCTextColor);
}

//--------------------------------------------------------------------------------------
// Mouse mode has changed, deal appropriately
//--------------------------------------------------------------------------------------
void DPCOverlayMouseMode(bool mode)
{
    for (int i = 0; i < kNumOverlays; i++)
    {
        // okay, if we need the mouse, and the mouse has just gone away, auto close down
        // is this the right behavior?  who knows!
        if (gOverlayNeedMouse[i] && !mode)
        {
            DPCOverlayChange(i,kOverlayModeOff);
        }
    }
    // there are certain overlays that are always up or
    // down in the given modes
    if (mode)
    {
        DPCOverlayChange(kOverlayFrame, kOverlayModeOn);
        DPCOverlayChange(kOverlayTicker, kOverlayModeOn);
        //DPCOverlayChange(kOverlayMiniFrame, kOverlayModeOff);
        DPCOverlayChange(kOverlayCrosshair, kOverlayModeOff);

        DPCOverlayChange(kOverlayInv, kOverlayModeOn);
        DPCOverlayChange(kOverlayMouseMode, kOverlayModeOn);

        SchemaPlay((Label *)"mainpanel_op", NULL);
    }
    else
    {
        DPCOverlayChange(kOverlayFrame, kOverlayModeOff);
        DPCOverlayChange(kOverlayTicker, kOverlayModeOff);
        //DPCOverlayChange(kOverlayMiniFrame, kOverlayModeOn);
        DPCOverlayChange(kOverlayCrosshair, kOverlayModeOn);
        SchemaPlay((Label *)"mainpanel_cl", NULL);
    }
}


//--------------------------------------------------------------------------------------
void DeferOverlayClose(void *which)
{
    int overlay = (int)which;
    DPCOverlayChange(overlay,kOverlayModeOff);
}

//--------------------------------------------------------------------------------------
void DeferOverlayOpen(void *which)
{
    int overlay = (int)which;
    DPCOverlayChange(overlay,kOverlayModeOn);
}

//--------------------------------------------------------------------------------------
void DeferOverlayToggle(void *which)
{
    int overlay = (int)which;
    DPCOverlayChange(overlay,kOverlayModeToggle);
}

//--------------------------------------------------------------------------------------
extern void DPCOpenMFD(int which)
{
    BOOL slim_mode;
    AutoAppIPtr(QuestData);
    slim_mode = pQuestData->Get("HideInterface");
    if (!slim_mode)
        DPCOverlayChange(which, kOverlayModeToggle);
}

//--------------------------------------------------------------------------------------
bool DPCOverlayFullTransp(Point pt)
{
    return(TRUE);
}

//--------------------------------------------------------------------------------------
float DPCOverlayGetDist(int which)
{
    if (gOverlayDistance[which])
    {
        float maxdist;
        sGameParams *params = GetGameParams();
        maxdist = params->overlaydist;
        return(maxdist);
    }
    else
        return(0);
}

//--------------------------------------------------------------------------------------
void DPCOverlaySetDist(int which, BOOL dist)
{
    gOverlayDistance[which] = dist;
}

//--------------------------------------------------------------------------------------
BOOL DPCOverlayPlayerContainsListener(eContainsEvent event, ObjID container, ObjID containee, eContainType ctype, ContainCBData)
{
    // This actually gets called for any containment change, but we
    // only care about the player:
    if (container != PlayerObject())
    {
        return TRUE;
    }

    // is this an object losing contains link from player?
    if (event != kContainRemove)
    {
        return TRUE;
    }

    // is this the MFD obj
    if (DPCOverlayGetObj() != containee)
    {
        return TRUE;
    }

    // iterate over all up MFDs
    // if they care about contains maintenance then bring 'em down
    
    for (int i = 0; i < kNumOverlays; i++)
    {
        if (gOverlayOn[i])
        {
            if (gOverlayCheckContains[i])
            {
                DPCOverlayChange(i,kOverlayModeOff);
            }
        }
    }

    // we always want to allow the action
    return(TRUE);
}

//--------------------------------------------------------------------------------------
void DPCOverlayListenInit()
{
    AutoAppIPtr(ContainSys);
    pContainSys->Listen(gPlayerObj,DPCOverlayPlayerContainsListener,NULL);
}
