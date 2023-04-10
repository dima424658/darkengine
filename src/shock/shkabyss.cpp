/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkabyss.cpp,v 1.6 1999/06/17 01:22:25 dc Exp $

//#define STANDALONE

#ifdef STANDALONE
#define OUR_OWN_CANVAS
#endif

#include <r3d.h>
#include <mouse.h>
#include <dev2d.h>
#include <mxmats.h>
#include <mprintf.h>
#include <timer.h>
#include <random.h>
#include <g2.h>
#include <mxangs.h>

#include <shkabyss.h>

/////////
// necessary maybe for shock minigame hookup stuff, maybe, who knows
#include <shkovrly.h>
#include <shkutils.h>
#include <shkovcst.h>
#include <shkmfddm.h>
#include <shkobjst.h>
#include <shkprop.h>
#include <shkiftul.h>
#include <shkplayr.h>
#include <shkminig.h>

#include <filevar.h>   // for save load

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
}

#include <dbmem.h>

// Shots
const float kShotSpeed = 8;  // m/s
const float kShotLength = 1.5; // meters

// Colors
enum eAbyssColors {
   kBlack,
   kWhite,
   kRed,
   kBlue,
   kGreen,
   kYellow,
   kPurple,
   kPink,
   kLtBlue,
   kNumColors
};

// color def
struct sCol {
   uchar r,g,b;
};

sCol colSrc[kNumColors] = {
   {0,0,0},
   {255,255,255},
   {255,0,0},
   {0,0,255},
   {0,255,0},
   {255,255,0},
   {255,0,255},
   {255,128,128},
   {128,128,255}
};

static int colors[kNumColors];

// Given 8 bit color, returns 8 or 16, depending 
static int PalClosestG2(uchar r,uchar g,uchar b)
{
   if (grd_bpp==15) return ((r>>3)<<10)|((g>>3)<<5)|(b>>3);
   return ((r>>3)<<11)|((g>>2)<<5)|(b>>3);
}

static void colorInit()
{
   int i;
   for (i=0;i<kNumColors;++i) {
      colors[i] = PalClosestG2(colSrc[i].r,colSrc[i].g,colSrc[i].b);
   }
}

// Monster definitions
sVert playerVerts[7] = {
   {0,0},{.4,1},{.6,1},{1,0},{.55,.7},{.5,.1},{.45,.7}
};
sEdge playerEdges[7] = {
   {0,1,kYellow},{1,2,kYellow},{2,3,kYellow},{3,4,kYellow},{4,5,kYellow},{5,6,kYellow},{6,0,kYellow}
};

sMonster playerMonster = {
   0,0,.2,0,      // hp, speed, height, flip
   7, playerVerts,       // vertices
   7, playerEdges       // edges
};

// bow tie
sVert monster1Verts[4] = {
   {0,0},{1,1},{1,0},{0,1}
};
sEdge monster1Edges[4] = {
   {0,1,kGreen},{1,2,kGreen},{2,3,kGreen},{3,0,kGreen}
};

// ant
sVert monster2Verts[12] = {
   {0,0},{.25,.25},{0,1},{.25,.75},{1,1},{.75,.75},{1,0},{.75,.25},
   {.5,.1},{.1,.5},{.5,.9},{.9,.5}
};
sEdge monster2Edges[8] = {
   {0,1,kLtBlue},{2,3,kLtBlue},{4,5,kLtBlue},{6,7,kLtBlue},
   {8,9,kGreen},{9,10,kGreen},{10,11,kGreen},{11,8,kGreen}
};

// snakey
sVert monster3Verts[8] = {
   {.6,.1},{.3,.2},{.7,.5},{.2,.6},{.5,.7},
   {1,.7},{0,.7},{.5,1}
};
sEdge monster3Edges[7] = {
   {0,1,kYellow},{1,2,kPurple},{2,3,kPurple},{3,4,kPurple},
   {5,6,kRed},{6,7,kRed},{7,5,kRed}
};

// bem
sVert monster4Verts[11] = {
   {.2,.7},{.1,.8},{.2,.9},{.3,.8},
   {.8,.7},{.7,.8},{.8,.9},{.9,.8},
   {0,0},{.5,.4},{1,0}
};
sEdge monster4Edges[13] = {
   {0,1,kYellow},{1,2,kYellow},{2,3,kYellow},{3,0,kYellow},
   {4,5,kYellow},{5,6,kYellow},{6,7,kYellow},{7,4,kYellow},
   {0,9,kRed},{4,9,kRed},
   {8,9,kRed},{9,10,kRed},{10,8,kRed}
};

// ship
sVert monster5Verts[9] = {
   {.5,0},{0,.3},{0,.8},{.5,.5},{1,.8},{1,.3},
   {0,1},{1,1},{.5,.9}
};

sEdge monster5Edges[7] = {
   {0,1,kPink},{2,3,kPink},{3,4,kPink},{5,0,kPink},
   {1,6,kPink},{0,8,kYellow},{5,4,kPink}
};

sMonster monster1 = {
   10,1,1,4,      // hp, speed, height, flip
   4, monster1Verts,       // vertices
   4, monster1Edges       // edges
};
sMonster monster2 = {
   20,1.3,1,3.2,      // hp, speed, height, flip
   12, monster2Verts,  // vertices
   8, monster2Edges       // edges
};
sMonster monster3 = {
   40,1.6,1.7,3,      // hp, speed, height, flip
   8, monster3Verts,       // vertices
   7, monster3Edges       // edges
};
sMonster monster4 = {
   50,1.8,1,2.5,      // hp, speed, height, flip
   11, monster4Verts,       // vertic
   13, monster4Edges       // edges
};
sMonster monster5 = {
   100,2.0,1,1.8,      // hp, speed, height, flip
   9, monster5Verts,       // vertices
   7, monster5Edges       // edges
};

const kNumMonsters=5;
sMonster *pMonsters[] = {
   &monster1,&monster2,&monster3,&monster4,&monster5
};

#ifndef STANDALONE
///////////////////////////////////////////
// more zany support for the MFD
#define GetNamedString(buf,name)    ShockStringFetch(buf,sizeof(buf),name,"minigame")

////////////////////
// high score table support
sFileVarDesc gAbyssScoreDesc =
{
   kCampaignVar,
   "AbyssScr",
   "Abyss High Scores",
   FILEVAR_TYPE(sMiniGameScores),
   {1,0},
   {1,0},
   "shock",
};

cFileVar<sMiniGameScores,&gAbyssScoreDesc> gAbyssScoreTable;

// some day, do this less dippily
void show_game_over(void)
{
   char buf[64];
   GetNamedString(buf,"ab_gameover");
   gr_font_string(gShockFont,buf,48,84);
   ShockMiniGameDisplayHighScores(&gAbyssScoreTable,8,110);
   gr_font_string(gShockFont,gMiniGameScoreMsg,20,232);
}

void store_high_score(int score, int lvl)
{
   char buf[64], form_buf[64];

   GetNamedString(form_buf,"ab_score_form");
   sprintf(buf,form_buf,score,lvl);
   ShockMiniGameAddHighScore(&gAbyssScoreTable,score,buf);
}

#else
#define show_game_over()
#define store_high_score(s,l)
#endif

sAbyss::sAbyss()
{
   m_pt.x=0;  // duh!
   m_pt.y=0; 
   m_usedBombThisLevel=FALSE;
}

sAbyss::~sAbyss()
{
   delete m_pWeb;
}

// Initialize
void sAbyss::Reset()
{
   colorInit();
#ifdef OUR_OWN_CANVAS
   gr_init_sub_canvas(grd_canvas,&m_playCanv,0,0,2*140,2*276);
#endif
   m_mode = kAbyssNewGame;
}

// Render the stats, score, lives, etc
void sAbyss::m_StatsRender()
{
   int i;
   for (i=0;i<m_lives;++i) {
      m_MonsterBitmap(&playerMonster,i*17+5,grd_bm.h-20,15,15);
   }

#ifdef STANDALONE
   mono_setxy(30,0);
   int t = floor(m_secondsRemaining);
   mprintf("%d   ",t);

   mono_setxy(40,0);
   mprintf("%d   ",m_level+1);

   mono_setxy(50,0);
   mprintf("%d   ",m_score);
#else
   char buf[64];
   sprintf(buf,"%d",m_level+1);
   gr_font_string(gShockFont,buf,4,3);
   int t = floor(m_secondsRemaining);
   int frac=(int)((m_secondsRemaining-t)*100);
   sprintf(buf,"%d.%d",t,frac);
   gr_font_string(gShockFont,buf,52,3);  // who knows, whatever, 52, sure
   sprintf(buf,"%d",m_score);
   gr_font_string(gShockFont,buf,110,3);
#endif
}

void sAbyss::m_IncrementScore(int inc)
{
   int old = m_score;
   m_score += inc;
   if (old < m_lastScoreBoundary && m_score>=m_lastScoreBoundary)
   {
      m_lastScoreBoundary *= 2;
      m_lives++;
      if (m_lives>7) m_lives = 7;
   }
}

// returns true when done
bool sAbyss::m_Waited(float *pDelta,float sec)
{
   static double last = 0;      
   float delta;
   double cur =(double)tm_get_millisec()/(double)1000.0;
   if (last == 0) {
      last=cur;
   }
   delta = cur-last;
   bool done = delta > sec;
   // reset if waited long enough
   if (done) last = 0;
   if (pDelta) *pDelta = delta;
   return done;
}

void sAbyss::m_UpdateTime()
{
   double cur =(double)tm_get_millisec()/(double)1000.0;
   if (m_lastTime>0) {
      m_frameDelta = cur-m_lastTime;
   }    
   m_lastTime = cur;
#ifdef STANDALONE
   mono_setxy(0,0);
#endif
}

// Render an process a frame;
void sAbyss::Frame()
{
   // Update the time
   m_UpdateTime();

#ifdef OUR_OWN_CANVAS
   gr_push_canvas(&m_playCanv);
#endif
   r3_start_frame();
   // Set the camera
   r3_set_view(&sMxVector(-1,0,0));
   
   r3_start_block();
   
   // Check for down event
   bool downEvent;
   bool button;
   mouse_check_btn(0,&button);
   downEvent = (button&&!m_lastButton);
   m_lastButton = button;

   gr_clear(0);
   
   switch (m_mode)
   {
      case kAbyssNewGame:
      {
         m_level = 0;
         m_score = 0;
         m_pWeb = NULL;
         m_lastButton = 0;
         m_frameDelta = 0;
         m_lastTime = 0;
         m_playerSpoke = 0;
         m_lives = 3;
         m_lastScoreBoundary = 5000;
         NewLevel(0);

         // Go into the level
         m_mode = kAbyssInLevel;
         break;
      }
      case kAbyssInLevel: 
      {
         // Render the stats
         m_StatsRender();

         // Render the web
         m_pWeb->Render(colors[kWhite]);
   
         // Move and Render the ship
         m_PlayerMove();
         m_PlayerRender();

         // Render monsters
         m_MonstersMove();

         m_MonstersRender();
   
         // Render any shots
         m_ShotsRender();

         if (downEvent) {
            m_ShotAdd(m_playerSpoke);
         }

         // only count down in the sim
         m_secondsRemaining -= m_frameDelta;
         if (m_secondsRemaining < 0) {
            m_secondsRemaining = 0;
            m_mode = kAbyssFinished;
         }

         break;
      }   
      case kAbyssDied:
      {
         float delta;
         if (!m_Waited(&delta,5))
         {
            r3_end_block();
            sMxVector pos(-1-delta*8,0,0);

            r3_set_view(&pos);
            r3_start_block();

            // Render the stats
            m_StatsRender();

            // Render the web
            m_pWeb->Render(colors[kRed]);
   
            // Render the ship
            m_PlayerRender();

            // Render the monsters
            m_MonstersRender();
         } else {
            NewLevel(-1);
            m_mode = kAbyssInLevel;
         }
         break;
      }
      case kAbyssFinished:
      {
         float delta;
         if (!m_Waited(&delta,5)) {

            // if first time, create the floaters
            if (delta==0) {
               m_FloatingCreate(m_pWeb->m_depth+1,-1+5*5,2,.7*kMaxFloatingMonsters);
            }

            // zoom and break;
            r3_end_block();
            sMxVector pos(-1+delta*5,0,0);
            r3_set_view(&pos);
            r3_start_block();

            // Render the stats
            m_StatsRender();

            // Render the web
            m_pWeb->Render(colors[kWhite]);
   
            // Render the ship
            m_PlayerRender();

            // Render the monsters
            m_MonstersRender();

            // Render the floaters
            m_FloatingsRender();
         } else {
            // next level
            NewLevel(m_level+1);
            m_mode = kAbyssInLevel;
         }
         break;
      }
      case kAbyssGameOver:
      {
         static sMxAngVec a(0,0,0);
         // zoom and break;
         static double start = 0;
         if (start==0) {
            start = m_lastTime;
            a.tx = 0;
         } else {
            // lock out for 5 seconds
            if ((m_lastTime - start) > 5) {
               if (downEvent) {
                  m_mode = kAbyssNewGame;
                  start = 0;
               }
            }
         }  
         
         a.tx += .1 * m_frameDelta * MX_ANG_PI;
         r3_end_block();
         r3_set_view_angles(&sMxVector(-1,0,0),
                            &a,
                            R3_DEFANG);
         r3_start_block();
         
         // Render the stats
         m_StatsRender();
         
         // Render the web
         m_pWeb->Render(colors[kRed]);
         
         // Render the ship
         m_PlayerRender();
         
         // Render the monsters
         m_MonstersRender();

         // draw some text, eh?
         show_game_over();
         
         break;
      }
   }

   r3_end_block();
   r3_end_frame();
#ifdef OUR_OWN_CANVAS
   gr_pop_canvas();
#endif
}

void sAbyss::m_PlayerMove()
{
   short x,y;
   mouse_get_xy(&x,&y);
   // relativize to the point here....
   x-=m_pt.x;
   y-=m_pt.y;
   int spoke = m_pWeb->FindSpoke(x,y,TRUE);
   if (spoke>=0) {
      m_playerSpoke = spoke;
   }
}

void sAbyss::m_PlayerRender()
{
   sMonsterRef player;
   player.m_pMonster = &playerMonster;
   player.m_spoke = m_playerSpoke;
   player.m_x = -.21;

   m_MonsterRender(player);
}


// Render a monster ref
void sAbyss::m_MonsterRender(const sMonsterRef &rRef)
{
   // Maximum size of 16
   sMxVector v[16];
   r3s_point p[16];
   sMonster *pMon = rRef.m_pMonster;

   // has x,y,z but x is zero
   sMxVector c1 = m_pWeb->m_pOuter[rRef.m_spoke];
   sMxVector c2 = m_pWeb->m_pOuter[(rRef.m_spoke+1)%m_pWeb->m_spokes];
   
   int i;
   for (i=0;i<pMon->m_numVerts;++i) {
      v[i].Interpolate(c1,c2,pMon->m_pVerts[i].x);
      v[i].x = rRef.m_x + pMon->m_height*(1.0-pMon->m_pVerts[i].y);
   }
   r3_transform_block(pMon->m_numVerts,p,v);
   for (i=0;i<pMon->m_numEdges;++i) {
      sEdge *pEdge = pMon->m_pEdges+i;
      // set color...
      r3_set_color(colors[pEdge->color]);
      r3_draw_line(p+pEdge->v1,p+pEdge->v2);
   }
}

// Blit onto the screen like a bitmap, uses gr_fix_line
void sAbyss::m_MonsterBitmap(sMonster *pMon,int x,int y,int w,int h)
{
   // Maximum size of 16
   fix xs[16];
   fix ys[16];

   // make all the verts
   int i;
   for (i=0;i<pMon->m_numVerts;++i) {
      xs[i] = fix_from_float(pMon->m_pVerts[i].x * w + x);
      ys[i] = fix_from_float((1.0-pMon->m_pVerts[i].y) * h + y);
   }
   for (i=0;i<pMon->m_numEdges;++i) {
      sEdge *pEdge = pMon->m_pEdges+i;
      gr_set_fcolor(colors[pEdge->color]);
      gr_fix_line(xs[pEdge->v1],ys[pEdge->v1],xs[pEdge->v2],ys[pEdge->v2]);
   }
}

// Go to next level
void sAbyss::NewLevel(int level)
{
   if (level>-1) {
      int spokes = 8.0+(float)(level%7) + level/7;
      if (spokes>14) spokes = 14;

      float radiusVar = .1 + (float)(level%7) *.1;
      if (radiusVar>.5) radiusVar = .5;

      float depth = 8.0 - (level%7)*.25 - RandFloat()*.3;

      if (m_pWeb) delete m_pWeb;
      m_pWeb = new sWeb(.92,spokes,radiusVar,.2,1.7,depth);

      // 3 is MAX
      m_speedScale = 1.1+level*.02;
      if (m_speedScale>2.5) m_speedScale = 2.5;

      m_monsterFreq = .95*pow(.97,level);
      if (m_monsterFreq<.2) m_monsterFreq = .2;

      m_averageMonster = (float)level/8.0 + RandFloat()*.5;
      if (m_averageMonster > 5) m_averageMonster = 5;

      m_level = level;
      m_usedBombThisLevel = FALSE;
   }

   m_secondsRemaining = 12 + m_level;
   if (m_secondsRemaining > 30) m_secondsRemaining = 30;

   // Kill all the shots
   m_ShotClear();

   m_MonsterClear();

   // Reset the player
   m_playerSpoke = 0;

}

void sAbyss::m_ShotClear()
{
   int i;
   for (i=0;i<kMaxShots;++i) {
      m_shots[i].m_spoke = -1;
   }
}

void sAbyss::m_ShotsRender()
{

   r3_set_color(colors[kBlue]);
   int i;
   for (i=0;i<kMaxShots;++i)
   {
      int spoke = m_shots[i].m_spoke;
      if (spoke==-1) continue;

      // get z's
      sMxVector ends[2];
      r3s_point pends[2];
      m_pWeb->SpokeCenter(&ends[0],spoke);
      ends[1] = ends[0];
      ends[0].x = (m_lastTime-m_shots[i].m_launchTime)*kShotSpeed;
      ends[1].x = ends[0].x - kShotLength;

      // clip
      if (ends[0].x > m_pWeb->m_depth) ends[0].x = m_pWeb->m_depth;
      if (ends[1].x < 0) ends[1].x = 0;

      // punt if off end
      if (ends[1].x > m_pWeb->m_depth) {
         m_shots[i].m_spoke = -1;
         continue;
      }

      // Draw the line!
      r3_transform_block(2,pends,ends);
      r3_draw_line(pends,pends+1);
   }
}


// Returns whether or not could actually launch the shot
bool sAbyss::m_ShotAdd(int spoke)
{
   // Find a free slot and add it there
   int i;
   for (i=0;i<kMaxShots;++i)
   {
      if (m_shots[i].m_spoke==-1) {
         m_shots[i].m_spoke = spoke;
         m_shots[i].m_launchTime = m_lastTime;
         return TRUE;
      }
   }
   // no slots free, punt
   return FALSE;
}

void sAbyss::m_MonsterClear()
{
   int i;
   for (i=0;i<kMaxMonsters;++i) {
      m_monsters[i].m_x = -1;
   }
}

// Returns whether or not could actually launch the shot
bool sAbyss::m_MonsterAdd(int spoke,int mon)
{
   // Find a free slot and add it there
   int i;
   for (i=0;i<kMaxMonsters;++i)
   {
      if (m_monsters[i].m_x==-1) {
         m_monsters[i].m_x = m_pWeb->m_depth;
         m_monsters[i].m_spoke = spoke;
         m_monsters[i].m_pMonster = pMonsters[mon];
         m_monsters[i].m_lastFlipX = m_pWeb->m_depth + 2;
         return TRUE;
      }
   }
   // no slots free, punt
   return FALSE;
}


void sAbyss::m_MonstersRender()
{
   int i;
   for (i=0;i<kMaxMonsters;++i)
   {
      if (m_monsters[i].m_x == -1) continue;

      // Render the monster
      m_MonsterRender(m_monsters[i]);
   }
}

void sAbyss::m_MonstersMove()
{
   int i;
   for (i=0;i<kMaxMonsters;++i)
   {
      // punt if bad
      if (m_monsters[i].m_x == -1) continue;
      sMonster *pMonster = m_monsters[i].m_pMonster;


      // flip if far from edge or advance the thing
      float prob;
      if ((m_monsters[i].m_x < 2) || ((m_monsters[i].m_lastFlipX - m_monsters[i].m_x)<2) ) prob=0;
      else prob = 1.0 - exp(-m_frameDelta/pMonster->m_flip);
      if (RandFloat()<prob) {
         int flip = (RandFloat()<.5)?-1:1;
         m_monsters[i].m_spoke = (m_monsters[i].m_spoke+flip+m_pWeb->m_spokes)%m_pWeb->m_spokes;
         m_monsters[i].m_lastFlipX = m_monsters[i].m_x;
      } else {
         // Advance the monster
         float dx = pMonster->m_speed * m_frameDelta * m_speedScale;
         m_monsters[i].m_x -= dx;
      }

      // kill monster if negative
      if (m_monsters[i].m_x < 0) {
         // actually you lose!
         // redo the level
         m_lives--;
         if (m_lives<0) {
            m_mode = kAbyssGameOver;
            store_high_score(m_score,m_level+1);
            m_lives = 0;
         } else {
            m_mode = kAbyssDied;
         }
         continue;
      }

      // kill monster if shot!  
      int j;
      for (j=0;j<kMaxShots;++j)
      {
         if (m_shots[j].m_spoke!=m_monsters[i].m_spoke) continue;

         // get z
         float x = (m_lastTime-m_shots[j].m_launchTime)*kShotSpeed;
         if (x>m_monsters[i].m_x) {
            m_shots[j].m_spoke = -1;
            m_monsters[i].m_x = -1;     
            m_IncrementScore(pMonster->m_score);
            break;
         }
      }

   }

   // Regenerate monsters
   if (m_secondsRemaining<2) return;

   static int m_lastMonsterSpoke = 0;
   float prob = 1.0 - exp(-m_frameDelta/m_monsterFreq);
   if (RandFloat()<prob) {
      int spoke = RandFloat()*m_pWeb->m_spokes;
      if (spoke == m_lastMonsterSpoke) return;
      int monster = floor(RandNorm()*3 + m_averageMonster);
      if (monster<0) monster=0;
      if (monster>(kNumMonsters-1)) monster = (kNumMonsters-1);
      m_MonsterAdd(spoke,monster); 
      m_lastMonsterSpoke = spoke;
   }
}


// Smart bomb the bastards!  And get credit for it
void sAbyss::SmartBomb()
{
   int i;
   if (m_usedBombThisLevel)
      return;   
   m_usedBombThisLevel=TRUE;
   for (i=0;i<kMaxMonsters;++i)
   {
      // punt if bad
      if (m_monsters[i].m_x == -1) continue;
      sMonster *pMonster = m_monsters[i].m_pMonster;
      m_monsters[i].m_x = -1;     
      m_IncrementScore(pMonster->m_score);
   }
}



// Creates num floating monsters
// randomly seeded from startx to endx distributed about some radius
// clears all the existing ones
// sets m_numFloating
void sAbyss::m_FloatingCreate(float startx,float endx,float radius,int num)
{
   int i;
   
   sMonsterFloat *pFloat;
   for (i=0;i<num;++i) 
   {
      pFloat = m_floating+i;
      int monster = floor(RandNorm()*3.4 + m_averageMonster);
      if (monster<0) monster=0;
      if (monster>(kNumMonsters-1)) monster = (kNumMonsters-1);
      pFloat->m_pMonster = pMonsters[monster];
      float radAng = RandFloat()*MX_REAL_2PI;
      float curRadius = radius*(RandFloat()*.9 + .3);
      pFloat->m_pos.x = RandFloat()*(endx-startx)+startx;
      pFloat->m_pos.y = cos(radAng)*curRadius;
      pFloat->m_pos.z = sin(radAng)*curRadius;
      sMxRadVec rvec(RandFloat()*MX_REAL_2PI,RandFloat()*MX_REAL_2PI,RandFloat()*MX_REAL_2PI);
      pFloat->m_ang.Set(rvec);
      
      pFloat->m_width = .9;
   }
   m_numFloating = num;
}


// Render a monster ref
void sAbyss::m_FloatingRender(const sMonsterFloat &rMon)
{
   // Maximum size of 16
   sMxVector v[16];
   r3s_point p[16];
   sMonster *pMon = rMon.m_pMonster;

   r3_end_block();
   r3_start_object_angles((mxs_vector*)&rMon.m_pos,(mxs_angvec*)&rMon.m_ang,R3_DEFANG);
   r3_start_block();

   int i;
   for (i=0;i<pMon->m_numVerts;++i) {
      v[i].y = pMon->m_pVerts[i].x * rMon.m_width;
      v[i].z = 0;
      v[i].x = pMon->m_height*(1.0-pMon->m_pVerts[i].y);
   }

   r3_transform_block(pMon->m_numVerts,p,v);
   for (i=0;i<pMon->m_numEdges;++i) {
      sEdge *pEdge = pMon->m_pEdges+i;
      // set color...
      r3_set_color(colors[pEdge->color]);
      r3_draw_line(p+pEdge->v1,p+pEdge->v2);
   }

   r3_end_block();
   r3_end_object();
   r3_start_block();
}



// Render all the floating monsters and rotate them a bit
void sAbyss::m_FloatingsRender()
{
   int i;
   for (i=0;i<m_numFloating;++i) 
   {
      m_FloatingRender(m_floating[i]);
   }
}


#ifndef STANDALONE

////////////////
// the interface from the minigame player

static sAbyss *pAbyss=NULL;

void AbyssInit(void)
{
   pAbyss = new sAbyss();
   pAbyss->Reset();
}

void AbyssTerm(void)
{
   delete pAbyss;
}

static Rect base_rect = {{ 13, 11},{154,288}};

void AbyssDraw(Point pt)
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
   pAbyss->m_pt=pt;
   pAbyss->Frame();
   gr_pop_canvas();

   // blit it
   gr_bitmap(bmp,pt.x + base_rect.ul.x, pt.y + base_rect.ul.y);

   // clear up our alloc
   gr_free(bmp);
}

void AbyssMouse(Point pt)
{
}

void AbyssRightMouse(Point pt)
{
   if (ShockMiniGameIsHacked())
      pAbyss->SmartBomb();
}

#endif
