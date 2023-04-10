/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Radiation meter
#include <2d.h>

#include <resapi.h>
#include <playrobj.h>
#include <scrnmode.h>
#include <mprintf.h>

#include <schema.h>

#include <dpchazrd.h>
#include <dpcovrly.h>
#include <dpcovcst.h>
#include <dpcutils.h>
#include <dpchazpr.h>

IRes *gRadMeter = NULL;
IRes *gRadBack  = NULL;
IRes *gRadIcon  = NULL;
IRes *gRadGray  = NULL;

static Rect rad_rect = {{10,379},{10 + 128, 379 + 32}};
//--------------------------------------------------------------------------------------
void DPCRadInit(int which)
{
// @NOTE:  This stuff is here only for reference purposes.  - Bodisafa
#if 0
    gRadMeter = LoadPCX("radmeter");
    gRadBack  = LoadPCX("radback");
    gRadIcon  = LoadPCX("radicon");
    gRadGray  = LoadPCX("radgray");

    Rect use_rect;
    sScrnMode smode;
    ScrnModeGet(&smode);

    use_rect.ul.x = rad_rect.ul.x;
    use_rect.ul.y = smode.h - (480 - rad_rect.ul.y);
    use_rect.lr.x = use_rect.ul.x + RectWidth(&rad_rect);
    use_rect.lr.y = use_rect.ul.y + RectHeight(&rad_rect);

    DPCOverlaySetRect(which,use_rect);
    DPCOverlaySetFlags(which, kOverlayFlagBufferTranslucent);
#endif // 0
}

//--------------------------------------------------------------------------------------
void DPCRadTerm(void)
{
// @NOTE:  This stuff is here only for reference purposes.  - Bodisafa
#if 0
    SafeFreeHnd(&gRadBack);
    SafeFreeHnd(&gRadMeter);
    SafeFreeHnd(&gRadIcon);
    SafeFreeHnd(&gRadGray);
#endif // 0
}

//--------------------------------------------------------------------------------------
#define RAD_X  33
#define RAD_Y  0
void DPCRadDraw(unsigned long inDeltaTicks)
{
// @NOTE:  This stuff is here only for reference purposes.  - Bodisafa
#if 0
    //Point p;
    ObjID obj = PlayerObject();
    Rect r = { {0,0} , {0,0} }; // DPCOverlayGetRect(kOverlayRadiation);
    float radval;
    if (!gPropRadLevel->Get(obj, &radval))
    {
        radval = 0;
    }

    float maxval = 35.0f; // how to determine for real??

    // fill in the meter from the left
    grs_bitmap *bm = (grs_bitmap *) gRadMeter->Lock();
    grs_clip saveClip = grd_gc.clip;                 // Clip it good!

    int clippix = 0;
    if ((radval != 0) && (maxval != 0))
    {
        clippix = ((bm->w - RAD_X) * radval / maxval);
    }
    if (clippix < 0)
    {
        clippix = 0;
    }

    gr_safe_set_cliprect(r.ul.x + RAD_X, r.ul.x + RAD_Y, r.ul.x + RAD_X + clippix, r.ul.y + RAD_Y + bm->h);
    gr_bitmap(bm, r.ul.x + RAD_X, r.ul.y + RAD_Y);
    gRadMeter->Unlock();

    grd_gc.clip = saveClip;

    // draw the framing element
    /*
    p.x = r.ul.x + RAD_X;
    p.y = r.ul.y + RAD_Y;
    DrawByHandle(gRadBack,p);
    */

    // draw the icon to the side
    float radamb = 0;
    gPropRadAmbient->Get(PlayerObject(),&radamb);

    if (radamb == 0)
    {
        DrawByHandle(gRadGray,r.ul);
    }
    else
    {
        DrawByHandle(gRadIcon,r.ul);
    }

#endif // 0
}
//--------------------------------------------------------------------------------------
IRes *DPCRadBitmap(void)
{
    return(gRadBack);
}
//--------------------------------------------------------------------------------------
sOverlayFunc OverlayRad = 
{ 
    DPCRadDraw,               // draw
    DPCRadInit,               // init
    DPCRadTerm,               // term
    NULL,                     // mouse
    NULL,                     // dclick (really use)
    NULL,                     // dragdrop
    NULL,                     // key
    DPCRadBitmap,             // bitmap
    "",                       // upschema
    "",                       // downschema
    NULL,                     // state
    DPCOverlayFullTransp,     // transparency
    0,                        // distance
    FALSE,                    // needmouse
    180,                      // alpha
};

