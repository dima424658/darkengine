/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkmenu.cpp,v 1.26 2000/02/19 13:09:08 toml Exp $
#include <drkmenu.h>
#include <appagg.h>
#include <drkpanl.h>
#include <drkmislp.h>
#include <drksavui.h>
#include <drkamap.h>
#include <drkdebrf.h>

#include <resapi.h>
#include <imgrstyp.h>

#include <gcompose.h>
#include <questapi.h>
#include <drkdiff.h>
#include <config.h>
#include <uigame.h>

#include <panltool.h>
#include <appapi.h>
#include <mprintf.h>
#include <campaign.h>
#include <metasnd.h>
#include <gamemode.h>
#include <contentctx.h>
#include <dbfile.h>
#include <filepanl.h>
#include <platform_services.h>
#include <resapp.h>
#include <scrptprp.h>

#pragma pack(push, 8)
#include <algorithm>
#include <string>
#include <vector>
#pragma pack(pop)

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <command.h>
#include <uiamov.h>
#include <uianim.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


EXTERN void SwitchToNewGameMode(BOOL push) ;
EXTERN void SwitchToOptionsMode(BOOL push) ;

//starting difficulty
extern int g_diff;


////////////////////////////////////////////////////////////
// DARK METAGAME MENUS 
//

//
// Base Menu class
//
// Some day, I expect this will
// deal with levers and other festoonery
//

class cMenuBase: public cDarkPanel, public cUiAnim
{
public: 
   cMenuBase(const sDarkPanelDesc* desc)
      : cDarkPanel(desc), cUiAnim(desc->panel_name,INTERFACE_PATH)
   {
   }; 

   ~cMenuBase() {}; 

protected:
   void InitUI()
   {
      cDarkPanel::InitUI(); 
      cUiAnim::InitUI();
   }

   void TermUI()
   {
      cUiAnim::TermUI();
      cDarkPanel::TermUI(); 
   }

   void OnLoopMsg(eLoopMessage msg, tLoopMessageData data) 
   {
      cUiAnim::OnLoopMsg(msg,data);
      cDarkPanel::OnLoopMsg(msg,data);
   }
}; 

//------------------------------------------------------------
// STARTUP CONTENT / FAN-MISSION MENU
//
// Reuse the load-game panel and its sixteen selectable rows. Large FM
// collections are paged through rows at the top and bottom, so the menu is
// usable with the mouse without adding platform widgets or new art assets.
//

namespace
{

enum eContentChoice
{
   kContentConfigured,
   kContentVanilla,
   kContentFanMission,
};

struct sContentItem
{
   eContentChoice choice;
   std::string fmName;
   std::string name;
   std::string description;
};

static std::string TrimContentText(const std::string& value)
{
   size_t first = 0;
   size_t last = value.size();
   while (first < last && isspace((unsigned char)value[first]))
      ++first;
   while (last > first && isspace((unsigned char)value[last - 1]))
      --last;
   return value.substr(first, last - first);
}

static void MakeContentPath(char* output, int size, const char* root,
                            const char* leaf)
{
   if (!output || size <= 0)
      return;

   size_t used = 0;
   output[0] = '\0';
   const auto append = [output, size, &used](const char* text)
   {
      if (!text || used >= (size_t)size - 1)
         return;
      const size_t count = std::min(strlen(text),
                                    (size_t)size - used - 1);
      memcpy(output + used, text, count);
      used += count;
      output[used] = '\0';
   };

   const size_t rootLength = root ? strlen(root) : 0;
   append(root);
   if (rootLength && root[rootLength - 1] != '/' &&
       root[rootLength - 1] != '\\')
      append("/");
   append(leaf);
}

static BOOL ContentFileExists(const char* directory, const char* name)
{
   char path[DARK_CONTENT_MAX_PATH];
   char resolved[DARK_CONTENT_MAX_PATH];
   MakeContentPath(path, sizeof(path), directory, name);
   return PlatformResolvePath(path, resolved, sizeof(resolved));
}

static BOOL ContentHasFiles(const char* directory, const char* pattern)
{
   char path[DARK_CONTENT_MAX_PATH];
   sPlatformFileFind find = {};
   MakeContentPath(path, sizeof(path), directory, pattern);
   if (!PlatformFindFirst(path, &find))
      return FALSE;
   do
   {
      if (!(find.attributes & kPlatformFileDirectory))
      {
         PlatformFindClose(&find);
         return TRUE;
      }
   } while (PlatformFindNext(&find));
   PlatformFindClose(&find);
   return FALSE;
}

static BOOL LooksLikeFanMission(const char* directory)
{
   static const char* markers[] = {
      "fm.ini", "fmsel.inf", "dark.cfg", "fm.cfg", "startmis.sav"
   };
   for (int i = 0; i < sizeof(markers) / sizeof(markers[0]); ++i)
      if (ContentFileExists(directory, markers[i]))
         return TRUE;
   return ContentHasFiles(directory, "*.mis") ||
          ContentHasFiles(directory, "*.gam") ||
          ContentHasFiles(directory, "*.cow");
}

static void ReadFanMissionInfo(const char* directory, std::string& name,
                               std::string& description)
{
   char path[DARK_CONTENT_MAX_PATH];
   MakeContentPath(path, sizeof(path), directory, "fm.ini");
   FILE* file = fopen(path, "rt");
   if (!file)
      return;
   char line[2048];
   while (fgets(line, sizeof(line), file))
   {
      char* newline = strpbrk(line, "\r\n");
      char* equals;
      if (newline)
         *newline = '\0';
      equals = strchr(line, '=');
      if (!equals)
         continue;
      *equals = '\0';
      const std::string key = TrimContentText(line);
      const std::string value = TrimContentText(equals + 1);
      if (_stricmp(key.c_str(), "NiceName") == 0 && !value.empty())
         name = value;
      else if (_stricmp(key.c_str(), "Descr") == 0 && !value.empty())
         description = value;
   }
   fclose(file);
}

static void BuildContentCatalog(std::vector<sContentItem>& items)
{
   sContentItem vanilla;
   vanilla.choice = kContentVanilla;
   vanilla.name = "Original Missions";
   vanilla.description =
      "Use only the base retail resources; this is the most compatible original campaign mode.";
   items.push_back(vanilla);

   sContentItem configured;
   configured.choice = kContentConfigured;
   configured.name = "Original Missions - Modded";
   configured.description =
      "Use configured mod paths; installed mods may require unsupported NewDark features.";
   items.push_back(configured);

   char root[DARK_CONTENT_MAX_PATH];
   char configuredRoot[DARK_CONTENT_MAX_PATH] = "FMs";
   char pattern[DARK_CONTENT_MAX_PATH];
   config_get_raw("fm_path", configuredRoot, sizeof(configuredRoot));
   configuredRoot[sizeof(configuredRoot) - 1] = '\0';
   const BOOL absoluteRoot = configuredRoot[0] == '/' ||
      configuredRoot[0] == '\\' ||
      (isalpha((unsigned char)configuredRoot[0]) && configuredRoot[1] == ':');
   MakeContentPath(root, sizeof(root),
                   absoluteRoot ? "" : DarkContentBasePath(), configuredRoot);
   MakeContentPath(pattern, sizeof(pattern), root, "*");
   sPlatformFileFind find = {};
   if (!PlatformFindFirst(pattern, &find))
      return;
   do
   {
      if (!(find.attributes & kPlatformFileDirectory) ||
          find.name[0] == '.')
         continue;
      char directory[DARK_CONTENT_MAX_PATH];
      MakeContentPath(directory, sizeof(directory), root, find.name);
      if (!LooksLikeFanMission(directory))
         continue;
      sContentItem item;
      item.choice = kContentFanMission;
      item.fmName = find.name;
      item.name = find.name;
      item.description = "Launch this fan mission using the configured mod paths.";
      ReadFanMissionInfo(directory, item.name, item.description);
      items.push_back(item);
   } while (PlatformFindNext(&find));
   PlatformFindClose(&find);

   if (items.size() > 2)
      std::sort(items.begin() + 2, items.end(),
         [](const sContentItem& left, const sContentItem& right)
         {
            return _stricmp(left.name.c_str(), right.name.c_str()) < 0;
         });
}

}

class cContentMenu: public cFilePanel
{
public:
   cContentMenu() : cFilePanel(&gDesc), mPage(0)
   {
      BuildContentCatalog(mItems);
      for (int i = 0; i < kNumSlots; ++i)
         mVisible[i] = -1;
      MapPage();
   }

protected:
   enum { kPageItems = kNumSlots - 2, kPreviousPage = -2,
          kNextPage = -3 };
   static sDarkPanelDesc gDesc;
   std::vector<sContentItem> mItems;
   int mVisible[kNumSlots];
   int mPage;

   std::string FitSlotTitle(const std::string& title)
   {
      // The file-panel text gadget always word-wraps its buffer.  Content
      // names are labels, not paragraphs, so elide them before the gadget can
      // turn a long FM title into a second, clipped line.
      guiStyleSetupFont(&mSlotStyle,StyleFontNormal);
      const int availableWidth = RectWidth(&mRects[(int)kFiles]) - 4;
      if (gr_string_width(title.c_str()) <= availableWidth)
      {
         guiStyleCleanupFont(&mSlotStyle,StyleFontNormal);
         return title;
      }

      static const char ellipsis[] = "...";
      std::string fitted = title;
      while (!fitted.empty())
      {
         while (!fitted.empty() && isspace((unsigned char)fitted.back()))
            fitted.pop_back();
         if (gr_string_width((fitted + ellipsis).c_str()) <= availableWidth)
            break;
         fitted.pop_back();
      }
      fitted += ellipsis;
      guiStyleCleanupFont(&mSlotStyle,StyleFontNormal);
      return fitted;
   }

   std::string FitDescription(const std::string& description)
   {
      // FM metadata is often prose copied from a readme, with arbitrary
      // whitespace and far more text than the fixed save/load message panel
      // can display.  Keep the normal UI font and elide whole trailing words
      // instead of drawing wrapped lines above and below the panel.
      std::string text;
      bool pendingSpace = false;
      for (std::string::const_iterator it = description.begin();
           it != description.end(); ++it)
      {
         if (isspace((unsigned char)*it))
         {
            pendingSpace = !text.empty();
         }
         else
         {
            if (pendingSpace)
               text += ' ';
            text += *it;
            pendingSpace = false;
         }
      }

      if (text.empty())
         return text;

      Rect& rect = mRects[(int)kMessage];
      guiStyleSetupFont(&mMessageStyle,StyleFontNormal);

      const int availableHeight = RectHeight(&rect);
      const int availableWidth = RectWidth(&rect);
      const auto fits = [availableHeight,availableWidth](const std::string& value)
      {
         std::vector<char> wrapped(value.begin(),value.end());
         wrapped.push_back('\0');
         gr_font_string_wrap(gr_get_font(),wrapped.data(),availableWidth);
         short width,height;
         gr_string_size(wrapped.data(),&width,&height);
         return width <= availableWidth && height <= availableHeight;
      };

      std::string fitted = text;
      while (!fits(fitted))
      {
         const std::string::size_type suffix = fitted.size() >= 3 &&
            fitted.compare(fitted.size() - 3,3,"...") == 0 ? 3 : 0;
         std::string::size_type cut = fitted.rfind(' ',fitted.size() - suffix);
         if (cut == std::string::npos)
         {
            fitted = "...";
            break;
         }
         fitted.erase(cut);
         fitted += "...";
      }

      guiStyleCleanupFont(&mMessageStyle,StyleFontNormal);
      return fitted;
   }

   void MapPage()
   {
      for (int i = 0; i < kNumSlots; ++i)
         mVisible[i] = -1;
      const int first = mPage * kPageItems;
      if (mPage > 0)
         mVisible[0] = kPreviousPage;
      for (int i = 0; i < kPageItems && first + i < (int)mItems.size(); ++i)
         mVisible[i + 1] = first + i;
      if (first + kPageItems < (int)mItems.size())
         mVisible[kNumSlots - 1] = kNextPage;
   }

   void RefreshPage()
   {
      SelectSlot(-1);
      MapPage();
      for (int i = 0; i < kNumSlots; ++i)
      {
         ComputeSlot(i);
         RedrawSlot(i);
      }
      Message("Select original content or a fan mission, then choose Play.");
   }

   const char* SlotFileName(int) { return ""; }

   void ComputeSlot(int which)
   {
      sSlot& slot = mpSlots[which];
      slot.flags = 0;
      const int visible = mVisible[which];
      if (visible == kPreviousPage)
         strncpy(slot.buf, "< Previous page", sizeof(slot.buf));
      else if (visible == kNextPage)
         strncpy(slot.buf, "Next page >", sizeof(slot.buf));
      else if (visible >= 0 && visible < (int)mItems.size())
      {
         const sContentItem& item = mItems[visible];
         const std::string label = FitSlotTitle(
            std::string(item.choice == kContentFanMission ? "FM: " : "") +
            item.name);
         strncpy(slot.buf,label.c_str(),sizeof(slot.buf) - 1);
      }
      else
      {
         slot.buf[0] = '\0';
         slot.flags |= kUnused;
      }
      slot.buf[sizeof(slot.buf) - 1] = '\0';
      LGadUpdateTextBox(&slot.box);
   }

   void OnSelect(int which)
   {
      const int visible = mVisible[which];
      if (visible < 0 && visible != kPreviousPage && visible != kNextPage)
      {
         EnableFileOp(FALSE);
         return;
      }
      if (visible == kPreviousPage || visible == kNextPage)
         Message("Choose Play to change pages.");
      else
      {
         const std::string description =
            FitDescription(mItems[visible].description);
         Message(description.c_str());
      }
   }

   void DoFileOp(int which)
   {
      const int visible = mVisible[which];
      if (visible == kPreviousPage)
      {
         --mPage;
         RefreshPage();
         return;
      }
      if (visible == kNextPage)
      {
         ++mPage;
         RefreshPage();
         return;
      }
      if (visible < 0 || visible >= (int)mItems.size())
         return;

      const sContentItem& item = mItems[visible];
      const char* mode = item.choice == kContentVanilla ?
                         "vanilla" : "configured";
      const char* fmName = item.choice == kContentFanMission ?
                           item.fmName.c_str() : NULL;
      if (!DarkContentApplySelection(mode, fmName))
      {
         Message("Could not activate the selected content.");
         PlatformDiagnosticsLog("ERROR","content-bootstrap",
            "Could not apply the themed content selection in place");
         return;
      }

      // The content picker is a logical engine mode, not a launcher. Keep the
      // process, top-level window, renderer, and presentation surface alive;
      // only republish content-owned search paths before constructing the
      // first database and switching to the ordinary main menu.
      ResSysRefreshContentPath();
      RefreshScriptModulePath();
      dbBuildDefault();

      char diagnostic[512];
      _snprintf(diagnostic,sizeof(diagnostic) - 1,
                "Activated in place with content=%s%s%s",mode,
                fmName ? " fm=" : "",fmName ? fmName : "");
      diagnostic[sizeof(diagnostic) - 1] = '\0';
      PlatformDiagnosticsLog("INFO","content-bootstrap",diagnostic);
      SwitchToMainMenuMode(FALSE);
   }

   void TotalFailure()
   {
      AutoAppIPtr_(Application, pApp);
      pApp->Quit();
   }

   void InitUI()
   {
      cFilePanel::InitUI();
      mStrings[(int)kDoFile] = "Play";
      mStrings[(int)kDone] = "Exit";
      mElems[(int)kDoFile].draw_data =
         (void*)(const char*)mStrings[(int)kDoFile];
      mElems[(int)kDone].draw_data =
         (void*)(const char*)mStrings[(int)kDone];
      Message("Select original content or a fan mission, then choose Play.");
   }
};

static const char* content_button_names[] = { "load", "done" };

sDarkPanelDesc cContentMenu::gDesc =
{
   "gamelod",
   cContentMenu::kNumButts,
   cContentMenu::kNumRects,
   cContentMenu::kNumButts,
   content_button_names,
   NULL,
   0,
   kMetaSndPanelSaveLoad,
};

static cContentMenu* gpContentMenu = NULL;

const sLoopInstantiator* DescribeContentMenuMode(void)
{
   if (gpContentMenu)
   {
      cAutoIPtr<IPanelMode> panel = gpContentMenu->GetPanelMode();
      return panel->Instantiator();
   }
   return NULL;
}

//------------------------------------------------------------
// MAIN MENU 
//

class cMainMenu: public cMenuBase
{
   static sDarkPanelDesc gDesc; 

public:
   cMainMenu() : cMenuBase(&gDesc)
   {
   };

   enum eRects
   {
      kNewGame,
      kLoadGame,
      kOptions,
      kCredits,
      kIntro,
      kQuit,

      kNumRects
   };

   
protected:
   void OnButtonList(ushort action, int button)
   {
      if (!(action & BUTTONGADG_LCLICK))
         return ;

      switch (button)
      {
         case kNewGame:
            SwitchToNewGameMode(TRUE); 
            break; 

         case kLoadGame:
            SwitchToLoadGameMode(TRUE); 
            break; 

         case kOptions:
            SwitchToOptionsMode(TRUE);
            break; 

         case kCredits:
            PushMovieOrBookMode("credits");
            break; 

         case kIntro:
            PushMovieOrBookMode("intro");
            break; 

         case kQuit:
         {
            AutoAppIPtr_(Application,pApp);
            pApp->Quit(); 
            return; 
         }
         break;
            
      }
   }

   void OnEscapeKey()  // trap escape
   {
   }
}; 

//
// Main Menu descriptor 
//

static const char* main_button_names[] = 
{
   "new_game",
   "load_game",
   "options",
   "credits",
   "intro",
   "quit",
}; 

sDarkPanelDesc cMainMenu::gDesc = 
{
   "main",
   cMainMenu::kNumRects,
   cMainMenu::kNumRects,
   cMainMenu::kNumRects,
   main_button_names, 
   NULL, // font
   0,
   kMetaSndPanelMain,
}; 


static cMainMenu* gpMainMenu = NULL; 

void SwitchToMainMenuMode(BOOL push) 
{
   if (gpMainMenu)
   {
      cAutoIPtr<IPanelMode> panel = gpMainMenu->GetPanelMode(); 
      panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
   }
}


const sLoopInstantiator* DescribeMainMenuMode(void)
{
   if (gpMainMenu)
   {
      cAutoIPtr<IPanelMode> panel = gpMainMenu->GetPanelMode(); 
      return panel->Instantiator(); 
   }
   return NULL; 
}

//------------------------------------------------------------
// SIM MENU 
//

class cSimMenu: public cMenuBase
{
   static sDarkPanelDesc gDesc; 

public:
   cSimMenu() : cMenuBase(&gDesc)
   {
   };

   enum eRects
   {
      kContinue,
      kOptions,
      kSaveGame,
      kLoadGame,
      kMap,
      kObjectives,
      kQuit,
      kRestart,

      kNumRects
   };
   
protected:

   void OnButtonList(ushort action, int button)
   {
      if (!(action & BUTTONGADG_LCLICK))
         return ;

      switch (button)
      {
         case kContinue:
         {
            cAutoIPtr<IPanelMode> mode = GetPanelMode(); 
            mode->Exit(); 
         }
         break; 

         case kLoadGame:
            SwitchToLoadGameMode(TRUE); 
            break; 

         case kOptions:
            SwitchToOptionsMode(TRUE);
            break; 

         case kSaveGame:
            SwitchToSaveGameMode(TRUE); 
            break; 

         case kMap:
            SwitchToDarkAutomapMode(TRUE); 
            break; 

         case kObjectives:
            SwitchToObjectivesMode(TRUE); 
            break; 

         case kRestart:
         {
            MissionLoopReset(kMissLoopRestartMission); 
            UnwindToMissionLoop(); 
         }
         break; 

         case kQuit:
         {
#ifdef EDITOR
            // In DromEd this is "leave play preview", not "quit the editor".
            // Pop the menu first and let game mode switch cleanly back to the
            // editor so no suspended gameplay mode remains on the loop stack.
            GameModeExitToEditor();
#else
            MissionLoopReset(kMissLoopMainMenu); 
            UnwindToMissionLoop(); 
#endif
         }
         break;
            
      }
   }
}; 

//
// Sim Menu descriptor 
//

static const char* sim_button_names[] = 
{
   "continue",
   "options",
   "save_game",
   "load_game",
   "map",
   "objectives",
   "quit",
   "restart",
}; 

sDarkPanelDesc cSimMenu::gDesc = 
{
   "sim",
   cSimMenu::kNumRects,
   cSimMenu::kNumRects,
   cSimMenu::kNumRects,
   sim_button_names, 
}; 


static cSimMenu* gpSimMenu = NULL; 

void SwitchToSimMenuMode(BOOL push) 
{
   if (gpSimMenu)
   {
      cAutoIPtr<IPanelMode> panel = gpSimMenu->GetPanelMode(); 
      panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
   }
}


const sLoopInstantiator* DescribeSimMenuMode(void)
{
   if (gpSimMenu)
   {
      cAutoIPtr<IPanelMode> panel = gpSimMenu->GetPanelMode(); 
      return panel->Instantiator(); 
   }
   return NULL; 
}

//------------------------------------------------------------
// NewGame MENU 
//

class cNewGame: public cDarkPanel
{
   static sDarkPanelDesc gDesc; 
   static sDarkPanelDesc gThief1Desc;

public:
   cNewGame() 
      : cDarkPanel(IsThief1() ? &gThief1Desc : &gDesc),
        mHasTrainingButton(IsThief1())
   {
   };

   static BOOL IsThief1()
   {
      return _stricmp(DarkContentProfile(), "gold") == 0 ||
             _stricmp(DarkContentProfile(), "tdp") == 0;
   }

   enum { kNumDiffs = 3 };

protected:
   BOOL mHasTrainingButton;
   guiStyle mDiffStyle; 
   LGadButtonList mDiffButtons; 
   DrawElement mDiffElems[kNumDiffs];
   cStr mDiffStrs[kNumDiffs]; 

   static bool DiffCB (ushort action, int button, void *data, LGadBox *)
   {
      g_diff = button;
      return FALSE;
   }

   void InitUI()
   {
      cDarkPanel::InitUI(); 
      
      // default to current style colors
      AssertMsg(GetCurrentStyle(),"No current style for diff defaults"); 
      memset(&mDiffStyle,0,sizeof(mDiffStyle));
      memcpy(mDiffStyle.colors,GetCurrentStyle()->colors,sizeof(mDiffStyle.colors)); 
      uiGameLoadStyle("diff_",&mDiffStyle,mResPath); 

      // set up drawlelems for diff 
      for (int i = 0; i < kNumDiffs; i++)
      {
         DrawElement& elem = mDiffElems[i];
         memset(&elem,0,sizeof(elem)); 

         char buf[16];
         sprintf(buf,"diff_%d",i); 
         mDiffStrs[i] = FetchUIString(panel_name,buf,mResPath); 
         elem.draw_type = DRAWTYPE_TEXT; 
         elem.draw_data = (void*)(const char*)mDiffStrs[i]; 
      }

      LGadButtonListDesc desc = 
      { 
         kNumDiffs,
         &mRects[num_butts + 1],
         mDiffElems,
         DiffCB,
         0,
         BUTTONLIST_RADIO_FLAG,
      };

      memset( &mDiffButtons, 0, sizeof(LGadButtonList) );
      LGadCreateButtonListDesc(&mDiffButtons,LGadCurrentRoot(),&desc); 

      //set the current difficulty setting
      LGadRadioButtonSelect (&mDiffButtons, g_diff); 

      LGadBoxSetStyle(&mDiffButtons,&mDiffStyle); 
   }

   void TermUI()
   {
      g_diff = LGadRadioButtonSelection(&mDiffButtons); 
      LGadDestroyButtonList(&mDiffButtons); 
      memset(&mDiffElems,0,sizeof(mDiffElems)); 
      for (int i = 0; i < kNumDiffs; i++)
         mDiffStrs[i] = ""; 

      uiGameUnloadStyle(&mDiffStyle); 
      cDarkPanel::TermUI(); 
   }

   void RedrawDisplay()
   {

      // Draw the difficulty string.  This is a lot of code to draw a lousy string. 

      cStr str = FetchUIString(panel_name,"difficulty",mResPath);  // copy the string 
      char* s = (char*)(const char*)str;  // get a mutable pointer

      Rect& r = mRects[num_butts];

      GUIcompose c; 
      GUIsetup(&c,&r,GUIcomposeFlags(ComposeFlagClear|ComposeFlagRead),GUI_CANV_ANY); 

      guiStyleSetupFont(&mDiffStyle,StyleFontTitle); 
      gr_set_fcolor(guiStyleGetColor(&mDiffStyle,StyleColorText)); 

      gr_font_string_wrap(gr_get_font(),s,RectWidth(&r)); 

      short w,h; 
      gr_string_size(s,&w,&h); 

      gr_string(s,(RectWidth(&r) - w)/2,(RectHeight(&r) - h)/2); 
      //      gr_string(s,gr_char_width('X'),(RectHeight(&r) - h)/2); 

      guiStyleCleanupFont(&mDiffStyle,StyleFontTitle) ;

      GUIdone(&c); 

      // 
      // Make sure the buttonlist gets drawn
      //
      region_expose(LGadBoxRegion(&mDiffButtons),LGadBoxRect(&mDiffButtons)); 
      
   }

   void StartGame(BOOL training = FALSE)
   {
      // Reset the campaign
      AutoAppIPtr(Campaign); 
      pCampaign->New(); 

      // Set the difficulty
      g_diff = LGadRadioButtonSelection(&mDiffButtons); 
      AutoAppIPtr(QuestData); 
      pQuestData->Create (DIFF_QVAR, g_diff, kQuestDataCampaign);

      // Campaign::New resets the mission loop to mission 1.  In Thief 1 that
      // is the training mission, so the second button must explicitly advance
      // to Lord Bafford (mission 2).  An FM-provided starting_mission below
      // still takes precedence over this stock default.
      int mission = mHasTrainingButton ? (training ? 1 : 2) : GetNextMission();
      const int defaultMission = mission;
      
      if (!training && config_get_int("starting_mission",&mission))
      {
         static int mission_map[32] = { 1,2,4,5,6,7,9,8,10,11,14,15,12,13,16 }; 
         int cnt = sizeof(mission_map)/sizeof(mission_map[0]); 

         if (!config_get_value("mission_map",CONFIG_INT_TYPE,(config_valptr)mission_map,&cnt))
            cnt = 16;  // @HACK for back-compatibility with thief 1.   
         
         if (mission > 0 && (mission-1) < cnt && mission_map[mission-1] != 0)
         {
            mission = mission_map[mission-1]; //lose the zero index
         }
         else
            mission = defaultMission;
      }
      
      SetNextMission(mission); 

      // Start the mission loop 
      MissionLoopReset(kMissLoopStartLoop); 
      UnwindToMissionLoop(); 
   }

   void OnButtonList(ushort action, int button)
   {
      if (!(action & BUTTONGADG_LCLICK))
         return ;

      if (mHasTrainingButton)
      {
         switch (button)
         {
            case 0: StartGame(TRUE); break;
            case 1: StartGame(FALSE); break;
            case 2: SwitchToOptionsMode(TRUE); break;
            case 3:
            {
               cAutoIPtr<IPanelMode> mode = GetPanelMode();
               mode->Exit();
            }
            break;
         }
      }
      else
      {
         switch (button)
         {
            case 0: StartGame(FALSE); break;
            case 1: SwitchToOptionsMode(TRUE); break;
            case 2:
            {
               cAutoIPtr<IPanelMode> mode = GetPanelMode();
               mode->Exit();
            }
            break;
         }
      }
   }
}; 

//
// NewGame Menu descriptor 
//

static const char* newgame_button_names[] = 
{
   "skip_training",
   "options",
   "cancel"
}; 

static const char* thief1_newgame_button_names[] =
{
   "start_training",
   "skip_training",
   "options",
   "cancel"
};

sDarkPanelDesc cNewGame::gDesc = 
{
   "newgame",
   3,
   7,
   3,
   newgame_button_names,
   NULL,
   0,
   kMetaSndPanelMain,
}; 

sDarkPanelDesc cNewGame::gThief1Desc =
{
   "newgame",
   4,
   8,
   4,
   thief1_newgame_button_names,
   NULL,
   0,
   kMetaSndPanelMain,
};


static cNewGame* gpNewGame = NULL; 

void SwitchToNewGameMode(BOOL push) 
{
   if (gpNewGame)
   {
      cAutoIPtr<IPanelMode> panel = gpNewGame->GetPanelMode(); 
      panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
   }
}


const sLoopInstantiator* DescribeNewGameMode(void)
{
   if (gpNewGame)
   {
      cAutoIPtr<IPanelMode> panel = gpNewGame->GetPanelMode(); 
      return panel->Instantiator(); 
   }
   return NULL; 
}



//------------------------------------------------------------------
// INIT/TERM
//


static void init_commands();

EXTERN void OptionsMenuInit ();
EXTERN void OptionsMenuTerm ();


void DarkMenusInit()
{
   if (DarkContentShowPicker())
      gpContentMenu = new cContentMenu;
   gpMainMenu = new cMainMenu; 
   gpSimMenu = new cSimMenu; 
   gpNewGame = new cNewGame; 
   OptionsMenuInit ();

   init_commands(); 
}

void DarkMenusTerm()
{
   delete gpContentMenu;
   gpContentMenu = NULL;
   delete gpMainMenu; 
   delete gpSimMenu; 
   delete gpNewGame;
   OptionsMenuTerm ();
}


//----------------------------------------
// COMMANDS
//

static void do_mainmenu()
{
   SwitchToMainMenuMode(TRUE); 
}

static void do_simmenu()
{
   SwitchToSimMenuMode(TRUE); 
}



static Command commands[] = 
{
   { "main_menu", FUNC_VOID, do_mainmenu, "Go to main menu.", HK_ALL }, 
   { "sim_menu", FUNC_VOID, do_simmenu, "Go to sim menu.", HK_ALL }, 

}; 


static void init_commands()
{
   COMMANDS(commands,HK_ALL); 
}



