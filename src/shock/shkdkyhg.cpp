/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkdkyhg.cpp,v 1.7 2000/02/19 13:20:12 toml Exp $

// 
// Donkey Hog
//

//
// Donkey Hog, the evil pig, has captured the lovely Princess Jennifer and taken her
// to the top of a construction site. Her handsome and daring boyfriend, the plubmer Timio, 
// must rescue her. Donkey Hog, to thwart the dashing Timio, rolls hams down on him from above.
//
// This is a completely original idea. Really. No kidding.
//
// "It'sa me! Timio!"
//

#include <r3d.h>
#include <mouse.h>
#include <dev2d.h>
#include <mxmats.h>
#include <timer.h>
#include <random.h>
 
#include <palmgr.h>
#include <shkdkyhg.h>

#include <rect.h>
#include <mxangs.h>

#include <shkutils.h>
#include <string.h>
#include <dev2d.h>
#include <shkminig.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
// globals
sDonkeyHog *g_pGame;
bool gUpdateFrames = TRUE;


//
// Board structure
//
//
// The board information is encoded with each grid position providing information
// on how the board affects movement.
//
// The values are bitmasks
//
//
// Value - Description -               Effect on movement
//
//    0     Open                       No player movement, hams fall
//    1     Right Slope                Barrel rolls right
//    2     Left Slope                 Barrel rolls left
//    4     Ladder                     Only up/down movement allowed
//    8     Top of ladder              left/right/down movement allowed
//    16    Bottom of ladder           left/right/up movement allowed
//    32    Winning zone               none
//    64    Ham kill zone              kills hams

#define kBoardOpen         0
#define kBoardRightSlope   1
#define kBoardLeftSlope    2
#define kBoardLadder       4
#define kBoardTopLadder    8
#define kBoardBottomLadder 16
#define kBoardWinningZone  32
#define kBoardKillHam      64

// resolution of board grid: in pixels
#define DH_GRID_RES_X  8        
#define DH_GRID_RES_Y  4

// 17 x 68
#define DH_BOARD_SIZE_COL  (140 / DH_GRID_RES_X)
#define DH_BOARD_SIZE_ROW  (276 / DH_GRID_RES_Y)


Point gBoard1Pos = {1,66};

char kBoard1[DH_BOARD_SIZE_ROW][DH_BOARD_SIZE_COL] = {
/* 0  */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 1  */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 2  */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 3  */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 4  */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 5  */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 6  */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 7  */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 8  */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 9  */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 10 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 11 */   1,   1,   1,   1,  33,  33,   1,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 12 */   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   9,   1,   1,   0,   0,
/* 13 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 14 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 15 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 16 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 17 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 18 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 19 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,  18,   2,   2,   2,   2,
/* 20 */   0,   0,   0,   0,   0,   0,   2,   2,   2,  10,   2,   0,   0,   0,   0,   0,   0,
/* 21 */   0,   0,   0,   2,  10,   2,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,
/* 22 */   0,   0,   2,   0,   4,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,
/* 23 */   0,   0,   0,   0,   4,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,
/* 24 */   0,   0,   0,   0,   4,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,
/* 25 */   0,   0,   0,   0,   4,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,
/* 26 */   0,   0,   0,   0,   4,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,
/* 27 */   0,   0,   0,   0,   4,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,
/* 28 */   0,   0,   0,   0,   4,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,
/* 29 */   0,   0,   0,   0,   4,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,
/* 30 */   1,   1,   0,   0,   4,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,
/* 31 */   0,   0,   1,   1,  17,   1,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,
/* 32 */   0,   0,   0,   0,   0,   0,   1,   1,   1,  17,   1,   1,   9,   1,   0,   0,   0,
/* 33 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   1,   1,
/* 34 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 35 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 36 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 37 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 38 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 39 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 40 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 41 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,  18,   2,   2,   2,   2,
/* 42 */   0,   0,   0,   0,   0,   2,   2,   2,   2,   2,   2,   0,   0,   0,   0,   0,   0,
/* 43 */   0,   0,   2,  10,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 44 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 45 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 46 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 47 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 48 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 49 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 50 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 51 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 52 */   1,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 53 */   0,   1,   1,   17,  1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 54 */   0,   0,   0,   0,   0,   1,   1,   1,   1,   0,   0,   0,   0,   0,   0,   0,   0,
/* 55 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   9,   1,   1,   0,   0,
/* 56 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 57 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 58 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 59 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 60 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 61 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 62 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 63 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,
/* 64 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  18,   2,   2,   2,   2,
/* 65 */   0,   0,   0,   0,   0,   0,   0,   2,   2,   2,   2,   2,   0,   0,   0,   0,   0,
/* 66 */   0,   0,   0,   2,   2,   2,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 67 */  66,  66,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0};





Point gBoard2Pos = {16,66};


char kBoard2[DH_BOARD_SIZE_ROW][DH_BOARD_SIZE_COL] = {
/* 0  */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 1  */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 2  */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 3  */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 4  */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 5  */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 6  */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 7  */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 8  */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 9  */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 10 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 11 */   1,   1,   1,   1,  33,  33,   1,   9,   1,   0,   0,   0,   0,   0,   0,   0,   0,
/* 12 */   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 13 */   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 14 */   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 15 */   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 16 */   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 17 */   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 18 */   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 19 */   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 20 */   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 21 */   0,   0,   0,   0,   0,   0,   1,  17,   1,   1,   1,   1,   0,   0,   0,   0,   0,
/* 22 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   9,   1,   0,   0,
/* 23 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,
/* 24 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,
/* 25 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,
/* 26 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,
/* 27 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,
/* 28 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   2,   2,
/* 29 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   2,  18,   2,   0,   0,
/* 30 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,
/* 31 */   0,   0,   0,   0,   0,   0,   0,   2,   2,   2,   0,   0,   0,   0,   0,   0,   0,
/* 32 */   0,   0,   0,   0,   2,   2,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 33 */   0,   0,   2,  10,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 34 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 35 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 36 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 37 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 38 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 39 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 40 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 41 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 42 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 43 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 44 */   1,   1,   1,  17,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 45 */   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 46 */   0,   0,   0,   0,   0,   0,   1,   1,   1,   9,   1,   0,   0,   0,   0,   0,   0,
/* 47 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   1,   0,   0,   0,   0,   0,
/* 48 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   1,   9,   1,   0,   0,
/* 49 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   4,   0,   0,   0,
/* 50 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   4,   0,   0,   0,
/* 51 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   4,   0,   0,   0,
/* 52 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   4,   0,   0,   0,
/* 53 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   2,  18,   2,   2,   2,
/* 54 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   2,   0,   0,   0,   0,   0,
/* 55 */   0,   0,   0,   0,   0,   2,   2,   2,   2,  18,   2,   0,   0,   0,   0,   0,   0,
/* 56 */   0,   0,   2,  10,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 57 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 58 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 59 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 60 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 61 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 62 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 63 */   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 64 */   1,   1,   1,  17,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 65 */   0,   0,   0,   0,   0,   1,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/* 66 */   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   0,   0,   0,   0,
/* 67 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,  65,  65};






struct sBoard
{
   
   char *m_pData;

   int m_xSize;
   int m_ySize;

   IRes *m_pGirder;
   IRes *m_pTopLadder;
   IRes *m_pBottomLadder;
   IRes *m_pLadder;


   sBoard(void)
   {
      m_xSize = DH_BOARD_SIZE_COL;
      m_ySize = DH_BOARD_SIZE_ROW;
      
      m_pGirder = LoadPCX("dhgird");
      m_pTopLadder = LoadPCX("dhladtop");
      m_pBottomLadder = LoadPCX("dhladbot");
      m_pLadder = LoadPCX("dhladder");

   }

   ~sBoard(void)
   {
      SafeFreeHnd(&m_pGirder);
      SafeFreeHnd(&m_pTopLadder);
      SafeFreeHnd(&m_pBottomLadder);
      SafeFreeHnd(&m_pLadder);
   }

   // x and y are in board coords, NOT screen coords
   char GetData(int x, int y)
   {
      // row = y col = x
      return(  *(m_pData+(y*m_xSize + x))  );
   }

   void Set(char *pData)
   {
      m_pData = pData;
   }

   void Draw(void)
   {
      for (int row = 0; row < m_ySize; row++)
      {
         for (int col = 0; col < m_xSize; col++)
         {
            int data = *(m_pData+(row*m_xSize + col));

            if (!data) continue;

            Point loc;
            loc.x = col*DH_GRID_RES_X;
            loc.y = row*DH_GRID_RES_Y;

            if (data & kBoardLadder)
            {
               DrawByHandle(m_pLadder, loc);
               continue;
            }

            if (data & kBoardTopLadder)
            {
               DrawByHandle(m_pTopLadder, loc);
               continue;
            }

            if (data & kBoardBottomLadder)
            {
               DrawByHandle(m_pBottomLadder, loc);
               continue;
            }

            if ( (data & kBoardRightSlope) || (data & kBoardLeftSlope) )
            {
               DrawByHandle(m_pGirder, loc);
            }
         }
      }
   }


};


//
// sHam : the hams that DH throws
//
#define DH_HAM_X_VEL_DEFAULT  3
#define DH_HAM_Y_VEL_DEFAULT  3

#define DH_HAM_LADDER_CHANCE 0.15
#define DH_HAM_XSIZE 8
#define DH_HAM_YSIZE 8

#define DH_HAM_NUM_FRAMES 4

#define DH_HAM_DRAW_TICKS 
struct sHam {

   int m_xVel; // in x direction. + means to right
   int m_yVel; // in y direction, - means down

   int m_xSize;
   int m_ySize;

   float m_fallDownLadderChance;

   Point m_pos;
   int m_curFrame;
   bool m_active;
   bool m_checkJump;
   bool m_side;
   bool m_switched;



   int m_tickCount;

   IRes *m_pHamFrames[DH_HAM_NUM_FRAMES];

   sHam(void)
   {
      m_xVel = DH_HAM_X_VEL_DEFAULT;
      m_yVel = DH_HAM_Y_VEL_DEFAULT;
      m_fallDownLadderChance = DH_HAM_LADDER_CHANCE;

      m_xSize = DH_HAM_XSIZE;
      m_ySize = DH_HAM_YSIZE;

      m_active = FALSE;
      m_checkJump = FALSE;
      m_side = FALSE;
      m_switched = FALSE;

      m_pHamFrames[0] = LoadPCX("dhham0.pcx");
      m_pHamFrames[1] = LoadPCX("dhham1.pcx");
      m_pHamFrames[2] = LoadPCX("dhham2.pcx");
      m_pHamFrames[3] = LoadPCX("dhham3.pcx");


      m_curFrame = 0;
      m_tickCount = 0;
   }

   ~sHam() 
   {
      for(int i=0;i<DH_HAM_NUM_FRAMES;++i) SafeFreeHnd(&m_pHamFrames[i]);
   }


   void Place(int x, int y)
   {
      m_pos.x = x;
      m_pos.y = y;
      m_active = TRUE;
   }

   void Kill(void)
   {
      m_active = FALSE;
   }

   void Draw(void)
   {

      DrawByHandle(m_pHamFrames[m_curFrame], m_pos);

      if (gUpdateFrames == FALSE) return;

      // figure out the next frame according to x velocity

      if (m_xVel > 0)
      {
         // going to the right
         m_curFrame++;
         if (m_curFrame >= DH_HAM_NUM_FRAMES)
         {
            m_curFrame = 0;
         }
      }
      else
      {
          // going to the left
         m_curFrame--;
         if (m_curFrame < 0)
         {
            m_curFrame = DH_HAM_NUM_FRAMES - 1 ;
         }
      }
   }

   // move according to info provided by board
   void Move(void)
   {
      char currBoardPos; // flags
      currBoardPos = g_pGame->m_pBoard->GetData(m_pos.x/DH_GRID_RES_X, (m_pos.y+m_ySize+1)/DH_GRID_RES_Y);
       
      if (currBoardPos & kBoardKillHam)
      {
         // kill myself
         m_active = FALSE;
         return;
      }

      if (currBoardPos == kBoardOpen)
      {
         // fall down

         // based on the current xVel, choose the corner which determines when to fall
         // if moving right, use the ll corner, but if moving left, use the lr corner
         if (m_xVel > 0)
         {
            m_pos.y += m_yVel;
         }
         else
         {
            // re-get the data, based on the lr corner
            currBoardPos = g_pGame->m_pBoard->GetData((m_pos.x + m_xSize)/DH_GRID_RES_X, (m_pos.y+m_ySize+1)/DH_GRID_RES_Y);
            if (currBoardPos == kBoardOpen)
            {
               m_pos.y += m_yVel;
            }
            else
            {
               // continue along current direction
               m_pos.x += m_xVel;
            }
         }
      }
      else
      {
         if (currBoardPos & kBoardTopLadder)
         {
            if (RandFloat() < m_fallDownLadderChance)
            {
               // fall down the ladder
               m_pos.y += m_yVel;
               return;
            }
         }

         if (currBoardPos & kBoardRightSlope)
         {
            // if we were moving left, now move right
            if (m_xVel < 0)
            {
               m_xVel *= -1;
            }
            m_pos.x += m_xVel;
         }
         else
         {
            if (currBoardPos & kBoardLeftSlope)
            {
               // if we were moving right, now move right
               if (m_xVel > 0)
               {
                  m_xVel *= -1;
               }
               m_pos.x += m_xVel;
            }
            else
            {
               if (currBoardPos  & kBoardLadder)
               {
                  // hams can only fall down ladders
                  m_pos.y += m_yVel;
               }
            }
         }
      }

      KeepOnBoard();
   }

   void Update(void)
   {
      Draw();
      if (gUpdateFrames == FALSE) return;
      Move();
   }

   bool CheckCollision(sTimio *pPlayer)
   {
      Point playerLR;
      Point playerUL;

      playerLR.x = pPlayer->m_pos.x + pPlayer->m_xSize;
      playerLR.y = pPlayer->m_pos.y + pPlayer->m_ySize;

      playerUL.x = pPlayer->m_pos.x;
      playerUL.y = pPlayer->m_pos.y;

      // hackly
      if (pPlayer->m_jumping)
      {
         // his jumping art has 3 black pixels on the bottom. Make it look right
         playerLR.y -= DH_TIMIO_BLANK_JMP_PXLS;
         playerUL.y -= DH_TIMIO_BLANK_JMP_PXLS;
      }
      else
      {
         if ((pPlayer->m_xVel == 0) && (pPlayer->m_yVel == 0) )
         {
            // if he's standing still, compensate for the blank pixels in the art
            playerUL.x += DH_TIMIO_BLANK_PIXELS;
            playerLR.x -= DH_TIMIO_BLANK_PIXELS;
         }
      }


      Point myLR;
      myLR.x = m_pos.x + m_xSize;
      myLR.y = m_pos.y + m_ySize;

      if (m_pos.x > playerLR.x)
      {
         return FALSE;
      }
      if (myLR.x < playerUL.x)
      {
         return(FALSE);
      }
      if (m_pos.y > playerLR.y)
      {
         return(FALSE);
      }
      if (myLR.y < playerUL.y)
      {
         return(FALSE);
      }
      return(TRUE);
   }


   void KeepOnBoard()
   {
      // make sure you don't go off the board
      if (m_pos.x < 0)
      {
         m_pos.x = 0;
      }

      if (m_pos.x >= (140 - m_xSize)) 
      {
         m_pos.x = (139 - m_xSize);
      }
   }

};


//  a buffer of hams
#define DH_NUM_HAMS 32

// y value we use to determine if we have just jumped a ham
#define DH_HAM_JUMP_Y 10

// since it's possible for some other factor to deactivate a ham, just loop through them
// all each time
struct sHamBuffer 
{
   sHam m_buffer[DH_NUM_HAMS];

   int m_currHamSpeed;

   sHamBuffer()
   {
      m_currHamSpeed =  DH_HAM_X_VEL_DEFAULT;
   }

   // place a ham at (x,y)
   void CreateHam(int x, int y)
   {
      for (int i = 0; i < DH_NUM_HAMS; i++)
      {
         if (m_buffer[i].m_active == FALSE)
         {
            m_buffer[i].Place(x,y);
            break;
         }
      }
   }

   // update all active hams
   void Update(void)
   {
      for (int i = 0; i < DH_NUM_HAMS; i++)
      {
         if (m_buffer[i].m_active == TRUE)
         {
            m_buffer[i].Update();
         }
      }
   }

   
   // set a flag on the hams, so we know to check them against jumping
   void FlagHams()
   {
      for (int i = 0; i < DH_NUM_HAMS; i++)
      {
         if (m_buffer[i].m_active == TRUE)
         {
            m_buffer[i].m_checkJump = TRUE;
            m_buffer[i].m_side = 0;
            m_buffer[i].m_switched = FALSE;
         }
      }
   }


   // return a tally of all jumped hams
   // 'pos' should be foot of jumper
   int GetScore(Point pos)
   {
      int score = 0;
      // loop through the hams. Check every ham which has m_switched set. if it is
      // within a certain y of the player, consider it jumped
      for (int i = 0; i < DH_NUM_HAMS; i++)
      {
         sHam *pHam;

         pHam = &m_buffer[i];

         if ( (pHam->m_active == TRUE) && (pHam->m_switched == TRUE) )
         {
            if (abs((pHam->m_pos.y + pHam->m_ySize) - pos.y) < DH_HAM_JUMP_Y)
            {
               score += 1;
            }
         }
      }
      return score;
   }

   void UnFlagHams()
   {
      for (int i = 0; i < DH_NUM_HAMS; i++)
      {
         if (m_buffer[i].m_active == TRUE)
         {
            m_buffer[i].m_checkJump = FALSE;
         }
      }
   }

   // loop through all of the hams and see if any of them are being jumped
   void CheckHams(Point pos)
   {
      sHam *pHam;

      for (int i = 0; i < DH_NUM_HAMS; i++)
      {
         pHam = &m_buffer[i];
         if ( (pHam->m_active == TRUE) && (pHam->m_switched == FALSE) )
         {
            // on what side of 'pos' is the ham?
            if (pHam->m_pos.x < pos.x)
            {
               if (pHam->m_side == 1)
               {
                  // the player has passed the ham. 
                  pHam->m_switched = TRUE;
               }
               else
               {
                  // he's on my left side
                  pHam->m_side = -1;
               }
            }
            else
            {
               if (pHam->m_pos.x >= pos.x)
               {
                  if (pHam->m_side == -1)
                  {
                     // the player has passed the ham
                     pHam->m_switched = TRUE;
                  }
                  else
                  {
                     // he's on my right side
                     pHam->m_side = 1;
                  }
               }
            }
         }
      }
   }


  void Clear()
  {
      for (int i = 0; i < DH_NUM_HAMS; i++)
      {
         m_buffer[i].m_active = FALSE;
      }
  }

  void IncBarrelSpeed()
  {
     m_currHamSpeed++;
      for (int i = 0; i < DH_NUM_HAMS; i++)
      {
         // if hams go to fast, they will start flying off
         // the side, or falling through girders
         if (m_currHamSpeed < 6)
         {
            // they look stupid faster than this
            m_buffer[i].m_xVel = m_currHamSpeed;
         }
         else
         {
            m_buffer[i].m_xVel = 5;
         }

         if (m_currHamSpeed < 5)
         {
            m_buffer[i].m_yVel = m_currHamSpeed;
         }
         else
         {
            m_buffer[i].m_yVel = 4;
         }
      }
  }

   bool CheckCollisions(sTimio *pPlayer)
   {
      for (int i = 0; i < DH_NUM_HAMS; i++)
      {
         if (m_buffer[i].m_active == TRUE)
         {
            if (m_buffer[i].CheckCollision(pPlayer))
            {
               if (pPlayer->m_invulnerable)
               {
                  // kill the ham
                  m_buffer[i].m_active = FALSE;
               }
               else
               {
                  return(TRUE);
               }
            }
         }
      }

      return(FALSE);
   }

};

//
// sDH : The ham-throwing pig
//
enum DHState
{
   eIdle,
   eGettingHam,
   eThrowingHam
};

#define DH_GET_HAM_CHANCE 0.10

// number of ticks (calls to Update()) to display each frame
#define DH_FRAME_LENGTH_GET_HAM 20
#define DH_FRAME_LENGTH_IDLE 20
#define DH_FRAME_LENGTH_THROW_HAM 20

#define DH_INITIAL_HAM_TIME 1.0

// time between ham throws
#define DH_HAM_INTERVAL_MIN 1.0
#define DH_HAM_INTERVAL_MAX 3.0

struct sDH
{

   Point m_pos;
   DHState m_state;
   int m_tickCount;
   int m_xSize;
   int m_ySize;

   float m_getHamChance;
   double m_nextHamTime;

   IRes *m_pKong[3];

   sDH(void)
   {
      m_state = eIdle;
      m_tickCount = 0;
      m_xSize = m_ySize = 32;
      m_getHamChance = DH_GET_HAM_CHANCE;
      m_nextHamTime = DH_INITIAL_HAM_TIME;

      // temp
      m_getHamChance = 1.0;

      // Grab the bitmaps
      m_pKong[0] = LoadPCX("dhkong0");
      m_pKong[1] = LoadPCX("dhkong1");
      m_pKong[2] = LoadPCX("dhkong2");
   }

   ~sDH() 
   {
      for(int i=0;i<3;++i) SafeFreeHnd(&m_pKong[i]);
   }

   void Place(int x, int y)
   {
      m_pos.x = x;
      m_pos.y = y;
   }

   void Draw(void)
   {
      switch(m_state)
      {
         case eIdle:
         {
            DrawIdleFrame();
            break;
         }
         case eGettingHam:
         {
            DrawGettingHamFrame();
            break;
         }
         case eThrowingHam:
         {
            DrawThrowingHamFrame();
            break;
         }
      }
   };


   // get a ham from offscreen
   void GetHam(void)
   {
      m_state = eGettingHam;
      m_tickCount = 0;
   }

   // throw the ham at Timio
   void ThrowHam(void)
   { 
      m_state = eThrowingHam;
      m_tickCount = 0;
   }

   void Update(void)
   {
      Draw();

      if (gUpdateFrames == FALSE) return;

      m_tickCount++;

      switch (m_state)
      {
         case eIdle:
         {
            if (g_pGame->m_GetTime() > m_nextHamTime)
            {
               m_tickCount = 0;
               GetHam();
            }

            break;
         }
         case eGettingHam:
         {
            if (m_tickCount > DH_FRAME_LENGTH_GET_HAM)
            {
               m_tickCount = 0;
               // you've got the ham. Now through it at Timio
               ThrowHam();
            }
            break;
         }
         case eThrowingHam:
         {
            if (m_tickCount > DH_FRAME_LENGTH_GET_HAM)
            {
               m_tickCount = 0;
               // make a ham and throw it
               LaunchHam();
               // throw the next ham DH_HAM_INTERVAL_MIN to DH_HAM_INTERVAL_MAX + MIN seconds from now
               m_nextHamTime = g_pGame->m_GetTime() + (RandFloat() * DH_HAM_INTERVAL_MAX) + DH_HAM_INTERVAL_MIN;

               m_state = eIdle;
            }
            break;
         }
      }
   }

   void LaunchHam(void)
   {
      // create the next ham and send it on it's way
      g_pGame->m_pHamBuffer->CreateHam(m_pos.x+m_xSize, m_pos.y+m_ySize-(DH_HAM_YSIZE));
   }


   //
   // since we can't get res stuff working at home, just fake it for now. Later,
   // change to draw the actual PCX
   //
   void DrawIdleFrame(void)
   {
      DrawByHandle(m_pKong[0],m_pos);
   }

   void DrawGettingHamFrame(void)
   {
      DrawByHandle(m_pKong[1],m_pos);
   }

   void DrawThrowingHamFrame(void)
   {
      DrawByHandle(m_pKong[2],m_pos);
   }
};


sTimio::sTimio(void)
{

   m_yFallVel = DH_TIMIO_FALL_VEL;
   m_yClimbingVel = DH_TIMIO_CLIMB_VEL;


   m_xSize = DH_TIMIO_XSIZE;
   m_ySize = DH_TIMIO_YSIZE;

   m_xVel = 0;
   m_yVel = 0; 
   m_jmpXVel = DH_TIMIO_DEF_JMPXVEL;

   m_jmpCount = 0;
   m_jmpDir = 1;
   m_jumping = FALSE;
   m_falling = FALSE;
   m_climbing = FALSE;
   m_checkHams = FALSE;
   m_invulnerable = FALSE;
   m_queueStop = FALSE;

   m_lastDir = 1; // what was the last way we were moving?

   m_currFrame = 0;

   m_pos.x = 0;
   m_pos.y = 0;

   char name[32];
   int i;


   for (i = 0; i < DH_TIMIO_NUM_WALK_FRAMES; i++)
   {
      sprintf(name, "dhtimwl%d", i);
      m_pLeftWalkFrames[i] = LoadPCX(name);
   }

   for (i = 0; i < DH_TIMIO_NUM_WALK_FRAMES; i++)
   {
      sprintf(name, "dhtimwr%d", i);
      m_pRightWalkFrames[i] = LoadPCX(name);
   }

   for (i = 0; i < DH_TIMIO_NUM_CLIMB_FRAMES; i++)
   {
      sprintf(name, "dhtimcl%d", i);
      m_pClimbFrames[i] = LoadPCX(name);
   }

   m_pJumpLeftFrame = LoadPCX("dhtimjpl");
   m_pJumpRightFrame = LoadPCX("dhtimjpr");

}

sTimio::~sTimio(void)
{
   int i;

   for (i = 0; i < DH_TIMIO_NUM_WALK_FRAMES; i++)
   {
      SafeFreeHnd(&m_pLeftWalkFrames[i]);
   }

   for (i = 0; i < DH_TIMIO_NUM_WALK_FRAMES; i++)
   {
      SafeFreeHnd(&m_pRightWalkFrames[i]);
   }

   for (i = 0; i < DH_TIMIO_NUM_CLIMB_FRAMES; i++)
   {
      SafeFreeHnd(&m_pClimbFrames[i]);
   }

   SafeFreeHnd(&m_pJumpLeftFrame);
   SafeFreeHnd(&m_pJumpRightFrame);
}

void sTimio::Reset()
{
   m_xVel = 0;
   m_yVel = 0; 

   m_jmpCount = 0;
   m_jmpDir = 1;
   m_jumping = FALSE;
   m_falling = FALSE;
   m_climbing = FALSE;
   m_checkHams = FALSE;
   m_queueStop = FALSE;


   m_lastDir = 1; // what was the last way we were moving?

   m_currFrame = 0;
}


// makes sure that Timio doesn't go off the screen
void sTimio::KeepOnBoard()
{
   // make sure you don't go off the board
   if (m_pos.x < 0)
   {
      m_pos.x = 0;
   }

   if (m_pos.x >= (140 - m_xSize)) 
   {
      m_pos.x = (139 - m_xSize);
   }
}

void sTimio::Move(void)
{
   Point nextUnderFootPos;
   Point nextUnderOtherFootPos;
   Point nextToMePos;

   if (gUpdateFrames == FALSE) return;

   //m_lastDir = (m_xVel > 0)?1:-1;

   if (m_jumping)
   {
      // keep moving in x while jumping
      if (m_xVel)
      {
//         m_pos.x += m_xVel;
         if (m_xVel > 0)
         {
            m_pos.x += m_jmpXVel;
         }
         else
         {
            m_pos.x -= m_jmpXVel;
         }
      }

      // I'm going up
      if (m_jmpDir == 1)
      {
         m_pos.y -= m_yVel;
         m_jmpCount++;
         if (m_jmpCount > 5)
         {
            m_jmpCount = 0;
            m_jmpDir = -1; // coming back down
            m_yVel = 0;
         }
         else
         {
            if (m_jmpCount)
            {
               m_yVel--;
            }
         }
      }
      else
      {
         // coming down
         m_pos.y += m_yVel;
         m_jmpCount++;
         if (m_jmpCount > 5)
         {
            // done jumping
            m_yVel = 0;
            m_jmpDir = 1;
            m_jmpCount = 0;
            m_jumping = FALSE;
            m_checkHams = FALSE;
            Point pos;
            pos.x = m_pos.x;
            pos.y = m_pos.y + m_ySize;
            g_pGame->StopCheckHams(pos);
            m_checkHams = FALSE;
         }
         else
         {
            if (m_jmpCount != 5)
            {
               m_yVel++;
            }
   
         }
      }


      // did I hit anything? if so, end jumping and put me on it
      Point myPosOnBoardLeft;
      Point myPosOnBoardRight;
      Point myPosOnBoardMiddle;


      myPosOnBoardLeft.x = m_pos.x / DH_GRID_RES_X;
      myPosOnBoardLeft.y = (m_pos.y + m_ySize) / DH_GRID_RES_Y;

      myPosOnBoardRight.x = (m_pos.x + m_xSize) / DH_GRID_RES_X;
      myPosOnBoardRight.y = (m_pos.y + m_ySize) / DH_GRID_RES_Y;

      myPosOnBoardMiddle.x = (m_pos.x + m_xSize/2) / DH_GRID_RES_X;
      myPosOnBoardMiddle.y = (m_pos.y + m_ySize) / DH_GRID_RES_Y;

      char myPosDataLeft = g_pGame->m_pBoard->GetData(myPosOnBoardLeft.x, myPosOnBoardLeft.y);
      char myPosDataRight = g_pGame->m_pBoard->GetData(myPosOnBoardRight.x, myPosOnBoardRight.y);
      char myPosDataMiddle = g_pGame->m_pBoard->GetData(myPosOnBoardMiddle.x, myPosOnBoardMiddle.y);
      if ( (myPosDataLeft & kBoardRightSlope) || (myPosDataLeft & kBoardLeftSlope))
      {
         // put me on top of it
         m_pos.y = myPosOnBoardLeft.y * DH_GRID_RES_Y - m_ySize;
         m_jumping = FALSE;

         Point tempPos;
         tempPos.x = m_pos.x;
         tempPos.y = m_pos.y + m_ySize;
         g_pGame->StopCheckHams(tempPos);
         m_checkHams = FALSE;
         KeepOnBoard();
         m_xVel = 0;
         m_yVel = 0;

         if (myPosDataLeft & kBoardWinningZone)
         {
            // woo-hoo!
            g_pGame->WonLevel();
         }
         return;

      }
         
         
      if ((myPosDataRight & kBoardRightSlope) || (myPosDataRight & kBoardLeftSlope) )
      {
         // put me on top of it
         m_pos.y = myPosOnBoardRight.y * DH_GRID_RES_Y - m_ySize;
         m_jumping = FALSE;
         Point tempPos;
         tempPos.x = m_pos.x;
         tempPos.y = m_pos.y + m_ySize;
         g_pGame->StopCheckHams(tempPos);
         m_checkHams = FALSE;
         KeepOnBoard();
         m_xVel = 0;
         m_yVel = 0;

         if (myPosDataRight & kBoardWinningZone)
         {
            // woo-hoo!
            g_pGame->WonLevel();
         }
         return;

      }

      if ((myPosDataMiddle & kBoardRightSlope) || (myPosDataMiddle & kBoardLeftSlope) )
      {
         // put me on top of it
         m_pos.y = myPosOnBoardMiddle.y * DH_GRID_RES_Y - m_ySize;
         m_jumping = FALSE;
         m_xVel = 0;
         m_yVel = 0;
         Point tempPos;
         tempPos.x = m_pos.x;
         tempPos.y = m_pos.y + m_ySize;
         g_pGame->StopCheckHams(tempPos);
         m_checkHams = FALSE;
         if (myPosDataMiddle & kBoardWinningZone)
         {
            // woo-hoo!
            g_pGame->WonLevel();
         }
         return;
      }



      KeepOnBoard();

      return;
   }         


   if (m_queueStop)
   {
      Stop(); m_queueStop = FALSE;
      return;
   }

   // am I falling?
   if (m_falling == TRUE)
   {
      Point myPosOnBoardLeft;
      Point myPosOnBoardRight;
      Point myPosOnBoardMiddle;

      m_pos.y += m_yFallVel;

      myPosOnBoardLeft.x = m_pos.x / DH_GRID_RES_X;
      myPosOnBoardLeft.y = (m_pos.y + m_ySize) / DH_GRID_RES_Y;

      myPosOnBoardRight.x = (m_pos.x + m_xSize) / DH_GRID_RES_X;
      myPosOnBoardRight.y = (m_pos.y + m_ySize) / DH_GRID_RES_Y;

      myPosOnBoardMiddle.x = (m_pos.x + m_xSize/2) / DH_GRID_RES_X;
      myPosOnBoardMiddle.y = (m_pos.y + m_ySize) / DH_GRID_RES_Y;


      char myPosDataLeft = g_pGame->m_pBoard->GetData(myPosOnBoardLeft.x, myPosOnBoardLeft.y);
      char myPosDataRight = g_pGame->m_pBoard->GetData(myPosOnBoardRight.x, myPosOnBoardRight.y);
      char myPosDataMiddle = g_pGame->m_pBoard->GetData(myPosOnBoardMiddle.x, myPosOnBoardMiddle.y);
      if ( (myPosDataLeft & kBoardRightSlope) || (myPosDataLeft & kBoardLeftSlope) ||
           (myPosDataRight & kBoardRightSlope) || (myPosDataRight & kBoardLeftSlope) ||
           (myPosDataMiddle & kBoardRightSlope) || (myPosDataMiddle & kBoardLeftSlope) 
         )
      {
         // put me on top of it
         m_pos.y = myPosOnBoardLeft.y * DH_GRID_RES_Y - m_ySize;
         m_falling = FALSE;
      }
      return; // don't allow any other movement while falling
   }


   if (m_climbing == TRUE)
   {
      Point myPosOnBoardLeft;
      Point myPosOnBoardRight;
      Point myPosOnBoardMiddle;

      m_pos.y += m_yClimbingVel;

      myPosOnBoardLeft.x = m_pos.x / DH_GRID_RES_X;
      myPosOnBoardLeft.y = (m_pos.y + m_ySize) / DH_GRID_RES_Y;

      myPosOnBoardRight.x = (m_pos.x + m_xSize) / DH_GRID_RES_X;
      myPosOnBoardRight.y = myPosOnBoardLeft.y;

      myPosOnBoardMiddle.x = (m_pos.x + m_xSize/2) / DH_GRID_RES_X;
      myPosOnBoardMiddle.y = myPosOnBoardLeft.y;



      char myPosDataLeft = g_pGame->m_pBoard->GetData(myPosOnBoardLeft.x, myPosOnBoardLeft.y);
      char myPosDataRight = g_pGame->m_pBoard->GetData(myPosOnBoardRight.x, myPosOnBoardRight.y);
      char myPosDataMiddle = g_pGame->m_pBoard->GetData(myPosOnBoardMiddle.x, myPosOnBoardMiddle.y);
     
      if ( (myPosDataLeft & kBoardRightSlope) || (myPosDataLeft & kBoardLeftSlope)   || 
           (myPosDataRight & kBoardRightSlope) || (myPosDataRight & kBoardLeftSlope) ||
           (myPosDataMiddle & kBoardRightSlope) || (myPosDataMiddle & kBoardLeftSlope) 
           )
      {
         // if i'm moving down, don't put me on top of a girder with a top ladder
         if ( (m_yClimbingVel >= 0)  && ( (myPosDataRight & kBoardTopLadder) || 
                                          (myPosDataLeft & kBoardTopLadder)  ||
                                          (myPosDataMiddle & kBoardTopLadder) )  )
         {
            return;
         }

         // put me on top of it
         m_pos.y = myPosOnBoardLeft.y * DH_GRID_RES_Y - m_ySize;
         m_climbing = FALSE;
      }
      return; // don't allow any other movement while climbing
   }



#if 0
   if ( (m_xVel == 0)  && (m_yVel == 0) )
   {
      return;
   }
#endif

   if (m_xVel)
   {
      // get the terrain at the next board grid BELOW my feet
      if (m_xVel > 0)
      {
         nextUnderFootPos.x = (m_pos.x + m_xSize + m_xVel) / DH_GRID_RES_X;
      }
      else
      {
         nextUnderFootPos.x = (m_pos.x  + m_xVel) / DH_GRID_RES_X;
      }
      nextUnderFootPos.y = ((m_pos.y + m_ySize) / DH_GRID_RES_Y);
   
      // get the terrain next to me AT my shin, to see if I bump into it
      nextToMePos.x = nextUnderFootPos.x;
      nextToMePos.y = nextUnderFootPos.y - 1;



   #if 0
      // put this back in
      if (nextBoardPos.x == boardPos.x)
      {
         // didn't change board pos. 
         m_pos.x += m_xVel;
         return;
      }
   #endif

      // what's under my other foot (nextUnderFootPos is based on movement direction)
      if (m_xVel > 0)
      {
         nextUnderOtherFootPos.x = (m_pos.x + m_xVel) / DH_GRID_RES_X;
      }
      else
      {
         nextUnderOtherFootPos.x = (m_pos.x + m_xSize + m_xVel) / DH_GRID_RES_X;
      }
      nextUnderOtherFootPos.y = nextUnderFootPos.y;

      char nextStepData = g_pGame->m_pBoard->GetData(nextToMePos.x, nextToMePos.y);
      char nextUnderFootData = g_pGame->m_pBoard->GetData(nextUnderFootPos.x, nextUnderFootPos.y);
      char nextUnderOtherFootData = g_pGame->m_pBoard->GetData(nextUnderOtherFootPos.x, nextUnderOtherFootPos.y);


      if ( (nextUnderFootData & kBoardWinningZone) || (nextUnderOtherFootData & kBoardWinningZone) )
      {
         // woo-hoo!
         g_pGame->WonLevel();
         return;
      }

      // is our way blocked?
      if ( (nextStepData & kBoardRightSlope) || (nextStepData & kBoardLeftSlope) )
      {
         // just move him up

         // put me on TOP of the thing in my way
         m_pos.y -= DH_GRID_RES_Y;
      }
      else
      {
         // what if nothing is under the next step?
         if ( (!nextUnderFootData) && (!nextUnderOtherFootData) )
         {
            // move him down
            m_pos.y += m_yFallVel;
            m_falling = TRUE;

            // as long as fall vel < Y_RES, we don't need to do anything here
         }
      }

      m_pos.x += m_xVel;
   }
   else
   {
      // when you're not moving, just check for falling, and use the blank pixel hack


      nextUnderFootPos.x = (m_pos.x + DH_TIMIO_BLANK_PIXELS) / DH_GRID_RES_X;
      nextUnderFootPos.y = ((m_pos.y + m_ySize) / DH_GRID_RES_Y);

      nextUnderOtherFootPos.x = (m_pos.x + m_xSize - DH_TIMIO_BLANK_PIXELS) / DH_GRID_RES_X;
      nextUnderOtherFootPos.y = nextUnderFootPos.y;

    
      char nextUnderFootData = g_pGame->m_pBoard->GetData(nextUnderFootPos.x, nextUnderFootPos.y);
      char nextUnderOtherFootData = g_pGame->m_pBoard->GetData(nextUnderOtherFootPos.x, nextUnderOtherFootPos.y);

      // what if nothing is under the next step?
      if ( (!nextUnderFootData) && (!nextUnderOtherFootData) )
      {
         // move him down
         m_pos.y += m_yFallVel;
         m_falling = TRUE;

         // as long as fall vel < Y_RES, we don't need to do anything here
      }
   }

   KeepOnBoard();

}

void sTimio::Draw(void)
{

   if (m_jumping)
   {
      if (m_xVel > 0)
      {
         DrawByHandle(m_pJumpRightFrame, m_pos);
      }
      else
      {
         if (m_xVel < 0)
         {
            DrawByHandle(m_pJumpLeftFrame, m_pos);
         }
         else
         {
            // not moving. Use last facing
            if (m_lastDir < 0)
            {
               DrawByHandle(m_pJumpLeftFrame, m_pos);
            }
            else
            {
               DrawByHandle(m_pJumpRightFrame, m_pos);
            }
         }
      }

      return;
   }

   if (m_climbing == TRUE)
   {

      DrawByHandle(m_pClimbFrames[m_currFrame], m_pos);

      if ( (gUpdateFrames == TRUE) && (m_yClimbingVel != 0) )
      {
         m_currFrame++;
         if (m_currFrame >= DH_TIMIO_NUM_CLIMB_FRAMES)
         {
            m_currFrame = 0;
         }
      }
      return;
   }



   if ( (m_xVel == 0) && (m_yVel == 0) )
   {
      // draw the idle for the current facing
      if (m_lastDir < 0)
      {
         DrawByHandle(m_pLeftWalkFrames[0], m_pos);
      }
      else
      {
         DrawByHandle(m_pRightWalkFrames[0], m_pos);
      }
      m_currFrame = 0;
      return;
   }

   if (gUpdateFrames == TRUE)
   {
      m_currFrame++;
      if (m_currFrame >= DH_TIMIO_NUM_WALK_FRAMES)
      {
         m_currFrame = 0;
      }
   }

   if (m_xVel < 0)
   {
      // walking left
      DrawByHandle(m_pLeftWalkFrames[m_currFrame], m_pos);
   }
   else
   {
      DrawByHandle(m_pRightWalkFrames[m_currFrame], m_pos);
   }

}

// places Timio in board coords, not pixel coords
void sTimio::PlaceOnBoard(Point boardPos)
{
   m_pos.x = boardPos.x * DH_GRID_RES_X;
   m_pos.y = boardPos.y * DH_GRID_RES_Y - m_ySize;
}

void sTimio::Update(void)
{
   Move();
   Draw();

   //  check for jumping over Hams
   if (m_checkHams)
   {
      g_pGame->CheckHams(m_pos);
   }
}

void sTimio::Jump(void)
{
   if (m_jumping == TRUE) return;
   if (m_falling == TRUE) return;
   if (m_climbing == TRUE) return;

   // don't allow jump on ladder


   m_yVel = DH_TIMIO_JUMP_VEL;
   m_jmpCount = 0;
   m_jmpDir = 1;
   m_jumping = TRUE;
   m_checkHams = TRUE;
   g_pGame->StartCheckHams();
}


   // change direction to left
void sTimio::MoveLeft()
{
   m_xVel = -DH_TIMIO_DEF_XVEL;
   m_lastDir = -1;
}

// change direction to right
void sTimio::MoveRight()
{
   m_xVel = DH_TIMIO_DEF_XVEL;
   m_lastDir = 1;
}

void sTimio::MoveUp()
{
   // can I go up a ladder?

   if (m_jumping == TRUE) return;
   if (m_falling == TRUE) return;
   if (m_xVel) return;

   // if we're climbing up, don't set up again
   if ( (m_climbing == TRUE) && (m_yClimbingVel < 0) )
   {
      return;
   }


   // check the board at my left extreme and my right extreme. If there is a ladder at either
   // place, go up it
   // also check my middle, now that art is bigger
   Point leftBoardPos;
   Point rightBoardPos;
   Point middleBoardPos;


   // look at left foot
   leftBoardPos.x = m_pos.x / DH_GRID_RES_X;
   leftBoardPos.y = (m_pos.y + m_ySize) / DH_GRID_RES_Y;
   // look at right foot
   rightBoardPos.x = (m_pos.x + m_xSize) / DH_GRID_RES_X;
   rightBoardPos.y = leftBoardPos.y;
   // look at middle
   middleBoardPos.x = (m_pos.x + m_xSize/2) / DH_GRID_RES_X;
   middleBoardPos.y = leftBoardPos.y;

   char leftStepData = g_pGame->m_pBoard->GetData(leftBoardPos.x, leftBoardPos.y);
   char rightStepData = g_pGame->m_pBoard->GetData(rightBoardPos.x, rightBoardPos.y);
   char middleStepData = g_pGame->m_pBoard->GetData(middleBoardPos.x, middleBoardPos.y);

   if ( (m_climbing) || (leftStepData & kBoardBottomLadder) || (rightStepData & kBoardBottomLadder) || (middleStepData & kBoardBottomLadder) )
   {
      m_yClimbingVel = -DH_TIMIO_CLIMB_VEL;
      if (!m_climbing)
      {
         // figure out which one hit, and center us on that
         if (middleStepData & kBoardBottomLadder)
         {
//            m_pos.x = middleBoardPos.x * DH_GRID_RES_X - (DH_GRID_RES_X / 2);
            m_pos.x = middleBoardPos.x * DH_GRID_RES_X - 2;
         }
         else
         {
            if (rightStepData & kBoardBottomLadder)
            {
//               m_pos.x = rightBoardPos.x * DH_GRID_RES_X - (DH_GRID_RES_X / 2);
               m_pos.x = rightBoardPos.x * DH_GRID_RES_X - 2;
            }
            else
            {
//               m_pos.x = leftBoardPos.x * DH_GRID_RES_X - (DH_GRID_RES_X / 2);
               m_pos.x = leftBoardPos.x * DH_GRID_RES_X - 2;
            }
         }
      }
      m_climbing = TRUE;
   }
}

void sTimio::MoveDown()
{

   if (m_jumping == TRUE) return;
   if (m_falling == TRUE) return;
   if (m_xVel) return;

   // if we're climbing down, don't set down again
   if ( (m_climbing == TRUE) && (m_yClimbingVel > 0) )
   {
      return;
   }

   // check the board at my left extreme and my right extreme. If there is a ladder at either
   // place, go down it
   // also check my middle, now that art is bigger
   Point leftBoardPos;
   Point rightBoardPos;
   Point middleBoardPos;

  // look at left foot
   leftBoardPos.x = m_pos.x / DH_GRID_RES_X;
   leftBoardPos.y = (m_pos.y + m_ySize) / DH_GRID_RES_Y;
   // look at right foot
   rightBoardPos.x = (m_pos.x + m_xSize) / DH_GRID_RES_X;
   rightBoardPos.y = leftBoardPos.y;
   // look at middle
   middleBoardPos.x = (m_pos.x + m_xSize/2) / DH_GRID_RES_X;
   middleBoardPos.y = leftBoardPos.y;

   char leftStepData = g_pGame->m_pBoard->GetData(leftBoardPos.x, leftBoardPos.y);
   char rightStepData = g_pGame->m_pBoard->GetData(rightBoardPos.x, rightBoardPos.y);
   char middleStepData = g_pGame->m_pBoard->GetData(middleBoardPos.x, middleBoardPos.y);

   if ( (m_climbing) || (leftStepData & kBoardTopLadder) || (rightStepData & kBoardTopLadder) || (middleStepData & kBoardTopLadder) )
   {
      m_yClimbingVel = DH_TIMIO_CLIMB_VEL;
      if (!m_climbing)
      {
         // figure out which one hit, and center us on that
         if (middleStepData & kBoardTopLadder)
         {
            //m_pos.x = middleBoardPos.x * DH_GRID_RES_X - (DH_GRID_RES_X / 2);
            m_pos.x = middleBoardPos.x * DH_GRID_RES_X - 2;
         }
         else
         {
            if (rightStepData & kBoardTopLadder)
            {
//               m_pos.x = rightBoardPos.x * DH_GRID_RES_X - (DH_GRID_RES_X / 2);
               m_pos.x = rightBoardPos.x * DH_GRID_RES_X - 2;
            }
            else
            {
//               m_pos.x = leftBoardPos.x * DH_GRID_RES_X - (DH_GRID_RES_X / 2);
               m_pos.x = leftBoardPos.x * DH_GRID_RES_X - 2;
            }
         }
      }
      m_climbing = TRUE;
   }
}

void sTimio::Stop()
{
   if (m_jumping)
   {
      m_queueStop = TRUE;
      return;
   }
   m_xVel = 0;
   m_yClimbingVel = 0;
}


sDonkeyHog::sDonkeyHog()
{
   m_pDHog = new sDH;
   m_pHamBuffer = new sHamBuffer;
   m_pBoard = new sBoard;
   m_pTimio = new sTimio;
   g_pGame = this;

   m_mouseIsDown = FALSE;
   m_lastMouseX = 0;
   m_lastMouseY = 0;

   m_level = 0;
   m_score = 0;

   m_waiting = FALSE;

}

sDonkeyHog::~sDonkeyHog()
{
   delete m_pDHog;
   delete m_pHamBuffer;
   delete m_pBoard;
   delete m_pTimio;
}


#define DH_DEF_HAM_JUMP_VALUE 100

// Initialize
void sDonkeyHog::Reset()
{
   m_mode = kDonkeyHogNewGame;

   m_pDHog->Place(1,11);
   m_pBoard->Set((char *)kBoard1);
   m_pTimio->PlaceOnBoard(gBoard1Pos);
   m_score = 0;
   m_hamJumpValue = DH_DEF_HAM_JUMP_VALUE;
   m_lives = 3;

}

// Render the stats, score, lives, etc
void sDonkeyHog::m_StatsRender()
{
   char buf[64];
   sprintf(buf,"%d",m_level+1);
   gr_font_string(gShockFont,buf,4,3);
   sprintf(buf,"%d",m_score);
   gr_font_string(gShockFont,buf,110,3);
   if (m_pTimio->m_invulnerable)
   {
      sprintf(buf,"%d", m_lives);
      gr_font_string(gShockFont,"!HACKED!",44,3);
   }
   else
   {
      sprintf(buf,"%d", m_lives);
      gr_font_string(gShockFont,buf,66,3);
   }
}



#define DH_TIME_PER_FRAME  (1 / 20.0)
void sDonkeyHog::m_UpdateTime()
{

   if (m_waiting)
   {
      // don't do anything while you're waiting: we will use gUpdateFrames for other
      // reasons while waiting
      return;
   }

   double cur = m_GetTime();
   if (m_lastTime>0) {
      m_frameDelta = cur-m_lastTime;
   }  
   else
   {
      m_lastTime = cur;
      return;
   }

   if (m_frameDelta < DH_TIME_PER_FRAME)
   {
      // wait for time to pass
      gUpdateFrames = FALSE;
   }
   else
   {
      m_lastTime = cur;
      gUpdateFrames = TRUE;
   }
}

#define DH_MOUSE_X_RANGE 10
#define DH_MOUSE_Y_RANGE 20

void sDonkeyHog::m_GetInput(void)
{
   short x,y;
   bool button;


   // jumping takes precedence
   mouse_check_btn(1,&button);
   if (button)
   {
      m_pTimio->Jump();
      return;
   }

   mouse_get_xy(&x,&y);
   // relativize to the point here....
   x-=m_pt.x;
   y-=m_pt.y;

   mouse_check_btn(0,&button);

   if (m_mouseIsDown)
   {
      // is the mouse still down?
      if (button)
      {

         // check y first, so we can exit if we decide to move up or down
         if (y != m_lastMouseY)
         {
            // limit this so only a little bit of y movement is allowed
            if ( (y >= (m_lastMouseY + DH_MOUSE_Y_RANGE) ) && (abs(x - m_lastMouseX) < DH_MOUSE_X_RANGE) )
            {
               m_pTimio->MoveDown();
               return;
            }
            else
            {
               if ((y < (m_lastMouseY + DH_MOUSE_Y_RANGE) ) && (abs(x - m_lastMouseX) < DH_MOUSE_X_RANGE) )
               {
                  m_pTimio->MoveUp();
                  return;
               }
            }
         }


         if (x != m_lastMouseX)
         {
            // left mouse still down. Move relative to original position
            if (x <= m_lastMouseX)
            {
               m_pTimio->MoveLeft();
            }
            else
            {
               m_pTimio->MoveRight();
            }
         }
      }
      else
      {
         // button is up. Stop.
         m_pTimio->Stop();
         m_mouseIsDown = FALSE;
      }
   }
   else
   {
      // did the player press the left button?
      if (button)
      {
         m_mouseIsDown = TRUE;
         m_lastMouseX = x;
         m_lastMouseY = y;
      }
   }

}




// check to see if Timio is jumping over any Hams. Called when jumping
void sDonkeyHog::StartCheckHams()
{
   // tell the buffer to set the jump check flag on the Hams, so they'll only 
   // be checked once
   m_pHamBuffer->FlagHams();
}

// should call with footpos
void sDonkeyHog::StopCheckHams(Point footPos)
{
   int score = m_pHamBuffer->GetScore(footPos);
   m_pHamBuffer->UnFlagHams();

   m_score += (score * m_hamJumpValue);

   if (m_score)
   {
      if ((m_score % 5000) == 0)
      {
         m_lives++;
         if (m_lives > 7)
         {
            m_lives = 7;
         }
      }
   }
}

void sDonkeyHog::CheckHams(Point pos)
{
   m_pHamBuffer->CheckHams(pos);
}

void sDonkeyHog::WonLevel()
{

   // This will be called during the celebration jumps
   if (m_mode == kDonkeyHogFinished) return;

   // display jumping
   m_mode = kDonkeyHogFinished;

   m_score += 500;

   m_pTimio->m_jumping = FALSE;
   m_pTimio->m_falling = FALSE;
   m_pTimio->m_climbing = FALSE;
   m_pTimio->m_xVel = 0;
   m_pTimio->m_yVel = 0;

   m_waiting = TRUE;
   m_waitTime = m_GetTime() + 4.0;

}


void sDonkeyHog::m_ResetBoard()
{
   m_pHamBuffer->Clear();
   m_pDHog->m_state = eIdle;
   m_pDHog->DrawIdleFrame();
   m_pTimio->Reset();
   if (m_level % 2)
   {
      // use 2nd board

      m_pBoard->Set((char *)kBoard2);
      m_pTimio->PlaceOnBoard(gBoard2Pos);
      m_pTimio->m_lastDir = -1; // face him the right way

   }
   else
   {
      // use 1st board
      m_pBoard->Set((char *)kBoard1);
      m_pTimio->PlaceOnBoard(gBoard1Pos);
      m_pTimio->m_lastDir = 1; // face him the right way
   }
}

void sDonkeyHog::m_Wait(double time, eDonkeyHogMode nextMode)
{
   double curTime = m_GetTime();

   m_waitTime = curTime + time;
   m_queuedMode = nextMode;
   m_waiting = TRUE;
   m_mode = kDonkeyHogWait;
}

double sDonkeyHog::m_GetTime()
{
   return   (double)tm_get_millisec()/(double)1000.0;
}

// Render an process a frame;
void sDonkeyHog::Frame()
{
   // Update the time
   m_UpdateTime();

   r3_start_frame();
   // Set the camera
   r3_set_view(&sMxVector(-1,0,0));
   
   r3_start_block();

   
   switch (m_mode)
   {
      case kDonkeyHogNewGame:
      {
         m_level = 0;
         m_score = 0;
         m_lastButton = 0;
         m_frameDelta = 0;
         m_lastTime = 0;
         m_lives = 3;
         //NewLevel(0);

         // Go into the level
         m_mode = kDonkeyHogInLevel;
         break;
      }
      case kDonkeyHogInLevel: 
      {

         // Clear the background or use art with some decorations
         gr_clear(0);

         if (ShockMiniGameIsHacked())
         {
            m_pTimio->m_invulnerable = TRUE;
         }
         else
         {
            m_pTimio->m_invulnerable = FALSE;
         }

         // Render the stats
         m_StatsRender();

         m_GetInput();

         m_pBoard->Draw();
         m_pDHog->Update();
         m_pHamBuffer->Update();
         m_pTimio->Update();
        
         if (m_pHamBuffer->CheckCollisions(m_pTimio))
         {
            m_mode = kDonkeyHogDied;
         }
         break;
      }   
      case kDonkeyHogDied:
      {
         m_lives--;
         if (m_lives <= 0)
         {
            m_mode = kDonkeyHogGameOver;
            break;
         }
         
         m_Wait(3, kDonkeyHogInLevel);
         break;
      }
      case kDonkeyHogWait:
      {
         gr_clear(0);
         if (m_GetTime() < m_waitTime)
         {
            // just draw the screen
            gUpdateFrames = FALSE;
            m_pBoard->Draw();
            m_pDHog->Update();
            m_pHamBuffer->Update();
            m_pTimio->Update();
            m_StatsRender();

            if (m_lives <= 0)
            {
               gr_font_string(gShockFont,"GAME OVER",46,134);
            }
            break;
         }
         
         m_waiting = FALSE;
         gUpdateFrames = TRUE;
         // ok, done waiting
         m_ResetBoard();
         m_mode = m_queuedMode;
         break;
      }

      case kDonkeyHogGameOver:
      {
         gr_font_string(gShockFont,"GAME OVER",46,134);
   
         m_Wait(5, kDonkeyHogReset);

         break;
      }

      case kDonkeyHogReset:
      {
         Reset();
         break;
      }


      case kDonkeyHogFinished:
      {
         gr_clear(0);


         // jump up and down for a while
         if (m_GetTime() < m_waitTime)
         {
            // fake out the frame delta stuff
            m_waiting = FALSE;
            m_UpdateTime();
            m_pTimio->Jump();
            m_pTimio->Update();
            m_waiting = TRUE;

            // just draw the screen
            gUpdateFrames = FALSE;
            m_pBoard->Draw();
            m_pDHog->Update();
            m_pHamBuffer->Update();
            m_StatsRender();
            break;
         }


         gUpdateFrames = TRUE;
         m_waiting = FALSE;
         m_level++;

         m_pHamBuffer->IncBarrelSpeed();
         m_ResetBoard();
         m_mode = kDonkeyHogInLevel;
         break;
      }

   }

   r3_end_block();
   r3_end_frame();
}

// Go to next level
void sDonkeyHog::NewLevel(int level)
{
}

////////////////
// the interface from the minigame player

static sDonkeyHog *pDonkeyHog=NULL;

void DonkeyHogInit(void)
{
   pDonkeyHog = new sDonkeyHog();
   pDonkeyHog->Reset();
}

void DonkeyHogTerm(void)
{
   delete pDonkeyHog;
}

static Rect base_rect = {{ 13, 11},{154,288}};

void DonkeyHogDraw(Point pt)
{
   // push a canvas here for the current draw rectangle?
   grs_canvas cnv;
   grs_bitmap *bmp;
   /*
   gr_init_sub_canvas(grd_canvas,&cnv,
                      pt.x+base_rect.ul.x,pt.y+base_rect.ul.y,
                      base_rect.lr.x-base_rect.ul.x,
                      base_rect.lr.y-base_rect.ul.y);
   */
   // Jaemz: I changed this to alloc, even though it is slower, because
   // many HW cards barf if you subcanvas them, so life is rough. -- X
   bmp = gr_alloc_bitmap(BMT_FLAT16, 0, RectWidth(&base_rect),RectHeight(&base_rect));
   gr_make_canvas(bmp,&cnv);

   // do the actual drawing
   gr_push_canvas(&cnv);
   pDonkeyHog->m_pt=pt;
   pDonkeyHog->Frame();
   gr_pop_canvas();

   // blit it
   gr_bitmap(bmp,pt.x + base_rect.ul.x, pt.y + base_rect.ul.y);

   // clear up our alloc
   gr_free(bmp);
}

void DonkeyHogMouse(Point pt)
{
   pDonkeyHog->m_btnDown=TRUE;
}

void DonkeyHogRightMouse(Point pt)
{
   pDonkeyHog->m_btnRightDown=TRUE;
}

