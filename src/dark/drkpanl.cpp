/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkpanl.cpp,v 1.15 1999/11/02 15:55:08 BODISAFA Exp $
#include <appagg.h>
#include <drkpanl.h>

#include <cursors.h>

#include <scrnmode.h>
#include <scrnman.h>

#include <panlmode.h>
#include <loopapi.h>
#include <loopmsg.h>
#include <panlguid.h>

#include <dispapi.H>

#include <drkuires.h>
#include <imgsrc.h>
#include <resapi.h>
#include <fonrstyp.h>
#include <palrstyp.h>

#include <string.h>
#include <str.h>
#include <appsfx.h>

#include <appapi.h>

#include <command.h>
#include <config.h>
#include <cfgdbg.h>
#include <ctype.h>

#include <buttpanl.h>
#include <uigame.h>
#include <keydefs.h>
#include <metasnd.h>

//
// Include these last!
//
#include <dbmem.h>
#include <initguid.h>
#include <drkpanid.h>


//
// Setup Anti-Aliased Fonts
//


#define FONT_ROWS 16
#define FONT_ROW_SIZE 512

static ushort pFontTable[FONT_ROWS][FONT_ROW_SIZE];  // I wish I knew what the last 4 

// Make a 16 bit font table for the little loves
static void make_font_table(void)
{
   for (int shadIndex = 0; shadIndex < FONT_ROWS; shadIndex++)
   {
      float shadLevel = 1.0 - (float)shadIndex/((float)(FONT_ROWS-1));

      ushort* Ptr = pFontTable[shadIndex]; 

      if ( grd_bpp == 15 )
         gr_init_clut16(Ptr, 0, shadLevel, BMF_RGB_555);
      else
         gr_init_clut16(Ptr, 0, shadLevel, BMF_RGB_565);

   }

   // Set the lighting table attributes for a font.
   // For this not to have its own state is criminal.
   // It should NOT be a 2d thing, but a font thing.
   gr_set_light_tab((uchar *)pFontTable);
   gr_set_light_tab_size(FONT_ROWS);
}

//
// Default gui stule
//

static guiStyle def_style = 
{
   0, // palette
   {  // colors
      guiRGB(255,255,255), // fg
      guiRGB(  5,  5,  5), // bg
      guiRGB(255,255,200), // text
      guiRGB(100,150,  0), // hilite
      guiRGB(255,  0,100), // bright
      guiRGB( 64, 64, 64), // dim
      guiRGB(255,255,255), // fg2
      guiRGB( 64, 64, 64), // bg2
      guiRGB(200,200,200), // border
      guiRGB(255,255,255), // white
      guiRGB(  5,  5,  5), // black
      1, // xor
      1, // light bevel
      0, // dark bevel
   },
};


bool cDarkPanel::region_cb(uiEvent *ev, Region *reg, void *data)
{
   cDarkPanel* us = (cDarkPanel*)data; 
   if (ev->type == UI_EVENT_KBD_COOKED)
      switch(ev->subtype ^ KB_FLAG_DOWN)
      {
         case KEY_ESC:
            us->OnEscapeKey(); 
            return TRUE; 
      }
   return FALSE;
}

//
// Handle escape key
//

void cDarkPanel::OnEscapeKey()
{
   mpPanelMode->Exit(); 
}


//
// Build the buttons 
//

void cDarkPanel::InitUI()
{  
   ConfigSpew("dark_panel_spew",("Entering panel: %d\n",panel_name)); 

   // shoot me now... talk with Mahk about a better way
   metaSndEnterPanel(panelsndtype);
   SetMetaSndGUI();
   
   mStyle = def_style; 

   // Load the default panel style 
   uiGameLoadStyle("panel_",&mStyle,mResPath); 
   uiGameLoadStyle(cStr(panel_name)+"_",&mStyle,mResPath); 
   SetCurrentStyle(&mStyle); 
   
   mElems.SetSize(num_butts);
   mStrings.SetSize(num_string_butts);
   mElemData.SetSize(num_string_butts); 

   mRects.SetSize(num_rects); 

   cStr basename = panel_name; 

   // get the rects 
   cRectArray tmpRects; 
   FetchUIRects(basename+"r", tmpRects,mResPath);
      
   Assert_(tmpRects.Size() >= num_rects);
   int i; 

   int* map = new int[num_rects]; 
   int cnt = num_rects; 

   config_get_value(cStr("rect_")+basename,CONFIG_INT_TYPE,map,&cnt); 
   // Re-sort the rects according to enum/cfg vars
   for (i = 0; i < num_rects; i++)
   {
      int idx = (i < cnt) ? map[i] : i; 
      mRects[i] = tmpRects[idx]; 
   }

   if (!mpFontRes)
      mpFontRes = guiStyleGetFont(NULL,StyleFontNormal);

   if (mpFontRes)
      mpFontRes->AddRef(); 
   else
   {
      const char* fontname = string_font ? string_font : "textfont" ; 
      AutoAppIPtr(ResMan); 
      mpFontRes = pResMan->Bind(fontname,
                                RESTYPE_FONT,
                                NULL,
                                mResPath); 
   }
   
   for (i = 0; i < num_butts ; i++)
   {
      DrawElement& elem = mElems[i];
      memset(&elem,0,sizeof(elem)); 

      if (i < num_string_butts)
      {
         // fetch the string 
         mStrings[i] = FetchUIString(panel_name,string_names[i],mResPath); 

         elem.draw_type = DRAWTYPE_TEXT;
         elem.draw_data = (void*)(const char*)mStrings[i]; 
         elem.draw_flags |= INTERNAL(DRAWFLAG_INT_TRANSP); 
      }
   }

   Region* root = LGadBoxRegion(LGadCurrentRoot());
   int handler; 
   uiInstallRegionHandler(root, UI_EVENT_KBD_COOKED, region_cb, this, &handler);

   make_font_table(); 
}


void cDarkPanel::TermUI()
{
   SafeRelease(mpFontRes); 

   mStrings.SetSize(0); 
   mRects.SetSize(0); 
   mElems.SetSize(0); 
   mElemData.SetSize(0); 

   uiGameUnloadStyle(&mStyle); 
   uiGameStyleCleanup();

   metaSndExitPanel(FALSE);
   RestoreMetaSndGUI();

   ConfigSpew("dark_panel_spew",("Exiting panel: %d\n",panel_name)); 
}

// Loop message handler
//

void cDarkPanel::OnLoopMsg(eLoopMessage msg, tLoopMessageData data) 
{
   LoopMsg info; 
   info.raw = data; 

   switch (msg)
   {
      case kMsgEnterMode:
      case kMsgResumeMode:
         RedrawDisplay();
         break;

      case kMsgNormalFrame:
         SFX_Frame(NULL,NULL);
         break;
   }   
}

//
// Static: descriptor
//

sLoopClientDesc cDarkPanel::Descriptor = 
{
   &LOOPID_DarkPanel,
   "Generic Dark Panel Client", 
   kPriorityNormal, 
   kMsgEnd | kMsgsMode | kMsgsFrame,

   kLCF_None,
   NULL,
   
   NO_LC_DATA,

   {
      { kConstrainAfter, &LOOPID_Panel, kMsgsMode|kMsgsFrame},
      { kNullConstraint },
   }
}; 




//------------------------------------------------------------
// PANEL MODE
//

static sScrnMode scrnmode_desc = 
{
   kScrnModeDimsValid|kScrnModeBitDepthValid,
   640, 480, 
   16,
}; 


static sPanelModeDesc stat_desc = 
{ 
   &LOOPMODE_DarkPanel,
   0, 
   &LOOPID_DarkPanel,
   &scrnmode_desc,
   NULL, 
}; 

// stupid C++ trick to initialize a temporary desc and mask it 
// all before the left brace of cDarkPanel::cDarkPanel
struct sTempDesc: public sPanelModeDesc
{
   sTempDesc(const sDarkPanelDesc* desc) : sPanelModeDesc(stat_desc) 
   { 
      flags ^= desc->flagmask; 
      if (desc->loop_id)
         id = desc->loop_id; 
   }

}; 

//
// Cursor Image
//

IDataSource* cDarkPanel::CursorPalette()
{
   AutoAppIPtr(ResMan);
   return pResMan->Bind("cursor",RESTYPE_PALETTE,NULL,INTERFACE_PATH); 
}

//
// Construction/Destruction
//

cDarkPanel::cDarkPanel(const sDarkPanelDesc* desc, const char* respath)
   : cModalButtonPanel(&sTempDesc(desc), &Descriptor), 
     mpFontRes(NULL),
     sDarkPanelDesc(*desc)
{
   SetResPath(respath); 
}

cDarkPanel::~cDarkPanel()
{
   sPanelModeDesc pdesc = *mpPanelMode->Describe();
   delete [] (char*) pdesc.pal; 
   SafeRelease(mpFontRes); 
}

//
// Set path 
//

void cDarkPanel::SetResPath(const char* path)
{
   mResPath = path; 
   sPanelModeDesc pdesc = *mpPanelMode->Describe(); 
   char* p = new char[strlen(mResPath)+strlen("\\")+strlen(panel_name)+1];  

   sprintf(p,"%s\\%s",(const char*)mResPath,panel_name); 
   delete [] (char*)pdesc.pal; 
   pdesc.pal = p;  // from our desc 
   mpPanelMode->SetDescription(&pdesc); 

   char backname[32];
   sprintf(backname,"%sb",panel_name);
   IImageSource* bg = FetchUIImage(backname,mResPath); 
   if (bg==NULL) {
      bg = FetchUIImage(panel_name,mResPath); 
   }

   mpPanelMode->SetImage(kPanelBG,bg);
   SafeRelease(bg); 
}

// DarkIsToGameModeGUID - Game-specific mode switching goodness.
// Keeps engine from knowing the internals of Thief 2's GUIDs.
// TRUE: We're headed into the game.
BOOL DarkIsToGameModeGUID(REFGUID rguid)
{
   return IsEqualGUID(rguid, LOOPMODE_DarkPanel); 
}
