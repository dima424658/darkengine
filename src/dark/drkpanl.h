/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkpanl.h,v 1.8 1999/11/02 15:55:20 BODISAFA Exp $
#pragma once  
#ifndef __DRKPANL_H
#define __DRKPANL_H

#include <buttpanl.h>
#include <drawelem.h>
#include <drkuires.h>
#include <str.h>
#include <resapi.h>
#include <guistyle.h>

////////////////////////////////////////////////////////////
// REFINED BUTTON-PANEL FOR DARK 
//

struct sDarkPanelDesc
{
   const char* panel_name; 
   int num_butts;  // how many buttons
   int num_rects;  // how many total rects
   int num_string_butts;  // how many buttons have strings
   const char** string_names;  
   const char* string_font; 
   ulong flagmask;
   int   panelsndtype;
   const GUID* loop_id; 
};

class cDarkPanel : public cModalButtonPanel, public sDarkPanelDesc
{
public:

   static sLoopClientDesc Descriptor; 

   cDarkPanel(const sDarkPanelDesc* desc, const char* respath = INTERFACE_PATH);
   ~cDarkPanel();

   // set the resource path
   void SetResPath(const char* path);

protected:  
   static BOOL region_cb(uiEvent *ev, Region *reg, void *data);
   //
   // Modal button panel ops
   //

   void InitUI(); 
   void TermUI(); 

   int NumButtons()
   {
      return num_butts;
   }

   cRectArray& Rects()
   {  
      return mRects; 
   }

   cElemArray& Elems()
   {  
      return mElems; 
   }

   IImageSource* CursorImage()
   {
      return FetchUIImage("cursor"); 
   }

   IDataSource* CursorPalette();

   static void TextDrawCB(DrawElement* drawme, DrawElemState state); 
   void OnTextDraw(int butt, DrawElement* drawme, DrawElemState state);
   void OnLoopMsg(eLoopMessage, tLoopMessageData ); 

   //
   // Overridables
   //

   // called at mode start, and whenever display needs to be fully redrawn.  
   virtual void RedrawDisplay() { }; 

   // What to do on escape, base implemenation exits. 
   virtual void OnEscapeKey(void);

   //
   // Members
   //

   struct sData2
   {
      cDarkPanel* us; 
      int i; 
   }; 

   typedef cDynClassArray<cStr> cStringArray; 
   typedef cDynArray<sData2> cData2Array; 


   cRectArray mRects; 
   cElemArray mElems; 
   cStringArray mStrings; 
   IDataSource* mpFontRes; 
   cData2Array mElemData;
   cStr mPaletteRes; 
   cStr mResPath; 
   guiStyle mStyle; 
};

// TRUE:  We're headed into Thief 2's game mode.
EXTERN BOOL DarkIsToGameModeGUID(REFGUID rguid);
#endif // __DRKPANL_H
