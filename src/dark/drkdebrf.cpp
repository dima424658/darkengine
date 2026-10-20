/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkdebrf.cpp,v 1.22 2000/03/23 21:33:06 adurant Exp $
#include <appagg.h>
#include <drkdebrf.h>

#include <cursors.h>

#include <scrnmode.h>
#include <scrnman.h>

#include <drkpanl.h>

#include <drkuires.h>
#include <imgsrc.h>
#include <resapi.h>
#include <respaths.h>
#include <fonrstyp.h>
#include <palrstyp.h>
#include <palmgr.h>

#include <string.h>
#include <str.h>

#include <appapi.h>

#include <command.h>
#include <config.h>
#include <cfgdbg.h>
#include <ctype.h>

#include <simman.h>
#include <playrobj.h>
#include <buttpanl.h>
#include <guistyle.h>
#include <uigame.h>
#include <drkgoalr.h>
#include <drkgoalt.h>
#include <questapi.h>
#include <drkmiss.h>
#include <drkamap.h>
#include <drkmenu.h>

#include <dbfile.h>
#include <dbasemsg.h>
#include <drkmislp.h>
#include <tagfile.h>
#include <dbtagfil.h>
#include <quesfile.h>
#include <scrnman.h>
#include <cursors.h>
#include <drkdiff.h>

#include <gcompose.h>

// For status dials
#include <stubfile.h>
#include <gshelapi.h>
#include <gameinfo.h>
#include <drksave.h>
#include <missiondiag.h>
#include <dataguard.h>
#include <gameapp.h>

#include <btffact.h>


//
// Include these last!
//
#include <dbmem.h>

enum eFlags 
{
   kShowStats = 1 << 0, 
   kFictionGoals = 1 << 1, 
}; 


class cObjectivesBase: public cDarkPanel
{
public:
   cObjectivesBase(const sDarkPanelDesc *desc, ulong flags)
      : cDarkPanel(desc), mFlags(flags)
   {
   }

   ~cObjectivesBase() {}; 

   
protected:
   BOOL DisplayFits(const Rect& r, grs_font* font); 

   void RedrawDisplay(); 
   void InitUI()
   {
      cDarkPanel::InitUI(); 
      memset(&mTextStyle,0,sizeof(mTextStyle));
      // default to current style colors
      AssertMsg(GetCurrentStyle(),"No current style for diff defaults"); 
      memcpy(mTextStyle.colors,GetCurrentStyle()->colors,sizeof(mTextStyle.colors)); 
      uiGameLoadStyle("goal_text_",&mTextStyle,mResPath); 

      memset(&mGreyStyle,0,sizeof(mGreyStyle));
      // default to current style colors
      AssertMsg(GetCurrentStyle(),"No current style for diff defaults"); 
      memcpy(mGreyStyle.colors,GetCurrentStyle()->colors,sizeof(mGreyStyle.colors)); 
      uiGameLoadStyle("grey_",&mGreyStyle,mResPath);      
   }

   void TermUI()
   {
      uiGameUnloadStyle(&mTextStyle); 
      uiGameUnloadStyle(&mGreyStyle);
      cDarkPanel::TermUI();
   }

   ulong mFlags;
   guiStyle mTextStyle;
   guiStyle mGreyStyle; 
};

enum eMargins
{
   kLeftGoalTextMargin = 16,
   kStatsVertSeparator = 16,
   kTextIconAlign = -3,
};

enum 
{
   kMinYSpace = 5, 
};

static int compute_yspace(grs_font* font)
{
   int retval = gr_font_string_height(font,"X")/4; 
   if (retval > kMinYSpace)
      return retval; 
   return kMinYSpace; 
}

enum eDrawGoalResult
{
   kNoGoal, 
   kSkipGoal, 
   kDrawGoal,
};


static eDrawGoalResult should_draw_goal(IQuestData* pQuest, int i)
{
   cStr var = GoalStateVarName(i); 
   // If there's no goal state, then we've passed the last goal 
   if (!pQuest->Exists(var))
      return kNoGoal; 

   cStr visvar = GoalVisibleVarName(i); 
   // Check to see if the goal is visible 
   if (!pQuest->Get(visvar))
      return kSkipGoal; 
      
   int state = pQuest->Get(var); 

   int diff = pQuest->Get(DIFF_QVAR); 

   // check against min difficulty 
   cStr minvar = GoalMinDiffVarName(i); 
   if (pQuest->Exists(minvar)
       && diff < pQuest->Get(minvar))
      return kSkipGoal; 


      // check against max difficulty 
   cStr maxvar = GoalMaxDiffVarName(i); 
   if (pQuest->Exists(maxvar)
       && diff > pQuest->Get(maxvar))
      return kSkipGoal; 

   return kDrawGoal; 
}

void cObjectivesBase::RedrawDisplay()
{
   // push a sub-canvas
   Rect& area = mRects[num_butts]; 
   GUIcompose c; 
   int compose_flags = ComposeFlagRead|ComposeFlagClear; 
   GUIsetup(&c,&area,(GUIcomposeFlags)compose_flags,GUI_CANV_ANY); 
   int y = 0; 
   int max = RectHeight(&area); 
   int width = RectWidth(&area); 

   //
   // Set up font 
   //

   guiStyleSetupFont(&mTextStyle,StyleFontNormal); 
   IRes* altfont = NULL; 


   // If the display doesn't fit, use the smaller font 

   BOOL show_stats = mFlags & kShowStats; 

   if (!show_stats && !DisplayFits(area,gr_get_font()))
   {

      static const char* alt_fonts[] = { "textfont", "smalfont" }; 
      int nfonts = (sizeof(alt_fonts)/sizeof(alt_fonts[0]));

      guiStyleCleanupFont(&mTextStyle,StyleFontNormal);

      int i;
      for (i = 0; i < nfonts; i++)
      {
         char var[64]; 
         sprintf(var,"%s_alt_font_%d",panel_name,i); 
         const char* fontname = alt_fonts[i]; 

         char buf[64];
         if (config_get_raw(var,buf,sizeof(buf)))
            fontname = buf; 
         // Grab the tiny font and go

         AutoAppIPtr(ResMan);

         SafeRelease(altfont); 
         altfont = pResMan->Bind(fontname,RESTYPE_FONT,NULL,mResPath); 

         grs_font* font = (grs_font*)altfont->Lock(); 

         if (DisplayFits(area,font))
         {
            gr_set_font(font);
            break; 
         }
         altfont->Unlock(); 

      }
   
      if (i >= nfonts)
      {
         Warning(("No objectives font could fit, using smallest\n")); 
         gr_set_font((grs_font*)altfont->Lock()); 
      }
   }

   int yspace = compute_yspace(gr_get_font()); 




   //
   // Draw title
   //

   {
      char buf[16]; 
      sprintf(buf,"title_%d",GetMissionData()->num); 
      cStr title_str = FetchUIString("titles",buf,"strings"); 
      char* title = (char*)(const char*)title_str; 
      int x = (width - gr_string_width(title))/2; 
      gr_set_fcolor(guiStyleGetColor(&mTextStyle,StyleColorText)); 

      gr_string(title,x,y); 
      y += gr_string_height(title) + yspace; 
   }

   
   AutoAppIPtr_(QuestData,pQuest); 
   int diff = pQuest->Get(DIFF_QVAR); 

   //
   // Draw difficulty
   //

   {
      char buf[16];
      cStr diff_str = FetchUIString("newgame","difficulty") ; 
      sprintf(buf,"diff_%d",diff); 
      diff_str += " "; 
      diff_str += FetchUIString("newgame",buf); 
      char* diffstr = (char*)(const char*)diff_str; 

      gr_set_fcolor(guiStyleGetColor(&mTextStyle,StyleColorText)); 

      gr_string(diffstr,0,y); 
      y += gr_string_height(diffstr); 
   }


   // 
   // Draw goals 
   //
   if (!show_stats)
   {
      int between = 0; 

      BOOL done = FALSE; 
      for (int i = 0; !done; i++)
      {
         int draw = should_draw_goal(pQuest,i); 
         if (draw == kNoGoal)
            break; 
      
         if (draw == kSkipGoal)
            continue; 

         y += between ; // first time through this is zero
         between = yspace; 

         cStr text = (mFlags & kFictionGoals) ? GoalFiction(i) : GoalDescription(i); 

         // Perilous, we mutate a cstr in place
         char* s = (char*)(const char*)text; 
      
         int x = kLeftGoalTextMargin;
         gr_font_string_wrap(gr_get_font(), s, width - x); 

         int h = gr_string_height(s); 

         // We already did this in should_draw_goal.  Oh well. 
         int state = pQuest->Get(GoalStateVarName(i)); 


	 if (state != 2)
	   {
	     // setup font, move "cursor" to just after icon
	     gr_set_fcolor(guiStyleGetColor(&mTextStyle,StyleColorText)); 
	   }
	 else
	   {
	     gr_set_fcolor(guiStyleGetColor(&mGreyStyle,StyleColorText));
	   }

         gr_string(s,x,y); 


         // Now draw the icon 
         IImageSource* icon = GoalStatus(state); 
         grs_bitmap* bm = (grs_bitmap*)icon->Lock(); 
         int icon_h = bm->h + kTextIconAlign; 
         gr_bitmap(bm,0,y+(gr_string_height("X") - icon_h)/2); 

         icon->Unlock(); 
         SafeRelease(icon); 

         y += h; 


      }
   }

   //
   // Draw stats
   //
   if (show_stats)
   {
      int x = 0, draw_line=TRUE; 
      // y += kStatsVertSeparator;
      for (int i = 0, done = FALSE; !done; i++)
      {
         int val=0;
         char buf[256];
         char outbuf[256]; 
         
         sprintf(buf,"stat_%d",i); 
         cStr var = FetchUIString(panel_name,buf); 
      
         if (var[0] == '\0')
            break; 

         // if (!pQuest->Exists(var)) continue;

         if (pQuest->Exists(var))
            val = pQuest->Get(var); 

         sprintf(buf,"text_%d",i); 
         cStr text = FetchUIString(panel_name,buf); 

         sprintf(buf,"format_%d",i); 
         cStr format = FetchUIString(panel_name,buf); 

         // this is the fancy formatter from hell...
         if (format == "" || format == "*missing string*")
            format = "%d"; 

         BOOL have_cr=TRUE, no_sprint=FALSE, skip=FALSE, maybe_abort=FALSE, abort=FALSE;
         char *use_str=(char *)(const char *)format, sep[8]={0};
         int miss_val=0;
         
         while (*use_str=='@')
         {  // do we go to next line?? - strings ending in | do not
            // do magic @ sign stuff
            switch (use_str[1])
            {
               case 'x':
                  maybe_abort=TRUE;
                  break;
               case 'n':
                  y+=gr_string_height("X");
                  x=0;
                  break;
               case '<':
                  miss_val=((use_str[2]-'0')*10)+(use_str[3]-'0');
                  use_str+=2;
                  if (GetMissionData()->num<miss_val)
                     if (maybe_abort) abort=TRUE;
                     else skip=TRUE;
                  break;
               case '>':
                  miss_val=((use_str[2]-'0')*10)+(use_str[3]-'0');
                  use_str+=2;
                  if (GetMissionData()->num>miss_val)
                     if (maybe_abort) abort=TRUE;
                     else skip=TRUE;
                  break;
               case '=':
                  miss_val=((use_str[2]-'0')*10)+(use_str[3]-'0');
                  use_str+=2;
                  if (GetMissionData()->num==miss_val)
                     if (maybe_abort) abort=TRUE;
                     else skip=TRUE;
                  break;
               case '!':
                  miss_val=((use_str[2]-'0')*10)+(use_str[3]-'0');
                  use_str+=2;
                  if (GetMissionData()->num!=miss_val)
                     if (maybe_abort) abort=TRUE;
                     else skip=TRUE;
                  break;
               case 'c': have_cr=FALSE; break;
               case 'b':
                  sprintf(buf,"%s",(const char *)FetchUIString(panel_name,val?"true":"false"));
                  no_sprint=TRUE;
                  break;
               case 'q':
                  sprintf(buf,"%s",(const char *)FetchUIString(panel_name,val?"some":"none"));
                  no_sprint=TRUE;
                  break;
               case 't':
                  {
                     int s=val/1000;
                     int m=s/60;
                     int h=m/60;
                     buf[0]='\0';
                     if (h)
                        sprintf(buf+strlen(buf),"%d %s ",h,(const char *)FetchUIString(panel_name,h!=1?"hours":"hour"));
                     if (m)
                        sprintf(buf+strlen(buf),"%d %s ",m%60,(const char *)FetchUIString(panel_name,m!=1?"minutes":"minute"));
                     if (s)
                        sprintf(buf+strlen(buf),"%d %s",s%60,(const char *)FetchUIString(panel_name,s!=1?"seconds":"second"));
                     no_sprint=TRUE;
                  }
                  break;
               case '+': skip=(val==0); break;
               case 's': draw_line=(val!=0); have_cr=FALSE; break;
               case ':': strcpy(sep,": "); break;
               case '/': strcpy(sep,"/"); break;
            }
            use_str+=2;
         }

         if (abort)
            break;
         if (!draw_line)
         {
            if (have_cr)        // if we arent drawing this line
               draw_line=TRUE;  //    but have a carriage return
            continue;           //  then we should draw the next
         }
         if (skip)
         {
            if (have_cr&&(x>0))
            {
               y+=gr_string_height("X");
               x =0;
            }
            if (y >= max) break;
            continue;
         }

         if (!no_sprint)
            sprintf(buf,use_str,val);
         sprintf(outbuf,"%s%s%s",(const char*)text,sep,buf);

         gr_font_string_wrap(gr_get_font(), outbuf, width - x); 
         gr_set_fcolor(guiStyleGetColor(&mTextStyle,StyleColorText)); 
         gr_string(outbuf,x,y);

         if (have_cr)
         {
            y += gr_string_height(outbuf);
            x  = 0;
            draw_line = TRUE;
         }
         else
            x += gr_string_width(outbuf);

         if (y >= max) break;
      }

   }


   if (!altfont)
      guiStyleCleanupFont(&mTextStyle,StyleFontNormal);
   else
   {
      altfont->Unlock();
      altfont->Release(); 
   }
   
   GUIdone(&c); 
}

//
// Figure out if the display is going to fit 
// @TODO: Factor this code! 
//

BOOL cObjectivesBase::DisplayFits(const Rect& area, grs_font* font)
{
   int y = 0;
   int max = RectHeight(&area); 
   int width = RectWidth(&area); 

   //
   // Set up font 
   //

   int yspace = compute_yspace(font); 

   // Title and difficulty space
   // @HACK: These had better be 1 line each 
   y += gr_font_string_height(font,"X")*2 + yspace; 

   AutoAppIPtr_(QuestData,pQuest); 

   // 
   // Goals
   //
   int between = 0; 

   BOOL done = FALSE; 
   for (int i = 0; !done; i++)
   {
      int draw = should_draw_goal(pQuest,i); 
      if (draw == kNoGoal)
         break; 
      
      if (draw == kSkipGoal)
         continue; 

      y += between; 
      between = yspace; 

      // setup font, move "cursor" to just after icon
      int x = kLeftGoalTextMargin;

      cStr text = (mFlags & kFictionGoals) ? GoalFiction(i) : GoalDescription(i); 

      // Perilous, we mutate a cstr in place
      char* s = (char*)(const char*)text; 
      
      gr_font_string_wrap(font, s, width - x); 
      y += gr_font_string_height(font,s);

      if (y >= max) return FALSE; 
   }

   return TRUE; 
}



//------------------------------------------------------------
// CLASS: cDebrief
//

class cDebrief : public cObjectivesBase
{
   static sDarkPanelDesc vars;

public:

   cDebrief()
      : cObjectivesBase(&vars,0)
   {
      
   }

   enum eRects
   {
      kMenu,
      kContinue, 
      kStats, 
      kNumButts, 
      kText = kNumButts,

      kNumRects
   }; 


protected:
   void OnButtonList(ushort action, int button); 

   void InitUI()
   {
      cObjectivesBase::InitUI(); 
      AutoAppIPtr(QuestData); 
      if (!pQuestData->Get(MISSION_COMPLETE_VAR))
      {
         int i = kContinue; 
         mStrings[i] = FetchUIString(panel_name,"restart",mResPath);
         mElems[i].draw_data = (void*)(const char*)mStrings[i]; 
      }

      mFlags &= ~kShowStats; 
   }

   void OnEscapeKey()
   {
      // push to sim menu 
      SwitchToSimMenuMode(TRUE); 
   }

};




static const char* debrief_button_names[] = 
{
   "menu",
   "continue",
   "stats",
};

sDarkPanelDesc cDebrief::vars = 
{
   "debrief", 
   cDebrief::kNumButts, 
   cDebrief::kNumRects, 
   cDebrief::kNumButts, 
   debrief_button_names,
}; 

void cDebrief::OnButtonList(ushort action, int button)
{
   if (action & BUTTONGADG_LCLICK)
   {
      switch (button)
      {
         case kContinue:
         {
            IPanelMode* pMode = GetPanelMode(); 
            pMode->Exit(); 
            SafeRelease(pMode); 
            break; 
         }
         case kMenu:
            SwitchToSimMenuMode(TRUE); 
            break; 

         case kStats:
         {
            int i = kStats; 
            // switch the button text
            // toggle stat mode
            mFlags ^= kShowStats;
            const char* button_name = (mFlags & kShowStats) ? "goals" : "stats"; 
            mStrings[i] = FetchUIString(panel_name,button_name,mResPath);
            mElems[i].draw_data = (void*)(const char*)mStrings[i]; 

            RedrawDisplay(); 
         }
         break; 

      }

   }
}





static cDebrief* gpDebrief = NULL; 

void SwitchToDebriefMode(BOOL push) 
{
   if (!gpDebrief)
      gpDebrief = new cDebrief; 

   IPanelMode* panel = gpDebrief->GetPanelMode(); 

   panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
   SafeRelease(panel); 
}

const sLoopInstantiator* DescribeDebriefMode(void)
{
   if (!gpDebrief)
      gpDebrief = new cDebrief; 

   if (gpDebrief)
   {
      cAutoIPtr<IPanelMode> panel = gpDebrief->GetPanelMode(); 

      return panel->Instantiator(); 
   }
   return NULL; 
}


//------------------------------------------------------------
// SUBCLASS: cObjectives
//

class cObjectives : public cObjectivesBase
{
   static sDarkPanelDesc vars;

public:

   cObjectives()
      : cObjectivesBase(&vars,0)
   {
      
   }

   enum eRects
   {
      kMap,
      kQuit,
      kNumButts, 
      kText = kNumButts,

      kNumRects
   }; 


protected:
   void OnButtonList(ushort action, int button);


};




static const char* objectives_button_names[] = 
{
   "map",
   "done",
   "text", 
};

sDarkPanelDesc cObjectives::vars = 
{
   "objctiv", 
   cObjectives::kNumButts, 
   cObjectives::kNumRects, 
   cObjectives::kNumButts, 
   objectives_button_names,
}; 

void cObjectives::OnButtonList(ushort action, int button)
{
   if (action & BUTTONGADG_LCLICK)
   {
      switch (button)
      {
         case kMap:
            SwitchToDarkAutomapMode(FALSE); 
            break; 
         case kQuit:
            mpPanelMode->Exit(); 
            break; 
      }

   }
}


static cObjectives* gpObjectives = NULL; 

void SwitchToObjectivesMode(BOOL push) 
{
   if (!gpObjectives)
      gpObjectives = new cObjectives; 
   IPanelMode* panel = gpObjectives->GetPanelMode(); 

   panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
   SafeRelease(panel); 
}


const sLoopInstantiator* DescribeObjectivesMode(void)
{
   if (!gpObjectives)
      gpObjectives = new cObjectives; 

   cAutoIPtr<IPanelMode> panel = gpObjectives->GetPanelMode(); 

   return panel->Instantiator(); 
}

//------------------------------------------------------------
// SUBCLASS: cLoading
//

class cLoading;
static cLoading* gpLoading = NULL; 

class cLoading : public cObjectivesBase
{
   static sDarkPanelDesc vars;

public:

   cLoading()
      : cObjectivesBase(&vars,kFictionGoals), 
        mpMissionFile(NULL),
        mState(kWaitForLoad),
        mFileTotal(1),
        mFileCur(0),
        mDialRate(kFineDialRate),
        mStateLocked(FALSE)
   {
      for (int i = 0; i < kNumDials; ++i)
         for (int j = 0; j < kNumDialImages; ++j)
         {
            mpDialImages[i][j] = NULL;
            mDialPaletteSlots[i][j] = -1;
         }
   }

   enum eRects
   {
      kDifficulty,
      kGoOn, 
      kNumButts,  
      kText = kNumButts, 
      kCoarseDial,
      kFineDial,

      kNumRects,
      
      kFirstDial = kCoarseDial,
   }; 

   enum
   {
      kNumDiffLevels = 3,
      kNumDialImages = 20, 
      kNumDials = 2, 
      kFineDialRate = 16384,  // bytes per frame 
      kFineDialFrames = 18,
   };

   enum eState
   {
      kWaitForLoad,
      kLoad,
      //      kWaitForContinue,
      kContinue,
      kWaitForQuit, 
      kQuit,
   }; 


   // Add a file's size to our totals
   void AddFile(ITagFile* file)
   {
      if (!file)
         return;
      cAutoIPtr<ITagFileIter> iter = file->Iterate(); 
      for (iter->Start(); !iter->Done(); iter->Next())
      {
         const ulong size = file->BlockSize(iter->Tag());
         uint64_t total = 0;
         if (size == kTagNoSuchBlock ||
             !DarkCheckedAddU64(mFileTotal,size,&total))
         {
            MissionDiagnosticsIssue("ERROR","mission-data",
               "Mission progress size overflow; using a saturated progress total");
            mFileTotal = UINT64_MAX;
            break;
         }
         mFileTotal = total;
      }
   }

   void SetFile(ITagFile* file)
   {
      SafeRelease(mpMissionFile); 

      mpMissionFile = file; 
      if (file)
         file->AddRef();
   }

   

   void NextState(void)
   {
      mStateLocked=TRUE;
      SetState(mState+1); 
   }

   void SetState(int state)
   {
      mState = state; 

      static const char* button_names[] = { "continue", "loading", "loading", "continue", "continue" }; 
      // change the button 
      int i = kGoOn; 
      mStrings[i] = FetchUIString(panel_name,button_names[mState],mResPath);
      mElems[i].draw_data = (void*)(const char*)mStrings[i]; 
      region_expose(LGadBoxRegion(LGadCurrentRoot()),&mRects[i]);      
   }

   void UpdateDials(int bytes)
   {
      // Read/Move report requested byte counts.  NewDark WREXT missions can
      // exceed 100 MiB, so the old signed 32-bit `current * 18` expression
      // overflowed and produced a negative frame index.  The loading panel
      // then indexed before mpDialImages and called through a null pointer.
      if (bytes <= 0)
         return;
      uint64_t old = mFileCur;
      uint64_t updated = 0;
      if (DarkCheckedAddU64(mFileCur,(unsigned int)bytes,&updated))
         mFileCur = updated;
      else
         mFileCur = UINT64_MAX;

      //ConfigSpew("dial_spew",("bytes = %d delta = %d\n",mFileCur,bytes)); 

      // update fine dial 
      {
         int frame = (int)((mFileCur/(unsigned int)mDialRate) %
                           kNumDialImages);
         int oldframe = (int)((old/(unsigned int)mDialRate) %
                              kNumDialImages);
         if (frame != oldframe)
         {
            DrawDialImage(kFineDial,frame); 

         }
      }

      // update coarse dial 
      {
         const uint64_t total = mFileTotal ? mFileTotal : 1;
         const uint64_t current = mFileCur < total ? mFileCur : total;
         const uint64_t previous = old < total ? old : total;
         uint64_t product = 0;
         uint64_t oldProduct = 0;
         int frame = DarkCheckedMulU64(current,kFineDialFrames,&product) ?
            (int)(product/total) :
            (int)((long double)current*kFineDialFrames/(long double)total);
         int oldframe = DarkCheckedMulU64(previous,kFineDialFrames,&oldProduct) ?
            (int)(oldProduct/total) :
            (int)((long double)previous*kFineDialFrames/(long double)total);
         if (frame != oldframe)
         {
            DrawDialImage(kCoarseDial,frame); 
         }
      }

      // pump events
      pGameShell->PumpEvents(kPumpAll);             
   }


   //
   // File proxy that snoops reads and updates dials
   //

   class cLoadingFile: public cTagFileProxy
   {
      cLoading* mpUs;
   public: 
      cLoadingFile(ITagFile* file, cLoading* us) 
         : cTagFileProxy(file), mpUs(us)
      {
         mpUs->AddFile(file); 
      } 

      STDMETHOD_(long,Read)(char* buf, int buflen)
      {
         long result = cTagFileProxy::Read(buf,buflen);
         mpUs->UpdateDials(result);
         return result;
      }

      STDMETHOD_(long,Move)(char* buf, int buflen)
      {
         long result = cTagFileProxy::Move(buf,buflen);
         mpUs->UpdateDials(result);
         return result;
      }


   }; 


   //  
   // DB file factory that generates cLoadingFile objects wrapped 
   // around regular disk tag files.
   //

   class cLoadingFileFactory : public cDBFileFactory
   {
      cLoading* mpUs; 
   public: 
      cLoadingFileFactory(cLoading* us)
         : mpUs(us)
      {
      }; 

      ITagFile* Open(const char* filename, TagFileOpenMode mode)
      {
         char buf[256]; 
         ConfigSpew("loading_open_spew",("Opening %s\n",filename)); 
         // The factory is required to find the file
         if (!dbFind(filename,buf))
         {
            char diagnostic[384];
            _snprintf(diagnostic,sizeof(diagnostic)-1,
                      "Referenced mission file was not found: %s",
                      filename ? filename : "(null)");
            diagnostic[sizeof(diagnostic)-1] = '\0';
            dbLoadReportFailure("mission-data",diagnostic);
            return NULL;
         }
         cAutoIPtr<ITagFile> file = BufTagFileOpen(buf,mode);
         if (!file)
         {
            char diagnostic[384];
            _snprintf(diagnostic,sizeof(diagnostic)-1,
                      "Referenced mission file could not be opened: %s",buf);
            diagnostic[sizeof(diagnostic)-1] = '\0';
            dbLoadReportFailure("mission-data",diagnostic);
            return NULL;
         }
         return new cLoadingFile(file,mpUs); 
      }
   }; 


protected:
   ITagFile* mpMissionFile; 
   IDataSource* mpDialImages[kNumDials][kNumDialImages]; 
   int mDialPaletteSlots[kNumDials][kNumDialImages];

   int mState; 
   uint64_t mFileTotal;
   uint64_t mFileCur;
   int mDialRate;
   BOOL mStateLocked; //sigh.  If TRUE, kGoOn cannot use NextState. 
                      //set false on every frame.  Prevents kGoOn button
                      //from changing state while frame locked.

   void InitUI()
   {
      cObjectivesBase::InitUI(); 
      AutoAppIPtr(ResMan);
      int missingDialImages = 0;

       for (int i = 0; i < kNumDials; i++)
         for(int j = 0; j < kNumDialImages; j++)
         {
            char buf[16];
            sprintf(buf,"load%c_%d",'A'+i,j+1);
            ConfigSpew("dial_spew",("Binding %s\n",buf)); 
            mpDialImages[i][j] = FetchUIImage(buf,mResPath+"\\Dials"); 
            mDialPaletteSlots[i][j] = -1;
            if (!mpDialImages[i][j])
               ++missingDialImages;

            // The background and the dial frames need not share a palette.
            // In particular, NewDark FMs can replace LOADING.PCX with a
            // true-colour PNG while retaining the stock indexed LoadA/LoadB
            // frames.  Drawing those frames through the panel's synthetic
            // PNG palette produces the characteristic neon/rainbow dials.
            // Give each indexed frame its authored palette instead.
            IRes* imageRes = NULL;
            if (mpDialImages[i][j] &&
                SUCCEEDED(mpDialImages[i][j]->QueryInterface(
                   IID_IRes,(void**)&imageRes)))
            {
               grs_bitmap* bm = (grs_bitmap*)imageRes->Lock();
               if (bm && bm->type == BMT_FLAT8)
               {
                  IRes* palRes = pResMan->Retype(imageRes,
                                                 RESTYPE_PALETTE,0);
                  if (palRes)
                  {
                     uchar* pal = (uchar*)palRes->Lock();
                     if (pal)
                     {
                        int slot = palmgr_alloc_pal(pal);
                        bm->align = (uchar)slot;
                        mDialPaletteSlots[i][j] = slot;
                        palRes->Unlock();
                     }
                     palRes->Release();
                  }
               }
               if (bm)
                  imageRes->Unlock();
               imageRes->Release();
            }
         }
       if (missingDialImages)
       {
          char diagnostic[192];
          _snprintf(diagnostic,sizeof(diagnostic)-1,
                    "Loading progress artwork is missing %d of %d dial "
                    "frames; unavailable frames will retain the background",
                    missingDialImages,kNumDials*kNumDialImages);
          diagnostic[sizeof(diagnostic)-1]='\0';
          MissionDiagnosticsIssue("ERROR","resource",diagnostic);
       }
       if (mFileTotal == 0)
          mFileTotal = 1;  // make sure this is not zero 

       mDialRate = kFineDialRate; 
       config_get_int("progress_dial_rate",&mDialRate); 
      if (mDialRate <= 0)
      {
         MissionDiagnosticsIssue("ERROR","configuration",
            "progress_dial_rate must be positive; using 16384");
         mDialRate = kFineDialRate;
      }
   }

   void TermUI()
   {
       for (int i = 0; i < 2; i++)
         for(int j = 0; j < kNumDialImages; j++)
         {
            SafeRelease(mpDialImages[i][j]); 
            if (mDialPaletteSlots[i][j] > 0)
               palmgr_release_slot(mDialPaletteSlots[i][j]);
            mDialPaletteSlots[i][j] = -1;
         }

       cObjectivesBase::TermUI(); 
   }

   void DrawDialImage(int dial, int image)
   {
      GUIcompose c; 

      if (image < 0)
         image = 0;
      if (image >= kNumDialImages)
         image = kNumDialImages - 1; 
      Rect& area = mRects[dial]; 
      IDataSource* img = mpDialImages[dial-kFirstDial][image]; 

      // Missing UI art is an error state, but it must not terminate mission
      // loading. InitUI reports the incomplete set once; leave the already
      // drawn loading background intact for an unavailable frame.
      if (!img)
         return;

      //      ConfigSpew("dial_spew",("[%d %d] ",dial-kFirstDial,image)); 


      int compose_flags = ComposeFlagRead; 
      GUIsetup(&c,&area,(GUIcomposeFlags)compose_flags,GUI_CANV_ANY); 

      grs_bitmap* bm = (grs_bitmap*)img->Lock();
      gr_bitmap(bm,0,0); 
      img->Unlock(); 

      GUIdone(&c); 

      ScrnForceUpdateRect(&area); 
   }

   void OnButtonList(ushort action, int button)
   {
      if (action & BUTTONGADG_LCLICK)
      {
         switch (button)
         {
            case kDifficulty:
               if (mState < kLoad)
               {
                  AutoAppIPtr(QuestData); 
                  int diff = pQuestData->Get(DIFF_QVAR); 
                  diff++; 
                  if (diff >= kNumDiffLevels)
                     diff = 0; 
                  pQuestData->Set(DIFF_QVAR,diff); 
                  RedrawDisplay(); 
               }
            break;    
            
            case kGoOn:
            {
               if (!mStateLocked)
               {
                  NextState(); 
               }
            }
            break; 

         }
      }
   }

   
   static long move_func(void* buf, size_t sz, size_t n)
   {
   return gpLoading->mpMissionFile->Read((char*)buf,(int)(sz*n));
   }

   void ReadMission(void)
   {
      if (!mpMissionFile)
         return;
      // load the objectives 
      AutoAppIPtr(QuestData); 
      pQuestData->DeleteAllType(kQuestDataMission); 

      QuestDataLoadTagFile(kQuestDataMission,mpMissionFile); 

      // load mission data
      LoadMissionData(mpMissionFile); 
      LoadMapSourceInfo(mpMissionFile);

   }
   
   virtual void DoFileLoad(ITagFile* file)
   {
      const edbFiletype loaded = dbLoadTagFile(file,kFiletypeAll);
      if (config_is_defined("mission_loop_test_expect_failure"))
      {
         MissionDiagnosticsIssue(loaded == 0 ? "INFO" : "ERROR",
            "mission-regression",loaded == 0 ?
            "PASS: untrusted mission data was rejected without entering gameplay" :
            "FAIL: malformed mission data unexpectedly reached post-load");
         SetState(kQuit);
         quit_game();
         return;
      }
      if (loaded != 0)
         NextState();
      else
      {
         MissionDiagnosticsIssue("ERROR","mission-data",
            dbLoadFailureReason()[0] ? dbLoadFailureReason() :
            "Mission load failed; returning to the main menu");
         SetState(kQuit);
      }

   }

   virtual void SetInitialFile(void)
   {
      ITagFile* file = NULL;
      char regressionFile[1024];
      if (config_get_raw("mission_loop_test_file",regressionFile,
                         sizeof(regressionFile)))
      {
         regressionFile[sizeof(regressionFile)-1] = '\0';
         file = BufTagFileOpen(regressionFile,kTagOpenRead);
      }
      else
         file = OpenMissionFile(GetNextMission());
      SetFile(file); 
      SafeRelease(file); 
      if (!mpMissionFile)
      {
         MissionDiagnosticsIssue("ERROR","mission-data",
            "Selected mission could not be opened; returning to the main menu");
         if (config_is_defined("mission_loop_test_expect_failure"))
         {
            MissionDiagnosticsIssue("INFO","mission-regression",
               "PASS: malformed tag-file container was rejected before decoding");
            quit_game();
         }
         SetState(kQuit);
      }
   }

   void OnLoopMsg(eLoopMessage msg, tLoopMessageData data)
   {
      switch(msg)
      {
         case kMsgEnterMode:
         {
            mState = kWaitForLoad;
            SetInitialFile(); 

            if (mpMissionFile)
               ReadMission();
         }
         break; 

         case kMsgNormalFrame:
         case kMsgPauseFrame:
         {
            mStateLocked = FALSE;
            // Deterministic regression path for the real mission-loop loader.
            // Unlike quickstart this retains database progress wrapping,
            // mission preparation, and the transition into gameplay.
            if (config_is_defined("mission_loop_test") &&
                mState == kWaitForLoad)
               NextState();
            switch (mState)
            {
               case kLoad:
               {
                  uiHideMouse(NULL); 
                  ScrnForceUpdate(); 
                  // Load the mission, then go to next state
                  mFileCur = 1; // reset dial state 
                  mFileTotal = 1; // no dividing by zero 

                  // Set up the file factory 
                  cLoadingFileFactory fact(this); 
                  dbSetFileFactory(&fact); 

                  // load the file 
                  cLoadingFile* file = mpMissionFile ?
                     new cLoadingFile(mpMissionFile,this) : NULL;
                  if (file)
                  {
                     DoFileLoad(file);
                     if (mState != kQuit)
                        DrawDialImage(kCoarseDial,kNumDialImages-1);
                  }
                  else
                     SetState(kQuit);

                  uiShowMouse(NULL); 
                  dbSetFileFactory(NULL); 
                  SafeRelease(file); 
               }
               break; 

               case kQuit: 
                  MissionLoopReset(kMissLoopMainMenu); 
                  // fall through
               case kContinue:
                  UnwindToMissionLoop(); 
                  break; 
            }

         }
         break; 

         case kMsgExitMode:
         {
            SafeRelease(mpMissionFile); 
         }
         break;
      }
      cObjectivesBase::OnLoopMsg(msg,data); 
   }

   void OnEscapeKey()
   {
      // push to sim menu 
      SwitchToSimMenuMode(TRUE); 
   }


   
};




static const char* loading_button_names[] = 
{
   "difficulty",
   "continue", 
};

sDarkPanelDesc cLoading::vars = 
{
   "loading", 
   cLoading::kNumButts, 
   cLoading::kNumRects, 
   cLoading::kNumButts, 
   loading_button_names,
}; 


void SwitchToLoadingMode(BOOL push) 
{
   if (!gpLoading)
      gpLoading = new cLoading; 
   if (gpLoading)
   {
      IPanelMode* panel = gpLoading->GetPanelMode(); 

      panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
      SafeRelease(panel); 
   }
}




const sLoopInstantiator* DescribeLoadingMode(void)
{
   if (!gpLoading)
      gpLoading = new cLoading; 
   if (gpLoading)
   {
      cAutoIPtr<IPanelMode> panel = gpLoading->GetPanelMode(); 

      return panel->Instantiator(); 
   }
   return NULL; 
}


class cLoadingSaveGame: public cLoading
{

   void DoFileLoad(ITagFile* file)
   {
      HRESULT retval = DarkLoadGame(file); 
      if (FAILED(retval))  
      {
         // Replace the objectives with the "load failed" string

         // Set up the canvas & font 
         Rect& area = mRects[num_butts]; 
         GUIcompose c; 
         int compose_flags = ComposeFlagRead|ComposeFlagClear; 
         GUIsetup(&c,&area,(GUIcomposeFlags)compose_flags,GUI_CANV_ANY); 
         guiStyleSetupFont(&mTextStyle,StyleFontNormal); 

         // Get the string 
         cStr failed_str = FetchUIString("gamelod","failed",mResPath); 
         char* failed = (char*)(const char*)failed_str; 


         // Draw it 
         short w,h; 
         gr_string_size(failed,&w,&h);
         gr_set_fcolor(guiStyleGetColor(&mTextStyle,StyleColorText)); 
         gr_string(failed,(RectWidth(&area) - w)/2,(RectHeight(&area)-h)/2);

         guiStyleCleanupFont(&mTextStyle,StyleFontNormal); 
         GUIdone(&c); 
         
         // wait for acknowledgement 
         SetState(kWaitForQuit);          
      }
      else
         NextState(); 
   }

   void SetInitialFile()
   {
      // we already have a file, we promise.  
      Assert_(mpMissionFile); 

      // Load the campaign vars out of it. 
      // All we really need is difficulty.  Oh well. 
      AutoAppIPtr(QuestData); 
      pQuestData->DeleteAllType(kQuestDataCampaign); 

      QuestDataLoadTagFile(kQuestDataCampaign,mpMissionFile); 

   }

   
   void OnLoopMsg(eLoopMessage msg, tLoopMessageData data)
   {
      cLoading::OnLoopMsg(msg,data); 
      switch(msg)
      {
         case kMsgNormalFrame:
         case kMsgPauseFrame:
            if (mState == kWaitForLoad)
               NextState(); // present the panel before the synchronous load
            break;
      }
   }

   void InitUI()
   {
      cLoading::InitUI();
      // erase diff button
      mElems[(int)kDifficulty].draw_data = (void*)""; 
      // Do all goals have fiction strings?
      mFlags &= ~kFictionGoals; 
   }
};


static cLoadingSaveGame* gpLoadingSaveGame = NULL; 

void PushToSaveGameLoadingMode(ITagFile* file)
{
   if (!gpLoadingSaveGame)
      gpLoadingSaveGame = new cLoadingSaveGame; 

   gpLoadingSaveGame->SetFile(file); 
   IPanelMode* panel = gpLoadingSaveGame->GetPanelMode(); 
   panel->Switch(kLoopModePush);
   SafeRelease(panel); 
}



//------------------------------------------------------------------
// INIT/TERM
//


static void init_commands();

void DebriefInit()
{
   init_commands(); 
}

void DebriefTerm()
{
   delete gpObjectives;
   delete gpDebrief;
   delete gpLoading; 
   delete gpLoadingSaveGame;
}


//----------------------------------------
// COMMANDS
//

static void do_debrief()
{
   SwitchToDebriefMode(TRUE); 
}

static void do_objectives()
{
   SwitchToObjectivesMode(TRUE); 
}


static Command commands[] = 
{
   { "debrief", FUNC_VOID, do_debrief, "Go to debrief UI.", HK_ALL }, 
   { "objectives", FUNC_VOID, do_objectives, "Go to objectives UI.", HK_ALL }, 
}; 


static void init_commands()
{
   COMMANDS(commands,HK_ALL); 
}



