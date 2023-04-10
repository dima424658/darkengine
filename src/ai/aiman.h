/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiman.h,v 1.28 1999/06/15 18:23:23 dc Exp $
//
//
//

#ifndef __AIMAN_H
#define __AIMAN_H

#include <comtools.h>
#include <dynarray.h>

#include <aggmemb.h>
#include <loopapi.h>

#include <nettypes.h>
#include <propbase.h>
#include <netmsg.h>

#include <aiapi.h>
#include <aiapinet.h>
#include <aicmds.h>
#include <aitype.h>

#include <phlistyp.h>

#pragma once
#pragma pack(4)

class cAIBehaviorSets;
class cAIManager;
class cAI;
class cAIConversationManager;

typedef cDynArray<cAI *> cAIArray;

typedef ulong eDamageResult;
typedef void* tDamageCallbackData;
struct sDamageMsg;

typedef int eWeaponEvent;
typedef struct sPropertyListenMsg sPropertyListenMsg;  

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIManager
//

extern cAIManager * g_pAIManager;

///////////////////////////////////////

class cAIManager : public cCTDelegating<IAIManager>,
                   public cCTDelegating<IAINetManager>,
                   public cCTDelegating<IAINetServices>,
                   public cCTAggregateMemberControl<kCTU_Default>,
                   public cLoopClient<kCTU_NoSelfDelete>
{
public:
   cAIManager(IUnknown * pOuter);
   ~cAIManager();

   //
   // Expose a behavior factory to the AI
   //
   STDMETHOD (InstallBehaviorSet)(IAIBehaviorSet * pBehaviorSet);

   //
   // Find the AI for a given object. The second function does not bump reference count.
   //
   STDMETHOD_(IAI *, GetAI)(ObjID objId);
   cAI * AccessAI(ObjID objId);

   //
   // Make an AI out of a given object
   //
   STDMETHOD_(BOOL, CreateAI)(ObjID objId, const char * pszBehaviorSet = g_pszAIDefBehaviorSet);
   STDMETHOD_(BOOL, DestroyAI)(ObjID objId);
   STDMETHOD_(BOOL, SelfSlayAI)(ObjID objId);

   //
   // Count the AIs
   //
   STDMETHOD_(unsigned, CountAIs)();

   //
   // Iteration protocol
   //
   STDMETHOD_(IAI *, GetFirst)(tAIIter *);
   STDMETHOD_(IAI *, GetNext)(tAIIter *);
   STDMETHOD_(void, GetDone)(tAIIter *);

   // Internal AI iteration protocol
   const cAIArray & AccessAIs() const;
   #define kAINoIndex (0xffffffff)
   int GetAIIndex(ObjID id);

   //
   // Motion hooks
   //
   STDMETHOD (SetMotor)(ObjID obj, IMotor * pMotor);
   STDMETHOD (ResetMotionTags)(ObjID obj);
   
   // 
   // Projectile hook
   // 
   STDMETHOD (NotifyFastObj)(ObjID firer, ObjID projectile, const mxs_vector & velocity);

   //
   // Sound hook
   //
   STDMETHOD (NotifySound)(const sSoundInfo *);
   STDMETHOD_(void, SetIgnoreSoundTime)(tSimTime time);

   //
   // ConversationManager
   //
   STDMETHOD_(BOOL, StartConversation)(ObjID conversationID);
   STDMETHOD_(IAIConversationManager*, GetConversationManager)(void);

   ////////////////////////////////////
   //
   // Common property listening. Routed through component notificaton. 
   //

   STDMETHOD_(void, ListenToProperty)(IProperty *);

   //
   // Physics listening, triggered from components. 
   //
   STDMETHOD_(void, ListenForCollisions)(ObjID);
   STDMETHOD_(void, UnlistenForCollisions)(ObjID);

   ////////////////////////////////////
   //
   // Loop system hook
   //

   STDMETHOD_(eLoopMessageResult,  ReceiveMessage)(eLoopMessage, tLoopMessageData hData);

   ////////////////////////////////////
   //
   // IAINetManager members
   //
   
   //
   // Query if an AI is a proxy
   //
   STDMETHOD_(BOOL, IsProxy)(ObjID id);

   //
   // Get the state information about this AI that will be needed to
   // transfer it from machine to machine.
   //
   STDMETHOD (GetAITransferInfo)(ObjID id, sAINetTransfer **);

   //
   // Turn an AI into a proxy, passing it off to a new player.
   //
   STDMETHOD (MakeProxyAI)(ObjID id, ObjID targetPlayer);
   
   //
   // Turn a proxy into a full AI given received data. AI will not
   // attempt to free the data.
   //
   STDMETHOD (MakeFullAI)(ObjID id, const sAINetTransfer *);

   //
   // Run through the AIs, and make sure they are all properly owned.
   // This should be run when we have reason to believe that some of
   // our AIs might have an incorrect understanding of whether they are
   // Proxies or Full.
   //
   STDMETHOD (RecheckAIOwnership)();

   //
   // The handler id to use in network messages that should be handled by the AI manager.
   //
   STDMETHOD_(tNetMsgHandlerID,  NetMsgHandlerID)();

   ////////////////////////////////////
   //
   // IAINetServices members
   //

   //
   // Query if in a network game
   //
   STDMETHOD_(BOOL, Networking)();
   
   //
   // Get a list of all player objects. List is null terminated; also returns number
   //
   STDMETHOD_(int, GetPlayers)(tAIPlayerList *);

   //
   // Set the physics for a ballistic AI
   //
   STDMETHOD_(void, SetObjImpulse)(ObjID obj, float x, float y, float z, float facing, BOOL rotating = TRUE);

   //
   // Initiate a transfer of an AI to another machine
   //
   STDMETHOD_(BOOL, TransferAI)(ObjID objAI, ObjID objDestPlayer);

   //
   // Query what the last movement desire was, in the form of an
   // impulse. Returns TRUE if there actually was one
   //
   STDMETHOD_(BOOL, GetTargetVel)(ObjID ai, sAIImpulse * pResult);

   ////////////////////////////////////
   //
   // Aggregate hooks
   //

   STDMETHOD (Init)();
   STDMETHOD (End)();

   //////////
   //
   // Network message callbacks
   //
   static void handleTransferAI(ObjID objId, int size, void *pTransferData,
                                cAIManager *pAIManager);
   
protected:

   //
   // Run a frame
   //
   virtual void OnNormalFrame();
   void ScheduledNormalFrame();

   //
   // Handle global database operations
   //
   virtual void DatabaseReset();
   virtual void DatabasePostLoad();

   //
   // Handle an incoming network message.
   //
   void NetworkMessageHandler(const sAINetMsg_Header *netmsg, ObjID from);
   static void NetworkMessageCallback(const sNetMsg_Generic *pMsg, ulong size, ObjID from, void *pClientData);

   // Do initialization once all players have joined a newly loaded level.
   void AllNetPlayersJoined();
   static void AllNetPlayersJoinedCallback(eNetListenMsgs situation,
                                           DWORD data,
                                           void *pClientData);

   // Should an AI being created for this obj be hosted here in a networked game?
   BOOL ShouldHostAI(ObjID objId);

private:

   //
   // Callbacks
   //
   static void DispatchListenFunc(const struct sDispatchMsg* msg, const struct sDispatchListenerDesc* desc); 
   static eDamageResult LGAPI DamageListener(const sDamageMsg* msg, tDamageCallbackData data);
   static void AIWeaponListener(eWeaponEvent ev, ObjID victim, ObjID culprit, void *data);
   static void LGAPI PropListener(sPropertyListenMsg* msg, PropListenerData data);
   static void LGAPI PhysicsListener(ObjID, const sPhysListenMsg*, PhysListenerData);

   // install weapon callbacks
   void AIInitWeaponCallbacks();
   void AITermWeaponCallbacks();
   
   // networking utilities
   const char * NetSelectBehaviorSet(ObjID objId, const char * pszBehaviorSet);
   void NetSetupAIGhost(ObjID objId);
   void NetRemoveAIGhost(ObjID objId);
   void NetInit();
   void NetTerm();
   void TransferDeferred();

   ////////////////////////////////////
   
   BOOL PreFrameValidate();

   ////////////////////////////////////
   
   enum eFlags
   {
      kDeferDestroy = 0x01,
   };

   // Table of live AIs
   cAIArray                m_AIs;
   
   // Flags
   unsigned                m_flags;

   // Component factory table
   cAIBehaviorSets * const m_pBehaviorSets;

   // AIs in line for destruction when destruction has been deferred
   cAIArray                m_Destroys;
   cDynArray<ObjID>        m_SelfSlays;
   
   // AIs in line for transfer when destruction has been deferred
   struct sTransferInfo
   {
      void Set(ObjID who, ObjID where) { ai = who; destination = where; }
      
      ObjID ai;
      ObjID destination;
   };
   
   cDynArray<sTransferInfo> m_Transfers;
   
   ////////////////////////////////////
   //
   // Properties presently listening to
   //
   cDynArray<IProperty *>  m_ListenedProperties;

   ////////////////////////////////////

   cAIConversationManager* m_pConversationManager;

   ////////////////////////////////////
   //
   // Top-level AI scheduling
   //
   
   /*
   enum eScheduleFlags
   {
      kSF_Visible    = 0x01,
      kSF_Audible    = 0x02,
      
      kSF_Rendered   = (kSF_Visible | kSF_Audible),
      
      kSF_Refused    = 0x04
   };
   */

   enum eScheduleTimes
   {
      kST_MinimumRenderedRun = 5000,
      kST_MinimumRefusalRun  = 10000,
   };
   
   struct sAIScheduleInfo
   {
      cAI *    pAI;
      unsigned lastRun;
      unsigned lastRender;
      unsigned lastRefusal;
      //unsigned flags;
      
      void Init(cAI * pInitAI);
      BOOL IsRunRequired() const;
   };

   friend struct sAIScheduleInfo; // to see enum

   unsigned m_FrameStartTime;
   cDynArray<sAIScheduleInfo> m_Schedule;
   static int AIScheduleSort(sAIScheduleInfo * const * ppLeft, sAIScheduleInfo * const * ppRight);

   ////////////////////////////////////

   PhysListenerHandle m_physListenerHandle;

   ////////////////////////////////////

   tNetMsgHandlerID m_NetMsgHandlerID; // Used for the first byte of AI network messages.

   cDynArray<ObjID> m_NetHostedAIs;  // AI's hosted on this machine in a network game.

   cNetMsg *m_pTransferAIMsg;

   ////////////////////////////////////

   tSimTime m_ignoreSoundTime;   // Ignore all sounds until this time
};

///////////////////////////////////////

inline cAI * cAIManager::AccessAI(ObjID objId)
{
   int i = GetAIIndex(objId);
   
   if (i != kAINoIndex)
      return m_AIs[i];

   return NULL;
}

///////////////////////////////////////

inline const cAIArray & cAIManager::AccessAIs() const
{
   return m_AIs;
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIMAN_H */
