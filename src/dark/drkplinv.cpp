/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkplinv.cpp,v 1.16 2000/02/05 15:10:20 adurant Exp $

#include <drkplinv.h>
#include <appagg.h>
#include <aggmemb.h>
#include <drkinvpr.h>
#include <contain.h>
#include <propface.h>
#include <sdesbase.h>
#include <sdestool.h>
#include <playrobj.h>
#include <frobctrl.h>
#include <objpos.h>

// database file stuff
#include <vernum.h>
#include <iobjsys.h>
#include <tagfile.h>
#include <dbasemsg.h>
#include <objremap.h>

#include <config.h>
#include <cfgdbg.h>

#include <rendprop.h>
#include <physapi.h>

#include <simman.h>
#include <dspchdef.h>
#include <simdef.h>

#include <drkvo.h>

#include <traitman.h>
#include <iobjsys.h>

#include <stdlib.h>
#include <gen_bind.h>
#include <command.h>

// Include these last
#include <dbmem.h>

static eWhichInvObj type_to_slot[] = { kCurrentItem, kCurrentItem, kCurrentWeapon };

void setup_contains_sdesc();
////////////////////////////////////////////////////////////
// PLAYER INVENTORY AGGREGATE MEMBER
//

class cInventory : public cCTDelegating<IInventory>,
                   public cCTAggregateMemberControl<kCTU_Default>
{
protected:
   static sRelativeConstraint gConstraints[];



public:
   cInventory(IUnknown* pOuter)
   {
      MI_INIT_AGGREGATION_1(pOuter,IInventory,kPriorityNormal,gConstraints);
   }

   ~cInventory()
   {
   }

   //
   // AGG PROTOCOL
   //


   STDMETHOD(Init)()
   {
      mpInvType = CreateInvTypeProperty();
      mpCycOrder = CreateCycleOrderProperty();
      mpContainSys = AppGetObj(IContainSys);
      mpContainSys->Listen(OBJ_NULL,ContainCB,this);
      setup_contains_sdesc();
      SetupSimCB();
      mInSim = FALSE;

      return S_OK;
   }

   STDMETHOD(Term)()
   {
      SafeRelease(mpInvType);
      SafeRelease(mpContainSys);
      SafeRelease(mpCycOrder);

      return S_OK;
   }

   //------------------------------------------------------------
   // HELPERS
   //

   // unfocus the currently wielded object
   void Unfocus()
   {
      FrobSetCurrent(kFrobLocInv,kFrobLevSelect,OBJ_NULL);
   }

   void Focus()
   {
      FrobSetCurrent(kFrobLocInv,kFrobLevSelect,mCurSel[mInHand].obj);
   }

   void OnAdd(ObjID obj, BOOL select = TRUE)
   {
      BOOL speak = mInSim;  // should we play a voice over

      // when an object goes into your inventory, select it,
      // so you get feedback
      if (GetType(obj) == kInvTypeJunk)
      {
         Select(obj);
         Wield(kCurrentItem);
      }
      else if (select)
         Select(obj);
      else
         speak = FALSE;

      if (speak)
      {
         DarkVoiceOverEvent(obj,kVoicePickup);
      }
   }

   void OnRemove(ObjID obj)
   {
      for (int i = 0; i < kNumItemSels; i++)
         if (mCurSel[i].obj == obj)
         {
            ClearSelection((eWhichInvObj)i);
            break;
         }

   }

   void Load(ITagFile* file);
   void Save(ITagFile* file);

   //------------------------------------------------------------
   // API METHODS
   //

   STDMETHOD_(eInventoryType,GetType)(ObjID obj)
   {
      int retval = kInvTypeJunk;
      mpInvType->Get(obj,&retval);
      return (eInventoryType)retval;
   }

   STDMETHOD(Add)(ObjID obj)
   {
      BOOL junk = GetType(obj) == kInvTypeJunk;
      // can't have two junk items
      if (junk && WieldingJunk())
         return E_FAIL;

      HRESULT result = mpContainSys->Add(PlayerObject(),obj,kContainTypeInventory,CTF_COMBINE);

      if (FAILED(result)) return result;

      return result;
   }

   STDMETHOD(Remove)(ObjID obj)
   {
      return mpContainSys->Remove(PlayerObject(),obj);
   }

   STDMETHOD(Select)(ObjID obj)
   {
      ObjID old_obj;

      if (obj == OBJ_NULL) return E_FAIL;

      if (!mpContainSys->Contains(PlayerObject(),obj))
         return E_FAIL;

      eWhichInvObj which = type_to_slot[GetType(obj)];

      if (obj == mCurSel[which].obj)
         return S_FALSE;

      if (WieldingJunk())
         return E_FAIL;

      old_obj = mCurSel[kCurrentItem].obj;

      if (which == mInHand)
         Unfocus();
      else if (old_obj != obj) // send message for general inventory stuff
         FrobSendInvSelectionMsg(kFrobLocInv,kFrobLevSelect, old_obj, OBJ_NULL);

      mCurSel[which].obj = obj;

      if (which == mInHand)
         Focus();
      else if (old_obj != obj) // send message for general inventory stuff
         FrobSendInvSelectionMsg(kFrobLocInv,kFrobLevSelect, OBJ_NULL, obj);

      if (GetType(obj) == kInvTypeJunk)
         Wield(which);

#ifdef DBG_ON
      static char* sel_names[] = { "weapon", "item" };
      ConfigSpew("inv_select_spew",("Current %s is now %d\n",sel_names[which],obj));
#endif // DBG_ON

      return S_OK;
   }

   STDMETHOD_(ObjID,Selection)(eWhichInvObj which)
   {
      return mCurSel[which].obj;
   }

   STDMETHOD(ClearSelection)(eWhichInvObj which)
   {
      if (which == mInHand)
      {
         Unfocus();
      }

      if (which != mInHand) //send deselect message when we clear out.
        FrobSendInvSelectionMsg(kFrobLocInv,kFrobLevSelect,mCurSel[which].obj,OBJ_NULL);

      mCurSel[which].obj = OBJ_NULL;

      if (which == mInHand)
         Wield(kCurrentWeapon); // if we got a weapon selection, wield it

      return S_OK;
   }

   const char* CycleOrder(ObjID obj)
   {
      const char* order;
      if (mpCycOrder->Get(obj,&order))
         return order;
      AutoAppIPtr_(TraitManager,pTraitMan);
      AutoAppIPtr_(ObjectSystem,pObjSys);

      // ascend hierarchy looking for a symname
      for (;obj != OBJ_NULL; obj = pTraitMan->GetArchetype(obj))
      {
         order =pObjSys->GetName(obj);
         if (order)
            return order;
      }

      return "";
   }

   static inline int Compare(const char* s1, const char* s2, ObjID o1, ObjID o2)
   {
      int cmp = strcmp(s1,s2);
      if (cmp == 0) cmp = o1 - o2;
      return cmp;
   }

   // @OPTIMIZE: This is the slowest possible implementation
   STDMETHOD_(ObjID, CycleSelection)(eWhichInvObj which, eCycleDirection dir)
   {
      if (WieldingJunk())
         return WieldedObj();

      Assert_(dir != 0);

      ObjID cur = mCurSel[which].obj;

      const char* cur_order = CycleOrder(cur);

      ObjID best = OBJ_NULL;
      const char* best_order = NULL;

      ObjID bottom = OBJ_NULL;
      const char* bottom_order = OBJ_NULL;

      sContainIter* iter = mpContainSys->IterStart(PlayerObject());
      ObjID prev = OBJ_NULL;


      for(; !iter->finished; mpContainSys->IterNext(iter))
      {
         ObjID item = iter->containee;

         if (item == cur)
            continue;

         // skip past the wrong type
         int itemtype = GetType(item);
         if (type_to_slot[itemtype] != which)
            continue;

         const char* item_order = CycleOrder(item);


         // update bottom
         if (bottom == OBJ_NULL
             || Compare(item_order,bottom_order,item,bottom)*dir < 0)
         {
            bottom = item;
            bottom_order = item_order;
         }

         // we have to come after the target to be best
         if (Compare(item_order,cur_order,item,cur)*dir < 0)
            continue;

         if (best == OBJ_NULL
             || Compare(item_order,best_order,item,best)*dir < 0)
         {
            best = item;
            best_order = item_order;
         }
      }

      mpContainSys->IterEnd(iter);

      if (cur == OBJ_NULL)
         best = bottom;

      if (best != OBJ_NULL)
         Select(best);
      else
         ClearSelection(which);

      return best;
   }

   STDMETHOD(Wield)(eWhichInvObj which)
   {
      if (which == mInHand)
         return S_FALSE;

      ObjID wielded = mCurSel[mInHand].obj;
      if (WieldingJunk())
         return E_FAIL;
      Unfocus();
      mInHand = which;
      Focus();
      return S_OK;
   }

   STDMETHOD_(ObjID,WieldedObj)()
   {
      return mCurSel[mInHand].obj;
   }

   STDMETHOD_(BOOL,WieldingJunk)()
   {
      ObjID wielded = mCurSel[mInHand].obj;
      return wielded != OBJ_NULL && GetType(wielded) == kInvTypeJunk;
   }


   STDMETHOD(DatabaseNotify)(ulong msg, ITagFile* file);

   //------------------------------------------------------------
   // CONTAINS LISTENER
   //

    BOOL OnContainCB(eContainsEvent ev, ObjID container, ObjID containee, eContainType type)
   {
      if (ev == kContainCombine && PlayerObjectExists())
      {
         // check for the case where something gets combined into our inventory
         if (mpContainSys->IsHeld(PlayerObject(),container) != ECONTAIN_NULL)
         {
	    //check "auto_equip" variable
	    if (atof(g_pInputBinder->ProcessCmd("echo $auto_equip"))!=0.0)
	      Select(container);
            return TRUE;
         }
      }

      if (!IsPlayerObj(container))
         return TRUE;

      switch (ev)
      {
         case kContainRemove:
            OnRemove(containee);
            break;
         case kContainAdd:
	    //third argument is the "should I select?" bool.
	    //it's false if we aren't autoequipping or aren't in sim.
            OnAdd(containee,
		  (mInSim &&
		  (atof(g_pInputBinder->ProcessCmd("echo $auto_equip"))!=0.0)));
            break;
      }

      return TRUE;
   }

   static BOOL ContainCB(eContainsEvent ev, ObjID container, ObjID containee, eContainType type, ContainCBData data)
   {
      cInventory* us = (cInventory*)data;
      return us->OnContainCB(ev,container,containee,type);
   }

   //------------------------------------------------------------
   // Sim Listener
   //
   void OnSimMsg(const sDispatchMsg& msg)
   {

      switch(msg.kind)
      {
         case kSimInit:
         case kSimResume:
         {
            // Clear selections
            for (int i = 0; i < kNumItemSels; i++)
               ClearSelection((eWhichInvObj)i);

            sContainIter* iter = mpContainSys->IterStart(PlayerObject());
            for(; !iter->finished; mpContainSys->IterNext(iter))
            {
               OnAdd(iter->containee,FALSE); // add, but don't select
               ObjSetHasRefs(iter->containee,FALSE);
               PhysDeregisterModel(iter->containee);
            }
            mpContainSys->IterEnd(iter);

            mInSim = TRUE;


         }
         break;
         default:
            mInSim = FALSE;
            break;
      }
   }

   static void SimCB(const sDispatchMsg* msg, const sDispatchListenerDesc* desc)
   {
      cInventory* us = (cInventory*)desc->data;
      us->OnSimMsg(*msg);
   }

   void SetupSimCB(void)
   {
      sDispatchListenerDesc desc =
      {
         &IID_IInventory,
         kSimInit|kSimResume,
         SimCB,
         this
      };

      AutoAppIPtr_(SimManager,pSimMan);
      pSimMan->Listen(&desc);
   }

protected:
   struct sSel
   {
      ObjID obj;
   };

   IIntProperty* mpInvType;
   IStringProperty* mpCycOrder;
   IContainSys*  mpContainSys;

   sSel mCurSel[kNumItemSels];
   eWhichInvObj mInHand;  // which inv obj is in hand
   BOOL mInSim;           // are we in the sim

};

F_DECLARE_INTERFACE(IPropertyManager);

sRelativeConstraint cInventory::gConstraints[] =
{
   { kConstrainAfter, &IID_IPropertyManager},
   { kConstrainAfter, &IID_IContainSys},
   { kConstrainAfter, &IID_IStructDescTools},

   { kNullConstraint },
};

////////////////////////////////////////////////////////////
// Database stuff
//

STDMETHODIMP cInventory::DatabaseNotify(ulong msg, ITagFile* file)
{
   AutoAppIPtr_(ObjectSystem,pObjSys);
   eObjPartition partition = (eObjPartition)DB_OBJ_PARTITION(msg);

   switch(DB_MSG(msg))
   {
      case kDatabaseReset:
      {
         mInHand = kCurrentWeapon;
         for (int i = 0; i < kNumItemSels; i++)
            mCurSel[i].obj = OBJ_NULL;
      }
      break;

      case kDatabaseLoad:
         if (ContainsPlayerTag(file))
            Load(file);
         break;

      case kDatabaseSave:
         if (PlayerObjectExists())
            Save(file);
         break;
   }
   return S_OK;
}

////////////////////////////////////////

static TagVersion inv_version = { 1, 0 };
static TagFileTag inv_tag = { "PlayerInv" };



void cInventory::Save(ITagFile* file)
{
   TagVersion v = inv_version;
   HRESULT result = file->OpenBlock(&inv_tag,&v);
   if (SUCCEEDED(result))
   {
      Verify(file->Write((char*)&mInHand,sizeof(mInHand)) == sizeof(mInHand));
      Verify(file->Write((char*)&mCurSel,sizeof(mCurSel)) == sizeof(mCurSel));
      file->CloseBlock();
   }

}

void cInventory::Load(ITagFile* file)
{
   TagVersion v = inv_version;
   HRESULT result = file->OpenBlock(&inv_tag,&v);
   if (SUCCEEDED(result))
   {
      Verify(file->Read((char*)&mInHand,sizeof(mInHand)) == sizeof(mInHand));
      Verify(file->Read((char*)&mCurSel,sizeof(mCurSel)) == sizeof(mCurSel));
      file->CloseBlock();

      for (int i = 0; i < kNumItemSels; i++)
         mCurSel[i].obj = ObjRemapOnLoad(mCurSel[i].obj);
   }

}


////////////////////////////////////////////////////////////
// Factory
//

void InventoryCreate()
{
   AutoAppIPtr(Unknown);
   new cInventory(pUnknown);
}


////////////////////////////////////////////////////////////
// Contains sdesc
//

static char* contain_type_strings[] =
{
   "Alternate",
   "Hand",
   "Belt",

   "Generic Contents",   // 0
};

#define NUM_STRINGS (sizeof(contain_type_strings)/sizeof(contain_type_strings[0]))

static sFieldDesc contain_fields[] =
{
   { "", kFieldTypeEnum, sizeof(int), 0, 0, kContainTypeMin, kContainTypeMax, NUM_STRINGS, contain_type_strings },
};


static sStructDesc contain_sdesc =
{
   CONTAIN_SDESC_NAME,
   sizeof(int),
   kStructFlagNone,
   sizeof(contain_fields)/sizeof(contain_fields[0]),
   contain_fields,
};

static void setup_contains_sdesc()
{
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Register(&contain_sdesc);
}
