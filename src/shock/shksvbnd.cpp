/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shksvbnd.cpp,v 1.4 1999/06/24 23:34:50 XEMU Exp $
#include <filepanl.h>
#include <appagg.h>
#include <gen_bind.h>
#include <direct.h>
#include <contexts.h>
#include <shksvbnd.h>

#include <dbmem.h>

#define SAVE_PATH "binds"

//============================================================
// Load Bind File Panel
//

class cShockLoadBndPanel: public cFilePanel
{

public:
   cShockLoadBndPanel() : cFilePanel(&gDesc) {}; 

protected:
   static sDarkPanelDesc gDesc; 

   void InitUI()
   {
      mkdir(SAVE_PATH); 
      cFilePanel::InitUI(); 
      MessageNamed("initial"); 
   }

   const char* SlotFileName(int which);
   void ComputeSlot(int which);
   void DoFileOp(int which);
   void OnSelect(int which); 

   void TotalFailure() { }; 
}; 

//----------------------------------------

static const char* slot_file_name(int which)
{
   static char buf[64]; 
   sprintf(buf,"%s\\cfga%04d.bnd",SAVE_PATH,which); 
   return buf; 
}


const char* cShockLoadBndPanel::SlotFileName(int which)
{
   return slot_file_name(which);
}

static  char* SlotFileName2(int which)
{
   static char buf[64]; 
   sprintf(buf,"%s\\cfgb%04d.bnd",SAVE_PATH,which); 
   return buf; 
}


//----------------------------------------

void cShockLoadBndPanel::ComputeSlot(int which)
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

void cShockLoadBndPanel::DoFileOp(int which)
{
   if (mpSlots[which].flags & kUnused)
      return; 
   const char* fname = SlotFileName(which); 

   MessageNamed("loading"); 

   ulong oldcontext;
   g_pInputBinder->GetContext(&oldcontext);

   // first, clear out our two contexts
   g_pInputBinder->SetContext(HK_GAME_MODE, FALSE);
   g_pInputBinder->Reset();
   g_pInputBinder->SetContext(HK_GAME2_MODE, FALSE);
   g_pInputBinder->Reset();

   // oh, restore our old context, BTW
   g_pInputBinder->SetContext(oldcontext, FALSE);

   // then load in the new Bnd Data
   g_pInputBinder->LoadBndFile ((char *)fname, HK_GAME_MODE, NULL);
   g_pInputBinder->LoadBndFile (SlotFileName2(which), HK_GAME2_MODE, NULL);

   
   mpPanelMode->Exit(); 
}

//----------------------------------------

void cShockLoadBndPanel::OnSelect(int which)
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

sDarkPanelDesc cShockLoadBndPanel::gDesc = 
{
   "bndload",
   cShockLoadBndPanel::kNumButts,
   cShockLoadBndPanel::kNumRects,
   cShockLoadBndPanel::kNumButts,
   loadbnd_strings,
}; 

//----------------------------------------

static cShockLoadBndPanel* gpLoadBnd = NULL; 

void SwitchToShockLoadBndMode(BOOL push)
{
   if (!gpLoadBnd)
      gpLoadBnd = new cShockLoadBndPanel; 
   if (gpLoadBnd)
   {
      cAutoIPtr<IPanelMode> mode = gpLoadBnd->GetPanelMode(); 
      mode->Switch(push ? kLoopModePush : kLoopModeSwitch); 
   }
}


//============================================================
// Save Bind File Panel
//

class cShockSaveBndPanel: public cFilePanel
{

public:
   cShockSaveBndPanel() : cFilePanel(&gDesc) {}; 

protected:
   static sDarkPanelDesc gDesc; 

   void InitUI()
   {
      mkdir(SAVE_PATH); 
      cFilePanel::InitUI(); 
      MessageNamed("initial"); 
   }

   const char* cShockSaveBndPanel::SlotFileName(int which);
   void cShockSaveBndPanel::ComputeSlot(int which);
   void DoFileOp(int which);
   void BndAppendControls (char *fname);
   void OnSelect(int which); 
   void OnDeselect(int which); 

   void TotalFailure() { }; 

}; 

//----------------------------------------

const char* cShockSaveBndPanel::SlotFileName(int which)
{
   return slot_file_name(which); 
}

//----------------------------------------

void cShockSaveBndPanel::ComputeSlot(int which)
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

void cShockSaveBndPanel::DoFileOp(int which)
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
   g_pInputBinder->SetContext (HK_GAME_MODE,FALSE);
   g_pInputBinder->SaveBndFile ((char *)fname, header);
   //plop in input variable control settings
   BndAppendControls ((char *)fname);

   g_pInputBinder->SetContext (HK_GAME2_MODE,FALSE);
   g_pInputBinder->SaveBndFile (SlotFileName2(which), header);

   g_pInputBinder->SetContext (old_context,FALSE);

   MessageNamed("success"); 
}

//----------------------------------------

void cShockSaveBndPanel::BndAppendControls (char *fname)
{
   FILE *fp = fopen (fname, "ab");

   if (fp)
   {
      fprintf (fp, "\n");

      char *controls[] = {"bow_zoom", "lookspring", "mouse_invert", "freelook", "mouse_sensitivity", "\0"};
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

void cShockSaveBndPanel::OnDeselect(int which)
{
   sSlot& slot = mpSlots[which]; 
   LGadTextBoxClrFlag(&slot.box,TEXTBOX_EDIT_EDITABLE); 
   LGadUnfocusTextBox(&slot.box); 
   slot.flags &= ~kNoCompute; 
}

//------------------------------------------------------------

void cShockSaveBndPanel::OnSelect(int which)
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

sDarkPanelDesc cShockSaveBndPanel::gDesc = 
{
   "bndsave",
   cShockSaveBndPanel::kNumButts,
   cShockSaveBndPanel::kNumRects,
   cShockSaveBndPanel::kNumButts,
   savebnd_strings,
}; 

//----------------------------------------

static cShockSaveBndPanel* gpSaveBnd = NULL; 

void SwitchToShockSaveBndMode(BOOL push)
{
   if (!gpSaveBnd)
      gpSaveBnd = new cShockSaveBndPanel; 

   if (gpSaveBnd)
   {
      cAutoIPtr<IPanelMode> mode = gpSaveBnd->GetPanelMode(); 
      mode->Switch(push ? kLoopModePush : kLoopModeSwitch); 
   }
}

void ShockInitBindSaveLoad(void)
{
}

void ShockTermBindSaveLoad(void)
{
   delete gpSaveBnd; 
   delete gpLoadBnd; 
   gpSaveBnd = NULL; 
   gpLoadBnd = NULL; 
}

