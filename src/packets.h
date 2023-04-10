/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/packets.h,v 1.9 2000/01/29 12:41:40 adurant Exp $
#pragma once

#ifndef __PACKETS_H
#define __PACKETS_H

#include <globalid.h> // @TODO: get rid of this

typedef struct packetHeader {
   char type;
   char pad[3];
} packetHeader;

// for now, since i suck, every new packet type goes here
// lame, isnt it
#define PKT_LOGIN     0
#define PKT_CREATE    1
#define PKT_PHYSICS   2
#define PKT_STATE     3
#define PKT_COLLIDE   4
#define PKT_DELETE    5
#define PKT_HANDOFF   6
#define PKT_REPORT    7
#define PKT_TALK      8

// actual prototypes
EXTERN int packetSend(uchar *buffer, ulong len);

// sendtarget stuff
EXTERN int packetSendTarget;
#define packetGetTarget()  (packetSendTarget)
#define packetSetTarget(x) (packetSendTarget=x)

#define pkSendBroadcast    (0)
#define pkSendGroup       (-1)

// currentfrom stuff
EXTERN int packetCurrentFrom;

// are we onthe network.  why is it here? who knows
EXTERN bool netWork;

#endif // __PACKETS_H

