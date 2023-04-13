/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drksavui.cpp,v 1.23 2000/02/26 16:49:55 toml Exp $

#include <appagg.h>
#include <drksavui.h>
#include <gadtext.h>
#include <drkpanl.h>
#include <drkmislp.h>
#include <drksave.h>
#include <drkuires.h>
#include <uigame.h>
#include <tagfile.h>
#include <gcompose.h>
#include <scrnman.h>
#include <direct.h>
#include <simtime.h>
#include <gen_bind.h>
#include <keydefs.h>
#include <metasnd.h>
#include <diskfree.h>
#include <drkdebrf.h>
#include <mprintf.h>

#include <panltool.h>
#include <command.h>
#include <uianim.h>
#include <filepanl.h>

#include <btffact.h>

#include <dbmem.h>

//============================================================
// Base file panel, derived from cFilePanel
//

class cDarkFilePanel: public cFilePanel
{
public:
   cDarkFilePanel(const sDarkPanelDesc* desc) 
      : cFilePanel(desc)
   {
   }

   virtual const char* SlotFileName(int which); 
   virtual void ComputeSlot(int which); 
   virtual void TotalFailure(); 

}; 


//----------------------------------------


const char* cDarkFilePanel::SlotFileName(int which)
{
   if (which == kQuickSlot)
      return SAVE_PATH "\\" QUICK_SAVE; 
   static char buf[64]; 
   sprintf(buf,"%s\\game%04d.sav",SAVE_PATH,which); 
   return buf; 
}

//----------------------------------------

void cDarkFilePanel::ComputeSlot(int which)
{
   sSlot& slot = mpSlots[which]; 
   if (!(slot.flags & kNoCompute))
   {
      slot.flags = 0; 
      ITagFile* file = TagFileOpen(SlotFileName(which),kTagOpenRead); 
      BOOL read = FALSE; 
      if (file)
         read = DarkLoadGameDescription(file,slot.buf,sizeof(slot.buf)); 
      if (!read)
      {
         strncpy(slot.buf,FetchUIString(panel_name,"unused",mResPath),sizeof(slot.buf)); 
         slot.flags |= kUnused; 
      }
      SafeRelease(file); 
   }
   LGadUpdateTextBox(&slot.box); 

}

void cDarkFilePanel::TotalFailure()
{
   MissionLoopReset(kMissLoopMainMenu); 
   UnwindToMissionLoop(); 
}


//============================================================
// Load File Panel
//

class cLoadPanel: public cDarkFilePanel
{

public:
   cLoadPanel() : cDarkFilePanel(&gDesc) {}; 

protected:
   static sDarkPanelDesc gDesc; 

   void InitUI()
   {
      cDarkFilePanel::InitUI(); 
      MessageNamed("initial"); 
   }

   void DoFileOp(int which);
   void OnSelect(int which); 
}; 

//----------------------------------------

void cLoadPanel::DoFileOp(int which)
{
   if (mpSlots[which].flags & kUnused)
      return; 
   MessageNamed("loading"); 

   const char* fname = SlotFileName(which); 

   HRESULT result = DarkQuickLoadGameFile(fname); 
   if (FAILED(result))
   {
      MessageNamed("failed"); 
      if (result == kQuickLoadFailed)
      {
         mTotalFailure = TRUE;
         EnableFileOp(FALSE); 
      }

      
   }
}

//----------------------------------------

void cLoadPanel::OnSelect(int which)
{
   sSlot& slot = mpSlots[which]; 
   LGadTextBoxClrFlag(&slot.box,TEXTBOX_EDIT_EDITABLE); 
   LGadUnfocusTextBox(&slot.box); 
   if (slot.flags & kUnused)
      EnableFileOp(FALSE); 
}


//----------------------------------------

static const char* load_strings[] = 
{
   "load",
   "done", 
}; 

sDarkPanelDesc cLoadPanel::gDesc = 
{
   "gamelod",
   cLoadPanel::kNumButts,
   cLoadPanel::kNumRects,
   cLoadPanel::kNumButts,
   load_strings,
   NULL,
   0,
   kMetaSndPanelSaveLoad,
}; 

//----------------------------------------

static cLoadPanel* gpLoadPanel = NULL; 

void SwitchToLoadGameMode(BOOL push)
{
   if (!gpLoadPanel)
      gpLoadPanel = new cLoadPanel; 
   if (gpLoadPanel)
   {
      cAutoIPtr<IPanelMode> mode = gpLoadPanel->GetPanelMode(); 
      mode->Switch(push ? kLoopModePush : kLoopModeSwitch); 
   }
}


//============================================================
// Save File Panel
//

class cSavePanel: public cDarkFilePanel
{

public:
   cSavePanel() : cDarkFilePanel(&gDesc) {}; 

protected:
   static sDarkPanelDesc gDesc; 

   void InitUI()
   {
      cDarkFilePanel::InitUI(); 
      MessageNamed("initial"); 
   }

   void DoFileOp(int which);
   void OnSelect(int which); 
   void OnDeselect(int which); 

}; 

//----------------------------------------

#define MIN_SAVE_FREE_STORAGE (5 << 20)

void cSavePanel::DoFileOp(int which)
{
   int space;
   const char* fname = SlotFileName(which); 

   // ok, this isn't really right, because if we're
   // overwriting an existing save, we don't need free
   // space, or not as much, right?  really, maybe
   // we shouldn't do this, we should just let the
   // save detect that it failed
   space = compute_free_diskspace(NULL);
   if (space >= 0 && space < MIN_SAVE_FREE_STORAGE) {
      MessageNamed("no_disk_space");
      return;
   }

   mkdir(SAVE_PATH); 

   ITagFile* file = TagFileOpen(fname,kTagOpenWrite); 
   HRESULT retval = E_FAIL; 
   if (file)
   {
      MessageNamed("saving"); 
      retval = DarkSaveGame(mpSlots[which].buf,file); 
      SafeRelease(file); 
   }

   MessageNamed(SUCCEEDED(retval)? "success" : "failure" ); 

   if (SUCCEEDED(retval))
      mpPanelMode->Exit(); 
}

//----------------------------------------

void cSavePanel::OnDeselect(int which)
{
   sSlot& slot = mpSlots[which]; 
   LGadTextBoxClrFlag(&slot.box,TEXTBOX_EDIT_EDITABLE); 
   LGadUnfocusTextBox(&slot.box); 
   slot.flags &= ~kNoCompute; 
}

//------------------------------------------------------------

void cSavePanel::OnSelect(int which)
{
   sSlot& slot = mpSlots[which]; 

   // slot zero is the quick save, don't edit it 
   if (which == kQuickSlot)
      strncpy(slot.buf,FetchUIString(panel_name,"quick_save"),sizeof(slot.buf));
   else
   {
      LGadTextBoxSetFlag(&slot.box,TEXTBOX_EDIT_EDITABLE|TEXTBOX_EDIT_BRANDNEW); 
      LGadFocusTextBox(&slot.box); 

      // Fill slot with a default title
      // Start with the mission name 
      int miss = GetNextMission(); 
      char buf[64]; 

      sprintf(buf,"short_%d",miss); 
      cStr title = FetchUIString("titles",buf,"strings");

#define CONDITIONAL_FILL
#ifdef CONDITIONAL_FILL
#ifdef MAHKS_WAY
      BOOL fill = slot.flags & kUnused; 
      // if the slot is used, only fill if it matches the title
      if (!fill)
         fill = strncmp(title,slot.buf,strlen(title)) == 0; 
#else
      BOOL fill = slot.flags & kUnused;
      if (!fill)
      {  // if end of string in our time format, go ahead and do the fill
         char *p=slot.buf;
         char *colon1, *colon2;
         colon2=strrchr(p,':');
         if (colon2)
         {
            *colon2='\0';
            colon1=strrchr(p,':');
            *colon2=':';
            if (colon1&&colon2)
            {
               char *eos=p+strlen(p);
               if ((colon2==eos-3)&&(colon1==eos-6))
                  fill=TRUE;
            }
         }
      }
#endif
#else
      BOOL fill = TRUE; 
#endif // CONDITIONAL FILL

      if (fill)
      {
         strncpy(slot.buf,title,sizeof(slot.buf));

         // Add in sim time 
         int time = GetSimTime()/SIM_TIME_SECOND;
         // I wonder if the compiler will optimize this math?
         sprintf(buf," %d:%02d:%02d",time/(60*60),(time/60)%60,time%60); 

         int len = strlen(slot.buf); 
         strncpy(slot.buf + len,buf,sizeof(slot.buf)-len); 
      }
   }

   slot.flags |= kNoCompute; 

   LGadUpdateTextBox(&slot.box); 
}



//----------------------------------------

static const char* save_strings[] = 
{
   "save",
   "done", 
}; 

sDarkPanelDesc cSavePanel::gDesc = 
{
   "gamesav",
   cSavePanel::kNumButts,
   cSavePanel::kNumRects,
   cSavePanel::kNumButts,
   save_strings,
   NULL,
   0,
   kMetaSndPanelSaveLoad,
}; 

//----------------------------------------

static cSavePanel* gpSavePanel = NULL; 

void SwitchToSaveGameMode(BOOL push)
{
   if (!gpSavePanel)
      gpSavePanel = new cSavePanel; 

   if (gpSavePanel)
   {
      cAutoIPtr<IPanelMode> mode = gpSavePanel->GetPanelMode(); 
      mode->Switch(push ? kLoopModePush : kLoopModeSwitch); 
   }
}






//============================================================
// Load Bind File Panel
//

class cLoadBndPanel: public cDarkFilePanel
{

public:
   cLoadBndPanel() : cDarkFilePanel(&gDesc) {}; 

protected:
   static sDarkPanelDesc gDesc; 

   void InitUI()
   {
      cDarkFilePanel::InitUI(); 
      MessageNamed("initial"); 
   }

   const char* SlotFileName(int which);
   void ComputeSlot(int which);
   void DoFileOp(int which);
   void OnSelect(int which); 
}; 

//----------------------------------------

const char* cLoadBndPanel::SlotFileName(int which)
{
   static char buf[64]; 
   sprintf(buf,"%s\\cfg%04d.bnd",SAVE_PATH,which); 
   return buf; 
}

//----------------------------------------

void cLoadBndPanel::ComputeSlot(int which)
{
   sSlot& slot = mpSlots[which]; 
   if (!(slot.flags & kNoCompute))
   {
      slot.flags = 0; 
      static char str[128], *p_str;
      FILE *fp = fopen (SlotFileName (which), "rb");
      if (fp) {
         fgets (str, 127, fp);
         fclose (fp);
         for (p_str = str; ((*p_str == ';' || *p_str == ' ') && *p_str != '\n'); p_str++);
         strncpy (slot.buf, p_str, sizeof (slot.buf));
      }

      else {
         strncpy(slot.buf,FetchUIString(panel_name,"unused",mResPath),sizeof(slot.buf)); 
         slot.flags |= kUnused; 
      }
   }
   LGadUpdateTextBox(&slot.box); 

}

//----------------------------------------

void cLoadBndPanel::DoFileOp(int which)
{
   if (mpSlots[which].flags & kUnused)
      return; 
   const char* fname = SlotFileName(which); 

   MessageNamed("loading"); 
   
   g_pInputBinder->LoadBndFile ((char *)fname, HK_GAME_MODE, NULL);
   mpPanelMode->Exit(); 
}

//----------------------------------------

void cLoadBndPanel::OnSelect(int which)
{
   sSlot& slot = mpSlots[which]; 
   LGadTextBoxClrFlag(&slot.box,TEXTBOX_EDIT_EDITABLE); 
   LGadUnfocusTextBox(&slot.box); 
   if (slot.flags & kUnused)
      EnableFileOp(FALSE); 
}


//----------------------------------------

static const char* loadbnd_strings[] = 
{
   "load",
   "done", 
}; 

sDarkPanelDesc cLoadBndPanel::gDesc = 
{
   "bndload",
   cLoadBndPanel::kNumButts,
   cLoadBndPanel::kNumRects,
   cLoadBndPanel::kNumButts,
   loadbnd_strings,
}; 

//----------------------------------------

static cLoadBndPanel* gpLoadBndPanel = NULL; 

void SwitchToLoadBndMode(BOOL push)
{
   if (!gpLoadBndPanel)
      gpLoadBndPanel = new cLoadBndPanel; 
   if (gpLoadBndPanel)
   {
      cAutoIPtr<IPanelMode> mode = gpLoadBndPanel->GetPanelMode(); 
      mode->Switch(push ? kLoopModePush : kLoopModeSwitch); 
   }
}


//============================================================
// Save Bind File Panel
//

class cSaveBndPanel: public cDarkFilePanel
{

public:
   cSaveBndPanel() : cDarkFilePanel(&gDesc) {}; 

protected:
   static sDarkPanelDesc gDesc; 

   void InitUI()
   {
      cDarkFilePanel::InitUI(); 
      MessageNamed("initial"); 
   }

   const char* cSaveBndPanel::SlotFileName(int which);
   void cSaveBndPanel::ComputeSlot(int which);
   void DoFileOp(int which);
   void BndAppendControls (char *fname);
   void OnSelect(int which); 
   void OnDeselect(int which); 

}; 

//----------------------------------------

const char* cSaveBndPanel::SlotFileName(int which)
{
   static char buf[64]; 
   sprintf(buf,"%s\\cfg%04d.bnd",SAVE_PATH,which); 
   return buf; 
}

//----------------------------------------

void cSaveBndPanel::ComputeSlot(int which)
{
   sSlot& slot = mpSlots[which]; 
   if (!(slot.flags & kNoCompute))
   {
      slot.flags = 0; 
      static char str[128], *p_str;
      FILE *fp = fopen (SlotFileName (which), "rb");
      if (fp) {
         fgets (str, 127, fp);
         fclose (fp);
         for (p_str = str; ((*p_str == ';' || *p_str == ' ') && *p_str != '\n'); p_str++);
         strncpy (slot.buf, p_str, sizeof (slot.buf));
      }

      else {
         strncpy(slot.buf,FetchUIString(panel_name,"unused",mResPath),sizeof(slot.buf)); 
         slot.flags |= kUnused; 
      }
   }
   LGadUpdateTextBox(&slot.box); 

}

//----------------------------------------

void cSaveBndPanel::DoFileOp(int which)
{
   const char* fname = SlotFileName(which); 

   mkdir(SAVE_PATH); 
   MessageNamed("saving"); 

   //get header
   char header[128];
   strcpy (header, "; ");
   strcat (header, mpSlots[which].buf);


   //change to game context
   ulong old_context;
   g_pInputBinder->GetContext (&old_context);
   g_pInputBinder->SetContext (HK_GAME_MODE,TRUE);
   g_pInputBinder->SaveBndFile ((char *)fname, header);
   //plop in input variable control settings
   BndAppendControls ((char *)fname);
   g_pInputBinder->SetContext (old_context,TRUE);

   MessageNamed("success"); 
}

//----------------------------------------

void cSaveBndPanel::BndAppendControls (char *fname)
{
   FILE *fp = fopen (fname, "ab");

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
         fprintf (fp, "%s %s\n", *p_control, g_pInputBinder->ProcessCmd (str));
         p_control++;
      }

      fclose (fp);
   }
}


//----------------------------------------

void cSaveBndPanel::OnDeselect(int which)
{
   sSlot& slot = mpSlots[which]; 
   LGadTextBoxClrFlag(&slot.box,TEXTBOX_EDIT_EDITABLE); 
   LGadUnfocusTextBox(&slot.box); 
   slot.flags &= ~kNoCompute; 
}

//------------------------------------------------------------

void cSaveBndPanel::OnSelect(int which)
{
   sSlot& slot = mpSlots[which]; 

   LGadTextBoxSetFlag(&slot.box,TEXTBOX_EDIT_EDITABLE|TEXTBOX_EDIT_BRANDNEW); 
   LGadFocusTextBox(&slot.box); 

   // Fill slot with stuff
   strncpy(slot.buf,FetchUIString(panel_name,"my_binds",mResPath),sizeof(slot.buf));

   slot.flags |= kNoCompute;
   LGadUpdateTextBox(&slot.box); 
}



//----------------------------------------

static const char* savebnd_strings[] = 
{
   "save",
   "done", 
}; 

sDarkPanelDesc cSaveBndPanel::gDesc = 
{
   "bndsave",
   cSaveBndPanel::kNumButts,
   cSaveBndPanel::kNumRects,
   cSaveBndPanel::kNumButts,
   savebnd_strings,
}; 

//----------------------------------------

static cSaveBndPanel* gpSaveBndPanel = NULL; 

void SwitchToSaveBndMode(BOOL push)
{
   if (!gpSaveBndPanel)
      gpSaveBndPanel = new cSaveBndPanel; 

   if (gpSaveBndPanel)
   {
      cAutoIPtr<IPanelMode> mode = gpSaveBndPanel->GetPanelMode(); 
      mode->Switch(push ? kLoopModePush : kLoopModeSwitch); 
   }
}





//------------------------------------------------------------
// Init/Term
//

void init_commands(); 

void DarkSaveInitUI()
{
   init_commands(); 
}

void DarkSaveTermUI()
{
   delete gpLoadPanel; 
   delete gpSavePanel; 
   delete gpLoadBndPanel;
   delete gpSaveBndPanel;
}


//------------------------------------------------------------
// Commands
//

static void do_load()
{
   SwitchToLoadGameMode(TRUE); 
}

static void do_save()
{
   SwitchToSaveGameMode(TRUE); 
}


static Command commands[] = 
{
   { "load_game", FUNC_VOID, do_load, "Go to the game load UI", HK_ALL }, 
   { "save_game", FUNC_VOID, do_save, "Go to the game save UI", HK_ALL }, 

}; 


static void init_commands()
{
   COMMANDS(commands,HK_ALL); 
}

