/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <2d.h>
#include <appagg.h>

#include <res.h>
#include <guistyle.h>

#include <resapi.h>
#include <mprintf.h>
#include <scrnmode.h>

#include <playrobj.h>
#include <objhp.h>
#include <plyrmode.h>

#include <netman.h>    // for IsNetworkGame()

#include <dpcgame.h>
#include <dpcutils.h>
#include <dpcovrly.h>
#include <dpcovcst.h>
#include <dpccurm.h>
#include <dpcinv.h>
#include <dpcplayr.h>
#include <dpcplcst.h>
#include <dpclooko.h>
#include <dpcifstr.h>

#include <dpcmeter.h>

#include <vismeter.h>   // Visibility meter.

IRes *gHndHealth  = NULL;
IRes *gHndBio     = NULL;
IRes *gHndBioFull = NULL;

//--------------------------------------------------------------------------------------
// HP Meter initializer
//--------------------------------------------------------------------------------------
#define BIO_X  2
#define BIO_Y  414
Rect meters_rect = {{BIO_X,BIO_Y},{BIO_X + 260,BIO_Y+ 64}}; // 128

void DPCMetersInit(int which)
{
    gHndHealth  = LoadPCX("hpbar");
    gHndBio     = LoadPCX("bio");
    gHndBioFull = LoadPCX("biofull");
    AssertMsg(gHndHealth,   "DPCMetersInit:  Missing health bar art");
    AssertMsg(gHndBio,      "DPCMetersInit:  Missing bio art");
    AssertMsg(gHndBioFull,  "DPCMetersInit:  Missing bio full art");

    Rect use_rect;
    sScrnMode smode;
    ScrnModeGet(&smode);
    use_rect.ul.x = meters_rect.ul.x;
    use_rect.ul.y = smode.h - (480 - meters_rect.ul.y);
    use_rect.lr.x = use_rect.ul.x + RectWidth(&meters_rect);
    use_rect.lr.y = use_rect.ul.y + RectHeight(&meters_rect);

    DPCOverlaySetRect(which,use_rect);

    // @NOTE:  Visibility meter code
    VisMeterEnterMode();
}

//--------------------------------------------------------------------------------------
// Main Meters shutdown
//--------------------------------------------------------------------------------------
void DPCMetersTerm(void)
{
    SafeFreeHnd(&gHndHealth);
    SafeFreeHnd(&gHndBio);
    SafeFreeHnd(&gHndBioFull);

    // @NOTE:  Visibility meter code
    VisMeterExitMode();
}

//--------------------------------------------------------------------------------------
// Draw health meters.  
//--------------------------------------------------------------------------------------
#define HP_X     8
#define HP_Y     18
#define HPTEXT_X 92
#define HPTEXT_Y 18

// @TODO: compute top and bottom margins for real
static float gStatusH      = 0.25;		  // %of screen
static float gStatusMargin = 4.0f/480.0f; // %of screen
static int	 air_x[2]      = {655, 644};
static int	 hp_x[2]       = {1,   505};

static void DrawMeters(unsigned long inDeltaTicks)
{
#if 0
   char temp[255];
    Rect r = DPCOverlayGetRect(kOverlayMeters);

    // set initial vals in case we don't have relevant properties
    int hp    = 0;
    int hpmax = 0;

    strcpy(temp,"");

    if (gHndHealth == NULL)
    {
        //WARNING(("Could not get meter art!\n"));
        return;
    }

    // Determine the percentages.
    ObjID po = PlayerObject();
    ObjGetHitPoints(po, &hp);
    if (hp < 0)
    {
       hp = 0;
    }
    if (GetPlayerMode() == kPM_Dead)
    {
       hp = 0;
    }
    ObjGetMaxHitPoints(po, &hpmax);

    grs_clip saveClip = grd_gc.clip;                 // Clip it good!

    grd_gc.clip = saveClip;
    gr_set_fcolor(gDPCTextColor);

    grs_bitmap *bm = (grs_bitmap *) gHndHealth->Lock();

    int clippix = 0;
    if ((hp == 0) || (hpmax == 0))
    {
        clippix = 0;
    }
    else
    {
        clippix = (bm->w * hp / hpmax);
    }
    
    if (clippix < 0)
    {
        clippix = 0;
    }

    gr_safe_set_cliprect(HP_X + r.ul.x,HP_Y + r.ul.y,HP_X + r.ul.x + clippix,HP_Y + r.ul.y + bm->h);
    gr_bitmap(bm, HP_X + r.ul.x, HP_Y + r.ul.y);
    gHndHealth->Unlock();

    AssertMsg(gDPCFont, "DrawMeters:  No font!");
    grd_gc.clip = saveClip;
    sprintf(temp,"%d", hp); //  / %d",hp,hpmax); 
    gr_font_string(gDPCFont,temp, HPTEXT_X + r.ul.x, HPTEXT_Y + r.ul.y);
#endif // 0

    // @NOTE:  Visibility meter code
    int bot = (int)(grd_canvas->bm.h * (1.0f - gStatusMargin));
    
    if (bot > grd_canvas->bm.h - 1)
    {
        bot = grd_canvas->bm.h -1;
    }
    
    int top = bot-(int)(grd_canvas->bm.h*gStatusH);
//  @ render_stat_bars(msec,top,bot);
    
    // compute rect for vis meter
    enum {kExtentX = 640, kExtentY = 480};
    Rect vis_r = { 0, top, 0, bot};
    vis_r.ul.x = hp_x[1]  * grd_canvas->bm.w/kExtentX;
    vis_r.lr.x = air_x[0] * grd_canvas->bm.w/kExtentX;

    VisMeterUpdate(inDeltaTicks, &vis_r);
}

//--------------------------------------------------------------------------------------
// Draw main Meters
//--------------------------------------------------------------------------------------
void DPCMetersDraw(unsigned long inDeltaTicks)
{
#if 0
   // @Note:  Draws the bio meter.
   IRes *useres = (!DPC_mouse) ? gHndBio : gHndBioFull;

    grs_bitmap *bm = (grs_bitmap *)useres->Lock();
    Rect r = DPCOverlayGetRect(kOverlayMeters);
    gr_bitmap(bm, r.ul.x, r.ul.y);
    useres->Unlock();

    Point mpt;
    mouse_get_xy(&mpt.x,&mpt.y);
    if (RectTestPt(&r,mpt))
    {
        if (mpt.x - r.ul.x  < HPTEXT_X)
        {
            DPCStringFetch(gHelpString, sizeof(gHelpString), "MouseHelpHP", "misc");
        }
    }
#endif // 0
    DrawMeters(inDeltaTicks);
}

//--------------------------------------------------------------------------------------
// Basic Meters handling.  Also covers the inventory popup by virtue of
// calling the InvFindObjSlot which covers both quick slots & real slots
//--------------------------------------------------------------------------------------
bool DPCMetersHandleMouse(Point pt)
{
    bool retval = TRUE;

    if (DPC_cursor_mode == SCM_DRAGOBJ)
        DPCInvAddObj(PlayerObject(),drag_obj);

    AutoAppIPtr(DPCPlayer);

    if (DPC_cursor_mode == SCM_LOOK)
    {
        ObjID fakeobj = OBJ_NULL;
        extern Rect fake_rects[4];
        Rect r = DPCOverlayGetRect(kOverlayTicker);
        Rect r2 = DPCOverlayGetRect(kOverlayMeters);
        Point usept;
        usept.x = pt.x + r2.ul.x;
        usept.y = pt.y + r2.ul.y;

        // okay, we have to fixup the fake_rects for high resolution positioning, thus the offset
        Rect userect;
        Point offset;
        offset.x = 0;
        offset.y = r.ul.y;

        // query nanites?
        RectOffsettedRect(&fake_rects[0], offset, &userect);
        if (RectTestPt(&userect,usept))
        {
            fakeobj = pDPCPlayer->GetEquip(PlayerObject(), kEquipFakeNanites);
        }

        // query cookies?
        RectOffsettedRect(&fake_rects[1], offset, &userect);
        if (RectTestPt(&userect,usept))
        {
            fakeobj = pDPCPlayer->GetEquip(PlayerObject(), kEquipFakeCookies);
        }

        if (fakeobj != OBJ_NULL)
        {
            DPCLookPopup(fakeobj);
            ClearCursor();
        }
    }

    return(retval);
}
//--------------------------------------------------------------------------------------
// Handle drag/drop. This is mainly intended so we can split nanites.
// It only works in multiplayer, since it's kinda pointless in SP.
extern void DPCSplitStack(ObjID o, BOOL leaveEmpty);
bool DPCMetersDragDrop(Point pt, BOOL start)
{
    AutoAppIPtr(NetManager);
    if (!pNetManager->IsNetworkGame())
        return(TRUE);

    // For now, this only deals with the case where the player wants to
    // split his nanites or cookies
    if ((DPC_cursor_mode == SCM_SPLIT) && start)
    {
        AutoAppIPtr(DPCPlayer);
        extern Rect fake_rects[4];
        Point usept;
        Rect r = DPCOverlayGetRect(kOverlayTicker);
        Rect r2 = DPCOverlayGetRect(kOverlayMeters);

        usept.x = pt.x + r2.ul.x;
        usept.y = pt.y + r2.ul.y;

        // okay, we have to fixup the fake_rects for high resolution positioning, thus the offset
        Rect userect;
        Point offset;
        offset.x = 0;
        offset.y = r.ul.y;

        // query nanites?
        RectOffsettedRect(&fake_rects[0], offset, &userect);

        if (RectTestPt(&userect, usept))
        {
            ObjID o = pDPCPlayer->GetEquip(PlayerObject(), kEquipFakeNanites);
            ClearCursor();
            if (o != OBJ_NULL)
            {
                DPCSplitStack(o, TRUE);
            }
        }
    }
    return(TRUE);
}

//--------------------------------------------------------------------------------------
// Check for transparency in the source art
//--------------------------------------------------------------------------------------
bool DPCMetersCheckTransp(Point p)
{
    /*
    bool retval = FALSE;
    int pix = HandleGetPix(gHndBio,p);
    if (pix == 0)
       retval = TRUE;
    //mprintf("SICT: %d (%d)\n",retval,pix);
    return(retval);
    */
    return(FALSE);
}
