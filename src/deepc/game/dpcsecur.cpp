/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <2d.h>

#include <resapi.h>
#include <appagg.h>

#include <schema.h>
#include <playrobj.h>
#include <mprintf.h>

#include <keyprop.h>

#include <dpcsecur.h>
#include <dpcovrly.h>
#include <dpcovcst.h>
#include <dpcutils.h>
#include <dpcplayr.h>
#include <dpcplprp.h>
#include <dpcplcst.h>
#include <dpcmfddm.h>
#include <dpciftul.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
}

static IRes *gBackHnd;

static Rect full_rect = {{LMFD_X, LMFD_Y}, {LMFD_X + LMFD_W, LMFD_Y + LMFD_H}}; 
static Rect close_rect = {{163,8},{163 + 20, 8 + 21}};

static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

//--------------------------------------------------------------------------------------
void DPCSecurityInit(int which)
{
   close_handles[0] = LoadPCX("CloseOff"); 
   close_handles[1] = LoadPCX("CloseOn"); 
   close_bitmaps[0] = (grs_bitmap *) close_handles[0]->Lock();
   close_bitmaps[1] = (grs_bitmap *) close_handles[1]->Lock();
   for (int i = 2; i < 4; i++)
   {
      close_bitmaps[i] = close_bitmaps[0];
   }

   gBackHnd = LoadPCX("security");
   SetLeftMFDRect(kOverlaySecurity,full_rect);
}

//--------------------------------------------------------------------------------------
void DPCSecurityTerm(void)
{
   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);

   SafeFreeHnd(&gBackHnd);
}

//--------------------------------------------------------------------------------------

#define TEXT_DY   15
#define TEXT_DX   77
#define TEXT_X    17
#define TEXT_Y    14
/*
0    "MED", "SCI", "R&D", "CREW", "CARGO",
5    "REC","HYD A","HYD B","HYD D","SHUTTLE", 
10   "CRYO", "MED ANNEX", "SECURITY", "BRIDGE", "CARGO2",
15   "CREW2", "RICKENBACHER", "RICK ROOM",
*/
int bitshift[32] = { 
   10, 1, 0, 3, 2, 11,
   12, 4, 14, 
   6, 7, 8,
   5, 15,
   13, 19, 9,
   17,

   18,
   16,

   20,21,22,23,24,25,26,27,28,29,30,31,
};

void DPCSecurityDraw(unsigned long inDeltaTicks)
{
    char temp[255];
    int bright_color[3] = { 0, 255, 0};
    int dim_color[3] = { 100, 100, 100};
    int i, bit, access;
    
    Rect  r      = DPCOverlayGetRect(kOverlaySecurity);
    ObjID player = PlayerObject();
    
    AutoAppIPtr(DPCPlayer);
    
    DrawByHandle(gBackHnd,r.ul);
    
    ObjID keyobj = pDPCPlayer->GetEquip(player, kEquipFakeKeys);
    if (keyobj == NULL)
      return;
    
    int dx = TEXT_X + r.ul.x;
    int dy = TEXT_Y + r.ul.y;
    
    if (!KeySrcProp->IsRelevant(keyobj))
    {
        access = 0;
    }
    else
    {
        sKeyInfo *skip;
        KeySrcProp->Get(keyobj, &skip);
        access = skip->region_mask;
    }
    int count = 0;
    for (i = 0; i < 32; i++)
    {
        bit = (1 << bitshift[i]);
        if (access & bit)
        {
            DPCStringFetch(temp,sizeof(temp),"access","misc",bitshift[i]);
            gr_set_fcolor(FindColor(bright_color));
            gr_font_string(gDPCFont, temp, dx, dy);
            count++;
            
            //gr_set_fcolor(FindColor(dim_color));
            if (count == 18)
            {  
                dy = TEXT_Y + r.ul.y;
                dx = dx + TEXT_DX;
            }
            else
            {
                dy = dy + TEXT_DY;
            }
        }
    }
    
    // close button
    LGadDrawBox(VB(&close_button),NULL);
}
//--------------------------------------------------------------------------------------
bool DPCSecurityHandleMouse(Point mpt)
{
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   if (action == BUTTONGADG_LCLICK)
   {
      uiDefer(DeferOverlayClose,(void *)kOverlaySecurity);
      SchemaPlay((Label *)"subpanel_cl",NULL);
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r= DPCOverlayGetRect(kOverlaySecurity);

   close_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   close_elem.draw_data = close_bitmaps;
   close_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&close_button, LGadCurrentRoot(), close_rect.ul.x + r.ul.x, close_rect.ul.y + r.ul.y,
      RectWidth(&close_rect), RectHeight(&close_rect), &close_elem, close_cb, 0);
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   LGadDestroyBox(VB(&close_button),FALSE);
}
//--------------------------------------------------------------------------------------
void DPCSecurityStateChange(int which)
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
bool DPCSecurityCheckTransp(Point pos)
{
   return(FALSE);
}
//--------------------------------------------------------------------------------------
