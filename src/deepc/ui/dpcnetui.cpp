/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <dpcnetui.h>
#include <drkpanl.h>
#include <metasnd.h>

#include <gadget.h>
#include <gadblist.h>
#include <gadtext.h>
#include <guistyle.h>
#include <uigame.h>
#include <gcompose.h>
#include <keydefs.h>

#include <config.h>
#include <cfgdbg.h>
#include <ctype.h>

#include <lazyagg.h>
#include <objtype.h>
#include <dpcnet.h>
#include <netman.h>
#include <netnotif.h>
#include <netmsg.h>

#include <scrnman.h>
#include <dpclding.h>
#include <panltool.h>
#include <dpcsavui.h>
#include <dpcuitul.h>

class cDPCMultiplayUI; 
class cMultiplayUIPage; 

static LazyAggMember(INetManager) gpNetMan; 

//------------------------------------------------------------
// Class: cDPCMultiplayUI
// (Declaration)

class cDPCMultiplayUI: public cDarkPanel
{
   typedef cDarkPanel cParent; 
   static sDarkPanelDesc gDesc; 

public:
   cDPCMultiplayUI();
   ~cDPCMultiplayUI(); 

   enum eRects
   {
      kPrev,
      kCancel, 
      kNext,
      kNumButts,

      kTitle = kNumButts,
      kScreen,

      kNumRects
   };

   enum 
   {
      // Net messages we listen to
      kNetMessageInterests = ~0,
   }; 

   //
   // Expose bunches of stuff our pages want
   // 
   LGadRoot* ScreenRoot() { return &mScreenRoot; }; 
   guiStyle* Style() { return &mStyle; }; 
   const char* ResPath() { return mResPath; }; 

   cStr GetString(const char* name)
   {
      return FetchUIString(panel_name,name,mResPath); 
   }

   //
   // Change the overall page sequence.
   //
   void SetPages(int* pages, int idx = 0); 

   // Spoof a button click
   void SpoofUserClick(int button); 

   // correctly remove the next & prev buttons based on page index
   // and flags 
   void SetupButtons(); 

   

protected:

   // Panel overrides 
   void OnButtonList(ushort action, int button);
   void InitUI();
   void TermUI();
   void OnLoopMsg(eLoopMessage, tLoopMessageData );
   void RedrawDisplay();
   void OnEscapeKey(); 

   // Helpers 
   void SetupPage(BOOL draw = TRUE); 
   void CleanupPage(); 
   cMultiplayUIPage* CreatePage(int page_id); 
   void ChangePage(); 

   // net listener 
   static void net_cb(eNetListenMsgs msg, DWORD data, void* clientdata);
   void OnNetMsg(eNetListenMsgs msg, DWORD data); 
   
   LGadRoot mScreenRoot;      // Gadget root for use by pages
   int* mpPageList;           // Array of page id's 
   int mPageIdx;              // Which page index into list
   cMultiplayUIPage* mpPage;  // Current page 
   int mPageDelta;            // Direction for page change
   tNetListenerHandle mNetListener;// Net listener handle
}; 


//------------------------------------------------------------
// Class: cMultiplayUIPage
//
// A single "page" of our wizard.  A page only exists while it is being displayed;
// it's created and destroyed every time.   
//

enum eDataExchange
{
   kInitialize, 
   kSaveAndValidate, 
   kAbort, 
}; 

enum eMultiplayPageFlags 
{

   kNoNextButton  = 1 << 0, // This page should not have a next button.  
   //   kFinishButton  = 1 << 1, // This page's next button should say "finish" 
}; 

// I can't write a class without a descriptor, baby 
struct sMultiplayUIPageDesc 
{
   char name[32]; // name for resources & stuff 
   ulong flags; 
};




class cMultiplayUIPage
{

public: 
   typedef sMultiplayUIPageDesc sDesc; 

   cMultiplayUIPage(const sDesc& desc, cDPCMultiplayUI* panel); 
   virtual ~cMultiplayUIPage();

   const sMultiplayUIPageDesc& Desc() { return mDesc; }; 

   //----------------------------------------
   // Overridables
   //

   //
   // Init/Term UI
   //
   virtual void InitUI(); 
   virtual void TermUI(); 

   //
   // Data Exchange, ala windows dialogs
   //
   virtual BOOL ExchangeData(eDataExchange op) = 0; 

   //
   // Draw the page
   //
   virtual void Draw();

   //
   // Do Frame Stuff
   //
   virtual void OnFrame() {}; 

   //
   // Handle net messages
   //
   virtual void OnNetMsg(eNetListenMsgs , DWORD ) {}; 
   
public:
   //----------------------------------------
   // Special accessors
   //
   LGadRoot* Root() { return mpPanel->ScreenRoot(); }; 
   guiStyle* Style() { return mpPanel->Style(); }; 
   const char* ResPath() { return mpPanel->ResPath(); }; 
   cStr GetString(const char* name) { return mpPanel->GetString(name); }; 
   const Rect& UsableArea() { return mUsableArea; }; 
   int TextHeight() { return mTextHeight; }; 

   // Get the rect describing text line N.  Starts with zero
   Rect LineRect(int n); 

   // Absolutize a screen rect in place
   void Absolutize(Rect* r); 

   // Output a message on the message line 
   void Message(const char* msg); 


   // Draw an arbitrary string in a rect, centered and top aligned.
   // Return the height drawn, that is to say the y coord of the bottom of text  
   int DrawString(const char* str, const Rect& r, ulong flags = 0, guiStyle* style = NULL, StyleFontKind font = StyleFontNormal); 

protected: 
   enum 
   {
      // margins, in percent 
      kXMargin = 15,
      kYMargin = 5, 
   }; 

   //----------------------------------------
   // Data members
   //
   cDPCMultiplayUI* mpPanel; // the UI panel
   sDesc mDesc; // our desc 

   // Text message line support 
   Rect mMessageRect; 
   guiStyle mMessageStyle; 

   // Recommended area to put your stuff in, relative to root 
   Rect mUsableArea;  

   // Height of a text line
   int mTextHeight;

   // Current message string
   cStr mMsg; 

}; 

//------------------------------------------------------------
// More helpers for page implementors 
//

//----------------------------------------
// Get the height of a style font 
//
static int style_font_height(guiStyle& style, StyleFontKind which)
{
   short w,h; 
   guiStyleSetupFont(&style,which); 
   gr_string_size("X",&w,&h); 
   guiStyleCleanupFont(&style,which);
   return h; 
}

//----------------------------------------
// Goofy draw element class for us C++-ers 
//



static const DrawElement def_draw_elem = 
{ 
   0, 0, 0, // type, data, data2
   0, 0, // fcolor bcolor
   INTERNAL(DRAWFLAG_INT_TRANSP),  // flags
}; 


struct sDrawElem: public DrawElement
{
   sDrawElem() :DrawElement(def_draw_elem) {}; 

   // For text elems
   void SetText(const char* text) 
   {
      draw_data = (void*)text; 
      draw_type = DRAWTYPE_TEXT; 
   }

}; 

//------------------------------------------------------------
// Copy a gui style.  Should be in uiGame, but who wants to 
// change the header 
//

static void CopyGuiStyle(guiStyle* targ, const guiStyle* src)
{
   *targ = *src; 
   for (int i = 0;  i < StyleNumFonts; i++)
      if (targ->fonts[i])
         targ->fonts[i]->AddRef(); 
}

//------------------------------------------------------------
// Set up a box the way I like it
//

static void setup_box(LGadBox* box)
{
   int flags = LGadBoxFlags(box); 
   LGadBoxSetFlags(box, flags | BOXFLAG_CLEAR); 
}


////////////////////////////////////////

#define Y_MARGIN 2
#define MESSAGE_LINES 2

cMultiplayUIPage::cMultiplayUIPage(const sDesc& desc, cDPCMultiplayUI* panel)
   :mpPanel(panel), mDesc(desc)
{

}


cMultiplayUIPage::~cMultiplayUIPage()
{
}

////////////////////////////////////////

void cMultiplayUIPage::InitUI()
{
   CopyGuiStyle(&mMessageStyle,Style()); 
   uiGameLoadStyle("netplay_msg_",&mMessageStyle); 

   // Make the message rect, only two lines tall 
   Rect& screen_r = *LGadBoxRect(Root());
   Rect rel_screen_r = { {0, 0}, {RectWidth(&screen_r), RectHeight(&screen_r) }}; 

   mMessageRect = rel_screen_r; 
   mMessageRect.lr.y = mMessageRect.ul.y + Y_MARGIN + style_font_height(mMessageStyle,StyleFontNormal)*MESSAGE_LINES; 


   // Make the usable area rect, just below the message line, brought in by margins
   mUsableArea = rel_screen_r; 
   mUsableArea.ul.y = mMessageRect.lr.y; 
   int xmargin = RectWidth(&mUsableArea) * kXMargin/100; 
   int ymargin = RectHeight(&mUsableArea) * kYMargin/100; 
   mUsableArea.ul.x += xmargin; 
   mUsableArea.ul.y += ymargin; 
   mUsableArea.lr.x -= xmargin; 
   mUsableArea.lr.y -= ymargin; 

   // get font hight
   mTextHeight = style_font_height(*Style(),StyleFontNormal); 

   // Initialize message
   mMsg = GetString(cStr(mDesc.name)+"_init_msg"); 
   ConfigSpew("netplay_ui_spew",("Init Message: '%s'\n",(const char*)mMsg));

}

void cMultiplayUIPage::TermUI()
{
   uiGameUnloadStyle(&mMessageStyle); 
}

////////////////////////////////////////


int cMultiplayUIPage::DrawString(const char* str, const Rect& relr, ulong flags, guiStyle* style, StyleFontKind font)
{

   if (!style)
      style = Style(); 

   Rect r = relr;
   Absolutize(&r); 

   return DPCDrawString(str,r,flags,style,font); 
}

////////////////////////////////////////

Rect cMultiplayUIPage::LineRect(int n)
{
   Rect r = mUsableArea; 
   r.ul.y += mTextHeight*n; 
   r.lr.y = r.ul.y + mTextHeight; 
   return r; 
}

////////////////////////////////////////

void cMultiplayUIPage::Absolutize(Rect* r)
{
   Region* reg = LGadBoxRegion(Root()); 
   Point abs_ul = MakePoint(reg->abs_x,reg->abs_y); 
   RECT_MOVE(r,abs_ul); 
}

////////////////////////////////////////

void cMultiplayUIPage::Message(const char* str)
{
   mMsg = str; 
   DrawString(str,mMessageRect,0, &mMessageStyle); 
   ConfigSpew("netplay_ui_spew",("Message: '%s'\n",str));

   // absolutize rect 
   Rect r = mMessageRect; 
   Absolutize(&r); 
   ScrnForceUpdateRect(&r); 
}

////////////////////////////////////////

void cMultiplayUIPage::Draw()
{
   DrawString(mMsg,mMessageRect,0, &mMessageStyle); 
}

//------------------------------------------------------------
// The global list of all multiplayer ui pages 
//

enum eAllPages
{
   kNullPage,  // the null page
   kFrontPage,  // choose your path
   kGameName,  // Enter game name (host only)
   kChooseHost,  // Choose your host (join only)
   kHostLobby,     // Wait for other players 
   kClientLobby,     // Wait for other players 
   kHostStart,       // Host-side start game
   kClientStart,     // Client-side start game 

   kNumPages
}; 

// The host sequence

static int gHostSeq[] = 
{
   kFrontPage,
   kGameName,
   kHostLobby,
   kHostStart,
   kNullPage,
}; 

// The client sequence 

static int gClientSeq[] = 
{
   kFrontPage,
   kChooseHost, 
   kClientLobby,
   kClientStart,  
   kNullPage,

}; 

//------------------------------------------------------------
// Class: cMPFrontPage
// The front page
//


class cMPFrontPage: public cMultiplayUIPage
{
protected: 
   static sDesc gDesc; 
   typedef cMultiplayUIPage cParent; 


   enum eButtons
   {
      kHost,
      kClient, 
      kNumButtons
   }; 

   LGadButtonList mButtons;
   cStr mStrings[kNumButtons]; 
   Rect mRects[kNumButtons];
   sDrawElem mElems[kNumButtons]; 

   // Go from a button idx to a resource name. 
   static const char* ButtResName(int i)
   {
      static const char* names[] = { "fp_host", "fp_client" };
      return names[i]; 
   }

   static bool blist_cb(ushort, int, void*, LGadBox* )
   {
      return FALSE; 
   }

public:
   cMPFrontPage(cDPCMultiplayUI* pUI)
      : cParent(gDesc,pUI)
   {
      memset(&mButtons,0,sizeof(mButtons)); 
   }; 

   // ----------------------------------------

   void InitUI()
   {
      cParent::InitUI(); 

      Rect area = UsableArea(); 

      // Get the strings.  This will totally break if they need to wrap 
      for (int i = 0; i < kNumButtons; i++)
      {
         mStrings[i] = GetString(ButtResName(i)); 
         
         // Fill out the draw element 
         mElems[i].SetText(mStrings[i]); 
      }

      for (i = 0; i < kNumButtons; i++)
         mRects[i] = LineRect(i*2+1); 

      // Create the button list 
      LGadButtonListDesc bdesc = 
      {
         kNumButtons, mRects, mElems, blist_cb, 0, BLIST_RADIO_FLAG 
      }; 

      LGadCreateButtonListDesc(&mButtons,Root(),&bdesc); 
      setup_box(VB(&mButtons)); 
   }

   // ----------------------------------------

   void TermUI()
   {
      LGadDestroyButtonList(&mButtons); 

      cParent::TermUI(); 
   }

   // ----------------------------------------

   BOOL ExchangeData(eDataExchange op)
   {
      static const char* cfg_var = "netplay_frontpage_default"; 
      switch (op)
      {
         case kInitialize:
         {
            // Default radio button 
            int butt = 0; 
            config_get_int(cfg_var,&butt); 
            LGadRadioButtonSelect(&mButtons,butt); 
         }
         break; 

         case kSaveAndValidate:
         {
            static int* sequences[] = { gHostSeq, gClientSeq}; 

            int butt = LGadRadioButtonSelection(&mButtons); 
            config_set_int(cfg_var,butt); 
            // Set the page sequence 
            mpPanel->SetPages(sequences[butt]); 

         }
         break; 

      }

      return TRUE; 
   }

   // ----------------------------------------
   
   void Draw()
   {
      cParent::Draw(); 
      LGadDrawBox(VB(&mButtons),NULL); 
   }

   static cMultiplayUIPage* Create(cDPCMultiplayUI* pUI)
   {
      return new cMPFrontPage(pUI); 
   }

};

sMultiplayUIPageDesc cMPFrontPage::gDesc = { "front" }; 

//------------------------------------------------------------
// Class: cPageTextBox 
// A text box control for implementing pages 
//

class cPageTextBox 
{
protected: 
   enum 
   {
      kDefaultLine = 3, 
      kDefaultBufSize = 128, 
      kYMargin = 2,
   };

   cMultiplayUIPage* mpPage; // The page we are on
   cStr mResName;            // Our unique res name 
   int mLine;                // Our first line 
   LGadTextBox mBox;         // the gadget itself 
   int mBufSize;             // our buffer size
   char* mBuf;               // our buffer 
   int mBoxFlags;          // box flags for gadget creation.  
   
public:
   cPageTextBox(cMultiplayUIPage* page, const char* resname, 
                int line = kDefaultLine, int bufsize = kDefaultBufSize, 
                int boxflags = 0)
      : mpPage(page),
        mResName(resname), 
        mLine(line),
        mBufSize(bufsize),
        mBuf(new char[bufsize]),
        mBoxFlags(boxflags)
   {
      memset(&mBox,0,sizeof(mBox));
   };

   ~cPageTextBox()
   {
      delete [] mBuf; 
   }

   // ----------------------------------------
   // Accessors 
   // 

   const char* GetText() { return mBuf; }; 

   void SetText(const char* s)
   {
      strncpy(mBuf,s,mBufSize);
      mBuf[mBufSize-1] = '\0'; 
      LGadUpdateTextBox(&mBox); 
   }

   //----------------------------------------
   // Text Gadget Callback.   
   //

   void OnTextBox(LGadTextBoxEvent event, int eventdata)
   {
      if (event == TEXTBOX_SPECKEY 
          && eventdata == (KB_FLAG_DOWN|KEY_ENTER))
      {
         ExchangeData(kSaveAndValidate); 
         LGadUnfocusTextBox(&mBox); 
      }
   }

   static bool box_cb(LGadTextBox* box, LGadTextBoxEvent event, int eventdata, void *user_data)
   {
      cPageTextBox* us = (cPageTextBox*)user_data; 
      us->OnTextBox(event,eventdata);
      return FALSE; 
   }

   //------------------------------------------
   // UI Page protocol 
   //

   void InitUI()
   {
      // Create the edit box 
      LGadTextBoxDesc desc = 
      { 
         { 0 }, 
         mBuf, mBufSize, 
         TEXTBOX_BORDER_FLAG|TEXTBOX_FOCUS_FLAG, 
         box_cb, this, 
         mpPage->Style() 
      }; 

      desc.bounds = mpPage->LineRect(mLine+1); // First line is prompt
      desc.bounds.lr.y += kYMargin; 
      LGadCreateTextBoxDesc(&mBox,mpPage->Root(),&desc); 
      setup_box(VB(&mBox)); 
   }

   // ----------------------------------------

   void TermUI()
   {
      LGadDestroyTextBox(&mBox); 
   }

   BOOL ExchangeData(eDataExchange op)
   {
      cStr cfg_var = mResName + "_default";
      switch (op)
      {
         case kInitialize:
         {
            mBuf[0] = '\0'; 
            // Get name from cfg 
            if (!config_get_raw(cfg_var,mBuf,mBufSize))
               strncpy(mBuf,mpPage->GetString(cfg_var),mBufSize);

            mBuf[mBufSize-1] = '\0'; 
            // just for safety, stomp whitespace.  May truncate the string
            if (mBoxFlags & TEXTBOX_EDIT_NOSPACES)
               for (char* s = mBuf; *s != '\0'; s++)
                  if (isspace(*s))
                     *s = '\0'; 


            LGadTextBoxClrFlag(&mBox,~mBoxFlags); 
            LGadTextBoxSetFlag(&mBox,mBoxFlags); 
            LGadUpdateTextBox(&mBox); 

         }
         break; 

         case kSaveAndValidate:
         {
            config_set_string(cfg_var,mBuf); 
         }
         break; 

      }

      return TRUE; 
   }

   // ----------------------------------------
   
   void Draw()
   {
      // Draw prompt
      mpPage->DrawString(mpPage->GetString(mResName+"_prompt"),mpPage->LineRect(mLine), kLeftAlign); 

      // Draw the box 
      LGadDrawBox(VB(&mBox),NULL);
   }

};  


//------------------------------------------------------------
// Class: cMPGameNamePage
// The game name page
//


class cMPGameNamePage: public cMultiplayUIPage
{
protected: 
   static sDesc gDesc; 
   typedef cMultiplayUIPage cParent; 


   cPageTextBox mGame; 
   cPageTextBox mChar;
   cStr mAddress;

   enum 
   { 
      kIPLine = 1,
      kBoxLine = 3, 
      kCharLine = 6,
   }; 


public:
   cMPGameNamePage(cDPCMultiplayUI* pUI)
     : cParent(gDesc,pUI),
       mGame(this,"gamename",kBoxLine,MAX_HOSTNAME_LEN,TEXTBOX_EDIT_NOSPACES),
       mChar(this,"charname",kCharLine,MAX_PLAYER_NAME_LEN) 
   {

   }; 

   // ----------------------------------------

   

   void InitUI()
   {
      cParent::InitUI(); 

      mGame.InitUI(); 
      mChar.InitUI(); 



   }

   // ----------------------------------------

   void TermUI()
   {
      mGame.TermUI(); 
      mChar.TermUI(); 

      cParent::TermUI(); 
   }

   // ----------------------------------------

   BOOL ExchangeData(eDataExchange op)
   {
      mGame.ExchangeData(op); 
      mChar.ExchangeData(op); 

      switch (op)
      {
         case kInitialize:
            // If we come back up from the lobby page, then we don't want
            // to re-host.
            if (!gpNetMan->IsNetworkGame())
            {
               gpNetMan->Host(NULL,NULL); 
            }

            // Get the ip address 
            mAddress = gpNetMan->GetPlayerAddress(OBJ_NULL); 
            break; 

         case kAbort:
            gpNetMan->Leave(); 
            break; 

         case kSaveAndValidate:
            // Prohibit "shodan" as a player name
            if (stricmp(mChar.GetText(),GetString("shodan")) == 0)
            {
               Message(GetString("no_shodan")); 
               return FALSE; 
            }

            DPCSetHostName(mGame.GetText()); 
            gpNetMan->SetPlayerName(mChar.GetText()); 
            break; 
            
      }

      return TRUE; 
   }

   // ----------------------------------------
   
   void Draw()
   {
      cParent::Draw(); 

      cStr IP = GetString("host_address") + mAddress; 
      DrawString(IP, LineRect(kIPLine), kLeftAlign); 
      mGame.Draw(); 
      mChar.Draw(); 
   }

   static cMultiplayUIPage* Create(cDPCMultiplayUI* pUI)
   {
      return new cMPGameNamePage(pUI); 
   }

};

sMultiplayUIPageDesc cMPGameNamePage::gDesc = { "game_name" }; 

//------------------------------------------------------------
// Class: cMPHostAddressPage
// The host address page
//


class cMPHostAddressPage: public cMultiplayUIPage
{
protected: 
   static sDesc gDesc; 
   typedef cMultiplayUIPage cParent; 


   cPageTextBox mHost; 
   cPageTextBox mChar;

   enum eJoinState
   {
      kWaiting,
      kJoining,
      kJoined, 
   };

   eJoinState mJoinState; 

   enum 
   { 
      kBoxLine = 3, 
      kCharLine = 6,
   }; 


public:
   cMPHostAddressPage(cDPCMultiplayUI* pUI)
     : cParent(gDesc,pUI),
       mHost(this,"hostaddr",kBoxLine,128,TEXTBOX_EDIT_NOSPACES),
       mChar(this,"charname",kCharLine,MAX_PLAYER_NAME_LEN) ,
       mJoinState(kWaiting)
   {

   }; 

   // ----------------------------------------

   

   void InitUI()
   {
      cParent::InitUI(); 

      mHost.InitUI(); 
      mChar.InitUI(); 



   }

   // ----------------------------------------

   void TermUI()
   {
      mHost.TermUI(); 
      mChar.TermUI(); 

      cParent::TermUI(); 
   }

   // ----------------------------------------

   BOOL ExchangeData(eDataExchange op)
   {
      BOOL retval = TRUE; 
      mHost.ExchangeData(op); 
      mChar.ExchangeData(op); 

      switch (op)
      {
         case kInitialize:
            gpNetMan->Leave(); 
            mJoinState = kWaiting; 
            break; 

         case kAbort:
            gpNetMan->Leave(); 
            break; 

         case kSaveAndValidate:
            // Prohibit "shodan" as a player name
            if (stricmp(mChar.GetText(),GetString("shodan")) == 0)
            {
               Message(GetString("no_shodan")); 
               return FALSE; 
            }

            gpNetMan->SetPlayerName(mChar.GetText()); 
            retval =  mJoinState == kJoined; 

            if (mJoinState == kWaiting)
            {
               Message(GetString("joining"));

               BOOL success = gpNetMan->Join(NULL,NULL,mHost.GetText()); 
#ifndef SHIP
               if (config_is_defined("netplay_ui_spoof_join"))
               {
                  retval = success = TRUE; 
               }
#endif 

               if (success)
               {
                  mJoinState = kJoining; 
                  Message(GetString("wait_for_hi")); 
               }
               else
                  Message(GetString("join_fail")); 
            }
            break; 

            
      }

      return retval; 
   }

   // ----------------------------------------

   void OnNetMsg(eNetListenMsgs msg, DWORD data)
   {
      ConfigSpew("netplay_ui_spew",("Join state is %d\n",mJoinState)); 
      switch (msg)
      {
         case kNetMsgHi:
            if (mJoinState == kJoining)
            {
               mJoinState = kJoined; 
               mpPanel->SpoofUserClick(cDPCMultiplayUI::kNext); 
            }
            break; 

         case kNetMsgRejected:
            if (mJoinState == kJoining)
            {
               switch (data)
               {
                  case kNetRejectStarted:
                     Message(GetString("join_started"));
                     break;
                  case kNetRejectTooMany:
                     Message(GetString("join_too_many"));
                     break;
                  case kNetRejectVersion:
                     Message(GetString("join_version"));
                     break;
                  default:
                     Message(GetString("join_refused"));
                     break;
               }
               mJoinState = kWaiting; 
               gpNetMan->Leave(); 
            }
            break; 
      } 
   }

   // ----------------------------------------
   
   void Draw()
   {
      cParent::Draw(); 

      mHost.Draw(); 
      mChar.Draw(); 
   }

   static cMultiplayUIPage* Create(cDPCMultiplayUI* pUI)
   {
      return new cMPHostAddressPage(pUI); 
   }

};

sMultiplayUIPageDesc cMPHostAddressPage::gDesc = { "hostaddr" }; 

// A silly little boolean, so that we don't try to start the game
// multiply:
static BOOL gbIHaveStarted = FALSE;

//------------------------------------------------------------
// Class: cMPLobbyBase
// The base class lobby page
//


class cMPLobbyBase: public cMultiplayUIPage
{
protected: 
   typedef cMultiplayUIPage cParent; 

   int mNumPlayers;

   enum
   {
      kMaxPlayers = 4, 
   };



public:
   cMPLobbyBase(const sDesc& desc, cDPCMultiplayUI* pUI)
     : cParent(desc,pUI),
       mNumPlayers(0)
   {

   }; 


   // ----------------------------------------

   BOOL ExchangeData(eDataExchange op)
   {
      switch (op)
      {
         case kInitialize:
            mNumPlayers = gpNetMan->NumPlayers(); 
            gbIHaveStarted = FALSE;
            break; 

         case kSaveAndValidate:
            return gpNetMan->IsNetworkGame(); 
            break; 
      }

      return TRUE; 
   }

   // ----------------------------------------
   
   void Draw()
   {
      Rect* r = LGadBoxRect(Root()); 
      GUIErase(r); 

      cParent::Draw(); 


      int line = 1; // skip a line, as usual 
      char buf[256]; 
      sprintf(buf,"%s: %d",(const char*)GetString("num_players"),mNumPlayers); 
      DrawString(buf,LineRect(line++),kLeftAlign); 

      line++; // skip a line 

      // Put a box around the list of players 
      Rect box = LineRect(line); 
      int ymargin = RectHeight(&box)/3; 
      int xmargin = RectHeight(&box)/2; // @HACK: xmarign in terms of height, I know
      box.lr = LineRect(line+kMaxPlayers - 1).lr;
      box.ul.y -= ymargin; box.lr.y += ymargin; 
      Absolutize(&box); 

      GUIcompose c; 
      GUIsetup(&c,&box,GUIcomposeFlags(ComposeFlagClear|ComposeFlagRead),GUI_CANV_ANY); 
      gr_set_fcolor(guiStyleGetColor(Style(),StyleColorBorder)); 
      gr_box(0,0,grd_canvas->bm.w,grd_canvas->bm.h); 
      GUIdone(&c); 



      // Show the list of players
      for (int i = 0; i < mNumPlayers; i++)
      {
         int playnum = i + 1; 
         const char* name = gpNetMan->GetPlayerNameByNum(playnum); 
         const char* addr = gpNetMan->GetPlayerAddressByNum(playnum);

         if (!name) name = ""; 
         if (!addr) addr = ""; 

         Rect liner = LineRect(line++); 
         Rect r = liner; 
         int half = RectWidth(&r)/2; 
         // halve the width 
         r.lr.x -= half; 
         r.ul.x += xmargin; // Margin is really in Y, but so what. 
         DrawString(name,r,kLeftAlign); 
         
         // Draw the address on the right half of the screen
         r.ul.x += half; 
         r.lr.x += half - xmargin; 
         DrawString(addr,r,kLeftAlign);
         
      }
      
   }

   // ----------------------------------------

   void OnNetMsg(eNetListenMsgs msg, DWORD )
   {
      switch (msg)
      {
         case kNetMsgNetworkLost: 
            Message(GetString("lost_network")); 
            mDesc.flags |= kNoNextButton; 
            mpPanel->SetupButtons(); 
            // fall through 
         case kNetMsgPlayerLost: 
         case kNetMsgPlayerInfo:
            // Force an update in OnFrame below
            mNumPlayers = -1;
            break; 
      }
   }

   // ----------------------------------------

   void OnFrame()
   {
      cParent::OnFrame(); 

      int num = gpNetMan->NumPlayers();
      if (num != mNumPlayers)
      {
         mNumPlayers = num; 
         Draw(); 
      }
   }

};

//------------------------------------------------------------
// Class cMPHostLobbyPage
// Host-side lobby 
//

class cMPHostLobbyPage: public cMPLobbyBase
{
   static sDesc gDesc; 
   typedef cMPLobbyBase cParent; 

public:
   cMPHostLobbyPage(cDPCMultiplayUI* pUI)
      : cParent(gDesc,pUI)
   {

   }

   static cMultiplayUIPage* Create(cDPCMultiplayUI* pUI)
   {
      return new cMPHostLobbyPage(pUI); 
   }



}; 

sMultiplayUIPageDesc cMPHostLobbyPage::gDesc = { "host_lobby" }; 

//------------------------------------------------------------
// Class cMPClientLobbyPage
// Host-side lobby 
//

class cMPClientLobbyPage: public cMPLobbyBase
{
   static sDesc gDesc; 
   typedef cMPLobbyBase cParent; 

public:
   cMPClientLobbyPage(cDPCMultiplayUI* pUI)
      : cParent(gDesc,pUI)
   {

   }

   static cMultiplayUIPage* Create(cDPCMultiplayUI* pUI)
   {
      return new cMPClientLobbyPage(pUI); 
   }

}; 

sMultiplayUIPageDesc cMPClientLobbyPage::gDesc = { "client_lobby" }; 


//------------------------------------------------------------
// Class: cMPStartPage
// The start game page
//

//
// Start Game network message
//
// When the host pushes the Start Game button, he sends this message to
// all the other players. This tells them to start, if they haven't done
// so already.
//
static cNetMsg *gpStartGameMsg = NULL;

static void handleStartGame()
{
   if (!gbIHaveStarted)
   {
      gbIHaveStarted = TRUE;
      SwitchToDPCInitGame(FALSE); 
      MoviePanel("cs1.avi"); 
   }
}

static sNetMsgDesc sStartGameDesc =
{
   kNMF_MetagameBroadcast,
   "StartGame",
   "Start Game",
   NULL,
   handleStartGame,
   {{kNMPT_End}}
};


class cMPStartPage: public cMultiplayUIPage
{
protected: 
   static sDesc gHostDesc; 
   static sDesc gClientDesc; 

   typedef cMultiplayUIPage cParent; 

   static BOOL gmbHost;


   enum eButtons
   {
      kStart,
      kLoad, 
      kNumButtons,
      kHostNumButtons = kNumButtons,
      kClientNumButtons = kLoad, 
   }; 

   enum
   {
      kTopLine = 3,
   };

   LGadButtonList mButtons;
   cStr mStrings[kNumButtons]; 
   Rect mRects[kNumButtons];
   sDrawElem mElems[kNumButtons]; 
   int mNumButtons; 

   // Go from a button idx to a resource name. 
   static const char* ButtResName(int i)
   {
      static const char* names[] = { "start_game", "load_game" }; 
      return names[i]; 
   }

   static bool blist_cb(ushort action, int button, void*, LGadBox* )
   {
      if (!(action & BUTTONGADG_LCLICK))
         return FALSE; 

      switch (button)
      {
         case kStart:
            if (gmbHost)
               // I'm the host; tell everyone else to start, too...
               gpStartGameMsg->Send(OBJ_NULL);
            handleStartGame();
            break; 
            
         case kLoad:
            SwitchToDPCLoadGameMode(TRUE); 
            break; 
      }


      return FALSE; 
   }

public:
   cMPStartPage(BOOL host, cDPCMultiplayUI* pUI)
      : cParent(host ? gHostDesc : gClientDesc,pUI), 
        mNumButtons(host ? kHostNumButtons : kClientNumButtons)
   {
      memset(&mButtons,0,sizeof(mButtons)); 
      gmbHost = host;
   }; 

   // ----------------------------------------

   void InitUI()
   {
      cParent::InitUI(); 

      Rect area = UsableArea(); 

      // Get the strings.  This will totally break if they need to wrap 
      for (int i = 0; i < mNumButtons; i++)
      {
         mStrings[i] = GetString(ButtResName(i)); 
         
         // Fill out the draw element 
         mElems[i].SetText(mStrings[i]); 
         mElems[i].draw_flags |= BORDER(DRAWFLAG_BORDER_OUTLINE); 
      }

      for (i = 0; i < mNumButtons; i++)
         mRects[i] = LineRect(i*2+kTopLine); 

      // Create the button list 
      LGadButtonListDesc bdesc = 
      {
         mNumButtons, mRects, mElems, blist_cb, 0, 0,
      }; 

      LGadCreateButtonListDesc(&mButtons,Root(),&bdesc); 
      setup_box(VB(&mButtons)); 
   }

   // ----------------------------------------

   void TermUI()
   {
      LGadDestroyButtonList(&mButtons); 

      cParent::TermUI(); 
   }


   // ----------------------------------------
   
   void Draw()
   {
      cParent::Draw(); 
      LGadDrawBox(VB(&mButtons),NULL); 

   }

   // ----------------------------------------

   BOOL ExchangeData(eDataExchange )
   {
      return TRUE; 
   }

   // ---------------------------------------- 

   static cMultiplayUIPage* CreateHost(cDPCMultiplayUI* pUI)
   {
      return new cMPStartPage(TRUE,pUI); 
   }

   static cMultiplayUIPage* CreateClient(cDPCMultiplayUI* pUI)
   {
      return new cMPStartPage(FALSE,pUI); 
   }


};

sMultiplayUIPageDesc cMPStartPage::gHostDesc = { "host_start", kNoNextButton }; 
sMultiplayUIPageDesc cMPStartPage::gClientDesc = { "client_start", kNoNextButton }; 
BOOL cMPStartPage::gmbHost = FALSE;




//------------------------------------------------------------
// The global page factory table 
//

typedef cMultiplayUIPage* (*tPageFactory)(cDPCMultiplayUI* ); 

static tPageFactory gFactories[kNumPages] = 
{
   NULL,                         // Null
   cMPFrontPage::Create,         // Front page
   cMPGameNamePage::Create,      // Game Name
   cMPHostAddressPage::Create,   // Host address
   cMPHostLobbyPage::Create,     // Host Lobby
   cMPClientLobbyPage::Create,   // Client Lobby
   cMPStartPage::CreateHost,     // Host Start Game
   cMPStartPage::CreateClient,   // Client Start Game
}; 



//------------------------------------------------------------
// Class: cDPCMultiplayUI
// (Implementation)


static int default_page_list[] = { kFrontPage }; 

cDPCMultiplayUI::cDPCMultiplayUI()
   : cParent(&gDesc),
     mPageIdx(0),
     mpPageList(default_page_list),
     mpPage(NULL),
     mPageDelta(0)
{
   memset(&mScreenRoot,0,sizeof(mScreenRoot)); 
}

cDPCMultiplayUI::~cDPCMultiplayUI()
{
   delete mpPage; 
}

////////////////////////////////////////

void cDPCMultiplayUI::InitUI()
{
   cParent::InitUI(); 

   // Listen to all net messages
   mNetListener = gpNetMan->Listen(net_cb,kNetMessageInterests,this); 
      
   Rect& screenr = mRects[(int)kScreen]; 
   LGadSetupSubRoot(&mScreenRoot,LGadCurrentRoot(),screenr.ul.x,screenr.ul.y,RectWidth(&screenr),RectHeight(&screenr)); 

   if (!mpPage)
      mpPage = CreatePage(mpPageList[mPageIdx]); 

   SetupPage(FALSE); // setup page, but don't draw
}

void cDPCMultiplayUI::TermUI()
{
   CleanupPage(); 
   LGadDestroyRoot(&mScreenRoot); 
   delete mpPage;
   mpPage = NULL; 

   gpNetMan->Unlisten(mNetListener); 
   cParent::TermUI(); 
}

////////////////////////////////////////


void cDPCMultiplayUI::RedrawDisplay()
{
   LGadRoot* root = LGadCurrentRoot(); 
   // Redraw all gadgets on the screen,
   region_expose(LGadBoxRegion(root),LGadBoxRect(root)); 
   cParent::RedrawDisplay(); 
   if (mpPage)
      mpPage->Draw(); 
}

////////////////////////////////////////

void cDPCMultiplayUI::OnButtonList(ushort action, int button)
{
   if (!(action & BUTTONGADG_LCLICK))
      return ;


   switch (button)
   {
      case kPrev:
         if (mPageIdx > 0)
         {
            if (mpPage)
               mpPage->ExchangeData(kAbort); 

            // Go to previous page
            mPageDelta = -1; 
         }
         break; 

      case kNext:
         Assert_(mpPage); 

         if (mpPage->Desc().flags & kNoNextButton)
            break; 

         if (mpPage->ExchangeData(kSaveAndValidate))
         {
            // Go to next page
            mPageDelta = +1; 
         }
         break; 

      case kCancel:
         OnEscapeKey(); 
         break; 
   }
}

////////////////////////////////////////

void cDPCMultiplayUI::SpoofUserClick(int button)
{
   OnButtonList(BUTTONGADG_LCLICK,button); 
}

////////////////////////////////////////

void cDPCMultiplayUI::ChangePage()
{
   if (mPageDelta == 0)
      return; 

   int delta = mPageDelta; 
   mPageDelta = 0; 

   // can't go back beyond first page
   if (mPageIdx + delta < 0)
      return ; 

   CleanupPage(); 
   delete mpPage; 
   mPageIdx += delta; 
   mpPage = CreatePage(mpPageList[mPageIdx]);

   if (!mpPage) // Last page, exit
   {
      mpPanelMode->Exit(); 
      return;
   }

   SetupPage(); 
}

////////////////////////////////////////

void cDPCMultiplayUI::OnLoopMsg(eLoopMessage msg, tLoopMessageData data)
{
   cParent::OnLoopMsg(msg,data);

   switch (msg)
   {
   case kMsgNormalFrame:
      ChangePage(); 
      if (mpPage)
         mpPage->OnFrame(); 
      break; 
   }; 
   
}


////////////////////////////////////////
   
void cDPCMultiplayUI::SetupPage(BOOL draw)
{
   if (!mpPage)
      return ;
   mpPage->InitUI(); 
   mpPage->ExchangeData(kInitialize); 


   SetupButtons(); 


   if (draw)  
      RedrawDisplay();  // A little clunky, but hey
}

void cDPCMultiplayUI::CleanupPage()
{
   if (mpPage)
      mpPage->TermUI(); 
   GUIErase(LGadBoxRect(&mScreenRoot)); 
}

////////////////////////////////////////


void cDPCMultiplayUI::SetupButtons()
{
   const sMultiplayUIPageDesc& desc = mpPage->Desc(); 

   // Change button text as appropriate
   if (desc.flags & kNoNextButton)
      mElems[(int)kNext].draw_data = ""; 
   else 
      mElems[(int)kNext].draw_data = (void*)(const char*)mStrings[(int)kNext]; 

   if (mPageIdx <= 0)
      mElems[(int)kPrev].draw_data = ""; 
   else 
      mElems[(int)kPrev].draw_data = (void*)(const char*)mStrings[(int)kPrev]; 
}


////////////////////////////////////////

cMultiplayUIPage* cDPCMultiplayUI::CreatePage(int page_id)
{
   if (page_id >= kNumPages)
      return NULL; 
   tPageFactory fact = gFactories[page_id]; 
   if (fact)
      return fact(this); 

   return NULL; 
}

////////////////////////////////////////

void cDPCMultiplayUI::SetPages(int* pages, int idx)
{
   mpPageList = pages;
   mPageIdx = idx; 
}

////////////////////////////////////////

void cDPCMultiplayUI::OnEscapeKey()
{
   if (mpPage)
      mpPage->ExchangeData(kAbort);
   gpNetMan->Leave(); 
   cParent::OnEscapeKey(); 
}

////////////////////////////////////////

void cDPCMultiplayUI::net_cb(eNetListenMsgs msg, DWORD dat, void* data)
{
   cDPCMultiplayUI* us = (cDPCMultiplayUI*)data;
   us->OnNetMsg(msg,dat); 
}

void cDPCMultiplayUI::OnNetMsg(eNetListenMsgs msg, DWORD data)
{
   ConfigSpew("netplay_ui_spew",("Got Net Message %x, %d\n",msg,data)); 
   if (mpPage)
      mpPage->OnNetMsg(msg,data); 
}


//------------------------------------------------------------
// Global cDPCMultiplayUI stuff
//

//
// Multiplay UI descriptor 
//

static const char* button_names[] = 
{
   "prev",
   "cancel",
   "next",
}; 

sDarkPanelDesc cDPCMultiplayUI::gDesc = 
{
   "netplay",
   cDPCMultiplayUI::kNumButts,
   cDPCMultiplayUI::kNumRects,
   cDPCMultiplayUI::kNumButts,
   button_names, 
   NULL, // font
   0,
   kMetaSndPanelMain,
}; 


static cDPCMultiplayUI* gpMultiplayUI = NULL; 

void SwitchToDPCMultiplayUIMode(BOOL push) 
{
   if (gpMultiplayUI)
   {
      gpMultiplayUI->SetPages(default_page_list); 
      cAutoIPtr<IPanelMode> panel = gpMultiplayUI->GetPanelMode(); 
      panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
   }
}



void DPCMultiplayUIInit()
{
   gpMultiplayUI = new cDPCMultiplayUI; 
   gpStartGameMsg = new cNetMsg(&sStartGameDesc);
}

void DPCMultiplayUITerm()
{
   delete gpMultiplayUI; 
   delete gpStartGameMsg;
   gpMultiplayUI = NULL; 
   gpNetMan->Release(); 
}




