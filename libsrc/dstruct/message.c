/* message.c
**
** Message passing routines
**
** $Header: r:/prj/lib/src/dstruct/rcs/message.c 1.3 1993/11/04 11:22:09 rex Exp $
** $Log: message.c $
 * Revision 1.3  1993/11/04  11:22:09  rex
 * Changed sprintf() to lg_sprintf()
 * 
 * Revision 1.2  1993/10/15  15:01:59  rex
 * Added check for (mL == NULL) in MsgPunt(), so BABL2 can call MsgPunt()
 * for a thread and not worry if message system is in use or not
 * 
 * Revision 1.1  1993/10/07  10:21:15  rex
 * Initial revision
 * 
 * 
*/

#include <string.h>
#include <stdlib.h>

#include <message.h>
#include <lgsprntf.h>
#include <_dstruct.h>

#define MESSAGE_HASH_SIZE 16	// Number of hash chains to messages

// An entry in the message hash table, hashed by the Talker recipient
typedef struct MessageHashEntry {
	MsgID head;		// 1st message in this hash entry (0 if empty)
	MsgID tail;		// last message in this hash entry (0 if empty)
} MessageHashEntry;

// The hash table.  Various elements keep track of where we've looked recently,
// to make performing many searches for one recipient fast 
typedef struct MessageHashTable {
	MessageHashEntry entries[MESSAGE_HASH_SIZE];
	Talker curtalker;				// current Talker who's checking
	MessageHashEntry *pecurr;	// ptr to current hash entry chain
	MsgID currMsg;					// id of current msg looking at
	MsgID prevMsg;					// id of message before curr (for link patch)
} MessageHashTable;

// The message list, which is Malloc()'ed in MsgInit()
Message *mL;
static int totMsgs;		// total # messages in list

// The hash table for finding messages by recipient 
static MessageHashTable mHT;

//	Ptr to timestamp, points at 0 till set
static ulong zeroStamp = 0;
ulong *pMsgTimestamp = &zeroStamp;

//	Ptr to function to sprint message
void MsgSprintDefault(char *buff, Message *m);
void (*f_MsgSprint)(char *buff, Message *m) = MsgSprintDefault;

// t is a Talker
#define HASH(t) ((((t).type * 7) + ((t).id * 3)) & 0xf)

//	Get next message ptr given current msg ptr

//	Debug & Spew macros
#define DBG_Msg(x) DBG(DSRC_DSTRUCT_Msg,x)
#define SpewMsg(x) Spew(DSRC_DSTRUCT_Msg,x)

//////////////////////////////
//
// Initializes the Message system

void MsgInit (int maxMsgs)
{                                      
	int i;

	if (mL == NULL)
		{
		// Allocate the message list (actually full at totMsgs - 1 messages)
		totMsgs = maxMsgs;
		mL = Calloc(totMsgs * sizeof(Message));

		// Initialize the free message chain
		// mL[totMsgs - 1].next left equal to 0
		for (i = 0; i < (totMsgs - 1); ++i)
			mL[i].next = i + 1;

		// Initialize the hash table 
		memset (&mHT, 0, sizeof (mHT));

		// Make sure we deallocate this, I suppose
		AtExit(MsgTerm);
		}
}

//////////////////////////////
//
//	Terminates the Message system, freeing up message list memory.

void MsgTerm (void)
{
	if (mL)
		{
		Free(mL);
		mL = NULL;
		}
}

//////////////////////////////
//
// Reserves a free Message and returns a pointer to it, or NULL if there are no free
// messages to be had.

Message *MsgGrab (void)
{
	MsgID msg;

// ml[0].next is always the first free message
// ml[0] itself is not used for anything else

	msg = mL[0].next;

	if (msg == 0)
		{
		Warning (("Out of message space in MsgGrab\n"));
		return NULL;
		}
	else
		{
		// Take it out of the free message chain, init default priority
		mL[0].next = mL[msg].next;
      mL[msg].priority = 0;
		return &(mL[msg]);
		}
}

//////////////////////////////
//
// Frees the space used by msg for further use

void MsgFree (Message *message)
{
// Put it back into the free message chain 

	message->next = mL[0].next;
	mL[0].next = MessageToMsgID(message);
}

//////////////////////////////
//
// Sends message.  Puts it in the message hash table.
// Returns whether it was successful.

void MsgSend (Message *message)
{
	MessageHashEntry *pentry;	// ptr to hash entry
	MsgID msg;						// id of this message

//	Timestamp the message

	message->timeStamp = *pMsgTimestamp;

// Find what hash entry it goes in

	pentry = &mHT.entries[HASH(message->dest)];

//	Maybe log message

	DBG_Msg ({ SpewMsg (("Send ")); MsgPrint (message); })

// If we're sending a message to the Talker currently checking
// for messages, invalidate curtalker to be sure he gets it

	if (message->dest.tkref == mHT.curtalker.tkref)
		mHT.curtalker.type = TALKER_NULL;

// Add to tail of this hash entry, set our next field to 0

	msg = MessageToMsgID(message);
	if (pentry->tail)
		MsgIDToMessage(pentry->tail)->next = msg;
	else
		pentry->head = msg;
	pentry->tail = msg;
	message->next = 0;
}

//////////////////////////////
//
// Checks for messages and returns the first message, or NULL
// if there are none.  Marks message as received.
//
// Notes: It would be a real pain to do getting and deleting messages
//   as two separate operations, since we'd have to go searching for
//   the message in the hash table again (and it could have moved, if
//   other Talkers had accessed the hash table in the meantime).  So
//   we delete the message from the hash table.  If you really don't
//   want to delete it, it is pretty trivial to reinsert it.

Message *MsgGet (Talker *checker)
{
	Message *pmsg;								// message ptr we're returning

//	If new talker, start at top of hash entry chain

	if (checker->tkref != mHT.curtalker.tkref)
		{
		mHT.curtalker = *checker;
		mHT.pecurr = &mHT.entries[HASH(*checker)];
		mHT.currMsg = mHT.pecurr->head;
		mHT.prevMsg = 0;
		}

//	Look for matching talker, while advance curr (and prev) msg ids
//	until curr hits 0 or a match is found.

	while (mHT.currMsg)
		{

//	Get ptr to current message
//	Check if current message's talker exactly matches who we're looking for

		pmsg = MsgIDToMessage(mHT.currMsg);
		if (pmsg->dest.tkref == mHT.curtalker.tkref)
			{

//	Hey, we found a match!

//	If we're not at head of hash chain, unlink us from middle
//	If the entry chain tail was pointing at us, then set it to prev
//	since we're pulling out of this chain

			if (mHT.prevMsg)
				{
				MsgIDToMessage(mHT.prevMsg)->next = pmsg->next;
				if (mHT.pecurr->tail == mHT.currMsg)
					mHT.pecurr->tail = mHT.prevMsg;
				}

//	Else if we are at head of hash chain, just modify entry head/tail
//	Set head to next message, if that's 0 then zero tail too.

			else
				{
				mHT.pecurr->head = pmsg->next;
				if (mHT.pecurr->head == 0)
					mHT.pecurr->tail = 0;
				}

//	Advance current id to after us, leave prev untouched

			mHT.currMsg = pmsg->next;

//	Maybe log message, and return it

			DBG_Msg ({SpewMsg (("Rec ")); MsgPrint (pmsg); })
			return pmsg;
			}

//	Hey! We didn't find a match, then just advance prev & curr

		mHT.prevMsg = mHT.currMsg;
		mHT.currMsg = pmsg->next;
		}

//	Looked thru entire hash chain, no messages for this talker

	return NULL;
}

//////////////////////////////
//
// Throws away all messages for a given Talker.  Useful if, for example,
// an AI dies, or we're shutting down the object simulator.

void MsgPunt (Talker *t)
{
   Message *msg;

	if (mL == NULL)	// safety so BABL2 can punt when kill thread
		return;			// without worrying about whether MsgInit() called

   while ((msg = MsgGet (t)) != NULL) MsgFree (msg);
}

//////////////////////////////
//
// Print out all the message data

void MsgListPrint (void)
{
#ifdef SPEW_ON
	int i;
	MsgID id;
   char str[80];
#endif

	SpewMsg (("===\n"));

DBGS (DSRC_DSTRUCT_MsgList, {
   for (i = 0; i < MESSAGE_HASH_SIZE; ++i)
		{
		id = mHT.entries[i].head;
		while (id)
			{
			MsgSprint (str, MsgIDToMessage(id));
			SpewMsg (("%s", str));
			id = mL[id].next;
			}
   	}
	SpewMsg (("---\n"));
})

DBGS (DSRC_DSTRUCT_MsgTable, {
	for (i = 0; i < MESSAGE_HASH_SIZE; ++i)
		{
		id = mHT.entries[i].head;
		while (id)
			{
			SpewMsg (("%d:", i));
			SpewMsg ((" %d", id));
			SpewMsg (("\n"));
			id = mL[id].next;
			}
		}
	SpewMsg (("===\n"));
})

}

///////////////////////////////////
//
//	Print out single message

void MsgPrint (Message *m)
{
	char str[80];

	if (f_MsgSprint)
		{
		MsgSprint(str, m);
		SpewMsg ((str));
		}
}

////////////////////////////////////
//
//	Default message sprinter, prints message info as 8 words.

void MsgSprintDefault(char *buff, Message *m)
{
	lg_sprintf (buff, "[%d] %d.%d -> %d.%d: [%x %x %x %x %x %x %x %x]\n",
		MessageToMsgID(m),
		m->src.type, m->src.id,
		m->dest.type, m->dest.id,
		m->info.shortinfo[0],
		m->info.shortinfo[1],
		m->info.shortinfo[2],
		m->info.shortinfo[3],
		m->info.shortinfo[4],
		m->info.shortinfo[5],
		m->info.shortinfo[6],
		m->info.shortinfo[7]
		);
}