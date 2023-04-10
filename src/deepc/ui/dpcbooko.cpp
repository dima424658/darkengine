/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <2d.h>

#include <resapi.h>

#include <dpcbooko.h>
#include <dpcovrly.h>
#include <dpcovcst.h>
#include <dpcutils.h>
#include <dpcemail.h>
#include <dpcmfddm.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
}

static IRes *gCurrBook;
static IRes *gBookBack;

static Rect full_rect = {{LMFD_X, LMFD_Y}, {LMFD_X + LMFD_W, LMFD_Y + LMFD_H}}; 
static Rect close_rect = {{163,8},{163 + 20, 8 + 21}};

static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

static Rect return_rect = {{156,183},{156 + 18, 183 + 73}};
static LGadButton return_button;
static DrawElement return_elem;
static IRes *return_handles[2];
static grs_bitmap *return_bitmaps[4];

//--------------------------------------------------------------------------------------
void DPCBookInit(int )
{
    int i;

    close_handles[0] = LoadPCX("CloseOff"); 
    close_handles[1] = LoadPCX("CloseOn"); 
    AssertMsg(close_handles[0], "DPCBookInit:  Missing Book CloseOff art.");
    AssertMsg(close_handles[1], "DPCBookInit:  Missing Book CloseOn art.");

    close_bitmaps[0] = (grs_bitmap *) close_handles[0]->Lock();
    close_bitmaps[1] = (grs_bitmap *) close_handles[1]->Lock();
    for (i = 2; i < 4; i++)
    {
        close_bitmaps[i] = close_bitmaps[0];
    }
    return_handles[0] = LoadPCX("return0"); 
    return_handles[1] = LoadPCX("return1"); 
    return_bitmaps[0] = (grs_bitmap *) return_handles[0]->Lock();
    return_bitmaps[1] = (grs_bitmap *) return_handles[1]->Lock();
    for (i = 2; i < 4; i++)
    {
        return_bitmaps[i] = return_bitmaps[0];
    }

    gCurrBook = NULL;
    gBookBack = LoadPCX("media");

    DPCOverlaySetRect(kOverlayBook,full_rect);
}

//--------------------------------------------------------------------------------------
void DPCBookTerm(void)
{
    SafeFreeHnd(&gBookBack);
    if (gCurrBook != NULL)
        SafeFreeHnd(&gCurrBook);

    close_handles[0]->Unlock();
    close_handles[1]->Unlock();
    SafeFreeHnd(&close_handles[0]);
    SafeFreeHnd(&close_handles[1]);

    return_handles[0]->Unlock();
    return_handles[1]->Unlock();
    SafeFreeHnd(&return_handles[0]);
    SafeFreeHnd(&return_handles[1]);
}

//--------------------------------------------------------------------------------------
void DPCBookDraw(unsigned long inDeltaTicks)
{
    Rect r = DPCOverlayGetRect(kOverlayBook);
    Point drawpt;

    DrawByHandle(gBookBack,r.ul);

    drawpt = r.ul;
    drawpt.x += 15;
    drawpt.y += 13;
    DrawByHandle(gCurrBook,drawpt);

    LGadDrawBox(VB(&return_button),NULL);
    LGadDrawBox(VB(&close_button),NULL);
}
//--------------------------------------------------------------------------------------
bool DPCBookHandleMouse(Point pos)
{
    return(FALSE);
}
//--------------------------------------------------------------------------------------
void DPCBookDisplay(int level, int which)
{
    char artname[256];
    if (gCurrBook != NULL)
        SafeFreeHnd(&gCurrBook);

    sprintf(artname,"..\\logs\\%s%02d_%d","med",level+1,which);
    DPCEmailPlayAudio(artname);

    sprintf(artname,"book%d_%d",level+1,which);
    gCurrBook = LoadPCX(artname, "book\\");
    DPCOverlayChange(kOverlayBook, kOverlayModeOn);
}
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
    uiDefer(DeferOverlayClose,(void *)kOverlayBook);
    return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool return_cb(short action, void* data, LGadBox* vb)
{
    // close
    uiDefer(DeferOverlayClose,(void *)kOverlayBook);
    // open PDA
    uiDefer(DeferOverlayOpen,(void *)kOverlayPDA);
    return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
    Rect r = DPCOverlayGetRect(kOverlayBook);

    close_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
    close_elem.draw_data = close_bitmaps;
    close_elem.draw_data2 = (void *)4; // should be 2 but hackery required

    LGadCreateButtonArgs(&close_button, LGadCurrentRoot(), close_rect.ul.x + r.ul.x, close_rect.ul.y + r.ul.y,
                         RectWidth(&close_rect), RectHeight(&close_rect), &close_elem, close_cb, 0);

    return_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
    return_elem.draw_data = return_bitmaps;
    return_elem.draw_data2 = (void *)4; // should be 2 but hackery required

    LGadCreateButtonArgs(&return_button, LGadCurrentRoot(), return_rect.ul.x + r.ul.x, return_rect.ul.y + r.ul.y,
                         RectWidth(&return_rect), RectHeight(&return_rect), &return_elem, return_cb, 0);
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
    LGadDestroyBox(VB(&close_button),FALSE);
    LGadDestroyBox(VB(&return_button),FALSE);
}
//--------------------------------------------------------------------------------------
void DPCBookStateChange(int which)
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
