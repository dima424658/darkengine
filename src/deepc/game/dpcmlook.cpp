/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <2d.h>

#include <resapi.h>

#include <dpcovrly.h>
#include <dpcovcst.h>
#include <dpcutils.h>
#include <dpcgame.h>
#include <dpcifstr.h>

#include <scrnmode.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
}

static IRes *gButtBack;

static Rect full_rect = {{281,434},{281 + 78, 434 + 44}};
static Rect close_rect = {{23,3},{23 + 32, 3 + 40}};

static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

//--------------------------------------------------------------------------------------
void DPCMouseModeInit(int )
{
// @NOTE:  TEMPCODE
#if 0
    int i;

    close_handles[0] = LoadPCX("ML0"); 
    close_handles[1] = LoadPCX("ML1"); 
    close_bitmaps[0] = (grs_bitmap *) close_handles[0]->Lock();
    close_bitmaps[1] = (grs_bitmap *) close_handles[1]->Lock();
    for (i = 2; i < 4; i++)
    {
        close_bitmaps[i] = close_bitmaps[0];
    }

    gButtBack = LoadPCX("MICELOOK");

    Rect use_rect;
    sScrnMode smode;
    ScrnModeGet(&smode);

    if (smode.w > 640)
    {
        use_rect.ul.x = (smode.w - RectWidth(&full_rect)) / 2;
        use_rect.ul.y = smode.h - RectHeight(&full_rect);
        use_rect.lr.x = use_rect.ul.x + RectWidth(&full_rect);
        use_rect.lr.y = use_rect.ul.y + RectHeight(&full_rect);
        DPCOverlaySetRect(kOverlayMouseMode,use_rect);
    }
    else
        DPCOverlaySetRect(kOverlayMouseMode,full_rect);
#endif // 0
}

//--------------------------------------------------------------------------------------
void DPCMouseModeTerm(void)
{
// @NOTE:  TEMPCODE
#if 0
    SafeFreeHnd(&gButtBack);

    close_handles[0]->Unlock();
    close_handles[1]->Unlock();
    SafeFreeHnd(&close_handles[0]);
    SafeFreeHnd(&close_handles[1]);
#endif // 0
}

//--------------------------------------------------------------------------------------
void DPCMouseModeDraw(unsigned long inDeltaTicks)
{
// @NOTE:  TEMPCODE
#if 0
    Rect r = DPCOverlayGetRect(kOverlayMouseMode);

    DrawByHandle(gButtBack,r.ul);

    LGadDrawBox(VB(&close_button),NULL);

    Point mpt;
    mouse_get_xy(&mpt.x,&mpt.y);
    if (RectTestPt(&r,mpt))
        DPCStringFetch(gHelpString,sizeof(gHelpString),"MouseHelpMouseMode","misc");
#endif // 0
}
//--------------------------------------------------------------------------------------
void DeferredMouseMode(void *)
{
    MouseMode(FALSE,TRUE);
}
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
    uiDefer(DeferredMouseMode,NULL);
    return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
// @NOTE:  TEMPCODE
#if 0
    Rect r = DPCOverlayGetRect(kOverlayMouseMode);

    close_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
    close_elem.draw_data = close_bitmaps;
    close_elem.draw_data2 = (void *)4; // should be 2 but hackery required

    LGadCreateButtonArgs(&close_button, LGadCurrentRoot(), close_rect.ul.x + r.ul.x, close_rect.ul.y + r.ul.y,
                         RectWidth(&close_rect), RectHeight(&close_rect), &close_elem, close_cb, 0);
#endif // 0
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
#if 0
    LGadDestroyBox(VB(&close_button),FALSE);
#endif
}
//--------------------------------------------------------------------------------------
void DPCMouseModeStateChange(int which)
{
    if (DPCOverlayCheck(which))
    {
        BuildInterfaceButtons();
    }
    else
    {
        DestroyInterfaceButtons();
    }
}
//--------------------------------------------------------------------------------------

sOverlayFunc OverlayMouseMode =
{ 
    DPCMouseModeDraw,          // draw
    DPCMouseModeInit,          // init
    DPCMouseModeTerm,          // term
    NULL,                      // mouse
    NULL,                      // dclick (really use)
    NULL,                      // dragdrop
    NULL,                      // key
    NULL,                      // bitmap
    "",                        // upschema
    "",                        // downschema
    DPCMouseModeStateChange,   // state
    NULL,                      // transparency
    0,                         // distance
    TRUE,                      // needmouse
};
