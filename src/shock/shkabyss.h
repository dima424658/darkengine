/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header:

#ifndef __SHKABYSS_H
#define __SHKABYSS_H
#pragma once

#include <dev2d.h>
#include <shkweb.h>
#include <rect.h>

// Maximum of 16 shots at one time
const kMaxShots=16;
// One shot
struct sShot {
   int m_spoke; // spoke it was shot from (-1 means available slot)
   double m_launchTime; // the time it was launched
};

const kMaxMonsters=6;
struct sEdge
{
   uchar v1;    // indices of vertex
   uchar v2;
   uchar color; // color array
};

struct sVert {
   float x;
   float y;
};

// Definition, not instance, of a monster
struct sMonster {
   int m_score;         // how much to add to score
   float m_speed;       // speed of travel
   float m_height;      // height of monster
   float m_flip;        // chances of flipping
   int m_numVerts;      // number of vertices in the monster
   sVert *m_pVerts;   // pointer to vert array
   int m_numEdges;
   sEdge *m_pEdges;
};

// Instance of a monster
struct sMonsterRef {
   sMonster *m_pMonster;  // which kind of monster
   int m_spoke;
   float m_x;     // x coordinate of the head, moves upwards
   float m_lastFlipX;
};

// one floating monster...
const kMaxFloatingMonsters = 16;
struct sMonsterFloat {
   sMonster *m_pMonster;
   sMxAngVec m_ang;
   sMxVector m_pos;
   float m_width;
};


enum eAbyssMode {
   kAbyssNewGame,       // Start a new game
   kAbyssInLevel,       // in the level
   kAbyssDied,          // you died!
   kAbyssFinished,      // you successfullly finished a level
   kAbyssGameOver       // games over!
};

struct sAbyss { 
   eAbyssMode m_mode;
   int m_level;
   int m_score;
   float m_secondsRemaining;
   int m_lives;
   float m_averageMonster;// average monster level
   Point m_pt;
   bool m_usedBombThisLevel;

   // last state of the button used for detecting down events
   bool m_lastButton;
   // last time and the delta in seconds
   double m_lastTime;
   float m_frameDelta;


   // playing field canvas
   grs_canvas m_playCanv;
   // Pointer to current web
   sWeb *m_pWeb;

   sAbyss();
   ~sAbyss();

   // used to increment the score, duh
   int m_lastScoreBoundary;
   void m_IncrementScore(int inc);

   // Render the stats, score, time, etc
   void m_StatsRender();

   // Given an x,y, find the spoke that it's on
   int m_FindSpoke(int x,int y);

   // Update the time, including frame delta
   void m_UpdateTime();

   // returns true when has waited sec
   // stuff pDelta with time waited so far
   bool m_Waited(float *pDelta,float sec);    

   // reset the game Initialize
   void Reset();
   // Render an process a frame;
   void Frame();
   // Go to next level, pass it the number
   void NewLevel(int level);

   // Fire a new shot from this spoke
   bool m_ShotAdd(int spoke);
   // Render the shots, and kill any that are too old
   void m_ShotsRender();
   // Clear all the shots
   void m_ShotClear();  

   // Player stuff
   int m_playerSpoke;

   // Player Move
   void m_PlayerMove();

   // Player Render
   void m_PlayerRender();

   // Render a monster ref
   void m_MonsterRender(const sMonsterRef &rRef);

   // Blit onto the screen like a bitmap, uses gr_fix_line
   void m_MonsterBitmap(sMonster *pMon,int x,int y,int w,int h);

   // Add a monster to this spoke
   bool m_MonsterAdd(int spoke,int which);

   // Render the monsters, advance them, and kill them if hit
   void m_MonstersRender();
   void m_MonstersMove();

   // Clear all the monsters
   void m_MonsterClear();

   // Kill all the monsters, giving you scoreage
   void SmartBomb();

   // Shot array
   sShot m_shots[kMaxShots];

   // Monster array
   sMonsterRef m_monsters[kMaxMonsters];

   // speed scale for monsters
   float m_speedScale;

   // frequency of appearance for monsters
   float m_monsterFreq;

   // Creates num floating monsters
   // randomly seeded from startx to endx distributed about some radius
   // clears all the existing ones
   // sets m_numFloating
   void m_FloatingCreate(float startx,float endx,float rad,int num);

   // Render all the floating monsters and rotate them a bit
   void m_FloatingsRender();

   // Render one floating monster
   void m_FloatingRender(const sMonsterFloat &rMon);

   // Floating monsters for the fly-through scene
   sMonsterFloat m_floating[kMaxFloatingMonsters];
   int m_numFloating;
};

#endif  // __SHKABYSS_H




