/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/player.h,v 1.2 2000/01/31 10:00:15 adurant Exp $
// app player stuff
#pragma once

#ifndef __PLAYER_H
#define __PLAYER_H

#ifdef NETWORK_ENABLED

#define PLAYER_NAME_LEN 44

typedef struct {
   int  netPID;   // if we were clever, this would have an inactive code
   char name[PLAYER_NAME_LEN]; // so it is 64 total
   int  team;     // from config
   int  stats1;   // probably "frags"
   int  stats2;   // probably something else
   bool active;   // is this rec active
   bool pad[3];   // power o 2
} player_rec;

#define MAX_PLAYERS 64
extern player_rec ourGame[];

#define NO_PLAYER (-1)

int playerAddRec(int PID, char *name);
int playerFindPID(int PID);
bool playerDelRec(int PID);

#ifdef DBG_ON
#define playerNetName(PID) \
   (playerFindPID(PID)==NO_PLAYER?"Unknown":ourGame[playerFindPID(PID)].name)
#define playerNetTeam(PID) \
   (playerFindPID(PID)==NO_PLAYER?0:ourGame[playerFindPID(PID)].team)
#define playerSetTeam(PID,newteam) \
   playerFindPID(PID)==NO_PLAYER?0:ourGame[playerFindPID(PID)].team=newteam
#else  // DBG_ON
#define playerNetName(PID) (ourGame[playerFindPID(PID)].name)
#define playerNetTeam(PID) (ourGame[playerFindPID(PID)].team)
#define playerSetTeam(PID,newteam) \
   ourGame[playerFindPID(PID)].team=newteam
#endif // DBG_ON

#else  // NETWORK_ENABLED

#define playerNetName(PID) "You"
#define playerNetTeam(PID) 0
#define playerSetTeam(PID,newteam)

#endif // NETWORK_ENABLED

#endif // __PLAYER_H
