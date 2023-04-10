/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkminig.h,v 1.12 2000/01/31 09:58:01 adurant Exp $
#pragma once

#ifndef __SHKMINIG_H
#define __SHKMINIG_H

typedef enum eMFDGame { kGameIndex,
   kGameSlots, kGamePing, kGameSwine, kGameOverWorld, kGameKaBacon, kGameAbyss,
   kGameHogger, kGameTicTacToe, kGameRacer, kGameTetris, kDonkeyHog, kGameGolf,
   kGameMax, kGameNone = -1};

EXTERN void SetGame(eMFDGame which);

EXTERN BOOL ShockMiniGameIsHacked(void);

////////////////
// high score "system"
#define MAX_HIGH_SCORES       (10)
#define HIGH_SCORE_MSG_LEN    (128)
#define HIGH_SCORE_MAX_LINES  (18)
struct sMiniGameSlot {
   int  pts;
   char msg[HIGH_SCORE_MSG_LEN];
};

struct sMiniGameScores {
   sMiniGameSlot scores[MAX_HIGH_SCORES];
};

EXTERN void ShockMiniGameAddHighScore(sMiniGameScores *pScores, int score, char *buf);
EXTERN void ShockMiniGameDisplayHighScores(sMiniGameScores *pScores, int x, int y);

EXTERN char gMiniGameScoreMsg[HIGH_SCORE_MSG_LEN];

////////////////
// this is for canvas blit setup
EXTERN BOOL ShockMiniGameSetupCanvas(Point pt, Rect base, int clear_color);
EXTERN void ShockMiniGameBlitCanvas(Point pt, Rect base);

// and this, of course, is just silly   
EXTERN void ShockMiniGameScreenSaver(Point pt, Rect base);

#endif
