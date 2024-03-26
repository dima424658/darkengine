/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/filepanl.cpp,v 1.1 1999/02/22 13:06:01 mahk Exp $

#include <appagg.h>
#include <filepanl.h>
#include <gadtext.h>
#include <drkpanl.h>
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

#include <dbmem.h>

////////////////////////////////////////////////////////////
//
// cFilePanel implementation 
//
//


void cFilePanel::LoadStyle(const char* prefix, guiStyle* style)
{
   guiStyle* cur_style = GetCurrentStyle(); 
   memset(style,0,sizeof(*style)); 
   if (cur_style)
      memcpy(style->colors,cur_style->colors,sizeof(style->colors)); 
   uiGameLoadStyle(prefix,style,mResPath); 
}


//----------------------------------------

void cFilePanel::InitUI()
{
   cDarkPanel::InitUI(); 
   cUiAnim::InitUI();

   mTotalFailure = FALSE; 
   mpSlots = new sSlot[kNumSlots]; 
   memset(mpSlots,0,sizeof(sSlot)*kNumSlots); 

   LoadStyle("file_slot_",&mSlotStyle); 
   LoadStyle("file_msg_",&mMessageStyle); 


   Rect files = mRects[(int)kFiles]; 
   for (int i = 0; i < kNumSlots; i++)
   {
      sSlot& slot = mpSlots[i]; 
      slot.us = this; 

      LGadTextBoxDesc desc = 
      { {0 }, slot.buf, sizeof(slot.buf), 
        0,
        textgadg_cb,
        &slot, 
        &mSlotStyle,
      };  

      Rect& r = desc.bounds; 
      r = files; 
      // interpolate top & bottom of rect
      r.ul.y = (files.ul.y*(kNumSlots-i) + files.lr.y*i)/kNumSlots; 
      r.lr.y = (files.ul.y*(kNumSlots - (i+1)) + files.lr.y*(i+1))/kNumSlots;


      LGadCreateTextBoxDesc(&slot.box,LGadCurrentRoot(),&desc); 
      LGadTextBoxClrFlag(&slot.box,TEXTBOX_EDIT_BRANDNEW|TEXTBOX_EDIT_EDITABLE);

      int cookie; 
      uiInstallRegionHandler(LGadBoxRegion(&slot.box),UI_EVENT_MOUSE,textbox_cb,&slot,&cookie); 

      ComputeSlot(i); 
   }

   EnableFileOp(FALSE); 
   mCurSlot = -1; 
   mFileOp = kFileOpNone; 
}

//----------------------------------------

void cFilePanel::TermUI()
{
   mCurSlot = -1; 
   for (int i = 0; i < kNumSlots; i++)
      LGadDestroyTextBox(&mpSlots[i].box); 

   uiGameUnloadStyle(&mSlotStyle); 
   uiGameUnloadStyle(&mMessageStyle); 

   cUiAnim::TermUI();
   cDarkPanel::TermUI(); 
}

//----------------------------------------

BOOL cFilePanel::textbox_cb(uiEvent* ev, Region* r, void* user_data)
{
   sSlot* slot = (sSlot*)user_data; 
   slot->us->OnTextBox(slot,ev); 
   return FALSE; 
}


bool cFilePanel::textgadg_cb(LGadTextBox* box, LGadTextBoxEvent event, int eventdata, void *user_data)
{
   sSlot* slot = (sSlot*)user_data; 
   slot->us->OnTextGadg(slot,event,eventdata); 
   return FALSE; 
}



//----------------------------------------

void cFilePanel::OnTextBox(sSlot* slot, uiEvent* ev)
{
   // never trusted pointer subtraction 
   int slotnum = ((char*)slot - (char*)mpSlots)/sizeof(sSlot);

   if (ev->type == UI_EVENT_MOUSE)
      switch (ev->subtype)
      {  
         case MOUSE_LDOWN:
            SelectSlot(slotnum); 
            break; 
      }
}

//----------------------------------------

void cFilePanel::OnTextGadg(sSlot* slot, LGadTextBoxEvent event, int eventdata)
{

   if (event == TEXTGADG_SPECKEY)
   {
      switch(eventdata & ~KB_FLAG_DOWN)
      {
         case KEY_ENTER:
         {
            LGadTextBoxClrFlag(&slot->box,TEXTBOX_EDIT_EDITABLE|TEXTBOX_EDIT_BRANDNEW); 
            LGadUnfocusTextBox(&slot->box); 
            mFileOp = mCurSlot; 
         }
         break; 

         case KEY_ESC:
         {
            int slotnum = ((char*)slot - (char*)mpSlots)/sizeof(sSlot);

            if (mCurSlot == slotnum)
               mFileOp = kFileOpExit; 
         }
         break; 
      }
   }
}


//----------------------------------------

void cFilePanel::SelectSlot(int which)
{
   if (which != mCurSlot)
   {
      int old = mCurSlot; 
      mCurSlot = which; 
      if (old >= 0)
      {
         // reload text from file, just in case
         LGadTextBoxFlags(&mpSlots[old].box) &= ~TEXTBOX_BORDER_FLAG; 
         OnDeselect(old); 
         ComputeSlot(old);
         RedrawSlot(old); 
      }

      if (which >= 0)
      {
         EnableFileOp(TRUE); 
         OnSelect(which); 
         sSlot& slot = mpSlots[which]; 
         LGadTextBoxFlags(&slot.box) |= TEXTBOX_BORDER_FLAG; 
         RedrawSlot(which);
      }
      else
         EnableFileOp(FALSE); 
   }
}
//----------------------------------------

void cFilePanel::EnableFileOp(BOOL enabled)
{
   if (enabled && !mTotalFailure)
      mElems[(int)kDoFile].fcolor = 0;
   else
      mElems[(int)kDoFile].fcolor = guiStyleGetColor(NULL,StyleColorDim); 

   region_expose(LGadBoxRegion(LGadCurrentRoot()),&mRects[(int)kDoFile]); 
}

//----------------------------------------

void cFilePanel::RedrawSlot(int which)
{
   sSlot& slot = mpSlots[which]; 
   region_expose(LGadBoxRegion(&slot.box),LGadBoxRect(&slot.box));
}

//----------------------------------------

void cFilePanel::RedrawDisplay()
{
   for (int i = 0; i < kNumSlots; i++)
      RedrawSlot(i); 

   // @TODO: update screen shot 
}

//----------------------------------------

void cFilePanel::OnButtonList(ushort action, int button)
{
   if (action & BUTTONGADG_LCLICK)
      switch(button)
      {
         case kDoFile:
            if (mCurSlot >= 0)
               mFileOp = mCurSlot; 
            break; 

         case kDone:
            mFileOp = kFileOpExit; 
            break; 
      }
}

//----------------------------------------

void cFilePanel::OnLoopMsg(eLoopMessage msg, tLoopMessageData data)
{
   cUiAnim::OnLoopMsg(msg,data);
   cDarkPanel::OnLoopMsg(msg,data); 
   switch(msg)
   {
      case kMsgNormalFrame:
         switch (mFileOp)
         {
            case kFileOpNone:
               break; 
            case kFileOpExit:
               if (mTotalFailure)
                  TotalFailure(); 
               else
                  mpPanelMode->Exit(); 
               break; 
            case kFileOpDeselect:
               SelectSlot(-1); 
               break; 
            default:
               if (mFileOp >= 0 && !mTotalFailure)
                  DoFileOp(mFileOp); 
               break;
         }
         mFileOp = kFileOpNone; 
         break; 
   }
}

//----------------------------------------

void cFilePanel::Message(const char* msg)
{
   cStr str = msg;  // copy the string 
   char* s = (char*)(const char*)str;  // get a mutable pointer

   Rect& r = mRects[(int)kMessage]; 

   GUIcompose c; 

   GUIsetup(&c,&r,GUIcomposeFlags(ComposeFlagClear|ComposeFlagRead),GUI_CANV_ANY); 

   guiStyleSetupFont(&mMessageStyle,StyleFontNormal); 
   gr_set_fcolor(guiStyleGetColor(&mMessageStyle,StyleColorText)); 

   gr_font_string_wrap(gr_get_font(),s,RectWidth(&r)); 

   short w,h; 
   gr_string_size(s,&w,&h); 

   gr_string(s,0,(RectHeight(&r) - h)/2); 

   guiStyleCleanupFont(&mMessageStyle,StyleFontNormal) ;

   GUIdone(&c); 
   ScrnForceUpdateRect(&r); 
}

//----------------------------------------

void cFilePanel::MessageNamed(const char* name)
{
   Message(FetchUIString(panel_name,name,mResPath)); 
}

