/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkoptmn.cpp,v 1.20 1999/08/27 13:04:56 mahk Exp $
#include <shkoptmn.h>
#include <string.h>
#include <dev2d.h>
#include <keydefs.h>

#include <shkmenu.h>
#include <shksvbnd.h>
#include <appagg.h>
#include <drkpanl.h>

#include <appsfx.h>
#include <gamescrn.h>
#include <scrnmode.h>

#include <resapi.h>
#include <imgrstyp.h>
#include <strrstyp.h>

#include <gcompose.h>
#include <questapi.h>
#include <drkdiff.h>
#include <config.h>
#include <uigame.h>

#include <panltool.h>
#include <appapi.h>
#include <mprintf.h>
#include <campaign.h>

#include <gadslide.h>
#include <gen_bind.h>
#include <lgd3d.h>

#include <command.h>
#include <gamma.h>
#include <objmodel.h>
#include <metasnd.h>

#include <simman.h>
#include <simdef.h>
#include <netman.h>

#include <volconv.h>

#include <shkdiff.h>

#include <math.h>

// Include this last. 
#include <dbmem.h>


#define kVolumeSliderNotches 25
#define kOneOverVolumeSliderNotches ((1.0)/(float)kVolumeSliderNotches)

static void TouchGamma (float gamma)
{
   g_gamma = gamma;
   gamma_update();

}

//is the difficulty setting greyed out or not?
static BOOL g_diff_active = TRUE;

//starting difficulty
extern int g_diff; 

#define MIN_DIFF 1 

inline int DIFF2BUTT(int x)
{
   // boundary cases
   if (x < kShockFirstDiff)
      x = kShockFirstDiff;
   if (x >= kShockLimDiff)
      x = kShockDiffNormal;

   return x - 1; 
}

#define BUTT2DIFF(x) ((x) + 1)


EXTERN BOOL g_zbuffer_toggle;
extern int sfx_use_channels;


/////////////////

#define MAX_NUM_BINDABLE 64
#define QUICKCONFIG_BINDNUM 15 /*number of quick configurable commands*/
#define NUM_BIND_SLOTS 13
//max cmds we will display per a given cmd
#define MAX_BINDS_PER_CMD 3

/////////////////

#define SLIDER_BMPRES 0
#define BINDUP_BMPRES 1
#define BINDDN_BMPRES 2

#define NUM_LIST 13
#define FILL_SUBPANEL 666

#define LRMOD(x) (!strcmp (x, "lalt") || !strcmp (x, "ralt") || \
                  !strcmp (x, "lctrl") || !strcmp (x, "rctrl") || \
                  !strcmp (x, "lshift") || !strcmp (x, "rshift"))



#define MIN_RES_X 640
#define MIN_RES_Y 480
#define MAX_RES_Y 768

//all 8-bit for software
static short soft_modes[] = {
   GRM_400x300x8,
   GRM_512x384x8,
   GRM_640x400x8, 
   GRM_640x480x8,
   GRM_800x600x8,
   -1
};

//#define CanChangeEAX() (SFX_Is_EAX_Available())
#define CanChangeEAX() (SFX_Is_EAX_Available() && (SFX_GetSoundDevice () == SFXDEVICE_A3D))

static BOOL InSim()
{
   AutoAppIPtr(SimManager); 
   return (pSimManager->LastMsg() & (kSimInit | kSimResume)) != 0;
}

#define CanChangeSoundDeviceNow() (!InSim())


BOOL CanChangeDifficultyNow() 
{
   AutoAppIPtr(NetManager);
   return !pNetManager->IsNetworkGame(); 
}


static const DrawElement def_draw_elem = 
{ 
   0, 0, 0, // type, data, data2
   0, 0, // fcolor bcolor
   INTERNAL(DRAWFLAG_INT_TRANSP),  // flags
}; 

static void InitDrawElem(DrawElement* elem)
{
   *elem = def_draw_elem; 
}


//------------------------------------------------------------
// OPTIONS MENU
//
class cShockOptions; 
typedef class cShockOptions cOptions; 

static cOptions *gpOptions = NULL; 


class cShockOptions: public cDarkPanel
{
   static sDarkPanelDesc gDesc; 

public:
   cOptions() : cDarkPanel(&gDesc)
   {
      memset(&mTabButtons,0,sizeof(mTabButtons)); 
      memset(mSubPanelButtons,0,sizeof(mSubPanelButtons)); 
      memset(&mListButtons,0,sizeof(mListButtons)); 
      memset(&mBindButtons,0,sizeof(mBindButtons)); 
      memset(&mBindScrollers,0,sizeof(mBindScrollers)); 
      memset(mSliders,0,sizeof(mSliders)); 
   }


   void SetInitialSub ()
   {
      mCurSub = kSubControls;
   }

   //Will stuff retrieve_num many controls bound to cmd into the dest
   //string array, and will return how many actually got stuff
   int GetCmdBinds (const char *cmd, cStr *dest, int retrieve_num, cStr* raw_dest = NULL)
   {
      char control_buf[64];
      char controls[4][32];
      long num_controls, i, cur_control;

      g_pInputBinder->GetControlFromCmdStart ((char *)cmd, control_buf);


      for (cur_control = 0; *control_buf != '\0' && cur_control < retrieve_num;) {
         g_pInputBinder->DecomposeControl (control_buf, controls, &num_controls);
         //we don't want to throw in the l/r mods
         for (i = 0; i < num_controls && !LRMOD (controls[i]); i++);

         //no l/r's, this control is cool. fetch names
         if (i == num_controls) 
         {
            cStr& cur_dest = dest[cur_control]; 

            cStr temp_raw;  // We might not have a raw_dest
            cStr& cur_raw = (raw_dest) ? raw_dest[cur_control] : temp_raw; 
            
            cur_dest = "";
            cur_raw = ""; 
            for (i = 0; i < num_controls; i++) 
            {
               if (i)
               {
                  cur_dest += "+";
                  cur_raw  += "+"; 
               }

               cStr str = FetchUIString ("controls", controls[i], mResPath);

               //does this key have a translated string?
               cur_dest += (*str) ? str : controls[i]; 
               cur_raw  += controls[i]; 
            }
            
            cur_control++;
         }

         g_pInputBinder->GetControlFromCmdNext (control_buf);
      }

      return cur_control;
   }

   enum
   {
      kDone,
      kNumButts,
      kSubFillRect = kNumButts,
      kNumRects
   };

   enum
   {
      kTab0,
      kTab1,
      kTab2,
      kTab3,
      kNumTabs
   };

   enum {
      //control buttons
      kFirstControlButt,
      kCustomize = kFirstControlButt,
      kLookspring,
      kMouseInvert,
      kFreelook,
      kJoyRotate,

      kFirstControlRect, 
      kMouseSensSliderRect = kFirstControlRect, 
      kMouseSensTextRect,
      kLimControlRect, 
      kNumControlButts = kFirstControlRect - kFirstControlButt,
      kNumControlRects = kLimControlRect - kFirstControlButt,

      //video buttons
      kFirstBasicVideoButt = kLimControlRect,
      kScreenRes = kFirstBasicVideoButt,
      kHardwareDriver,

      kFirstBasicVideoRect,
      kGammaSliderRect = kFirstBasicVideoRect,
      kGammaSliderText,
      kLimBasicVideoRect,
      kNumBasicVideoButts = kFirstBasicVideoRect - kFirstBasicVideoButt,
      kNumBasicVideoRects = kLimBasicVideoRect - kFirstBasicVideoRect,

      kFirstAdvancedVideoButt = kLimBasicVideoRect, 
      kHardwareDriverOld = kFirstAdvancedVideoButt,
      kZBufferToggle,
      kBasicVidOptions,
      kLimAdvancedVideoRect,
      kNumAdvancedVideoRects = kLimAdvancedVideoRect - kFirstAdvancedVideoButt,

      //audio buttons/rects
      kFirstAudioButt = kLimAdvancedVideoRect, 
      kSpeakerTest = kFirstAudioButt,
      kStereoToggle,
      kA3DToggle,
      kAudioChannels,
      kAudioEAXToggle,

      kFirstAudioRect,
      kVolumeSliderRect = kFirstAudioRect,
      kVolumeSliderText,
      k3dVolumeSliderRect,
      k3dVolumeSliderText,
      kMusicSliderRect,
      kMusicSliderText,

      kLimAudioRect,
      kNumAudioButts = kFirstAudioRect - kFirstAudioButt,
      kNumAudioRects = kLimAudioRect - kFirstAudioButt,

      //game buttons  (Note irregularity) 
      kFirstGameRect = kLimAudioRect,
      kDiffTextRect = kFirstGameRect, 
      kFirstGameButt,
      kDiff0 = kFirstGameButt,
      kDiff1,
      kDiff2,
      kDiff3,
      kLimGameRect,
      kNumGameButts = kLimGameRect - kFirstGameButt,
      kNumGameRects = kLimGameRect - kFirstGameRect,

      //bind buttons
      kFirstBindButt = kLimGameRect, 
      kBindLoad = kFirstBindButt,
      kBindBack,
      kBindSave,
      kQuickConfigure,
      kBind,
      kBindClear,
      kBindButt0,//0 to NUM_BIND_SLOTS - 1

      kBindScrollUp = kBindButt0 + NUM_BIND_SLOTS,
      kBindScrollDown,
      kLimBindRects,
      kNumBindButts = kBindButt0 - kFirstBindButt,
      kNumBindRects = kLimBindRects - kFirstBindButt,

      kFirstListButt = kLimBindRects,
      kListBack = kFirstListButt,
      kList0,//13 of these, just add what is needed.
      kNumListButts = kList0 - kFirstListButt,
      kNumListRects = NUM_LIST + kNumListButts,

      
      kNumSubPanelRects = kNumControlRects + kNumBasicVideoRects + kNumAdvancedVideoRects
                          + kNumAudioRects + kNumGameRects + kNumBindRects + kNumListRects,
   };

   enum {
      kVolumeSlider,
      k3dVolumeSlider, 
      kMusicSlider,
      kNumVolumeSliders, 
      kMouseSensSlider = kNumVolumeSliders,
      kGammaSlider,
      kNumSubPanelSlides
   };


   enum {
      kSubControls,
      kSubBasicVideo,
      kSubAdvancedVideo,
      kSubAudio,
      kSubGame,
      kSubBind,
      kSubList,
      kNumSubs
   };

   enum {
      kMiscStrOn,
      kMiscStrOff,
      kMiscStrEmpty,
      kMiscStrLow,
      kMiscStrHigh,
      kMiscStrLeftRight,
      kMiscStrForwardBackward,
      kNumMiscStrs
   };



   static int gFirstSubRect[];  


protected:
   guiStyle mTextStyle;
   guiStyle mGreyStyle;

   cStr mMiscStrs[kNumMiscStrs]; 

   IRes *mButtBmpRes[3][4];//4 button draw states
   DrawElement mButtDrawElem[3];


   LGadButtonList mTabButtons; 
   LGadButtonListDesc mTabDesc;
   cRectArray mTabRects;
   DrawElement mTabElems[kNumTabs];
   cStr mTabStrs[kNumTabs]; 

   int mCurSub;
   int mSwap;
   LGadButtonList mSubPanelButtons[kNumSubs];
   LGadButtonListDesc mSubPanelDesc[kNumSubs];
   cRectArray mSubPanelRects;
   DrawElement mSubPanelElems[kNumSubPanelRects];
   cStr mSubPanelStrs[kNumSubPanelRects];

   //list
   int mListSub;
   int mListPrevPick;
   int mTopList;
   int mNumListTotal;
   LGadButtonList mListButtons;
   LGadButtonListDesc mListButtonDesc;
   DrawElement mListButtonElems[NUM_LIST];
   cStr mListButtonStrs[NUM_LIST];

   //video-related data
   BOOL mNumVidDevices;
   int mSelectedRes;
   int m3dDriver;

   //binding-related data
   int mTopBind;
   int mNumBindable;
   guiStyle mBindStyle;

   LGadButtonList mBindButtons;
   LGadButtonListDesc mBindButtonDesc;
   DrawElement mBindButtonElems[NUM_BIND_SLOTS];
   cStr mBindButtonStrs[MAX_NUM_BINDABLE];
   cStr mBindCmd[MAX_NUM_BINDABLE];
   bool mBindButtsFilled[MAX_NUM_BINDABLE];

   LGadButtonList mBindScrollers;
   LGadButtonListDesc mBindScrollerDesc;

   int m_cookie;


   //slider stuff
   LGadSlider mSliders[kNumSubPanelSlides];
   bool mSlidersOn[kNumSubPanelSlides];
   DrawElement mSliderBaseBmp;

   ulong m_old_context;

   //slider variables
   int m_sens;
   int m_volume[kNumVolumeSliders];
   int m_gamma;


   typedef bool (*ButtListCB)(ushort, int, void *, LGadBox *);


   //////////////////////////////////////////////////////

   void SetBoxData(LGadBox* box)
   {
      LGadBoxSetUserData (box, this);

      int flags = LGadBoxFlags(box); 
      LGadBoxSetFlags(box, flags | BOXFLAG_CLEAR); 
   }

   typedef int (*vol_get_func)(void); 

   uint MillibelToSlider(int vol)
   {
      uint retval = (uint) ( VolMillibelToLinear((float)vol) * kVolumeSliderNotches); 
      if (retval >= kVolumeSliderNotches)
         retval = kVolumeSliderNotches - 1;
      return retval; 

   }

   // get slider volume, in slider notches
   uint GetVolume(uint which)
   {
      static vol_get_func funcs[] = 
      {
         SFX_GetMasterVolume,
         SFX_Get3dVolume,
         SFX_GetMusicVolume, 
      };
      
      if (which == k3dVolumeSlider)
      {
         uint vol3d = MillibelToSlider(SFX_Get3dVolume()); 
         uint vol2d = MillibelToSlider(SFX_Get2dVolume()); 
         if (vol3d > vol2d)
         {
            vol2d = kVolumeSliderNotches - vol2d; 
            return (kVolumeSliderNotches + vol2d)/2; 
         }
         else
            return vol3d/2; 
      }
      
      int vol = funcs[which](); 
      return MillibelToSlider(vol); 
   }


   typedef void (*vol_set_func)(int); 

   // set volume, in slider notches 
   void SetVolume(int which, int vol)
   {
      static vol_set_func funcs[] = 
      {
         SFX_SetMasterVolume,
         SFX_Set3dVolume,
         SFX_SetMusicVolume, 
      };

      vol_set_func func = funcs[which];
      // the 3d volume slider is really a "mix" between 2d and 3d 
      if (which == k3dVolumeSlider)
      {
         int half = (kVolumeSliderNotches + 1)/2; 
         if (vol < half)
         {
            SFX_Set2dVolume(0); 
            func = SFX_Set3dVolume; 
            vol *= 2; 
         }
         else
         {
            SFX_Set3dVolume(0);  
            func = SFX_Set2dVolume;
            vol = (kVolumeSliderNotches - vol)*2; 
         }
      }

      Assert_(which < kNumVolumeSliders); 

      float fvol = VolLinearToMillibel((float)vol * kOneOverVolumeSliderNotches);
      
      func((int)fvol); 

   }

   static int gVolSliderRects[]; 


   void InitUI()
   {
      cDarkPanel::InitUI(); 

      //load in the tab and subpanel rectangles
      AppendRects ("optiontr", mTabRects);//tabs
      AppendRects ("optioncr", mSubPanelRects);//controls
      AppendRects ("optnbvr", mSubPanelRects);//basic video
      AppendRects ("optnavr", mSubPanelRects);//advanced video
      AppendRects ("optionar", mSubPanelRects);//audio
      AppendRects ("optiongr", mSubPanelRects);//game
      AppendRects ("optionbr", mSubPanelRects);//bindings
      AppendRects ("optionlr", mSubPanelRects);//list

      //load miscellaneous strings
      char buf[16];
      for (long i = 0; i < kNumMiscStrs; i++) {
         sprintf (buf, "misc_%d", i);
         mMiscStrs[i] = FetchUIString (panel_name, buf, mResPath);
      }

      //create our "tabs" at the top of the menu for the different major categories
      InitButtonList (&mTabDesc, &mTabRects[0], mTabElems, OnTabButton, mTabStrs, "tab_", kNumTabs, BUTTONLIST_RADIO_FLAG);
      LGadCreateButtonListDesc (&mTabButtons, LGadCurrentRoot(), &mTabDesc);   
      SetBoxData(VB(&mTabButtons));
      //muy sucky
      int sub_tabs[] = {0, 1, 1, 2, 3, 0, 1};
      LGadRadioButtonSelect (&mTabButtons, sub_tabs[mCurSub]); 

      // default to current style colors
      AssertMsg (GetCurrentStyle (), "No current style for diff defaults"); 
      memset (&mTextStyle, 0, sizeof (guiStyle));
      memcpy (mTextStyle.colors, GetCurrentStyle ()->colors, sizeof(mTextStyle.colors)); 
      uiGameLoadStyle ("opttext_", &mTextStyle, mResPath); 

      // style for greyed area
      AssertMsg (GetCurrentStyle (), "No current style for diff defaults"); 
      memcpy (&mGreyStyle, GetCurrentStyle (), sizeof(guiStyle)); 
      uiGameLoadStyle ("grey_", &mGreyStyle, mResPath); 
      
      //get our slider button bitmap
      AutoAppIPtr (ResMan);
      for (i = 0; i < 4; i++) {
         mButtBmpRes[SLIDER_BMPRES][i] = pResMan->Bind ("slidbutt", RESTYPE_IMAGE, NULL, mResPath);
         if (!mButtBmpRes[SLIDER_BMPRES][i])
            AssertMsg (0, "Error loading button bitmap");
      }
      InitDrawElem(&mButtDrawElem[SLIDER_BMPRES]); 
      mButtDrawElem[SLIDER_BMPRES].draw_data = (void *)mButtBmpRes[SLIDER_BMPRES];
      mButtDrawElem[SLIDER_BMPRES].draw_type = DRAWTYPE_RESOFFSET;

      InitDrawElem(&mSliderBaseBmp); 
      mSliderBaseBmp.draw_type = DRAWTYPE_RES;
      mSliderBaseBmp.draw_data = (void *)pResMan->Bind ("slidbase", RESTYPE_IMAGE, NULL, mResPath);

      for (i = 0; i < kNumSubPanelSlides; i++)
         mSlidersOn[i] = FALSE;

      //set sliders initial vals
      m_sens = int((atof (g_pInputBinder->ProcessCmd ("echo $mouse_sensitivity")) - 0.5) * 2.0);
      
      for (int v = 0; v < kNumVolumeSliders; v++)
         m_volume[v] = GetVolume(v); 
      m_gamma = 20-(20.0 / (MAX_GAMMA - MIN_GAMMA)) * (g_gamma - MIN_GAMMA);

      m3dDriver = 0;
      if (config_is_defined("d3d_driver_index"))
         config_get_int("d3d_driver_index", &m3dDriver);

      //initialize our subpanels
      mSwap = mCurSub;
      mCurSub = -1;
      InitControlOptions ();
      InitVideoOptions ();
      InitAudioOptions ();
      InitGameOptions ();
      InitBindOptions ();
      InitListOptions ();

      //our handler for handling keyboard arrows and enter in the binding menu
      uiSlab *slab;
      uiGetCurrentSlab (&slab);

      uiInstallRegionHandler (slab->creg, UI_EVENT_KBD_COOKED, StaticBindKeyHandler, NULL, &m_cookie);
   }

   //////////////////////////////////////////////////////

   void TermUI()
   {
      LGadDestroyButtonList(&mTabButtons); 

      memset(&mTabElems,0,sizeof(mTabElems)); 
      for (int i = 0, j; i < kNumTabs; i++)
         mTabStrs[i] = ""; 

      TermSubPanel ();
      uiGameUnloadStyle (&mBindStyle); 
      uiGameUnloadStyle (&mTextStyle);

      for (i = 0; i < 3; i++)
         for (j = 0; j < 4; j++)
            mButtBmpRes[i][j]->Release ();

      ((IRes *)(mSliderBaseBmp.draw_data))->Release ();

      //save the current bindings upon exit of the options menu
      //change to game context
      ulong old_context;
      g_pInputBinder->GetContext (&old_context);
      g_pInputBinder->SetContext (HK_GAME_MODE,FALSE);
      cStr header = FetchUIString (panel_name, "bnd_header", mResPath);
      g_pInputBinder->SaveBndFile ("user.bnd", (char *)(const char *)header);
      //plop in input variable control settings
      BndAppendControls ();

      g_pInputBinder->SetContext (HK_GAME2_MODE,FALSE);
      g_pInputBinder->SaveBndFile ("user2.bnd", (char *)(const char *)header);
      g_pInputBinder->SetContext (old_context,FALSE);

      //remove the binding keyboard handler
      uiSlab *slab;
      uiGetCurrentSlab (&slab);
      uiRemoveRegionHandler (LGadBoxRegion (slab->creg), m_cookie);

      cDarkPanel::TermUI();
   }


   //////////////////////////////////////////////////////

   void TermSubPanel ()
   {
      LGadDestroyButtonList (&mSubPanelButtons[mCurSub]); 

      if (mCurSub == kSubBind) {
         LGadDestroyButtonList (&mBindButtons); 
         LGadDestroyButtonList (&mBindScrollers); 
      }

      memset (mSubPanelElems, 0, kNumSubPanelRects * sizeof(DrawElement)); 
      for (int i = 0; i < kNumSubPanelRects; i++)
         mSubPanelStrs[i] = ""; 

      //if there are any sliders, kill them
      DestroyAllSliders ();

      mTabRects.SetSize (0);
      mSubPanelRects.SetSize (0);
   }

   //////////////////////////////////////////////////////

   void TermSpecial ()
   {
      if (mCurSub == kSubBind) {
         LGadDestroyButtonList (&mBindButtons); 
         LGadDestroyButtonList (&mBindScrollers); 
      }

      else if (mCurSub == kSubList) {
         if (mListSub == kSubBasicVideo) {
            SetVidRes ();
         }

         LGadDestroyButtonList (&mListButtons);
      }
   }

   //////////////////////////////////////////////////////

   void OnLoopMsg (eLoopMessage msg, tLoopMessageData msg_data)
   {  
      if (SFX_GetSoundDevice () == SFXDEVICE_A3D) {
         mxs_vector origin = {0.0, 0.0, 0.0};
         SFX_Frame (&origin, (mxs_angvec *)&origin);
      }
      else
         SFX_Frame (NULL, NULL);

      if (mSwap != -1) {
         SwapSubPanel (mSwap);
         mSwap = -1;
      }

      //check slider data
      //volume

      switch (mCurSub)
      {
         case kSubAudio:
         {
            for (int i = 0; i < kNumVolumeSliders; i++)
               SetVolume(i,m_volume[i]); 
         }
         break; 

         case kSubControls:
         {
            char buf[32];
            sprintf (buf, "mouse_sensitivity %8.8f", ((double)m_sens / 2.0) + 0.5);//slide goes from 0.5 (slow) to 10.5 (fast)
            g_pInputBinder->ProcessCmd (buf);
         }
         break; 


         case kSubBasicVideo:
         {
            //gamma
            static int prev_gamma;
            if (prev_gamma != m_gamma) {
               TouchGamma ((((MAX_GAMMA - MIN_GAMMA) / 20.0) * (float)(20-m_gamma)) + MIN_GAMMA);
               prev_gamma = m_gamma;
            }
         }
         break; 

      }
   }

   //////////////////////////////////////////////////////

   void InitControlOptions ()
   {
      InitButtonList(&mSubPanelDesc[kSubControls], &mSubPanelRects[(int)kCustomize], &mSubPanelElems[(int)kCustomize], OnSubPanelButton,
         &mSubPanelStrs[(int)kCustomize], "control_", kNumControlButts, 0);

      //set starting button strings

      // watch me factor code like the dickens. 
      static const char* cmds[] = 
      { 
         "",
         "echo $lookspring", 
         "echo $mouse_invert",
         "echo $freelook",
         "echo $joy_rotate",
      };

      for (int i = kLookspring; i < kFirstControlRect; i++)
      {
         int idx = i - kFirstControlButt; 

         char *str = g_pInputBinder->ProcessCmd((char*)cmds[idx]); // command value
         cStr& substr = mSubPanelStrs[i];  // button string
         DrawElement& elem = mSubPanelElems[i]; // button elem
         char res_str[16]; 
         sprintf(res_str,"control_%d",i); 

         BOOL state = atof(str) != 0.0; 
         const char* state_str; 

         if (i == kJoyRotate) // exception 
         {
            if (g_joystickActive)
               state_str = mMiscStrs[(state) ? kMiscStrLeftRight : kMiscStrForwardBackward ]; 
            else
            {
               *res_str = '\0';
               state_str = NULL; 
            }
         }
         else
            state_str = mMiscStrs[(state) ? kMiscStrOn : kMiscStrOff ]; 

         SetUIString(substr,elem,res_str,(char*)state_str); 
      }


   }

   //////////////////////////////////////////////////////

   void BndAppendControls ()
   {
      FILE *fp = fopen ("user.bnd", "at");
      if (fp)
      {
         fprintf (fp, "\n");

         char *controls[] = {"lookspring", "mouse_invert", "freelook", "mouse_sensitivity", "\0"};
         char str[32];
         char **p_control = controls;

         while (**p_control) {
            sprintf (str, "echo $%s", *p_control);
            fprintf (fp, "%s %s\r\n", *p_control, g_pInputBinder->ProcessCmd (str));
            p_control++;
         }

         fclose (fp);
      }
   }

   //////////////////////////////////////////////////////

   void InitVideoOptions ()
   {
      InitButtonList (&mSubPanelDesc[kSubBasicVideo], &mSubPanelRects[(int)kFirstBasicVideoButt], &mSubPanelElems[(int)kFirstBasicVideoButt],
         OnSubPanelButton, &mSubPanelStrs[(int)kFirstBasicVideoButt], "videob_", kNumBasicVideoButts, 0);

      int first = kFirstAdvancedVideoButt; 
      InitButtonList (&mSubPanelDesc[kSubAdvancedVideo], &mSubPanelRects[first], &mSubPanelElems[first],
         OnSubPanelButton, &mSubPanelStrs[(int)first], "videoa_", kNumAdvancedVideoRects, 0);


      SetVidResStr(); 

      //let's get some info about the video card
      mNumVidDevices = lgd3d_enumerate_devices ();

      sScrnMode mode = *GetGameScreenMode(); 

      if (mNumVidDevices > 0 && (mode.flags & kScrnMode3dDriver)) {
         g_zbuffer_toggle = TRUE;
         mode.flags |= kScrnMode3dDriver;
         SetUIString (mSubPanelStrs[(int)kZBufferToggle], mSubPanelElems[(int)kZBufferToggle], "videoa_1",
            (char *)(const char *)mMiscStrs[(g_zbuffer_toggle && (mode.flags & kScrnMode3dDriver) != 0) ? kMiscStrOn : kMiscStrOff]);
      }

      else {
         g_zbuffer_toggle = FALSE;
         mode.flags &= ~kScrnMode3dDriver;
         SetUIString (mSubPanelStrs[(int)kZBufferToggle], mSubPanelElems[(int)kZBufferToggle], "videoa_1",
            (char *)(const char *)mMiscStrs[kMiscStrOff]);
      }

      SetGameScreenMode (&mode);

   }

   //////////////////////////////////////////////////////

   void InitAudioOptions ()
   {
      InitButtonList (&mSubPanelDesc[kSubAudio], &mSubPanelRects[(int)kSpeakerTest], &mSubPanelElems[(int)kSpeakerTest], OnSubPanelButton,
         &mSubPanelStrs[(int)kSpeakerTest], "audio_", kNumAudioButts, 0);
   
      SetUIString (mSubPanelStrs[(int)kStereoToggle], mSubPanelElems[(int)kStereoToggle], "audio_1",
         (char *)(const char *)mMiscStrs[(SFX_StereoReversed ()) ? kMiscStrOn : kMiscStrOff]);

      SetUIString (mSubPanelStrs[(int)kA3DToggle], mSubPanelElems[(int)kA3DToggle], "audio_2",
         (char *)(const char *)mMiscStrs[(SFX_GetSoundDevice () == SFXDEVICE_A3D) ? kMiscStrOn : kMiscStrOff]);

      char buf[8];
      sprintf (buf, "%d", sfx_use_channels);
      mSubPanelStrs[(int)kAudioChannels] += buf;
      SetUIString (mSubPanelStrs[(int)kAudioChannels], mSubPanelElems[(int)kAudioChannels], "audio_3", buf);

      BOOL isEAXOn = SFX_Is_EAX_Enabled();
      SetUIString (mSubPanelStrs[(int)kAudioEAXToggle], mSubPanelElems[(int)kAudioEAXToggle], "audio_4",
         (char *)(const char *)mMiscStrs[isEAXOn ? kMiscStrOn : kMiscStrOff]);
   }

   //////////////////////////////////////////////////////

   void InitGameOptions ()
   {
      int first = kFirstGameButt;
      InitButtonList (&mSubPanelDesc[kSubGame], &mSubPanelRects[first], &mSubPanelElems[first], OnSubPanelButton,
         &mSubPanelStrs[first], "game_", kNumGameButts, BUTTONLIST_RADIO_FLAG);
   }

   //////////////////////////////////////////////////////

   void InitBindOptions ()
   {
      InitButtonList (&mSubPanelDesc[kSubBind], &mSubPanelRects[(int)kBindLoad], &mSubPanelElems[(int)kBindLoad], OnSubPanelButton,
         &mSubPanelStrs[(int)kBindLoad], "bind_", kNumBindButts, 0);

      //set up description for binding buttons. joy.
      //rearrange a few rects
      mTopBind = 0;
      cRectArray tmp_rects; 
      tmp_rects.SetSize (NUM_BIND_SLOTS);
      for (long i = 0; i < (NUM_BIND_SLOTS - 2); i++)
         tmp_rects[i] = mSubPanelRects[(int)kBindButt0 + 2 + i];
      tmp_rects[NUM_BIND_SLOTS - 2] = mSubPanelRects[(int)kBindScrollUp];
      tmp_rects[NUM_BIND_SLOTS - 1] = mSubPanelRects[(int)kBindButt0 + 1];
      for (i = 0; i < NUM_BIND_SLOTS; i++) 
      {
         mSubPanelRects[(int)kBindButt0 + 1 + i] = tmp_rects[i];
         InitDrawElem(&mBindButtonElems[i]); 
         mBindButtonElems[i].draw_type = DRAWTYPE_TEXT;
      }
      //description of binding radio buttons
      {
         LGadButtonListDesc tmp_desc = {NUM_BIND_SLOTS, (Rect *)&mSubPanelRects[(int)kBindButt0], 
            mBindButtonElems, OnBindButton, 0, BUTTONLIST_RADIO_FLAG};
         memcpy (&mBindButtonDesc, &tmp_desc, sizeof (LGadButtonListDesc));
      }

      // default to current style colors
      AssertMsg (GetCurrentStyle (), "No current style for diff defaults"); 
      memset (&mBindStyle, 0, sizeof (mBindStyle));
      memcpy (mBindStyle.colors, GetCurrentStyle()->colors, sizeof (mBindStyle.colors)); 
      uiGameLoadStyle("bind_", &mBindStyle, mResPath); 

      //load bind cmds
      cStr bind_cmd;
      char buf[16];
      mNumBindable = 0;
      while (1) {
         sprintf (buf, "bindcmd_%d", mNumBindable);
         bind_cmd = FetchUIString ("bindcmds", buf, mResPath);
   
         if (bind_cmd == "" || mNumBindable >= MAX_NUM_BINDABLE)
            break;

         mBindCmd[mNumBindable] = bind_cmd;
         mBindButtsFilled[mNumBindable] = FALSE;
         mNumBindable++;
      }

      FillBindStrs ();


      //load bitmaps for up/down scroll arrows
      LoadButtBmp (mButtBmpRes[BINDUP_BMPRES], &mButtDrawElem[BINDUP_BMPRES], "bup_");
      LoadButtBmp (mButtBmpRes[BINDDN_BMPRES], &mButtDrawElem[BINDDN_BMPRES], "bdn_");

      //description of binding scroll buttons
      {
         LGadButtonListDesc tmp_desc = {2, (Rect *)&mSubPanelRects[(int)kBindScrollUp], 
            &mButtDrawElem[BINDUP_BMPRES], OnScrollButton, 0, 0};
         memcpy (&mBindScrollerDesc, &tmp_desc, sizeof (LGadButtonListDesc));
      }

   }

   //////////////////////////////////////////////////////

   void InitListOptions ()
   {
      //description of back button
      {
         LGadButtonListDesc tmp_desc = {kNumListButts, (Rect *)&mSubPanelRects[(int)kListBack], 
            &mSubPanelElems[(int)kListBack], OnSubPanelButton, 0, 0};
         mSubPanelDesc[kSubList] = tmp_desc; 
      }

      mSubPanelStrs[(int)kListBack] = FetchUIString (panel_name, "bind_1", mResPath);

      DrawElement& backelem = mSubPanelElems[(int)kListBack]; 
      InitDrawElem(&backelem); 
      backelem.draw_data = (void *)(const char *)mSubPanelStrs[(int)kListBack];
      backelem.draw_type = DRAWTYPE_TEXT;

      //description of list buttons
      {
         LGadButtonListDesc tmp_desc = {NUM_LIST, (Rect *)&mSubPanelRects[(int)kList0], 
            mListButtonElems, OnListButton, 0, BUTTONLIST_RADIO_FLAG};
         mListButtonDesc = tmp_desc; 
      }

      mTopList = 0;
   }



   //////////////////////////////////////////////////////

   void FillBindStr (long bind_num, long butt_num)
   {
      if (bind_num >= mNumBindable) {
         static char *dummy = "";
         mBindButtonElems[butt_num].draw_data = dummy;
         return;
      }

      if (mBindButtsFilled[bind_num]) {
         mBindButtonElems[butt_num].draw_data = (void*)(const char*)mBindButtonStrs[bind_num];
         return;
      }
      mBindButtsFilled[bind_num] = TRUE;

      ulong old_context;
      g_pInputBinder->GetContext (&old_context);
      if (old_context != HK_GAME_MODE)
         g_pInputBinder->SetContext (HK_GAME_MODE,FALSE);

      //get our bindings
      char buf[64];
      sprintf (buf, "bindname_%d", bind_num);

      mBindButtonStrs[bind_num] = FetchUIString (panel_name, buf, mResPath);
      mBindButtonStrs[bind_num] += " = ";

      cStr controls[MAX_BINDS_PER_CMD];
      cStr raw_controls[MAX_BINDS_PER_CMD]; 
      int num = GetCmdBinds (mBindCmd[bind_num], controls, MAX_BINDS_PER_CMD, raw_controls);

      for (long i = 0; i < num; i++) {
         if (i != 0)
            mBindButtonStrs[bind_num] += ", ";
         mBindButtonStrs[bind_num] += controls[i];
      }

      //there were no controls
      if (i == 0)
         mBindButtonStrs[bind_num] += mMiscStrs[kMiscStrEmpty];
      else
      {
         // Copy the controls over to game2
         g_pInputBinder->SetContext(HK_GAME2_MODE,FALSE); 
         for (int i = 0; i < num; i++)
         {
            const char* cmd = mBindCmd[bind_num]; 
            const char* ctrl = raw_controls[i]; 

            // if cmd has a space, we have to quote it, because the binder is stupid
            cStr newcmd = "";  
            if (strchr(cmd,' ') != NULL)
            {
               newcmd += "\"";
               newcmd += cmd;
               newcmd += "\"";
               cmd = newcmd; 
            }

            // Check the config to make sure we can bind this stuff in game2
            BOOL can_bind = TRUE; 
            char varbuf[80]; 
            sprintf(varbuf,"%s_bind_game2",ctrl); 
            config_get_int(varbuf,&can_bind); 
            
            if (can_bind)
               g_pInputBinder->Bind((char*)ctrl,(char*)cmd); 
         }
      }
        
      if (old_context != HK_GAME2_MODE)
         g_pInputBinder->SetContext (old_context,FALSE);

      mBindButtonElems[butt_num].draw_data = (void*)(const char*)mBindButtonStrs[bind_num];
   }

   //////////////////////////////////////////////////////

   void FillBindStrs ()
   {
      AutoAppIPtr(ResMan); 
      IRes* ctrls = pResMan->Bind("controls",RESTYPE_STRING,NULL,mResPath); 
      ctrls->Lock(); 

      IRes* panel = pResMan->Bind(panel_name,RESTYPE_STRING,NULL,mResPath); 
      panel->Lock(); 

      ulong old_context;
      g_pInputBinder->GetContext (&old_context);
      g_pInputBinder->SetContext (HK_GAME_MODE,TRUE);


      for (long i = 0, j = mTopBind; i < NUM_BIND_SLOTS; i++, j++) {

         FillBindStr (j, i);
      }

      ctrls->Unlock(); 
      panel->Unlock();
      SafeRelease(ctrls);
      SafeRelease(panel);

      //change back to previous mode
      g_pInputBinder->SetContext (old_context,TRUE);

   }

   //////////////////////////////////////////////////////

   static bool StaticBindKeyHandler (uiEvent *p_event, Region *p_reg, void *state)
   {
      bool ret = FALSE;
      if (gpOptions->mCurSub == gpOptions->kSubBind && p_event->type == UI_EVENT_KBD_COOKED)
         ret = gpOptions->BindKeyHandler ((uiCookedKeyEvent *)p_event);
      return ret;
   }

   bool BindKeyHandler (uiCookedKeyEvent *event)
   {
      //only use up actions
      if (event->code & KB_FLAG_DOWN)
         return FALSE;


      BOOL redraw = TRUE;
      int cur_selected = LGadRadioButtonSelection (&mBindButtons);

      switch (event->code) {
         case KEY_UP:
         case KEY_PAD_UP:
            if (cur_selected > 0)
               LGadRadioButtonSelect (&mBindButtons, cur_selected - 1);
            else if (mTopBind > 0) {
               mTopBind--;
               FillBindStrs ();
            }
            else
               redraw = FALSE;
            
         break;


         case KEY_DOWN:
         case KEY_PAD_DOWN:
            if (cur_selected < (NUM_BIND_SLOTS - 1))
               LGadRadioButtonSelect (&mBindButtons, cur_selected + 1);
            else if (mTopBind < mNumBindable - NUM_BIND_SLOTS) {
               mTopBind++;
               FillBindStrs ();
            }
            else
               redraw = FALSE;

         break;

         case KEY_PGUP:
         case KEY_PAD_PGUP:
            mTopBind -= NUM_BIND_SLOTS;
            if (mTopBind < 0) {
               mTopBind = 0;
               LGadRadioButtonSelect (&mBindButtons, 0);
            }

            FillBindStrs ();

         break;

         case KEY_PGDN:
         case KEY_PAD_PGDN:
            mTopBind += NUM_BIND_SLOTS;
            if (mTopBind >= (mNumBindable - NUM_BIND_SLOTS)) {
               mTopBind = mNumBindable - NUM_BIND_SLOTS;
               LGadRadioButtonSelect (&mBindButtons, NUM_BIND_SLOTS - 1);
            }

            FillBindStrs ();

         break;

         
         case KEY_HOME:
         case KEY_PAD_HOME:
            LGadRadioButtonSelect (&mBindButtons, 0);
            if (mTopBind != 0) {
               mTopBind = 0;
               FillBindStrs ();
            }
         break;

         
         case KEY_END:
         case KEY_PAD_END:
            LGadRadioButtonSelect (&mBindButtons, NUM_BIND_SLOTS - 1);
            if (mTopBind != (mNumBindable - NUM_BIND_SLOTS)) {
               mTopBind = mNumBindable - NUM_BIND_SLOTS;
               FillBindStrs ();
            }
         break;

         
         case KEY_ENTER:
         case KEY_GREY_ENTER:
            OnSubPanelButtonList (BUTTONGADG_LCLICK, kBind - kBindLoad);
         
         break;

         default:
            redraw = FALSE;
      }

      if (redraw) {
         FillBlack (FILL_SUBPANEL);
         RedrawDisplay ();
      }

      return TRUE;
   }


   //////////////////////////////////////////////////////

   void SetVidRes ()
   {
      //has not changed
      if (mSelectedRes == -1)
         return;

      sScrnMode mode = *GetGameScreenMode(); 

      char *mode_desc = (char *)(const char *)mListButtonStrs[mSelectedRes];
      sscanf (mode_desc, "%dx%dx%d", &mode.w, &mode.h, &mode.bitdepth);

      SetGameScreenMode (&mode);

      SetVidResStr(); 
   }

   // Set the resolution string 
   void SetVidResStr()
   {
      int residx = kScreenRes; 
      cStr& resstr = mSubPanelStrs[residx];
      DrawElement& elem = mSubPanelElems[residx]; 

      char buf[64]; 
      const sScrnMode& mode = *GetGameScreenMode();
      sprintf(buf,"%dx%dx%d",mode.w,mode.h,mode.bitdepth); 

      SetUIString(resstr,elem,"videob_0",buf); 
   }


   //////////////////////////////////////////////////////

   void FillVidResStrs ()
   {
      //int mTopList;
      const sScrnMode *mode = GetGameScreenMode ();
      lgd3ds_device_info *info;

      int idx = mNumVidDevices - 1; 
      if (config_is_defined("d3d_driver_index"))
         config_get_int("d3d_driver_index", &idx);
      info = lgd3d_get_device_info (idx);

      int bpp;
      short *cur_mode;
      char buf[32], mode_str[32];
      
      if (mNumVidDevices > 0 && (mode->flags & kScrnMode3dDriver)) {
         cur_mode = &info->supported_modes[mTopList];
         bpp = 16;
      }
      else {
         cur_mode = &soft_modes[mTopList];
         bpp = 8;
      }

      //get current mode string so we know what to select
      sprintf (mode_str, "%dx%dx%d", mode->w, mode->h, mode->bitdepth);
      //dunny button will be default selection if we dont have a matching res for some reason
      LGadRadioButtonSelect (&mListButtons, mListPrevPick = NUM_LIST - 1); 

      mNumListTotal = 0;
      for (long i = 0, inc = 0; i < NUM_LIST; i += inc) {
         inc = 0;
         if (*cur_mode != -1 && i < NUM_LIST - 1) 
         {
               grs_mode_info& info = grd_mode_info[*cur_mode]; 

            if (info.bitDepth == bpp                 
                && info.w >= MIN_RES_X
                && info.h >= MIN_RES_Y
                && info.h <= MAX_RES_Y) 

            {

               sprintf (buf, "%dx%dx%d", grd_mode_info[*cur_mode].w,
                  grd_mode_info[*cur_mode].h, grd_mode_info[*cur_mode].bitDepth);

               mListButtonStrs[i] = buf;
            
               if (!strcmp (buf, mode_str))
                  LGadRadioButtonSelect (&mListButtons, mListPrevPick = i); 

               inc = 1;
               mNumListTotal++;
            }
            cur_mode++;
         }
         else {
            mListButtonStrs[i] = "";
            inc = 1;
         }

         DrawElement& elem = mListButtonElems[i]; 
         InitDrawElem(&elem); 
         elem.draw_data = (void *)(const char *)mListButtonStrs[i];
         elem.draw_type = DRAWTYPE_TEXT;
      }

   }


   //////////////////////////////////////////////////////

   void FillVidDevStrs ()
   {
      lgd3ds_device_info *info;

      for (long i = 0; i < NUM_LIST; i++) {
         if (i < mNumVidDevices) {
            info = lgd3d_get_device_info (i);
            mListButtonStrs[i] = info->p_ddraw_desc;
         }
         else
            mListButtonStrs[i] = "";

         DrawElement& elem = mListButtonElems[i]; 
         InitDrawElem(&elem); 
         elem.draw_data = (void *)(const char *)mListButtonStrs[i];
         elem.draw_type = DRAWTYPE_TEXT;
      }

      int idx = mNumVidDevices - 1; 
      if (config_is_defined("d3d_driver_index"))
         config_get_int("d3d_driver_index", &idx);
      LGadRadioButtonSelect (&mListButtons,  mListPrevPick = idx);
   }

   //////////////////////////////////////////////////////

   static BOOL BindFilter (char *control, char *cmd, void *data)
   {
      //we'll reserve ESC for breaking from a bind query
      if (!strcmp (control, "esc"))
         return FALSE;

      //don't bind joystick movement
      if (!strcmp (control, "joy_move"))
         return FALSE;

      //scroll lock doesnt have up/down events like a normal key
      if (!strcmp (control, "scroll_lock"))
         return FALSE;

      // don't bind the F keys since the game needs them for psi powers
      int i;
      for (i=1; i <= 12; i++)
      {
         char fkey[32];
         sprintf(fkey,"F%d",i);
         if (!stricmp(control, fkey))
            return(FALSE);
      }

      // otherwise, it's legal
      return TRUE;
   }

   //////////////////////////////////////////////////////

   static void PostBindFunc (BOOL bound)
   {  
      for (long i = 0; i < gpOptions->mNumBindable; i++)
         gpOptions->mBindButtsFilled[i] = FALSE;
      gpOptions->FillBindStrs ();
      gpOptions->FillBlack (FILL_SUBPANEL);
      gpOptions->RedrawDisplay ();

      g_pInputBinder->SetContext (gpOptions->m_old_context,TRUE);
   }

   //////////////////////////////////////////////////////

   static void QuickConfigPostBindFunc (BOOL bound)
   {  
      int bind_num = gpOptions->mTopBind + LGadRadioButtonSelection (&gpOptions->mBindButtons) + 1;
      if (bind_num < QUICKCONFIG_BINDNUM && bound) {
         if (bind_num > NUM_BIND_SLOTS - 1) {
            gpOptions->mTopBind++;
         }
         LGadRadioButtonSelect (&gpOptions->mBindButtons, bind_num - gpOptions->mTopBind);
         char buf[16];
         sprintf (buf, "bindname_%d", bind_num);
         gpOptions->mBindButtonStrs[bind_num] = FetchUIString (gpOptions->panel_name, buf, gpOptions->mResPath);
         gpOptions->mBindButtonStrs[bind_num] += " = ?";
      
         g_pInputBinder->TrapBind ((char *)(const char *)gpOptions->mBindCmd[bind_num],
            gpOptions->BindFilter, gpOptions->QuickConfigPostBindFunc, NULL);
      }

      else
         g_pInputBinder->SetContext (gpOptions->m_old_context,TRUE);

      for (long i = 0; i < gpOptions->mNumBindable; i++)
         gpOptions->mBindButtsFilled[i] = ((i != bind_num || !bound) || (bind_num == QUICKCONFIG_BINDNUM)) ? FALSE : TRUE;

      gpOptions->FillBindStrs ();
      gpOptions->FillBlack (FILL_SUBPANEL);
      gpOptions->RedrawDisplay ();

   }

   //////////////////////////////////////////////////////

   void InitButtonList (LGadButtonListDesc *desc, Rect *rect_array, DrawElement *draw_elem,
                        ButtListCB cb, cStr *strs, const char *prefix, int num, int flags)
   {
      // set up drawlelems
      for (int i = 0; i < num; i++) {
         DrawElement *elem = &draw_elem[i];
         InitDrawElem(elem); 

         char buf[16];
         sprintf(buf, "%s%d", prefix, i); 
         strs[i] = FetchUIString (panel_name, buf, mResPath); 
         elem->draw_type = DRAWTYPE_TEXT; 
         elem->draw_data = (void*)(const char*)strs[i]; 
         
      }

      LGadButtonListDesc tmp_desc = { 
         num,
         rect_array, 
         draw_elem,
         cb,
         0,
         flags,
      };

      memcpy (desc, &tmp_desc, sizeof (LGadButtonListDesc));

   }

   //////////////////////////////////////////////////////

   void DrawButtonList (LGadButtonListDesc *blist)
   {
      Rect rect = {{0, 0}, {640, 480}};
      uiHideMouse (&rect);

      int num = blist->num_buttons;
      for (int i = 0; i < num; i++) {
         ElementDraw (&blist->button_elems[i], dsNORMAL, blist->button_rects[i].ul.x, blist->button_rects[i].ul.y,
            blist->button_rects[i].lr.x - blist->button_rects[i].ul.x, blist->button_rects[i].lr.y - blist->button_rects[i].ul.y);
      }

      uiShowMouse (&rect);
   }
      
   //////////////////////////////////////////////////////

   void DrawSlider (int num, int rect_num, BOOL draw_slide = TRUE)
   {
      Rect rect = {{0, 0}, {640, 480}};
      uiHideMouse (&rect);

      Rect *slide_rect = (Rect *)&mSubPanelRects[rect_num];
      ElementDraw (&mSliderBaseBmp, dsNORMAL, slide_rect->ul.x, slide_rect->ul.y,
         slide_rect->lr.x - slide_rect->ul.x, slide_rect->lr.y - slide_rect->ul.y);

      uiShowMouse (&rect);
      if (draw_slide)
         LGadDrawBox ((LGadBox*)&mSliders[num], NULL);
   }
      
   //////////////////////////////////////////////////////

   void LoadButtBmp (IRes *butt_res[4], DrawElement *draw_elem, char *prefix)
   {
      char *sffx [] = {"norm", "down", "hlit", "hlit"}, str[128];
      AutoAppIPtr (ResMan);
      for (int i = 0; i < 4; i++) {
         butt_res[i] = pResMan->Bind (strcat (strcpy (str, prefix), sffx[i]), RESTYPE_IMAGE, NULL, mResPath);
         if (!butt_res[i])
            AssertMsg (0, "Error loading button bitmap");
      }

      InitDrawElem(draw_elem); 
      draw_elem->draw_data = (void *)butt_res;
      draw_elem->draw_type = DRAWTYPE_RESOFFSET;
   }

   //////////////////////////////////////////////////////

   void SetUIString (cStr &str, DrawElement &draw_elem, char *name, char *suffix)
   {
      str = FetchUIString (panel_name, name, mResPath);
      if (suffix)
         str += suffix;
      draw_elem.draw_data = (void*)(const char*)str;
   }

   //////////////////////////////////////////////////////

   void AppendRects (char *rect_file, cRectArray &old_rects)
   {
      long old_sz, tmp_sz;
      cRectArray tmp_rects; 
      FetchUIRects(rect_file, tmp_rects, mResPath);
      old_rects.SetSize ((old_sz = old_rects.Size ()) + (tmp_sz = tmp_rects.Size ()));

      for (long i = 0, j = old_sz; i < tmp_sz; i++, j++)
         old_rects[j] = tmp_rects[i];      
   }

   //////////////////////////////////////////////////////

   void CreateSlider (int slider_num, int rect_num, int *val_ptr, int num_notches, int inc)
   {
      LGadSlider *slider = &mSliders[slider_num];
      Rect *rect = (Rect *)&mSubPanelRects[rect_num];
      memset (slider, 0, sizeof (LGadSlider));
      slider->gadg.draw = mButtDrawElem[SLIDER_BMPRES]; 

      //this is absolutely a hack.
      //make the slider's range of motion smaller since the slider's base art
      //presents the slide as having a more confined range. 12 pixels will do the trick.
      short x, w;
      x = rect->ul.x + 12;
      w = rect->lr.x - rect->ul.x - 24;

      LGadCreateSliderArgs (slider, LGadCurrentRoot (), x, rect->ul.y + ((rect->lr.y - rect->ul.y)>>1), 0, 0, NULL,
         val_ptr, num_notches, inc, w, LGSLIDER_HORIZONTAL, 0);

      mSlidersOn[slider_num] = TRUE;

      LGadDrawBox ((LGadBox*)slider, NULL);
   }

   //////////////////////////////////////////////////////

   void DestroySlider (int slider_num)
   {
      if (mSlidersOn[slider_num]) {
         LGadDestroySlider (&mSliders[slider_num], FALSE);
         mSlidersOn[slider_num] = FALSE;
      }
   }

   //////////////////////////////////////////////////////

   void DestroyAllSliders ()
   {
      for (int i = 0; i < (int)kNumSubPanelSlides; i++)
         DestroySlider (i);
   }

   //////////////////////////////////////////////////////

   void DrawString (char *strname, int rect_num)
   {
      cStr str = FetchUIString (panel_name, strname, mResPath);//copy the string 
      char *s = (char*)(const char*)str;//get a mutable pointer

      Rect &r = mSubPanelRects[rect_num]; 

      DrawElement draw;
      InitDrawElem(&draw); 
      draw.draw_type = DRAWTYPE_TEXT;
      draw.draw_data = s;
      ElementSetStyle(&mTextStyle); 
      ElementDraw (&draw, dsNORMAL, r.ul.x, r.ul.y, r.lr.x - r.ul.x, r.lr.y - r.ul.y);
   }

   //////////////////////////////////////////////////////

   void RedrawDisplay ()
   {
      // draw buttonlist
      region_expose (LGadBoxRegion (&mTabButtons), LGadBoxRect (&mTabButtons));
      DrawButtonList (&mSubPanelDesc[mCurSub]);

      //any sub-specific stuff
      switch (mCurSub) {
         case kSubControls:
            DrawSlider ((int)kMouseSensSlider, (int)kMouseSensSliderRect);
            DrawString ("mouse_sens", kMouseSensTextRect);
         break;

         case kSubBasicVideo:
            DrawSlider ((int)kGammaSlider, (int)kGammaSliderRect);
            DrawString ("gamma", kGammaSliderText);
         break;

         case kSubAudio:
         {
            for (int i = 0; i < kNumVolumeSliders; i++)
            {
               DrawSlider (i, gVolSliderRects[i]);
               char name[16]; 
               sprintf(name,"volume_%d",i); 
               DrawString (name,gVolSliderRects[i] + 1); // plus one to get from rect to text rect
            }
         }
         break;

         case kSubGame:
            region_expose (LGadBoxRegion (&mSubPanelButtons[kSubGame]), LGadBoxRect (&mSubPanelButtons[kSubGame]));
            DrawString ("diff", kDiffTextRect);
         break;

         case kSubBind:
            DrawButtonList (&mBindScrollerDesc);
            region_expose (LGadBoxRegion (&mBindButtons), LGadBoxRect (&mBindButtons)); 
         break;       

         case kSubList:
            region_expose (LGadBoxRegion (&mListButtons), LGadBoxRect (&mListButtons)); 
//            DrawButtonList (&mBindScrollerDesc);
         break;       
        }
   }

   //////////////////////////////////////////////////////

   void FillBlack (int rect_num)
   {
      Rect *rect;
      if (rect_num == FILL_SUBPANEL)
         rect = (Rect *)&mRects[(int)kSubFillRect];
      else
         rect = (Rect *)&mSubPanelRects[rect_num];

      GUIErase(rect); 
   }

   //////////////////////////////////////////////////////

   void SwapSubPanel (int new_sub)
   {
      if (mCurSub != -1) {
         LGadDestroyButtonList (&mSubPanelButtons[mCurSub]); 
         TermSpecial ();
      }

      if (new_sub!=kSubAudio)
         metaSndEnterPanel(kMetaSndPanelOptions);
      else
         metaSndExitPanel(TRUE);

      LGadCreateButtonListDesc (&mSubPanelButtons[new_sub], LGadCurrentRoot (), &mSubPanelDesc[new_sub]);
      SetBoxData(VB(&mSubPanelButtons[new_sub]));

      switch (new_sub)
      {
         case kSubControls:      
         {
            DrawSlider ((int)kMouseSensSlider, (int)kMouseSensSliderRect, FALSE);
            CreateSlider ((int)kMouseSensSlider, (int)kMouseSensSliderRect, &m_sens, 20, 1);
         }
         break; 

         case kSubBasicVideo:
         {
            DrawSlider ((int)kGammaSlider, (int)kGammaSliderRect, FALSE);
            CreateSlider ((int)kGammaSlider, (int)kGammaSliderRect, &m_gamma, 20, 1);
         }
         break; 

         case kSubAudio:
         {
            for (int i = 0; i < kNumVolumeSliders; i++)
            {
               DrawSlider(i,gVolSliderRects[i],FALSE); 
               CreateSlider(i,gVolSliderRects[i],&m_volume[i],kVolumeSliderNotches - 1, 1); 
            }

            if (! CanChangeSoundDeviceNow ()) {
               mSubPanelDesc[kSubAudio].button_elems[kAudioChannels - kFirstAudioButt].fcolor = guiStyleGetColor(&mGreyStyle,StyleColorText); 
               mSubPanelDesc[kSubAudio].button_elems[kA3DToggle - kFirstAudioButt].fcolor = guiStyleGetColor(&mGreyStyle,StyleColorText); 
            }

            if (!CanChangeEAX())
               mSubPanelDesc[kSubAudio].button_elems[kAudioEAXToggle - kSpeakerTest].fcolor = guiStyleGetColor(&mGreyStyle,StyleColorText);
         }
         break; 

         case kSubGame:
         {
            g_diff_active = CanChangeDifficultyNow(); 
            if (InSim())
            {
               AutoAppIPtr(QuestData); 
               if (pQuestData->Exists(DIFF_QVAR))
                  g_diff = pQuestData->Get(DIFF_QVAR); 
            }

            if (!g_diff_active)
               LGadBoxSetStyle (&mSubPanelButtons[kSubGame], &mGreyStyle); 

            //set the current difficulty setting
            LGadRadioButtonSelect (&mSubPanelButtons[kSubGame], DIFF2BUTT(g_diff));
         }
         break; 

         case kSubBind:
         {
            LGadCreateButtonListDesc (&mBindButtons, LGadCurrentRoot (), &mBindButtonDesc);
            SetBoxData(VB(&mBindButtons));
            LGadRadioButtonSelect (&mBindButtons,0); 
            LGadBoxSetStyle (&mBindButtons, &mBindStyle); 

            LGadCreateButtonListDesc (&mBindScrollers, LGadCurrentRoot (), &mBindScrollerDesc);
            SetBoxData(VB(&mBindScrollers));
         }
         break; 

         case kSubList:
         {
            LGadCreateButtonListDesc (&mListButtons, LGadCurrentRoot (), &mListButtonDesc);
            SetBoxData(VB(&mListButtons));
            LGadBoxSetStyle (&mListButtons, &mBindStyle); 

            //         LGadCreateButtonListDesc (&mBindScrollers, LGadCurrentRoot (), &mBindScrollerDesc);
            //         SetBoxData(VB(&mBindScrollers));

            if (mListSub == kSubAdvancedVideo)
               FillVidDevStrs ();
            else if (mListSub == kSubBasicVideo)
               FillVidResStrs ();
            mTopList = 0;
         }
         break; 
      }

      FillBlack (FILL_SUBPANEL);

      mCurSub = new_sub;
      RedrawDisplay ();
   }

   //////////////////////////////////////////////////////

   static void PlayRightTest (int, void *)
   {
      sfx_parm parm;
      memset (&parm, 0, sizeof (sfx_parm));
      parm.pan = SFX_StereoReversed() ? -10000 : 10000;
      cStr snd_name = FetchUIString (gpOptions->panel_name, "rightsound", gpOptions->mResPath);

      if (SFX_GetSoundDevice () == SFXDEVICE_A3D) {
         //@TODO: make the a3d speaker test sound crisper, clearer, more refreshed
         mxs_vector pos = {0.0, SFX_StereoReversed() ? 10.0 : -10.0, 0.0};
         SFX_Play_Vec (SFX_3D, &parm, (char *)(const char *)snd_name, &pos);
      }
      else
         SFX_Play_Raw (SFX_STATIC, &parm, (char *)(const char *)snd_name);
   }

   //////////////////////////////////////////////////////
   
   static bool OnTabButton (ushort action, int button, void *data, LGadBox *)
   {
      cOptions *panel = (cOptions*)data; 

      panel->OnTabButtonList (action, button); 
      return FALSE; 
   }

   //////////////////////////////////////////////////////

   void OnTabButtonList (ushort action, int button)
   {      
      if (!(action & BUTTONGADG_LCLICK))
         return;

      switch (button)
      {
         //controls
         case kTab0:
         {
            if (mCurSub != kSubControls) {
               mSwap = kSubControls;
               DestroyAllSliders ();
            }
         }
         break; 

         //video
         case kTab1:
         {
            if (mCurSub != kSubBasicVideo && mCurSub != kSubAdvancedVideo) {
               DestroyAllSliders ();
               mSwap = kSubBasicVideo;
            }
         }
         break; 

         //audio
         case kTab2:
         {
            if (mCurSub != kSubAudio) {
               DestroyAllSliders ();
               mSwap = kSubAudio;
            }
         }
         break; 

         //game
         case kTab3:
         {
            if (mCurSub != kSubGame) {
               DestroyAllSliders ();
               mSwap = kSubGame;
            }
         }
         break; 
      }
   }


   //////////////////////////////////////////////////////
   
   static bool OnSubPanelButton (ushort action, int button, void *data, LGadBox *)
   {
      cOptions *panel = (cOptions*)data; 

      panel->OnSubPanelButtonList (action, button); 
      return FALSE; 
   }

   //////////////////////////////////////////////////////
      
   void OnSubPanelButtonList (ushort action, int button)
   {      
      if (!(action & BUTTONGADG_LCLICK))
         return;

      int idx = button + gFirstSubRect[mCurSub];


      switch (idx)
      {

         case kScreenRes:
            mSwap = kSubList;
            mListSub = kSubBasicVideo;
            mSelectedRes = -1;
            mTopList = 0;
         break;


#ifdef ADVANDED_VID_PANEL
         case kAdvancedVidOptions:
            DestroyAllSliders ();
            mSwap = kSubAdvancedVideo;
         break;
#endif 

         case kHardwareDriver:
            mSwap = kSubList;
            mListSub = kSubAdvancedVideo;
            mTopList = 0;
         break;

         case kZBufferToggle:
            {
            const sScrnMode *old_mode = GetGameScreenMode ();
            if (mNumVidDevices > 0 && old_mode->flags & kScrnMode3dDriver) {
                  if (!g_zbuffer_toggle) {
                     SetUIString (mSubPanelStrs[(int)kZBufferToggle], mSubPanelElems[(int)kZBufferToggle], "videoa_1",
                        (char *)(const char *)mMiscStrs[kMiscStrOn]);
                     g_zbuffer_toggle = TRUE;
                  }
                  else {
                     SetUIString (mSubPanelStrs[(int)kZBufferToggle], mSubPanelElems[(int)kZBufferToggle], "videoa_1",
                        (char *)(const char *)mMiscStrs[kMiscStrOff]);
                     g_zbuffer_toggle = FALSE;
                  }
                  FillBlack ((int)kZBufferToggle);
                  RedrawDisplay ();
               }
            }
         break;

         case kBasicVidOptions:
            DestroyAllSliders ();
            mSwap = kSubBasicVideo;
         break;


         case kSpeakerTest:
         {
            sfx_parm parm;
            memset (&parm, 0, sizeof (sfx_parm));
            parm.pan = SFX_StereoReversed() ? 10000 : -10000;
            parm.end_callback = PlayRightTest;
            cStr snd_name = FetchUIString (panel_name, "leftsound", mResPath);

            if (SFX_GetSoundDevice () == SFXDEVICE_A3D) {
               mxs_vector pos = {0.0, SFX_StereoReversed() ? -10.0 : 10.0, 0.0};
               //@TODO: make the a3d speaker test sound crisper, clearer, more refreshed
               SFX_Play_Vec (SFX_3D, &parm, (char *)(const char *)snd_name, &pos);
            }
            else
               SFX_Play_Raw (SFX_STATIC, &parm, (char *)(const char *)snd_name);
         }
         break;

         case kStereoToggle:
            SFX_SetReverseStereo (!SFX_StereoReversed ());
            SetUIString (mSubPanelStrs[(int)kStereoToggle], mSubPanelElems[(int)kStereoToggle], "audio_1",
               (char *)(const char *)mMiscStrs[(SFX_StereoReversed ()) ? kMiscStrOn : kMiscStrOff]);
            FillBlack (kStereoToggle);
            RedrawDisplay ();
         break;

         case kA3DToggle:
            if (CanChangeSoundDeviceNow()) {
               if (SFX_GetSoundDevice () != SFXDEVICE_A3D)
               {
                  // Try to use 3D sound hardware
                  if (! SFX_SetSoundDevice (SFXDEVICE_A3D, TRUE))
                  {
                     // If no 3D sound hardware, turn back on software.
                     SFX_SetSoundDevice (SFXDEVICE_Software, TRUE);
                  }
               }
               else
                  SFX_SetSoundDevice (SFXDEVICE_Software, TRUE);
            
               SetUIString (mSubPanelStrs[(int)kA3DToggle], mSubPanelElems[(int)kA3DToggle], "audio_2",
                  (char *)(const char *)mMiscStrs[(SFX_GetSoundDevice () == SFXDEVICE_A3D) ? kMiscStrOn : kMiscStrOff]);
               FillBlack (kA3DToggle);
               // RedrawDisplay ();

               // EAX settings may have changed.
               BOOL newEAXState = SFX_Is_EAX_Enabled();
               SetUIString (mSubPanelStrs[(int)kAudioEAXToggle], mSubPanelElems[(int)kAudioEAXToggle], "audio_4",
                  (char *)(const char *)mMiscStrs[newEAXState ? kMiscStrOn : kMiscStrOff]);

               // Set EAX button color as appropriate.
               if ( CanChangeEAX() )
                  // Hack: This takes the "green" color from the "speaker test" button..... there must be a better way...
                  mSubPanelDesc[kSubAudio].button_elems[kAudioEAXToggle - kSpeakerTest].fcolor = mSubPanelDesc[kSubAudio].button_elems[0].fcolor;
               else
                  mSubPanelDesc[kSubAudio].button_elems[kAudioEAXToggle - kSpeakerTest].fcolor = guiStyleGetColor(&mGreyStyle,StyleColorText); 
               FillBlack (kAudioEAXToggle);
               
               RedrawDisplay();
            }
         break;

         case kAudioChannels:
            if (CanChangeSoundDeviceNow())
            {  
               // List of valid selections 
               static int selections[] = { 4, 8, 12, 16, 0 }; 
               int chan = sfx_use_channels; 
               for (int* sel = selections; *sel != 0 && *sel <= chan; sel++)
                  ;
               if (*sel == 0)
                  chan = selections[0]; 
               else
                  chan = *sel; 

               SFXClose ();
               sfx_use_channels = chan; 
               config_set_int("sfx_channels", chan);
               SFXInit ();
               
               char buf[8];
               sprintf (buf, "%d", chan);
               mSubPanelStrs[(int)kAudioChannels] += buf;
               SetUIString (mSubPanelStrs[(int)kAudioChannels], mSubPanelElems[(int)kAudioChannels], "audio_3", buf);
            }
            break;

         case kAudioEAXToggle:
            if (CanChangeEAX())
            {
               // Toggle EAX state.
               BOOL newEAXState = !SFX_Is_EAX_Enabled();
               if (newEAXState)
                  SFX_Enable_EAX();
               else
                  SFX_Disable_EAX();

               // Double-check to make sure state actually toggled.
               newEAXState = SFX_Is_EAX_Enabled();
               config_set_int ("sfx_eax", (int) newEAXState);
               SetUIString (mSubPanelStrs[(int)kAudioEAXToggle], mSubPanelElems[(int)kAudioEAXToggle], "audio_4",
                  (char *)(const char *)mMiscStrs[newEAXState ? kMiscStrOn : kMiscStrOff]);
            }
            break;

         case kCustomize:
            DestroyAllSliders ();
            mSwap = kSubBind;
         break;


         // I may weep openly
         case kDiff0:
         case kDiff1:
         case kDiff2:
         case kDiff3:
         {
            if (g_diff_active) 
            {
               // Set the difficulty
               g_diff = BUTT2DIFF(button);
               if (InSim())
               {
                  AutoAppIPtr(QuestData);
                  if (pQuestData->Exists(DIFF_QVAR))
                     pQuestData->Set(DIFF_QVAR,g_diff); 
               }
            }
            //not active, don't let them change buttons
            else 
               LGadRadioButtonSelect (&mSubPanelButtons[kSubGame], DIFF2BUTT(g_diff));
         }
         break;


         case kBindLoad:
            SwitchToShockLoadBndMode (TRUE); 
         break;


         case kBindSave:
            SwitchToShockSaveBndMode (TRUE); 
         break;
         
         case kBindBack:
            DestroyAllSliders ();
            mSwap = kSubControls;
         break;


         case kQuickConfigure:
            LGadRadioButtonSelect (&mBindButtons, mTopBind = 0); 
            mBindButtonStrs[0] = FetchUIString (panel_name, "bindname_0", mResPath);
            mBindButtonStrs[0] += " = ?";

            //change to game context
            g_pInputBinder->GetContext (&m_old_context);
            g_pInputBinder->SetContext (HK_GAME_MODE,TRUE);
            g_pInputBinder->TrapBind ((char *)(const char *)mBindCmd[0], BindFilter, QuickConfigPostBindFunc, NULL);

            FillBindStrs ();
            FillBlack (FILL_SUBPANEL);
            RedrawDisplay ();
         break;

         
         case kBind:
         {
            int bind_num = mTopBind + LGadRadioButtonSelection (&mBindButtons);
            char buf[16];
            sprintf (buf, "bindname_%d", bind_num);
            mBindButtonStrs[bind_num] = FetchUIString (panel_name, buf, mResPath);
            mBindButtonStrs[bind_num] += " = ?";

            //change to game context
            g_pInputBinder->GetContext (&m_old_context);
            g_pInputBinder->SetContext (HK_GAME_MODE,TRUE);
            g_pInputBinder->TrapBind ((char *)(const char *)mBindCmd[bind_num], BindFilter, PostBindFunc, NULL);

            FillBindStrs ();
            FillBlack (FILL_SUBPANEL);
            RedrawDisplay ();
         }
         break;


         case kBindClear:
         {
            //change to game context
            g_pInputBinder->GetContext (&m_old_context);
            g_pInputBinder->SetContext (HK_GAME_MODE,TRUE);

            char str[128], control_buf[64];
            int bind_num = mTopBind + LGadRadioButtonSelection (&mBindButtons);

            g_pInputBinder->GetControlFromCmdStart ((char *)(const char *)mBindCmd[bind_num], control_buf);
            while (*control_buf != '\0') {
               strcpy (str, "unbind ");
               strcat (str, control_buf);
               g_pInputBinder->ProcessCmd (str);
               g_pInputBinder->GetControlFromCmdStart ((char *)(const char *)mBindCmd[bind_num], control_buf);
            }

            g_pInputBinder->SetContext (m_old_context,TRUE);

            for (int i = 0; i < mNumBindable; i++)
               mBindButtsFilled[i] = FALSE;
            FillBindStrs ();
            FillBlack (FILL_SUBPANEL);
            RedrawDisplay ();
         }
         break;

         case kLookspring:
         {
            char *inverted = g_pInputBinder->ProcessCmd ("echo $lookspring"), buf[16];
            sprintf (buf, "control_%d", button);
            if (atof (inverted) == 0.0) {
               g_pInputBinder->ProcessCmd ("lookspring 1");
               SetUIString (mSubPanelStrs[(int)kLookspring], mSubPanelElems[(int)kLookspring], buf, (char *)(const char *)mMiscStrs[kMiscStrOn]);

               //when lookspring is turned on, turn off freelook if it is on
               if (atof (g_pInputBinder->ProcessCmd ("echo $freelook")) != 0.0)
                  OnSubPanelButtonList (action, kFreelook - kCustomize);
            }
            else {
               g_pInputBinder->ProcessCmd ("lookspring 0");
               SetUIString (mSubPanelStrs[(int)kLookspring], mSubPanelElems[(int)kLookspring], buf, (char *)(const char *)mMiscStrs[kMiscStrOff]);
            }
            FillBlack ((int)kLookspring);
            RedrawDisplay ();
         }
         break;

         case kJoyRotate:
         {
            if (g_joystickActive)
            {
               char *rotate = g_pInputBinder->ProcessCmd ("echo $joy_rotate"), buf[16];
               sprintf (buf, "control_%d", button);
               if (atof (rotate) == 0.0) {
                  g_pInputBinder->ProcessCmd ("joy_rotate 1");
                  SetUIString (mSubPanelStrs[(int)kJoyRotate], mSubPanelElems[(int)kJoyRotate], buf, (char *)(const char *)mMiscStrs[kMiscStrLeftRight]);
               }
               else {
                  g_pInputBinder->ProcessCmd ("joy_rotate 0");
                  SetUIString (mSubPanelStrs[(int)kJoyRotate], mSubPanelElems[(int)kJoyRotate], buf, (char *)(const char *)mMiscStrs[kMiscStrForwardBackward]);
               }
               FillBlack ((int)kJoyRotate);
               RedrawDisplay ();
            }
         }
         break;
      
         case kMouseInvert:
         {
            char *inverted = g_pInputBinder->ProcessCmd ("echo $mouse_invert"), buf[16];
            sprintf (buf, "control_%d", button);
            if (atof (inverted) == 0.0) {
               g_pInputBinder->ProcessCmd ("mouse_invert 1");
               SetUIString (mSubPanelStrs[(int)kMouseInvert], mSubPanelElems[(int)kMouseInvert], buf, (char *)(const char *)mMiscStrs[kMiscStrOn]);
            }
            else {
               g_pInputBinder->ProcessCmd ("mouse_invert 0");
               SetUIString (mSubPanelStrs[(int)kMouseInvert], mSubPanelElems[(int)kMouseInvert], buf, (char *)(const char *)mMiscStrs[kMiscStrOff]);
            }
            FillBlack ((int)kMouseInvert);
            RedrawDisplay ();
         }
         break;

      
         case kFreelook:
         {
            char *inverted = g_pInputBinder->ProcessCmd ("echo $freelook"), buf[16];
            sprintf (buf, "control_%d", button);
            if (atof (inverted) == 0.0) {
               g_pInputBinder->ProcessCmd ("freelook 1");
               SetUIString (mSubPanelStrs[(int)kFreelook], mSubPanelElems[(int)kFreelook], buf, (char *)(const char *)mMiscStrs[kMiscStrOn]);

               //when freelook is turned on, turn off lookspring if it is on
               if (atof (g_pInputBinder->ProcessCmd ("echo $lookspring")) != 0.0)
                  OnSubPanelButtonList (action, kLookspring - kCustomize);
            }
            else {
               g_pInputBinder->ProcessCmd ("freelook 0");
               SetUIString (mSubPanelStrs[(int)kFreelook], mSubPanelElems[(int)kFreelook], buf, (char *)(const char *)mMiscStrs[kMiscStrOff]);
            }
            FillBlack ((int)kFreelook);
            RedrawDisplay ();
         }
         break;

      
         case kListBack:
            mSwap = mListSub;
            DestroyAllSliders ();

            if (mListSub == kSubAdvancedVideo) {
               config_set_int ("d3d_driver_index", m3dDriver = LGadRadioButtonSelection (&mListButtons));
               // Go back to basic video instead of advanced
               mSwap = kSubBasicVideo; 
            }

            //resolution choice
            else if (mListSub == kSubBasicVideo) {
               SetVidRes ();
            }
         break;

      }
   }

   //////////////////////////////////////////////////////
   
   static bool OnScrollButton (ushort action, int button, void *data, LGadBox *)
   {
      cOptions *panel = (cOptions*)data; 

      panel->OnScrollList (action, button); 
      return FALSE; 
   }

   //////////////////////////////////////////////////////

   void OnScrollList (ushort action, int button)
   {      
      if (!(action & BUTTONGADG_LCLICK))
         return;

      BOOL redraw = TRUE;

      switch (button)
      {
         //scroll up
         case 0:
            switch (mCurSub) {
               case kSubBind:
                  if (mTopBind > 0) {
                     mTopBind--;
                     FillBindStrs ();
                  }
                  else
                     redraw = FALSE;
               break;

               //scrolling through resolutions
               case kSubList:
                  if (mTopList > 0) {
                     mTopList--;
                     FillVidResStrs ();
                  }
                  else
                     redraw = FALSE;
               break;
            }

         break;

         //scroll down
         case 1:
            switch (mCurSub) {
               case kSubBind:
                  if (mTopBind <= mNumBindable - NUM_BIND_SLOTS) {
                     mTopBind++;
                     FillBindStrs ();

                     if (mTopBind == mNumBindable - (NUM_BIND_SLOTS - 1) && LGadRadioButtonSelection (&mBindButtons) == (NUM_BIND_SLOTS - 1))
                        LGadRadioButtonSelect (&mBindButtons, 8);
                  }
                  else
                     redraw = FALSE;
               break;

               //resolutions
               case kSubList:
                  if (mNumListTotal >= NUM_LIST - 1) {
                     mTopList++;
                     FillVidResStrs ();
                  }
                  else
                     redraw = FALSE;
               break;
            }

         break;
      }

      if (redraw) {
         FillBlack (FILL_SUBPANEL);
         RedrawDisplay ();
      }
   
   }

   //////////////////////////////////////////////////////
   
   static bool OnBindButton (ushort action, int button, void *data, LGadBox *)
   {
      cOptions *panel = (cOptions*)data; 

      panel->OnBind (action, button); 
      return FALSE; 
   }

   //////////////////////////////////////////////////////

   void OnBind (ushort action, int button)
   {      
      if (!(action & BUTTONGADG_LCLICK))
         return;

      static int prev_pick;
      if (!strcmp ((const char *)mBindButtonElems[button].draw_data, "")) {
         LGadRadioButtonSelect (&mBindButtons, prev_pick);
      }
      else
         prev_pick = button;
   }

   //////////////////////////////////////////////////////
   
   static bool OnListButton (ushort action, int button, void *data, LGadBox *)
   {
      cOptions *panel = (cOptions*)data; 

      panel->OnList (action, button); 
      return FALSE; 
   }

   //////////////////////////////////////////////////////

   void OnList (ushort action, int button)
   {      
      if (!(action & BUTTONGADG_LCLICK))
         return;

      BOOL valid_butt = TRUE;

      //display device choice
      if (mListSub == kSubAdvancedVideo) {
         //the button clicked was not filled
         if (button >= mNumVidDevices) {
            LGadRadioButtonSelect (&mListButtons, mListPrevPick);
            valid_butt = FALSE;
         }

         //there was something there
         else {
            config_set_int ("d3d_driver_index", button);
            m3dDriver = button;
         }
      }

      //resolution choice
      else if (mListSub == kSubBasicVideo) {
         //the button clicked was not filled
         if (button >= mNumListTotal) {
            LGadRadioButtonSelect (&mListButtons, mListPrevPick);
            valid_butt = FALSE;
         }
         else {
            mSelectedRes = mTopList + button;
            SetVidRes ();
         }
      }
      
      if (valid_butt)
         mListPrevPick = button;
   }

   //////////////////////////////////////////////////////

   void OnButtonList(ushort action, int button)
   {
      if (!(action & BUTTONGADG_LCLICK))
         return;

      switch (button)
      {
         case kDone:
         {
            if (mCurSub == kSubList)
               OnSubPanelButtonList (action, 0);

            cAutoIPtr<IPanelMode> mode = GetPanelMode(); 
            mode->Exit(); 
         }
         break;
      }
   }

}; 


int cShockOptions::gFirstSubRect[] = 
{ 
   cShockOptions::kFirstControlButt,
   cShockOptions::kFirstBasicVideoButt,
   cShockOptions::kFirstAdvancedVideoButt,
   cShockOptions::kFirstAudioButt,
   cShockOptions::kFirstGameButt, 
   cShockOptions::kFirstBindButt,
   cShockOptions::kFirstListButt,
}; 

int cShockOptions::gVolSliderRects[] = 
{
   cShockOptions::kVolumeSliderRect, 
   cShockOptions::k3dVolumeSliderRect, 
   cShockOptions::kMusicSliderRect, 
}; 


//
// Options Menu descriptor 
//

static const char* options_button_names[] = 
{
   "done"
}; 

sDarkPanelDesc cOptions::gDesc = 
{
   "options",
   cOptions::kNumButts,
   cOptions::kNumRects,
   cOptions::kNumButts,
   options_button_names, 
   NULL,
   0,
   kMetaSndPanelOptions
}; 



EXTERN void SwitchToShockOptionsMode(BOOL push) 
{
   if (gpOptions)
   {
      cAutoIPtr<IPanelMode> panel = gpOptions->GetPanelMode(); 
      panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
   }
}


const sLoopInstantiator* DescribeShockOptionsMode(void)
{
   if (gpOptions)
   {
      cAutoIPtr<IPanelMode> panel = gpOptions->GetPanelMode(); 
      return panel->Instantiator(); 
   }
   return NULL; 
}


//Wrapper so that others can use this easier..
//Will stuff retrieve_num many controls bound to cmd into the dest
//string array, and will return how many actually got stuff
#ifdef WE_NEED_THIS
int GetCmdBinds (const char *cmd, cStr *dest, int retrieve_num)
{
   return gpOptions->GetCmdBinds (cmd, dest, retrieve_num);
}
#endif 



//
// Init and term stuff
//

void ShockOptionsMenuInit ()
{   
   gpOptions = new cOptions;
   gpOptions->SetInitialSub ();
   ShockInitBindSaveLoad();
}

void ShockOptionsMenuTerm ()
{
   delete gpOptions;
   ShockTermBindSaveLoad();
}

