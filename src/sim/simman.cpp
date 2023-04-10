/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/simman.cpp,v 1.12 1999/12/14 14:32:50 BODISAFA Exp $

#include <lg.h>
#include <simman.h>
#include <simdef.h>
#include <appagg.h>
#include <aggmemb.h>
#include <lststtem.h>
#include <idispat_.h>

#include <dbasemsg.h>
#include <dbtype.h>
#include <tagfile.h>
#include <vernum.h>

// Sim message stuff
#include <scrptbas.h>
#include <scrptapi.h>
#include <iobjsys.h>
#include <objquery.h>
#include <simscrm.h>
#include <objdef.h>

// Include these last
#include <dbmem.h>
#include <initguid.h>
#include <simmanid.h>


class cBaseSimMan : public cCTDelegating<ISimManager>,
                    public cCTAggregateMemberControl<kCTU_Default>
{


}; 



class cSimMan: public cDispatch<cBaseSimMan>
{
   typedef cDispatch<cBaseSimMan> cParent; 

public:
   cSimMan(IUnknown* pOuter)
      :mLastMsg(0) 
   {
      MI_INIT_AGGREGATION_1(pOuter,ISimManager,kPriorityNormal,NULL); 
   }

   ~cSimMan() {}; 

   STDMETHOD(SendMsg)(const sDispatchMsg* msg)
   {
      Assert_(msg != NULL && msg->kind != 0); 
      mLastMsg = msg->kind; 

      if (msg->kind == kSimInit || msg->kind == kSimResume)
         SendScriptMsg(msg->kind); 

      HRESULT retval = cParent::SendMsg(msg);

      if (msg->kind == kSimTerm || msg->kind == kSimSuspend)
         SendScriptMsg(msg->kind); 


      return retval; 

   }

   STDMETHOD(StartSim)()
   {
      sDispatchMsg msg; 
      
      switch (mLastMsg)
      {
         case 0: // no messages ever sent. 
         {
            msg.kind = kSimInit; 
            break;
         }
         case kSimSuspend:
         {
            msg.kind = kSimResume; 
            break; 
         }
         case kSimInit:
         {
            // sim already started, don't warn.
            return S_FALSE; 
         }
         default:
         {
            Warning(("StartSim: Cannot start the sim from state 0x%X\n",mLastMsg)); 
            return E_FAIL; 
         }
      }
      
      return SendMsg(&msg); 
   }

   STDMETHOD(SuspendSim)()
   {
      if (mLastMsg & (kSimInit|kSimResume))
      {
         sDispatchMsg msg = { kSimSuspend}; 
         return SendMsg(&msg); 
      }
      return S_FALSE; 
   }

   STDMETHOD(StopSim)()
   {
      if (mLastMsg == kSimTerm || mLastMsg == 0) 
         return S_FALSE; 
      sDispatchMsg msg = { kSimTerm };
      return SendMsg(&msg); 
   }


   STDMETHOD_(ulong,LastMsg)() { return mLastMsg; }; 

   STDMETHOD(DatabaseMsg)(ulong msg, ITagFile* file);


   void SendScriptMsg(int kind)
   {
      AutoAppIPtr_(ObjectSystem,ObjSys); 
      AutoAppIPtr(ScriptMan);
      
      BOOL start = kind == kSimInit || kind == kSimResume; 

      if (start)
         pScriptMan->BeginScripts(); 
      
      // Send start to all concrete objects!  Yum!
      if (kind == kSimInit || kind == kSimTerm)
      {
         cAutoIPtr<IObjectQuery> query ( ObjSys->Iter(kObjectConcrete)); 
         for (; !query->Done(); query->Next())
         {
            sSimMsg msg(query->Object(),start);
            pScriptMan->SendMessage(&msg);
         }
      }

      if (!start)
         pScriptMan->EndScripts(); 
   }

protected:
   ulong mLastMsg; 
   
}; 

////////////////////////////////////////

static TagVersion sim_ver = { 1, 0}; 
static TagFileTag sim_tag = { "SIM_MAN" }; 

STDMETHODIMP cSimMan::DatabaseMsg(ulong msg, ITagFile* file)
{
   switch(DB_MSG(msg))
   {
      case kDatabaseReset: 
      {
         mLastMsg = 0; 
      }
      break;

      case kDatabaseLoad:
      case kDatabaseSave:
         if (msg & kDBMission)
         {
            TagVersion v = sim_ver; 
            if (SUCCEEDED(file->OpenBlock(&sim_tag,&v)))
            {
               file->Move((char*)&mLastMsg,sizeof(mLastMsg)); 
               file->CloseBlock(); 
            }
         }
      break; 



   }
   return S_OK; 
}

void SimManagerCreate(void)
{
   AutoAppIPtr(Unknown);
   new cSimMan(pUnknown); 
}


//----------------------------------------
// SCRIPT MESSAGE IMPLEMENTATION 
//

#define kSimMsgVer 1

IMPLEMENT_SCRMSG_PERSISTENT(sSimMsg)
{
   PersistenceHeader(sScrMsg,kSimMsgVer);
   
   Persistent(starting); 
   return TRUE; 
}



