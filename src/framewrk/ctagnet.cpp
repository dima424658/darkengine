/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/ctagnet.cpp,v 1.6 1999/06/15 13:16:33 Justin Exp $
//
// Networking for the cTags array.
//
// This stuff is pretty closely coupled with ctag.cpp; if you want to
// understand this, make sure you grok that first. This is fairly ugly
// stuff -- ctag wasn't really designed with networking in mind. If we
// ever want to grab some bandwidth efficiency, we might consider
// building the legal tag-table in advance, so it's the same on all
// machines, and none of this mapping stuff is needed...
//
// Assumption: new players don't join in mid-game. If they do, then
// broadcastTagsArray will have to be beefed up to have a way to send
// the array to new players.
//
// @TBD: this is basically all hacked into the side of ctag and ctagset;
// it was designed to be minimally invasive. We could almost certainly do
// something cleaner by restructuring the tags code and putting in a
// "tag manager", which would subsume all of this. However, ctag is used
// by static constructors in other classes, so we have to avoid anything
// that can't happen at static elaboration time...
//
// @NOTE (justin 6/15/99): Tag networking is no longer being used, and
// I've observed a strange hang that occasionally happens at synch time;
// it seems as if DPlay gets hung up in trying to send the tag array.
// So I've disabled tag networking for now.
//

#ifdef TAG_NETWORKING_ON

#include <appagg.h>
#include <hashset.h>
#include <hshsttem.h>
#include <objtype.h>

#include <netman.h>
#include <netnotif.h>
#include <netmsg.h>
#include <ctag.h>
#include <ctagset.h>
#include <ctagnet.h>
#include <cfgdbg.h>

// Must be last
#include <dbmem.h>

//////////
//
// Utility: growing block
//
// This is more or less ripped out of resstr.
// @TBD: when I get a minute to breathe, unify these and put them into
// cpptools. Make the block size specifiable at construction time.
//

// Tunable parameter: this is the increment we use for allocating the
// string block. Ideally, it should be just a bit larger than we actually
// want the final table to be. But it's only semi-relevant, since this is
// only being used for temporary memory anyway...
#define TAG_STRING_BLOCK_INCREMENT (1024)

// There's gotta be a better way to do this:
class cGrowingBlock {
public:
   cGrowingBlock()
      : m_numIncs(0),
        m_pTempBlock(NULL),
        m_curp(0),
        m_blocksize(0)
   {
      m_pTempBlock = (char *) malloc(TAG_STRING_BLOCK_INCREMENT);
      m_blocksize = TAG_STRING_BLOCK_INCREMENT;
      m_numIncs = 1;
   }

   ~cGrowingBlock()
   {
      free(m_pTempBlock);
   }

   void Append(char c)
   {
      if (m_curp >= m_blocksize) {
         // Grow the block
         m_blocksize += TAG_STRING_BLOCK_INCREMENT;
         char *pNewBlock = (char *) malloc(m_blocksize);
         memcpy(pNewBlock, m_pTempBlock, m_curp);
         free(m_pTempBlock);
         m_pTempBlock = pNewBlock;
      }
      m_pTempBlock[m_curp++] = c;
   }

   // @TBD: this could be more efficient...
   void Append(const char *s)
   {
      while (*s != '\0')
         Append(*s++);
   }

   int GetSize()
   {
      return m_curp;
   }

   char *GetBlock()
   {
      return m_pTempBlock;
   }

private:
   // The current size of the block, in increments:
   int m_numIncs;
   // The temporary holding cell for the strings:
   char *m_pTempBlock;
   // The next index to put a character into (aka the current length):
   int m_curp;
   // The current block size:
   int m_blocksize;
};

//////////
//
// Data Structures and Stuff
//

// This is just a tad dangerous: it's an extra hidden flag that we're
// giving to tags. This *must* be kept distinct from any other flags.
#define kTagFlagSent 128

// A static pointer to the Network Manager, so we don't have to keep
// fetching it
static INetManager *g_pNetManager = NULL;

// Flags indicating what phase we're in WRT networking
// We are currently Synchronizing databases:
static BOOL g_bSynchronizing = FALSE;
// We are currently in full Networking:
static BOOL g_bNetworking = FALSE;

//
// A tag mapping index
//
// This structure represents the index for this tag on a specific remote
// machine. It is used to key the hash table, from which we get the
// local version.
//
// Note that we currently assume that this will fit into 32 bits.
//
struct sTagMapping {
   uint32 toInt()
   {
      return (player << 16) | remoteIndex;
   }

   NetObjID player;
   ushort remoteIndex;
};

//
// A specific record's mapping
//
// Each of these represents a type on a specific remote system, and
// gives the mapping for that type onto our own system. It is what
// you get out of the mapping hash table.
//
class cTagMapRecord {
public:
   cTagMapRecord(int localIndex, NetObjID player, int remoteIndex)
      : m_localRecordIndex(localIndex)
   {
      m_hashIndex.player = player;
      m_hashIndex.remoteIndex = remoteIndex;
   }

   // The index in the local gm_RecordList for this type
   int m_localRecordIndex;
   // The enumerations, if relevant. The index into this array is the
   // remote value; the value of that slot is the local value.
   cDynArray<int> m_Enums;
   // The key for the hash table
   sTagMapping m_hashIndex;
};

class cHashByTagMapping 
   : public cHashSet<cTagMapRecord *, uint32, cHashFunctions>
{
public:
   cHashByTagMapping()
      : cHashSet<cTagMapRecord *, uint32, cHashFunctions>()
   {
   }

   virtual ~cHashByTagMapping()
   {
      DestroyAll();
   }

   tHashSetKey GetKey(tHashSetNode node) const
   {
      return (tHashSetKey) (((cTagMapRecord *) node)->m_hashIndex.toInt());
   }
};
#ifdef _MSC_VER
template cHashSet<cTagMapRecord *, uint32, cHashFunctions>;
#endif
static cHashByTagMapping TagMapTable;

// We currently assume that there are no more than 65535 tag types:
typedef ushort tNetTagType;
#define kTagRecordParamType kNMPT_UShort
// We also assume that there are no more than 65535 enum values for a tag:
typedef ushort tNetEnumType;
#define kTagEnumParamType kNMPT_UShort

//////////
//
// The TagRecord message
//
// Note that this handles both kTagIntRecord and kTagEnumRecord; those
// are distinguished only by the type of the message.
//

static cNetMsg *g_pTagRecordMsg = NULL;

//
// Creates a new mapping in the hash table, and makes sure that the
// tag exists locally.
//
static void handleTagRecord(tNetTagType index, 
                            BOOL isInt,
                            const char *typeName, 
                            ObjID player)
{
   // Find the corresponding record in the local table, and
   // create it if necessary:
   cTagRecord *pRecord = 
      cTag::GetOrMakeRecord(typeName,
                            (isInt ? kTagFlagInt : kTagFlagEnum));
   // Create the mapping, and insert it into the table, if we
   // don't already have it:
   sTagMapping mapKey = {player, index};
   cTagMapRecord *pMapping = TagMapTable.Search(mapKey.toInt());
   if (pMapping) {
      AssertMsg3(pRecord == cTag::GetTagRecord(pMapping->m_localRecordIndex),
                 "Bad tag remapping for %d:%s (%d)!",
                 player, typeName, index);
      ConfigSpew("net_tag_spew",
                 ("Got duplicate tag %d:%s\n", player, typeName));
   } else {
      cTagMapRecord *pMapping = new cTagMapRecord(pRecord->m_iIndex,
                                                  player,
                                                  index);
      TagMapTable.Insert(pMapping);
   }
}

static sNetMsgDesc sTagRecordDesc =
{
   kNMF_MetagameBroadcast | kNMF_AppendSenderID,
   "TagRecord",
   "Tag Record",
   "net_tag_spew",
   handleTagRecord,
   {{kTagRecordParamType, kNMPF_None, "Type Index"},
    {kNMPT_BOOL, kNMPF_None, "Is Int"},
    {kNMPT_String, kNMPF_None, "Type name"},
    {kNMPT_End}}
};

//
// Send a record out to the other players. If it already contains enums,
// send those out as well.
//
void BroadcastTagRecord(cTagRecord *pRecord)
{
   if (!g_bNetworking && !g_bSynchronizing) {
      pRecord->m_iFlags &= ~kTagFlagSent;
      return;
   }

   BOOL isInt = (pRecord->m_iFlags & kTagFlagInt);

   g_pTagRecordMsg->Send(OBJ_NULL,
                         pRecord->m_iIndex,
                         isInt,
                         pRecord->m_TypeName);

   // Now, if there are enumerations already in this record, send those
   // as well:
   if (pRecord->m_iFlags & kTagFlagEnum)
   {
      int numEnums = pRecord->m_EnumValues.Size();
      int i;
      for (i = 0; i < numEnums; i++) {
         BroadcastTagEnum(pRecord, i, pRecord->m_EnumValues[i]);
      }
   }
}

//////////
//
// The TagEnum message
//
// Describes a single enumeration value within a enum tag
//

static cNetMsg *g_pTagEnumMsg = NULL;

static void handleTagEnum(tNetTagType recordIndex,
                          tNetEnumType enumIndex,
                          const char *valueName,
                          ObjID player)
{
   // Find the appropriate record in the mapping table:
   sTagMapping mapKey = {player, recordIndex};
   cTagMapRecord *pMapping = TagMapTable.Search(mapKey.toInt());
   if (!pMapping) {
      Warning(("Got enumeration for unknown tag type %d\n", 
               recordIndex));
      return;
   }
   cTagRecord *pRecord = cTag::GetTagRecord(pMapping->m_localRecordIndex);
   AssertMsg(pRecord, "Tag mapping without a local record!");
   // Make sure it's an enumeration type:
   AssertMsg((pRecord->m_iFlags & kTagFlagEnum),
             "Got enumeration for non-enum tag!");

   // Look this enumeration up in the local table, or create it if
   // necessary:
   int localIndex = cTag::GetOrMakeEnum(pRecord, valueName);
   // Insert it into the local mapping:
   if (!pMapping->m_Enums.IsValidIndex(enumIndex)) {
      // We need to grow the array first
      pMapping->m_Enums.SetSize(enumIndex + 1);
   }
   pMapping->m_Enums[enumIndex] = localIndex;
}

static sNetMsgDesc sTagEnumDesc =
{
   kNMF_MetagameBroadcast | kNMF_AppendSenderID,
   "TagEnum",
   "Tag Enumeration Value",
   "net_tag_spew",
   handleTagEnum,
   {{kTagRecordParamType, kNMPF_None, "Type Index"},
    {kTagEnumParamType, kNMPF_None, "Enum Index"},
    {kNMPT_String, kNMPF_None, "Value name"},
    {kNMPT_End}}
};

//
// Send around a single enumeration from a tag record. (That is, a string
// value that that tag can take.)
//
void BroadcastTagEnum(cTagRecord *pRecord,
                      int index,
                      const char *pValueName)
{
   if (!g_bNetworking && !g_bSynchronizing) {
      pRecord->m_iFlags &= ~kTagFlagSent;
      return;
   }

   g_pTagEnumMsg->Send(OBJ_NULL,
                       pRecord->m_iIndex,
                       index,
                       pValueName);
}

//////////
//
// Routines for creating and translating blocks suitable for sending
// within network messages.
//

//
// The data structure for representing a single tag over the net. It's
// basically the same as the real cTag structure, but a little more
// compact, to save bytes. It explicitly assumes that there will be no
// more than 65535 tag types, and that we won't be using it for integer
// values larger than 65535. (If we *do* discover that we have tags
// holding larger ints than that, then we might consider having two forms
// of this structure, distinguished by the high bit of the tag type: one
// with a one-byte value for enums and small ints, and one with a
// four-byte value for large ints.)
//
struct sNetTag {
   ushort typeToken;
   ushort value;
};

//
// A "Net Block" is a 2-byte tag count, followed by that many sNetTag
// structures.
//
struct sNetBlock {
   ushort numTags;
   sNetTag tags[1];
};

int TagSetToNetBlock(const cTagSet *pTagSet, void **ppBlock)
{
#ifdef PLAYTEST
   cStr tagSetStr;
   pTagSet->ToString(&tagSetStr);
   ConfigSpew("net_tag_spew",
              ("SEND: encoding tagset %s\n", (const char *) tagSetStr));
#endif

   // Get some space to put the block into:
   int numTags = pTagSet->Size();
   AssertMsg((numTags < 65536), "Too many tags in cTagSet!");
   int size = 2 + (sizeof(sNetTag) * numTags);
   *ppBlock = malloc(size);

   // Record the number of tags:
   sNetBlock *pNetBlock = (sNetBlock *) *ppBlock;
   pNetBlock->numTags = numTags;

   // Now record all of the tags themselves:
   sNetTag *pNetTag = (sNetTag *) &(pNetBlock->tags[0]);
   int i;
   for (i = 0; i < numTags; i++) {
      const cTag *pTag = pTagSet->GetEntry(i);
      pNetTag->typeToken = (ushort) pTag->GetTypeToken();
      pNetTag->value = (ushort) pTag->GetValueToken();
      // Make sure that cutting it down to 2 bytes didn't truncate:
      AssertMsg((pNetTag->value == pTag->GetValueToken()),
                "Truncated value for networked tag!");
      pNetTag++;
   }

   return size;
}

void NetBlockToTagSet(void *pRawBlock, ObjID player, cTagSet *pTagSet)
{
   // First, find out how many tags we need to build:
   sNetBlock *pBlock = (sNetBlock *) pRawBlock;
   int numTags = pBlock->numTags;
   
   // Now, build them:
   sNetTag *pNetTag = &(pBlock->tags[0]);
   int i;
   for (i = 0; i < numTags; i++) {
      // What tag type is this really?
      sTagMapping mapKey = {player, pNetTag->typeToken};
      cTagMapRecord *pMapping = TagMapTable.Search(mapKey.toInt());
      if (!pMapping) {
         Warning(("Got enumeration for unknown tag type %d\n", 
                  pNetTag->typeToken));
         pNetTag++;
         continue;
      }
      cTagRecord *pRecord = cTag::GetTagRecord(pMapping->m_localRecordIndex);
      AssertMsg(pRecord, "Tag mapping without a local record!");
      
      if (pRecord->m_iFlags & kTagFlagEnum) {
         // Okay, it's an enumeration tag type. Get the local enumeration:
         AssertMsg(pMapping->m_Enums.IsValidIndex(pNetTag->value),
                   "Got enumeration tag that was never defined!");
         int localEnum = pMapping->m_Enums[pNetTag->value];
         // Finally, add this tag to the tagset:
         cTag Tag;
         Tag.Set(pRecord->m_iIndex, localEnum);
         pTagSet->Add(Tag);
      } else {
         // It's an integer tag, so we don't need to translate:
         cTag Tag;
         Tag.Set(pRecord->m_iIndex, pNetTag->value);
         pTagSet->Add(Tag);
      }

      // And on to the next...
      pNetTag++;
   }

#ifdef PLAYTEST
   cStr tagSetStr;
   pTagSet->ToString(&tagSetStr);
   ConfigSpew("net_tag_spew",
              ("RECEIVE: decoding tagset %s\n", (const char *) tagSetStr));
#endif
}

//////////
//
// Routines for sending and receiving the contents of a player's tags
// array.
//

// Send the entire tags array to the other players.
//
// This should be called just after we've connected to the other players,
// to initialize their view of our tags table. After that, we just send
// out the records as they get created, which gets pretty low-bandwidth
// pretty quickly.
//
// Once upon a time, we did this nice and cleanly by transmitting it all
// as separate records. Sadly, that proves to take way too much time. So
// we do it the hard way, bundling this up as a single big message...
//
static cNetMsg *g_pTagsArrayMsg = NULL;

// Wow, what a mess. Surely there must be a better way to do this...
void handleTagArray(int size, char *pTags, ObjID player)
{
   ConfigSpew("net_tag_spew",
              ("RECEIVE: tags array for player %d:\n", player));

   char *pEnd = pTags + size;
   char Name[64];
   // For each tag...
   while (pTags < pEnd) {
      // ... make an entry for that tag...
      BOOL isInt = (*pTags++ == 'i');
      int recordNum = *((ushort *) pTags);
      pTags += sizeof(ushort);
      char *pName = Name;
      while ((*pTags != ' ') && (*pTags != '\n')) {
         *pName++ = *pTags++;
      }
      *pName = '\0';
      handleTagRecord(recordNum, isInt, Name, player);
      ConfigSpew("net_tag_spew",
                 ("  %s tag #%d: %s\n",
                  isInt ? "int" : "enum",
                  recordNum,
                  Name));

      // ... and if it's an enumeration, deal with those.
      int enumNum = 0;
      while (*pTags != '\n') {
         // Skip the leading space
         pTags++;
         pName = Name;
         while ((*pTags != ' ') && (*pTags != '\n')) {
            *pName++ = *pTags++;
         }
         *pName = '\0';
         handleTagEnum(recordNum, enumNum++, Name, player);
         ConfigSpew("net_tag_spew",
                    ("    %s\n", Name));
      }

      // Finally, skip the closing newline:
      pTags++;
   }

   ConfigSpew("net_tag_spew", ("Finished tags.\n"));
}

static sNetMsgDesc sTagsArrayDesc =
{
   kNMF_MetagameBroadcast | kNMF_AppendSenderID,
   "TagArray",
   "Tag Array",
   NULL,
   handleTagArray,
   {{kNMPT_DynBlock, kNMPF_None, "Array"},
    {kNMPT_End}}
};

static void broadcastTagsArray()
{
   Assert_(g_bSynchronizing);

   ConfigSpew("net_tag_spew",
              ("SEND: Tags array:\n"));

   int numTags = cTag::GetNumTags();
   int i;
   cGrowingBlock Block;
   for (i = 0; i < numTags; i++) {
      cTagRecord *pRecord = cTag::GetTagRecord(i);
      if (pRecord->m_iFlags & kTagFlagSent)
         // We've already broadcast this one
         continue;

      // Record the record's type:
      if (pRecord->m_iFlags & kTagFlagInt) {
         Block.Append('i');
      } else {
         AssertMsg1(pRecord->m_iFlags & kTagFlagEnum,
                    "Tag record %s of unknown type!",
                    pRecord->m_TypeName);
         Block.Append('e');
      }

      // Record its index:
      ushort index = pRecord->m_iIndex;
      Block.Append(index & 0x00ff);
      Block.Append((index & 0xff00) >> 8);

      // Record its name:
      Block.Append(pRecord->m_TypeName);

      ConfigSpew("net_tag_spew",
                 ("  %s tag #%d: %s\n",
                  (pRecord->m_iFlags & kTagFlagInt) ? "int" : "enum",
                  pRecord->m_iIndex,
                  pRecord->m_TypeName));

      // If it's an enum, record its values:
      if (pRecord->m_iFlags & kTagFlagEnum) {
         int numEnums = pRecord->m_EnumValues.Size();
         int i;
         for (i = 0; i < numEnums; i++) {
            Block.Append(' ');
            Block.Append(pRecord->m_EnumValues[i]);
            ConfigSpew("net_tag_spew",
                       ("    %s\n", pRecord->m_EnumValues[i]));
         }
      }

      // Finally, terminate the tag entry...
      Block.Append('\n');

      // ... and record that we've dealt with it.
      pRecord->m_iFlags |= kTagFlagSent;
   }

   // Okay, the block should be ready -- send it on out...
   g_pTagsArrayMsg->Send(OBJ_NULL, Block.GetSize(), Block.GetBlock());

   ConfigSpew("net_tag_spew", ("Finished tags.\n"));
}

//
// Listen in for significant events in the networking system.
//
// Currently, we care about two major kinds of events:
// -- Entering or exiting network mode (in which we can generally send
//    tags)
// -- Being in synchronize mode (when we should send the full tags array.
//
static void networkListener(eNetListenMsgs situation,
                            DWORD /* data */,
                            void * /* pClientData */)
{
   switch (situation) {
      case kNetMsgNetworking:
         g_bNetworking = TRUE;
         break;
      case kNetMsgReset:
         g_bNetworking = FALSE;
         break;
      case kNetMsgSynchronize:
         g_bSynchronizing = TRUE;
         broadcastTagsArray();
         g_bSynchronizing = FALSE;
         break;
   }
}

//////////
//
// Startup & Shutdown
//
// We assume that these are only called if NEW_NETWORK_ENABLED is set.
//
void TagsNetInit()
{
   g_pNetManager = AppGetObj(INetManager);
   g_pNetManager->Listen(networkListener, 
                         (kNetMsgSynchronize | 
                          kNetMsgNetworking | 
                          kNetMsgReset), 
                         NULL);

   // Create the actual messages:
   g_pTagRecordMsg = new cNetMsg(&sTagRecordDesc);
   g_pTagEnumMsg = new cNetMsg(&sTagEnumDesc);
   g_pTagsArrayMsg = new cNetMsg(&sTagsArrayDesc);
}

void TagsNetTerm()
{
   delete g_pTagRecordMsg;
   delete g_pTagEnumMsg;
   delete g_pTagsArrayMsg;
   SafeRelease(g_pNetManager);
}

#endif // TAG_NETWORKING_ON
