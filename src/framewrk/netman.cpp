/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/netman.cpp,v 1.119 1999/10/20 16:43:10 Justin Exp $
//
// NetMan.cpp
//

#include <net.h>
#include <appagg.h>
#include <aggmemb.h>
#include <netman.h>
#include <inetstat.h>
#include <netloop.h>
#include <netprops.h>
#include <netnotif.h>
#include <netapp.h>

#include <lg.h>
#include <mprintf.h>
#include <config.h>
#include <lgassert.h>
#include <cfgdbg.h>   // for ConfigSpew
#include <timer.h>    // for tm_get_millisec

#include <dynarray.h> // for cDynArray
#include <dlist.h>
#include <dlisttem.h>

#include <iobjsys.h>
#include <iobjnet.h>
#include <objdef.h>   // for kObjectConcrete
#include <loopmsg.h>
#include <loopapi.h>  // for CreateSimpleLoopClient
#include <simtime.h>  // for GetSimTime()
#include <dispbase.h> // for kMsgAppInit
#include <objloop.h>  // for LOOPID_ObjSyscls
#include <gamemode.h> // for LOOPID_GameMode

#include <uiloop.h>   // for LOOPID_UI
#include <physloop.h> // for LOOPID_Physics
#include <propman.h>  // for GetPropertyNamed()
#include <posprop.h>
#include <propert_.h> // used for gm_NetPlayerProp
#include <dataops_.h> // for cClassDataOps
#include <proplist.h> // for cListPropertyStore
#include <wrtype.h>   // for Position type
#include <playrobj.h> // for PlayerObject()
#include <dbasemsg.h> // for kMsgDatabase && kDatabaseReset
#include <command.h>  // for COMMANDS
#include <hashpp.h>   // for cHashTable
#include <physapi.h>
#include <traitman.h> // for SetArchetype
#include <simstate.h> // for SimStateCheckFlags()
#include <simflags.h> // for kSimPhysics
#include <doorphys.h> // for IsDoor()
#include <avatar.h>   // for cAvatar
#include <vernum.h>   // for VersionNum
#include <version.h>  // for AppVersion()
#include <tagfile.h>
#include <contain.h>  // for the PlayerContents message
#include <biploop.h>  // for LOOPID_Biped
#include <status.h>   // for Status()
#include <dwordset.h> // for cDWORDSet
#include <objhp.h>    // for ObjGet|Set[Max]HitPoints
#include <netvoice.h>
#include <objpos.h>   // attempt to put avatars in the right place...
#include <ghostapi.h>
#include <rendprop.h> // for ObjSetHasRefs()

#include <netmsg.h>

// Include these absolutely last
#include <dbmem.h>

const tNetPlayerID NULL_NET_ID = DPID_UNKNOWN;

const int MAX_MESSAGE_SIZE = (8 * 1024);

const char *DEFAULT_AVATAR_NAME = "Default Avatar";

////////////////////////////////////////
//
// RAW NETWORK MESSAGES
//
// These are the only messages that need to be sent in "raw" form, rather
// than through the usual cNetMsg mechanism...
//

// Enumation of the types of raw network messages.
typedef enum eNetMessageEnum {
   // Sent to new players from existing players:
   kNetMsg_Hi,
   // Sent during synch, when we're ready to announce ourself:
   kNetMsg_CreatePlayer,
   // A collection of messages, sent as a bundle:
   kNetMsg_Bundle,
   // Sent when we quit out of the game:
   kNetMsg_Quit,
};
typedef char eNetMessageType;

struct sNetMsg_NetGeneric {
   tNetMsgHandlerID handlerID;
   eNetMessageType type;
};

// Message from the default host to new players. This *must* be handled
// through primitive mechanisms, rather than through cNetMsg; there just
// isn't enough information yet for cNetMsg.
// If this client has been refused, then the player num will be the
// negative of the eNetListenRejectReasons enum for the reason.
struct sNetMsg_Hi {
   tNetMsgHandlerID handlerID;
   eNetMessageType type;
   char yourPlayerNum;
   VersionNum hostVersion;
};

// The message used for creating the player. This can't be done through
// cNetMsg, because it needs to get at the actual DPID.
struct sNetMsg_CreatePlayer {
   tNetMsgHandlerID handlerID;
   eNetMessageType type;
   NetObjID abstractPlayer;    // the abstract object to use for creation.
   NetObjID realPlayer;   // the actual host player objID
   BOOL isHost;
   mxs_vector loc;
   mxs_ang tz;
   char playerNum; // We assume no more than 255 players
};

// A bunch of messages, aggregated together.
struct sNetMsg_Bundle {
   tNetMsgHandlerID handlerID;
   eNetMessageType type;
   char block[1];  // So that the compilers don't complain
};

// The message used to quit. This can't be done through cNetMsg because
// it may happen after that mechanism has shut down, and because we need
// to block for this one -- it's the only blocking message in the whole
// system. We have to block because DPlay will shut down immediately after
// we send it, so the "guaranteed" packet isn't actually guaranteed if
// we send it asynch.
struct sNetMsg_Quit {
   tNetMsgHandlerID handleID;
   eNetMessageType type;
};

////////////////////////////////////////
//
// NETWORK MESSAGES
//
// The Network Manager has several messages that it uses itself. These are
// all defined here.
//
// Note that most of the handlers are down at the bottom of this file,
// not for any especially good reason, but just as a workaround to the
// fact that I'm having trouble dealing with the forward references to
// cNetManager...
//

class cNetManager;

//////////
//
// Player-contents message, so that each player can tell the others what
// content he has.
//
static sNetMsgDesc PlayerContentsDesc = {
   kNMF_MetagameBroadcast,
   "PlyrCont",
   "Player Contents",
   NULL,
   NULL,
   {{kNMPT_SenderObjID, kNMPF_None, "Contained Obj"},
    {kNMPT_End}}
};

class cPlayerContentsMsg : public cNetMsg
{
public:
   cPlayerContentsMsg(sNetMsgDesc *pDesc)
      : cNetMsg(pDesc, NULL)
   {}

   // This message tells us that this object is now in the player's
   // inventory.
   void InterpretMsg(ObjID fromPlayer)
   {
      ObjID obj = GetParam(0);
      AutoAppIPtr(ObjectNetworking);
      AssertMsg3(fromPlayer == pObjectNetworking->ObjHostPlayer(obj),
                 "Player %d contains object %d, but I think %d owns it!",
                 fromPlayer,
                 obj,
                 pObjectNetworking->ObjHostPlayer(obj));
      // Under a few circumstances (mainly the beginning of the game), we
      // can be told about an object that we think still has physics. Make
      // sure it doesn't:
      if (PhysObjHasPhysics(obj))
         PhysDeregisterModel(obj);
      AutoAppIPtr(ContainSys);
      pContainSys->Add(fromPlayer, obj, 0, 0);
   }
};

//////////
//
// Finish-synchronize message, which tells the other players that we
// are done with our synchronization, and ready to enter the game.
// Message handler is at the bottom.
//
static void handleFinishSynchMsg(cNetManager *pNetManager);

struct sNetMsgDesc sFinishSynchDesc =
{
   kNMF_MetagameBroadcast,
   "FinishSynch",
   "Finished Synch Messages",
   NULL,
   handleFinishSynchMsg,
   {{kNMPT_End}}
};

//////////
//
// Start-synch message, which tells the other players that I have
// reset my database, and it's now safe to send synch info.
// Message handler is at the bottom.
//
static void handleStartSynchMsg(cNetManager *pNetManager);

struct sNetMsgDesc sStartSynchDesc =
{
   kNMF_MetagameBroadcast,
   "StartSynch",
   "Start Synchronization",
   NULL,
   handleStartSynchMsg,
   {{kNMPT_End}}
};

//////////
//
// SynchFlush message
//
// Says that this player is finished flushing buffers.
//
static void handleSynchFlushMsg(int round, int numMsgs,
                                cNetManager *pNetManager);

struct sNetMsgDesc sSynchFlushDesc =
{
   kNMF_MetagameBroadcast,
   "SynchFlush",
   "Finished Flushing Buffer",
   NULL,
   handleSynchFlushMsg,
   {{kNMPT_Int, kNMPF_None, "Round"},
    {kNMPT_Int, kNMPF_None, "Msgs Sent This Round"},
    {kNMPT_End}}
};

//////////
//
// Player Info message
//
// Tells the other players about this one. This is probably sent more
// frequently that it needs to be, just to be safe.
//
static void handlePlayerInfoMsg(int playerNum,
                                const char *pName,
                                const char *pAddress,
                                ObjID player,
                                cNetManager *pNetManager);

struct sNetMsgDesc sPlayerInfoDesc =
{
   kNMF_MetagameBroadcast | kNMF_AppendSenderID,
   "PlayerInfo",
   "Player Info",
   NULL,
   handlePlayerInfoMsg,
   {{kNMPT_Int, kNMPF_None, "Player Number"},
    {kNMPT_String, kNMPF_None, "Name"},
    {kNMPT_String, kNMPF_None, "Address"},
    {kNMPT_End}}
};

////////////////////////////////////////

// State record for use by the network loop client
typedef struct _StateRecord
{
   cNetManager *netman;
} StateRecord;

typedef cHashTableFunctions<ObjID> ObjIDHashFunctions;
typedef cHashTable<ObjID, ObjID, ObjIDHashFunctions> cNetObjIDTable;

// for m_Parsers
typedef struct sMessageParserInfo {
   tNetMessageParser parser;
   Label moduleName;
   eNetHandlerFlags flags;
   ulong numReceived;
   ulong bytesReceived;
   void *clientData;
} sMessageParserInfo;

#ifdef PLAYTEST

enum _eHistoType
{
   kHistoSend    = 0x0001,
   kHistoReceive = 0x0002,
};
typedef int eHistoType;

struct sHistoSlotInfo
{
   int index;
   ulong bytes;
};

struct sHistoSlot
{
   ulong count;
   cDynArray<sHistoSlotInfo> info;
};

struct sHistogram
{
   cDynClassArray<sHistoSlot> slots;
   cDynArray<ulong> indexAccum;
   ulong update_accum;

   ulong update_rate;
   ulong slot_size;
   BOOL  active;
};

#endif

////////////////////////////////////////
//
// TimeTable
//
// Simple little utility table, for tracking what messages are outstanding,
// and their latency. Stolen directly from cDWORDSet.
//
class TimeTable {
public:
   TimeTable() {}
   ~TimeTable() {}

   void Add(DWORD val, ulong time) { Set.Set(val, time); }
   BOOL InSet(DWORD val, ulong *pTime) { return Set.Lookup(val, pTime); }
   void Delete(DWORD val) { Set.Delete(val); }
   int Size() { return Set.nElems(); }
   void ClearAll() { Set.Clear(); }

private:
   typedef cScalarHashFunctions<DWORD> cSetHashFns;
   typedef cHashTable<DWORD, ulong, cSetHashFns> cSetTable;
   cSetTable Set;
};

////////////////////////////////////////
//
// MESSAGE BUNDLING
//
// The following mechanisms are designed to allow us to bundle messages
// together, sending a single larger packet instead of lots of little
// ones. The principal motivation is to avoid strange anomalies during
// game mode, so that operations that are naturally grouped in the
// engine will come through as a group. (For example, object creation
// typically involves several related messages.) This also reduces
// bandwidth mildly, although not dramatically.
//
// We keep a separate bundle list for each player, and duplicate the
// messages that are broadcast. At first blush this may seem wasteful,
// but we can't keep a single bundle list for broadcasts, because of
// message-order issues. If broadcasts are interspersed with direct
// sends, we need to make sure that message order is preserved, which
// mandates entirely separate lists. However, we do use the same
// message object for all of the lists, keeping the overhead down a
// little.
//
// The actual control code is below, in NetManager itself; these are
// the classes involved.
//

//
// A single message
//
// A pending message is represented by this structure. Note that it is
// refcounted; that is so that the same message can be easily used on
// multiple players' pending message lists.
//
class cBundledMessage
{
public:
   cBundledMessage(const char *msg, ulong size)
      : m_Size(size),
        m_Refs(1)
   {
      AssertMsg(size > 0, "Can't have zero-length net messages!");
      m_Msg = (char *) malloc(size);
      memcpy(m_Msg, msg, size);
   }

   ~cBundledMessage()
   {
      free(m_Msg);
   }

   void AddRef()
   {
      m_Refs++;
   }

   int Release()
   {
      m_Refs--;
      if (m_Refs == 0)
      {
         delete this;
         return 0;
      } else {
         return m_Refs;
      }
   }

   char *m_Msg;
   ulong m_Size;
   int m_Refs;
};

typedef cContainerDList<cBundledMessage *, 1> cBundledMessageList;
typedef cContDListNode<cBundledMessage *, 1> cBundledMessageNode;

//
// The list of players
//
// We keep a data structure for each player, giving that player's current
// message-bundle situation. For now, we're just managing the players as
// a DList, on the assumption that the number of players is small -- if
// we're only dealing with up to three players (the limit as of this
// writing), then a hashtable is simply excess overhead.
//
class cPlayerMsgBundle;
typedef cDListNode<cPlayerMsgBundle, 1> cPlayerMsgBundleNode;

class cPlayerMsgBundle : public cPlayerMsgBundleNode
{
public:
   cPlayerMsgBundle(DPID me)
      : m_DPID(me),
        m_NumMsgs(0),
        m_TotalMsgSize(0)
   {}

   ~cPlayerMsgBundle()
   {
      Clear();
   }

   void AddMsg(cBundledMessage *pMsg)
   {
      pMsg->AddRef();
      cBundledMessageNode *pNode = new cBundledMessageNode;
      pNode->item = pMsg;
      m_Msgs.Append(pNode);
      m_NumMsgs++;
      m_TotalMsgSize += pMsg->m_Size;
   }

   void Clear()
   {
      // Clear out any pending messages for this player:
      cBundledMessageNode *pNode = m_Msgs.GetFirst();
      cBundledMessageNode *pNext;
      cBundledMessage *pMsg;
      while (pNode)
      {
         pMsg = pNode->item;
         pMsg->Release();

         pNext = pNode->GetNext();
         m_Msgs.Remove(pNode);
         delete pNode;
         pNode = pNext;
      }

      m_NumMsgs = 0;
      m_TotalMsgSize = 0;
   }

   DPID m_DPID;
   int m_NumMsgs;
   ulong m_TotalMsgSize;
   cBundledMessageList m_Msgs;
};

class cPlayerMsgBundleList : public cDList<cPlayerMsgBundle, 1>
{
public:
   ~cPlayerMsgBundleList()
   {
      DestroyAll();
   }

   // Add a player to this list:
   void AddPlayer(DPID player)
   {
      if (GetPlayer(player) == NULL)
      {
         cPlayerMsgBundle *pBundle = new cPlayerMsgBundle(player);
         Append(pBundle);
      }
   }

   cPlayerMsgBundle *GetPlayer(DPID player)
   {
      cPlayerMsgBundle *pBundle = GetFirst();
      while (pBundle)
      {
         if (pBundle->m_DPID == player)
            return pBundle;
         else
            pBundle = pBundle->GetNext();
      }
      return NULL;
   }

   // Remove a player from this list:
   void KillPlayer(DPID player)
   {
      cPlayerMsgBundle *pBundle = GetPlayer(player);
      if (pBundle)
      {
         Remove(pBundle);
         delete pBundle;
      }
   }
};

////////////////////////////////////////

//////////
//
// Message Storage
//
// Under certain circumstances, we have to store incoming messages until
// the system is ready to process them. This is a very simple message-storage
// list for that purpose. It stores raw messages, that have just been
// received from DirectPlay.

class cStoredMessage;
typedef cDList<cStoredMessage, 1> cStoredMessageList;
typedef cDListNode<cStoredMessage, 1> cStoredMessageNode;

class cStoredMessage : public cStoredMessageNode
{
public:
   cStoredMessage(DPID from, const char *msg, ulong size)
      : m_From(from),
        m_Size(size)
   {
      AssertMsg(size > 0, "Can't have zero-length net messages!");
      m_Msg = (char *) malloc(size);
      memcpy(m_Msg, msg, size);
   }

   ~cStoredMessage()
   {
      free(m_Msg);
   }

   DPID m_From;
   char *m_Msg;
   ulong m_Size;
};

//////////
//
// PlayerInfo
//
// This is some basic information about each player, organized by player
// number. It is available very early in the system, and is intended for
// metagame use. When we switch over to using player numbers more widely,
// this should probably get beefed up.
//
// @TBD: We currently have a perfectly ridiculous number of mappings
// for players. Besides this PlayerInfo table, there's the IAvatar, the
// PlayerInfo property, the cPlayerMsgBundle, and ObjID stored in the
// DPlay info. It's a real mess to work with. All of these should be
// unified into one structure, which is properly save/loaded, and is
// efficiently indexed into by all three indexes (DPID, playerNum, and
// ObjID).
//

//
// NOTES TOWARDS A BETTER PLAYER MECHANISM:
//
// Currently, the concept of "player" in netman is an unholy mess. We have
// a *lot* of different mechanisms that are all dancing around the same
// idea. There's the cPlayerMsgBundle; the cPlayerInfoTable; the cAvatar;
// the sNetPlayer property; the m_WaitingAvatars table; and probably others
// that I'm forgetting. All have their own notion of the players who are
// in the game, producing a nasty, confusing tangle.
//
// As of this writing (the final push push for Shock), I don't think we
// can afford the risk of redoing this properly. But having just analyzed
// the problem, I want to record my conclusions here, where they will be
// staring me in the face until I deal with them. This should be considered
// high-priority restructuring work, when we have a chance.
//
// What we really want is a proper cPlayerInfo structure, which will more
// or less subsume all of these old concepts. This will contain the
// player's name and IP address; his DPID, ObjID, and playerNumber, as
// far as those are yet known; and his message bundle. This structure
// should probably be defined in a separate file, both to reduce the
// size of netman and to force me to think more clearly about the
// interface. The individual elements, including cAvatar, will go away.
//
// We should probably replace all of the indexes to players with pointers
// to this structure. Certainly the NetPlayer property should just point
// to it. (The reason to not just put all of these elements into the
// property is because the net player can conceptually exist well before
// we have an objsys avatar to attach him to.) Also, the gm_Net local
// info *may* want to become a pointer to this. (Currently, it's the
// player's ObjID; we should consider the merits of making this
// consistent.)
//
// The structure should be semi-persistent, picking up the elements of
// persistence that we now have. cAvatar currently saves the player's
// number and objID; along with that, we save the DPID. This triplet
// should be saved out of the new structure; if we do it right, we can
// probably even avoid invalidating old savegames.
//
// We will probably continue to index into this in all three dimensions.
// Besides the property and gm_Net DPID pointer (both of which become
// slightly optional; we should consider their value), we will keep the
// array table, indexed by playerNum. This should probably be an array
// of pointers, rather than a container; that way, we can quickly see
// whether a given playerNum is filled or not by whether there is a
// pointer.
//
// We will also need a flag for whether this player has fully "connected";
// that is, whether he has synched or not. This will be used to replace
// the current m_WaitingAvatars mechanism with something more consistent,
// and allow us to detect avatars that have been loaded from savegames,
// but which do not correspond to any existing players.
//
// It should also absorbs the "player bundles" defined above, at least
// to some degree. We should think about the appropriate level of
// abstraction here -- whether to hide the details of the bundled
// messages within the cPlayerInfo object, or to more loosely attach
// the concepts of bundles and players. (If we defined a better
// abstraction for bundles, that might make such a loose connection
// make sense.)
//

struct sPlayerInfoEntry {
   char *pName;
   char *pAddress;
   ObjID playerObj;

   sPlayerInfoEntry::sPlayerInfoEntry()
      : pName(NULL),
        pAddress(NULL),
        playerObj(OBJ_NULL)
   {}
};

class cPlayerInfoTable
{
private:
   cDynClassArray<sPlayerInfoEntry> m_Table;

public:

   cPlayerInfoTable::~cPlayerInfoTable()
   {
      int i;
      for (i = 0;
           i < m_Table.Size();
           i++)
      {
         ClearInfo(&(m_Table[i]));
      }
   }

   // Note that this does *not* clear the objID. That's hackish, but
   // intentional.
   void ClearInfo(sPlayerInfoEntry *pEntry)
   {
      if (pEntry->pName != NULL)
      {
         free(pEntry->pName);
         pEntry->pName = NULL;
      }
      if (pEntry->pAddress != NULL)
      {
         free(pEntry->pAddress);
         pEntry->pAddress = NULL;
      }
   }

   void ClearObjIDs()
   {
      int i;
      for (i = 0;
           i < m_Table.Size();
           i++)
      {
         m_Table[i].playerObj = OBJ_NULL;
      }

   }

   void SetInfo(int playerNum, const char *pName, const char *pAddress)
   {
      if (m_Table.IsValidIndex(playerNum))
      {
         ClearInfo(&(m_Table[playerNum]));
      } else {
         // Grow large enough
         m_Table.SetSize(playerNum+1);
      }

      sPlayerInfoEntry *pEntry = &(m_Table[playerNum]);
      if (pName)
      {
         pEntry->pName = (char *) malloc(strlen(pName)+1);
         strcpy(pEntry->pName, pName);
      }
      if (pAddress)
      {
         pEntry->pAddress = (char *) malloc(strlen(pAddress)+1);
         strcpy(pEntry->pAddress, pAddress);
      }
   }

   void SetObjID(int playerNum, ObjID playerID)
   {
      if (!m_Table.IsValidIndex(playerNum))
      {
         // Grow large enough
         m_Table.SetSize(playerNum+1);
      }
      sPlayerInfoEntry *pEntry = &(m_Table[playerNum]);
      pEntry->playerObj = playerID;
   }

   ObjID GetObjID(int playerNum)
   {
      if (m_Table.IsValidIndex(playerNum))
         return m_Table[playerNum].playerObj;
      else
         return OBJ_NULL;
   }

   const char *GetPlayerName(int playerNum)
   {
      const char *ret = NULL;

      if (m_Table.IsValidIndex(playerNum))
         ret = m_Table[playerNum].pName;

      if (ret == NULL)
         return "";
      else
         return ret;
   }

   const char *GetPlayerAddress(int playerNum)
   {
      const char *ret = NULL;

      if (m_Table.IsValidIndex(playerNum))
         ret = m_Table[playerNum].pAddress;

      if (ret == NULL)
         return "";
      else
         return ret;
   }
};

///////////////////////////////////////////////////////////////////////////////
//
// NetPlayer Property.  (private to NetManager).
//
// All information that NetMan needs to associate with net players should be put in
// this property, and not stored using DirectPlay's local player data mechanism.
// Avatar information is stored in the avatar object.
//
////////////////////////////////////////

struct sNetPlayer {
   DPID dpId;         // the DirectPlay ID for this player.
   IAvatar *pAvatar;  // the avatar that represents this player locally
   tSimTime lastBeat; // the timestamp on the last msg from this player
   char name[MAX_PLAYER_NAME_LEN+1];  // this player's name
};

#undef  INTERFACE
#define INTERFACE INetPlayerProperty
DECLARE_PROPERTY_INTERFACE(INetPlayerProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sNetPlayer*); 
}; 

static sPropertyDesc NetPlayerPropDesc =
{
   "NetPlayer",          // Name (No defined constant, since it is a private property)
   kPropertyNoEdit | kPropertyTransient,   // Flags
   NULL, 0, 0, // constraints, versions
   { "Networking", "NetPlayer" },
   kPropertyChangeLocally
};

class cNetPlayerOps : public cClassDataOps<sNetPlayer> {};
typedef cListPropertyStore<cNetPlayerOps> cNetPlayerStore;
typedef cSpecificProperty<INetPlayerProperty, &IID_INetPlayerProperty, sNetPlayer*, cNetPlayerStore> cBaseNetPlayerProp;
class cNetPlayerProp : public cBaseNetPlayerProp
{
public:
   cNetPlayerProp(const sPropertyDesc* desc) : cBaseNetPlayerProp(desc) {};
   ~cNetPlayerProp()
   {
      sPropertyObjIter iter;
      ObjID player;
      sNetPlayer *netinfo;

      IterStart(&iter);
      while (IterNext(&iter, &player))
      {
         Get(player, &netinfo);
         SafeRelease(netinfo->pAvatar);
      }
   }

   STANDARD_DESCRIBE_TYPE(sNetPlayer);
};

#ifdef PLAYTEST
class NetStats
{
   struct FrameStat
   {
      tSimTime time;
      ulong    sent;
      ulong    received;
      int      handler;
   };

   // so we can shadow it from netman
   cDynArray<sMessageParserInfo> *m_pInfoParsers;   

   // m_StatList is a circular whose current position is m_ListPos.
#define STAT_LIST_LENGTH     32
#define STAT_MAX_HANDLER_ID  128
   FrameStat m_StatList[STAT_LIST_LENGTH];
   int m_HandlerBW[STAT_MAX_HANDLER_ID];
   int m_ListPos;  // Current position
   ulong m_SumSent;     // sum of 'sent' is StatList;
   ulong m_SumReceived; // sum of 'received' is StatList;

   // Overhead per message
   int m_PacketOverhead;
   // DPlay's bandwidth estimate, in bytes
   int m_Bandwidth;

   // Frame-tracking stuff
   BOOL m_bGotMsgs;        // TRUE iff we got messages this frame
   BOOL m_bGotGuaranteed;  // TRUE iff we got guaranteed messages
   int m_Frames;           // # of total frames
   int m_MsgFrames;        // # of frames with messages in them
   int m_GuaranteedFrames; // # of frames with guaranteed messages

   // Second-tracking stuff
   BOOL m_bDoHistogram;    // TRUE iff we should print a running histogram
   tSimTime m_LastSecond;  // The last time we ticked off a second
   ulong m_SecSendTotal;   // The bytes sent in the past second
   ulong m_SecRecvTotal;   // the bytes recv this past second
   ulong m_BandwidthCap;   // The theoretical cap on bandwidth, in BPS

   // my dippy histogram stuff
#define HIST_HEIGHT    20  // height of histogram
#define HIST_BINS      70  // number of bins in histogram
#define HIST_BIN_BYTES 32  // number of bytes per bin
   // @TBD: really should add a bin structure
   BOOL m_bHistActive;     // TRUE to do a _histogram_, as opposed to a graph
   int  m_HistBins[HIST_BINS];  // actual bins holding the histogram
   int  m_HistFullestBin;  // index of the fullest bin of all

   // chris' cool histogram stuff
   sHistogram m_HistogramSend;
   sHistogram m_HistogramReceive;

   // A fairly high cap, about as wide as we'd want the histogram to get:
#define DEFAULT_BANDWIDTH_CAP 6000
#define MAX_HISTOGRAM_WIDTH (DEFAULT_BANDWIDTH_CAP / 100)

public:
   BOOL m_bOutputExcel;     // output excel format data
   BOOL m_bOutputExcelFull; // output huge full excel data...
   BOOL m_bOutputExcelPacket; // output each packet
   
   //////////
   //
   // A simple histogram of messages/frame
   //
   BOOL m_bMsgHisto;
#define DEFAULT_MESSAGE_SLOTS 20
   uint m_nMsgSlots;
#define MSG_HISTO_WIDTH 50
   struct sMsgsPerFrameHisto
   {
      // Size is m_bMsgSlots, plus one at the end for excessively
      // crowded frames:
      cDynArray<uint> slots;

      void Inc(uint slotNum)
      {
         uint max = slots.Size() - 1;

         // The top slot gets all overflow:
         if (slotNum > max)
            slots[max]++;
         else
            slots[slotNum]++;
      }

      void Clear(int size)
      {
         slots.SetSize(size);
         int i;
         for (i = 0; i < size; i++)
            slots[i] = 0;
      }
   };
   sMsgsPerFrameHisto m_MsgsG;
   sMsgsPerFrameHisto m_MsgsNG;
   sMsgsPerFrameHisto m_MsgsGandNG;
   uint m_nFrameGMsgs;
   uint m_nFrameNGMsgs;

   void ResetMsgHisto()
   {
      m_bMsgHisto = config_is_defined("net_msg_histogram");
      if (m_bMsgHisto)
      {
         if (!config_get_int("net_msg_slots", &m_nMsgSlots))
            m_nMsgSlots = DEFAULT_MESSAGE_SLOTS;
         m_MsgsG.Clear(m_nMsgSlots + 1);
         m_MsgsNG.Clear(m_nMsgSlots + 1);
         m_MsgsGandNG.Clear(m_nMsgSlots + 1);
         m_nFrameGMsgs = 0;
         m_nFrameNGMsgs = 0;
      }
   }

   // Should be called for each message sent:
   void IncMsgHisto(BOOL guaranteed)
   {
      if (m_bMsgHisto)
      {
         if (guaranteed)
            m_nFrameGMsgs++;
         else
            m_nFrameNGMsgs++;
      }
   }

   // Should be called at the end of each frame:
   void UpdateMsgHisto()
   {
      if (m_bMsgHisto)
      {
         m_MsgsG.Inc(m_nFrameGMsgs);
         m_MsgsNG.Inc(m_nFrameNGMsgs);
         if (m_nFrameGMsgs && m_nFrameNGMsgs)
            m_MsgsGandNG.Inc(m_nFrameGMsgs + m_nFrameNGMsgs);
         else
            m_MsgsGandNG.Inc(0);
         m_nFrameGMsgs = 0;
         m_nFrameNGMsgs = 0;
      }
   }

   void DumpMsgHisto(int which)
   {
      switch (which)
      {
         case 0:
            // Dump all of 'em, through the beauty that is recursion:
            DumpMsgHisto(1);
            DumpMsgHisto(2);
            DumpMsgHisto(3);
            break;
         case 1:
            mprintf("Guaranteed Messages Per Frame:\n");
            DumpSpecificMsgHisto(&m_MsgsG);
            break;
         case 2:
            mprintf("Non-Guaranteed Messages Per Frame:\n");
            DumpSpecificMsgHisto(&m_MsgsNG);
            break;
         case 3:
            mprintf("Guaranteed and Non-Guaranteed Messages Per Frame:\n");
            DumpSpecificMsgHisto(&m_MsgsGandNG);
            break;
      }
   }

   void DumpSpecificMsgHisto(sMsgsPerFrameHisto *pHisto)
   {
      int slot;
      // The overflow slot:
      int max = pHisto->slots.Size() - 1;

      // First, calculate the histogram's width:
      int maxFrames = 0;
      for (slot = 1;
           slot <= max;
           slot++)
      {
         if (pHisto->slots[slot] > maxFrames)
            maxFrames = pHisto->slots[slot];
      }
      // Sanity check:
      if (maxFrames == 0)
      {
         mprintf("   (No relevant frames)\n");
         return;
      }
      float dotSize = ((float) maxFrames) / MSG_HISTO_WIDTH;
      mprintf("   (One * equals %g frames)\n", dotSize);

      // Just print the zero line without the histo bar:
      mprintf("%4d:  (%6d)\n", 0, pHisto->slots[0]);

      int frames;
      float width;
      int i;
      // Print out all of the slots:
      for (slot = 1;
           slot < max;
           slot++)
      {
         frames = pHisto->slots[slot];
         mprintf("%4d:  (%6d) ", slot, frames);
         width = frames / dotSize;
         for (i = 0; i < width; i++)
            mprintf("*");
         mprintf("\n");
      }

      // Print out the overflow slot:
      frames = pHisto->slots[max];
      mprintf("%4d+: (%6d) ", max, frames);
      width = frames / dotSize;
      for (i = 0; i < width; i++)
         mprintf("*");
      mprintf("\n\n");
   }

   //////////

   NetStats(cDynArray<sMessageParserInfo> *pParsers)
   {
      memset(m_HistBins,0,sizeof(m_HistBins));
      memset(m_StatList,0,sizeof(m_StatList));
      memset(m_HandlerBW,0,sizeof(m_HandlerBW));
      m_ListPos=m_SumSent=m_SumReceived=m_Frames=m_MsgFrames=m_GuaranteedFrames=0;
      m_PacketOverhead=m_Bandwidth=m_LastSecond=m_SecSendTotal=m_SecRecvTotal=0;
      m_HistFullestBin=m_bHistActive=m_bOutputExcel=m_bOutputExcelFull=m_bOutputExcelPacket=0;
      m_bDoHistogram = config_is_defined("net_bandwidth_histogram");
      m_bHistActive = config_is_defined("net_actual_histogram");
      m_BandwidthCap = DEFAULT_BANDWIDTH_CAP;
      if (config_get_int("net_bandwidth_cap", &m_BandwidthCap))
         if (m_BandwidthCap > DEFAULT_BANDWIDTH_CAP)  // dont get too big
            m_BandwidthCap = DEFAULT_BANDWIDTH_CAP;
      m_pInfoParsers=pParsers;
      ResetMsgHisto();
   }

   void SetOverhead(int overheadBytes, int bandwidth)
   {
      m_PacketOverhead = overheadBytes;
      m_Bandwidth = (bandwidth * 100) / 8;
   }

   ///////////////
   // chris' fancy histogram stuff
   void AddHistogramPacket(BOOL send, int handler, ulong bytes)
   {
      sHistogram *pUseHisto = send ? &m_HistogramSend : &m_HistogramReceive;

      // grow accum if necessary
      if (handler >= pUseHisto->indexAccum.Size())
      {
         int old_index = pUseHisto->indexAccum.Size();
         pUseHisto->indexAccum.SetSize(handler + 1);

         for (int i=old_index; i<=handler; i++)
            pUseHisto->indexAccum[i] = 0;
      }

      pUseHisto->indexAccum[handler] += bytes;
   }

   void ResetHistogram(int which)
   {
      if (which & kHistoSend)
         ResetSpecificHistogram(&m_HistogramSend);
      if (which & kHistoReceive)
         ResetSpecificHistogram(&m_HistogramReceive);
   }

   void ResetSpecificHistogram(sHistogram *pHisto)
   {
      pHisto->update_accum = 0;

      pHisto->update_rate = 1000;
      pHisto->slot_size   = 50;

      config_get_int("net_histo_rate", &pHisto->update_rate);
      config_get_int("net_histo_slot", &pHisto->slot_size);

      pHisto->active = config_is_defined("net_dump_histogram");

      pHisto->slots.SetSize(0);
      pHisto->indexAccum.SetSize(0);
   }

   void UpdateHistograms()
   {
      UpdateHistogram(&m_HistogramSend);
      UpdateHistogram(&m_HistogramReceive);
   }

   void UpdateHistogram(sHistogram *pHisto)
   {
      if (!pHisto->active)
         return;

      pHisto->update_accum += GetSimFrameTime();
      
      if (pHisto->update_accum > pHisto->update_rate)
      {
         ulong total = 0;
         int i, j;

         // Build up the total time we've spent this second
         for (i=0; i<pHisto->indexAccum.Size(); i++)
         {
            total += (ulong)((float)pHisto->indexAccum[i] / 
                             ((float)pHisto->update_accum / 1000.0));
         }

         // Find our slot
         sHistoSlot *pSlot;
         int slot_index = total / pHisto->slot_size;

         // Create, and clear new entries
         if (slot_index >= pHisto->slots.Size())
         {
            int old_size = pHisto->slots.Size();
            pHisto->slots.SetSize(slot_index + 1);

            for (j=old_size; j<=slot_index; j++)
               pHisto->slots[j].count = 0;
         }

         pSlot = &pHisto->slots[slot_index];
         pSlot->count++;

         // Distribute the bytes into the slot
         for (i=0; i<pHisto->indexAccum.Size(); i++)
         {
            if (pHisto->indexAccum[i] == 0)
               continue;

            // Scan for our entry
            BOOL found = FALSE;
            for (j=0; j<pSlot->info.Size(); j++)
            {
               if (pSlot->info[j].index == i)
               {
                  found = TRUE;
                  break;
               }
            }

            if (!found)
            {
               sHistoSlotInfo newSlot;

               newSlot.index = i;
               newSlot.bytes = pHisto->indexAccum[i];

               pSlot->info.Append(newSlot);
            }
            else
               pSlot->info[j].bytes += pHisto->indexAccum[i];

            pHisto->indexAccum[i] = 0;
         }

         pHisto->update_accum = 0;
      }
   }

#define HISTO_SCALED_WIDTH 55
#define OTHER_CUTOFF_PCT   3.0
#define HIST_NUM_TO_SHOW   4

   void DumpHistogram(int which)
   {
      if (which & kHistoSend)
         DumpSpecificHistogram(&m_HistogramSend);
      if (which & kHistoReceive)
         DumpSpecificHistogram(&m_HistogramReceive);
   }

   void DumpSpecificHistogram(sHistogram *pHisto)
   {
      // Find max slot hit count, so we can scale accordingly
      int max_count = 0;
      int i, j, k;

      for (i=1; i<pHisto->slots.Size(); i++)
      {
         if (pHisto->slots[i].count > max_count)
            max_count = pHisto->slots[i].count;
      }

      if (max_count == 0)
      {
         if ((pHisto->slots.Size() > 0) && (pHisto->slots[0].count > 0))
             max_count = pHisto->slots[0].count;
         else
            return;
      }

      mprintf("\nBYTES PER SECOND %s HISTOGRAM (1 * = %3g hits)\n",
              pHisto == &m_HistogramSend ? "SENT" : "RECEIVED",
              (float)max_count / (float)HISTO_SCALED_WIDTH);

      // Draw our histogram
      for (i=0; i<pHisto->slots.Size(); i++)
      {
         char buffer[256];

         sprintf(buffer, " %d-%d: ", (i == 0) ? 0 : (pHisto->slot_size * i), 
                 (pHisto->slot_size * (i+1)) - 1);

         while (strlen(buffer) < 12) strcat(buffer, " ");
         mprint(buffer);

         sprintf(buffer, "(%d)", pHisto->slots[i].count);
         while (strlen(buffer) < 8) strcat(buffer, " ");
         mprint(buffer);

         for (j = 0; j < (pHisto->slots[i].count * HISTO_SCALED_WIDTH) / max_count && j<HISTO_SCALED_WIDTH; j++)
            mprintf("*");
         mprintf("\n");
      }

      mprintf("\n");

      // Do our breakdown
      for (i=0; i<pHisto->slots.Size(); i++)
      {
         sHistoSlot *pSlot = &pHisto->slots[i];

         ulong total_bytes = 0;

         for (j=0; j<pSlot->info.Size(); j++)
            total_bytes += pSlot->info[j].bytes;

         if (total_bytes > 0)
         {
            //            mprintf("  Breakdown of %d-%d range:\n", (i == 0) ? 0 : (pHisto->slot_size * i), 
            //      pHisto->slot_size * (i+1) - 1);

            char buf[32];
            sprintf(buf," %4d->", (i == 0) ? 0 : (pHisto->slot_size * i));
            mprint(buf);

            char buffer[256];
            float other_accum = 0.0;

            cDynArray<int> sortMap;

            // Build sort mapping and accum other
            for (j=0; j<pSlot->info.Size(); j++)
            {
               float pct = ((float)pSlot->info[j].bytes / (float)total_bytes) * 100.0;

               if (pct > OTHER_CUTOFF_PCT)
               {
                  for (k=0; k<sortMap.Size(); k++)
                  {
                     if (pSlot->info[j].bytes > pSlot->info[sortMap[k]].bytes)
                     {
                        sortMap.InsertAtIndex(j, k);
                        break;
                     }
                  }
                  if (k == sortMap.Size())
                     sortMap.Append(j);
               }
               else
                  other_accum += pct;
            }

            // print 'em
            int parts_shown=0;
            for (j=0; j<sortMap.Size(); j++)
            {
               float pct = ((float)pSlot->info[sortMap[j]].bytes / (float)total_bytes) * 100.0;

               if (parts_shown++<HIST_NUM_TO_SHOW)
               {
                  sprintf(buffer, "  %2.0f%% %s", pct,
                          (*m_pInfoParsers)[pSlot->info[sortMap[j]].index].moduleName.text);
                  mprint(buffer);
               }
            }
            
            if ((other_accum > 0.0)&&(parts_shown<HIST_NUM_TO_SHOW))
            {
               sprintf(buffer, "  %2.0f%% Other", other_accum);
               mprint(buffer);
            }
            mprintf("\n");
         }
      }
   }

   ///////////
   // pure packet output

   void DoExcelSummary(void)
   {
      mprintf("&nxl %d %d %d %d",m_StatList[m_ListPos].time,m_StatList[m_ListPos].sent,
              m_StatList[m_ListPos].received,m_StatList[m_ListPos].handler);
      if (m_bOutputExcelFull)
         for (int i=0; i<STAT_MAX_HANDLER_ID; i++)
            mprintf(" %d",m_HandlerBW[i]);
      mprintf("\n");
   }

   void DoExcelPacket(BOOL send, int size, BOOL guar, int handler)
   {
      mprintf("&nxlp %c%c %d %d\n",send?'S':'R',guar?'G':'N',size,handler);
   }

   //////////////////////////////
   // draw dumb loser old style histograms
   void DrawHist(void)
   {
      float v_scale=((float)HIST_HEIGHT)/(m_HistBins[m_HistFullestBin]+1.0);
      mono_clear();
      for (int j=0; j<HIST_HEIGHT; j++)
      {
         for (int i=0; i<HIST_BINS; i++)
            if (m_HistBins[i]*v_scale>=(HIST_HEIGHT-j))
               mprintf("X");
            else
               mprintf(" ");
         mprintf("\n");
      }
      mprintf("\nFullest Bin (%d->%d) had %d entries, each bar %d bytes wide\n",
              m_HistFullestBin*HIST_BIN_BYTES,(m_HistFullestBin+1)*HIST_BIN_BYTES,
              m_HistBins[m_HistFullestBin],HIST_BIN_BYTES);
   }

   void ClearHist(void)
   {
      memset(m_HistBins,0,sizeof(m_HistBins));
      m_HistFullestBin=0;
   }

   void ToggleHist(void)
   {
      m_bHistActive=!m_bHistActive;
      if (m_bHistActive&&!m_bDoHistogram)
         m_LastSecond=GetSimTime();
   }

   void ToggleOldHist(void)
   {
      m_bDoHistogram=!m_bDoHistogram;
      if (!m_bHistActive&&m_bDoHistogram)
         m_LastSecond=GetSimTime();
   }

   void UpdateRealHist(tSimTime now)
   {
      ulong bandwidth=((m_SecSendTotal+m_SecRecvTotal) / ((now - m_LastSecond) / 1000.0));
      int bin=bandwidth/HIST_BIN_BYTES;
      if (bin<0) bin=0; else if (bin>=HIST_BINS) bin=HIST_BINS-1;
      m_HistBins[bin]++;
      if (bin!=0)
         if (m_HistBins[bin]>m_HistBins[m_HistFullestBin])
            m_HistFullestBin=bin;
   }

   void CheckSecond(tSimTime now)
   {
      // Only run if we've passed a second boundary:
      if (now > (m_LastSecond + 1000))
      {  // Normalize the bandwidth for the past second or so, and
         // divide by 100, to get the number for the histogram
         ulong normBandwidth = (m_SecSendTotal / ((now - m_LastSecond) / 1000.0));

         if (m_bHistActive)
            UpdateRealHist(now);
         if (m_bDoHistogram)
         {
            ulong hundredBytes  = normBandwidth / 100;

            // Print out the histogram
            mprintf("NETHIST: %-8d: ", now);
            if (normBandwidth > m_BandwidthCap)              // We've simply blown our bandwidth
               mprintf("------ EXCESSIVE BANDWIDTH: %d\n", normBandwidth);
            else
            {
               char spaces[MAX_HISTOGRAM_WIDTH+1];
               memset(spaces,' ',hundredBytes);
               spaces[hundredBytes]='\0';
#if 0            
               for (int i = 0; i < hundredBytes; i++)
                  spaces[i] = ' ';
               spaces[i] = '\0';
#endif            
               mprintf("%s* %d\n", spaces, normBandwidth);
            }
         }
         // Clear for next time
         m_LastSecond = now;
         m_SecSendTotal = m_SecRecvTotal = 0;
      }
   }

   void PickWinningHandler(void)
   {
      int handler=-1, max=0;
      for (int i=0; i<STAT_MAX_HANDLER_ID; i++)
         if (m_HandlerBW[i]>max)
            max=m_HandlerBW[handler=i];  // im so sneaky, woo woo
      m_StatList[m_ListPos].handler=handler;
   }

   void ClearHandlerData(void)
   {
      memset(m_HandlerBW,0,sizeof(m_HandlerBW));
   }

   void NewFrame(void)
   {
      tSimTime now = GetSimTime();

      PickWinningHandler();

      if (m_bOutputExcel||m_bOutputExcelFull)
         DoExcelSummary();

      if (m_bDoHistogram||m_bHistActive)
         CheckSecond(now);

      ClearHandlerData();

      m_ListPos = (m_ListPos+1) % STAT_LIST_LENGTH;
      m_SumSent -= m_StatList[m_ListPos].sent;
      m_SumReceived -= m_StatList[m_ListPos].received;

      m_Frames++;
      if (m_bGotMsgs)
         m_MsgFrames++;
      if (m_bGotGuaranteed)
         m_GuaranteedFrames++;

      // reset per frame stats
      m_bGotMsgs = m_bGotGuaranteed = FALSE;
      m_StatList[m_ListPos].time = now;
      m_StatList[m_ListPos].sent = m_StatList[m_ListPos].received = 0;

      // for the "real" chris histograms
      UpdateHistograms();

      // for the msgs/packet histograms
      UpdateMsgHisto();
   }

   ////////
   // send receive hookup

   void NewSend(ulong bytes, BOOL guaranteed, int handler)
   {
      if (bytes == 0)
         // No actual message. This usually means that we've sent a
         // broadcast, but there are no other players.
         return;

      //bytes += m_PacketOverhead;
      m_StatList[m_ListPos].sent += bytes;
      m_SumSent += bytes;
      if (m_bDoHistogram||m_bHistActive)
         m_SecSendTotal += bytes;
      m_bGotMsgs = TRUE;
      if (guaranteed)
         m_bGotGuaranteed = TRUE;
      if (handler<STAT_MAX_HANDLER_ID)
         m_HandlerBW[handler]+=bytes;
      if (m_bOutputExcelPacket)
         DoExcelPacket(TRUE,bytes,guaranteed,handler);
      AddHistogramPacket(TRUE,handler,bytes);
      IncMsgHisto(guaranteed);
   }

   void NewReceive(ulong bytes, int handler)
   {
      //bytes += m_PacketOverhead;
      m_StatList[m_ListPos].received += bytes;
      m_SumReceived += bytes;
      if (m_bDoHistogram||m_bHistActive)
         m_SecRecvTotal += bytes;      
      if (handler<STAT_MAX_HANDLER_ID)
         m_HandlerBW[handler]+=bytes;
      if (m_bOutputExcelPacket)
         DoExcelPacket(FALSE,bytes,FALSE,handler);
      AddHistogramPacket(FALSE,handler,bytes);
   }

   ///////////////////////////////////
   // acutal output stats
#define NO_CULPRIT "None"   

   // Bandwidth (BW) of sends and receives over TimeSpan. (All are output parameters)
   void BandwidthStats(float *timeSpan, ulong *sendBW, ulong *receiveBW, const char **culprit)
   {
      int dt=(m_StatList[m_ListPos].time - m_StatList[(m_ListPos+1) % STAT_LIST_LENGTH].time);
      *timeSpan = dt/1000.0;
      if (*timeSpan>0)
      {
         *sendBW = m_SumSent / *timeSpan;
         *receiveBW = m_SumReceived / *timeSpan;
      }
      else
         *sendBW = *receiveBW = 0;
      if ((m_Bandwidth != 0) && (*sendBW > m_Bandwidth))
         Warning(("NETSTATS: Overwhelming the bandwidth -- %d/%d!\n", *sendBW, m_Bandwidth));

      // burn through and pick current "best" culprit
      if (culprit)
      {
         int i, max=0, hnd_cnt[STAT_MAX_HANDLER_ID];
         memset(hnd_cnt,0,sizeof(hnd_cnt));
         for (i=0; i<STAT_LIST_LENGTH; i++)
            if (m_StatList[i].handler!=-1)
               hnd_cnt[m_StatList[i].handler]++;
         for (i=0; i<STAT_MAX_HANDLER_ID; i++)
            if (hnd_cnt[i]>hnd_cnt[max])
               max=i;   // lets just say no culprit if they have < half the votes
         if ((max!=0)||(hnd_cnt[max]>(STAT_LIST_LENGTH/2)))  // since 0 is valid, obviously
            *culprit=(*m_pInfoParsers)[max].moduleName.text;
         else
            *culprit=NO_CULPRIT;
      }
   }

   void FrameStats(ulong *bytesSent, ulong *bytesReceived, const char **culprit) 
   {
      int ix = (m_ListPos==0 ? STAT_LIST_LENGTH-1 : m_ListPos-1);
      *bytesSent = m_StatList[ix].sent;
      *bytesReceived = m_StatList[ix].received;
#if 1
      if (culprit)
         if (m_StatList[ix].handler!=-1)
            *culprit=(*m_pInfoParsers)[ix].moduleName.text;
         else
            *culprit=NO_CULPRIT;
#else
      if (culprit) *culprit=NO_CULPRIT;
#endif      
   }

   // max bytes sent/recv in the queue (hack for now)
   void MaxStats(ulong *bytesSent, ulong *bytesReceived)
   {
      int max_s=0, max_r=0;
      for (int i=0; i<STAT_LIST_LENGTH; i++)
      {
         if (m_StatList[i].sent>max_s)
            max_s=m_StatList[i].sent;
         if (m_StatList[i].received>max_r)
            max_r=m_StatList[i].received;
      }
      *bytesSent=max_s;
      *bytesReceived=max_r;
   }

   void DumpTotalStats()
   {
      int which;
      if (m_bMsgHisto && config_get_int("net_msg_dump_histogram", &which))
         DumpMsgHisto(which);
      if (m_Frames>0)
      {
         mprintf("Network traffic sent, broken down by frame --\n");
         mprintf("Total frames: %d\n", m_Frames);
         mprintf("Frames with messages: %d\n", m_MsgFrames);
         mprintf("Frames with guaranteed messages: %d\n", m_GuaranteedFrames);
      }
      else
         mprintf("No networking run, so no stats...\n");
   }
};
#endif // PLAYTEST

// The save/load tag information
static TagFileTag NetTag = {"NETWORKING"};
static TagVersion NetTagVersion = {0, 2};
static TagFileTag AvatarTag = {"AVATARS"};
static TagVersion AvatarTagVersion = {0, 1};

// The timeout interval, after which we give up on a player, in ms.
#define DEFAULT_PLAYER_TIMEOUT 60000

//////////
//
// Listener structure
//
// This defines a single listener; the m_Listeners structure keeps all
// of them.
//
class cNetListener {
public:
   cNetListener(tNetListenerCallback callback,
                eNetListenMsgs interests,
                void *pClientData,
                tNetListenerHandle handle)
      : m_callback(callback),
        m_interests(interests),
        m_pClientData(pClientData),
        m_handle(handle)
   {
   }

   tNetListenerCallback m_callback;
   eNetListenMsgs m_interests;
   void *m_pClientData;
   tNetListenerHandle m_handle;
};

//////////
//
// Spewage
//
// As a rule, use NetSpew for net_spew; that way, net_opt_spew will
// also work. Use NetRawSpew for net_raw_spew of ordinary messages --
// RawSpew is a real method, for spitting out bytestreams.
//

#ifdef PLAYTEST
#define NetSpew(msg) {if (gm_bDoSpew) {mprintf msg;}}
#else
#define NetSpew(msg)
#endif

#ifdef PLAYTEST
#define NetRawSpew(msg) {if (m_bRawSpew) {mprintf msg;}}
#else
#define NetRawSpew(msg)
#endif

///////////////////////////////////////////////////////////
//
// cNetManager
//
// A class for handling multi-player game code.  It uses
// the INet tech library, and also works in conjunction with
// the NetLoop loop client.
//
class cNetManager : public cCTDelegating<INetManager>,
#ifdef PLAYTEST                    
                    public cCTDelegating<INetStats>,
#endif                    
                    public cCTAggregateMemberControl<kCTU_Default>
{
public:
   // This is public, for any static methods that need to get through to
   // the net manager. (Mainly intended for temporary cheats and hacks that
   // I don't want to put into the real interface.)
   static cNetManager *gm_TheNetManager;  // needed by loop client code, which doesn't have 'this'

private:

   // Count of SuspendMessage() calls (suppresses outgoing messages):
   long m_SuspendMessaging;
   // This player's name
   char m_PlayerName[MAX_PLAYER_NAME_LEN+1];
   // Name of networking media to use
   char m_Media[128];
   // Name of session to use
   char m_SessionName[128];
   // Name of the host we are connected to
   char m_HostAddr[128];
   // TRUE iff this game has started, so it's too late to add new players:
   BOOL m_bGameStarted;
   // The number of players who have joined the game:
   ulong m_NumPlayers;
   // Players who have either been rejected or destroyed, listed by DPID:
   cDWORDSet m_DefunctPlayers;
   // How many players have indicated that they have finished
   // resetting their databases? (This is generally zero.)
   ulong m_PlayersReset;
   // How many players have sent their info for play on this level?
   ulong m_PlayersJoined;
   // A number between 1 & m_NumPlayers.  1 is the session host.
   ulong m_MyPlayerNum;
   tSimTime m_MsgFreq;
   // Handlers that parse incoming messages.
   cDynArray<sMessageParserInfo> m_Parsers;
   // Functions to call when something interesting happens:
   cDynArray<cNetListener *> m_Listeners;
   // The most recent listener handle given out:
   tNetListenerHandle m_LastListener;
   // Avatars that have been loaded, but not yet assigned to a player
   // Note that this is a 1-based array, based on player number
   cDynArray<IAvatar *> m_WaitingAvatars;
   // Basic information about each player.
   cPlayerInfoTable m_PlayerInfo;
   // TRUE iff we are dealing with a load instead of a create; this gets
   // set appropriate by the Database loop messages
   BOOL m_bLoadStart;
   // The player who sent the network message we are in the middle of
   // processing; OBJ_NULL if we're not in the middle of processing a
   // message.
   ObjID m_OriginatingPlayer;
   // The limit to the number of players allowed in a game:
   int m_MaxPlayers;
#define DEFAULT_MAX_PLAYERS 4

   // Message objects:
   cPlayerContentsMsg *m_pPlayerContentsMsg;
   cNetMsg *m_pFinishSynchMsg;
   cNetMsg *m_pSynchFlushMsg;
   cNetMsg *m_pStartSynchMsg;
   cNetMsg *m_pPlayerInfoMsg;

#ifdef PLAYTEST
   // Whether or not to do a complete raw spew of messages:
   BOOL m_bRawSpew;
#endif
   
   // The maximum number of messages to process in a single frame:
   int m_frameMsgMax;

   // The lapse before timing out a moribund player:
   int m_playerTimeout;

#ifdef PLAYTEST
   NetStats m_NetStats;
#endif
   // The packet overhead to figure into any message:
   int m_packetOverhead;

   // TRUE iff we are suppressing *all* messages, due to NonNetworkLevel:
   BOOL m_bNonNetworkLevel;
   // TRUE iff we are storing incoming messages, due to NonNetworkLevel.
   // Note that this doesn't become FALSE until we have Synch'ed in a
   // networked level, and processing the StoredMessageQueue:
   BOOL m_bStoringMessages;
   // The list of incoming messages we receive while we are in
   // NonNetworkLevel. Ideally, this should be pretty short. (Just
   // stuff that others players have sent after they left NonNetworkLevel.)
   cStoredMessageList m_StoredMessages;
   // TRUE iff we are finishing up a NonNetworkLevel; indicates that
   // player contents probably don't have remote proxies, so we need to
   // tell the other players to create proxies for them.
   BOOL m_bCreateContentProxies;

   // The set of pending network messages:
   TimeTable m_PendingMsgIDs;

   // The bundles of messages currently pending for various players:
   cPlayerMsgBundleList m_PlayerBundles;
   // Same for non-guaranteed messages:
   cPlayerMsgBundleList m_PlayerBundlesNG;

   // The following are static, because they are needed while running
   // callbacks that don't have a 'this' pointer.
   static BOOL gm_bAmSessionHost;  // TRUE iff this is the session host
   static DPID gm_PlayerDPID;   // This player's DPID.
   static ObjID gm_DefaultHostPlayer; // The player that will be designated the default host.
   static tNetMsgHandlerID gm_NetManagerHandlerID; // for representing the NetManager in messages
   static INetPlayerProperty *gm_NetPlayerProp; // a private property for the net manager
   static IObjectSystem *gm_ObjSys;
   static IObjectNetworking *gm_ObjNet;
   static INet *gm_Net;   // The INet COM Object
   static BOOL gm_bNetworkGame;  // Is this a multi-player game
   static BOOL gm_bNetworkingReady; // Are we ready to send/receive messages at this time?
   // TRUE iff we are currently in the process of synchronizing with
   // the other players:
   static BOOL gm_bSynchronizing;
   // The last known sim time, which we keep track of for efficiency, and
   // so we know the total time at shutdown, after ending the sim:
   static tSimTime gm_SimTime;
   // TRUE iff we're currently in game mode:
   static BOOL gm_bInGameMode;
   // The real time that we spent in game mode:
   static ulong gm_GameTime;
   // The time that we entered game mode most recently:
   static ulong gm_EnteredGameMode;
   // TRUE iff we should be printing net_spews; enables the NetSpew macro
   static BOOL gm_bDoSpew;

   //////////
   //
   // RAW SPEWS
   //
   // This method should be called to spew a pure, low-level output of
   // network messages.
   //
#ifndef SHIP
   void RawSpew(BOOL bSend,
                DPID player,
                ulong msgSize,
                sNetMsg_Generic *msg)
   {
      char buf[17];

      // First, the introduction to the line:
      mprintf("NET: %s %s:",
              (bSend ? "SEND to" : "RECEIVE from"),
              ((player == DPID_ALLPLAYERS) ?
               "all" : 
               _itoa(FromNetPlayerID(player), buf, 10)));
               
      // Next, the name of the message type:
      sMessageParserInfo *info = &m_Parsers[(int)msg->handlerID];
      mprintf(" %s --", info->moduleName.text);

      // Now, the raw message contents:
      int i;
      uchar *pRawMsg = ((uchar *) msg) + 1;
      for (i = 1; i < msgSize; i++, pRawMsg++) 
      {
         mprintf(" %s", _itoa((int) *pRawMsg, buf, 16));
      }
      mprintf("\n");
   }
#endif

   //////////
   //
   // A DirectPlay style callback for EnumPlayers. Used during
   // handling of database reset.
   static BOOL CALLBACK _DestroyNetPlayerCallback(DPID dpId, 
                                                  DWORD dwPlayerType, 
                                                  LPCDPNAME lpname, 
                                                  DWORD dwFlags, 
                                                  LPVOID lpContext)
   {
      cNetManager *This = (cNetManager *)lpContext;
      if (dpId != gm_PlayerDPID)
      {
         ObjID obj = This->FromNetPlayerID((tNetPlayerID)dpId);
         if (obj != OBJ_NULL)
         {
            GhostRemRemote(obj);
            gm_ObjSys->Destroy(obj);
            gm_Net->ResetPlayerData(dpId, NULL);
         }
      }
      return TRUE;
   }

   // What is *my* avatar's archetype?
   //
   // @TBD: this should get pulled out into app-specific code, exposed
   // through the API somehow. It is essentially a hack that it's
   // implemented at this level.
   //
   // @NOTE: This has now been pulled into the netapp interface. The
   // code remains here solely for temporary historical record.
#if 0
   ObjID MyAvatarArchetype()
   {
      char abstractName[128];
      ObjID abstractPlayer = OBJ_NULL;

      // The net_abstract_player is the archetype to be created for
      // our player on other player's machines.
      if (!config_get_raw("net_avatar", 
                          abstractName, 
                          sizeof abstractName))
      {
         // Use the default player
         sprintf(abstractName, "%s %d", DEFAULT_AVATAR_NAME, m_MyPlayerNum);
         abstractPlayer = gm_ObjSys->GetObjectNamed(abstractName);
         AssertMsg1(abstractPlayer,
                    "Default avatar %s not in gamesys!",
                    abstractName);
      } else {
         abstractPlayer = gm_ObjSys->GetObjectNamed(abstractName);
         if (abstractPlayer == OBJ_NULL) {
            Warning(("Unknown abstract player object %s",abstractName));
            // Use the default player
            sprintf(abstractName, "%s %d", DEFAULT_AVATAR_NAME, 
                    m_MyPlayerNum);
            abstractPlayer = gm_ObjSys->GetObjectNamed(abstractName);
            AssertMsg1(abstractPlayer,
                       "Default avatar %s not in gamesys!",
                       abstractName);
         }
      }

      return abstractPlayer;
   }
#endif

   // Broadcast information on how to create this player.
   void SendCreatePlayerNetMsg()
   {
      AutoAppIPtr(NetAppServices);
      ObjID abstractPlayer = pNetAppServices->MyAvatarArchetype();

      Position *pos=ObjPosGet(PlayerObject());  // well, i know ObjPos*=Position*
      Assert_(m_MyPlayerNum < 255);
      sNetMsg_CreatePlayer msg = {gm_NetManagerHandlerID, 
                                  kNetMsg_CreatePlayer,
                                  abstractPlayer,
                                  PlayerObject(),
                                  gm_bAmSessionHost,
                                  { pos->loc.vec.x, 
                                    pos->loc.vec.y, 
                                    pos->loc.vec.z 
                                  },
                                  // players have only tz
                                  pos->fac.tz,
                                  (char) m_MyPlayerNum};

      NetSpew(("SEND: PlayerCreate arch: %d obj: %d %s\n", 
               abstractPlayer, 
               PlayerObject(),
               gm_bAmSessionHost?"Is host":"Is NOT host"));

      MetagameBroadcast(&msg, sizeof(msg), FALSE);

      // If we are the default host, no one else will be telling us
      // that they are the session host, so we should initialize
      // gm_DefaultHostPlayer here.
      if (gm_bAmSessionHost)
         gm_DefaultHostPlayer = PlayerObject();

      GhostAddLocal(PlayerObject(),1.0,kGhostCfPlayer);
   }

   void HandleCreatePlayerMsg(DPID fromDPID, sNetMsg_CreatePlayer *msg)
   {
      ObjID arch;
      if (config_is_defined("net_simple_avatars")) {
         // This player wants to just use the default avatar for all
         // other players. Less cool, but much cheaper in terms of
         // video memory.
         // Use the default player
         arch = gm_ObjSys->GetObjectNamed(DEFAULT_AVATAR_NAME);
         AssertMsg1(arch,
                    "Default avatar %s not in gamesys!",
                    DEFAULT_AVATAR_NAME);

      } else {
         // The abstract object for players must use the same ObjID's on
         // all machines.
         arch = (ObjID)msg->abstractPlayer;  
      }

      // Create the object for the player.
      ObjID playerObj;
      IAvatar *pAvatar=NULL;
      if (m_bLoadStart) {
         // We're in the middle of loading a level. We should already have
         // all of the avatars loaded in, so figure out which one is
         // appropriate for this player:
         pAvatar = m_WaitingAvatars[(ulong) msg->playerNum];
         if (!pAvatar) {
            Warning(("Received create player msg for unknown player!\n"));
            return;
         }
         playerObj = pAvatar->PlayerObjID();
         m_WaitingAvatars[(ulong) msg->playerNum] = NULL;
         
         // mprintf("Yo Yo delayed start case in action\n");

         // In this case, we generally already have a ghost from the load
         // file. Kill it and start again with the new info from the real
         // player:
         GhostChangeRemoteConfig(playerObj, kGhostCfLoaded, TRUE);
         GhostRemRemote(playerObj);

         mxs_vector loc = msg->loc;
         float offset=0.0;
         gAvatarHeightOffsetProp->Get(playerObj,&offset);
         loc.z += offset;
         ObjTranslate(playerObj,&loc);      // an attempt to get the obj in the
         mxs_angvec angvec = { 0, 0, msg->tz };  // right place... unsure about how
         ObjRotate(playerObj,&angvec);           // well this is likely to work out...
         GhostAddRemote(playerObj,1.0,kGhostCfPlayer);
         AssertMsg(gm_ObjSys->Exists(playerObj),
                   "Avatar exists without a player object!");
         NetSpew(("Reused old avatar.\n"));
      } else {
         // @NOTE: The order of events used to be different here; we would
         // create the cAvatar in the middle of creating the object.
         // However, we now don't create the creature itself until EndCreate,
         // so we have to hold off on the cAvatar.
         playerObj = gm_ObjSys->BeginCreate(arch, kObjectConcrete);

         mxs_vector loc = msg->loc;
         float offset=0.0;
         gAvatarHeightOffsetProp->Get(arch,&offset);
         loc.z += offset;
         ObjTranslate(playerObj,&loc);    // set the obj data, so endcreate
         mxs_angvec angvec = { 0, 0, msg->tz };  // goes ahead and creates the 
         ObjRotate(playerObj,&angvec);           // physics objs in the right place
         gm_ObjSys->EndCreate(playerObj);        //    (namely, where the obj is)
         // Replace this line if we change IAvatar implementations:
         pAvatar = new cAvatar(playerObj,msg->playerNum);
         GhostAddRemote(playerObj,1.0,kGhostCfPlayer);
         NetSpew(("Created new avatar.\n"));
      }
      // what does this do?
      gm_ObjNet->ObjRegisterProxy(playerObj, msg->realPlayer, playerObj);

      NetSpew(("RECEIVE: PlayerCreate arch: %d obj: %d proxy: %d %s\n",
               arch, msg->realPlayer, playerObj, 
               msg->isHost ? "Is host" : "Is NOT host"));

      // Now store the avatar for this player
      sNetPlayer nplyr = { fromDPID, pAvatar, 0 };
      gm_NetPlayerProp->Set(playerObj, &nplyr);
      gm_Net->SetPlayerData(fromDPID, (void *)&playerObj, 4, DPSET_LOCAL);

      // See if this is the Default Host
      if (msg->isHost)
         gm_DefaultHostPlayer = playerObj;

      // Check whether there are any remappings waiting for this player
      gm_ObjNet->ResolveRemappings(msg->playerNum, playerObj);
   }

   // Handle an incoming non-system network message for netman
   // In practice, we don't wind up doing much with this; netman msgs all
   // currently get intercepted in HandleRawNetMsg, for various reasons.
   static void HandleNetMsg(const sNetMsg_Generic *genmsg, 
                            ulong size, 
                            ObjID fromPlayer, 
                            void *clientData)
   {
      sNetMsg_NetGeneric *netmsg = (sNetMsg_NetGeneric *)genmsg;
      cNetManager *This = (cNetManager *)clientData;

      switch (netmsg->type) {
         case kNetMsg_Hi:
         case kNetMsg_CreatePlayer:
         case kNetMsg_Bundle:
         {
            CriticalMsg("Create or Hi message should have"
                        "been processed earlier");
            break;
         }
         default:
            Warning(("Unknown player message\n"));
      }
   }

   // This is a filter for messages, to bundle some of them up to send on
   // a frame-by-frame basis. We only bundle up message if we're in game
   // mode, and it's a guaranteed message, currently.
   void SendOrBundle(DPID dpId,
                     void *msg,
                     ulong size,
                     BOOL guaranteed)
   {
      // We are intentionally networking *nothing*, despite the fact that
      // this is a network game:
      if (m_bNonNetworkLevel) {
         return;
      }

      if (!gm_bInGameMode ||
          !gm_bNetworkingReady ||
          m_bSynchFlushing)
      {
         // Don't try to bundle it up, just send it
         SendToDPID(dpId, msg, size, guaranteed, FALSE);

#ifdef PLAYTEST
         int totalSize = size + m_packetOverhead;
         m_NetStats.NewSend(dpId==DPID_ALLPLAYERS 
                            ? totalSize*(m_NumPlayers-1) 
                            : totalSize,
                            guaranteed, 
                            (int)(((sNetMsg_Generic *)msg)->handlerID));
#endif
      } else {
         // Let's bundle, baby! Yeah!
         cBundledMessage *pMsg = new cBundledMessage((char *) msg, size);
         if (dpId == DPID_ALLPLAYERS) {
            // It's a broadcast message, so put it on all the player lists:
            cPlayerMsgBundle *pBundle;
            if (guaranteed)
               pBundle = m_PlayerBundles.GetFirst();
            else
               pBundle = m_PlayerBundlesNG.GetFirst();
            while (pBundle)
            {
               pBundle->AddMsg(pMsg);
               pBundle = pBundle->GetNext();
            }
         } else {
            cPlayerMsgBundle *pBundle;
            if (guaranteed)
               pBundle = m_PlayerBundles.GetPlayer(dpId);
            else
               pBundle = m_PlayerBundlesNG.GetPlayer(dpId);
            AssertMsg1(pBundle,
                       "No message bundle for player %d!",
                       dpId);
            // If this actually arises in practice, let's survive it:
            if (pBundle)
               pBundle->AddMsg(pMsg);
         }
         pMsg->Release();
         NetRawSpew(("Bundled up message to %d\n", dpId));
      }
   }

   // How long to wait on a blocking message. Shouldn't be too long:
#define BLOCK_DELAY 5000
   // Send this message to 'player'
   void SendToDPID(DPID dpId, 
                   void *msg,
                   ulong size, 
                   BOOL guaranteed,
                   BOOL blocking = FALSE)
   {
      // We are intentionally networking *nothing*, despite the fact that
      // this is a network game:
      if (m_bNonNetworkLevel) {
         return;
      }

#ifdef PLAYTEST
      // This is a dumb little experimental flag, not intended for
      // real play, just to evaluate the impact of guaranteeed packets
      // on latency and bandwidth:
      if (config_is_defined("net_no_guaranteed"))
      {
         guaranteed = FALSE;
      }
#endif

      int flags = guaranteed ? DPSEND_GUARANTEED : 0;
      // give guaranteed messages a higher priority
      DWORD priority = 0; // guaranteed ? 1 : 0;
      // All messages need to set the right timeout, because DPlay will
      // die horribly if a message times out. (And it has a default
      // timeout for all messages.)
      DWORD timeout = m_playerTimeout;
      if (blocking) {
         // Don't wait *too* long on a blocking message. Use blocking
         // messages *very* sparingly, only when absolutely necessary:
         timeout = BLOCK_DELAY;
      } else {
         // Async messages don't block.
         flags |= DPSEND_ASYNC;
      }

#ifdef PLAYTEST
      if (m_bRawSpew)
         RawSpew(TRUE, dpId, size, (sNetMsg_Generic *) msg);
#endif

      // Nothing should send a message that is too big to be received
      // by the other side.
      AssertMsg(size < MAX_MESSAGE_SIZE, "Network message too big.");

      //HRESULT hr = gm_Net->Send(gm_PlayerDPID,dpId,flags,msg,size);
      DWORD msgID;
      HRESULT hr = gm_Net->SendEx(gm_PlayerDPID,
                                  dpId,
                                  flags,
                                  msg,
                                  size,
                                  priority,
                                  timeout,
                                  NULL,
                                  &msgID);
      if (hr == DPERR_PENDING) {
         // Record that the message is in progress
         m_PendingMsgIDs.Add(msgID, tm_get_millisec());
         if (m_bSynchFlushing)
            m_round[m_curRound].msgsSentByMe++;
         NetRawSpew(("Msg sent as %x; %d pending\n",
                     msgID,
                     m_PendingMsgIDs.Size()));
      } else if (SUCCEEDED(hr)) {
         NetRawSpew(("Msg sent synchronously\n"));
         if (m_bSynchFlushing)
            m_round[m_curRound].msgsSentByMe++;
      } else {
         Warning(("Send error.  Return value: %x %s\n", 
                  hr, gm_Net->DPlayErrorAsString(hr)));
      }
   }

#ifdef PLAYTEST
   static void StaticResetMsgHistogram()
   {
      gm_TheNetManager->m_NetStats.ResetMsgHisto();
   }

   static void StaticDumpMsgHistogram(int which = 0)
   {
      gm_TheNetManager->m_NetStats.DumpMsgHisto(which);
   }

   static void StaticResetHistogram(int which = 0)
   {
      if (which == 0)
         which = kHistoSend | kHistoReceive;

      gm_TheNetManager->m_NetStats.ResetHistogram(which);
   }

   static void StaticDumpHistogram(int which = 0)
   {
      if (which == 0)
         which = kHistoSend | kHistoReceive;

      gm_TheNetManager->m_NetStats.DumpHistogram(which);
   }
#endif   

   static void ClearNetStats(void)
   {
      for (int i=0; i < gm_TheNetManager->m_Parsers.Size(); i++)
      {
         sMessageParserInfo *info = &gm_TheNetManager->m_Parsers[i];
         info->numReceived=info->bytesReceived=0;
      }
      if (gm_bInGameMode)
         gm_EnteredGameMode=tm_get_millisec();
      else
         gm_GameTime=1;
   }

   static void DumpNetStats(void)
   {
#ifdef PLAYTEST
      ulong sumNum=0;
      ulong sumSize=0;

      // If we never left game mode, close off the timer:
      if (gm_bInGameMode)
         gm_GameTime += (tm_get_millisec() - gm_EnteredGameMode);

      if (gm_GameTime<=1)
         return;  // haha - no stats if totally bogus, really

      mprintf("Seconds spent in network game mode: %d (%d)\n", gm_GameTime/1000, gm_GameTime);
      mprintf("Data Received in this Session --\n");
      mprintf("Handler                     NumMsgs BytesReceived  Bytes/Sec\n");
      for (int i=0; i < gm_TheNetManager->m_Parsers.Size(); i++)
      {
#define NAME_SPACE_COUNT 20
         sMessageParserInfo *info = &gm_TheNetManager->m_Parsers[i];
         char space_buf[NAME_SPACE_COUNT];
         memset(&space_buf,' ',NAME_SPACE_COUNT);
         if (NAME_SPACE_COUNT<=strlen(info->moduleName.text))
            space_buf[NAME_SPACE_COUNT-1]='\0';
         else
            space_buf[NAME_SPACE_COUNT-strlen(info->moduleName.text)]='\0';  
         // 7 13 9??? what is up with that? - i mean, why these???
         mprintf("%2.2d %s%s %7d  %13d  %9d\n", 
                 i,
                 info->moduleName.text,
                 space_buf,
                 info->numReceived,
                 info->bytesReceived,
                 (int)(info->bytesReceived*1000.0/gm_GameTime));
         sumNum += info->numReceived;
         sumSize += info->bytesReceived;
      }
      // im gonna be super lame and just put spaces in here... so shoot me
      mprintf("Total:                  %7d  %13d  %9d\n",
              sumNum,
              sumSize,
              (int)(sumSize*1000/gm_GameTime));
      gm_TheNetManager->m_NetStats.DumpTotalStats();
#endif // playtest
   }

   static void CmdHost()
   {
      gm_TheNetManager->Host(NULL, NULL);
   }

   static void CmdClient(char *pAddress)
   {
      gm_TheNetManager->Join(NULL, NULL, pAddress);
   }

   static void CmdLeave()
   {
      gm_TheNetManager->Leave();
   }

#ifndef SHIP
   static void CmdState()
   {
      if (gm_TheNetManager->IsNetworkGame()) {
         if (gm_TheNetManager->AmDefaultHost()) {
            char temp[40];
            sprintf(temp, 
                    "Am host to %d players", 
                    gm_TheNetManager->NumPlayers());
            Status(temp);
         } else {
            char temp[128];
            sprintf(temp,
                    "Client to %s", gm_TheNetManager->m_HostAddr);
            Status(temp);
         }
      } else {
         Status("Not networked");
      }
   }

   static void CmdRawSpewOn()
   {
      gm_TheNetManager->m_bRawSpew = TRUE;
   }

   static void CmdRawSpewOff()
   {
      gm_TheNetManager->m_bRawSpew = FALSE;
   }

   static void CmdMyAddress()
   {
      const char *pAddr = gm_TheNetManager->GetPlayerAddress(OBJ_NULL);
      char buf[128];
      sprintf(buf, "My Address: %s", pAddr);
      Status(buf);
   }

   static void CmdPlayerName(int playerNum)
   {
      const char *pName = gm_TheNetManager->GetPlayerNameByNum(playerNum);
      char buf[128];
      sprintf(buf, "#%d is %s", playerNum, pName);
      Status(buf);
   }
#endif

#ifdef PLAYTEST   
   static void CmdNetHist(int cmd)
   {
      switch (cmd)
      {
         case 0: gm_TheNetManager->m_NetStats.ToggleHist(); break;      
         case 1: gm_TheNetManager->m_NetStats.DrawHist();   break;
         case 2: gm_TheNetManager->m_NetStats.ClearHist();  break;
         case 3: gm_TheNetManager->m_NetStats.ToggleOldHist(); break;
      }
   }
#endif
   
   static void InitCommands()
   {
      static Command commands [] = 
      {
         {"net_host",FUNC_VOID,CmdHost,"Host a networked game"},
         {"net_client",FUNC_STRING,CmdClient,"Join a networked game"},
         {"net_leave",FUNC_VOID,CmdLeave,"Leave a networked game"},
#ifndef SHIP
         {"net_state",FUNC_VOID,CmdState,"Display networking state"},
         {"net_raw_spew_on",FUNC_VOID,CmdRawSpewOn,"Turn low-level network spew on"},
         {"net_raw_spew_off",FUNC_VOID,CmdRawSpewOff,"Turn low-level network spew off"},
         {"net_raw_spew",TOGGLE_BOOL,&gm_TheNetManager->m_bRawSpew,"toggle net spew"},
         {"net_address",FUNC_VOID,CmdMyAddress,"Show my net address"},
         {"net_player_name",FUNC_INT,CmdPlayerName,"Show a player's name"},
#endif
#ifdef PLAYTEST         
         {"dump_net_stats",FUNC_VOID,DumpNetStats,"dump of messaging statistics by handlerID"},
         {"clear_net_stats",FUNC_VOID,ClearNetStats,"clear stats"},
         {"net_hist_cmd",FUNC_INT,CmdNetHist,"0toggle1draw2clear3toggleold"},
         {"net_xl_status",TOGGLE_BOOL,&gm_TheNetManager->m_NetStats.m_bOutputExcel,"net excel status"},
         {"net_xl_full",TOGGLE_BOOL,&gm_TheNetManager->m_NetStats.m_bOutputExcelFull,"net excel full status"},
         {"net_xl_packet",TOGGLE_BOOL,&gm_TheNetManager->m_NetStats.m_bOutputExcelPacket,"net excel packets"},
         {"dump_net_histogram",FUNC_INT,StaticDumpHistogram,"dump of histogram (1=send, 2=receive)"},
         {"reset_net_histogram",FUNC_INT,StaticResetHistogram,"resets histogram (1=send, 2=receive)"},
         {"reset_net_msg_histogram",FUNC_VOID,StaticResetMsgHistogram,"reset msg/frame histograms"},
         {"dump_net_msg_histogram",FUNC_INT,StaticDumpMsgHistogram,"dump msg/frame histogram (1=G, 2=NG, 3=both, 0=all)"},
#endif         
      };
      COMMANDS(commands,HK_ALL); 
   }

protected:

   STDMETHOD(Init)()
   {
      ClearState();

      gm_TheNetManager = (cNetManager *)this;

#ifndef SHIP
      m_bRawSpew = FALSE;
#ifdef SPEW_ON
      gm_bDoSpew = config_is_defined("net_spew");
#else
      // Allow spew in opt builds *if* net_opt_spew is also set:
      gm_bDoSpew = config_is_defined("net_spew") && config_is_defined("net_opt_spew");
#endif
#endif

      ClearFlush();

      // Register ourself for the messages that we handle. This should
      // always be registered, even if networking is turned off, for
      // consistency. (Send() always works.)
      gm_NetManagerHandlerID = RegisterMessageParser(HandleNetMsg,
                                                     "NetMan",
                                                     kNetSubparse,
                                                     (void *)this);

      if (config_is_defined("no_network")) {
         // The user has specifically said to cut off all networking.
         return S_OK;
      }

      // Set up the defaults for network media, session name, and player name
      if (!config_get_raw("net_media", m_Media, sizeof m_Media)) {
         NetSpew(("Assuming default network media\n"));
         strcpy(m_Media, "internet");
      }
      if (!config_get_raw("net_session", m_SessionName, sizeof m_SessionName))
      {
         NetSpew(("Assuming default session name\n"));
         strcpy(m_SessionName, "darksession");
      }
      if (!config_get_raw("net_player_name", 
                          m_PlayerName, 
                          sizeof m_PlayerName))
      {
         NetSpew(("Assuming default player name\n"));
         strcpy(m_PlayerName, "player");
      }

      // First null terminate any config strings that overflowed our
      // variables.
      m_PlayerName[sizeof m_PlayerName - 1] = '\0'; 
      m_Media[sizeof m_Media - 1] = '\0'; 
      m_SessionName[sizeof m_SessionName - 1] = '\0';

      // Get the underlying network 
      gm_Net = AppGetObj(INet);

      if (config_is_defined("net_host")) {
         // We've been told to pro-actively host a new session:
         Host(m_Media, m_SessionName);
      } else if (config_get_raw("net_client", m_HostAddr, sizeof m_HostAddr))
      {
         // We have been explicitly told to join an existing host:
         Join(m_Media, m_SessionName, m_HostAddr);
      }

      config_get_int("net_msg_frequency", &m_MsgFreq);

#ifdef NET_ALLOW_SIMULATION
      // Load the internet configuration parameters
      int simulate = 0, min, ave, max, loss;
      config_get_int("net_simulate_internet", &simulate);
      config_get_int("net_min_latency", &min);
      config_get_int("net_ave_latency", &ave);
      config_get_int("net_max_latency", &max);
      config_get_int("net_loss_percent", &loss);
      gm_Net->SetInternetParameters(loss, min, ave, max);
      // only turn on simulation if ave is between 1% & 49%.
      if (simulate!=0)
         if (( ave > min ) && (ave < (min+max/2)))
         {
            gm_Net->UseInternetSimulation(TRUE);
            mprintf("Note: Internet Simulation in Effect!");
         }
         else
            Warning(("Bogus net simulation parameters\n"));
#endif

#ifndef SHIP
      if (config_is_defined("net_raw_spew"))
         m_bRawSpew = TRUE;
#endif
      
      if (!config_get_int("net_player_timeout", &m_playerTimeout))
         m_playerTimeout = DEFAULT_PLAYER_TIMEOUT;

      // Does the user specify a limit on the number of messages to deal
      // with in a single frame?
      if (!config_get_int("net_frame_max_msgs", &m_frameMsgMax)) {
         m_frameMsgMax = 0;
      }

      return S_OK;
   }

   STDMETHOD(End)()
   {
#ifdef PLAYTEST      
      if (config_is_defined("net_dump_stats"))
         DumpNetStats();    // since they are bogus in non-network games
      if (config_is_defined("net_dump_histogram"))
         m_NetStats.DumpHistogram(kHistoSend | kHistoReceive);
#endif
      if (config_is_defined("net_voice"))
         StopVoiceChat();
      Leave();
      // Remove any lingering listeners:
      int i;
      int total = m_Listeners.Size();
      for (i = 0; i < total; i++) {
         cNetListener *entry = m_Listeners[0];
         m_Listeners.DeleteItem(0);
         delete entry;
      }
      SafeRelease(gm_Net);
      return S_OK;
   }

   void SendQuitMessage()
   {
      sNetMsg_Quit msg = {gm_NetManagerHandlerID, kNetMsg_Quit};
      NetSpew(("NET: Quitting session...\n"));
      SendToDPID(DPID_ALLPLAYERS, &msg, sizeof(msg), TRUE, TRUE);
   }

protected:
   ////////////////////////////////////////

   void DispatchNetMsg(sNetMsg_Generic *msg, ulong size, ObjID fromPlayer,
                       int overhead)
   {
      if (!m_Parsers.IsValidIndex(msg->handlerID))
         Warning(("Garbage network message handler specifier\n"));
      else
      {
         sMessageParserInfo *info = &m_Parsers[(int)msg->handlerID];
         if (info->parser == NULL)
            Warning(("Uninitialized network message parser\n"));
         else // call the registered message parser
         {
#ifdef PLAYTEST
            // Some basic statistics. We screen fromPlayer to avoid counting
            // loopbacks whenever possible. These stats can't happen earlier,
            // because we might have bundled many messages into a single
            // network packet.
            if (!IsPlayerObj(fromPlayer))
            {
               m_NetStats.NewReceive(size + overhead, (int)(msg->handlerID));
               info->numReceived++;
               info->bytesReceived += (size + overhead);
            }
#endif
            // Lock the objsys, to mimic the behaviour of frobctrl. If
            // we don't do this, then odd inconsistencies can creep in,
            // because no objects can actually be destroyed during frobs:
            gm_ObjSys->Lock();
            info->parser(msg, size, fromPlayer, info->clientData);
            gm_ObjSys->Unlock();
         }
      }
   }

   void KillDPlayer(DPID DPlayer, ObjID corpse)
   {
      if (m_DefunctPlayers.InSet(DPlayer)) {
         // Just ignore it; this player isn't really here any more
      } else {
         NetSpew(("Destroying player DPID %d\n", DPlayer));
         IAvatar *pAvatar = NULL;
         if (corpse != OBJ_NULL) {
            // Get the player's avatar
            sNetPlayer *netinfo;
            if (gm_NetPlayerProp->Get(corpse, &netinfo)) {
               pAvatar = netinfo->pAvatar;
            }
         }
         // Note that we now have one fewer player in the game. We
         // want to do this before we go notifying listeners,
         // since some of them will want to check NumPlayers().
         m_NumPlayers--;
         if (AmDefaultHost()) {
            KillMoribundPlayer(corpse, pAvatar);
         }

         m_DefunctPlayers.Add(DPlayer);
      }
   }

   // This is usually called as messages are coming in, but may be
   // called later if we have to store messages and process them later
   // due to NonNetworkLevel().
   void HandleRawNetMsg(DPID from, const char *buffer, ulong size)
   {
      // message from a player
      sNetMsg_Generic *msg = (sNetMsg_Generic *)buffer;

#ifndef SHIP
      if (m_bRawSpew)
         RawSpew(FALSE, from, size, msg);
#endif

      ObjID fromPlayer = FromNetPlayerID(from);

      // First check to see if this is a create or Hi message from
      // this player.  This needs to be special since we don't have a
      // player obj to pass to a msg parser. Also, we handle Bundle
      // here for efficiency, and Quit just to be consistent.
      if (msg->handlerID == gm_NetManagerHandlerID)
      {
         switch (((sNetMsg_NetGeneric *)msg)->type)
         {
            case kNetMsg_Hi:
            {
               char response = ((sNetMsg_Hi *)msg)->yourPlayerNum;
               if (response <= 0) {
                  DWORD failReason = -response;
                  // We've been rejected; horror, pity.
                  NotifyListeners(kNetMsgRejected, failReason);
                  gm_Net->DestroyPlayer(gm_PlayerDPID);
                  gm_bNetworkGame = FALSE;
                  NotifyListeners(kNetMsgNetworkLost, NULL);
                  gm_Net->Close();
                  ClearState();
               } else {
                  // Check that the version matches:
                  VersionNum myVersion = AppVersion()->num;
                  VersionNum hostVersion = ((sNetMsg_Hi *) msg)->hostVersion;
                  if (!VersionNumsEqual(&hostVersion, &myVersion))
                  {
                     // Version mismatch; don't let them connect:
                     Leave();
                     NotifyListeners(kNetMsgRejected, kNetRejectVersion);
                  } else {
                     m_MyPlayerNum = response;
                     NetSpew(("MY PLAYER NUM = %d\n", m_MyPlayerNum));
                     // The number of players is the number sent from the
                     // host, plus any players that we got CREATEPLAYER
                     // system messages for (which is m_MyPlayerNum-1).
                     m_NumPlayers += (m_MyPlayerNum-1);
                     // Tell any listeners that we have joined successfully:
                     NotifyListeners(kNetMsgHi, m_MyPlayerNum);
                     // Tell any other players about us:
                     SendPlayerInfo();
                  }
               }
               return;
            }
            case kNetMsg_CreatePlayer:
               HandleCreatePlayerMsg(from, (sNetMsg_CreatePlayer *)msg); 
               return;
            case kNetMsg_Bundle:
            {
               sNetMsg_Bundle *bundleMsg = (sNetMsg_Bundle *) msg;
               int blockSize =
                  size - (sizeof(tNetMsgHandlerID) + sizeof(eNetMessageType));
               UnbundleMessages(bundleMsg->block, fromPlayer, blockSize);
               return;
            }
            case kNetMsg_Quit:
            {
               KillDPlayer(from, fromPlayer);
               return;
            }
            default:
               ; // don't return
         }
      }
      if (fromPlayer == OBJ_NULL)
         // The messages themselves are responsible for accepting or
         // rejecting in this case, as appropriate. It's just a spew,
         // not a warning, because it's fairly common.
         NetSpew(("Net message received from a player without an ObjID\n"));
      // A mild hack: we keep track of the player who sent the message
      // we're working on, so that subsystems nestled deep down can get
      // at the information:
      m_OriginatingPlayer = fromPlayer;
      DispatchNetMsg(msg, size, fromPlayer, m_packetOverhead);
      m_OriginatingPlayer = OBJ_NULL;
   }

   void PollNetwork()
   {
      DPID from,to;
      static char buffer[MAX_MESSAGE_SIZE];
      DWORD size, flags;
      HRESULT hr;
      int msgCount = 0;
      while (TRUE)
      {
         size = sizeof buffer;
         flags = 0;
         if (m_frameMsgMax && (msgCount > m_frameMsgMax))
            // We've taken up enough time already this frame:
            return;
         hr = gm_Net->Receive(&from,&to,flags,(LPVOID)buffer,&size);
         if (hr == DPERR_NOMESSAGES)
            return;
         else if (hr != DP_OK)
         {
            Warning(("NET Receive got error: %x %s\n", hr, gm_Net->DPlayErrorAsString(hr)));
            NetRawSpew(("NET Receive got error: %x %s\n", hr, gm_Net->DPlayErrorAsString(hr)));
            return;
         }

         if (from==DPID_SYSMSG) 
         {
            DPMSG_GENERIC *msg=(DPMSG_GENERIC*)buffer;
            switch (msg->dwType) {
               case DPSYS_CREATEPLAYERORGROUP:
               {
                  DPMSG_CREATEPLAYERORGROUP *createMsg
                     = (DPMSG_CREATEPLAYERORGROUP *)msg;
                  if (m_bGameStarted ||
                      (m_NumPlayers >= m_MaxPlayers))
                  {
                     // It's too late to add any more players. Send a "Hi"
                     // with a player number of 0, which is the signal that
                     // we are refusing this connection:
                     char failReason;
                     if (m_bGameStarted)
                        failReason = kNetRejectStarted;
                     else if (m_NumPlayers >= m_MaxPlayers)
                        failReason = kNetRejectTooMany;
                     else
                        failReason = kNetRejectMisc;
                     sNetMsg_Hi msg = { gm_NetManagerHandlerID, 
                                        kNetMsg_Hi,
                                        -failReason };
                     // @HACK: this is pretty horrid, but we can't
                     // allow Hi messages to fall on the floor...
                     BOOL saveNonNetworkLevel = m_bNonNetworkLevel;
                     m_bNonNetworkLevel = FALSE;
                     SendToDPID(createMsg->dpId, &msg, sizeof(msg), TRUE);
                     m_bNonNetworkLevel = saveNonNetworkLevel;
                     m_DefunctPlayers.Add(createMsg->dpId);
                  } else {
                     m_NumPlayers++;
                     NetSpew(("DPSYS_CREATEPLAYERORGROUP (numplayers = %d)\n",
                              m_NumPlayers));
                     gm_Net->ResetPlayerData(createMsg->dpId, NULL);
                     m_PlayerBundles.AddPlayer(createMsg->dpId);
                     m_PlayerBundlesNG.AddPlayer(createMsg->dpId);
                     if (gm_bAmSessionHost)
                     {
                        // Send the new player a hello, to let them know
                        // about us.  m_NumPlayers is sent as the
                        // yourPlayerNum field.
                        sNetMsg_Hi msg = { gm_NetManagerHandlerID, 
                                           kNetMsg_Hi,
                                           m_NumPlayers };
                        msg.hostVersion = AppVersion()->num;
                        // @HACK: this is pretty horrid, but we can't
                        // allow Hi messages to fall on the floor...
                        BOOL saveNonNetworkLevel = m_bNonNetworkLevel;
                        m_bNonNetworkLevel = FALSE;
                        SendToDPID(createMsg->dpId, &msg, sizeof(msg), TRUE);
                        m_bNonNetworkLevel = saveNonNetworkLevel;
                        char temp[40];
                        sprintf(temp, "Added player #%d", m_NumPlayers);
                        Status(temp);
                     }
                     // Tell this player about me. It's a tad inefficient,
                     // but easy code. Note that all of the existing players
                     // will send their info around when a new one joins.
                     SendPlayerInfo();
                     NotifyListeners(kNetMsgPlayerConnect, m_NumPlayers);
                  }
                  break;
               }
               case DPSYS_DESTROYPLAYERORGROUP:
               {
                  DPMSG_DESTROYPLAYERORGROUP *destroyMsg=(DPMSG_DESTROYPLAYERORGROUP *)msg;
                  NetSpew(("DPSYS_DESTROYPLAYERORGROUP\n"));
                  ObjID corpse = (ObjID) destroyMsg->lpLocalData;
                  KillDPlayer(destroyMsg->dpId, corpse);
                  m_PlayerBundles.KillPlayer(destroyMsg->dpId);
                  m_PlayerBundlesNG.KillPlayer(destroyMsg->dpId);
                  break;
               }
               case DPSYS_SENDCOMPLETE:
               {
                  DPMSG_SENDCOMPLETE *sendMsg = (DPMSG_SENDCOMPLETE *) msg;
                  DWORD msgID = sendMsg->dwMsgID;
                  ulong timeSent;
                  NetRawSpew(("Finished sending %x; latency %d ms; "
                              "time %d ms.\n", 
                              msgID,
                              sendMsg->dwSendTime,
                              (m_PendingMsgIDs.InSet(msgID, &timeSent)
                               ? (tm_get_millisec() - timeSent)
                               : -1)));
                  switch (sendMsg->hr)
                  {
                     case DP_OK:
                        // Normal case -- delivered successfully
                        break;
                     case DPERR_TIMEOUT:
                        NetRawSpew(("ERROR: Message timed out!\n"));
                        break;
                     default:
                        NetRawSpew(("ERROR: Message returned %x!\n", hr));
                        break;
                  }
                  m_PendingMsgIDs.Delete(sendMsg->dwMsgID);
                  break;
               }
               case DPSYS_HOST:
                  NetSpew(("DPSYS_HOST\n")); 
                  break;
               case DPSYS_SESSIONLOST:
                  // We've been cut off! Horror, shame! Tell anyone
                  // who is listening what has happened...
                  NetSpew(("Networking Session has been lost!\n"));
                  gm_Net->DestroyPlayer(gm_PlayerDPID);
                  NotifyListeners(kNetMsgNetworkLost, NULL);
                  ClearState();
                  gm_bNetworkGame = FALSE;
                  NotifyListeners(kNetMsgReset, NULL);
                  break;
               default:
                  NetSpew(("unknown sysmsg\n"));
            }
         }
         else
         {
            // It's a real message:
            if (m_bStoringMessages) {
               // If we are currently storing messages, just store, don't
               // process the message yet.
               // @TBD: Note that this is also storing/suppressing the
               // "Hi" message. This might potentially cause problems if
               // we decide we want to let players join while the host is
               // running through the intro.
               // Allow through only messages that have the AllowNonNetwork
               // flag set.
               sNetMsg_Generic *msg = (sNetMsg_Generic *)buffer;
               AssertMsg1(m_Parsers.IsValidIndex(msg->handlerID),
                          "Garbage network message handler %d!",
                          msg->handlerID);
               sMessageParserInfo *info = &m_Parsers[(int)msg->handlerID];
               AssertMsg1(info->parser != NULL,
                          "Uninitialized network message parser %d!",
                          msg->handlerID);
               if (info->flags & kNetAllowNonNetwork) {
                  // This message should be let through; it's *very* metagame
                  HandleRawNetMsg(from, buffer, size);
               } else {
                  cStoredMessage *pStored =
                     new cStoredMessage(from, buffer, size);
                  m_StoredMessages.Append(pStored);
               }
            } else {
               // Process it now
               HandleRawNetMsg(from, buffer, size);
            }
         }
      }
   }

   // Send out all of the messages for a particular player, in a
   // single lump:
   void SendBundle(cPlayerMsgBundle *pBundle, BOOL guaranteed)
   {
      if (pBundle->m_NumMsgs == 0)
         // Nothing to worry about
         return;

      if (pBundle->m_NumMsgs == 1)
      {
         // Just a single message, so just send it straight:
         cBundledMessage *pMsg = pBundle->m_Msgs.GetFirst()->item;
         SendToDPID(pBundle->m_DPID,
                    pMsg->m_Msg,
                    pMsg->m_Size,
                    guaranteed,
                    FALSE);

#ifdef PLAYTEST
         m_NetStats.NewSend(pMsg->m_Size + m_packetOverhead,
                            guaranteed, 
                            (int)(((sNetMsg_Generic *)pMsg->m_Msg)->handlerID));
#endif
      } else {
         // Multiple msgs; collect them all into a single message:
         ulong totalSize = 
            1 +                            // handler
            1 +                            // subtype
            sizeof(ushort) +               // number of messages
            (sizeof(ushort) * pBundle->m_NumMsgs) +
                                           // size header for each msg
            pBundle->m_TotalMsgSize;       // the message data itself

#ifdef PLAYTEST
         // The overhead for the packet, amortized per message:
         int msgOverhead = 
            ((4 + m_packetOverhead) / pBundle->m_NumMsgs) + 2;
#endif

         char *pBlock = (char *) malloc(totalSize);
         char *cursor = pBlock;

         // Now assemble the bundled message:
         // The handler ID:
         *((tNetMsgHandlerID *) cursor) = gm_NetManagerHandlerID;
         cursor += sizeof(tNetMsgHandlerID);
         // The message type:
         *((eNetMessageType *) cursor) = kNetMsg_Bundle;
         cursor += sizeof(eNetMessageType);
         // The number of messages:
         *((ushort *) cursor) = pBundle->m_NumMsgs;
         cursor += sizeof(ushort);
         // Iterate over all of the messages:
         cBundledMessageNode *pNode = pBundle->m_Msgs.GetFirst();
         int i = 0;
         while (pNode)
         {
            cBundledMessage *pBundledMsg = pNode->item;

            // The size of this message:
            ushort size = pBundledMsg->m_Size;
            AssertMsg(size == pBundledMsg->m_Size,
                      "Message larger than a ushort can handle!!!");
            *((ushort *) cursor) = size;
            cursor += sizeof(ushort);
            // The message itself:
            memcpy(cursor, pBundledMsg->m_Msg, size);
            cursor += size;

#ifdef PLAYTEST
            m_NetStats.NewSend(pBundledMsg->m_Size + msgOverhead,
                               guaranteed, 
                               (int)(((sNetMsg_Generic *)pBundledMsg->m_Msg)->handlerID));
#endif

            // And on to the next:
            pNode = pNode->GetNext();
            i++;
         }
         AssertMsg(i == pBundle->m_NumMsgs,
                   "Inconsistent message bundle!");

         // Finally, send it along...
         NetRawSpew(("Sending bundle of %d %s messages to %d\n",
                     pBundle->m_NumMsgs,
                     (guaranteed ? "guaranteed" : "nonguaranteed"),
                     pBundle->m_DPID));
         SendToDPID(pBundle->m_DPID, pBlock, totalSize, guaranteed, FALSE);

         // And clean up:
         free(pBlock);
      }
      pBundle->Clear();
   }

   // Called once a frame or so, to send all of the bundled messages out.
   // Safe to call more often, but it sort of defeats the purpose.
   void SendBundles()
   {
      cPlayerMsgBundle *pBundle = m_PlayerBundles.GetFirst();
      while (pBundle)
      {
         SendBundle(pBundle, TRUE);
         pBundle = pBundle->GetNext();
      }

      pBundle = m_PlayerBundlesNG.GetFirst();
      while (pBundle)
      {
         SendBundle(pBundle, FALSE);
         pBundle = pBundle->GetNext();
      }
   }

   void UnbundleMessages(char *block, ObjID fromPlayer, ulong totalSize)
   {
      char *ptr = block;
      int numMsgs = *((ushort *) ptr);
      NetRawSpew(("Unbundling %d messages from %d.\n", numMsgs, fromPlayer));
      ptr += sizeof(ushort);
      // The overhead per packet:
      int overhead = 0;
#ifdef PLAYTEST
      overhead = ((4 + m_packetOverhead) / numMsgs) + 2;
#endif
      // For each message...
      while (numMsgs > 0)
      {
         int msgSize = *((ushort *) ptr);
         ptr += sizeof(ushort);
         DispatchNetMsg((sNetMsg_Generic *) ptr, msgSize, fromPlayer, overhead);
         ptr += msgSize;
         numMsgs--;
      }
      AssertMsg((block + totalSize) == ptr,
                "Received bundled block of incorrect size!");
   }

   // Called when we decide that it seems appropriate to kill off a
   // player.
   void KillMoribundPlayer(ObjID player, IAvatar *pAvatar)
   {
      // We don't own the player, so we have to take him over in order
      // to kill him. We'd better be damned sure that he's really gone,
      // because this will kill him if not:
      GhostRemRemote(player);
      if (player != OBJ_NULL)
         gm_ObjNet->ObjTakeOver(player);

      // Tell any listeners about the change:
      NotifyListeners(kNetMsgPlayerLost, player);

      // Remove any pending messages to that player:
      tNetPlayerID dpid = ToNetPlayerID(player);
      m_PlayerBundles.KillPlayer(dpid);
      m_PlayerBundlesNG.KillPlayer(dpid);

      // Kill the avatar...
      NetSpew(("Killing moribund player %d\n", player));
      SafeRelease(pAvatar);
      if (player != OBJ_NULL)
         gm_ObjSys->Destroy(player);
   }

   // Enumerate all of the players currently in the session, and set
   // up their message bundles. Should be called immediately after joining
   // the session.
   void AddPlayer(DPID player)
   {
      if (player != gm_PlayerDPID)
      {
         m_PlayerBundles.AddPlayer(player);
         m_PlayerBundlesNG.AddPlayer(player);
      }
   }

   static BOOL CALLBACK EnumPlayersCallback(DPID playerDPID,
                                            DWORD type,
                                            LPCDPNAME /* lpName */,
                                            DWORD flags,
                                            void *pNetMan)
   {
      if (type == DPPLAYERTYPE_PLAYER)
         ((cNetManager *) pNetMan)->AddPlayer(playerDPID);

      return(TRUE);
   }

   void GetOtherPlayers()
   {
      HRESULT hr = gm_Net->EnumPlayers(NULL,
                                       EnumPlayersCallback,
                                       this,
                                       DPENUMPLAYERS_REMOTE);
   }

   ////////////////////////////////////////////////////////////
   // DATABASE MESSAGE HANDLER 
   //

   struct sNetTag {
      ulong NumPlayers;
      ulong MyPlayerNum;
      char PlayerName[64];
      tSimTime MsgFreq;
      int playerTimeout;
      // We're not currently storing the non-network-level state info,
      // because we don't really support saving of MP games in nonnetwork
      // properly yet. Note that, if we *do* support this, it's essential
      // that this not get saved into briefcases, only into real levels...
      //BOOL bNonNetworkLevel;
      //BOOL bStoringMessages;
   };

   // Methods to write and read the basic net info. If we're player 1
   // (the default host), then we do *not* actually write this stuff
   // out, but leave it nulled out. At read time, if it's nulled, then
   // we stick with the current state of that info. This is to prevent
   // us from saving too much "game state" when we are creating a
   // level.
   void FillTagState(sNetTag *info)
   {
      if (AmDefaultHost()) {
         info->NumPlayers = 0;
         info->MyPlayerNum = 0;
      } else {
         info->NumPlayers = m_NumPlayers;
         info->MyPlayerNum = m_MyPlayerNum;
      }
      strcpy(info->PlayerName, m_PlayerName);
      info->MsgFreq = m_MsgFreq;
      info->playerTimeout = m_playerTimeout;
      //info->bNonNetworkLevel = m_bNonNetworkLevel;
      //info->bStoringMessages = m_bStoringMessages;
   }

   void GetTagState(sNetTag *info)
   {
      if (info->NumPlayers != 0) {
         // We don't change NumPlayers to match what's in the file:
         // the number of players connected is however many it is...
         if (m_NumPlayers < info->NumPlayers) {
            Warning(("Missing %d players who were in the file.\n",
                     (info->NumPlayers - m_NumPlayers)));
         }
         if (m_MyPlayerNum != info->MyPlayerNum) {
            m_MyPlayerNum = info->MyPlayerNum;
            NetSpew(("MY PLAYER NUM = %d\n", m_MyPlayerNum));
         }
      }
      // The player's name now gets set in the metagame
      // @TBD: what is the right way for this to work in the long run?
      //strcpy(m_PlayerName, info->PlayerName);
      m_MsgFreq = info->MsgFreq;
      m_playerTimeout = info->playerTimeout;
      //m_bNonNetworkLevel = info->bNonNetworkLevel;
      //m_bStoringMessages = info->bStoringMessages;
   }

   void WriteAvatarTag(ITagFile* file, eObjPartition part)
   {
      TagVersion v = AvatarTagVersion;
      if (SUCCEEDED(file->OpenBlock(&AvatarTag,&v)))
      {
         // Then, write out each of the avatars. Do any avatars that
         // are properly assigned to players:
         sPropertyObjIter iter;
         ObjID player;
         sNetPlayer *netinfo;
         gm_NetPlayerProp->IterStart(&iter);
         while (gm_NetPlayerProp->IterNext(&iter, &player))
         {
            if (gm_ObjSys->Exists(player) &&
                gm_ObjSys->IsObjSavePartition(player, part)) 
            {
               // Only actually write out the avatar if we're saving
               // the avatars, and if this avatar hasn't been blown
               // away...
               gm_NetPlayerProp->Get(player, &netinfo);
               netinfo->pAvatar->WriteTagInfo(file);
               // And write out the DPID for that player:
               tNetPlayerID dpid = ToNetPlayerID(player);
               file->Write((char *) &dpid,
                           sizeof(tNetPlayerID));
            }
         }

         // Finish off:
         file->CloseBlock();
      }
   }

   // Save the current state of networking out to the tagfile
   // @TBD: save m_StoredMessages.
   void WriteNetTag(ITagFile* file, eObjPartition part)
   {
      if (!gm_bNetworkGame || (m_NumPlayers == 1)) {
         // Not a networked game; don't bother to write out any networking
         // info:
         return;
      }

      TagVersion v = NetTagVersion;
      if (SUCCEEDED(file->OpenBlock(&NetTag,&v)))
      {
         // Write out the info about the networking in general:
         sNetTag tagData;
         FillTagState(&tagData);
         file->Write((char*)&tagData,sizeof(tagData));

         // Finish off:
         file->CloseBlock();
      }

      WriteAvatarTag(file, part);
   }

   void ReadAvatarTag(ITagFile *file, eObjPartition part)
   {
      TagVersion v = AvatarTagVersion;
      if (SUCCEEDED(file->OpenBlock(&AvatarTag, &v)))
      {
         // Read in each of the avatars. Note that this will read
         // in however many avatars were written out, regardless of the
         // theoretical m_NumPlayers. This is necessary, because there
         // are several kinds of files that don't get avatars.
         while (file->TellFromEnd() > 0) {
            // We let the avatar create itself from the file; we just
            // care about the player number it corresponds to
            IAvatar *pAvatar = new cAvatar(file);
            ulong playerNum = pAvatar->PlayerNum();
            int oldSize = m_WaitingAvatars.Size();
            if ((playerNum + 1) > oldSize) {
               m_WaitingAvatars.SetSize(playerNum + 1);
               int j;
               for (j = oldSize; j <= playerNum; j++) {
                  // Make sure the slots are initialized properly
                  m_WaitingAvatars[j] = NULL;
               }
            }
            m_WaitingAvatars[playerNum] = pAvatar;
            // Now read in the corresponding DPID, right after the
            // avatar:
            tNetPlayerID dpid;
            file->Read((char *) &dpid, sizeof(tNetPlayerID));
            // Set up the preliminary property value; this will get
            // changed later, if the player actually shows up:
            sNetPlayer nplayer = { dpid, pAvatar, 0 };
            gm_NetPlayerProp->Set(pAvatar->PlayerObjID(), &nplayer);
            // Keep track of the fact that there has been an avatar found:
            m_bLoadStart = TRUE;
            // This is clearly a saved game, so don't allow anyone else
            // to join:
            m_bGameStarted = TRUE;
         }

         // Finish off:
         file->CloseBlock();
      }
   }

   // Get the networking info from the tagfile
   void ReadNetTag(ITagFile* file, eObjPartition part)
   {
      if (!gm_bNetworkGame) {
         // Don't bother to read in the network info, since we're playing
         // single-user
         return;
      }

      // Read in the info about the networking in general:
      TagVersion v = NetTagVersion;
      if (SUCCEEDED(file->OpenBlock(&NetTag,&v)))
      {
         sNetTag tagData;
         file->Read((char*)&tagData,sizeof(tagData));
         GetTagState(&tagData);

         // Finish off:
         file->CloseBlock();
      }

      // Then read in the avatar info:
      ReadAvatarTag(file, part);
   }

   // Do any tweaking that needs to happen after level load
   void LoadCleanup()
   {
      gm_SimTime = GetSimFileTime();
   }

   void db_message(DispatchData *msg)
   {
      msgDatabaseData data;
      data.raw = msg->data;

      switch (DB_MSG(msg->subtype))
      {
         case kDatabaseReset:
            // We are done with this level.
            //gm_bNetworkingReady = FALSE;
            // Tell clients that Networking is now off:
            //NotifyListeners(kNetMsgReset, NULL);
            // Destroy all of the net players:
            gm_Net->EnumPlayers(NULL, _DestroyNetPlayerCallback, (void *)this, 0);
            m_PlayersJoined = 0;
            // ReadNetTag will set this iff an avatar is found in this level:
            m_bLoadStart = FALSE;
            // Since ObjIDs are now invalid, we don't officially know the
            // host's objID:
            gm_DefaultHostPlayer = OBJ_NULL;
            break;
         case kDatabaseLoad:
            ReadNetTag(data.load, msg->subtype);
            break;
         case kDatabasePostLoad:
            LoadCleanup();
            break;
         case kDatabaseSave:
            WriteNetTag(data.load, msg->subtype);
            break;
      }
   }

   ////////////////////////////////////////////////////////////

   void CreateMessages()
   {
      m_pPlayerContentsMsg = new cPlayerContentsMsg(&PlayerContentsDesc);
      m_pFinishSynchMsg = new cNetMsg(&sFinishSynchDesc, this);
      m_pSynchFlushMsg = new cNetMsg(&sSynchFlushDesc, this);
      m_pStartSynchMsg = new cNetMsg(&sStartSynchDesc, this);
      m_pPlayerInfoMsg = new cNetMsg(&sPlayerInfoDesc, this);
   }

   void DestroyMessages()
   {
      if (m_pPlayerContentsMsg) {
         delete m_pPlayerContentsMsg;
         m_pPlayerContentsMsg = 0;
      }
      if (m_pFinishSynchMsg) {
         delete m_pFinishSynchMsg;
         m_pFinishSynchMsg = 0;
      }
      if (m_pSynchFlushMsg) {
         delete m_pSynchFlushMsg;
         m_pSynchFlushMsg = NULL;
      }
      if (m_pStartSynchMsg) {
         delete m_pStartSynchMsg;
         m_pStartSynchMsg = NULL;
      }
      if (m_pPlayerInfoMsg) {
         delete m_pPlayerInfoMsg;
         m_pPlayerInfoMsg = NULL;
      }
   }

   ////////////////////////////////////////////////////////////

   // A little extra loop client, just for catching reset messages early:
   static eLoopMessageResult LGAPI _ResetLoopFunc(void* data,
                                                  eLoopMessage msg,
                                                  tLoopMessageData hdata)
   {
      // useful stuff for most clients
      eLoopMessageResult result = kLoopDispatchContinue; 
      StateRecord* state = (StateRecord*)data;
      cNetManager *This = gm_TheNetManager;
      LoopMsg info;
      info.raw = hdata;

      switch(msg) {
         case kMsgDatabase:
         {
            DispatchData *dbMsg = info.dispatch;
            switch (DB_MSG(dbMsg->subtype)) {
               case kDatabaseReset:
                  // Tell everyone that networking is now off:
                  This->gm_bNetworkingReady = FALSE;
                  This->m_PlayerInfo.ClearObjIDs();
                  This->NotifyListeners(kNetMsgReset, NULL);
                  break;
            }
            break;
         }
         case kMsgEnd:
            delete state;
            break;
      }

      return result;
   }

   static eLoopMessageResult LGAPI _LoopFunc(void* data, eLoopMessage msg, tLoopMessageData hdata)
   {
      // useful stuff for most clients
      eLoopMessageResult result = kLoopDispatchContinue; 
      StateRecord* state = (StateRecord*)data;
      cNetManager *This = state->netman;
      LoopMsg info;
      info.raw = hdata;

      // First, do the stuff that wants to happen whether or not we're
      // doing any networking this game (mainly setting up the static
      // properties):
      switch(msg)
      {
         case kMsgAppInit:
         {
            gm_ObjSys = AppGetObj(IObjectSystem);
            // These properties *must* always be created, or they'll get
            // lost in non-networked builds:
            NetPropsInit();
            // We need to initialize all this stuff even if we're not 
            // networked now, because we might *become* networked later:
            gm_NetPlayerProp = new cNetPlayerProp(&NetPlayerPropDesc);
            gm_ObjNet = AppGetObj(IObjectNetworking);
            This->CreateMessages();
            InitCommands();
#ifdef PLAYTEST
            gm_TheNetManager->m_NetStats.ResetHistogram(kHistoSend | kHistoReceive);
#endif
            break;
         }
         case kMsgAppTerm:
         {
            NetPropsShutdown();
            SafeRelease(gm_ObjSys);
            This->DestroyMessages();
            SafeRelease(gm_ObjNet);
            SafeRelease(gm_NetPlayerProp);
            break;
         }
         case kMsgEnd:
            delete state;
            break;
      }

      if (!gm_bNetworkGame) {
         // We aren't doing networking in this session.
         return result;
      }

      switch(msg)
      {
         case kMsgAppInit:
         {
#ifndef NEW_NETWORK_ENABLED
            AssertMsg(FALSE,"Network manager loop client shouldn't be installed");
#endif
            break;
         }
         case kMsgAppTerm:
            break;
         case kMsgDatabase:
         {
            This->db_message(info.dispatch);
            break;
         }
         case kMsgNormalFrame:
            This->PollNetwork();
            This->SendBundles();
#ifdef PLAYTEST
            ConfigSpew("net_frame_spew", 
                       ("******FRAME**** time:%d\n", tm_get_millisec()));
#endif
            if (This->gm_bSynchronizing) {
               // We're in the middle of synchronizing with other players
               if (This->SynchReady()) {
                  This->FinishSynch();
               }
            } else if (IsSimTimePassing()) {
               gm_SimTime = GetSimTime();
#ifdef PLAYTEST
               This->m_NetStats.NewFrame();
#endif
            }
            break;
         case kMsgEnterMode:
         case kMsgResumeMode:
            // Are we entering game mode?
            if (IsEqualGUID(*info.mode->to.pID,LOOPID_GameMode) &&
                gm_bNetworkGame &&
                !(This->m_bNonNetworkLevel))
            {
               gm_bInGameMode = TRUE;
#ifndef SHIP
               gm_EnteredGameMode = tm_get_millisec();
#endif
            }
            break;
         case kMsgSuspendMode:
         case kMsgExitMode:
            // Are we leaving game mode?
            if (IsEqualGUID(*info.mode->from.pID,LOOPID_GameMode) &&
                gm_bNetworkGame &&
                gm_bInGameMode)
            {
#ifndef SHIP
               gm_GameTime += (tm_get_millisec() - gm_EnteredGameMode);
#endif
               gm_bInGameMode = FALSE;
            }
            break;
         default:
            break;
      }
      return result;
   }


public:

   cNetManager(IUnknown* pOuter)
      : m_Parsers(),
        m_pPlayerContentsMsg(NULL),
        m_pFinishSynchMsg(NULL),
        m_pSynchFlushMsg(NULL),
        m_pStartSynchMsg(NULL),
        m_pPlayerInfoMsg(NULL),
        m_MsgFreq(200),
        m_LastListener(0),
#ifdef PLAYTEST
        m_NetStats(&m_Parsers),
#endif        
        m_MaxPlayers(DEFAULT_MAX_PLAYERS)
   {
      // Set ourselves up in the Application Aggregate:
      sRelativeConstraint NetManConstraints[] =
        { { kConstrainAfter, &IID_INet },
          { kNullConstraint, } };
#ifdef PLAYTEST                            
      MI_INIT_AGGREGATION_2(pOuter, INetManager,INetStats,
                            kPriorityNormal, NetManConstraints);
#else
      MI_INIT_AGGREGATION_1(pOuter, INetManager, kPriorityNormal, NetManConstraints);
#endif      

      if (config_is_defined("no_network"))
      {
         // The config file has specified that we are going to do
         // absolutely *no* networking, even initializing DirectPlay.
         NetSpew(("Network suppressed by config file\n"));
         return;
      }

      // We are the only people that know that we need to create the
      // net com object, so we do it here rather than in init.cpp.
      NetCreate(); 
   }

   // Zero out all of the dynamically-changing variables in the system
   // state.
   void ClearState()
   {
      gm_DefaultHostPlayer = OBJ_NULL;
      gm_PlayerDPID = DPID_UNKNOWN;
      gm_bAmSessionHost = FALSE;
      gm_bNetworkingReady = FALSE;
      gm_bNetworkGame = FALSE;
      gm_bSynchronizing = FALSE;
      gm_SimTime = 0;
      gm_bInGameMode = FALSE;
      gm_EnteredGameMode = 0;
      // gm_GameTime = 1; // since we seem to call ClearState a lot

      m_NumPlayers = 1;
      m_PlayersReset = 0;
      m_PlayersJoined = 0;
      m_MyPlayerNum = 0;
      m_SuspendMessaging = 0;
      m_bCreateContentProxies = FALSE;
      m_bGameStarted = FALSE;
      m_OriginatingPlayer = OBJ_NULL;
      m_bNonNetworkLevel = FALSE;
      m_bStoringMessages = FALSE;
      m_bSynchFlushing = FALSE;
      m_curRound = 0;
      m_flushStack = 0;
      m_packetOverhead = 0;

      // Clear out any record of pending messages
      m_PendingMsgIDs.ClearAll();
   }

   // Is this a networked game, ready for sending & receiving messages.
   STDMETHOD_(BOOL, Networking)(void)
   {
      return gm_bNetworkGame && gm_bNetworkingReady;
   }

   // Is this a multi-player game, not necessarily ready for
   // sending & receiving messages.
   STDMETHOD_(BOOL, IsNetworkGame)(void)
   {
      return gm_bNetworkGame;
   }

   // How many players, including this one.
   STDMETHOD_(ulong, NumPlayers)() 
   {
      if (m_bNonNetworkLevel || !gm_bNetworkGame)
         return 1;
      else 
         return m_NumPlayers;
   }

   // MyPlayerNum is a number between 1 & NumPlayers that is unique for us
   // (1 == defaulthost).
   STDMETHOD_(ulong, MyPlayerNum)() 
   {
      if (!gm_bNetworkGame)
         return 1;
      else
         return m_MyPlayerNum;
   }

   STDMETHOD_(ulong, ObjToPlayerNum)(ObjID player)
   {
      if (!gm_bNetworkGame ||
          (player == OBJ_NULL))
      {
         return 0;
      }

      if (player == PlayerObject()) {
         return MyPlayerNum();
      }

      sNetPlayer *netinfo;
      if (!gm_NetPlayerProp->Get(player, &netinfo)) {
         return 0;
      }
      AssertMsg(netinfo->pAvatar, "Network player without an avatar!");

      return netinfo->pAvatar->PlayerNum();
   }

   STDMETHOD_(ObjID, PlayerNumToObj)(ulong player)
   {
      // We have to check NetworkGame instead of Networking(), because
      // this is often called *before* we SynchFlush:
      if (!gm_bNetworkGame ||
          (player == 0))
      {
         return OBJ_NULL;
      }

      if (player == MyPlayerNum()) {
         return PlayerObject();
      }

      // Normal circumstances: we've already recorded this mapping in the
      // player info table:
      ObjID tryPlayer;
      tryPlayer = m_PlayerInfo.GetObjID(player);
      if (tryPlayer != OBJ_NULL)
         return tryPlayer;

      // Otherwise, find the mapping. This is hideous, but at the moment,
      // the avatar is responsible for saving the mapping:
      FOR_ALL_NET_PLAYERS(this, &tryPlayer) {
         sNetPlayer *netinfo;
         if (gm_NetPlayerProp->Get(tryPlayer, &netinfo)) {
            AssertMsg(netinfo->pAvatar, "Network player without an avatar!");
            if (netinfo->pAvatar->PlayerNum() == player) {
               // Okay, this is the one we want. Record the mapping and
               // return it:
               m_PlayerInfo.SetObjID(player, tryPlayer);
               return tryPlayer;
            }
         }
      }

      Warning(("Putative player %ld has no object?!?\n", player));
      return OBJ_NULL;
   }

   // Establish the player's name
   // @TBD: allow the player to change his name after the session is
   // established.
   STDMETHOD_(void, SetPlayerName)(const char *pName)
   {
      strncpy(m_PlayerName, pName, MAX_PLAYER_NAME_LEN);
      m_PlayerName[MAX_PLAYER_NAME_LEN] = '\0';
      SendPlayerInfo();
   }

   // Get the name of the current player, or one of the network players
   STDMETHOD_(const char *, GetPlayerName)(ObjID player)
   {
      if ((player == OBJ_NULL) || IsPlayerObj(player)) {
         return m_PlayerName;
      } else {
         sNetPlayer *netinfo;
         if (gm_NetPlayerProp->Get(player, &netinfo)) {
            return netinfo->name;
         } else {
            Warning(("GetPlayerName called on unknown player %d", player));
            return "";
         }
      }
   }

   // A semi-static buffer, for returning the player's address:
   char m_LastPlayerAddr[128];

   // Get the address of the specified player.
   STDMETHOD_(const char *, GetPlayerAddress)(ObjID player)
   {
      // Make sure things are cleared if needed:
      m_LastPlayerAddr[0] = '\0';

      // Only try this in a networked game:
      if (!gm_bNetworkGame)
         return m_LastPlayerAddr;

      // Go from the player object to the DPID:
      DPID playerDPID;
      if (player == OBJ_NULL) {
         playerDPID = gm_PlayerDPID;
      } else {
         playerDPID = (DPID) ToNetPlayerID(player);
      }

      gm_Net->GetPlayerAddress(playerDPID, m_LastPlayerAddr, 128);

      return m_LastPlayerAddr;
   }

   // Get the name of the current player, or one of the network players
   STDMETHOD_(const char *, GetPlayerNameByNum)(int playerNum)
   {
      if (!gm_bNetworkGame || (playerNum == m_MyPlayerNum))
         return GetPlayerName(OBJ_NULL);
      else
         return m_PlayerInfo.GetPlayerName(playerNum);
   }

   // Get the address of the specified player.
   STDMETHOD_(const char *, GetPlayerAddressByNum)(int playerNum)
   {
      if (!gm_bNetworkGame || (playerNum == m_MyPlayerNum))
         return GetPlayerAddress(OBJ_NULL);
      else
         return m_PlayerInfo.GetPlayerAddress(playerNum);
   }

   // Do any appropriate processing based on the current session's
   // network capabilities. Should only be run after we have established
   // a session.
   void PrepCaps()
   {
#ifdef PLAYTEST
      // @TBD: we do realize when we call this dwHundredBaud is always 0?
      DPCAPS caps;
      caps.dwSize = sizeof(DPCAPS);
      if (SUCCEEDED(gm_Net->GetCaps(&caps, 0))) {
         m_NetStats.SetOverhead(caps.dwHeaderLength, caps.dwHundredBaud);
         m_packetOverhead = caps.dwHeaderLength;  // not to be confused with m_PacketOverhead!
         NetSpew(("NET: Packet overhead is %d; bandwidth is %d.\n",
                  caps.dwHeaderLength,
                  caps.dwHundredBaud));
      }
#endif
   }

   // Host a session
   STDMETHOD_(BOOL, Host)(const char *pMedia, const char *pSession)
   {
      // If no explicit media is given, use the default. Ditto for session.
      if (!pMedia) {
         pMedia = m_Media;
      }
      if (!pSession) {
         pSession = m_SessionName;
      }

      if (gm_Net->Host((char *) pMedia, (char *) pSession)) {
         gm_PlayerDPID = gm_Net->SimpleCreatePlayer(m_PlayerName);
         gm_bNetworkGame = TRUE;
         gm_bAmSessionHost = TRUE;
         // The host doesn't get a kNetMsg_Hi message, so we initilize
         // these here.
         m_MyPlayerNum = 1;

         PrepCaps();

         // Initialize voice chat:
         if (config_is_defined("net_voice"))
            StartVoiceChat(NULL);

         Status("Now net host");
         NetSpew(("MY PLAYER NUM = %d\n", m_MyPlayerNum));

         NotifyListeners(kNetMsgNetworkGame, NULL);
         return TRUE;
      } else {
         Warning(("Couldn't host networked game!\n"));
         Status("Failed to host");
         gm_Net->Close();
         ClearState();
         return FALSE;
      }
   }

   // Join a session
   STDMETHOD_(BOOL, Join)(const char *pMedia, const char *pSession,
                          const char *pAddress)
   {
      // If no explicit media is given, use the default. Ditto for session.
      if (!pMedia) {
         pMedia = m_Media;
      }
      if (!pSession) {
         pSession = m_SessionName;
      }

      AssertMsg(pAddress, "No address given to join at!");

      if (gm_Net->Join((char *) pMedia, (char *) pSession, (char *) pAddress))
      {
         gm_PlayerDPID = gm_Net->SimpleCreatePlayer(m_PlayerName);
         gm_bNetworkGame = TRUE;
         char temp[128];
         sprintf(temp, "Joined %s", pAddress);
         Status(temp);
         if (pAddress != m_HostAddr) {
            strcpy(m_HostAddr, pAddress);
         }

         PrepCaps();

         if (config_is_defined("net_voice"))
            StartVoiceChat((char *) pAddress);

         NotifyListeners(kNetMsgNetworkGame, NULL);

         GetOtherPlayers();

         return TRUE;
      } else {
         Warning(("Couldn't join networked game!\n"));
         char temp[128];
         sprintf(temp, "Failed to join %s", pAddress);
         gm_Net->Close();
         ClearState();
         Status (temp);
         return FALSE;
      }
   }

   // End a session
   STDMETHOD_(void, Leave)()
   {
      if (gm_bNetworkGame) {
         SendQuitMessage();
         gm_Net->DestroyPlayer(gm_PlayerDPID);
         gm_Net->Close();
         NotifyListeners(kNetMsgNetworkLost, FALSE);
         ClearState();
         gm_bNetworkGame = FALSE;
      }
   }

   STDMETHOD_(ulong, GetTimeout)(void)
   {
      return m_playerTimeout;
   }

   STDMETHOD_(void, SetTimeout)(ulong timeout)
   {
      m_playerTimeout = timeout;
   }

   // Temporarily suspend outgoing messages. 
   STDMETHOD_(void, SuspendMessaging)(void)
   {
      if (m_SuspendMessaging == 0) {
         ConfigSpew("net_suspend_spew", ("Suspending Messages...\n"));
      }
      m_SuspendMessaging++;
   }

   // Is networking suspended?
   STDMETHOD_(int, Suspended)(void)
   {
      return m_SuspendMessaging;
   }

   // Resume temporarily suspended outgoing messages. 
   STDMETHOD_(void, ResumeMessaging)(void)
   {
      m_SuspendMessaging--;
      AssertMsg(m_SuspendMessaging >= 0,"negative m_SuspendMessageDepth");
      if (m_SuspendMessaging == 0) {
         ConfigSpew("net_suspend_spew", ("... Resuming Messages.\n"));
      }
   }

   // Send this message to 'player'
   // Note that you can *always* send to yourself, even if networking
   // isn't turned on. This is to simplify host/client code.
   STDMETHOD_(void, Send)(ObjID player, 
                          void *msg, 
                          ulong size, 
                          BOOL guaranteed)
   {
      sNetPlayer *netinfo;

#ifndef NEW_NETWORK_ENABLED
      CriticalMsg("Trying to send msg when networking not compiled!");
#endif

      if (player == PlayerObject()) {
         // This message is simply local, so just dispatch it. We allow this
         // case even in a non-networked game, for code simplicity in
         // separating the "client" and "host" parts of the code:
         DispatchNetMsg((sNetMsg_Generic *) msg, size, player, 0);
         return;
      }

      AssertMsg(gm_bNetworkGame, "Trying to send msg in non-network game!");

      gm_NetPlayerProp->Get(player, &netinfo);

      AssertMsg1(netinfo, "Trying to send msg to non-player %d!", player);
      AssertMsg1(netinfo->dpId, "Trying to send msg to player %d w/o DPID!",
                 player);

      // Don't send any messages until we have entered
      // GameMode the first time.
      if (!m_SuspendMessaging && gm_bNetworkingReady && netinfo != NULL) {
         SendOrBundle(netinfo->dpId,msg,size,guaranteed);
      } else {
         Warning(("Direct Send() message suppressed due to Suspend!\n"));
         //ConfigSpew("net_suppress_spew", ("SEND MSG SUPPRESSED\n"));
      }
   }

   // Send this message to all other players.
   STDMETHOD_(void, Broadcast)(void *msg, ulong size, BOOL guaranteed)
   {
      // Don't send any messages until we have entered
      // GameMode the first time.
#ifndef NEW_NETWORK_ENABLED
      CriticalMsg("Trying to send msg when networking not compiled!");
#endif
      AssertMsg(gm_bNetworkGame, "Trying to send msg in non-network game!");

      if (!m_SuspendMessaging && gm_bNetworkingReady) {
         SendOrBundle(DPID_ALLPLAYERS,msg,size,guaranteed);
      } else {
         ConfigSpew("net_suppress_spew", ("SEND MSG SUPPRESSED\n"));
      }
   }

   // Send this message to all other players, even if networking isn't
   // nominally ready.
   STDMETHOD_(void, MetagameBroadcast)(void *msg, ulong size, BOOL always)
   {
#ifndef NEW_NETWORK_ENABLED
      CriticalMsg("Trying to send msg when networking not compiled!");
#endif
      AssertMsg(gm_bNetworkGame, "Trying to send msg in non-network game!");

      // Yes, this is a horrid hack, but it works:
      BOOL saveNonNetworkLevel = m_bNonNetworkLevel;
      if (always)
         m_bNonNetworkLevel = FALSE;

      SendOrBundle(DPID_ALLPLAYERS,msg,size,TRUE);

      if (always)
         m_bNonNetworkLevel = saveNonNetworkLevel;
   }

   // The host that hosts most in-world objects (i.e. not in someone's
   // inventory, nor an AI).
   // If we are in NonNetworkLevel, then we return ourself, since we
   // aren't talking with the real host.
   // This isn't necessarily reliable before we have synched.
   STDMETHOD_(ObjID, DefaultHost)(void)
   {
      if (m_bNonNetworkLevel) {
         return PlayerObject();
      } else if (gm_bAmSessionHost) {
         // This clause is needed because we sometimes want to test this
         // before we set gm_DefaultHostPlayer:
         return PlayerObject();
      } else {
         return gm_DefaultHostPlayer;
      }
   }

   // Is this machine the default host. Unlike DefaultHost(), this works
   // very early in the intialization process (after netman::Init()).
   STDMETHOD_(BOOL, AmDefaultHost)(void)
   {
      if (m_bNonNetworkLevel) {
         return TRUE;
      } else {
         return gm_bAmSessionHost;
      }
   }

   // Convert a player objID into a player identifier that can be
   // passed as part of a message.
   STDMETHOD_(tNetPlayerID, ToNetPlayerID)(ObjID player)
   {
      sNetPlayer *netinfo;
      if (player == PlayerObject())
         return gm_PlayerDPID;
      else if (gm_NetPlayerProp->Get(player, &netinfo))
              return netinfo->dpId;
      else
         return NULL_NET_ID;
   }

   // Convert from a network passable player identifier to the ObjID of
   // that player.
   STDMETHOD_(ObjID, FromNetPlayerID)(tNetPlayerID netPlayer)
   {
      ObjID obj = OBJ_NULL;
      long data;   // Local player data must be 4 bytes.
      ulong size=4;
      if (netPlayer == gm_PlayerDPID)
         obj = PlayerObject();
      else if (SUCCEEDED(gm_Net->GetPlayerData((DPID)netPlayer, 
                                               &data, 
                                               &size, 
                                               DPGET_LOCAL)))
      {
         obj = (ObjID)data;
      } else {
         // If there are waiting avatars, this might be attached to one
         // of them, so check them all out. This should only happen
         // rarely.
         ObjID player;
         sPropertyObjIter iter;
         sNetPlayer *netinfo;
         gm_NetPlayerProp->IterStart(&iter);
         while ((obj == OBJ_NULL) &&
                gm_NetPlayerProp->IterNext(&iter, &player))
         {
            gm_NetPlayerProp->Get(player, &netinfo);
            if (netinfo->dpId == netPlayer) {
               obj = player;
            }
         }
      }
      return obj;
   }

   STDMETHOD_(tNetMsgHandlerID, RegisterMessageParser)
      (tNetMessageParser parser,
       const char *moduleName,
       eNetHandlerFlags flags,
       void *pClientData)
   {
      Assert_(strlen(moduleName) < sizeof(Label));
      sMessageParserInfo parserInfo;
      parserInfo.parser = parser;
      strcpy(parserInfo.moduleName.text, moduleName);
      parserInfo.flags = flags;
      parserInfo.numReceived = 0;
      parserInfo.bytesReceived = 0;
      parserInfo.clientData = pClientData;
      unsigned ix = m_Parsers.Append(parserInfo);
      AssertMsg(ix == (tNetMsgHandlerID)ix, 
                "Too many registered network message parsers");
      return (tNetMsgHandlerID)ix;
   }

   // Register a listener to be called when the appropriate event(s)
   // occur. Note that the handle is *not* equal to the array index,
   // except coincidentally.
   STDMETHOD_(tNetListenerHandle, Listen)
      (tNetListenerCallback callback,
       eNetListenMsgs interests,
       void *pClientData)
   {
      m_LastListener++;
      cNetListener *entry = 
         new cNetListener(callback, interests, pClientData, m_LastListener);
      m_Listeners.Append(entry);
      return m_LastListener;
   }

   STDMETHOD_(void, Unlisten)(tNetListenerHandle handle)
   {
      int i;
      for (i = 0; i < m_Listeners.Size(); i++) {
         cNetListener *entry = m_Listeners[i];
         if (entry->m_handle == handle) {
            m_Listeners.DeleteItem(i);
            delete entry;
            return;
         }
      }
   }

   // Tell all the appropriate listeners about an event
   void NotifyListeners(eNetListenMsgs situation, DWORD data)
   {
      int i;
      for (i = 0; i < m_Listeners.Size(); i++) {
         cNetListener *entry = m_Listeners[i];
         if (entry->m_interests & situation) {
            (entry->m_callback)(situation, 
                                data, 
                                entry->m_pClientData);
         }
      }
   }

   // Factory function for our loop client.
   // It is public so that it can be seen by NetworkLoopClientDesc
   static ILoopClient* LGAPI _CreateClient(sLoopClientDesc * pDesc,
                                           tLoopClientData data)
   {
      StateRecord* state;
      // allocate space for our state, and fill out the fields
      state = new StateRecord;
      state->netman = gm_TheNetManager;

      return CreateSimpleLoopClient(_LoopFunc,state,&NetworkLoopClientDesc);
   }

   // Factory function for our loop client.
   // It is public so that it can be seen by NetworkLoopClientDesc
   static ILoopClient* LGAPI _CreateResetClient(sLoopClientDesc * pDesc,
                                                tLoopClientData data)
   {
      StateRecord* state;
      // allocate space for our state, and fill out the fields
      state = new StateRecord;
      state->netman = gm_TheNetManager;

      return CreateSimpleLoopClient(_ResetLoopFunc,
                                    state,
                                    &NetworkResetLoopClientDesc);
   }

   ///////////////////////
   //
   // Routines for iterating over the network players (doesn't include
   // the local player) (Same as the players with the private
   // NetPlayerProperty).
   //
   // @NOTE: if gm_NetPlayerProp ever changes to a
   // non-linked list implementation, we will either need to add an
   // IterStop (which kills the FOR_ALL_NET_PLAYERS macro) or we would
   // need to come up with our own iteration.
   //
   STDMETHOD_(void, NetPlayerIterStart) (sPropertyObjIter* iter) const
   { 
      gm_NetPlayerProp->IterStart(iter);
   }
   STDMETHOD_(BOOL, NetPlayerIterNext) (THIS_ sPropertyObjIter* iter,
                                        ObjID* next) const
   {
      return gm_NetPlayerProp->IterNext(iter, next);
   }

   ///////////////////////////////
   //
   // Routines to convert to or from a global representation of an
   // object, for when the object is not known to be hosted by either
   // the sender or the receiver of the message. These *really* ought
   // to be in ObjectNetworking, but they predate it...
   //
   STDMETHOD_(sGlobalObjID, ToGlobalObjID)(ObjID obj)
   {
      sGlobalObjID result = { 0, OBJ_NULL };
      if (obj != OBJ_NULL)
      {
         if (!gm_bNetworkGame) {
            // This is simply for a loopback
            result.host = 0;
            result.obj = obj;
         } else if (!PlayerObjectExists() ||
                    gm_ObjNet->ObjLocalOnly(obj) ||
                    !gm_DefaultHostPlayer)
         {
            // This is a very early loopback, or a non-networkable
            // object:
            result.host = MyPlayerNum();
            result.obj = obj;
         } else {
            ObjID hostObj = gm_ObjNet->ObjHostPlayer(obj);
            AssertMsg1((hostObj != OBJ_NULL), 
                       "Object %d has no host player, but isn't local!",
                       obj);
            result.host = ObjToPlayerNum(hostObj);
         
            AssertMsg((result.host != 0),
                      "Object host player unknown to networking");
            result.obj = gm_ObjNet->ObjHostObjID(obj);

            if ((result.obj == OBJ_NULL) && (obj != OBJ_NULL))
            {
               // Okay, this thing's in a strange state. An ObjHostObjID of
               // OBJ_NULL means that we've given this to another player,
               // but we haven't yet received the new objID for the thing.
               // In this case, we have to just use our own info, and
               // assume that everyone else still has that on file to
               // deproxify from. This *should* only happen for objects
               // that are in transition like this.
               result.host = MyPlayerNum();
               result.obj = obj;
#ifdef PLAYTEST
               if (config_is_defined("net_proxy_spew")) {
                  mprintf("Object %d is now owned by %d, but I don't have"
                          " the objID yet, so using my own.\n",
                          obj, hostObj);
               }
#endif
            }
         }
      }
      return result;
   }

   STDMETHOD_(ObjID, FromGlobalObjID)(sGlobalObjID *gid) 
   {
      if (!gm_bNetworkGame)
         // It's just a loopback
         return gid->obj;

      if (gid->host == 0)
      {
         if (PlayerObjectExists())
         {
            return OBJ_NULL;
         } else {
            // It's a very early loopback
            return gid->obj;
         }
      }

      if (gid->obj == OBJ_NULL)
         return OBJ_NULL;
  
      ObjID hostObj = PlayerNumToObj(gid->host);
      if (hostObj == OBJ_NULL)
      {
         Warning(("Trying to convert a global obj id from an unknown host\n"));
         return OBJ_NULL;
      }
      else if (hostObj == PlayerObject())
         return gid->obj;
      else
         return gm_ObjNet->ObjGetProxy(hostObj, gid->obj);
   }

   //////////////////////////////////////////////////////////////////////////
   // Routines for reporting network statistics.

#ifdef PLAYTEST   
   // Bandwidth used in previous 32 frames.  All parameters are output
   // parameters in seconds or bytes/second.
   STDMETHOD_(void, BandwidthStats)(float *timeSpan,
                                    ulong *sendBW,
                                    ulong *receiveBW,
                                    const char **culprit) 
   {
      m_NetStats.BandwidthStats(timeSpan, sendBW, receiveBW, culprit);
   }

   // Bytes sent and received in the _previous_ frame.
   STDMETHOD_(void, FrameStats)(ulong *bytesSent, ulong *bytesReceived, const char **culprit) 
   {
      m_NetStats.FrameStats(bytesSent, bytesReceived, culprit);
   }

   STDMETHOD_(void, MaxStats)(ulong *bytesSent, ulong *bytesReceived) 
   {
      m_NetStats.MaxStats(bytesSent, bytesReceived);
   }
#endif // PLAYTEST

   STDMETHOD_(ObjID, OriginatingPlayer)()
   {
      if (m_OriginatingPlayer == OBJ_NULL) {
         return PlayerObject();
      } else {
         return m_OriginatingPlayer;
      }
   }

   //////////
   //
   // Synchronization code
   //
   // This is a tad messy, because we can't *begin* to synchronize until
   // everyone has at least reset their databases. Otherwise, avatars
   // that are sent along could be scragged by the dbReset. So it's really
   // a two-phase synchronization: first a phase to wait until everyone
   // has said that they have reset, then a second to actually synchronize
   // information. Less efficient than I might wish, but it works...
   //

   // A player has indicated that they are finished resetting:
   void HandleStartSynchMsg()
   {
      m_PlayersReset++;
   }

   // Actually send our synchronization messages. This doesn't happen until
   // everyone has indicated that they have reset their databases, so
   // they can safely cope:
   void SendSynchMessages()
   {
      // Okay, it's now too late for any more players to join:
      m_bGameStarted = TRUE;

      if (gm_bNetworkGame)
      {
         SendCreatePlayerNetMsg();
         SendPlayerInfo();
      }

      // @TBD: this could probably be in a listener:
      //m_pPlayerInfoMsg->Send(OBJ_NULL, m_PlayerName);

      // Call callbacks that want to know that we are in the middle of
      // synchronizing, and they should send any synch data. Only
      // things that do *not* need to know about the other players
      // should listen in on this; ones that do need to know the other
      // players' objIDs must listen to Networking instead:
      NotifyListeners(kNetMsgSynchronize, NULL);

      // Finally, tell the others that we are now ready to join the game:
      m_pFinishSynchMsg->Send(OBJ_NULL);

      // We generally leave m_PlayersReset at 0, unless we're actively
      // in the process of starting up:
      m_PlayersReset = 0;
   }

   // Begin the process of synchronizing with other players
   //
   // @TBD: in the long run, this probably should be restructured into
   // its own major mode. That will require a general restructuring of
   // netman, to separate the high-level player code from the low-level
   // stuff. Big project, although one that should definitely happen.
   // It'll be a lot easier once networking is no longer dependent on
   // player ObjIDs...
   STDMETHOD_(void, StartSynch)()
   {
      ObjID player = PlayerObject();
      if (player == OBJ_NULL) {
         Warning(("Can't JoinUp, because player is OBJ_NULL!\n"));
         return;
      }
      if (gm_bNetworkingReady) {
         Warning(("Trying to JoinUp when we're already set.\n"));
         return;
      }

      gm_bSynchronizing = TRUE;

      // Tell the other players that we're done with the reset process,
      // and ready to synchronize:
      m_PlayersReset++;
      m_pStartSynchMsg->Send(OBJ_NULL);

      // First, since the player is the only object that is created
      // before sim start that is not owned by the DefaultHost (@TBD:
      // Check this), we need to register it.  The player is the host,
      // the host obj id, and the proxy obj id.
      gm_ObjNet->ObjRegisterProxy(player, player, player);

      NetSpew(("Waiting for players to join\n"));

      // If we have been in NonNetworkLevel, and there are pending messages,
      // deal with them now:
      if (m_bStoringMessages) {
         m_bStoringMessages = FALSE;
         m_bCreateContentProxies = TRUE;
         cStoredMessage *pStored, *pNext;
         for (pStored = m_StoredMessages.GetFirst();
              pStored;
              pStored = pNext)
         {
            pNext = pStored->GetNext();
            m_StoredMessages.Remove(pStored);
            HandleRawNetMsg(pStored->m_From, pStored->m_Msg, pStored->m_Size);
            delete pStored;
         }
      }

      if (m_PlayersReset == m_NumPlayers) {
         SendSynchMessages();
      }
   }

   // Another player has finished sending their synchronize info, and is
   // ready to join the game:
   void HandleFinishSynchMsg()
   {
      m_PlayersJoined++;
   }

   // This should get called each frame after polling the network, to
   // determine whether we've finished synchronizing.
   BOOL SynchReady() {
      if (!gm_bNetworkGame)
         return TRUE;

      // First, check whether we're still waiting to send out our
      // own Synch messages:
      if (m_PlayersReset) {
         // We're still in reset synchronization. Are we ready to
         // finish that?
         if (m_PlayersReset == m_NumPlayers) {
            SendSynchMessages();
         }
      }

      // Now, check whether everyone else has joined up:
      if (m_MyPlayerNum > 0 && m_PlayersJoined == m_NumPlayers-1) {
         // Everyone else has synched up with us
         return TRUE;
      } else {
         return FALSE;
      }
   }

   // Finish up the synchronization process
   void FinishSynch() {
      // If we decided not to synchronize, don't do anything here:
      if (!gm_bSynchronizing)
         return;

      gm_bSynchronizing = FALSE;

      if (!gm_bNetworkGame)
         // This means that we lost the network sometime during load,
         // so just give up:
         return;

      m_PlayersJoined++; // has to be after PollNetwork, due to its Assertion.

      NetSpew(("All %d players have joined\n", m_NumPlayers));

      // Okay, we're ready to admit messages now:
      gm_bNetworkingReady = TRUE;

      // Tell anyone who wants to be notified that we're officially
      // Networking now. (Mainly for modules that don't want the overhead
      // of calling Networking() frequently.) This list includes objnet,
      // so it needs to be done promptly.
      NotifyListeners(kNetMsgNetworking, NULL);

      ObjID player = PlayerObject();

      // @TBD: Some of this should probably get moved out to listeners...
      // Tell the other players about this one's interesting properties.
      // For now, we just touch the ones we're interested in.
      // @TBD: devise a good method to transmit all of a specific object's
      // properties.
      int hp;
      ObjGetHitPoints(player, &hp);
      ObjSetHitPoints(player, hp);
      ObjGetMaxHitPoints(player, &hp);
      ObjSetMaxHitPoints(player, hp);

      // Tell the other players about this one's contents, just in case
      // we have been assigned initial content:
      AutoAppIPtr(ContainSys);
      AutoAppIPtr(TraitManager);
      sContainIter *pIter;
      for (pIter = pContainSys->IterStart(player);
           !pIter->finished;
           pContainSys->IterNext(pIter))
      {
         ObjID content = pIter->containee;
         // Take over responsibility for this thing
         if (m_bCreateContentProxies) {
            // We've been in NonNetworkLevels, so any player content
            // presumptively doesn't have proxies on the other machines.
            // Create those proxies.
            if (gm_ObjNet->ObjIsProxy(content)) {
               // It's a hosted object; register it to us:
               gm_ObjNet->ObjRegisterProxy(player, content, content);
            }
            if (gm_ObjNet->ObjHostedHere(content)) {
               // Tell the other players to create a proxy:
               gm_ObjNet->StartBeginCreate
                  (pTraitManager->GetArchetype(content), content);
               gm_ObjNet->FinishBeginCreate(content);
               gm_ObjNet->StartEndCreate(content);
               gm_ObjNet->FinishEndCreate(content);
               // It is always the responsibility of an object's owner
               // to decide on its refs. Normally, this happens when the
               // object is picked up, but in this case, we "took" it while
               // in single-player. So tell everyone else that it's unref'ed:
               ObjSetHasRefs(content, FALSE);
               m_pPlayerContentsMsg->Send(OBJ_NULL, content);
            }
         } else {
            // @NOTE: This is safety code, which was used extensively in
            // the early days. Now, it should be redundant, and is costing
            // a lot of bandwidth during level transitions, so it is
            // commented out.
            // Normal situation, moving between levels
            if (gm_ObjNet->ObjIsProxy(content)) {
               // I contain a proxy? This makes sense in Thief, where we
               // may have been assigned initial content at level start:
               gm_ObjNet->ObjTakeOver(content);
            }
#if 0
            // And tell the other players that I have it, unless it's a
            // local object:
            if (gm_ObjNet->ObjHostedHere(content)) {
               m_pPlayerContentsMsg->Send(OBJ_NULL, content);
            }
#endif
         }
      }
      pContainSys->IterEnd(pIter);
      m_bCreateContentProxies = FALSE;

      // Now, if there are any leftover avatars for players who didn't
      // come back, kill them. This only happens on the host, which is
      // responsible for such executions.
      if (AmDefaultHost()) {
         int i;
         for (i = 1; i < m_WaitingAvatars.Size(); i++) {
            if (m_WaitingAvatars[i] != NULL) {
               IAvatar *pAvatar = m_WaitingAvatars[i];
               // It's a leftover avatar
               KillMoribundPlayer(pAvatar->PlayerObjID(), pAvatar);
               m_WaitingAvatars[i] = NULL;
            }
         }
      }
   }

   // We are entering a situation where we should be doing *no* networking.
   STDMETHOD_(void, NonNetworkLevel)()
   {
      NetRawSpew(("Entering NonNetworkLevel.\n"));

      // This will remain TRUE until we get NormalLevel():
      m_bNonNetworkLevel = TRUE;
      // This will remain TRUE until we get a Synch:
      m_bStoringMessages = TRUE;
   }

   // We're now returning to normal.
   STDMETHOD_(void, NormalLevel)()
   {
      NetRawSpew(("Entering NormalLevel.\n"));

      m_bNonNetworkLevel = FALSE;
   }

   //////////
   //
   // SynchFlush
   //
   // This mechanism is used when everyone needs to clear their
   // buffers. It is mainly intended for use at save time, when we
   // need to make sure everyone is entirely synchronized.
   //

   // Information about each round:
   typedef struct _round {
      // The number of players who have synchFlushed in this round so far:
      int playersFlushed;
      // Number of messages I've sent in this round:
      int msgsSentByMe;
      // Has anyone sent any messages?
      BOOL anyMsgsSent;
   } sSynchRound;
   
   // The maximum number of rounds to allow before assuming things have
   // gone to hell:
   #define MAX_SYNCH_ROUNDS 8

   // Are we in the middle of synchronizing?
   BOOL m_bSynchFlushing;
   // The actual info about each round:
   sSynchRound m_round[MAX_SYNCH_ROUNDS];
   // The current round that we are working on:
   int m_curRound;
   // A "stack" of pending flushes, for the rare case when multiple players
   // decide to flush at once:
   int m_flushStack;

   void HandleSynchFlushMsg(int round, int numMsgs) {
      m_round[round].playersFlushed++;
      if (numMsgs > 0)
         m_round[round].anyMsgsSent = TRUE;
   }

   // Clear out the variables for a flush...
   void ClearFlush()
   {
      m_curRound = 0;

      // Clear out the synch information:
      int i;
      for (i = 0;
           i < MAX_SYNCH_ROUNDS;
           i++)
      {
         m_round[i].playersFlushed = 0;
         m_round[i].msgsSentByMe = 0;
         m_round[i].anyMsgsSent = FALSE;
      }

      m_bSynchFlushing = FALSE;

      // If a new request has come in during the last one, then start
      // again...
      if (m_flushStack > 0)
      {
         PreFlush();
         m_flushStack--;
      }
   }

   // Flush buffers, and synchronize with the other machines.
   // Assumes that the other machines have been told to do likewise.
   STDMETHOD_(void, PreFlush)()
   {
      if (!gm_bNetworkGame)
         return;

      // Are we already in the middle of a synch? If so, then "stack"
      // this request. It'll get dealt with when we finish the current
      // round...
      if (m_bSynchFlushing)
      {
         NetSpew(("Got a recursive request to PreFlush; stacking...\n"));
         m_flushStack++;
         return;
      }

      NetSpew(("Prepping for a SynchFlush...\n"));
      m_bSynchFlushing = TRUE;

      // Send any outstanding message bundles; bundling can confuse the
      // synch process, so let's not get into it:
      SendBundles();

      // m_PendingMsgIDs is the messages that have been sent, but not
      // yet acknowledged. Any messages we have pending at this point
      // are potentially dangerous: they could cause the recipients to
      // kick off other messages.
      m_round[0].msgsSentByMe = m_PendingMsgIDs.Size();
   }

   //
   // Flush all messages
   //
   // We wait until all known messages have successfully been sent,
   // before continuing. The complication here is the possibility of
   // messages that cause other messages to be sent. Because of this,
   // we operate in "rounds". We don't exit until everyone says that
   // they haven't sent any messages this round.
   //
   STDMETHOD_(BOOL, SynchFlush)(tNetSynchFlushCallback callback,
                             void *pClientData)
   {
      if (!gm_bNetworkGame)
         return TRUE;

      BOOL finished = FALSE;
      // We are sometimes totaling up the previous round, even while
      // we're accumulating info for the next one:
      int prevRound;

      NetSpew(("Flushing outgoing network message buffer...\n"));

      while (!finished) {

         NetSpew(("Starting flush round %d\n", m_curRound));

         while (m_PendingMsgIDs.Size() > 0) {
            PollNetwork();
            SendBundles();
            if (callback) {
               if (!callback(pClientData)) {
                  // The callback has told us to give up
                  NetSpew(("Giving up on SynchFlush...\n"));
                  ClearFlush();
                  return FALSE;
               }
            }
         }

         // Okay, now send the synch message, and continue to wait
         m_pSynchFlushMsg->Send(OBJ_NULL, 
                                m_curRound, 
                                m_round[m_curRound].msgsSentByMe);
         // Count that we have synchronized:
         m_round[m_curRound].playersFlushed++;
         // And start gathering info for the next round, if needed:
         prevRound = m_curRound;
         m_curRound++;
         if (m_curRound > MAX_SYNCH_ROUNDS) {
            CriticalMsg("Too many rounds of synchronization!");
            ClearFlush();
            return FALSE;
         }
         while (m_round[prevRound].playersFlushed < m_NumPlayers) {
            PollNetwork();
            SendBundles();
            if (callback) {
               if (!callback(pClientData)) {
                  // The callback has told us to give up
                  NetSpew(("Giving up on SynchFlush...\n"));
                  ClearFlush();
                  return FALSE;
               }
            }
         }
         // Check if anyone has sent any messages. Note that I will always
         // indicate at least 1: the SynchFlush message itself.
         AssertMsg(m_round[prevRound].msgsSentByMe >= 1,
                   "Somehow, I don't think I sent a synch message!");
         if (!m_round[prevRound].anyMsgsSent &&
             (m_round[prevRound].msgsSentByMe == 1))
         {
            // Let's check to make sure we haven't gone and sent any
            // further messages in the current round:
            AssertMsg(m_round[m_curRound].msgsSentByMe == 0,
                      "Argh! I've sent messages after synchronizing!");
            // Yay! Everyone is finished!
            finished = TRUE;
         }
      }

      NetSpew(("Finished SynchFlushronizing...\n"));

      // Okay, everyone's together:
      ClearFlush();
      return TRUE;
   }

   // Deal with someone else telling us about themselves. This is pretty
   // hackish, since it's dealing with two separate chunks of info, but
   // it's convenient for now.
   void HandlePlayerInfoMsg(ObjID player, 
                            int playerNum, 
                            const char *pName,
                            const char *pAddress)
   {
      sNetPlayer *pPlayerDesc;
      if (gm_NetPlayerProp->Get(player, &pPlayerDesc)) {
         strncpy(pPlayerDesc->name, pName, MAX_PLAYER_NAME_LEN);
         // Make sure it's NULL-terminated:
         pPlayerDesc->name[MAX_PLAYER_NAME_LEN] = '\0';
      } else {
         // This is now legit, since we can run before synch time:
         // Warning(("Got name for unknown player %d\n", player));
      }

      // And put it into the table by player number:
      m_PlayerInfo.SetInfo(playerNum, pName, pAddress);
      // And tell any listeners about it:
      NotifyListeners(kNetMsgPlayerInfo, playerNum);
   }

   // Send other random information about this player. Safe to call very
   // early, and repeatedly if needed.
   void SendPlayerInfo()
   {
      if (gm_bNetworkGame)
      {
         m_pPlayerInfoMsg->Send(OBJ_NULL, 
                                m_MyPlayerNum, 
                                GetPlayerName(OBJ_NULL),
                                GetPlayerAddress(OBJ_NULL));
      }
   }

   STDMETHOD_(void, SetMaxPlayers)(int maxPlayers)
   {
      m_MaxPlayers = maxPlayers;
   }

   STDMETHOD_(const char *, GetDefaultHostName)()
   {
#ifdef PLAYTEST
      return "HOST NAME METHOD NOW OBSOLETE";
#else
      return "Host";
#endif      
   }
};

// Definition of static members
cNetManager   *cNetManager::gm_TheNetManager = NULL;
INetPlayerProperty *cNetManager::gm_NetPlayerProp = NULL;
IObjectSystem *cNetManager::gm_ObjSys = NULL;
IObjectNetworking *cNetManager::gm_ObjNet = NULL;
tNetMsgHandlerID cNetManager::gm_NetManagerHandlerID = 0;
INet *cNetManager::gm_Net = NULL;
ObjID cNetManager::gm_DefaultHostPlayer = OBJ_NULL;
DPID cNetManager::gm_PlayerDPID = DPID_UNKNOWN;
BOOL cNetManager::gm_bAmSessionHost = FALSE;
BOOL cNetManager::gm_bNetworkingReady = FALSE;
BOOL cNetManager::gm_bNetworkGame = FALSE;
BOOL cNetManager::gm_bSynchronizing = FALSE;
tSimTime cNetManager::gm_SimTime = 0;
BOOL cNetManager::gm_bInGameMode = FALSE;
// Init to 1ms, to head off divide-by-zero bugs:
ulong cNetManager::gm_GameTime = 1;
ulong cNetManager::gm_EnteredGameMode = 0;
BOOL cNetManager::gm_bDoSpew = FALSE;

//
// Creation function for the one true net manager
//
extern "C"
void NetManagerCreate(void)
{
   IUnknown* outer = AppGetObj(IUnknown); 
   cNetManager* net = new cNetManager(outer);
}

// The public loop client descriptor
// used by loopapp.c
// Note that clients of RegisterMessageParser do *not* need to constrain
// themselves after Netman's kMsgAppInit! (And generally shouldn't.)
sLoopClientDesc NetworkLoopClientDesc =
{
   &LOOPID_Network,
   "Network Loop Client",              
   kPriorityNormal,              
   kMsgsAppOuter | kMsgDatabase | kMsgsMode | kMsgNormalFrame | kMsgEnd, // interests 

   kLCF_Callback,
   cNetManager::_CreateClient,
   
   NO_LC_DATA,

   {
      // After creating the PhysControl prop:
      { kConstrainAfter, &LOOPID_Physics, kMsgAppInit },
      // After we poll the keyboard:
      { kConstrainAfter, &LOOPID_UI, kMsgNormalFrame },
      // So physics sees network updates:
      { kConstrainBefore, &LOOPID_Physics, kMsgNormalFrame },
      // So we don't load avatars until after the creatures have been loaded:
      { kConstrainAfter, &LOOPID_Biped, kMsgDatabase },
      {kNullConstraint}
   }
};

// This stupid little horror is due to the fact that we *really* need
// to handle dbReset very early, but we need to handle dbLoad pretty
// late. So we have this extra loop client, whose sole purpose is to
// reset the Networking flag. Oi. Consider this a fine argument for
// splitting the kMsgDatabase into separate messages...
sLoopClientDesc NetworkResetLoopClientDesc =
{
   &LOOPID_NetworkReset,
   "Network Reset Loop Client",              
   kPriorityNormal,              
   kMsgDatabase | kMsgEnd, // interests 

   kLCF_Callback,
   cNetManager::_CreateResetClient,
   
   NO_LC_DATA,

   {
      // This list should include all systems that try to send messages
      // during dbReset:
      { kConstrainBefore, &LOOPID_Physics, kMsgDatabase },
      { kConstrainBefore, &LOOPID_PhysicsBase, kMsgDatabase },
      { kConstrainBefore, &LOOPID_ObjSys, kMsgDatabase },
      {kNullConstraint}
   }
};


//////////
//
// Message Handlers
//
// For reasons I don't entirely grok but don't have time to deal with,
// forward-referencing cNetManager doesn't seem to work. So I've put
// these static handlers down here for now...
//

static void handleFinishSynchMsg(cNetManager *pNetManager)
{
   pNetManager->HandleFinishSynchMsg();
}

static void handleSynchFlushMsg(int round, int numMsgs, 
                                cNetManager *pNetManager)
{
   pNetManager->HandleSynchFlushMsg(round, numMsgs);
}

static void handleStartSynchMsg(cNetManager *pNetManager)
{
   pNetManager->HandleStartSynchMsg();
}

static void handlePlayerInfoMsg(int playerNum,
                                const char *pName,
                                const char *pAddress,
                                ObjID player, 
                                cNetManager *pNetManager)
{
   pNetManager->HandlePlayerInfoMsg(player, playerNum, pName, pAddress);
}
