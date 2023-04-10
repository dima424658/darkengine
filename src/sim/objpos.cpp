/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/objpos.cpp,v 1.24 2000/02/24 23:43:12 mahk Exp $

#include <float.h>
#include <objpos.h>
#include <refsys.h>
#include <rendprop.h>
#include <portal.h>

#include <propert_.h>
#include <dataops_.h>
#include <proparry.h>

#include <posprop.h>
#include <osysbase.h>

#include <packflag.h>

#include <dlistsim.h>
#include <dlisttem.h>

#include <iobjsys.h>
#include <objquery.h>

#include <dbasemsg.h>
#include <objnotif.h>
#include <tagfile.h>
#include <vernum.h>

#include <sdesbase.h>
#include <sdestool.h>

//
// THESE MUST COME LAST
//

#include <initguid.h>
#include <posiid.h>

#include <dbmem.h>


////////////////////////////////////////////////////////////
// POSITION PROPERTY
//

struct cZeroPos : ObjPos // @Note (toml 05-19-98): the following code exists to work around a Wacom 10.6 bug
{
   cZeroPos()
   {
      loc.cell = CELL_INVALID;
      loc.hint = CELL_INVALID;
   }
};

static cZeroPos zeropos;

struct sPropPos : public Position
{
   sPropPos(const Position& pos = zeropos) 
   { *(Position*)this = pos; }; 
}; 

class cPosOps: public cClassDataOps<sPropPos>
{
public:
   // override default flags
   cPosOps() :cClassDataOps<sPropPos>(kNoFlags){};

   STDMETHOD(Write)(sDatum val, IDataOpsFile* file)
   {
      Position pos = *(Position*)val.value;
      pos.loc.cell = pos.loc.hint = CELL_INVALID; 
      return cClassDataOps<sPropPos>::Write(sDatum(&pos),file); 
   }
}; 

class cPosStore : public cArrayPropertyStore<cPosOps>
{
}; 

//
// We're using kPropertyConcrete, so we have to use cGenericProperty
//
class cPosProp: public cGenericProperty<IPositionProperty,&IID_IPositionProperty,Position*>
{

protected:

   typedef cGenericProperty<IPositionProperty,&IID_IPositionProperty,Position*> cParent; 

   // callback element 
   struct sCBElem
   {
      ObjPosCB cb;
      void* data; 

      sCBElem(ObjPosCB cb_, void* data_)
         : cb(cb_),data(data_) 
      {
      }
   };

   typedef cSimpleDList<ObjID> cObjList; 
   typedef cSimpleDList<sCBElem> cCBList; 

   class cPosObjIDSink: public cObjIDSink
   {
   public:
      cPosObjIDSink() 
      {
         AutoAppIPtr(ObjIDManager);
         pObjIDManager->Connect(this); 
      }

      ~cPosObjIDSink()
      {
         AutoAppIPtr(ObjIDManager);
         pObjIDManager->Disconnect(this); 
      }



      cPosProp& PosProp() { return *(cPosProp*)((char*)this - offsetof(cPosProp,m_Sink)); }; 

      void OnObjIDSpaceResize(const sObjBounds& bounds) 
      { 
         PosProp().mChanged.Resize(bounds.max); 
         PosProp().mNeedsReref.Resize(bounds.max); 
      }
   }; 

   friend class cPosObjIDSink; 


public:
   cPosProp(const sPropertyDesc* desc, cPosStore* store)
      : cParent(desc,store),
        mNeedsReref(gMaxObjID),
        mChanged(gMaxObjID),
        mStore(*store)
   {
      mpStore->SetOps(&mOps);
      mDesc.flags |= kPropertySendBeginCreate; 
   }
   
   STANDARD_DESCRIBE_TYPE(Position);
   //
   // Notify Msg
   // 
   STDMETHOD_(void, Notify) (ePropertyNotifyMsg msg, PropNotifyData data);

   //
   // Position callback management
   //
   void SendChangeCalls(void);
   void AddCB(ObjPosCB cb, void* data) { mCBs.Append(sCBElem(cb,data)); } 
   void AddChanged(ObjID obj);
   void ClearReref(ObjID obj) { mNeedsReref.Clear(obj); }
   void ResetChanged(void); 

protected:
   void LoadEOSPoses(ITagFile* file); 
   
   void OnListenMsg(ePropertyListenMsg msg, ObjID obj, uPropListenerValue val)
   {
      if (msg & kListenPropLoad)
         return ;
      if (OBJ_IS_CONCRETE(obj) && (msg & kListenPropModify))
         AddChanged(obj);
      cParent::OnListenMsg(msg,obj,val);
   }

   cPackedBoolSet mNeedsReref;
   cPackedBoolSet mChanged; 

   cObjList mChangedList; 
   cCBList mCBs; 
   cPosStore& mStore; 
   cPosOps mOps; 

   cPosObjIDSink m_Sink; 
   
};

static cPosProp* gPosProp = NULL; 
static cPosStore* gPosStore = NULL; 
// the pointer to our vector of positions
#define gPosVec ((Position**)&((*gPosStore)[0]))

static sPropertyDesc pos_prop_desc = 
{
   PROP_POSITION_NAME, 
   kPropertyConcrete|kPropertyInstantiate|kPropertyNoInherit,
   NULL, // constraints
   1, 1, // version
   { "", "Position" },
   kPropertyChangeLocally,   // net_flags
}; 

static void init_pos_prop(void) 
{
   gPosStore = new cPosStore; 
   gPosProp = new cPosProp(&pos_prop_desc,gPosStore);    
}

static void free_pos_prop(void) 
{
   SafeRelease(gPosStore);
   SafeRelease(gPosProp);
}

////////////////////////////////////////////////////////////
// POS ACCESSORS
//

ObjPos* ObjPosGet(ObjID obj)
{
   if (OBJ_IS_ABSTRACT(obj))
   {
      ObjPos* pos = NULL;
      gPosProp->Get(obj,&pos);
      return pos; 
   }
   else
      return gPosVec[obj]; 
}

mxs_vector *ObjPosGetLocVector(ObjID obj)
{
   mxs_vector *pVec=NULL;
   if (OBJ_IS_ABSTRACT(obj))
   {
      ObjPos* pos = NULL;
      gPosProp->Get(obj,&pos);
      if (pos!=NULL)
         pVec=&pos->loc.vec;
   }
   else
      pVec=&gPosVec[obj]->loc.vec;
   return pVec; 
}

////////////////////////////////////////



void ObjPosUpdate(ObjID obj, const mxs_vector* pos, const mxs_angvec* ang)
{
   Assert_(pos && ang); 
   Assert_(!_isnan(pos->x) && !_isnan(pos->y) && !_isnan(pos->z));

   ObjPos* thepos = NULL;

   gPosProp->Create(obj); 
   gPosProp->Get(obj,&thepos);

   // use previous location as hint
   MakeHintedLocationFromVector(&thepos->loc,pos,&thepos->loc);
   thepos->fac = *ang; 
   gPosProp->Touch(obj); 
}

void ObjRotate(ObjID obj, const mxs_angvec* ang)
{
   ObjPos dummy = zeropos; 
   ObjPos* thepos = &dummy; 
   gPosProp->Get(obj,&thepos); 
   thepos->fac = *ang; 
   gPosProp->Set(obj,thepos); 
}

void ObjTranslate(ObjID obj, const mxs_vector* vec)
{
   Assert_(!_isnan(vec->x) && !_isnan(vec->y) && !_isnan(vec->z));
   ObjPos dummy = zeropos; 
   ObjPos* thepos = &dummy;
   gPosProp->Get(obj,&thepos); 

   // use previous location as hint
   MakeHintedLocationFromVector(&thepos->loc,vec,&thepos->loc);
   gPosProp->Set(obj,thepos); 
}

void ObjPosSetLocation(ObjID obj, const Location *newloc)
{
   Assert_(newloc != NULL);
   Assert_(!_isnan(newloc->vec.x) && !_isnan(newloc->vec.y) && !_isnan(newloc->vec.z));

   ObjPos dummy = zeropos; 
   ObjPos *thepos = &dummy;
   gPosProp->Get(obj,&thepos); 

   // Location should already be set
   thepos->loc = *newloc;
   gPosProp->Set(obj,thepos); 
}

void ObjPosUpdateUnsafe(ObjID obj, const mxs_vector* pos, const mxs_angvec* ang)
{
   Assert_(pos && ang); 
   Assert_(OBJ_IS_CONCRETE(obj));
   Assert_(!_isnan(pos->x) && !_isnan(pos->y) && !_isnan(pos->z));


   ObjPos* thepos = gPosVec[obj]; 
   Assert_(thepos); 

   // use previous location as hint
   MakeHintedLocationFromVector(&thepos->loc,pos,&thepos->loc);
   thepos->fac = *ang; 

   // This is kinda slow, but only kinda.
   gPosProp->Set(obj,thepos); 
}

// someday maybe this could be less dorky
// assume the client has used the macros correctly
void ObjPosCopyUpdate(ObjID obj, ObjPos *vp)
{
   gPosProp->Set(obj,vp);
}


Position* ObjPosGetUnsafe(ObjID obj)
{
   Assert_(OBJ_IS_CONCRETE(obj));
   return gPosVec[obj]; 
}

void ObjForceReref(ObjID obj)
{
   if (OBJ_IS_CONCRETE(obj) && gPosVec[obj])
   {
      if (ObjHasRefs(obj))
         ObjUpdateLocs(obj); 
      else
         ObjDelRefs(obj);

      gPosProp->ClearReref(obj); 
   }
}

void ObjPosTouch(ObjID obj)
{
   if (OBJ_IS_CONCRETE(obj) && gPosVec[obj])
      gPosProp->AddChanged(obj); 
}


////////////////////////////////////////////////////////////
// NOTIFICATION 
//


void ObjPosListen(ObjPosCB cb, void* data)
{
   gPosProp->AddCB(cb,data);
}

void ObjPosSynchronize(void)
{
   gPosProp->SendChangeCalls(); 
}

////////////////////////////////////////////////////////////
// POSITION SDESC
//

static sFieldDesc pos_fields[] =
{
   { "Location", kFieldTypeVector, FieldLocation(Position,loc.vec), },
   { "Heading", kFieldTypeInt,FieldLocation(Position,fac.tz), kFieldFlagUnsigned|kFieldFlagHex }, 
   { "Pitch", kFieldTypeInt,FieldLocation(Position,fac.ty), kFieldFlagUnsigned|kFieldFlagHex }, 
   { "Bank", kFieldTypeInt,FieldLocation(Position,fac.tx), kFieldFlagUnsigned|kFieldFlagHex }, 
   { "Cell", kFieldTypeInt, FieldLocation(Position,loc.cell), kFieldFlagNotEdit }, 
   { "Hint", kFieldTypeInt, FieldLocation(Position,loc.cell), kFieldFlagNotEdit }, 

}; 

static sStructDesc pos_sdesc = StructDescBuild(Position,0,pos_fields); 

////////////////////////////////////////////////////////////
// INITIALIZATION 
//

void ObjPosInit(void)
{
   init_pos_prop(); 
   portal_object_pos = ObjPosGetUnsafe; 
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Register(&pos_sdesc); 
}

void ObjPosTerm(void)
{
   free_pos_prop();    
   SafeRelease(gPosProp); 
}

////////////////////////////////////////////////////////////
// Re-reffing
//

void ObjDeleteAllRefs(void)
{
   // not fast, but robust 
   for (int i = 0; i < gMaxObjID; i++)
   {
      ObjDelRefs(i); 
   }
}

/////////////////////////////////////////


void ObjBuildAllRefs(void)
{
   AutoAppIPtr_(ObjectSystem,pOS); 
   IObjectQuery* query = pOS->Iter(kObjectConcrete); 
   for(;!query->Done(); query->Next())
   {
      ObjID obj = query->Object(); 
      ObjPos* pos = gPosVec[obj];
      if (pos)
      {
         Location* loc = &pos->loc; 
         UpdateChangedLocation(loc);
         if (ObjHasRefs(obj))
            ObjUpdateLocs(obj);
         loc->cell = loc->hint = ComputeCellForLocation(loc);

         gPosProp->ClearReref(obj); 
      }
   }
   SafeRelease(query);
}

////////////////////////////////////////////////////////////
// cPosProp Methods
//

void cPosProp::SendChangeCalls(void)
{
   cObjList::cIter objiter;
   for (objiter = mChangedList.Iter(); !objiter.Done(); objiter.Next())
   {
      ObjID obj = objiter.Value(); 
      Position* pos = (Position*)mStore[obj].value;

      if (!pos)  
         continue;

      if (mNeedsReref.IsSet(obj))
      {
         if (ObjHasRefs(obj))
            ObjUpdateLocs(obj); 
         else
            ObjDelRefs(obj);             
      }

      cCBList::cIter cbiter;
      for (cbiter = mCBs.Iter(); !cbiter.Done(); cbiter.Next())
      {
         const sCBElem& val = cbiter.Value();
         val.cb(obj,pos,val.data); 
      }
   }
   mChangedList.DestroyAll();
   mNeedsReref.ClearAll();
   mChanged.ClearAll(); 
}

////////////////////////////////////////

void cPosProp::AddChanged(ObjID obj)
{
   if (!mChanged.IsSet(obj))
   {
      mChanged.Set(obj);
      mChangedList.Append(obj); 
      mNeedsReref.Set(obj); 
   }
}


////////////////////////////////////////

void cPosProp::ResetChanged()
{
   mChanged.ClearAll();
   mNeedsReref.ClearAll(); 
   mChangedList.DestroyAll(); 
}


////////////////////////////////////////

STDMETHODIMP_(void) cPosProp::Notify(ePropertyNotifyMsg msg, PropNotifyData raw) 
{
   uObjNotifyData data;
   data.raw = raw; 

   switch (NOTIFY_MSG(msg))
   {
      case kObjNotifyBeginCreate:
         if (OBJ_IS_CONCRETE(data.obj))
         {
            Create(data.obj); 
         }
         break; 

      case kObjNotifyDelete:
         if (OBJ_IS_CONCRETE(data.obj))
            ObjDelRefs(data.obj); 
         break; 

      case kObjNotifyReset:
         ResetChanged(); 
         break; 

      case kObjNotifyLoad:
         if (msg & kObjPartConcrete)
         {
            LoadEOSPoses(data.db.load); 
            ResetChanged(); 
         }
         break; 

      case kObjNotifyPostLoad:
         break; 
         
   }
   cParent::Notify(msg,raw); 
}


////////////////////////////////////////
//
// Parse the old EOS tag and read in object positions 
//

#define READ(f,v) if (f->Read((char*)&(v),sizeof(v)) != sizeof(v)) goto err

static const TagFileTag EOSTag = { "OBJECTS" };
static const TagVersion EOSVersion = { 0, 1, };

void cPosProp::LoadEOSPoses(ITagFile* file) 
{
   if (file->BlockSize(&EOSTag) == 0)
      return;

   TagVersion v = EOSVersion; 

   HRESULT result = file->OpenBlock(&EOSTag,&v); 
   if (SUCCEEDED(result))
   {
      char buf[256];
      int* sizes = NULL;  
      int i; 

      // code based obj ObjsLoad in EOS objsave.c 

      // read in number of phyla 
      int nphyla; 
      READ(file,nphyla);
      
      // read in a size for each phylum 
      sizes = new int[nphyla+1]; 
      for (i = 0; i <= nphyla; i++)
         READ(file,sizes[i]); 

      // read in max objID used
      ObjID max_id; 
      READ(file,max_id); 

      // read in phyla, throwing them away 
      // we could actually just seek, but why fix what aint broke 
      for (i = 0; i <= nphyla; i++)
      {
         Assert_(sizeof(buf) >= sizes[i]); 

         if (file->Read(buf,sizes[i]) != sizes[i])
            goto err; 
      }

      ObjID obj; 
      if (max_id > nphyla)
         do 
         {
            // read obj id 
            READ(file,obj); 

            // read phylum 
            uchar phylum; 
            READ(file,phylum); 

            // read the object
            if (file->Read(buf,sizes[phylum]) != sizes[phylum])
               goto err; 

            // this is what we want!  the obj pos! 
            ObjPos pos; 
            READ(file,pos); 
            gPosProp->Set(obj,&pos); 
         }
      while (obj < max_id);

   err:
      delete sizes; 
      file->CloseBlock(); 
   }
}




