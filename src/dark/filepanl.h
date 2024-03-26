/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#include <drkpanl.h>
#include <uianim.h>
#include <gadtext.h>
#include <guistyle.h>

//------------------------------------------------------------
// Base panel class
//

class cFilePanel: public cDarkPanel, public cUiAnim
{
public:
   cFilePanel(const sDarkPanelDesc* desc)
      : cDarkPanel(desc), cUiAnim(desc->panel_name,INTERFACE_PATH)
   {
   }

   ~cFilePanel()
   {
   }

   enum eRects
   {
      kDoFile, // Save or Load
      kDone,   // done
      kNumButts, 
      
      kMessage = kNumButts,
      kFiles,

      kNumRects
   };

   enum 
   {
      kNumSlots = 16, 
      kQuickSlot = 15,
   }; 

protected:

   enum eSlotFlags
   {
      kUnused = 1 << 0,   // No file there
      kNoCompute = 1 << 1, // Don't reload file desc
   }; 

   struct sSlot
   {
      LGadTextBox box;
      char buf[80]; 
      ulong flags; 
      cFilePanel* us; 
   }; 

   enum 
   {
      kFileOpNone    = -1,
      kFileOpExit    = -2,
      kFileOpDeselect = -3,
   }; 
      

   //
   // Helpers
   //
   void SelectSlot(int which); 
   void DeselectSlot(); 
   void RedrawSlot(int which); 
   void EnableFileOp(BOOL enabled); 
   void LoadStyle(const char* prefix, guiStyle* style); 
   void Message(const char* msg); 
   void MessageNamed(const char* name); 

   static BOOL textbox_cb(uiEvent* e, Region* r, void* data); 
   static bool textgadg_cb(LGadTextBox* box, LGadTextBoxEvent event, int eventdata, void *data);

   void OnTextGadg(sSlot* slot, LGadTextBoxEvent event, int eventdata); 
   void OnTextBox(sSlot* slot, uiEvent* e); 

   //
   // Overrides   
   //
   void RedrawDisplay(); 
   void InitUI(); 
   void TermUI();
   void OnButtonList(ushort action, int button); 
   void OnLoopMsg(eLoopMessage msg, tLoopMessageData ); 
   void OnEscapeKey()
   {
      mFileOp = kFileOpExit; 
   }

   //
   // Overridables
   //

   virtual const char* SlotFileName(int which) = 0 ; 
   virtual void ComputeSlot(int which) = 0; 

   virtual void DoFileOp(int which) = 0; 
   virtual void OnSelect(int which) {}; 
   virtual void OnDeselect(int which) {}; 

   virtual void TotalFailure() = 0; 

   // array of slots 
   sSlot* mpSlots; 

   // styles
   guiStyle mSlotStyle; 
   guiStyle mMessageStyle; 

   int mCurSlot; 
   int mFileOp; 
   BOOL mTotalFailure; 
};
