/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkldout.cpp,v 1.22 2000/02/28 14:17:04 kevin Exp $
#include <comtools.h>
#include <appagg.h>
#include <dispapi.h>

#include <drkldout.h>
#include <drkpanl.h>
#include <objtype.h>
#include <drkdebrf.h>
#include <gadblist.h>
#include <contain.h>
#include <appagg.h>
#include <inv_rend.h>
#include <gcompose.h>
#include <guistyle.h>
#include <playrobj.h>
#include <drkinvui.h>
#include <drkinvpr.h>
#include <iobjsys.h>
#include <objdef.h>
#include <propbase.h>
#include <questapi.h>
#include <gamestr.h>
#include <drkmislp.h>
#include <uigame.h>
#include <appsfx.h>    // horrible hack.. try and stop me
#include <rendprop.h>

#include <playtest.h>
#include <config.h>
#include <metasnd.h>
#include <drkmenu.h>

#include <command.h>

// Include these last
#include <dbmem.h>

static IBoolProperty* gpStoreProp = NULL; 
static IIntProperty* gpPriceProp = NULL; 
static IStringProperty* gpDescProp = NULL; 


////////////////////////////////////////////////////////////
// LOADOUT PANEL

//
// There are 3 regions
//

enum 
{ 
   kInvRegion,  // Inventory
   kCartRegion, // Shopping cart
   kStoreRegion,  // Store 
   kNumRegions, 

   kSlotsPerRegion = 10,
   kSlotRows = 5,
   kSlotCols = 2, 
};

//
// Here's how to figure out where an item is on screen
//  

static Rect slot_rect_rowcol(const Rect& src, int col, int row)
{
   Rect r; 
   r.ul.x = src.ul.x+RectWidth(&src)*col/kSlotCols; 
   r.ul.y = src.ul.y+RectHeight(&src)*row/kSlotRows; 
   r.lr.x = src.ul.x+RectWidth(&src)*(col+1)/kSlotCols; 
   r.lr.y = src.ul.y+RectHeight(&src)*(row+1)/kSlotRows; 
   return r; 
}

static Rect slot_rect_idx(const Rect& src, int idx)
{
   return slot_rect_rowcol(src,idx%kSlotCols, idx/kSlotCols); 
}


//
// Default theta of rendered items
//

#define ITEM_THETA (FIXANG_PI/2)

//------------------------------------------------------------
// PANEL CLASS
//

class cLoadout: public cDarkPanel
{
public:
   cLoadout(); 
   ~cLoadout(); 

   // Rects, before we cut each item area into 8    
   enum eRects
   {
      // String buttons 
      kClear, 
      kPlay,
      kBriefing,
      kGoals,
      kNumStringButts,

      kBuy = kNumStringButts,

      kNumButts, 

   // Now nonbuttons

      kInvRect  = kNumButts,         // inventory
      kCartRect,                     // shopping cart                   
      kStoreRect,                    // store
      kInvTitle,
      kCartTitle,
      kStoreTitle, 

      kCashRect,
      kItemRect, // item display 
      kTextRect, // item text

      kNumRects

   }; 


protected:
   // panel overrides 
   void RedrawDisplay(); 
   void OnLoopMsg(eLoopMessage msg, tLoopMessageData data); 
   void InitUI(); 
   void TermUI();
   void OnButtonList(ushort action, int button);
   void OnEscapeKey() 
   { 
      SwitchToSimMenuMode(TRUE); 
   }; 

   // other funcs 
   void DrawCash(); 
   void DrawDescription(ObjID obj); 
   void DrawItemRegion(int i); 
   void DrawItemTitle(int i); 
   void DrawSelection(); 

   // select an object 
   void Select(int region, int slot); 

   void ComputeContainers(); 
   void BuildContainers(); 
   void DestroyContainers(); 

   // Wrapper around AnnotateObj
   void Annotate(ObjID obj, int w, int h); 
   void draw_obj(ObjID obj, invRendState* state, Rect& r); 


   // +1 for buying, -1 for selling 
   BOOL MoveMerchandise(ObjID merch, int dir); 

   void ClearCart(); 
   void OnItemButton(ushort action, int region, int slot); 

   void OnFrame(); 

   // types 
   typedef cDynArray<ObjID> cObjArray; 
   struct sButtList : public LGadButtonList 
   {
      int region; 
      cLoadout* loadout; 
      cRectArray mRects; 
      cElemArray mElems; 

      LGadButtonList& AsBList() { return *this;}; 

      static bool BListCB(ushort action, int button, void* data, LGadBox* blist);
      
      void Build(cLoadout* ld, int region, const Rect& bounds); 
      void Destroy(); 
   };

   friend struct sButtList; 


   // state vars
   cObjArray mObjs[kNumRegions];  
   sButtList mButtLists[kNumRegions]; 

   int mRegionSel;
   int mSlotSel; 
   invRendState* mpSelState; 
   ObjID mContainers[kNumRegions]; 
   int mCash; // how much money you got...
   BOOL drewblank;  //if the last description we drew was the blank one.

   // separate copy of the "clear" string
   cStr mClearString;

   // Stuff for the changing "buy" button 
   DrawElement mBuyElems[kNumRegions]; 
   enum { kNumBuyImages = 4 } ; 
   IImageSource* mBuyImages[kNumBuyImages];  
   // background image for buy button 
   grs_bitmap mBuyBG; 
   IImageSource* mpBG;

   // Helpful stuff 
   IContainSys* mpContainSys; 
   IIntProperty* mpStackCount; 

   guiStyle mTextStyle; 

}; 

void cLoadout::RedrawDisplay()
{
   ComputeContainers(); 
   for (int i = 0; i < kNumRegions; i++)
   {
      DrawItemRegion(i); 
      DrawItemTitle(i); 
   }
   DrawCash(); 
}

//------------------------------------------------------------
// Set up an item button list
//

void cLoadout::sButtList::Build(cLoadout* ld, int reg, const Rect& bounds)
{
   loadout = ld; 
   region = reg; 
   int n = kSlotsPerRegion; 
   mRects.SetSize(n); 
   mElems.SetSize(n); 
   for (int i = 0; i < n; i++)
   {
      mRects[i] = slot_rect_idx(bounds,i); 
      memset(&mElems[i],0,sizeof(mElems[i])); 
      // for now, items aren't drawn through the BL.  
      // If we get ambitious, we'll do that. 
      mElems[i].draw_type = DRAWTYPE_NONE; 
      mElems[i].draw_flags = INTERNAL(DRAWFLAG_INT_TRANSP); 
   }

   LGadButtonListDesc desc = { n, mRects.AsPointer(), mElems.AsPointer(), BListCB, 0, 0}; 

   memset(this,0,sizeof(LGadButtonList)); 
   LGadCreateButtonListDesc(this,LGadCurrentRoot(),&desc); 
}

void cLoadout::sButtList::Destroy()
{
   LGadDestroyButtonList(this); 
   mRects.SetSize(0); 
   mElems.SetSize(0); 
}

//------------------------------------------------------------
// INIT/TERM UI
//

void cLoadout::InitUI()
{
   cDarkPanel::InitUI(); 
   memset(&mTextStyle,0,sizeof(mTextStyle)); 
   guiStyle* the_style = GetCurrentStyle(); 
   if (the_style)
      memcpy(mTextStyle.colors,the_style->colors,sizeof(mTextStyle.colors)); 

   uiGameLoadStyle("loadout_text_",&mTextStyle,mResPath); 

   // Build the button lists for the item regions 
   for (int i = 0; i < kNumRegions; i++)
      mButtLists[i].Build(this,i,mRects[kInvRect+i]); 
   mRegionSel = -1; 
   mSlotSel = -1; 
   drewblank = FALSE;

   // Initialize the "buy" button 
   for (int i = 0; i < kNumBuyImages; i++)
   {
      char buf[16]; 
      sprintf(buf,"arrow%03d",i); 
      IImageSource* src = FetchUIImage(buf); 
      mBuyImages[i] = src; 
   }

   memset(&mBuyElems[kInvRegion],0,sizeof(mBuyElems[kInvRegion])); 
   
   mBuyElems[kInvRegion].draw_type = DRAWTYPE_BITMAP; 
   mBuyElems[kInvRegion].draw_flags = INTERNAL(DRAWFLAG_INT_TRANSP); 

   // Make the sub-bitmap of the background that is the "buy" button 
   IImageSource* bg = mpPanelMode->GetImage(kPanelBG); 
   grs_bitmap* bm = (grs_bitmap*)bg->Lock(); 
   Rect& buyr = mRects[(int)kBuy];
   gr_init_sub_bitmap(bm,&mBuyBG, buyr.ul.x, buyr.ul.y, RectWidth(&buyr), RectHeight(&buyr));
   mBuyElems[kInvRegion].draw_data = &mBuyBG;  
   mpBG = bg; 
  

   // setup the elems for the other regions 
   int n = 0; 
   for (int i = 0; i < kNumRegions; i++)
      if (i != kInvRegion)
      {
         DrawElement& elem = mBuyElems[i];  
         memset(&elem,0,sizeof(elem));
         elem.draw_type = DRAWTYPE_BITMAP; 
         elem.draw_data  = mBuyImages[n++]->Lock(); 
         elem.draw_data2 = mBuyImages[n++]->Lock(); 
         elem.draw_flags = INTERNAL(DRAWFLAG_INT_TRANSP); 
      }

   mElems[(int)kBuy] = mBuyElems[0]; 

   // setup the "clear" button 
   mClearString = mStrings[(int)kClear]; 

}

void cLoadout::TermUI()
{
   // could probably just play it here
   
   // Build the button lists for the item regions 
   for (int i = 0; i < kNumRegions; i++)
   {
      mButtLists[i].Destroy(); 
      mObjs[i].SetSize(0); 
   }
   if (mpSelState)
   {
      invRendFreeState(mpSelState); 
      mpSelState = NULL; 
   }

   for (int i = 0; i < kNumBuyImages; i++)
   {
      mBuyImages[i]->Unlock(); 
      SafeRelease(mBuyImages[i]); 
   }

   gr_close_bitmap(&mBuyBG); 
   mpBG->Unlock(); 
   SafeRelease(mpBG); 
   
   uiGameUnloadStyle(&mTextStyle); 

   cDarkPanel::TermUI();
}

//------------------------------------------------------------
// Fill the object vectors
//
void cLoadout::ComputeContainers()
{
   for (int r = 0; r < kNumRegions; r++)
   {
      cObjArray& objs = mObjs[r]; 
      objs.SetSize(kSlotsPerRegion); 

      int i = 0; 
      sContainIter* iter = mpContainSys->IterStart(mContainers[r]); 
      for (;!iter->finished && i < objs.Size() ; mpContainSys->IterNext(iter), i++)
      {
         objs[i] = iter->containee; 
      }
      mpContainSys->IterEnd(iter); 
      for (; i < kSlotsPerRegion; i++)
         objs[i] = OBJ_NULL; 
   }
}

#define CART_NAME "@SHOP@CART@"

void cLoadout::BuildContainers()
{
   Assert_(PlayerObjectExists());

   // Create the store and the cart objects
   AutoAppIPtr_(ObjectSystem,pObjSys); 
   mContainers[kInvRegion] = PlayerObject(); 
   mContainers[kStoreRegion] = pObjSys->Create(ROOT_ARCHETYPE, kObjectConcrete);

   // Look for the cart
   ObjID cart = pObjSys->GetObjectNamed(CART_NAME); 
   if (cart == OBJ_NULL)
   {
      cart = pObjSys->Create(ROOT_ARCHETYPE, kObjectConcrete); 
      pObjSys->NameObject(cart,CART_NAME); 
   }
   mContainers[kCartRegion] = cart; 

   ObjSetHasRefs(mContainers[kStoreRegion],FALSE); 
   ObjSetHasRefs(mContainers[kCartRegion],FALSE); 

   // Force the player's inventory to recombine by moving it into the store and back
   mpContainSys->MoveAllContents(mContainers[kStoreRegion],PlayerObject(),CTF_COMBINE); 
   mpContainSys->MoveAllContents(PlayerObject(),mContainers[kStoreRegion],CTF_COMBINE); 


   // Fill the store
   pObjSys->Lock(); 
   sPropertyObjIter iter;
   ObjID obj; 
   BOOL in_store; 
   gpStoreProp->IterStart(&iter); 
   while (gpStoreProp->IterNextValue(&iter,&obj,&in_store))
      if (OBJ_IS_CONCRETE(obj) && in_store)
      {
         mpContainSys->MoveAllContents(mContainers[kStoreRegion],obj,CTF_COMBINE); 
         pObjSys->Destroy(obj); 
      }
   gpStoreProp->IterStop(&iter); 
   pObjSys->Unlock(); 

   gpStoreProp->Set(mContainers[kStoreRegion],TRUE); 

   // Set up your account 
   AutoAppIPtr(QuestData); 
   mCash = pQuestData->Get(MONEY_QVAR); 
   int bonus = 0; 
   config_get_int("cash_bonus",&bonus);
   mCash += bonus; 
}

void cLoadout::DestroyContainers()
{
   // You bought 'em, you keep 'em. 
   mpContainSys->MoveAllContents(PlayerObject(),mContainers[kCartRegion],CTF_COMBINE); 

   AutoAppIPtr_(ObjectSystem,pObjSys); 
   pObjSys->Destroy(mContainers[kCartRegion]); 

   AutoAppIPtr(QuestData); 
   pQuestData->Set(MONEY_QVAR,mCash); 
}


void cLoadout::Annotate(ObjID obj, int w, int h)
{
   // change styles 
   guiStyle* temp = GetCurrentStyle() ;
   SetCurrentStyle(&mTextStyle); 
   AnnotateObject(obj,w,h); 
   SetCurrentStyle(temp); 
}

//------------------------------------------------------------
// Draw a single object region 
//

void cLoadout::DrawItemRegion(int r)
{
   cObjArray& objs = mObjs[r]; 
   sButtList& blist= mButtLists[r]; 
   for (int i = 0; i < kSlotsPerRegion; i++)
   {
      GUIcompose c; 
      GUIsetup(&c,&blist.mRects[i],(GUIcomposeFlags)(ComposeFlagRead|ComposeFlagClear),GUI_CANV_ANY); 

      if (i < objs.Size() && objs[i] != OBJ_NULL)
      {
         invRendState* state = invRendBuildState(0,objs[i], NULL, grd_canvas); 
         sInvRendView view; 
         invRendGetView(state,&view); 
         view.off_ang.tz = ITEM_THETA; 
         invRendSetView(state,&view); 
         invRendDrawUpdate(state); 
         invRendFreeState(state); 
                                   
         Annotate(objs[i],grd_canvas->bm.w,grd_canvas->bm.h); 
      }
      GUIdone(&c); 

   }

   if (r == kCartRegion)
   {
      // disable the "clear" button 
      if (mObjs[kCartRegion][0] == OBJ_NULL)
      {
         mStrings[(int)kClear] = ""; 
         GUIErase(&mRects[(int)kClear]); 
      }
      else
      {
         mStrings[(int)kClear] = mClearString; 
         region_expose(LGadBoxRegion(LGadCurrentRoot()),&mRects[(int)kClear]);      
      }


   }
          
}

//------------------------------------------------------------
// Draw the title for an item
//

void cLoadout::DrawItemTitle(int reg)
{
   Rect r = mRects[kInvTitle+reg]; 
   GUIcompose c; 
   GUIsetup(&c,&r,(GUIcomposeFlags)(ComposeFlagRead|ComposeFlagClear),GUI_CANV_ANY); 

   guiStyleSetupFont(NULL,StyleFontTitle); 
   grs_font* font = gr_get_font(); 

   gr_set_fcolor(guiStyleGetColor(NULL,StyleColorFG)); 

   char buf[32]; 
   sprintf(buf,"title_%d",reg); 

   cStr title = FetchUIString(panel_name,buf); 
   char* s = (char*)(const char*)title; 

   short w,h; 
   gr_string_size(s,&w,&h); 
   int x = (grd_canvas->bm.w - w)/2;
   int y = (grd_canvas->bm.h - h)/2; 

   gr_string(s,x,y); 

   guiStyleCleanupFont(NULL,StyleFontTitle);


   GUIdone(&c); 
          
}


//------------------------------------------------------------
// Redraw the selection
//
void cLoadout::draw_obj(ObjID obj, invRendState* state, Rect& r)
{
// We used to do this crap, but that results in horrible mouse flicker
//   GUIcompose c; 
//   GUIsetup(&c,&r,(GUIcomposeFlags)(ComposeFlagRead|ComposeFlagClear),GUI_CANV_ANY); 
//   uiHideMouse(&r);
// Instead, we (mostly) bypass the gadget system and do a UI composed blit.
   int w = RectWidth(&r);
   int h = RectHeight(&r);
   if ((w <= 0) || (h <= 0))
      return;

   grs_canvas *draw_canvas;
   grs_canvas *base_canvas = DefaultGUIcanvas ? DefaultGUIcanvas : grd_canvas;
   draw_canvas = gr_alloc_canvas(gr_bm_type_to_flat(base_canvas->bm.type), w, h);
   gr_push_canvas(draw_canvas);
   gr_clear(0); // @HACK: should really guiclear here to get the right background, but for loadout, 
                // its just black anyway.

   invRendSetCanvas(state,grd_canvas); 
   invRendDrawUpdate(state); 
   Annotate(obj,grd_canvas->bm.w,grd_canvas->bm.h); 
   gr_pop_canvas();

   AutoAppIPtr(DisplayDevice);
   pDisplayDevice->Lock();
   gr_push_canvas(base_canvas);
   uiComposedBlit(&draw_canvas->bm, r.ul);
   gr_pop_canvas();
   pDisplayDevice->Unlock();

   gr_free_canvas(draw_canvas);

//   uiShowMouse(&r);
//   GUIdone(&c); 
}

void cLoadout::DrawSelection()
{
   if (mRegionSel < 0)
   {
	   if (!drewblank)	     
		   DrawDescription(OBJ_NULL); //meaning, Draw the blank_desc      
	   return; 
   }

   cObjArray& objs = mObjs[mRegionSel]; 
   
   if (objs[mSlotSel] == OBJ_NULL)
   {
      mRegionSel = -1; 
	  if (!drewblank)
		  DrawDescription(OBJ_NULL);
      return; 
   }
   sButtList& blist= mButtLists[mRegionSel]; 


   if (!mpSelState)
   {
      mpSelState = invRendBuildState(INVREND_ROTATE, objs[mSlotSel], NULL, grd_canvas); 
      sInvRendView view; 
      invRendGetView(mpSelState,&view); 
      view.off_ang.tz = ITEM_THETA;  
      invRendSetView(mpSelState,&view); 

      // New selection, update the item and text display 
      draw_obj(objs[mSlotSel],mpSelState,mRects[(int)kItemRect]); 
      DrawDescription(objs[mSlotSel]); 
      
   }

   draw_obj(objs[mSlotSel],mpSelState,blist.mRects[mSlotSel]); 
   
}

//------------------------------------------------------------
// Draw Cash display 
//

void cLoadout::DrawCash()
{
   Rect r = mRects[(int)kCashRect]; 
   GUIcompose c; 
   GUIsetup(&c,&r,(GUIcomposeFlags)(ComposeFlagRead|ComposeFlagClear),GUI_CANV_ANY); 

   grs_font* font = (grs_font*)mpFontRes->Lock(); 
   gr_set_font(font); 

   gr_set_fcolor(guiStyleGetColor(NULL,StyleColorFG)); 

   cStr cash = FetchUIString(panel_name,"cash"); 

   char buf[32]; 
   sprintf(buf,"%s: %d",(const char*)cash,mCash); 
   short w,h; 
   gr_string_size(buf,&w,&h); 
   int x = (grd_canvas->bm.w - w)/2;
   int y = (grd_canvas->bm.h - h)/2; 

   gr_string(buf,x,y); 
   mpFontRes->Unlock(); 


   GUIdone(&c); 

}

//------------------------------------------------------------
// Draw Description
//

void cLoadout::DrawDescription(ObjID obj)
{
   Rect r = mRects[(int)kTextRect]; 
   GUIcompose c; 
   GUIsetup(&c,&r,(GUIcomposeFlags)(ComposeFlagRead|ComposeFlagClear),GUI_CANV_ANY); 

   drewblank = (obj == OBJ_NULL); //if OBJ_NULL, we are drawing blank desc, if not, we aren't.

   //if (obj != OBJ_NULL) Now we embrace OBJ_NULL, or, at least, display other text. (blank_desc)
   //AMSD
   {
      guiStyleSetupFont(&mTextStyle,StyleFontNormal); 
      grs_font* font = gr_get_font(); 

      gr_set_fcolor(guiStyleGetColor(&mTextStyle,StyleColorFG)); 

	  cStr str;

	  if (obj != OBJ_NULL)
	  {
		  AutoAppIPtr(GameStrings); 
		  str = pGameStrings->FetchObjString(obj, DESCRIPTION_TABLE); 
      }
	  else
	  {
		  str = FetchUIString(panel_name,"blank_desc"); 
	  }
		  
	  char* s = (char*)(const char*)str; 
      gr_font_string_wrap(font,s,grd_canvas->bm.w); 

      gr_string(s,0,0); 
      guiStyleCleanupFont(&mTextStyle,StyleFontNormal); 
   }
   GUIdone(&c); 

}



//------------------------------------------------------------
// Buy or sell an item
//
// > 0 == buy
// < 0 == sell
BOOL cLoadout::MoveMerchandise(ObjID obj, int dir)
{
   ObjID targ= (dir < 0) ? mContainers[kStoreRegion] : mContainers[kCartRegion]; 

   // how much does this cost? 
   int price = 0;  
   gpPriceProp->Get(obj,&price); 
   // first, see if we have enough money 
   if (price*dir > mCash)
      return FALSE; 

   ObjID moveme = obj; 
   int count = 0; 
   if (mpStackCount->Get(obj,&count) && count > 1)
   {
      AutoAppIPtr_(ObjectSystem,pObjSys); 
      // clone the object
      moveme = pObjSys->BeginCreate(obj,kObjectConcrete); 
      mpStackCount->Set(moveme,1); 
      pObjSys->EndCreate(moveme); 

      // deduct the stack count 
      mpStackCount->Set(obj,count-1); 
   }


   // deduct the cost 
   mCash -= price*dir; 
   // move the merchandise
   mpContainSys->Add(targ,moveme,0,CTF_COMBINE); 


   return TRUE;
}

//------------------------------------------------------------
// Empty the shopping cart
//

void cLoadout::ClearCart()
{
   // Give you back the total price of the loot
   sContainIter* iter = mpContainSys->IterStart(mContainers[kCartRegion]); 
   for (; !iter->finished; mpContainSys->IterNext(iter))
   {
      ObjID obj = iter->containee; 
      int count = 1;
      mpStackCount->Get(obj,&count); 
      int price = 0; 
      gpPriceProp->Get(obj,&price); 

      mCash += price*count; 
   }
   mpContainSys->IterEnd(iter); 

   // Move the loot 
   mpContainSys->MoveAllContents(mContainers[kStoreRegion],mContainers[kCartRegion],CTF_COMBINE); 

   // refresh selection if it's cart or store
   if (mRegionSel > kInvRegion)
      Select(-1,-1); 
}


//------------------------------------------------------------
// Loop events
//

void cLoadout::OnFrame()
{
   DrawSelection(); 
}

void cLoadout::OnLoopMsg(eLoopMessage msg, tLoopMessageData data)
{
   switch(msg)
   {
      case kMsgNormalFrame:
         OnFrame(); 
         break; 
      case kMsgEnterMode:  // Only enter, NOT resume
         BuildContainers(); 
         break; 
      case kMsgExitMode:  // Only exit, NOT suspend
         DestroyContainers(); 
         break; 

      case kMsgSuspendMode:
      {
         AutoAppIPtr(QuestData); 
         pQuestData->Set(MONEY_QVAR,mCash); 
      }
      break;

      case kMsgResumeMode:
      {
         AutoAppIPtr(QuestData); 
         mCash = pQuestData->Get(MONEY_QVAR); 
      }
      break; 

   }
   cDarkPanel::OnLoopMsg(msg,data); 

}

//------------------------------------------------------------
// Handle clicking on an item
//

void cLoadout::OnItemButton(ushort action, int region, int slot)
{
   Select(region,slot);
}

void cLoadout::Select(int region, int slot)
{
   if (region > 0 && mObjs[region][slot] == OBJ_NULL)
      region = -1; 

   if (mpSelState)
   {
      // snap the selection back to initial theta
      sInvRendView view; 
      invRendGetView(mpSelState,&view); 
      view.off_ang.tz = ITEM_THETA; 
      invRendSetView(mpSelState,&view); 
      DrawSelection(); 
      
   // blast the state      
      invRendFreeState(mpSelState); 
      mpSelState = NULL; 

   }

   // set the buy button draw element 
   mElems[(int)kBuy] = mBuyElems[(region > 0) ? region : 0]; 
   // redraw it 
   region_expose(LGadBoxRegion(LGadCurrentRoot()),&mRects[(int)kBuy]); 
   
   mRegionSel = region;
   mSlotSel = slot; 
}

bool cLoadout::sButtList::BListCB(ushort action, int button, void* data, LGadBox* blist)
{
   sButtList* us = (sButtList*)blist; 
   us->loadout->OnItemButton(action, us->region, button); 
   return FALSE; 
}

#define kHackSndPlay  (0)
#define kHackSndBuy   (1)
#define kHackSndClear (2)
#define kHackSndError (3)
static const char *hacked_sound_name[]={"select1","kaching","select2","atten1"};
#define PlayHackedSound(idx) SFX_Play_Raw(SFX_STATIC,NULL,hacked_sound_name[idx]);

void cLoadout::OnButtonList(ushort action, int button)
{
   if (action & BUTTONGADG_LCLICK)
   {
      switch (button)
      {
         case kBriefing:
            PushBriefingMovieMode(GetNextMission()); 
            break; 
         case kPlay:
            PlayHackedSound(kHackSndPlay);
            // may want to sleep here briefly...
            mpPanelMode->Exit(); 
            break; 
         case kGoals:
            SwitchToObjectivesMode(TRUE); 
            break; 
         case kClear:
            if (mObjs[kCartRegion].Size() > 0 && mObjs[kCartRegion][0] != OBJ_NULL)
            {
               ClearCart(); 
               ComputeContainers(); 
               DrawItemRegion(kStoreRegion); 
               DrawItemRegion(kCartRegion); 
               DrawCash(); 
               if (mRegionSel > kInvRegion) 
                  Select(mRegionSel,mSlotSel);
               PlayHackedSound(kHackSndClear);
            }
            break; 

         case kBuy:
            if (mRegionSel > kInvRegion) // if the current selection is in cart or store
            {
               ObjID obj = mObjs [mRegionSel][mSlotSel]; 
               int dir = (mRegionSel == kStoreRegion) ? +1 : -1; 
               // buy the stuff
               if (MoveMerchandise(obj,dir))
               {
                  // refresh display 
                  ComputeContainers(); 
                  if (mObjs[mRegionSel][mSlotSel] != obj)
					      Select(-1,-1); //Don't keep buying.  AMSD
                  //   Select(mRegionSel,mSlotSel);
                  DrawItemRegion(kStoreRegion); 
                  DrawItemRegion(kCartRegion); 
                  DrawCash(); 
                  PlayHackedSound(kHackSndBuy);
               }
               else
                  PlayHackedSound(kHackSndError);
            }
            else
               PlayHackedSound(kHackSndError);               
            break;
      }

   }
}

static const char* button_names[] = 
{
   "clear", 
   "play",
   "briefing",
   "goals",
}; 

static sDarkPanelDesc panel_desc = 
{
   "loadout", 
   cLoadout::kNumButts,
   cLoadout::kNumRects, 
   cLoadout::kNumStringButts, 
   button_names, 
   NULL,
   0,
   kMetaSndPanelLoadout,
}; 

cLoadout::cLoadout()
   : cDarkPanel(&panel_desc), mpContainSys(AppGetObj(IContainSys)), mpSelState(NULL)
{
   mpStackCount = mpContainSys->StackCountProp(); 
}

cLoadout::~cLoadout()
{
   SafeRelease(mpContainSys); 
   SafeRelease(mpStackCount); 
}

//
// EXTERNAL API 
//


static cLoadout* gpLoadout = NULL; 

void SwitchToLoadoutMode(BOOL push) 
{
   if (gpLoadout)
   {
      IPanelMode* panel = gpLoadout->GetPanelMode(); 

      panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
      SafeRelease(panel); 
   }
}

const sLoopInstantiator* DescribeLoadoutMode(void)
{
   if (gpLoadout)
   {
      cAutoIPtr<IPanelMode> panel = gpLoadout->GetPanelMode(); 

      return panel->Instantiator(); 
   }
   return NULL; 
}



static void create_panel_modes()
{
   gpLoadout = new cLoadout; 
}


//------------------------------------------------------------------
// INIT/TERM
//

void init_commands(); 

void LoadoutInit()
{
   create_panel_modes(); 
   init_commands(); 
   gpStoreProp = CreateStoreProperty(); 
   gpPriceProp = CreatePriceProperty(); 
   gpDescProp = CreateDescriptionProperty(); 
}

void LoadoutTerm()
{
   delete gpLoadout;
   SafeRelease(gpStoreProp); 
   SafeRelease(gpPriceProp); 
   SafeRelease(gpDescProp); 
}


//----------------------------------------
// COMMANDS
//

static void do_loadout()
{
#ifdef PLAYTEST
   SwitchToLoadoutMode(TRUE); 
#endif 
}

static Command commands[] = 
{

   { "loadout", FUNC_VOID, do_loadout, "Go to loadout UI.", HK_ALL }, 
}; 


static void init_commands()
{
   COMMANDS(commands,HK_ALL); 
}



