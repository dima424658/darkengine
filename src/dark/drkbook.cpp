/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkbook.cpp,v 1.9 2000/02/18 13:58:53 patmac Exp $

#include <drkbook.h>
#include <drkpanl.h>
#include <appagg.h>

#include <config.h>
#include <cfgtool.h>
#include <command.h>
#include <ctype.h>

#include <dev2d.h>
#include <resapi.h>
#include <resistr.h>
#include <respaths.h>
#include <drkuires.h>
#include <gcompose.h>
#include <uigame.h>
#include <keydefs.h>
#include <metasnd.h>
#include <guistyle.h>

//
// INCLUDE THESE LAST
// 
#include <dbmem.h>


//------------------------------------------------------------
// Book button panel
//

class cBook: public cDarkPanel
{
   static sDarkPanelDesc gDesc; 

public:
   cBook(): cDarkPanel(&gDesc) {}; 
   ~cBook() {  }; 

   enum eRects
   {
      kStringButts, 

      kPrev = kStringButts, 
      kNext, 
      kNumButts, 
      kNumRects = kNumButts, 
   }; 

   void SetText(const char* resname) { mTextRes = resname; }; 
   BOOL FindText();
   BOOL FindArt();


protected:
   void RedrawDisplay(); 
   void OnButtonList(ushort action, int button); 
   void OnLoopMsg(eLoopMessage, tLoopMessageData ); 
   void InitUI(); 
   void TermUI(); 

   static BOOL region_cb(uiEvent *ev, Region *reg, void *data);


   // Lookup the text string
   const char* PageName(int i)
   {
      static char buf[8]; 
      sprintf(buf,"PAGE_%d",i); 
      return buf;
   }

   void RedrawButtons(); 

   int mPageNum; 
   int mMinPage;
   int mMaxPage; 
   cRectArray mTextRects; 
   cStr mTextRes; 
   IStringRes* mpTextStrings; 
   int mTextColor; 

   enum 
   { 
      kNumPageButts = kNumButts,
      kNumPageImages = 2*kNumPageButts
   }; 
   IImageSource* mPageImages[kNumPageImages]; // next/prev page images 
   

};

////////////////////////////////////////

void cBook::InitUI()
{
   char buf[64]; 
   BOOL foundbook = FALSE;

   cDarkPanel::InitUI();   
   int i; 
   for (i = 0; i < kNumPageImages; i++)
   {
      sprintf(buf,"bookr%03d",i); 
      mPageImages[i] = FetchUIImage(buf,mResPath); 
   }

   int n = 0; 
   for (i = 0; i < kNumPageButts; i++,n++)
   {
      DrawElement& elem = mElems[i]; 
      memset(&elem,0,sizeof(elem));
      elem.draw_type = DRAWTYPE_BITMAP; 
      elem.draw_data  = mPageImages[n]->Lock(); 
      elem.draw_data2 = mPageImages[n+kNumPageButts]->Lock(); 
      elem.draw_flags = INTERNAL(DRAWFLAG_INT_TRANSP); 
   }

   // Setup text strings and rectangles 
   FetchUIRects("textr",mTextRects,mResPath); 

   AutoAppIPtr(ResMan);
   IRes* pRes = pResMan->Bind(mTextRes,"String",gContextPath,"Books"); 
   if (pRes)
   {
      Verify(SUCCEEDED(pRes->QueryInterface(IID_IStringRes,(void**)&mpTextStrings))); 
      SafeRelease(pRes); 
      foundbook = TRUE;
   }

   if (!foundbook)
     {
#ifndef SHIP
       SetText("errorbk");
#endif
#ifdef SHIP
       SetText("blankbk");
#endif
       //try again
       IRes* pRes = pResMan->Bind(mTextRes,"String",gContextPath,"Books"); 
       if (pRes)
	 {
	   Verify(SUCCEEDED(pRes->QueryInterface(IID_IStringRes,(void**)&mpTextStrings))); 
	   SafeRelease(pRes); 
	   foundbook = TRUE;
	 }
     }

   AssertMsg(foundbook,"Failed to load book resource!");

   // find min and max page 
   for (i = 0; i < 8; i++)  // first page must be numbered 0-7
     if (mpTextStrings->StringExtract(PageName(i),buf,sizeof(buf)))
       break; 
   mMinPage = i; 
   
   BOOL done = FALSE; 
   for (i = mMinPage; !done; i++)
     if (!mpTextStrings->StringExtract(PageName(i),buf,sizeof(buf)))
       break; 
   mMaxPage = i-1; 
   
   // get the color
   int rgb[3] = { 1, 1, 1, }; 
   int cnt = 3; 
   sprintf(buf,"color_%s",mResPath);
   config_get_value(buf,CONFIG_INT_TYPE,rgb,&cnt); 
   mTextColor = gr_make_screen_fcolor(uiRGB(rgb[0],rgb[1],rgb[2])); 
   
   Region* root = LGadBoxRegion(LGadCurrentRoot());
   int handler; 
   uiInstallRegionHandler(root, UI_EVENT_MOUSE|UI_EVENT_KBD_COOKED, region_cb, this, &handler);
}

void cBook::TermUI()
{
   for (int i = 0; i < kNumPageImages; i++)
   {
      mPageImages[i]->Unlock(); 
      SafeRelease(mPageImages[i]); 
   }
   mTextRects.SetSize(0); 
   SafeRelease(mpTextStrings); 
   cDarkPanel::TermUI(); 
}

////////////////////////////////////////

BOOL cBook::region_cb(uiEvent *ev, Region *reg, void *data)
{
   cBook* us = (cBook*)data; 

   bool ateIt = FALSE;
   bool prevPage = FALSE;
   bool nextPage = FALSE;

   if (ev->type == UI_EVENT_KBD_COOKED && (ev->subtype & KB_FLAG_DOWN)) {
      us->mpPanelMode->Exit(); 
      return TRUE;
   }

   if (ev->type == UI_EVENT_MOUSE && (ev->subtype & (MOUSE_LUP|MOUSE_RUP))) {

      for (int i = 0; i < us->mRects.Size(); i++) {
         if (RECT_TEST_PT(&us->mRects[i],ev->pos)) {
            return FALSE;
         }
      }
      switch ( us->mTextRects.Size() ) {
         case 1:
            // on  a book which shows only 1 page at a time, clicking
            // anywhere to the left of the left edge of the text rect
            // goes to the previous page, clicking anywhere else goes
            // to the next page
            ateIt = TRUE; 
            if ( ev->pos.x < us->mTextRects[0].ul.x ) {
               prevPage = TRUE;
            } else {
               nextPage = TRUE;
            }
            break;
         case 2:
            // on a book which shows 2 pages at a time, clicking to the left
            // of the right edge of the left text rect goes to the previous
            // page, clicking to the right of the left edge of the right page
            // will go to the next page.  clicking in between these two pages
            // will have no effect
            if ( ev->pos.x <= us->mTextRects[0].lr.x ) {
               prevPage = TRUE;
            } else if ( ev->pos.x >= us->mTextRects[1].ul.x ) {
               nextPage = TRUE;
            }
            break;
      }
   }
   if ( nextPage && (us->mPageNum < us->mMaxPage) ) {
      us->OnButtonList( BUTTONGADG_LCLICK, kNext );
      metaSndPlayGUICallback( StyleSoundSelect );
      ateIt = TRUE;
   }
   if ( prevPage && (us->mPageNum > us->mMinPage) ) {
      us->OnButtonList( BUTTONGADG_LCLICK, kPrev );
      metaSndPlayGUICallback( StyleSoundSelect );
      ateIt = TRUE;
   }

   return ateIt;
}


////////////////////////////////////////


// I sure wish this were exposed by the 2d 
#define CHAR_SOFTCR 1 // Soft carriage return.  

//
// Behold the way in which we assume our font is of uniform vertical
// spacing.  A pretty safe assumption

void cBook::RedrawDisplay()
{
   RedrawButtons(); 

   cStr page = PageName(mPageNum); 
   char* text = mpTextStrings->StringLock(page); 
   if (text && *text != NULL)
   {
      // grab the background image
      IImageSource* src = mpPanelMode->GetImage(kPanelBG); 
      grs_bitmap* bg = (grs_bitmap*)src->Lock(); 

      // Set up the font 
      grs_font* font = (grs_font*)mpFontRes->Lock(); 
      // find out how tall it is.
      short ch = gr_font_string_height(font,"X"); 

      // lookup the the text color 
      

      // Fill the text rects with text
      char* s = text; 

      // skip past leading whitespace
      while (isspace(*s) && *s != '\0') 
         s++; 

// This loop doesn't clear the right page unless the text wraps over there.
// Really, ignore the fact that the string is null, and just wrap the
// blasted thing and display nothing.  So we spend a marginal extra 
// amount of time in the loop.  big whoop. Sim is stopped anyway.
//       for (int i = 0; i < mTextRects.Size() && s && *s != '\0'; i++)
      for (int i = 0; i < mTextRects.Size() && s; i++) //skip empty string test
      {
         Rect r = mTextRects[i];  

	 char *stemp;

	 //replace all CHAR_SOFTCHRs with spaces, so that we don't
	 //double format the right page.
	 while (stemp = strchr(s,CHAR_SOFTCR))
	   *stemp = ' ';

         // wrap the string.  
	 gr_font_string_wrap(font,s,RectWidth(&r)); 

         // Now grovel through it for the amount we can draw. 
         int rh = RectHeight(&r); // rect height
         int h = 0; // text height, so far 
	 //make sure that h+ch <= rh, so that you don't wrap it off.
         char* v;
         for (v = s; *v != '\0' && ((h + ch)<= rh); v++)
            if (*v == '\n' || *v == CHAR_SOFTCR)
               h += ch; 

         // stomp a null 
         char tmp = *v; 
         *v = '\0'; 

         // Draw the text 
         GUIcompose c;
         GUIsetup(&c,&r,ComposeFlagRead,GUI_CANV_ANY); 

         gr_bitmap(bg,-r.ul.x,-r.ul.y); 

         gr_set_font(font); 
         gr_set_fcolor(mTextColor); 
         gr_string(s,0,0); 
   
         GUIdone(&c); 

         gr_font_string_unwrap(s); 

         // unstomp null
         *v = tmp; 
         // go past the part we drew
         s = v; 

         // skip past leading whitespace
         while (*s != '\0' && isspace(*s))
            s++; 
      }
      src->Unlock(); 
      SafeRelease(src); 
      mpFontRes->Unlock(); 
   }
   if (text)
      mpTextStrings->StringUnlock(page);
}

////////////////////////////////////////

void cBook::RedrawButtons()
{
   if (mPageNum == mMinPage)
   {
      mElems[(int)kPrev].draw_type = DRAWTYPE_NONE; 
      GUIErase(&mRects[(int)kPrev]); 
   }
   else
   {
      mElems[(int)kPrev].draw_type = DRAWTYPE_BITMAP; 
      region_expose(LGadBoxRegion(LGadCurrentRoot()),&mRects[(int)kPrev]);      
   }

   if (mPageNum == mMaxPage)
   {
      mElems[(int)kNext].draw_type = DRAWTYPE_NONE; 
      GUIErase(&mRects[(int)kNext]); 
   }
   else
   {
      mElems[(int)kNext].draw_type = DRAWTYPE_BITMAP; 
      region_expose(LGadBoxRegion(LGadCurrentRoot()),&mRects[(int)kNext]);      
   }

}

////////////////////////////////////////

void cBook::OnButtonList(ushort action , int button)
{
   if (action & (BUTTONGADG_LCLICK|BUTTONGADG_RCLICK))
   {
      switch (button)
      {

         case kNext:
         {
            if (mPageNum < mMaxPage)
            {
               mPageNum++; 
               RedrawDisplay(); 
            }
         }
         break; 

         case kPrev:
         {
            if (mPageNum > mMinPage)
            {
               mPageNum--; 
               RedrawDisplay(); 
            }
         }
         break; 
      }
   }
}

////////////////////////////////////////

void cBook::OnLoopMsg(eLoopMessage msg, tLoopMessageData data)
{
   switch (msg)
   {
      case kMsgEnterMode:
         mPageNum = mMinPage; 
         break; 
   }
   cDarkPanel::OnLoopMsg(msg,data);
}


////////////////////////////////////////


sDarkPanelDesc cBook::gDesc = 
{
   "book",
   cBook::kNumButts,
   cBook::kNumRects,
   cBook::kStringButts, 
   NULL,
   NULL,
   0,
   kMetaSndPanelBook
}; 

////////////////////////////////////////

BOOL cBook::FindText()
{
  AutoAppIPtr(ResMan);
  IRes* pRes = pResMan->Bind(mTextRes,"String",gContextPath,"Books");
  if (pRes)
    {
      SafeRelease(pRes);
      return TRUE;
    }
  return FALSE;
}

////////////////////////////////////////

BOOL cBook::FindArt()
{
  char buf[80];
  char filename[255];
  sprintf(buf,"%s\\book.pcx",mResPath);
  if (!find_file_in_config_path(filename,buf,"resname_base"))
    return FALSE;
  return TRUE;
}

////////////////////////////////////////

static cBook* gpBook = NULL; 

void SwitchToDarkBookMode(BOOL push, const char* text, const char* art) 
{
   if (gpBook)
   {
      gpBook->SetText(text); 
      char buf[64]; 
      sprintf(buf,"Books\\%s",art); 
      gpBook->SetResPath(buf); 
      
      IPanelMode* panel = gpBook->GetPanelMode(); 
      panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
      SafeRelease(panel); 
   }
}

BOOL TestBookExists(const char* text, const char* art)
{
  if (!gpBook)
    return FALSE; //maybe it exists, but we can't display books anyway.

  cBook* testBook = new cBook;
  testBook->SetText(text);
  char buf[64];
  sprintf(buf,"Books\\%s",art);
  testBook->SetResPath(buf);
  
  if (!testBook->FindText())
    return FALSE;
  if (!testBook->FindArt())
    return FALSE;

  return TRUE;
}


static void create_panel_mode()
{
   gpBook = new cBook; 
}



void init_commands(void);

void DarkBookInit(void)
{
   create_panel_mode(); 
   init_commands(); 
}


void DarkBookTerm()
{
   delete gpBook; 
}

//
// Commands
//


static void do_book(char* arg)
{
   char* art = strchr(arg,','); 
   *art = '\0'; 
   art++; 
   while (isspace(*art))
      art++; 
   
   SwitchToDarkBookMode(TRUE,arg,art); 
}

static Command commands [] = 
{
   { "test_book", FUNC_STRING, do_book, "book <text>,<art>", HK_ALL },
};

static void init_commands()
{
   COMMANDS(commands,HK_ALL); 
}

