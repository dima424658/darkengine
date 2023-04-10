/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/buttpanl.h,v 1.5 1998/10/30 02:03:02 mahk Exp $
#pragma once  
#ifndef __BUTTPANL_H
#define __BUTTPANL_H
#include <comtools.h>
#include <loopapi.h>
#include <panlmode.h>
#include <gadblist.h>
#include <dynarray.h>
#include <cursors.h>
#include <imgsrc.h>

////////////////////////////////////////////////////////////
// SIMPLE BUTTON PANEL MODE CLASS
//
// This simple class uses panelmode to implement 
// a modal, full-screen button panel.  
//
// It's not terribly abstract, but it gets the job done. 
//

typedef cCTUnaggregated<ILoopClient, &IID_ILoopClient, kCTU_Default> cBaseLoopClient; 

typedef cDynArray<Rect> cRectArray; 
typedef cDynArray<DrawElement> cElemArray; 

class cModalButtonPanel 
{
protected:

   //------------------------------------------------------------
   //  VIRTUAL FUNCTIONS FOR DESCRIBING YOUR BUTTON PANEL
   //

   // Init/Term UI (optional, but way recommended) 
   virtual void InitUI() { }; 
   virtual void TermUI() { }; 
   
   // These will only get called inside init/term ui, we promise

   // How many buttons do I have?
   virtual int NumButtons() = 0; 
   // my button rectangles
   virtual cRectArray& Rects() = 0; 
   // my draw elements  
   virtual cElemArray& Elems() = 0; 

   // My cursor
   virtual IImageSource* CursorImage() = 0; 
   // Its palette
   virtual IDataSource*  CursorPalette() { return NULL; }; 

   // Handle UI events
   virtual void OnButtonList(ushort action, int button) = 0; 
   
   // Handle loop events (optional)
   // Can get called outside init/term ui 
   virtual void OnLoopMsg(eLoopMessage, tLoopMessageData ) {} ; 



public:

   //------------------------------------------------------------
   // CLIENT API
   //

   //
   // Constructor.  The descriptors specify *default* values.  
   // The button panel will override them when necessary 
   // 
   cModalButtonPanel(sPanelModeDesc* pmdesc, sLoopClientDesc* lcdesc);
   ~cModalButtonPanel(); 

   //
   // Spit out a panel mode, so that the client can call switch or whatever
   //

   IPanelMode* GetPanelMode() { mpPanelMode->AddRef(); return mpPanelMode; }; 

   //
   // Get the button list
   // 

   LGadButtonList& GetButtons() { return mpClient->mButtonList; }; 
   
   //------------------------------------------------------------
   // THE GRITTY DETAILS
   //

protected:
   IPanelMode* mpPanelMode;
   ulong mFactoryHandle; 

   //
   // LOOP CLIENT, ACTUALLY OWNS THE BUTTON LIST
   //

   class cClient : public cBaseLoopClient
   {
   public:
      cClient(const sLoopClientDesc* desc, cModalButtonPanel* panel); 
      ~cClient(); 

      STDMETHOD_(short,GetVersion)() { return kVerLoopClient; }; 
      STDMETHOD_(const sLoopClientDesc*, GetDescription)() { return mpDesc; }; 
      STDMETHOD_(eLoopMessageResult, ReceiveMessage)(eLoopMessage, tLoopMessageData hData);

      static bool BListCB(ushort action, int button, void* data, LGadBox* blist); 
      void SetupUI(); 
      void CleanupUI();
      static ILoopClient* LGAPI FactoryFunc(const sLoopClientDesc*, tLoopClientData); 

      const sLoopClientDesc* mpDesc; 
      cModalButtonPanel* mpPanel; 
      LGadButtonList mButtonList;
      IImageSource* mpCursorImg; 
      Cursor mCursor; 
      int mPaletteIdx; 

      BOOL mActive; 
   };
   
   friend class cClient; 

   cClient* mpClient; 


}; 

#endif // __BUTTPANL_H




