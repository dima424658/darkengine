/* message.h
**
 * $Header: r:/prj/lib/src/dstruct/rcs/message.h 1.2 1993/10/07 10:34:52 rex Exp $
 * $Log: message.h $
 * Revision 1.2  1993/10/07  10:34:52  rex
 * Fixed MsgSprint() macro
 * 
 * Revision 1.1  1993/10/07  10:21:18  rex
 * Initial revision
 * 
 * 
 * The Message system provides asynchronous communication between various
 * "talkers", which may be game AI's, scripting languages such as BABL2,
 * game physics systems, etc.  Messages are "sent" from a source talker
 * to a destination talker.  Messages include an application-defined
 * message type, a timestamp (application must provide ptr to ulong timer
 * var), an application-defined priority (not used to sort messages),
 * and an information block (16 bytes long).  The message is placed
 * into a "mailbox system".  Later, the destination talker presumable
 * "gets" the message(s) in its mailbox, processing them as it sees fit.
 *
 * To initialize the message system:
 *
 *     MsgInit(maxMsgs);             // allocate msg list of desired size
 *     MsgSetTimestampPtr(&ptime);   // set ptr to game's ulong time
 *     MsgSetPrintFunc(&func);       // set custom message sprintf if desired
 *
 * To send a message:
 *
 *     Message *pmsg = MsgGrab();    // get a free message struct
 *     ... fill in fields ...        // all except 'next' & 'timeStamp'
 *     MsgSend(pmsg);                // place in dest talker's mailbox
 *
 * To receive message(s):
 *
 *    Message *pmsg;
 *    Talker me;
 *    me.type = ??; me.id = ??;      // which talker wants his/her messages?
 *    while (pmsg = MsgGet())
 *        {
 *        ... process message ...    // this can include sending messages
 *        MsgFree(pmsg);             // very important!
 *        }
 *
*/

#ifndef MESSAGE_H
#define MESSAGE_H

#ifndef LG_H
#include <lg.h>
#endif

//	Talkers are who send and receive messages.  They consist of a 16-bit
//	type and a 16-bit unique id within that type.  The low 128 talker
//	types are reserved, the rest are available for application use.
//	The Talker struct allows access to the 32-bit "talker ref" as a unit.

typedef ushort TalkerType;		// talker type (TALKER_XXX)

#define TALKER_NULL		0		// Who's talking? Not me!
#define TALKER_SPECIAL	1		// Usually for src, when it doesn't matter
#define TALKER_BABL		2		// BABL2 script, id is script id

#define TALKER_USER		128	// 128-65535 for application

typedef struct Talker_ {
	union {
		struct {
			TalkerType type;	// talker type (TALKER_XXX)
			ushort id;			// unique id within talker type
			};
		ulong tkref;			// combined "talker ref"
		};
} Talker;

//	Messages are structs which include all information about a message
//	including source and destination talkers, time message was sent,
//	message type (application-defined), message priority (application-defined),
//	and message info block.

typedef ushort MsgID;		// internal message system id
typedef uchar MsgType;		// message type, 0-255, application-defined

#define MESSAGE_SIZE 16		// size of info in Message, in bytes

typedef struct Message {
	MsgID next;					// next linked message id (do not touch!)
	MsgType type;				// code for message type 
   uchar priority;			// importance (application-defined)
	ulong timeStamp;			// time sent (filled in by MsgSend())
	Talker src;					// who's sending message 
	Talker dest;				// who's receiving it 
	union {						// extra info specific to id type 
		char charinfo[MESSAGE_SIZE];
		short shortinfo[MESSAGE_SIZE/2];
		long longinfo[MESSAGE_SIZE/4];
	} info;
} Message;		// hey, it's 32 bytes total, yea!

//	Some globals, defined in message.c

extern Message *mL;		// message table, public so msg<->id macros will work
extern ulong *pMsgTimestamp;	// ptr to time variable used for stamping
extern void (*f_MsgSprint)(char *buff, Message *m);	// ptr to msg sprinter

/////// HAVE SOME FUNCTIONS

// initialize the Message system, allocating list of desired size
void MsgInit (int maxMsgs);

//	shut down the Message system (deallocate msg table)
void MsgTerm (void);

// reserve a free Message (returns a pointer to it,
// or NULL if there are no free messages to be had) 
Message *MsgGrab (void);

// free msg
void MsgFree (Message *message);

// send msg.  Puts it in the message hash table. 
void MsgSend (Message *message);

// check for messages and return the first message,
// or NULL if there are none.  Marks message as received. 
Message *MsgGet (Talker *checker);

// throw away all messages for a given Talker
void MsgPunt (Talker *t);

// print out all the message data via Spew()
void MsgListPrint (void);

// print out a single message via Spew()
void MsgPrint (Message *m);

//////// HAVE SOME HANDY MACROS

//	install custom message sprinter function
#define MsgSetPrintFunc(f) (f_MsgSprint = (f))

//	call default/custom function to sprintf a message into a buffer
#define MsgSprint(str,m) (*f_MsgSprint)(str,m)

//	set ptr to timestamper var, otherwise all messages get 0 timestamp
#define MsgSetTimestampPtr(p) (pMsgTimestamp = (p))

// convert from MsgID to Message *
#define MsgIDToMessage(msg) (&mL[(msg)])

// convert from Message * to MsgID
#define MessageToMsgID(message) ((message)-mL)


#endif // MESSAGE_H 

