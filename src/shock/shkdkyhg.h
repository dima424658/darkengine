/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkdkyhg.h,v 1.5 1999/06/11 19:55:17 tfarrar Exp $

#ifndef __SHKDKYHG_H
#define __SHKDKYHG_H
#pragma once

#include <dev2d.h>
#include <rect.h>

struct IRes;
struct sDH;
struct sHamBuffer;
struct sBoard;
struct sTimio;

enum eDonkeyHogMode {
   kDonkeyHogNewGame,       // Start a new game
   kDonkeyHogInLevel,       // in the level
   kDonkeyHogDied,          // you died!
   kDonkeyHogFinished,      // you successfullly finished a level
   kDonkeyHogWait,
   kDonkeyHogGameOver,
   kDonkeyHogReset
};

struct sDonkeyHog { 
   eDonkeyHogMode m_mode;
   int m_level;
   int m_score;
   float m_secondsRemaining;
   int m_lives;

   // last state of the button used for detecting down events
   bool m_lastButton;
   // Origin of frame
   Point m_pt;
   // left button is down
   bool m_btnDown;
   // right button is down
   bool m_btnRightDown;

   bool m_waiting;

   // last time and the delta in seconds
   double m_lastTime;
   float m_frameDelta;
   

   double m_waitTime;
   eDonkeyHogMode m_queuedMode;

   // The burro hog, himself
   sDH *m_pDHog;

   // the buffer which controls the hams
   sHamBuffer *m_pHamBuffer;

   // the game board
   sBoard *m_pBoard;

   // the player
   sTimio *m_pTimio;

   // mouse state info.
   bool m_mouseIsDown;
   int m_lastMouseX;
   int m_lastMouseY;
   int m_hamJumpValue;

   sDonkeyHog();
   ~sDonkeyHog();

   // Render the stats, score, time, etc
   void m_StatsRender();

   // Update the time, including frame delta
   void m_UpdateTime();

 
   // reset the game Initialize
   void Reset();
   // Render an process a frame;
   void Frame();
   // Go to next level, pass it the number
   void NewLevel(int level);

   // poll the mouse and buttons
   void m_GetInput(); 

   // check to see if Timio is currently jumping over any hams
   void StartCheckHams();
   void CheckHams(Point pos);
   void StopCheckHams(Point footPos);

   void WonLevel();

   void m_ResetBoard();
   void m_Wait(double time, eDonkeyHogMode nextMode);

   double m_GetTime();
};






//
// the player
//

#define DH_TIMIO_NUM_FRAMES 6

#define DH_TIMIO_NUM_WALK_FRAMES 4
#define DH_TIMIO_NUM_CLIMB_FRAMES 2


//#define DH_TIMIO_XSIZE 8
#define DH_TIMIO_XSIZE 12
#define DH_TIMIO_YSIZE 16

// used in hackly collision detection
//#define DH_TIMIO_BLANK_PIXELS 2
#define DH_TIMIO_BLANK_PIXELS 3
#define DH_TIMIO_BLANK_JMP_PXLS 3

#define DH_TIMIO_DEF_XVEL 3
#define DH_TIMIO_DEF_YVEL 3

#define DH_TIMIO_DEF_JMPXVEL 2

#define DH_TIMIO_JUMP_VEL 4


// KEEP THIS SMALLER THAN BOARD_Y_RES!!!
#define DH_TIMIO_FALL_VEL 2

#define DH_TIMIO_CLIMB_VEL 2

struct sTimio
{
   IRes *m_pLeftWalkFrames[DH_TIMIO_NUM_WALK_FRAMES];
   IRes *m_pRightWalkFrames[DH_TIMIO_NUM_WALK_FRAMES];
   IRes *m_pClimbFrames[DH_TIMIO_NUM_CLIMB_FRAMES];
   IRes *m_pJumpLeftFrame;
   IRes *m_pJumpRightFrame;

   int m_xSize;
   int m_ySize;

   int m_xVel;
   int m_yVel;
   int m_jmpXVel;

   int m_yFallVel;
   int m_yClimbingVel;

   int m_currFrame;
   int m_lastDir;

   Point m_pos;

   
   int m_jmpCount;
   int m_jmpDir;
   bool m_jumping;
   bool m_falling;
   bool m_climbing;
   bool m_checkHams;
   bool m_invulnerable;
   bool m_queueStop;

   sTimio(void);
   ~sTimio(void);

   // makes sure that Timio doesn't go off the screen
   void KeepOnBoard();
   void Move(void);
   void Draw(void);
   // places Timio in board coords, not pixel coords
   void PlaceOnBoard(Point boardPos);
   void Update(void);
   void Jump(void);
   // change direction to left
   void MoveLeft();
   // change direction to right
   void MoveRight();
   void MoveUp();
   void MoveDown();
   void Stop();

   void Reset();
};



#endif
