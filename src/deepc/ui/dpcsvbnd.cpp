/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <filepanl.h>
#include <appagg.h>
#include <gen_bind.h>
#include <direct.h>
#include <contexts.h>
#include <dpcsvbnd.h>

#include <dbmem.h>

#define SAVE_PATH "binds"

//============================================================
// Load Bind File Panel
//

class cDPCLoadBndPanel: public cFilePanel
{

public:
   cDPCLoadBndPanel() : cFilePanel(&gDesc) {}; 

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


const char* cDPCLoadBndPanel::SlotFileName(int which)
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

void cDPCLoadBndPanel::ComputeSlot(int which)
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

void cDPCLoadBndPanel::DoFileOp(int which)
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

void cDPCLoadBndPanel::OnSelect(int which)
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

sDarkPanelDesc cDPCLoadBndPanel::gDesc = 
{
   "bndload",
   cDPCLoadBndPanel::kNumButts,
   cDPCLoadBndPanel::kNumRects,
   cDPCLoadBndPanel::kNumButts,
   loadbnd_strings,
}; 

//----------------------------------------

static cDPCLoadBndPanel* gpLoadBnd = NULL; 

void SwitchToDPCLoadBndMode(BOOL push)
{
   if (!gpLoadBnd)
      gpLoadBnd = new cDPCLoadBndPanel; 
   if (gpLoadBnd)
   {
      cAutoIPtr<IPanelMode> mode = gpLoadBnd->GetPanelMode(); 
      mode->Switch(push ? kLoopModePush : kLoopModeSwitch); 
   }
}


//============================================================
// Save Bind File Panel
//

class cDPCSaveBndPanel: public cFilePanel
{

public:
   cDPCSaveBndPanel() : cFilePanel(&gDesc) {}; 

protected:
   static sDarkPanelDesc gDesc; 

   void InitUI()
   {
      mkdir(SAVE_PATH); 
      cFilePanel::InitUI(); 
      MessageNamed("initial"); 
   }

   const char* cDPCSaveBndPanel::SlotFileName(int which);
   void cDPCSaveBndPanel::ComputeSlot(int which);
   void DoFileOp(int which);
   void BndAppendControls (char *fname);
   void OnSelect(int which); 
   void OnDeselect(int which); 

   void TotalFailure() { }; 

}; 

//----------------------------------------

const char* cDPCSaveBndPanel::SlotFileName(int which)
{
   return slot_file_name(which); 
}

//----------------------------------------

void cDPCSaveBndPanel::ComputeSlot(int which)
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

void cDPCSaveBndPanel::DoFileOp(int which)
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

void cDPCSaveBndPanel::BndAppendControls (char *fname)
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

void cDPCSaveBndPanel::OnDeselect(int which)
{
   sSlot& slot = mpSlots[which]; 
   LGadTextBoxClrFlag(&slot.box,TEXTBOX_EDIT_EDITABLE); 
   LGadUnfocusTextBox(&slot.box); 
   slot.flags &= ~kNoCompute; 
}

//------------------------------------------------------------

void cDPCSaveBndPanel::OnSelect(int which)
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

sDarkPanelDesc cDPCSaveBndPanel::gDesc = 
{
   "bndsave",
   cDPCSaveBndPanel::kNumButts,
   cDPCSaveBndPanel::kNumRects,
   cDPCSaveBndPanel::kNumButts,
   savebnd_strings,
}; 

//----------------------------------------

static cDPCSaveBndPanel* gpSaveBnd = NULL; 

void SwitchToDPCSaveBndMode(BOOL push)
{
   if (!gpSaveBnd)
      gpSaveBnd = new cDPCSaveBndPanel; 

   if (gpSaveBnd)
   {
      cAutoIPtr<IPanelMode> mode = gpSaveBnd->GetPanelMode(); 
      mode->Switch(push ? kLoopModePush : kLoopModeSwitch); 
   }
}

void DPCInitBindSaveLoad(void)
{
}

void DPCTermBindSaveLoad(void)
{
   delete gpSaveBnd; 
   delete gpLoadBnd; 
   gpSaveBnd = NULL; 
   gpLoadBnd = NULL; 
}

