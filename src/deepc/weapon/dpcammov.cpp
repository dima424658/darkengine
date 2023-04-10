/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <2d.h>

#include <res.h>
#include <guistyle.h>
#include <appagg.h>

#include <resapi.h>
#include <mprintf.h>

#include <objtype.h>
#include <contain.h>
#include <playrobj.h>
#include <traitman.h>
#include <scrnmode.h>
#include <gamestr.h>
#include <command.h>
#include <schema.h>
#include <questapi.h>
#include <plyrmode.h>

#include <dpcgame.h>
#include <dpcutils.h>
#include <dpcovrly.h>
#include <dpcpgapi.h>
#include <gunapi.h>
#include <gunprop.h>
#include <dpcinv.h>
#include <dpcovcst.h>
#include <dpcplcst.h>
#include <dpcplayr.h>
#include <dpccurm.h>
#include <dpcprop.h>
#include <dpcobjst.h>
#include <dpcincst.h>
#include <dpcqbind.h>
#include <dpcmelpr.h>
#include <dpcifstr.h>

#include <dpcammov.h>

// ui library not C++ ized properly yet 
extern "C" 
{
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
}

static IRes *gHndAmmoBack = NULL;
static IRes *gHndAmmoBack2 = NULL;
static ObjID gPrevGun = OBJ_NULL;

static BOOL g_interface = FALSE;

// SHOW_AMMO_UI - #define SHOW_AMMO_UI to get all supplimental ammo UI.
// #define SHOW_AMMO_UI

#ifdef SHOW_AMMO_UI
static Rect setting_rect = {{118,15},{118 + 66, 15 + 20}};
static LGadButton setting_button;
static DrawElement setting_elem;
static IRes *setting_handles[2];
static grs_bitmap *setting_bitmaps[4];

static Rect cycle_rect = {{186,15},{186+12, 15 + 41}};
static LGadButton cycle_button;
static DrawElement cycle_elem;
static IRes *cycle_handles[2];
static grs_bitmap *cycle_bitmaps[4];

static Rect reload_rect = {{118,37},{118+ 66, 37 + 20}};
static LGadButton reload_button;
static DrawElement reload_elem;
static IRes *reload_handles[2];
static grs_bitmap *reload_bitmaps[4];
#endif // SHOW_AMMO_UI

#define NUM_WEAPSTATE_BITMAPS 11
static IRes *gWeapStateHnds[NUM_WEAPSTATE_BITMAPS];

#define NUM_IFACE_BUTTONS  6
extern Rect iface_rects[NUM_IFACE_BUTTONS];

//--------------------------------------------------------------------------------------
// Ammo indicator initializer
//--------------------------------------------------------------------------------------
#define AMMO_MODE_DX (166)
#define AMMO_X1      (544)
#define AMMO_Y1      (414)
#define AMMO_X2      (AMMO_X1 + 94)
#define AMMO_Y2      (AMMO_Y1 + 64)

#define AMMOTYPE1_X1 (200)
#define AMMOTYPE1_Y1 (14)
#define AMMOTYPE1_X2 (AMMOTYPE1_X1 + 49)
#define AMMOTYPE1_Y2 (AMMOTYPE1_Y1 + 41)

#define AMMOTYPE2_X1 (177)
#define AMMOTYPE2_Y1 (AMMOTYPE1_Y1)
#define AMMOTYPE2_X2 (AMMOTYPE2_X1 + 72)
#define AMMOTYPE2_Y2 (AMMOTYPE1_Y2)

Rect ammo_rect      = {{AMMO_X1, AMMO_Y1},{AMMO_X2, AMMO_Y2}};
Rect ammotype_rect  = {{AMMOTYPE1_X1, AMMOTYPE1_Y1}, {AMMOTYPE1_X2, AMMOTYPE1_Y2}};
Rect ammotype_rect2 = {{AMMOTYPE2_X1, AMMOTYPE2_Y1}, {AMMOTYPE2_X2, AMMOTYPE2_Y2}};

void DPCAmmoInit(int which)
{
    gHndAmmoBack  = LoadPCX("ammoback");
    gHndAmmoBack2 = LoadPCX("ammofull");

    //int i;
    char temp[255];

#if 0
    for (i=0; i < NUM_WEAPSTATE_BITMAPS; i++)
    {
        sprintf(temp,"wstate%d",i+1);
        gWeapStateHnds[i] = LoadPCX(temp);
    }
#endif
    sprintf(temp,"wstate%d",NUM_WEAPSTATE_BITMAPS);
    gWeapStateHnds[NUM_WEAPSTATE_BITMAPS-1] = LoadPCX(temp);

#ifdef SHOW_AMMO_UI
    setting_handles[0] = LoadPCX("ammo0"); 
    setting_handles[1] = LoadPCX("ammo1");
    setting_bitmaps[0] = (grs_bitmap *) setting_handles[0]->Lock();
    setting_bitmaps[1] = (grs_bitmap *) setting_handles[1]->Lock();
    for (i = 2; i < 4; i++)
    {
        setting_bitmaps[i] = setting_bitmaps[0];
    }

    cycle_handles[0] = LoadPCX("ammoarw0"); 
    cycle_handles[1] = LoadPCX("ammoarw1"); 
    cycle_bitmaps[0] = (grs_bitmap *) cycle_handles[0]->Lock();
    cycle_bitmaps[1] = (grs_bitmap *) cycle_handles[1]->Lock();
    for (i = 2; i < 4; i++)
    {
        cycle_bitmaps[i] = cycle_bitmaps[0];
    }

    reload_handles[0] = LoadPCX("ammo0"); 
    reload_handles[1] = LoadPCX("ammo1"); 
    reload_bitmaps[0] = (grs_bitmap *) reload_handles[0]->Lock();
    reload_bitmaps[1] = (grs_bitmap *) reload_handles[1]->Lock();
    for (i = 2; i < 4; i++)
    {
        reload_bitmaps[i] = reload_bitmaps[0];
    }
#endif // 0

    Rect use_rect;
    sScrnMode smode;
    ScrnModeGet(&smode);
    use_rect.ul.x = smode.w - (640 - ammo_rect.ul.x) - AMMO_MODE_DX;
    use_rect.ul.y = smode.h - (480 - ammo_rect.ul.y);
    use_rect.lr.x = use_rect.ul.x + RectWidth(&ammo_rect) + AMMO_MODE_DX;
    use_rect.lr.y = use_rect.ul.y + RectHeight(&ammo_rect);

    DPCOverlaySetRect(which,use_rect);
}

//--------------------------------------------------------------------------------------
// Main Meters shutdown
//--------------------------------------------------------------------------------------
void DPCAmmoTerm(void)
{
    SafeFreeHnd(&gHndAmmoBack);
    SafeFreeHnd(&gHndAmmoBack2);

#ifdef SHOW_AMMO_UI
    cycle_handles[0]->Unlock();
    cycle_handles[1]->Unlock();
    SafeFreeHnd(&cycle_handles[0]);
    SafeFreeHnd(&cycle_handles[1]);

    setting_handles[0]->Unlock();
    setting_handles[1]->Unlock();
    SafeFreeHnd(&setting_handles[0]);
    SafeFreeHnd(&setting_handles[1]);

    reload_handles[0]->Unlock();
    reload_handles[1]->Unlock();
    SafeFreeHnd(&reload_handles[0]);
    SafeFreeHnd(&reload_handles[1]);
#endif // SHOW_AMMO_UI

#if 0
    for (int n=0; n < NUM_WEAPSTATE_BITMAPS; n++)
    {
        SafeFreeHnd(&gWeapStateHnds[n]);
    }
#endif
    SafeFreeHnd(&gWeapStateHnds[NUM_WEAPSTATE_BITMAPS-1]);
}

//--------------------------------------------------------------------------------------
// Draw main Ammo
//--------------------------------------------------------------------------------------
extern char *setting_headlines[2];

// yow, this is getting really messy
void DPCAmmoDraw(unsigned long inDeltaTicks)
{
    char temp[255];
    ObjID ammotype;
    int wpntype;
    cStr str;
    Rect use_rect;
    int dx, dy,w, h;

    AutoAppIPtr(PlayerGun);
    AutoAppIPtr(GameStrings);
    AutoAppIPtr(DPCPlayer);

    Rect r = DPCOverlayGetRect(kOverlayAmmo);

// @NOTE:  Disabled the ammo background for the demo.
#ifdef SHOW_AMMO_UI
    int mode;
    Point pt;
    pt.x = r.ul.x;
    pt.y = r.ul.y;
    if (!DPC_mouse)
    {
        pt.x = pt.x + AMMO_MODE_DX;
        DrawByHandle(gHndAmmoBack, pt);
    }
    else
    {
        DrawByHandle(gHndAmmoBack2, pt);
    }
#endif // SHOW_AMMO_UI

    if (gPrevGun == OBJ_NULL)
    {
        return;
    }

    // find out what kind of weapon it is
    g_pWeaponTypeProperty->Get(gPrevGun,&wpntype);

    if (g_pMeleeTypeProperty->IsRelevant(gPrevGun))
    {
        // we need some sort of melee-ish icon here
    }
    else
    {
        // otherwise, draw the current ammo 
        ammotype = GetProjectile(gPrevGun);
        // does this want to become some sort of icon representation 
        // rather than the literal objicon?

        if (DPC_mouse)
        {
            RectOffsettedRect(&ammotype_rect, r.ul, &use_rect);
        }
        else
        {
            RectOffsettedRect(&ammotype_rect2, r.ul, &use_rect);
        }

        dx = use_rect.ul.x + (RectWidth(&use_rect) - INV_ICON_WIDTH) / 2;
        dy = use_rect.ul.y + (RectHeight(&use_rect) - INV_ICON_HEIGHT) / 2;
        DPCInvObjDraw(ammotype,dx,dy);

        str = pGameStrings->FetchObjString(ammotype,PROP_OBJSHORTNAME_NAME);
        strcpy(temp,str);
        w  = gr_font_string_width(gDPCFont, temp);
        h  = gr_font_string_height(gDPCFont, temp);
        dx = use_rect.ul.x + (RectWidth(&use_rect) - w) / 2;
        dy = use_rect.lr.y - h;
        gr_font_string(gDPCFont, temp, dx, dy);

        dx = use_rect.ul.x + 2;
        dy = use_rect.ul.y + 2;
        sprintf(temp,"%d",GunGetAmmoCount(gPrevGun));
        gr_font_string(gDPCFont, temp, dx, dy);

#if 0
        Point pt;
        int n;
        eObjState state = ObjGetObjState(gPrevGun);
        if (state != kObjStateNormal)
        
        {
            n = 10;  // use last one for invalid
        }
        else
        {
            n = ((100 - int(GunGetCondition(gPrevGun)) - 1) / 10);
        }
        if (n < 0)
        {
            n = 0;
        }
        if (n >= 10 )
        {
            n = 9;
        }
        DrawByHandle(gWeapStateHnds[n],pt);
#endif

        Point pt;
        pt.x = r.ul.x + 236;
        pt.y = r.ul.y + 15;
        eObjState state = ObjGetObjState(gPrevGun);
        if (state != kObjStateNormal) {
           DrawByHandle(gWeapStateHnds[NUM_WEAPSTATE_BITMAPS-1],pt);
        } 

#ifdef SHOW_AMMO_UI
        if (DPC_mouse)
        {
            LGadDrawBox(VB(&setting_button),NULL);

            if (wpntype != kWeaponEnergy)
            {
                LGadDrawBox(VB(&cycle_button),NULL);
                LGadDrawBox(VB(&reload_button),NULL);
            }

            mode = GunGetSetting(gPrevGun);
            str = pGameStrings->FetchObjString(gPrevGun,setting_headlines[mode]);
            strcpy(temp,str);
            w = gr_font_string_width(gDPCFont,temp);
            h = gr_font_string_height(gDPCFont,temp);
            dx = r.ul.x + setting_rect.ul.x + (RectWidth(&setting_rect) - w) / 2;
            dy = r.ul.y + setting_rect.ul.y +(RectHeight(&setting_rect) - h) / 2;
            gr_font_string(gDPCFont, temp, dx, dy);

            if (wpntype != kWeaponEnergy)
            {
                DPCStringFetch(temp,sizeof(temp),"Reload","misc");
                w = gr_font_string_width(gDPCFont,temp);
                h = gr_font_string_height(gDPCFont,temp);
                dx = r.ul.x + reload_rect.ul.x + (RectWidth(&reload_rect) - w) / 2;
                dy = r.ul.y + reload_rect.ul.y +(RectHeight(&reload_rect) - h) / 2;
                gr_font_string(gDPCFont, temp, dx, dy);
            }
            // mouseover help
            Point mpt;
            mouse_get_xy(&mpt.x,&mpt.y);
            mpt.x = mpt.x - r.ul.x;
            mpt.y = mpt.y - r.ul.y;
            
            if (RectTestPt(&cycle_rect,mpt) && (wpntype != kWeaponEnergy))
            {
                DPCStringFetch(gHelpString,sizeof(gHelpString),"MouseHelpAmmoCycle", "misc");
            }
            else if (RectTestPt(&setting_rect,mpt))
            {
                DPCStringFetch(gHelpString,sizeof(gHelpString),"MouseHelpSettings", "misc");
            }
            else if (RectTestPt(&reload_rect,mpt) && (wpntype != kWeaponEnergy))
            {
                DPCStringFetch(gHelpString,sizeof(gHelpString),"MouseHelpReload", "misc");
            }
        }
#endif // SHOW_AMMO_UI
    }
}

//--------------------------------------------------------------------------------------
// Basic Meters handling.  Also covers the inventory popup by virtue of
// calling the InvFindObjSlot which covers both quick slots & real slots
//--------------------------------------------------------------------------------------
bool DPCAmmoHandleMouse(Point pt)
{
    int wpntype;

    if (DPC_cursor_mode == SCM_DRAGOBJ)
    {
        DPCInvAddObj(PlayerObject(),drag_obj);
        return(TRUE);
    }

    if (gPrevGun == OBJ_NULL)
        return(TRUE);

    g_pWeaponTypeProperty->Get(gPrevGun,&wpntype);

    /*
    if (!RectTestPt(&cycle_rect,pt) && !RectTestPt(&setting_rect,pt) && (pt.x >= cycle_rect.ul.x))
    {
       CommandExecute("reload_gun");
    }
    */
    return(TRUE);
}
//--------------------------------------------------------------------------------------
// Check for transparency in the source art
//--------------------------------------------------------------------------------------
bool DPCAmmoCheckTransp(Point p)
{
    // hm, perhaps this wants to just always return false so that 
    // we don't deal with the problems of the interlacing?
    return(FALSE);
    /*
    bool retval = FALSE;
    int pix = HandleGetPix(gHndAmmoBack,p);
    if (pix == 0)
       retval = TRUE;
    //mprintf("SICT: %d (%d)\n",retval,pix);
    return(retval);
    */
}

//--------------------------------------------------------------------------------------
void DPCAmmoReload(void)
{
    sContainIter *scip;
    AutoAppIPtr(ContainSys);
    AutoAppIPtr(PlayerGun);
    ObjID obj = PlayerObject();

    // no reloading if dead
    if (GetPlayerMode() == kPM_Dead)
        return;

    // no reloading if no interface
    BOOL slim_mode;
    AutoAppIPtr(QuestData);
    slim_mode = pQuestData->Get("HideInterface");
    if (slim_mode)
        return;

    // no reloading if a melee weapon
    if (pPlayerGun->Get() == OBJ_NULL)
        return;

    // no reloading if an energy weapon
    int wpntype;
    g_pWeaponTypeProperty->Get(pPlayerGun->Get(),&wpntype);
    if (wpntype == kWeaponEnergy)
        return;

    // if we have a psi amp, then do something completely different
    // scan through all the items in the player's inventory
    scip = pContainSys->IterStart(obj); // ,0,DPCCONTAIN_PDOLLBASE - 1);
    while (!scip->finished)
    {
        if (pPlayerGun->MatchesCurrentAmmo(scip->containee) && !IsPlayerGun(scip->containee))
        {
            GunLoad(pPlayerGun->Get(), scip->containee, LF_AUTOPLACE);
            pContainSys->IterEnd(scip);
            DPCInvReset(); // do a full recompute
            return;
        }
        pContainSys->IterNext(scip);
    }
    // nope, didn't find it...
    pContainSys->IterEnd(scip);

    // give some feedback
    char temp[255];
    DPCStringFetch(temp,sizeof(temp),"CantReload","misc");
    DPCOverlayAddText(temp,DEFAULT_MSG_TIME);
}
//--------------------------------------------------------------------------------------
// find the next ammo type in inventory, and load it up
void DPCAmmoChangeTypes(void)
{
    ObjID curgun;
    ObjID arch;
    sContainIter *scip;
    AutoAppIPtr(ContainSys);
    AutoAppIPtr(PlayerGun);
    AutoAppIPtr(TraitManager);

    ObjID obj = PlayerObject();

    // no reloading if dead
    if (GetPlayerMode() == kPM_Dead)
    {
       return;
    }

    // scan through all the items in the player's inventory
    if ((curgun = pPlayerGun->Get()) == OBJ_NULL)
    {
       return;
    }

    // find out what kind of weapon it is
    int wpntype;
    g_pWeaponTypeProperty->Get(curgun,&wpntype);

    ObjID origproj = GetProjectile(curgun);
    ObjID nextproj = GetNextProjectile(curgun, origproj);
    while (nextproj != origproj)
    {
        scip = pContainSys->IterStart(obj); // ,0,DPCCONTAIN_PDOLLBASE - 1);
        while (!scip->finished)
        {
            arch = GetProjectileFromClip(scip->containee, curgun);
            if ((arch == nextproj) && !IsPlayerGun(scip->containee))
            {
                GunLoad(pPlayerGun->Get(), scip->containee, LF_JUGGLE);
                pContainSys->IterEnd(scip);
                DPCInvReset(); // do a full recompute
                return;
            }
            pContainSys->IterNext(scip);
        }
        // nope, didn't find it...
        pContainSys->IterEnd(scip);
        // go to the next in line
        // if we are cycling around, that will bump us out of the loop
        nextproj = GetNextProjectile(curgun, nextproj);
    }
}
//--------------------------------------------------------------------------------------
static bool cycle_cb(short action, void* data, LGadBox* vb)
{
    if (action != BUTTONGADG_LCLICK)
    {
       return(FALSE);
    }

    AutoAppIPtr(PlayerGun);
    ObjID gun = pPlayerGun->Get();
    int wpntype;
    g_pWeaponTypeProperty->Get(gun,&wpntype);
    if (wpntype == kWeaponEnergy)
    {
       return(FALSE);
    }

    SchemaPlay((Label *)"bammo",NULL);
    CommandExecute("cycle_ammo");
    return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool setting_cb(short action, void* data, LGadBox* vb)
{
    if (action != BUTTONGADG_LCLICK)
        return(FALSE);
    SchemaPlay((Label *)"bset",NULL);
    CommandExecute("wpn_setting_toggle");
    return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool reload_cb(short action, void* data, LGadBox* vb)
{
    if (action != BUTTONGADG_LCLICK)
        return(FALSE);

    AutoAppIPtr(PlayerGun);
    ObjID gun = pPlayerGun->Get();
    int wpntype;
    g_pWeaponTypeProperty->Get(gun,&wpntype);
    if (wpntype == kWeaponEnergy)
        return(FALSE);

    SchemaPlay((Label *)"bammo",NULL);
    CommandExecute("reload_gun");
    return(TRUE);
}
//--------------------------------------------------------------------------------------

static void BuildInterfaceButtons(void)
{
    g_interface = TRUE;

    AutoAppIPtr(PlayerGun);
    gPrevGun = pPlayerGun->Get();
    int wpntype;
    g_pWeaponTypeProperty->Get(gPrevGun,&wpntype);

    if (gPrevGun == OBJ_NULL)
    {
        return;
    }

#ifdef SHOW_AMMO_UI
    Rect r = DPCOverlayGetRect(kOverlayAmmo);
    cycle_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
    cycle_elem.draw_data = cycle_bitmaps;
    cycle_elem.draw_data2 = (void *)4; // should be 2 but hackery required

    LGadCreateButtonArgs(&cycle_button, LGadCurrentRoot(), cycle_rect.ul.x + r.ul.x, 
                         cycle_rect.ul.y + r.ul.y,
                         RectWidth(&cycle_rect), RectHeight(&cycle_rect), &cycle_elem, cycle_cb, 0);

    setting_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
    setting_elem.draw_data = setting_bitmaps;
    setting_elem.draw_data2 = (void *)4; // should be 2 but hackery required

    LGadCreateButtonArgs(&setting_button, LGadCurrentRoot(), setting_rect.ul.x + r.ul.x, 
                         setting_rect.ul.y + r.ul.y,
                         RectWidth(&setting_rect), RectHeight(&setting_rect), &setting_elem, setting_cb, 0);

    reload_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
    reload_elem.draw_data = reload_bitmaps;
    reload_elem.draw_data2 = (void *)4; // should be 2 but hackery required

    LGadCreateButtonArgs(&reload_button, LGadCurrentRoot(), reload_rect.ul.x + r.ul.x, 
                         reload_rect.ul.y + r.ul.y,
                         RectWidth(&reload_rect), RectHeight(&reload_rect), &reload_elem, reload_cb, 0);
#endif // SHOW_AMMO_UI
}
//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
    int wpntype;
    if (gPrevGun == OBJ_NULL)
    {
       return;
    }

    if (g_interface == FALSE)
    {
       return;
    }

    g_pWeaponTypeProperty->Get(gPrevGun,&wpntype);

#ifdef SHOW_AMMO_UI
    LGadDestroyBox(VB(&cycle_button),   FALSE);
    LGadDestroyBox(VB(&setting_button), FALSE);
    LGadDestroyBox(VB(&reload_button),  FALSE);
#endif // SHOW_AMMO_UI
    gPrevGun = OBJ_NULL;
    g_interface = FALSE;
}
//--------------------------------------------------------------------------------------
void DPCAmmoRefreshButtons(void)
{
    if (DPCOverlayCheck(kOverlayAmmo))
    {
        DestroyInterfaceButtons();
        BuildInterfaceButtons();
    }
}
//--------------------------------------------------------------------------------------
void DPCAmmoStateChange(int which)
{
    if (DPCOverlayCheck(which))
    {
        // just got put up
        BuildInterfaceButtons();
    }
    else
    {
        DestroyInterfaceButtons();
    }
}
//--------------------------------------------------------------------------------------
