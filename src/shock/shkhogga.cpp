/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkhogga.cpp,v 1.4 2000/02/19 13:25:22 toml Exp $

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

static Rect base_rect = {{13,11},{153,287}};  // bigger blit or something?
static Rect game_rect = {{13,11},{152,270}};

typedef int eHoggerState;
enum eHoggerState_ {kHoggerNone, kHoggerPlaying, kHoggerDeath, kHoggerWin, kHoggerScores,};
static eHoggerState g_hoggerState;
static tSimTime g_hoggerStateChangeTime;

const tSimTime kHoggerDeathTime = 2000;
const tSimTime kHoggerWinTime = 2000;

const int kHoggerColumnWidth = 16;
const int kHoggerRowHeight = 16;

static float g_hoggerDifficulty;
const float kHoggerDifficultyAdd = 0.05;

typedef int eHoggerObjs;
enum eHoggerObjs_ 
{
   kHoggerHog = 0,
   kHoggerSplat,
   kHoggerDance,
   kHoggerTruck,
   kHoggerRacer,
   kHoggerDozer,
   kHoggerNumObjs,
};

static int g_hoggerObjSpacing[kHoggerNumObjs] = {0, 0, 0, 70, 110, 85,};
static float g_hoggerObjSpeed[kHoggerNumObjs] = {0, 0, 0, 0.01, 0.03, 0.02,};

const char *g_hoggerArt1[] = {"Hog", "HogSp", "HogDan", "HogTrD", "HogRaD", "HogDoD", "Hog", "HogSp", "HogDan", "HogTrU", "HogRaU", "HogDoU", };
const char *g_hoggerArt2[] = {"Hog2", "HogSp2", "HogDan2", "HogTrD2", "HogRaD2", "HogDoD2", "Hog2", "HogSp2", "HogDan2", "HogTrU2", "HogRaU2", "HogDoU2", };

static IRes *g_pHoggerBack = NULL;
static IRes *g_pHoggerIcons1[2*kHoggerNumObjs];
static IRes *g_pHoggerIcons2[2*kHoggerNumObjs];
static IRes **g_ppHoggerIcons;

static Point g_hogPt;

const int kHoggerRows = 16;
const int kHoggerColumns = 8;

static int g_hogRow;
static int g_hogColumn;

static tSimTime g_hoggerLastFrameTime;

static tSimTime g_hoggerLastAnimTime;
const int kHoggerAnimSpeed = 500;

const int kHoggerScoreX = 10;
const int kHoggerMsgX = 55;
const int kHoggerLivesX = 90;

static int g_hoggerWins;
static int g_hoggerLives;

void HoggerReset(void);
void HoggerNewGame(void);
void HoggerSetState(eHoggerState state);

#define GetNamedString(buf,name)    ShockStringFetch(buf,sizeof(buf),name,"minigame")

// High scores
sFileVarDesc gHoggerScoreDesc =
{
   kCampaignVar,
   "HoggerScore",
   "Hogger High Scores",
   FILEVAR_TYPE(sMiniGameScores),
   {1,0},
   {1,0},
   "shock",
};

cFileVar<sMiniGameScores,&gHoggerScoreDesc> gHoggerScoreTable;

///////////////

int HoggerHogX(void)
{
   return game_rect.ul.x + g_hogColumn*kHoggerColumnWidth;
}

int HoggerHogY(void)
{
   return game_rect.ul.y + g_hogRow*kHoggerRowHeight;
}

/////////////////////////////////

class cHoggerColumn
{
public:
   eHoggerObjs m_objType;
   int m_offset;
   int m_spacing;
   int m_direction;
   int m_column;
   float m_subPixel;

   void Init(int column, int avoidObjType);
   void Draw(Point pt);
   BOOL InBounds(int i);
   void DrawObj(Point pt, int i);
   void Update(tSimTime delta);

   int YOffset(int i);
};

////////////////////////////////////////////

void cHoggerColumn::Init(int c, int avoidObjType)
{
   m_column = c;
   m_objType = Rand()%(kHoggerNumObjs-kHoggerTruck)+kHoggerTruck;
   while (m_objType == avoidObjType)
      m_objType = Rand()%(kHoggerNumObjs-kHoggerTruck)+kHoggerTruck;
   m_offset = Rand()%kHoggerRowHeight;
   m_direction = Rand()%2;
   m_subPixel = 0;
}

////////////////////////////////////////////

void cHoggerColumn::Draw(Point pt)
{
   int i = 0;

   while (InBounds(i))
   {
      DrawObj(pt, i);
      ++i;
   }
}

////////////////////////////////////////////

int cHoggerColumn::YOffset(int i)
{
   if (m_direction == 0)
      return m_offset + game_rect.ul.y + i*g_hoggerObjSpacing[m_objType];
   else
      return -m_offset + game_rect.lr.y -kHoggerRowHeight - i*g_hoggerObjSpacing[m_objType];
}

////////////////////////////////////////////

BOOL cHoggerColumn::InBounds(int i)
{
   if (m_direction == 0)
      return ((YOffset(i)+kHoggerRowHeight)<game_rect.lr.y);
   else
      return (YOffset(i)>game_rect.ul.y);
}

////////////////////////////////////////////

void cHoggerColumn::DrawObj(Point pt, int i)
{
   Point drawpt;

   // draw background
   drawpt.x = pt.x + game_rect.ul.x + m_column*kHoggerColumnWidth;
   drawpt.y = pt.y + YOffset(i);
   DrawByHandle(g_ppHoggerIcons[m_objType+m_direction*kHoggerNumObjs], drawpt);
}

////////////////////////////////////////////

void cHoggerColumn::Update(tSimTime delta)
{
   // move
   float m_move = g_hoggerObjSpeed[m_objType]*delta*g_hoggerDifficulty;
   m_offset += tSimTime(m_move+m_subPixel);
   m_subPixel = (m_move+m_subPixel)-float(int(m_move+m_subPixel));
   m_offset %= g_hoggerObjSpacing[m_objType];

   // test for collision
   if ((g_hoggerState == kHoggerPlaying) && (m_column == g_hogColumn))
   {
      int i = 0;
      
      while (InBounds(i))
      {
         int objY = YOffset(i);
         int hogY = HoggerHogY();
         if (((objY<hogY) && (objY+kHoggerRowHeight)>hogY)
            || ((objY>hogY) && (objY<hogY+kHoggerRowHeight)))
         {
            HoggerSetState(kHoggerDeath);
            break;
         }
         ++i;
      }
   }
}

//////////////////////////////

cHoggerColumn g_hoggerColumns[kHoggerColumns-2];

//////////////////////////////

void HoggerAddScore(int score)
{
   char form_buf[64], buf[64];
   // format string, pass it in
   GetNamedString(form_buf,"HoggerScore");
   sprintf(buf,form_buf,score);
   ShockMiniGameAddHighScore(&gHoggerScoreTable,score,buf);
}

//////////////////////////////

void HoggerSetState(eHoggerState state)
{
   switch (state)
   {
   case kHoggerPlaying:
      HoggerReset();
      break;
   case kHoggerWin:
      ++g_hoggerWins;
      g_hoggerDifficulty += kHoggerDifficultyAdd;
      break;
   case kHoggerScores:
      HoggerAddScore(g_hoggerWins);
      break;
   }

   g_hoggerState = state;
   g_hoggerStateChangeTime = GetSimTime();
}

//////////////////////////////

void HoggerFrame(tSimTime delta)
{
   switch (g_hoggerState)
   {
   case kHoggerDeath:
      if ((GetSimTime()-g_hoggerStateChangeTime)>kHoggerDeathTime)
      {
         if (g_hoggerLives>0)
         {
            --g_hoggerLives;
            HoggerSetState(kHoggerPlaying);
         }
         else
            HoggerSetState(kHoggerScores);
      }
      break;
   case kHoggerWin:
      if ((GetSimTime()-g_hoggerStateChangeTime)>kHoggerWinTime)
         HoggerSetState(kHoggerPlaying);
      break;
   }

   for (int c=0; c<kHoggerColumns-2; c++)
      g_hoggerColumns[c].Update(delta);

   if ((g_hoggerState == kHoggerPlaying) && (g_hogColumn >= kHoggerColumns-1))
      HoggerSetState(kHoggerWin);
}

///////////////
// draw

void HoggerDrawObjs(Point pt)
{
   int i;
   Point drawpt;

   if ((GetSimTime()-g_hoggerLastAnimTime)>kHoggerAnimSpeed)
   {
      g_hoggerLastAnimTime = GetSimTime();
      if (g_ppHoggerIcons == g_pHoggerIcons1)
         g_ppHoggerIcons = g_pHoggerIcons2;
      else
         g_ppHoggerIcons = g_pHoggerIcons1;
   }

   switch (g_hoggerState)
   {
   case kHoggerScores:
      for (i=0; i<3; i++)
      {
         drawpt.x=pt.x+32+i*32;  
         drawpt.y=pt.y+150;
         DrawByHandle(g_ppHoggerIcons[kHoggerDance],drawpt);
      }
      break;

   default:
      // draw hog
      drawpt.x = pt.x + HoggerHogX();
      drawpt.y = pt.y + HoggerHogY();
      if (g_hoggerState == kHoggerDeath)
         DrawByHandle(g_ppHoggerIcons[kHoggerSplat],drawpt);
      else
         DrawByHandle(g_ppHoggerIcons[kHoggerHog],drawpt);
      
      // draw objs
      for (int c=0; c<kHoggerColumns-2; c++)
         g_hoggerColumns[c].Draw(pt);
      break;
   }
}

///////////////
// draw

void HoggerDraw(Point pt)
{
   Point drawpt, ipt=pt;
   int i;

   ShockMiniGameSetupCanvas(ipt,base_rect,-1);
   pt.x=pt.y=0;

   HoggerFrame(GetSimTime()-g_hoggerLastFrameTime);
   g_hoggerLastFrameTime = GetSimTime();

   // draw background
   drawpt.x = pt.x;
   drawpt.y = pt.y;
   DrawByHandle(g_pHoggerBack,drawpt);

   HoggerDrawObjs(pt);

   switch (g_hoggerState)
   {
   case kHoggerScores:
      drawpt.x=pt.x+17;  // or something
      drawpt.y=pt.y+10;
      ShockMiniGameDisplayHighScores(&gHoggerScoreTable,drawpt.x,drawpt.y);
      drawpt.x=pt.x+21;
      drawpt.y=pt.y+(base_rect.lr.y-base_rect.ul.y)-21;
      gr_font_string(gShockFont, gMiniGameScoreMsg, drawpt.x, drawpt.y);
      break;

   default:
   
      // text
      char buf[64];
      char draw_buf[64];
      GetNamedString(buf,"HoggerScore");
      sprintf(draw_buf, buf, g_hoggerWins);
      gr_font_string(gShockFont, draw_buf, pt.x+kHoggerScoreX, pt.y + kHoggerRows*kHoggerRowHeight);
      
      if (g_hoggerState == kHoggerDeath)
      {
         GetNamedString(buf,"HoggerLose");
         gr_font_string(gShockFont, buf, pt.x+kHoggerMsgX, pt.y + kHoggerRows*kHoggerRowHeight);
      }
      else if (g_hoggerState == kHoggerWin)
      {
         GetNamedString(buf,"HoggerWin");
         gr_font_string(gShockFont, buf, pt.x+kHoggerMsgX, pt.y + kHoggerRows*kHoggerRowHeight);
      }
      
      // lives
      drawpt.y = pt.y + kHoggerRows*kHoggerRowHeight;
      for (i=0; i<g_hoggerLives; i++)
      {
         drawpt.x = pt.x+kHoggerLivesX+i*kHoggerColumnWidth;
         DrawByHandle(g_ppHoggerIcons[kHoggerHog],drawpt);
      }
      break;
   }

   ShockMiniGameBlitCanvas(ipt,base_rect);
}

///////////////
// mouse

void HoggerMouse(Point pt)
{
   int column, row;

   switch (g_hoggerState)
   {
   case kHoggerPlaying:
      column = ((pt.x-base_rect.ul.x)/kHoggerColumnWidth)-g_hogColumn;
      row = ((pt.y-base_rect.ul.y)/kHoggerRowHeight)-g_hogRow;
      
      if (abs(column)>abs(row))
      {
         if ((column>0) && (g_hogColumn<kHoggerColumns-1))
            g_hogColumn++;
         else if ((column<0) && (g_hogColumn>0))
            g_hogColumn--;
      }
      else
      {
         if ((row>0) && (g_hogRow<kHoggerRows-1))
            g_hogRow++;
         else if ((row<0) && (g_hogRow>0))
            g_hogRow--;
      }
      break;

   case kHoggerScores:
      HoggerNewGame();
      HoggerSetState(kHoggerPlaying);
      break;
   }
}

///////////////

void HoggerReset(void)
{
   g_hogColumn = 0;
   g_hogRow = 8;
   for (int c=0; c<kHoggerColumns-2; c++)
   {
      if (c>0)
         g_hoggerColumns[c].Init(c+1, g_hoggerColumns[c-1].m_objType);
      else
         g_hoggerColumns[c].Init(c+1, -1);
   }
   g_hoggerLastFrameTime = GetSimTime();
}

///////////////

void HoggerNewGame(void)
{
   g_hoggerWins = 0;
   g_hoggerDifficulty = 1;
   if (ShockMiniGameIsHacked())
      g_hoggerLives = 3;
   else
      g_hoggerLives = 2;
}

///////////////
// init and term

void HoggerInit(void)
{
   g_hoggerState = kHoggerNone;
   g_pHoggerBack = LoadPCX("hogback");
   for (int i=0; i<2*kHoggerNumObjs; i++)
      g_pHoggerIcons1[i] = LoadPCX(g_hoggerArt1[i]);
   for (i=0; i<2*kHoggerNumObjs; i++)
      g_pHoggerIcons2[i] = LoadPCX(g_hoggerArt2[i]);
   HoggerNewGame();
   HoggerSetState(kHoggerPlaying);
}

void HoggerTerm(void)
{
   SafeFreeHnd(&g_pHoggerBack);
   for (int i=0; i<2*kHoggerNumObjs; i++)
      SafeFreeHnd(&g_pHoggerIcons1[i]);
   for (i=0; i<2*kHoggerNumObjs; i++)
      SafeFreeHnd(&g_pHoggerIcons2[i]);
}
