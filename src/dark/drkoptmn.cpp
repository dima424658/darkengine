/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkoptmn.cpp,v 1.75 2000/03/22 18:19:55 patmac Exp $
#include <string.h>
#include <dev2d.h>
#include <keydefs.h>

#include <drkmenu.h>
#include <appagg.h>
#include <drkpanl.h>
#include <drkmislp.h>
#include <drksavui.h>
#include <drkamap.h>
#include <drkdebrf.h>
#include <drksound.h>

#include <appsfx.h>
#include <gamescrn.h>
#include <scrnmode.h>

#include <resapi.h>
#include <imgrstyp.h>
#include <strrstyp.h>

#include <gcompose.h>
#include <questapi.h>
#include <drkdiff.h>
#include <drkwbow.h>
#include <config.h>
#include <uigame.h>

#include <panltool.h>
#include <appapi.h>
#include <mprintf.h>
#include <campaign.h>
#include <metasnd.h>

#include <gadslide.h>
#include <gen_bind.h>
#include <lgd3d.h>

#include <command.h>
#include <gamma.h>
#include <objmodel.h>
#include <metasnd.h>

#include <skyrend.h>
#include <weather.h>
#include <math.h>
#include <rect.h>

#include <schbase.h>

#include <dbmem.h>

const int kVolumeSliderNotches = 25;
// allow 16 notches above and below neutral setting
const int kAmbientVolumeSliderNotches = 33;
const int kAmbientMillibelsPerNotch = 100;

//@TODO: Hook g_gamma up to game
static void TouchGamma (float gamma)
{
   g_gamma = gamma;
   gamma_update();

#if 0
   char str[32];
   sprintf (str, "Gamma: %4.2f\n", gamma);
   mprintf (str);
#endif
}

static int SliderVolumeLookup (int logVol, int lookupTable[], int lookupCount)
{
   for (int i = 0 ; i < lookupCount ; i++)
   {
      if (logVol <= lookupTable[i])
         return i;
   }

   // Should only get here if logVol is 0 or greater.
   return (lookupCount - 1);
}

//is the difficulty setting greyed out or not?
BOOL g_diff_active = 1;

static BOOL reslist = TRUE; //hack to put resolution and driver on same subpanel easily.

//starting difficulty
extern int g_diff;

EXTERN BOOL portal_fog_on;
EXTERN BOOL g_zbuffer_toggle;
EXTERN BOOL g_joystickActive;
extern BOOL g_climb_on_touch;
extern int sfx_use_channels;

/////////////////

#define MAX_NUM_BINDABLE 80
#define NUM_BIND_SLOTS 14
//max cmds we will display per a given cmd
#define MAX_BINDS_PER_CMD 5

/////////////////

#define SLIDER_BMPRES 0
#define BINDUP_BMPRES 1
#define BINDDN_BMPRES 2

#define NUM_LIST 13
#define FILL_SUBPANEL 666

#define LRMOD(x) (!strcmp (x, "lalt") || !strcmp (x, "ralt") || \
                  !strcmp (x, "lctrl") || !strcmp (x, "rctrl") || \
                  !strcmp (x, "lshift") || !strcmp (x, "rshift"))



#define MIN_RES_X 400
#define MIN_RES_Y 300
#define MAX_RES_Y 1200

//all 8-bit for software
short soft_modes[] = {
   GRM_400x300x8,
   GRM_512x384x8,
   GRM_640x400x8, 
   GRM_640x480x8,
   GRM_800x600x8,
   -1
};


      //#define CanChangeEAX() (CanChangeDifficultyNow() && SFX_Is_EAX_Available())
//#define CanChangeEAX() (SFX_Is_EAX_Available())
#define CanChangeEAX() (SFX_Is_EAX_Available() && (SFX_GetSoundDevice () == SFXDEVICE_A3D))

// this is REALLY REALLY NOT THE RIGHT THING... HOW DID THIS END UP THIS WAY
#define CanChangeSoundDeviceNow() CanChangeDifficultyNow()

//------------------------------------------------------------
// OPTIONS MENU
//
static class cOptions *gpOptions = NULL; 


class cOptions: public cDarkPanel
{
   static sDarkPanelDesc gDesc; 

public:
   cOptions() : cDarkPanel(&gDesc)
   {
   };

   void SetInitialSub ()
   {
      mCurSub = kSubControls;
   }

   //Will stuff retrieve_num many controls bound to cmd into the dest
   //string array, and will return how many actually got stuff
   int GetCmdBinds (const char *cmd, cStr *dest, int retrieve_num)
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
         if (i == num_controls) {
            dest[cur_control] = "";
            for (i = 0; i < num_controls; i++) {

               cStr str = FetchUIString ("controls", controls[i], mResPath);
               
               if (i)
                  dest[cur_control] += "+";

               //does this key have a translated string?
               if (strcmp ((char *)(const char *)str, "") != 0)
                  dest[cur_control] += str;
               else
                  dest[cur_control] += controls[i];
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
      kCustomize,
      kJoyEnable,
      kLookspring,
      kMouseInvert,
      kFreelook,
      kJoyRotate,

      kMouseSensSliderRect,
      kMouseSensTextRect,
      kNumControlButts = 6,
      kNumControlRects = 8,

      //video buttons
	  kHardwareDriver = kMouseSensTextRect + 1,
      kScreenRes,
      kFogToggle,
	  kWeatherToggle,
	  kSkyDetail,
      kGammaSliderRect,
      kGammaSliderText,
      kNumBasicVideoButts = 5,
      kNumBasicVideoRects = 7,

      //audio buttons/rects
      kSpeakerTest = kGammaSliderText + 1,
      kStereoToggle,
      kAudio3DHWToggle,
      kAudioChannels,
      kAudioEAXToggle,

      kVolumeSliderRect,
      kVolumeSliderText,
      kAmbientVolumeSliderRect,
      kAmbientVolumeSliderText,
      kNumAudioButts = 5,
      kNumAudioRects = 9,

      //game buttons
      kClimbTouch = kAmbientVolumeSliderText + 1,
      kAutoSearch,
      kBowZoom,
      kGoalNotify,
	  kAutoEquip,
      kNumGameRects = 5,

      //bind buttons
      kBindLoad = kAutoEquip + 1,
      kBindBack,
      kBindSave,
      kBindButt0, //0 to NUM_BIND_SLOTS - 1

      kBindScrollUp = kBindButt0 + NUM_BIND_SLOTS,
      kBindScrollDown,
      kNumBindButts = 3,
      kNumBindRects = 5 + NUM_BIND_SLOTS,

      kListBack = kBindScrollDown + 1,
      kList0,//13 of these, just add what is needed.
      kNumListButts = 1,
      kNumListRects = NUM_LIST + 1,

      
      kNumSubPanelRects = kNumControlRects + kNumBasicVideoRects + kNumAudioRects 
	                    + kNumGameRects + kNumBindRects + kNumListRects
   };

   enum {
      kVolumeSlider,
      kMouseSensSlider,
      kGammaSlider,
      kAmbientVolumeSlider,
      kNumSubPanelSlides
   };


   // NOTE: if you add to or change the kSub* enum below, you must update the
   //   array nextSubScreen in TermUI
   enum {
      kSubControls,
      kSubBasicVideo,
      kSubAudio,
      kSubGame,
      kSubBind,
      kSubList,
      kSubBindBlank,
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
      kMiscStrTouch,
      kMiscStrJump,
      kNumMiscStrs
   };


   int mPerSubRectNum [kNumSubs];


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
   int m_mwcookie;      // for mousewheel event handeler


   //slider stuff
   LGadSlider mSliders[kNumSubPanelSlides];
   bool mSlidersOn[kNumSubPanelSlides];
   DrawElement mSliderBaseBmp;

   ulong m_old_context;

   int m_volumeLookup[kVolumeSliderNotches];

   //slider variables
   int m_sens;
   int m_volume;
   int m_gamma;
   int m_ambientVolume;

   int mTmpBindNum;

   typedef bool (*ButtListCB)(ushort, int, void *, LGadBox *);


   //////////////////////////////////////////////////////

   void InitUI()
   {
      cDarkPanel::InitUI(); 

      //load in the tab and subpanel rectangles
      AppendRects ("optiontr", mTabRects);//tabs
      AppendRects ("optioncr", mSubPanelRects);//controls
      AppendRects ("optnbvr", mSubPanelRects);//basic video
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
      InitButtonList (&mTabDesc, &mTabButtons, &mTabRects[0], mTabElems, OnTabButton, mTabStrs, "tab_", kNumTabs, BUTTONLIST_RADIO_FLAG);
      LGadCreateButtonListDesc (&mTabButtons, LGadCurrentRoot(), &mTabDesc);   
      LGadBoxSetUserData (VB(&mTabButtons), this);
      //muy sucky
      //int sub_tabs[] = {0, 1, 1, 2, 3, 0, 1};
      LGadRadioButtonSelect (&mTabButtons, mCurSub);

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
      for (int i = 0; i < 4; i++) {
         mButtBmpRes[SLIDER_BMPRES][i] = pResMan->Bind ("slidbutt", RESTYPE_IMAGE, NULL, mResPath);
         if (!mButtBmpRes[SLIDER_BMPRES][i])
            AssertMsg (0, "Error loading button bitmap");
      }
      memset (&mButtDrawElem[SLIDER_BMPRES], 0, sizeof (DrawElement));
      mButtDrawElem[SLIDER_BMPRES].draw_data = (void *)mButtBmpRes[SLIDER_BMPRES];
      mButtDrawElem[SLIDER_BMPRES].draw_type = DRAWTYPE_RESOFFSET;

      //get our slider base bitmap
      memset (&mSliderBaseBmp, 0, sizeof (DrawElement));
      mSliderBaseBmp.draw_type = DRAWTYPE_RES;
      mSliderBaseBmp.draw_data = (void *)pResMan->Bind ("slidbase", RESTYPE_IMAGE, NULL, mResPath);

      for (int i = 0; i < (int)kNumSubPanelSlides; i++)
         mSlidersOn[i] = FALSE;

      // Initialize non-linear volume ramp.
      // Hard-code ends.
      m_volumeLookup[0] = -10000;
      m_volumeLookup[kVolumeSliderNotches - 1] = -1;
      // Decibel curve.
      for (int i = 1; i < (kVolumeSliderNotches - 1); i++) // A size 13 slider actually goes from 0..13, having 14 notches. doh
      {
         m_volumeLookup[i] = (int) (-1000.0F * (float) log ( (double) (kVolumeSliderNotches) / (double) (i+1)) / log(2.0) );
         //         m_volumeLookup[i] = - (1 << (13 - i));
      }

      //set sliders initial vals
      m_sens = int((atof (g_pInputBinder->ProcessCmd ("echo $mouse_sensitivity")) - 0.5) * 2.0);
//      m_volume = ((20.0 / 5001.0) * (SFX_GetMasterVolume () + 1.0)) + 20.0;//-5000 to -1
      m_volume = SliderVolumeLookup (SFX_GetMasterVolume(), m_volumeLookup, kVolumeSliderNotches);
      // hokay, we want to have each notch on the slider correspond to about 3 db, and the
      //   default ambient volume offset is 0, which should mean the slider is centered
      m_ambientVolume = (kAmbientVolumeSliderNotches/2) + (DarkSoundGetAmbientVolume() / kAmbientMillibelsPerNotch);
      if ( m_ambientVolume < 0 ) {
         m_ambientVolume = 0;
      }
      if ( m_ambientVolume > kAmbientVolumeSliderNotches - 1 ) {
         m_ambientVolume = kAmbientVolumeSliderNotches - 1;
      }
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
      uiInstallRegionHandler (slab->creg, UI_EVENT_MOUSE, StaticMouseWheelHandler, NULL, &m_mwcookie);
   }

   //////////////////////////////////////////////////////

   void TermUI()
   {
      // The comment formerly phrased as "muy sucky":
      // These tell which subscreen should be selected, after exiting the option panel
      //  and then returning.  This is so that if you escape-exit out of a sub-subscreen,
      //  you will be in the appropriate subscreen next time you click options
      static int nextSubScreen[] = { kSubControls, kSubBasicVideo, kSubAudio, kSubGame,
                                     kSubControls, kSubBasicVideo, kSubControls };

      LGadDestroyButtonList(&mTabButtons); 

      memset(&mTabElems,0,sizeof(mTabElems)); 
      for (int i = 0, j; i < kNumTabs; i++)
         mTabStrs[i] = ""; 

      TermSubPanel ();
      uiGameUnloadStyle (&mBindStyle); 
      uiGameUnloadStyle (&mTextStyle);

      for (int i = 0; i < 3; i++)
         for (int j = 0; j < 4; j++)
            mButtBmpRes[i][j]->Release ();

      ((IRes *)(mSliderBaseBmp.draw_data))->Release ();

      //save the current bindings upon exit of the options menu
      //change to game context
      ulong old_context;
      g_pInputBinder->GetContext (&old_context);
      g_pInputBinder->SetContext (HK_GAME_MODE,TRUE);
      cStr header = FetchUIString (panel_name, "bnd_header", mResPath);
      g_pInputBinder->SaveBndFile ("user.bnd", (char *)(const char *)header);
      //plop in input variable control settings
      BndAppendControls ();
      g_pInputBinder->SetContext (old_context,TRUE);

      //remove the binding keyboard handler
      uiSlab *slab;
      uiGetCurrentSlab (&slab);
      uiRemoveRegionHandler (LGadBoxRegion (slab->creg), m_cookie);
      uiRemoveRegionHandler (LGadBoxRegion (slab->creg), m_mwcookie);

      mCurSub = nextSubScreen[mCurSub];

      cDarkPanel::TermUI();
   }


   //////////////////////////////////////////////////////

   void TermSubPanel ()
   {
      LGadDestroyButtonList (&mSubPanelButtons[mCurSub]); 

      switch( mCurSub ) {
         case kSubBind:
            LGadDestroyButtonList (&mBindButtons);
            LGadDestroyButtonList (&mBindScrollers);
            break;
         case kSubList:
            LGadDestroyButtonList (&mListButtons);
            break;
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
			 if (reslist)  //doing resolution			
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
      if (mCurSub == kSubAudio) {
         SFX_SetMasterVolume (m_volumeLookup[m_volume]);
         DarkSoundSetAmbientVolume( kAmbientMillibelsPerNotch * (m_ambientVolume - (kAmbientVolumeSliderNotches/2)) );

//         SFX_SetMasterVolume (((5001.0 / 20.0) * (m_volume - 20.0)) - 1.0);//-5000 to -1
      }
      //sens
      else if (mCurSub == kSubControls) {
         char buf[32];
         sprintf (buf, "mouse_sensitivity %8.8f", ((double)m_sens / 2.0) + 0.5);//slide goes from 0.5 (slow) to 10.5 (fast)
         g_pInputBinder->ProcessCmd (buf);
      }
      //gamma
      if (mCurSub == kSubBasicVideo) {
         static int prev_gamma;
         if (prev_gamma != m_gamma) {
            TouchGamma ((((MAX_GAMMA - MIN_GAMMA) / 20.0) * (float)(20-m_gamma)) + MIN_GAMMA);
            prev_gamma = m_gamma;
         }
      }
   }

   //////////////////////////////////////////////////////

   void InitControlOptions ()
   {
      InitButtonList (&mSubPanelDesc[kSubControls], &mSubPanelButtons[kSubControls], 
                      &mSubPanelRects[(int)kCustomize], &mSubPanelElems[(int)kCustomize], OnSubPanelButton,
                      &mSubPanelStrs[(int)kCustomize], "control_", kNumControlButts, 0);
      mPerSubRectNum [(int)kSubControls] = (int)kNumControlRects;

      //set starting button strings
      char *str;

      str = g_pInputBinder->ProcessCmd ("echo $joystick_enable");
      SetUIString (mSubPanelStrs[(int)kJoyEnable], mSubPanelElems[(int)kJoyEnable], "control_1",
         (char *)(const char *)mMiscStrs[(atof (str) != 0.0) ? kMiscStrOn : kMiscStrOff]);
      
      str = g_pInputBinder->ProcessCmd ("echo $lookspring");
      SetUIString (mSubPanelStrs[(int)kLookspring], mSubPanelElems[(int)kLookspring], "control_2",
         (char *)(const char *)mMiscStrs[(atof (str) != 0.0) ? kMiscStrOn : kMiscStrOff]);
      
      str = g_pInputBinder->ProcessCmd ("echo $mouse_invert");
      SetUIString (mSubPanelStrs[(int)kMouseInvert], mSubPanelElems[(int)kMouseInvert], "control_3",
         (char *)(const char *)mMiscStrs[(atof (str) != 0.0) ? kMiscStrOn : kMiscStrOff]);

      str = g_pInputBinder->ProcessCmd ("echo $freelook");
      SetUIString (mSubPanelStrs[(int)kFreelook], mSubPanelElems[(int)kFreelook], "control_4",
         (char *)(const char *)mMiscStrs[(atof (str) != 0.0) ? kMiscStrOn : kMiscStrOff]);

      if (g_joystickActive)
      {
         str = g_pInputBinder->ProcessCmd ("echo $joy_rotate");
         SetUIString (mSubPanelStrs[(int)kJoyRotate], mSubPanelElems[(int)kJoyRotate], "control_5",
            (char *)(const char *)mMiscStrs[(atof (str) != 0.0) ? kMiscStrLeftRight : kMiscStrForwardBackward]);
      }
      else
         SetUIString (mSubPanelStrs[(int)kJoyRotate], mSubPanelElems[(int)kJoyRotate], "", 0);
   }

   //////////////////////////////////////////////////////

   void BndAppendControls ()
   {
      FILE *fp = fopen ("user.bnd", "at");
      if (fp)
      {
         fprintf (fp, "\n");

         char *controls[] = { "bow_zoom", "joystick_enable", "mouse_invert", "lookspring",
                              "freelook", "mouse_sensitivity", "joy_rotate", "joystick_sensitivity",
                              "joystick_deadzone", "rudder_sensitivity", "rudder_deadzone",
                              "auto_equip", "auto_search", "goal_notify", "climb_touch", "\0"};
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
      InitButtonList ( &mSubPanelDesc[kSubBasicVideo], &mSubPanelButtons[kSubBasicVideo],
                       &mSubPanelRects[(int)kHardwareDriver], &mSubPanelElems[(int)kHardwareDriver],
                       OnSubPanelButton, &mSubPanelStrs[(int)kHardwareDriver], "videob_", kNumBasicVideoButts, 0);
      mPerSubRectNum [(int)kSubBasicVideo] = kNumBasicVideoRects;
      
      //let's get some info about the video card
      mNumVidDevices = lgd3d_enumerate_devices ();

      sScrnMode mode;
      const sScrnMode *old_mode = GetGameScreenMode ();
      memcpy (&mode, old_mode, sizeof (sScrnMode));

      int fog_togval;
      config_get_int("fogging",&fog_togval);

	  g_zbuffer_toggle = TRUE; //make darn well sure.

      //the game pretty much requires these to be set.
      if (mNumVidDevices > 0 && (mode.flags & kScrnMode3dDriver)) {

		  portal_fog_on = (fog_togval == 1);
		  mode.flags |= kScrnMode3dDriver;         
		  SetUIString (mSubPanelStrs[(int)kFogToggle], mSubPanelElems[(int)kFogToggle], "videob_2",
            (char *)(const char *)mMiscStrs[(portal_fog_on && (mode.flags & kScrnMode3dDriver) != 0) ? kMiscStrOn : kMiscStrOff]);
      }

      else {	 
		  portal_fog_on = (fog_togval == 1);         
		  mode.flags &= ~kScrnMode3dDriver;         
      }

      SetGameScreenMode (&mode);

      SetUIString (mSubPanelStrs[(int)kWeatherToggle], mSubPanelElems[(int)kWeatherToggle], "videob_4", 
	 (char *)(const char *)mMiscStrs[(WeatherIsAllowed() && (mode.flags & kScrnMode3dDriver) != 0) ? kMiscStrOn : kMiscStrOff]);


      SetUIString (mSubPanelStrs[(int)kSkyDetail], mSubPanelElems[(int)kSkyDetail], "videob_3",
         (char *)(const char *)mMiscStrs[(SkyRendererTryEnhanced()) ? kMiscStrHigh : kMiscStrLow]);
   }

   //////////////////////////////////////////////////////

   void InitAudioOptions ()
   {
      InitButtonList ( &mSubPanelDesc[kSubAudio], &mSubPanelButtons[kSubAudio],
                       &mSubPanelRects[(int)kSpeakerTest], &mSubPanelElems[(int)kSpeakerTest], OnSubPanelButton,
                       &mSubPanelStrs[(int)kSpeakerTest], "audio_", kNumAudioButts, 0);
      mPerSubRectNum [(int)kSubAudio] = (int)kNumAudioRects; 

      SetUIString (mSubPanelStrs[(int)kStereoToggle], mSubPanelElems[(int)kStereoToggle], "audio_1",
         (char *)(const char *)mMiscStrs[(SFX_StereoReversed ()) ? kMiscStrOn : kMiscStrOff]);

      SetUIString (mSubPanelStrs[(int)kAudio3DHWToggle], mSubPanelElems[(int)kAudio3DHWToggle], "audio_2",
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
      InitButtonList ( &mSubPanelDesc[kSubGame], &mSubPanelButtons[kSubGame],
                       &mSubPanelRects[(int)kClimbTouch], &mSubPanelElems[(int)kClimbTouch], OnSubPanelButton,
                       &mSubPanelStrs[(int)kClimbTouch], "gameopt_", kNumGameRects, 0);

      mPerSubRectNum [(int)kSubGame] = kNumGameRects;
		      
      //set starting button strings
      char *str;

      str = g_pInputBinder->ProcessCmd ("echo $climb_touch");
      g_climb_on_touch = ((atof (str) != 0.0) ? TRUE : FALSE);
      SetUIString (mSubPanelStrs[(int)kClimbTouch], mSubPanelElems[(int)kClimbTouch], "gameopt_0",
	 (char *)(const char *)mMiscStrs[(atof (str) != 0.0) ? kMiscStrTouch : kMiscStrJump]);
      
      str = g_pInputBinder->ProcessCmd ("echo $auto_search");
      SetUIString (mSubPanelStrs[(int)kAutoSearch], mSubPanelElems[(int)kAutoSearch], "gameopt_1",
         (char *)(const char *)mMiscStrs[(atof (str) != 0.0) ? kMiscStrOn : kMiscStrOff]);

      str = g_pInputBinder->ProcessCmd ("echo $bow_zoom");
      SetUIString (mSubPanelStrs[(int)kBowZoom], mSubPanelElems[(int)kBowZoom], "gameopt_2",
         (char *)(const char *)mMiscStrs[(atof (str) != 0.0) ? kMiscStrOn : kMiscStrOff]);

      str = g_pInputBinder->ProcessCmd ("echo $goal_notify");
      SetUIString (mSubPanelStrs[(int)kGoalNotify], mSubPanelElems[(int)kGoalNotify], "gameopt_3",
         (char *)(const char *)mMiscStrs[(atof (str) != 0.0) ? kMiscStrOn : kMiscStrOff]);
      
      str = g_pInputBinder->ProcessCmd ("echo $auto_equip");
      SetUIString (mSubPanelStrs[(int)kAutoEquip], mSubPanelElems[(int)kAutoEquip], "gameopt_4",
         (char *)(const char *)mMiscStrs[(atof (str) != 0.0) ? kMiscStrOn : kMiscStrOff]);


   }

   //////////////////////////////////////////////////////

   void InitBindOptions ()
   {
      int i;

      InitButtonList ( &mSubPanelDesc[kSubBind], &mSubPanelButtons[kSubBind],
                       &mSubPanelRects[(int)kBindLoad], &mSubPanelElems[(int)kBindLoad], OnSubPanelButton,
                       &mSubPanelStrs[(int)kBindLoad], "bind_", kNumBindButts, 0);
      mPerSubRectNum [(int)kSubBind] = (int)kNumBindRects;

      //set up description for binding buttons. joy.
      // rearrange a few rects, because pink boxes are in order <scrollUp> <14 bind buttons> <scrollDn>,
      //  and we need the scrollUp & scrollDn buttons to have adjacent indices
      mTopBind = 0;
      Rect tmp_rect;
      tmp_rect = mSubPanelRects[kBindButt0];
      for (i = 0; i < NUM_BIND_SLOTS; i++) {
         mSubPanelRects[kBindButt0 + i] = mSubPanelRects[kBindButt0 + i + 1];
         memset (&mBindButtonElems[i], 0, sizeof(DrawElement)); 
         mBindButtonElems[i].draw_type = DRAWTYPE_TEXT;
      }
      mSubPanelRects[kBindScrollUp] = tmp_rect;

      //description of binding radio buttons
      {
         LGadButtonListDesc tmp_desc = {NUM_BIND_SLOTS, (Rect *)&mSubPanelRects[(int)kBindButt0], 
                                        //mBindButtonElems, OnBindButton, 0, BUTTONLIST_RADIO_FLAG};
            mBindButtonElems, OnBindButton, 0, 0};
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
         memcpy (&mSubPanelDesc[kSubList], &tmp_desc, sizeof (LGadButtonListDesc));
      }

      mSubPanelStrs[(int)kListBack] = FetchUIString (panel_name, "bind_1", mResPath);
      memset (&mSubPanelElems[(int)kListBack], 0, sizeof (DrawElement));
      mSubPanelElems[(int)kListBack].draw_data = (void *)(const char *)mSubPanelStrs[(int)kListBack];
      mSubPanelElems[(int)kListBack].draw_type = DRAWTYPE_TEXT;

      //description of list buttons
      {
         LGadButtonListDesc tmp_desc = {NUM_LIST, (Rect *)&mSubPanelRects[(int)kList0], 
            mListButtonElems, OnListButton, 0, BUTTONLIST_RADIO_FLAG};
         memcpy (&mListButtonDesc, &tmp_desc, sizeof (LGadButtonListDesc));
      }

      mPerSubRectNum [(int)kSubList] = (int)kNumListRects;
      mTopList = 0;
   }



   //////////////////////////////////////////////////////

   void GetBindingStrings( long bind_num, cStr *pFuncStr, cStr *pBindStr )
   {
      //get our bindings
      int i, j, len;

      char buf[128];
      sprintf (buf, "bindname_%d", bind_num);

      *pFuncStr = FetchUIString (panel_name, buf, mResPath);
      *pBindStr = "";

      cStr controls[MAX_BINDS_PER_CMD];
      int num = GetCmdBinds (mBindCmd[bind_num], controls, MAX_BINDS_PER_CMD);

      for (i = 0; i < num; i++) {
         if (i != 0)
            *pBindStr += ", ";
         // replace spaces within a binding name with underscores
         buf[sizeof(buf) - 1] = 0;
         strncpy( buf, (const char *) controls[i], sizeof(buf) - 1 );
         len = strlen( buf );
         for ( j = 0; j < len; j++ ) {
            if ( buf[j] == ' ' ) {
               buf[j] = '_';
            }
         }
         *pBindStr += buf;
      }

      //there were no controls
      if (i == 0)
         *pBindStr += mMiscStrs[kMiscStrEmpty];
        
   }


   //////////////////////////////////////////////////////

   void FillBindStr (long bind_num, long butt_num)
   {
      cStr tmpStr;

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
         g_pInputBinder->SetContext (HK_GAME_MODE,TRUE);

      GetBindingStrings( bind_num, &(mBindButtonStrs[bind_num]), &tmpStr );
      mBindButtonStrs[bind_num] += (" = " + tmpStr);

      if (old_context != HK_GAME_MODE)
         g_pInputBinder->SetContext (old_context,TRUE);

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

      switch (event->code) {
         case KEY_UP:
         case KEY_PAD_UP:
            if (mTopBind > 0) {
               mTopBind--;
               FillBindStrs ();
            }
            else
               redraw = FALSE;
            
         break;


         case KEY_DOWN:
         case KEY_PAD_DOWN:
            if (mTopBind < mNumBindable - NUM_BIND_SLOTS) {
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
            }

            FillBindStrs ();

         break;

         case KEY_PGDN:
         case KEY_PAD_PGDN:
            mTopBind += NUM_BIND_SLOTS;
            if (mTopBind >= (mNumBindable - NUM_BIND_SLOTS)) {
               mTopBind = mNumBindable - NUM_BIND_SLOTS;
            }

            FillBindStrs ();

         break;

         
         case KEY_HOME:
         case KEY_PAD_HOME:
            if (mTopBind != 0) {
               mTopBind = 0;
               FillBindStrs ();
            }
         break;

         
         case KEY_END:
         case KEY_PAD_END:
            if (mTopBind != (mNumBindable - NUM_BIND_SLOTS)) {
               mTopBind = mNumBindable - NUM_BIND_SLOTS;
               FillBindStrs ();
            }
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

   static bool 
   StaticMouseWheelHandler (uiEvent *p_event, Region *p_reg, void *state)
   {
      bool ret = FALSE;
      int i, j;
      uiMouseEvent *pMouseEvent = (uiMouseEvent *) p_event;

      if ( (gpOptions->mCurSub == gpOptions->kSubBind) && (p_event->type == UI_EVENT_MOUSE)
           && (pMouseEvent->wheel != 0) ) {

         if ( pMouseEvent->wheel > 0 ) {
            j = 0;
            i = pMouseEvent->wheel;
         } else {
            j = 1;
            i = (-pMouseEvent->wheel);
         }
         while ( i-- ) {
            gpOptions->OnScrollList( BUTTONGADG_LCLICK, j );
         }

         ret = TRUE;
      }
      return ret;
   }


   //////////////////////////////////////////////////////

   void SetVidRes ()
   {
      //has not changed
      if (mSelectedRes == -1)
         return;

      sScrnMode mode;
      const sScrnMode *old_mode = GetGameScreenMode ();
      memcpy (&mode, old_mode, sizeof (sScrnMode));

      char *mode_desc = (char *)(const char *)mListButtonStrs[mSelectedRes];
      sscanf (mode_desc, "%dx%dx%d", &mode.w, &mode.h, &mode.bitdepth);

      SetGameScreenMode (&mode);
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

         memset (&mListButtonElems[i], 0, sizeof (DrawElement));      
         mListButtonElems[i].draw_data = (void *)(const char *)mListButtonStrs[i];
         mListButtonElems[i].draw_type = DRAWTYPE_TEXT;
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

         memset (&mListButtonElems[i], 0, sizeof (DrawElement));      
         mListButtonElems[i].draw_data = (void *)(const char *)mListButtonStrs[i];
         mListButtonElems[i].draw_type = DRAWTYPE_TEXT;
      }

      int idx = mNumVidDevices - 1; 
      if (config_is_defined("d3d_driver_index"))
         config_get_int("d3d_driver_index", &idx);
      LGadRadioButtonSelect (&mListButtons,  mListPrevPick = idx);
   }

   //////////////////////////////////////////////////////

   void ClearSelectedBinding( void )
   {
      char str[128], control_buf[64];

      g_pInputBinder->GetControlFromCmdStart ((char *)(const char *)mBindCmd[mTmpBindNum], control_buf);
      while (*control_buf != '\0') {
         strcpy (str, "unbind ");
         strcat (str, control_buf);
         g_pInputBinder->ProcessCmd (str);
         g_pInputBinder->GetControlFromCmdStart ((char *)(const char *)mBindCmd[mTmpBindNum], control_buf);
      }
   }

   //////////////////////////////////////////////////////

   static BOOL BindFilter (char *control, char *cmd, void *data)
   {
      //we'll reserve ESC for breaking from a bind query
      if (!strcmp (control, "esc"))
         return FALSE;

      // alt-backspace means clear current bindings
      if ( !strcmp (control, "backspace+alt")) {
         gpOptions->ClearSelectedBinding();
         return FALSE;
      }

      //don't bind joystick movement
      if (!strcmp (control, "joy_move"))
         return FALSE;

      //scroll lock doesnt have up/down events like a normal key
      if (!strcmp (control, "scroll_lock"))
         return FALSE;

      return TRUE;
   }

   //////////////////////////////////////////////////////

   static void PostBindFunc (BOOL bound)
   {  
      for (long i = 0; i < gpOptions->mNumBindable; i++)
         gpOptions->mBindButtsFilled[i] = FALSE;
      gpOptions->FillBindStrs ();
      gpOptions->DelayedSwapSubPanel( kSubBind );

      g_pInputBinder->SetContext (gpOptions->m_old_context,TRUE);
   }

   //////////////////////////////////////////////////////

   void InitButtonList (LGadButtonListDesc *desc, LGadButtonList *list, Rect *rect_array, DrawElement *draw_elem,
                        ButtListCB cb, cStr *strs, const char *prefix, int num, int flags)
   {
      // set up drawlelems
      for (int i = 0; i < num; i++) {
         DrawElement *elem = &draw_elem[i];
         memset(elem, 0, sizeof(DrawElement)); 

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
      memset( list, 0, sizeof(LGadButtonList) );
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

      memset (draw_elem, 0, sizeof (DrawElement));
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
      memcpy (&slider->gadg.draw, &mButtDrawElem[SLIDER_BMPRES], sizeof (DrawElement));
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
      memset (&draw, 0, sizeof (DrawElement));
      draw.draw_type = DRAWTYPE_TEXT;
      draw.draw_data = s;
      draw.fcolor = mTextStyle.colors[0];
      ElementDraw (&draw, dsNORMAL, r.ul.x, r.ul.y, r.lr.x - r.ul.x, r.lr.y - r.ul.y);
   }

   //////////////////////////////////////////////////////

   void RedrawDisplay ()
   {
      // draw buttonlist
      region_expose (LGadBoxRegion (&mTabButtons), LGadBoxRect (&mTabButtons));
      if ( mCurSub != kSubBindBlank ) {
         DrawButtonList (&mSubPanelDesc[mCurSub]);
      }

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
            DrawSlider ((int)kVolumeSlider, (int)kVolumeSliderRect);
            DrawString ("volume", kVolumeSliderText);
            DrawSlider ((int)kAmbientVolumeSlider, (int)kAmbientVolumeSliderRect);
            DrawString ("ambient_volume", kAmbientVolumeSliderText);
         break;

         case kSubGame:
            region_expose (LGadBoxRegion (&mSubPanelButtons[kSubGame]), LGadBoxRect (&mSubPanelButtons[kSubGame]));
         break;

         case kSubBind:
            DrawButtonList (&mBindScrollerDesc);
            region_expose (LGadBoxRegion (&mBindButtons), LGadBoxRect (&mBindButtons)); 
         break;       

         case kSubList:
            region_expose (LGadBoxRegion (&mListButtons), LGadBoxRect (&mListButtons)); 
//            DrawButtonList (&mBindScrollerDesc);
         break;

         case kSubBindBlank:
            // just a test for now
            DrawElement draw;
            Rect r;
            cStr msg, tmpBindName, tmpBinding;
            grs_clip    oldClip;

            memset (&draw, 0, sizeof (DrawElement));
            draw.draw_type = DRAWTYPE_TEXT;
            draw.fcolor = mTextStyle.colors[0];
            GetBindingStrings( mTmpBindNum, &tmpBindName, &tmpBinding );

            // "Hit key or button to bind to";
            DrawString( "bind_title", kBindButt0 );

            draw.draw_data = (char *) (const char *) tmpBindName;
            r = mSubPanelRects[kBindButt0 + 1];
            ElementDraw (&draw, dsNORMAL, r.ul.x, r.ul.y, r.lr.x - r.ul.x, r.lr.y - r.ul.y);

            // "Current bindings:";
            DrawString( "bind_current", kBindButt0 + 4 );

            draw.draw_data = (char *) (const char *) tmpBinding;
            r.ul = mSubPanelRects[kBindButt0 + 5].ul;
            r.lr = mSubPanelRects[kBindButt0 + 6].lr;
            // save old clip window, clip to map window
            gr_save_cliprect( &oldClip.i );
            gr_set_cliprect( r.ul.x, r.ul.y, r.lr.x, r.lr.y );
            draw.draw_flags = FORMAT(DRAWFLAG_FORMAT_WRAP);
            ElementDraw (&draw, dsNORMAL, r.ul.x, r.ul.y, r.lr.x - r.ul.x, r.lr.y - r.ul.y);
            draw.draw_flags = 0;
            // restore old clip window
            gr_restore_cliprect( &oldClip.i );

            // "Hit escape to leave bindings unchanged"
            DrawString( "bind_cancel", kBindButt0 + 9 );

            // "Hit alt-backspace to clear bindings"
            DrawString( "bind_clear", kBindButt0 + 11 );

         break;       

        }
   }

   //////////////////////////////////////////////////////

   void FillBlack (int rect_num)
   {
      ushort old_fcolor = grd_canvas->gc.fcolor;
      grd_canvas->gc.fcolor = 0;//fill w/ black

      Rect *rect;
      if (rect_num == FILL_SUBPANEL)
         rect = (Rect *)&mRects[(int)kSubFillRect];
      else
         rect = (Rect *)&mSubPanelRects[rect_num];
      
      uiHideMouse (rect);
      gd_rect (rect->ul.x, rect->ul.y, rect->lr.x, rect->lr.y);
      uiShowMouse (rect);
   }

   //////////////////////////////////////////////////////

   void SwapSubPanel (int new_sub)
   {
      if ( (mCurSub != -1) && (mCurSub != kSubBindBlank) ) {
         LGadDestroyButtonList (&mSubPanelButtons[mCurSub]); 
         TermSpecial ();
      }

      if (new_sub!=kSubAudio)
         metaSndEnterPanel(kMetaSndPanelOptions);
      else
         metaSndExitPanel(TRUE);

      if ( new_sub != kSubBindBlank ) {
         LGadCreateButtonListDesc (&mSubPanelButtons[new_sub], LGadCurrentRoot (), &mSubPanelDesc[new_sub]);
         LGadBoxSetUserData (VB(&mSubPanelButtons[new_sub]), this);
      }

      if (new_sub == kSubControls) {
         DrawSlider ((int)kMouseSensSlider, (int)kMouseSensSliderRect, FALSE);
         CreateSlider ((int)kMouseSensSlider, (int)kMouseSensSliderRect, &m_sens, 20, 1);
      }
      else if (new_sub == kSubBasicVideo) {
         DrawSlider ((int)kGammaSlider, (int)kGammaSliderRect, FALSE);
         CreateSlider ((int)kGammaSlider, (int)kGammaSliderRect, &m_gamma, 20, 1);
      }
      else if (new_sub == kSubAudio) {
         DrawSlider ((int)kVolumeSlider, (int)kVolumeSliderRect, FALSE);
         CreateSlider ((int)kVolumeSlider, (int)kVolumeSliderRect, &m_volume, kVolumeSliderNotches - 1, 1);

         DrawSlider ((int)kAmbientVolumeSlider, (int)kAmbientVolumeSliderRect, FALSE);
         CreateSlider ((int)kAmbientVolumeSlider, (int)kAmbientVolumeSliderRect,
                       &m_ambientVolume, kAmbientVolumeSliderNotches - 1, 1);

         if (! CanChangeSoundDeviceNow ()) {
            mSubPanelDesc[kSubAudio].button_elems[kAudioChannels - kSpeakerTest].fcolor = mGreyStyle.colors[0];  
            mSubPanelDesc[kSubAudio].button_elems[kAudio3DHWToggle - kSpeakerTest].fcolor = mGreyStyle.colors[0];  
         }

         if (!CanChangeEAX())
            mSubPanelDesc[kSubAudio].button_elems[kAudioEAXToggle - kSpeakerTest].fcolor = mGreyStyle.colors[0];
      }
      else if (new_sub == kSubGame) {
      }
      else if (new_sub == kSubBind) {
         LGadCreateButtonListDesc (&mBindButtons, LGadCurrentRoot (), &mBindButtonDesc);
         LGadBoxSetUserData (VB(&mBindButtons), this);
         LGadBoxSetStyle (&mBindButtons, &mBindStyle); 

         LGadCreateButtonListDesc (&mBindScrollers, LGadCurrentRoot (), &mBindScrollerDesc);
         LGadBoxSetUserData (VB(&mBindScrollers), this);
      }

      else if (new_sub == kSubList) {
         LGadCreateButtonListDesc (&mListButtons, LGadCurrentRoot (), &mListButtonDesc);
         LGadBoxSetUserData (VB(&mListButtons), this);
         LGadBoxSetStyle (&mListButtons, &mBindStyle); 

//         LGadCreateButtonListDesc (&mBindScrollers, LGadCurrentRoot (), &mBindScrollerDesc);
//         LGadBoxSetUserData (VB(&mBindScrollers), this);

         if (mListSub == kSubBasicVideo)
		 {
			if (reslist)
				FillVidResStrs();
			else
				FillVidDevStrs();
		 }
         mTopList = 0;
      }

      FillBlack (FILL_SUBPANEL);

      mCurSub = new_sub;
      RedrawDisplay ();
   }

   void DelayedSwapSubPanel (int new_sub)
   {
      // swap to new sub panel on next loop
      mSwap = new_sub;
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
            if (mCurSub != kSubBasicVideo) {
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

      int scaled_button = button;
      int fog_togval, i;
      for (i = 0; i < mCurSub; i++)
         scaled_button += mPerSubRectNum[i];

      config_get_int("fogging",&fog_togval);
      
      switch (scaled_button)
      {

         case kScreenRes:
            mSwap = kSubList;
            mListSub = kSubBasicVideo;
            mSelectedRes = -1;
            mTopList = 0;
			reslist = TRUE; //hack to know which sublist we took
         break;


         case kWeatherToggle:
            WeatherSetAllowed(!WeatherIsAllowed()); //toggle
            SetUIString (mSubPanelStrs[(int)kWeatherToggle], mSubPanelElems[(int)kWeatherToggle], "videob_4",
               (char *)(const char *)mMiscStrs[(WeatherIsAllowed()) ? kMiscStrOn : kMiscStrOff]);
            FillBlack ((int)kWeatherToggle);
	    //tell anything that wants it that the sky is on here.
            RedrawDisplay ();
         break;

         case kSkyDetail:
            SkyRendererSetEnhanced(!SkyRendererTryEnhanced()); //toggle
            SetUIString (mSubPanelStrs[(int)kSkyDetail], mSubPanelElems[(int)kSkyDetail], "videob_3",
               (char *)(const char *)mMiscStrs[(SkyRendererTryEnhanced()) ? kMiscStrHigh : kMiscStrLow]);
            FillBlack ((int)kSkyDetail);
	    //tell anything that wants it that the sky is on here.
            RedrawDisplay ();
         break;

         case kHardwareDriver:
            mSwap = kSubList;
            mListSub = kSubBasicVideo;
            mTopList = 0;
			reslist = FALSE; //hack to know which sublist we took
         break;

	 case kFogToggle:
	    {
	    portal_fog_on = (fog_togval == 1);
            const sScrnMode *old_mode = GetGameScreenMode ();
            if (mNumVidDevices > 0 && old_mode->flags & kScrnMode3dDriver) 
	      {
                  if (!portal_fog_on) {
                     SetUIString (mSubPanelStrs[(int)kFogToggle], mSubPanelElems[(int)kFogToggle], "videob_2",
                        (char *)(const char *)mMiscStrs[kMiscStrOn]);
                     portal_fog_on = TRUE;
		     fog_togval = 1;
		     config_set_int("fogging",fog_togval);
                  }
                  else {
                     SetUIString (mSubPanelStrs[(int)kFogToggle], mSubPanelElems[(int)kFogToggle], "videob_2",
                        (char *)(const char *)mMiscStrs[kMiscStrOff]);
                     portal_fog_on = FALSE;
		     fog_togval = 0;
		     config_set_int("fogging",fog_togval);
                  }
                  FillBlack ((int)kFogToggle);
                  RedrawDisplay ();
		}
	  }
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

         case kAudio3DHWToggle:
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
            
               SetUIString (mSubPanelStrs[(int)kAudio3DHWToggle], mSubPanelElems[(int)kAudio3DHWToggle], "audio_2",
                  (char *)(const char *)mMiscStrs[(SFX_GetSoundDevice () == SFXDEVICE_A3D) ? kMiscStrOn : kMiscStrOff]);
               FillBlack (kAudio3DHWToggle);
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
                  mSubPanelDesc[kSubAudio].button_elems[kAudioEAXToggle - kSpeakerTest].fcolor = mGreyStyle.colors[0];
               FillBlack (kAudioEAXToggle);
               
               RedrawDisplay();
            }
         break;

         case kAudioChannels:
            if (CanChangeSoundDeviceNow())
            {  //4, 8, 12
               SFXClose ();
               sfx_use_channels += 4;
#define AUDIO_CHANNEL_LIMIT 24
               if (sfx_use_channels > AUDIO_CHANNEL_LIMIT)
                  sfx_use_channels = 4;
               config_set_int("sfx_channels", sfx_use_channels);

               // evil hack - modify the caps on the different types of sounds
               //   based on number of channels - patmc
               extern int gSoundTypeCaps[];
               for ( i = 0; i < kNumSchemaTypes; i++ ) {
                  gSoundTypeCaps[i] = 4;
               }
               if ( sfx_use_channels > 8 ) {
                  if ( sfx_use_channels > 12 ) {
                     for ( i = 0; i < kNumSchemaTypes; i++ ) {
                        gSoundTypeCaps[i] = 6;
                     }
                     gSoundTypeCaps[kSchemaTypeAmbient] = 10;
                  } else {
                     gSoundTypeCaps[kSchemaTypeAmbient] = 8;
                  }
               }

               SFXInit ();
               
               char buf[8];
               sprintf (buf, "%d", sfx_use_channels);
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

         case kBindLoad:
            SwitchToLoadBndMode (TRUE); 
         break;


         case kBindSave:
            SwitchToSaveBndMode (TRUE); 
         break;
         
         case kBindBack:
            DestroyAllSliders ();
            mSwap = kSubControls;
         break;


         case kJoyEnable:
         {
            char *joyed = g_pInputBinder->ProcessCmd ("echo $joystick_enable"), buf[16];
            sprintf (buf, "control_%d", button);
            if (atof (joyed) == 0.0) {
               g_pInputBinder->ProcessCmd ("joystick_enable 1");
               g_joystickActive=TRUE;
               SetUIString (mSubPanelStrs[(int)kJoyEnable], mSubPanelElems[(int)kJoyEnable], buf, (char *)(const char *)mMiscStrs[kMiscStrOn]);
            }
            else {
               g_pInputBinder->ProcessCmd ("joystick_enable 0");
               g_joystickActive=FALSE;
               SetUIString (mSubPanelStrs[(int)kJoyEnable], mSubPanelElems[(int)kJoyEnable], buf, (char *)(const char *)mMiscStrs[kMiscStrOff]);
            }
            FillBlack ((int)kJoyEnable);
	    //now that we've possibly enabled joystick, make sure
	    //we display the joyrotate bit.
	    if (g_joystickActive)
	      {
		char *str;
		str = g_pInputBinder->ProcessCmd ("echo $joy_rotate");
		SetUIString (mSubPanelStrs[(int)kJoyRotate], mSubPanelElems[(int)kJoyRotate], "control_5",
			     (char *)(const char *)mMiscStrs[(atof (str) != 0.0) ? kMiscStrLeftRight : kMiscStrForwardBackward]);
	      }
	    else
	      SetUIString (mSubPanelStrs[(int)kJoyRotate], mSubPanelElems[(int)kJoyRotate], "", 0);
	    
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

	 case kClimbTouch:
	 {
	   char *climbtouching = g_pInputBinder->ProcessCmd ("echo $climb_touch"), buf[16];
	   //reverse of above, since we are about to reverse it.
	   g_climb_on_touch = ((atof (climbtouching) != 0.0) ? FALSE : TRUE);
	   sprintf (buf, "gameopt_0", button);
	   if (atof (climbtouching) == 0.0) {
	     g_pInputBinder->ProcessCmd ("climb_touch 1");
	     //do thing here.
	     SetUIString (mSubPanelStrs[(int)kClimbTouch], mSubPanelElems[(int)kClimbTouch], buf, (char *)(const char *)mMiscStrs[kMiscStrTouch]);
	   }
	   else {
	     g_pInputBinder->ProcessCmd ("climb_touch 0");
	     //do thing here.
	     SetUIString (mSubPanelStrs[(int)kClimbTouch], mSubPanelElems[(int)kClimbTouch], buf, (char *)(const char *)mMiscStrs[kMiscStrJump]);
	   }
	   FillBlack ((int)kClimbTouch);
	   RedrawDisplay ();
         }
         break;

         case kAutoSearch:
         {
            char *autosearched = g_pInputBinder->ProcessCmd ("echo $auto_search"), buf[16];
            sprintf (buf, "gameopt_1");
            if (atof (autosearched) == 0.0) {
               g_pInputBinder->ProcessCmd ("auto_search 1");
	       //do work here
               SetUIString (mSubPanelStrs[(int)kAutoSearch], mSubPanelElems[(int)kAutoSearch], buf, (char *)(const char *)mMiscStrs[kMiscStrOn]);
            }
            else {
               g_pInputBinder->ProcessCmd ("auto_search 0");
	       //do work here
               SetUIString (mSubPanelStrs[(int)kAutoSearch], mSubPanelElems[(int)kAutoSearch], buf, (char *)(const char *)mMiscStrs[kMiscStrOff]);
            }
            FillBlack ((int)kAutoSearch);
            RedrawDisplay ();
         }
         break;

         case kBowZoom:
         {
            char *zoomed = g_pInputBinder->ProcessCmd ("echo $bow_zoom"), buf[16];
            sprintf (buf, "gameopt_2");
            if (atof (zoomed) == 0.0) {
               g_pInputBinder->ProcessCmd ("bow_zoom 1");
               SetBowZoomActive (TRUE);
               SetUIString (mSubPanelStrs[(int)kBowZoom], mSubPanelElems[(int)kBowZoom], buf, (char *)(const char *)mMiscStrs[kMiscStrOn]);
            }
            else {
               g_pInputBinder->ProcessCmd ("bow_zoom 0");
               SetBowZoomActive (FALSE);
               SetUIString (mSubPanelStrs[(int)kBowZoom], mSubPanelElems[(int)kBowZoom], buf, (char *)(const char *)mMiscStrs[kMiscStrOff]);
            }
            FillBlack ((int)kBowZoom);
            RedrawDisplay ();
         }
         break;

         case kGoalNotify:
         {
            char *notified = g_pInputBinder->ProcessCmd ("echo $goal_notify"), buf[16];
            sprintf (buf, "gameopt_3", button);
            if (atof (notified) == 0.0) {
               g_pInputBinder->ProcessCmd ("goal_notify 1");
               //do work here.
               SetUIString (mSubPanelStrs[(int)kGoalNotify], mSubPanelElems[(int)kGoalNotify], buf, (char *)(const char *)mMiscStrs[kMiscStrOn]);
            }
            else {
               g_pInputBinder->ProcessCmd ("goal_notify 0");
               //do work here.
               SetUIString (mSubPanelStrs[(int)kGoalNotify], mSubPanelElems[(int)kGoalNotify], buf, (char *)(const char *)mMiscStrs[kMiscStrOff]);
            }
            FillBlack ((int)kGoalNotify);
            RedrawDisplay ();
         }
         break;
      
		 case kAutoEquip:
         {
            char *notified = g_pInputBinder->ProcessCmd ("echo $auto_equip"), buf[16];
            sprintf (buf, "gameopt_4", button);
            if (atof (notified) == 0.0) {
               g_pInputBinder->ProcessCmd ("auto_equip 1");
               //do work here.
               SetUIString (mSubPanelStrs[(int)kAutoEquip], mSubPanelElems[(int)kAutoEquip], buf, (char *)(const char *)mMiscStrs[kMiscStrOn]);
            }
            else {
               g_pInputBinder->ProcessCmd ("auto_equip 0");
               //do work here.
               SetUIString (mSubPanelStrs[(int)kAutoEquip], mSubPanelElems[(int)kAutoEquip], buf, (char *)(const char *)mMiscStrs[kMiscStrOff]);
            }
            FillBlack ((int)kAutoEquip);
            RedrawDisplay ();
         }
         break;
      
         case kListBack:
            mSwap = mListSub;
            DestroyAllSliders ();

            if (mListSub == kSubBasicVideo) {
			   if (reslist)
				   SetVidRes();
			   else
                   config_set_int ("d3d_driver_index", m3dDriver = LGadRadioButtonSelection (&mListButtons));			   
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
				   if (reslist)
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
                  }
                  else
                     redraw = FALSE;
               break;

               //resolutions
               case kSubList:
				  if (reslist)                  					 
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

      int bind_num = mTopBind + LGadRadioButtonSelection (&mBindButtons);
      mTmpBindNum = bind_num;
      char buf[16];
      sprintf (buf, "bindname_%d", bind_num);
      mBindButtonStrs[bind_num] = FetchUIString (panel_name, buf, mResPath);
      mBindButtonStrs[bind_num] += " = ?";

      //change to game context
      g_pInputBinder->GetContext (&m_old_context);
      g_pInputBinder->SetContext (HK_GAME_MODE,TRUE);
      g_pInputBinder->TrapBind ((char *)(const char *)mBindCmd[bind_num], BindFilter, PostBindFunc, NULL);
      // input binder context will be restored by PostBindFunc

      mSwap = kSubBindBlank;
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

      //resolution choice
      if ((mListSub == kSubBasicVideo) && (reslist==TRUE)) {
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
	  //display device choice
	  else if ((mListSub == kSubBasicVideo) && (reslist == FALSE)) {
		  if (button >= mNumVidDevices) {
            LGadRadioButtonSelect (&mListButtons, mListPrevPick);
            valid_butt = FALSE;
         }

         //there was something there
         else {
            config_set_int ("d3d_driver_index", button);
            m3dDriver = button;
         } //not greater than number
      } //not sub basic video, or doing resolution
      
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



EXTERN void SwitchToOptionsMode(BOOL push) 
{
   if (gpOptions)
   {
      cAutoIPtr<IPanelMode> panel = gpOptions->GetPanelMode(); 
      panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
   }
}


const sLoopInstantiator* DescribeOptionsMode(void)
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
int GetCmdBinds (const char *cmd, cStr *dest, int retrieve_num)
{
   return gpOptions->GetCmdBinds (cmd, dest, retrieve_num);
}



//
// Init and term stuff
//

EXTERN void OptionsMenuInit ()
{   
   gpOptions = new cOptions;
   gpOptions->SetInitialSub ();
}

EXTERN void OptionsMenuTerm ()
{
   delete gpOptions;
}
