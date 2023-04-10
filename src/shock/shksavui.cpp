/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shksavui.cpp,v 1.13 1999/06/08 23:52:11 XEMU Exp $

#include <appagg.h>
#include <time.h>

#include <shksavui.h>
#include <shkmenu.h>
#include <gadtext.h>
#include <drkpanl.h>
#include <loopapi.h>
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
#include <mprintf.h>
#include <filepanl.h>
#include <simman.h>
#include <config.h>

#include <playrobj.h>
#include <shkplayr.h>
#include <shkprop.h>
#include <shkplprp.h>

#include <shkutils.h>
#include <shkmulti.h>
#include <transmod.h>

#include <panltool.h>
#include <command.h>
#include <uianim.h>

#include <netman.h>

#include <dbmem.h>

////////////////////////////////////////////////////////////
//
// SHOCK SAVE/LOAD PANELS 
// copied brutally from Thief
//
//

//============================================================
// Base file panel, derived from cFilePanel
//

class cShockFilePanel: public cFilePanel
{
public:
   cShockFilePanel(const sDarkPanelDesc* desc) 
      : cFilePanel(desc)
   {
   }

   virtual const char* SlotFileName(int which); 
   virtual void ComputeSlot(int which); 
   virtual void TotalFailure(); 
}; 

// These are temporary hacks 
#define SAVE_PATH "saves" 
#define QUICK_SAVE "current"

// actually returns the directory root that all the save game files
// should go into
const char* cShockFilePanel::SlotFileName(int which)
{
   if (which == kQuickSlot)
      return QUICK_SAVE; 
   static char buf[64]; 
   ShockGetSaveDir(which, buf);
   return buf; 
}

//----------------------------------------

void cShockFilePanel::ComputeSlot(int which)
{
   sSlot& slot = mpSlots[which]; 
   if (!(slot.flags & kNoCompute))
   {
      slot.flags = 0; 
      char temp[255];
      sprintf(temp,"%s\\campaign",SlotFileName(which));
      ITagFile* file = TagFileOpen(temp,kTagOpenRead); 
      //mprintf("opening tagfile %s\n",temp);
      BOOL read = FALSE; 
      if (file)
      {
         //mprintf("about to get savedesc %d\n",which);
         //mprintf("slot.buf is %x\n",slot.buf);
         if (slot.buf == NULL)
         {
            Warning(("cShockFilePanel::ComputeBlock: Hey, buf is NULL!\n"));
         }
         else
         {
            strncpy(slot.buf,"NAME ERROR",sizeof(slot.buf)); // in case we fail to get a gamedesc
            read = TRUE; 

            BOOL retval = ShockGetSaveGameDesc(file,slot.buf,sizeof(slot.buf));  
            if (!retval)
               Warning(("cShockFilePanel::ComputeBlock: Hey, savedesc is unreadable!\n"));
         }
      }
      if (!read)
      {
         strncpy(slot.buf,FetchUIString(panel_name,"unused",mResPath),sizeof(slot.buf)); 
         slot.flags |= kUnused; 
      }
      SafeRelease(file); 
   }
   LGadUpdateTextBox(&slot.box); 

}

//----------------------------------------

void cShockFilePanel::TotalFailure()
{
   // go back to the main menu 
   sLoopInstantiator* mainmenu = (sLoopInstantiator*)DescribeShockMainMenuMode(); 
   AutoAppIPtr(Loop); 
   pLoop->ChangeMode(kLoopModeUnwindTo,mainmenu); 
}


//============================================================
// Load File Panel
//

class cShockLoadPanel: public cShockFilePanel
{

public:
   cShockLoadPanel() : cShockFilePanel(&gDesc) {}; 

protected:
   static sDarkPanelDesc gDesc; 

   void InitUI()
   {
      cShockFilePanel::InitUI(); 
      MessageNamed("initial"); 
   }

   void DoFileOp(int which);
   void OnSelect(int which); 
}; 

//----------------------------------------

// This is a dummy, to be replaced
#define FAILED_BEYOND_HOPE(x) FALSE

void cShockLoadPanel::DoFileOp(int which)
{
   if (mpSlots[which].flags & kUnused)
      return; 
   MessageNamed("loading"); 

   //const char* fname = SlotFileName(which); 
   HRESULT result = S_OK; 

   if (which == kQuickSlot)  // swizzle quick save slot
      which = -1; 

#ifdef NO_LOADING_SCREEN

   result = ShockLoadGame(which);

   if (FAILED(result))
   {
      MessageNamed("failed"); 
      // this is the test for 
      // "we failed late enough that we can't just go back to the sim; 
      //  we gotta just punt back to main menu"
      if (FAILED_BEYOND_HOPE(result))
      {
         mTotalFailure = TRUE;
         EnableFileOp(FALSE); 
      }
   }
   else
   {
      AutoAppIPtr(SimManager);
      pSimManager->StartSim();
   }

#else 
   TransModeLoadGame(which); 
   // @TODO: detect failure cases 
#endif 
}

//----------------------------------------

void cShockLoadPanel::OnSelect(int which)
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
   "delete",
}; 

sDarkPanelDesc cShockLoadPanel::gDesc = 
{
   "gamelod",
   cShockLoadPanel::kNumButts,
   cShockLoadPanel::kNumRects,
   cShockLoadPanel::kNumButts,
   load_strings,
   NULL,
   0,
   kMetaSndPanelSaveLoad,
}; 

//----------------------------------------

static cShockLoadPanel* gpLoadPanel = NULL; 

void SwitchToShockLoadGameMode(BOOL push)
{
   if (!gpLoadPanel)
      gpLoadPanel = new cShockLoadPanel; 
   if (gpLoadPanel)
   {
      cAutoIPtr<IPanelMode> mode = gpLoadPanel->GetPanelMode(); 
      mode->Switch(push ? kLoopModePush : kLoopModeSwitch); 
   }
}


//============================================================
// Save File Panel
//


class cShockSavePanel: public cShockFilePanel
{

public:
   cShockSavePanel() : cShockFilePanel(&gDesc) {}; 

protected:
   static sDarkPanelDesc gDesc; 

   void InitUI()
   {
      cShockFilePanel::InitUI(); 
      MessageNamed("initial"); 
   }

   void DoFileOp(int which);
   void OnSelect(int which); 
   void OnDeselect(int which); 

}; 

//----------------------------------------

#define MIN_SAVE_FREE_STORAGE (15 << 20)

void cShockSavePanel::DoFileOp(int which)
{
   int space;
   //const char* fname = SlotFileName(which); 

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

   //mkdir(SAVE_PATH); 

   //ITagFile* file = TagFileOpen(fname,kTagOpenWrite); 
   HRESULT retval = E_FAIL; 
   sSlot& slot = mpSlots[which]; 

   MessageNamed("saving"); 
   retval = ShockSaveGame(which, slot.buf);
   //SafeRelease(file); 

   MessageNamed(SUCCEEDED(retval)? "success" : "failure" ); 

   if (SUCCEEDED(retval))
      mpPanelMode->Exit(); 
}

//----------------------------------------

void cShockSavePanel::OnDeselect(int which)
{
   sSlot& slot = mpSlots[which]; 
   LGadTextBoxClrFlag(&slot.box,TEXTBOX_EDIT_EDITABLE); 
   LGadUnfocusTextBox(&slot.box); 
   slot.flags &= ~kNoCompute; 
}

//------------------------------------------------------------

void cShockSavePanel::OnSelect(int which)
{
   sSlot& slot = mpSlots[which]; 

   // slot zero is the quick save, don't edit it 
   if (which == kQuickSlot)
      strncpy(slot.buf,FetchUIString(panel_name,"quick_save"),sizeof(slot.buf));
   else
   {
      LGadTextBoxSetFlag(&slot.box,TEXTBOX_EDIT_EDITABLE|TEXTBOX_EDIT_BRANDNEW); 
      LGadFocusTextBox(&slot.box); 



      // The point of this CONDITIONAL_FILL stuff is to not 
      // fill in a title that doesn't look like it was auto-filled
      // by the user.  It looks for a colon-delimited time.  
#define CONDITIONAL_FILL
#ifdef CONDITIONAL_FILL

      BOOL fill = slot.flags & kUnused;
      if (!fill)
      {  // if end of string in our time format, go ahead and do the fill
         if (config_is_defined("savename_autofill"))
            fill = TRUE;
         else
         {
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
      }
#else
      BOOL fill = TRUE; 
#endif // CONDITIONAL FILL

      if (fill)
      {
         // here's the code that fills in the "default" save game description
         //cStr title = "Time: "; // set the "mission title" here 

         char levelname[64];
         ShockGetLevelName(levelname,sizeof(levelname));
         // okay this is hack-a-riffic
         if (stricmp(levelname,"station") == 0)
         {
            int service;
            g_ServiceProperty->Get(PlayerObject(),&service);
            ShockStringFetch(slot.buf,sizeof(slot.buf),levelname,"savename",service);
         }
         else
            ShockStringFetch(slot.buf,sizeof(slot.buf),levelname,"savename");

         //strncpy(slot.buf,title,sizeof(slot.buf));

         //struct tm time_of_day;
         //time_t ltime;
         //time(&ltime);
         //memcpy(&time_of_day, localtime(&ltime), sizeof(struct tm));

         // Add in sim time 
         int time = GetSimTime()/SIM_TIME_SECOND;
         char buf[20]; 
         // I wonder if the compiler will optimize this math?
         sprintf(buf," %d:%02d:%02d",time/(60*60),(time/60)%60,time%60); 
         //strncpy(buf,asctime(&time_of_day),sizeof(buf));

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
   "delete",
}; 

sDarkPanelDesc cShockSavePanel::gDesc = 
{
   "gamesav",
   cShockSavePanel::kNumButts,
   cShockSavePanel::kNumRects,
   cShockSavePanel::kNumButts,
   save_strings,
   NULL,
   0,
   kMetaSndPanelSaveLoad,
}; 

//----------------------------------------

static cShockSavePanel* gpSavePanel = NULL; 

void SwitchToShockSaveGameMode(BOOL push)
{
   if (!gpSavePanel)
      gpSavePanel = new cShockSavePanel; 

   if (gpSavePanel)
   {
      cAutoIPtr<IPanelMode> mode = gpSavePanel->GetPanelMode(); 
      mode->Switch(push ? kLoopModePush : kLoopModeSwitch); 
   }
}


//------------------------------------------------------------
// Init/Term
//

void init_commands(); 

void ShockSaveUIInit()
{
   init_commands(); 
}

void ShockSaveUITerm()
{
   delete gpLoadPanel; 
   delete gpSavePanel; 
}


//------------------------------------------------------------
// Commands
//

static void do_load()
{
   SwitchToShockLoadGameMode(TRUE); 
}

static void do_save()
{
   SwitchToShockSaveGameMode(TRUE); 
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

