/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/dpshell.h,v 1.4 2000/01/29 13:20:44 adurant Exp $
#pragma once

#ifndef __DPSHELL_H
#define __DPSHELL_H

// main function protos
int dPlayInit(void);
int dPlayJoin(bool newGame, char *whome, void far *game_ref); // really LPGUID
int dPlayClose(void);
int dPlaySend(void *ptr, int len, int target);
int dPlayRead(void *ptr, ulong *max_len, int *from_who);

// Silly stuff for guaranteed packet fun?
void dPlaySetSendType(bool safe);

// this stuff only exists if latency emulation is on
bool dPlaySetupLatencyBuffers(int size, int ntimein, int ntimeout);
bool dPlaySpikeEmulate(bool onoff, int spike_freq, int spike_time, int dropin, int dropout);
bool dPlayCheckLatent(void);

// this stuff only exists if stat gathering is on
void dPlayStatsMode(bool on);
bool dPlayStatsDump(char *fname);

ulong dPlayGetSelf(void);

// System packets and system info
typedef struct {
   int   netID;
   int   netOP;
   char  name[64];         // should do this for real
   int   netType;
} netSysPacketInfo;

int dPlayParseSys(void *ptr, netSysPacketInfo *nspi);
typedef bool (*ParseSysCallback)(netSysPacketInfo *);

bool dPlayParseCurrentSystem(ParseSysCallback readinf);

#define netOpAdd      0
#define netOpRem      1
#define netOpFailure  2    // we just lost it all, go home

#define netTypePlayer 0
#define netTypeGroup  1

// packets from the mahster
#define netSystemMsg  0

// types of service - total hack for now
bool dPlayServiceSelect(int servID);
#define netServTCP    0
#define netServIPX    1
#define netServModem  2

// returned by PlayRead
#define netGotMsg            1

// generic returns
#define netOK               0
#define netErr             -1
#define netNoNet           -2
#define netNoMem           -3

// returned by PlayInit
#define netNoService       -10
#define netNoUsableService -11
#define netCannotCreate    -12

// returned by PlayJoin
#define netNoGame          -20
#define netCantOpen        -21
#define netCantPlayer      -22
#define netNoSess          -23

// returned by PlaySend
#define netNoSelf          -30
#define netBusy            -31
#define netNoPlayer        -32

// returned by PlayRead
#define netNoMsgs          -40
#define netNoSpace         -41

// really from the Latency code
#define netBufferFull      -50

#endif // __DPSHELL_H
