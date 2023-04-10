/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkbacon.cpp,v 1.6 2000/02/19 12:36:36 toml Exp $

#include <math.h>
#include <2d.h>
#include <timer.h>
#include <resapi.h>
#include <appagg.h>
#include <minmax.h>
#include <mprintf.h>
#include <rand.h>
#include <random.h>
#include <config.h>

#include <playrobj.h>
#include <simtime.h>
#include <schema.h>

#include <shkovrly.h>
#include <shkutils.h>
#include <shkovcst.h>
#include <shkmfddm.h>
#include <shkobjst.h>
#include <shkprop.h>
#include <shkiftul.h>
#include <shkplayr.h>
#include <shkminig.h>

#include <filevar.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
}

////////////////
// our layout

static Rect base_rect = {{ 13, 11},{152,286}};
#define GAME_CENTER_X ((152-13)/2)
// ???
#define PADDLE_MIN_X  (PADDLE_X_SIZE/2)
#define PADDLE_MAX_X  ((152-13)-(PADDLE_X_SIZE/2))

////////////////
// overall game state

typedef enum eKBGameMode { kGameInit, kGamePlay, kGameReset, kGameScores };

static eKBGameMode    gCurGameMode = kGameInit;
static int            gLastModeChange = 0;

static int            gResetTimer = 0;
#define RESET_TIME   (1000)

////////////////
// game state

// "paddle" state
static float          gPaddleX;
static float          gPaddleSpeed;
static int            gPaddleCount;
static int            gPaddleCaught;
#define MAX_PADDLES   (3)
#define PADDLE_Y      (200)
#define PADDLE_X_SIZE (24)   // maybe should go down to 18?
#define PADDLE_Y_SIZE (8)
#define PADDLE_Y_SPC  (6)

// "bomber" state
static float          gBomberX;
static float          gBomberSpeed;
static float          gBombDropVelocity;
static int            gBombDropRate;
static int            gNextBombTimer;
static int            gBombsDropped;
#define BOMBER_Y      (32)             // this is all totally made up
#define BOMB_SRC_Y    (BOMBER_Y+24) 
#define INIT_BOMB_VEL (50.0)
#define INC_BOMB_VEL  (1.0)
#define INIT_BOMB_RATE (800)
#define INC_BOMB_RATE (-4.0)
#define MIN_BOMB_RATE (175)
#define BOMBER_X_SIZE (32)
#define BOMBER_MIN_X  (PADDLE_MIN_X)
#define BOMBER_MAX_X  (PADDLE_MAX_X)

// "falling bacon" state
#define MAX_BACON     (32)
typedef struct {
   float  x,y;
   float  spd;
} sFallingBacon;
static sFallingBacon gBacon[MAX_BACON];

//////////////////////////////
// high scores

sFileVarDesc gKaBaconScoreDesc =
{
   kCampaignVar,
   "BaconScr",
   "KaBacon High Scores",
   FILEVAR_TYPE(sMiniGameScores),
   {1,0},
   {1,0},
   "shock",
};

cFileVar<sMiniGameScores,&gKaBaconScoreDesc> gKaBaconScoreTable;

////////////////
// misc string state
#define GetNamedString(buf,name)    ShockStringFetch(buf,sizeof(buf),name,"minigame")

///////////////
// the "sim"

void _check_n_add_score(int score)
{
   char form_buf[64], buf[64];
   GetNamedString(form_buf,"kb_score_form");  // format string, pass it in
   sprintf(buf,form_buf,score);
   ShockMiniGameAddHighScore(&gKaBaconScoreTable,score,buf);
}

void _lose_a_paddle(void)
{
   if (--gPaddleCount==0)
   {
      gCurGameMode=kGameScores;
      _check_n_add_score(gPaddleCaught);
   }
   else
      gCurGameMode=kGameReset;
}

void _update_bacon(sFallingBacon *pBacon, float dt)
{
   int y_min=PADDLE_Y+(MAX_PADDLES-gPaddleCount)*(PADDLE_Y_SIZE+PADDLE_Y_SPC);
   float new_y=pBacon->y+pBacon->spd*dt;
   if (new_y>y_min)
   {
      int y_max=PADDLE_Y+(MAX_PADDLES*PADDLE_Y_SIZE)+((MAX_PADDLES-1)*PADDLE_Y_SPC);
      // @TBD: bug - should intersect the line old_y-new_y to the paddle rects...
      if (gCurGameMode!=kGamePlay)
      {
         if (new_y>y_max)
            pBacon->spd=0;
      }
      else if (new_y>y_max)
      {
#ifndef SHIP         
         if (!config_is_defined("kabacon_perfect"))
#endif
            _lose_a_paddle();  // you lose, that bomb fell off the bottom
         pBacon->spd=0;         
      }
      else if ((pBacon->x>gPaddleX-(PADDLE_X_SIZE/2))&&
               (pBacon->x<gPaddleX+(PADDLE_X_SIZE/2)))
      {
         pBacon->spd=0;
         gPaddleCaught++;
      }
   }
   // we dont care about the y field if we deleted ourselves (set spd=0)
   // so just always set it...
   pBacon->y=new_y;
}

int _find_free_bacon(void)
{
   for (int i=0; i<MAX_BACON; i++)
      if (gBacon[i].spd==0)
         return i;
   return -1;
}

void _update_bomber(float dt)
{
   gNextBombTimer+=GetSimFrameTime();
   while (gNextBombTimer>gBombDropRate)
   {
      gNextBombTimer-=gBombDropRate;
      int idx=_find_free_bacon();
      if (idx!=-1)
      {  // go create the bomb, eh
         gBacon[idx].x   = gBomberX;
         gBacon[idx].y   = BOMB_SRC_Y;
         gBacon[idx].spd = gBombDropVelocity;

         ++gBombsDropped;          // get faster twice as fast at first... smoothly slow back to 1.0
         float speed_mul=(gBombsDropped<30)?2.0:
                         (gBombsDropped>60)?1.0:2.0-((gBombsDropped-30)/30.0);
         // place it with partial frame update, ideally, but oh well         
         gBombDropVelocity+=INC_BOMB_VEL*speed_mul;
         if (gBombDropRate>MIN_BOMB_RATE)            
            gBombDropRate+=INC_BOMB_RATE*speed_mul;

         
      }
   }
   // now move the bomber.... [@TBD: THIS CODE SUCKS]
   if ((Rand()&0x1f)==0)
      switch (Rand()&0x3)
      {  // occasionally do discontinuous stuff
         case 0: gBomberSpeed=-gBomberSpeed; break;
         case 1: gBomberSpeed*=3; break;
         case 2: gBomberSpeed=0; break;
         case 3: gBomberSpeed/=2; break;
      }
   if (Rand()&0xf)
      gBomberSpeed+=(RandFloat()-0.5)*gBombDropVelocity;
   gBomberX+=gBomberSpeed*dt;
   if (gBomberX<BOMBER_MIN_X)
    { gBomberX=BOMBER_MIN_X; gBomberSpeed= 5.0; }
   else if (gBomberX>BOMBER_MAX_X)
    { gBomberX=BOMBER_MAX_X; gBomberSpeed=-5.0; }
}

void _update_paddle(float dt)
{
   gPaddleX+=gPaddleSpeed*dt;
   if (gPaddleX<PADDLE_MIN_X)
      gPaddleX=PADDLE_MIN_X;
   else if (gPaddleX>PADDLE_MAX_X)
      gPaddleX=PADDLE_MAX_X;
}

//////////////////////////////
// reset/update game state

void KaBaconStartGame(void)
{
   gCurGameMode=kGamePlay;
   gPaddleX=gBomberX=GAME_CENTER_X;
   gPaddleSpeed=gBomberSpeed=0.0;
   for (int i=0; i<MAX_BACON; i++)
      gBacon[i].spd=0;
   gPaddleCount=MAX_PADDLES;
   gBombDropVelocity=INIT_BOMB_VEL;
   gNextBombTimer=gPaddleCaught=gBombsDropped=0;
   gBombDropRate=INIT_BOMB_RATE;
}

// youve losta paddle
void KaBaconNewPaddle(void)
{
   for (int i=0; i<MAX_BACON; i++)
      gBacon[i].spd=0.0;
   gBombDropRate     -= 8*INC_BOMB_RATE;
   gBombDropVelocity -= 8*INC_BOMB_VEL;
   gNextBombTimer     = 0;
}

//////////////////////////////
// the sim
void KaBaconRunSim(void)
{
   float dt=GetSimFrameTime()/1000.0;
   if (gCurGameMode==kGameReset)
   {
      KaBaconNewPaddle();
      gCurGameMode=kGamePlay;
   }
   for (int i=0; i<MAX_BACON; i++)
      if (gBacon[i].spd!=0.0)
         _update_bacon(&gBacon[i],dt);
   if (gCurGameMode==kGamePlay)
   {
      _update_bomber(dt);
      _update_paddle(dt);
   }
}

void _get_paddle_input(Point pt)
{
   short x,y;
   float dx;
   mouse_get_xy(&x,&y);
   // cant detect out of window, i dont think....
   //   mprintf("Saw %d %d, paddle now %g, pt %d %d\n",x,y,gPaddleX,pt.x,pt.y);
   x-=pt.x;
   x-=base_rect.ul.x;  // do we need to do this?
   dx=x-gPaddleX;
   gPaddleSpeed=dx*5.5;
}

///////////////
// draw

#define MULTIPADDLE
#ifdef  MULTIPADDLE
IRes *gResPaddles[3];
#define GetPaddle(which) (gResPaddles[which])
#else
IRes *gResPaddle;
#define GetPaddle(which) (gResPaddle)
#endif
IRes *gResBomber;
IRes *gResBacon;

void KaBaconDraw(Point pt)
{
   Point ipt=pt;

   ShockMiniGameSetupCanvas(ipt,base_rect,0);
   pt.x=pt.y=0;
   
   // first, get the input and run the sim
   _get_paddle_input(ipt);  // since mouse needs to look relative to rect
   KaBaconRunSim();

   // now draw
   Point drawpt;

   // bomber
   drawpt.x=pt.x+(int)gBomberX;
   drawpt.y=pt.y+BOMBER_Y;
   DrawByHandleCenter(gResBomber,drawpt);

   // paddle
   if (gCurGameMode==kGamePlay||gCurGameMode==kGameReset)
   {
      drawpt.x=pt.x+(int)gPaddleX;
      drawpt.y=pt.y+PADDLE_Y;
      for (int i=0; i<MAX_PADDLES; i++)
      {
         if (((MAX_PADDLES-1)-i)<gPaddleCount)
            DrawByHandleCenter(GetPaddle(i),drawpt);
         drawpt.y+=PADDLE_Y_SIZE+PADDLE_Y_SPC;
      }
   }
   
   // bacon
   for (int i=0; i<MAX_BACON; i++)
      if (gBacon[i].spd!=0)
      {
         drawpt.x=pt.x+(int)gBacon[i].x;
         drawpt.y=pt.y+(int)gBacon[i].y;
         DrawByHandleCenter(gResBacon,drawpt);
      }

   drawpt.x=pt.x+1;
   drawpt.y=pt.y+1;
   char form_buf[64], msg_buf[64];
   GetNamedString(form_buf,"kb_score");
   sprintf(msg_buf,form_buf,gPaddleCaught);
   gr_font_string(gShockFont,msg_buf,drawpt.x,drawpt.y);

   if (gCurGameMode==kGameScores)
   {
      drawpt.x=pt.x+17;  // or something
      drawpt.y=pt.y+BOMB_SRC_Y+10;
      ShockMiniGameDisplayHighScores(&gKaBaconScoreTable,drawpt.x,drawpt.y);
      drawpt.x=pt.x+21;
      drawpt.y=pt.y+(base_rect.lr.y-base_rect.ul.y)-21;
      gr_font_string(gShockFont, gMiniGameScoreMsg, drawpt.x, drawpt.y);

      // goofy game over thing...
      if (gLastModeChange+3000<GetSimTime())
      {
         int y_off=(GetSimTime()-3000-gLastModeChange)/8;
#define BOUNCE_HT (220)
         y_off%=BOUNCE_HT*2;
         if (y_off>BOUNCE_HT) y_off=BOUNCE_HT*2-y_off;
         drawpt.y=pt.y+1+y_off;
         drawpt.x=pt.x+1+GAME_CENTER_X-(GAME_CENTER_X/3); // argh, hack hack hack
         GetNamedString(msg_buf,"kb_gameover");
         gr_font_string(gShockFont, msg_buf, drawpt.x, drawpt.y);
      }
   }

   ShockMiniGameBlitCanvas(ipt,base_rect);   
}

///////////////
// mouse

void KaBaconMouse(Point pt)
{
   if (gCurGameMode==kGamePlay)
      if (ShockMiniGameIsHacked())
         if ((abs(pt.x-base_rect.ul.x-(int)gBomberX)<16)&&
             (abs(pt.y-base_rect.ul.y-(int)BOMBER_Y)<16))
            if (gPaddleCount<MAX_PADDLES)
               gPaddleCount++;
   if ((gCurGameMode!=kGamePlay)&&(gCurGameMode!=kGameReset))
      if (gLastModeChange+3000<GetSimTime())
         KaBaconStartGame();
}

///////////////
// init and term

void KaBaconInit(void)
{
#ifdef MULTIPADDLE
   gResPaddles[0]=LoadPCX("kbpad_0");
   gResPaddles[1]=LoadPCX("kbpad_1");
   gResPaddles[2]=LoadPCX("kbpad_2");
#else
   gResPaddle=LoadPCX("kbpaddle");
#endif
   gResBomber=LoadPCX("kbbomber");
   gResBacon=LoadPCX("kbbacon");
   KaBaconStartGame();
}

void KaBaconTerm(void)
{
#ifdef MULTIPADDLE
   SafeFreeHnd(&gResPaddles[0]);
   SafeFreeHnd(&gResPaddles[1]);
   SafeFreeHnd(&gResPaddles[2]);
#else
   SafeFreeHnd(&gResPaddle);
#endif
   SafeFreeHnd(&gResBomber);
   SafeFreeHnd(&gResBacon);   
}
