/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkswine.cpp,v 1.4 2000/02/19 13:26:18 toml Exp $

// alarm icon
#include <2d.h>

#include <resapi.h>
#include <appagg.h>
#include <minmax.h>
#include <mprintf.h>
#include <rand.h>

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

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
}

//--------------------------------------------------------------------------------------
// SWINE-KEEPER
// Okay, here is the idea -- you have a field.  There are a bunch of areas growing corn
// on this field.  You have to find the areas without actually stepping on them, since that
// would crush the corn.  You can find it because each corn area attracts swine to nearby
// areas.  The number of swine in a square indicates how many ears of corn are adjacent
// including diagonals.
// 
// This is, of course, completely unlike Minesweeper.
//--------------------------------------------------------------------------------------

typedef enum eSwineGameMode { kGameNormal, kGameLost, kGameWon, kGameFlagPlace };

static Rect base_rect = {{13,11},{152,286}};
static Rect flag_rect = {{112,0},{139,25}};

#define TEXT_W 110
#define TEXT_X 3
#define TEXT_Y 4

#define FLAG_X 113
#define FLAG_Y 1

static int game_mode;
static int gFlagCount;

static char gSwineText[255];

#define FIELD_X   6
#define FIELD_Y   10

static int gCorn[FIELD_X][FIELD_Y];
// 0 = unrevealed
// 1 = revealed
// 2 = flag placed
static int gRevealed[FIELD_X][FIELD_Y];

#define NUM_CORN_STALKS 12

static void PlaceCorn()
{
   int i;
   int x,y;

   // clear the field
   for (x=0; x < FIELD_X; x++)
   {
      for (y=0; y < FIELD_Y; y++)
      {
         gCorn[x][y] = 0;
         gRevealed[x][y] = 0;
      }
   }

   // randomly place some corn
   for (i=0; i < NUM_CORN_STALKS; i++)
   {
      // should bulletproof against maximum fill case?
      do
      {
         x = Rand() % FIELD_X;
         y = Rand() % FIELD_Y;
      } 
      while (gCorn[x][y] == 1);
      gCorn[x][y] = 1;
   }
   
   gFlagCount = NUM_CORN_STALKS;
}

// returns 1 if there is corn here
// 0 if not, or off edge of field
static int CornVal(int x, int y)
{
   if ((x < 0) || (x >= FIELD_X) || (y < 0) || (y >= FIELD_Y))
      return(0);
   
   return(gCorn[x][y]);
}

// how many swine are in this square?
static int CountSwine(int x, int y)
{
   int retval = 0;

   retval = retval + CornVal(x-1,y-1);
   retval = retval + CornVal(x,y-1);
   retval = retval + CornVal(x+1,y-1);

   retval = retval + CornVal(x-1,y);
   retval = retval + CornVal(x+1,y);

   retval = retval + CornVal(x-1,y+1);
   retval = retval + CornVal(x,y+1);
   retval = retval + CornVal(x+1,y+1);

   AssertMsg2(retval <= 8,"Too many swine at %d, %d",x,y);

   return(retval);
}

static void RevealTile(int x, int y)
{
   if ((x < 0) || (y < 0) || (x >= FIELD_X) || (y >= FIELD_Y))
      return;

   if (gRevealed[x][y] != 0)
      return;

   gRevealed[x][y] = 1;

   if (gCorn[x][y])
   {
      // you lose!
      ShockStringFetch(gSwineText, sizeof(gSwineText), "SwineLose", "minigame");
      gr_font_string_wrap(gShockFont,gSwineText,TEXT_W);
      game_mode = kGameLost;
      return;
   }

   // if a zero-square, reveal all adjacent zero-squares
   if (CountSwine(x,y) == 0)
   {
      RevealTile(x-1,y-1);
      RevealTile(x,y-1);
      RevealTile(x+1,y-1);

      RevealTile(x-1,y);
      RevealTile(x+1,y);

      RevealTile(x-1,y+1);
      RevealTile(x,y+1);
      RevealTile(x+1,y+1);
   }
}

// do we have a flag on all of the corn squares?
static void CheckVictory()
{
   // clearly no victory if not all flags placed.
   if (gFlagCount != 0)
      return;

   int x,y;
   BOOL win = TRUE;

   for (x=0; win && (x < FIELD_X); x++)
   {
      for (y=0; win && (y < FIELD_Y); y++)
      {
         if (gCorn[x][y] && (gRevealed[x][y] != 2))
         {
            win = FALSE;
         }
         if ((gRevealed[x][y] == 2) && (!gCorn[x][y]))
            win = FALSE;
      }
   }

   if (win)
   {
      ShockStringFetch(gSwineText, sizeof(gSwineText), "SwineWin", "minigame");
      gr_font_string_wrap(gShockFont,gSwineText,TEXT_W);
      game_mode = kGameWon;
   }
}

IRes *gSwineHnd[9];
IRes *gUnexploredHnd;
IRes *gCornHnd;
IRes *gFlagHnd;
IRes *gBackHnd;

#define TILE_X    1
#define TILE_Y    41 
#define TILE_DX   23 
#define TILE_DY   23

void SwineInit()
{
   PlaceCorn();

   int i;
   char temp[255];
   for (i=0; i < 9; i++)
   {
      sprintf(temp,"swine%d",i);
      gSwineHnd[i] = LoadPCX(temp);
   }

   gUnexploredHnd = LoadPCX("swineune");
   gCornHnd = LoadPCX("swinecrn");
   gFlagHnd = LoadPCX("swineflg");

   gBackHnd = LoadPCX("swinebck");

   game_mode = kGameNormal;
   ShockStringFetch(gSwineText,sizeof(gSwineText),"SwineStart","minigame");
   gr_font_string_wrap(gShockFont,gSwineText,TEXT_W);
}

void SwineTerm()
{
   int i;
   for (i=0 ; i < 9; i++)
      SafeFreeHnd(&gSwineHnd[i]);

   SafeFreeHnd(&gUnexploredHnd);
   SafeFreeHnd(&gCornHnd);
   SafeFreeHnd(&gFlagHnd);

   SafeFreeHnd(&gBackHnd);
}

void SwineDraw(Point pt)
{
   int fx, fy;
   Point drawpt;

   drawpt.x = pt.x + base_rect.ul.x;
   drawpt.y = pt.y + base_rect.ul.y;
   DrawByHandle(gBackHnd,drawpt);

   for (fx = 0; fx < FIELD_X; fx++)
   {
      for (fy = 0; fy < FIELD_Y; fy++)
      {
         drawpt.x = TILE_X + (fx * TILE_DX) + pt.x + base_rect.ul.x;
         drawpt.y = TILE_Y + (fy * TILE_DY) + pt.y + base_rect.ul.y;

         if ((game_mode == kGameWon) || (game_mode == kGameLost))
         {
            if (gCorn[fx][fy])
               DrawByHandle(gCornHnd,drawpt);
            else if (gRevealed[fx][fy] == 2)
               DrawByHandle(gFlagHnd,drawpt);
            else
            {
               // boy, this should probably be cached off
               int n = CountSwine(fx,fy);
               DrawByHandle(gSwineHnd[n],drawpt);
            }
         }
         else
         {
            switch (gRevealed[fx][fy])
            {
            case 0:
               DrawByHandle(gUnexploredHnd,drawpt);
               break;
            case 1:
               if (gCorn[fx][fy])
                  DrawByHandle(gCornHnd,drawpt);
               else
               {
                  // boy, this should probably be cached off
                  int n = CountSwine(fx,fy);
                  DrawByHandle(gSwineHnd[n],drawpt);
               }
               break;
            case 2:
               DrawByHandle(gFlagHnd,drawpt);
               break;
            }
         }
      }
   }

   gr_font_string(gShockFont, gSwineText, base_rect.ul.x + pt.x + TEXT_X, base_rect.ul.y + pt.y + TEXT_Y);

   if (game_mode == kGameFlagPlace)
   {
      drawpt.x = FLAG_X + base_rect.ul.x + pt.x;
      drawpt.y = FLAG_Y + base_rect.ul.y + pt.y;
      DrawByHandle(gFlagHnd,drawpt);
   }

   char temp[255];
   sprintf(temp,"%d",gFlagCount);
   gr_font_string(gShockFont, temp, pt.x + FLAG_X + 16 + base_rect.ul.x, pt.y + FLAG_Y + 13 + base_rect.ul.y);

}

void SwineMouse(Point pt)
{
   int fx, fy;
   float dx, dy;
   pt.x = pt.x - base_rect.ul.x;
   pt.y = pt.y - base_rect.ul.y;

   dx = pt.x - TILE_X;
   dy = pt.y - TILE_Y;
   if (dx < 0)
      fx = -1;
   else
      fx = float(dx / (float)TILE_DX);

   if (dy < 0)
      fy = -1;
   else
      fy = float(dy / (float)TILE_DY);

//   mprintf("clicking on %d, %d\n",fx,fy);
   if ((game_mode == kGameLost) || (game_mode == kGameWon))
   {
      SetGame(kGameSwine); 
      return;
   }

   if (RectTestPt(&flag_rect,pt))
   {
      if (game_mode == kGameFlagPlace)
         game_mode = kGameNormal;
      else
         game_mode = kGameFlagPlace;
   }

   if (game_mode == kGameNormal)
   {
      if ((fx < 0) || (fy < 0) || (fx >= FIELD_X) || (fy >= FIELD_Y))
         return;

      RevealTile(fx,fy);
   }
   else if (game_mode == kGameFlagPlace)
   {
      if ((fx < 0) || (fy < 0) || (fx >= FIELD_X) || (fy >= FIELD_Y))
         return;

      if (gRevealed[fx][fy] == 0)
      {
         if (gFlagCount <= 0)
            return;
         gRevealed[fx][fy] = 2;
         gFlagCount--;
         game_mode = kGameNormal;
         CheckVictory();
      }
      else if (gRevealed[fx][fy] == 2)
      {
         gRevealed[fx][fy] = 0;
         gFlagCount++;
      }
   }
}

void SwineRightMouse(Point pt)
{
   int fx, fy;
   float dx, dy;
   pt.x = pt.x - base_rect.ul.x;
   pt.y = pt.y - base_rect.ul.y;

   dx = pt.x - TILE_X;
   dy = pt.y - TILE_Y;
   if (dx < 0)
      fx = -1;
   else
      fx = float(dx / (float)TILE_DX);

   if (dy < 0)
      fy = -1;
   else
      fy = float(dy / (float)TILE_DY);

//   mprintf("clicking on %d, %d\n",fx,fy);
   if ((game_mode == kGameLost) || (game_mode == kGameWon))
   {
      SetGame(kGameIndex); 
      return;
   }

   // basically just like flag placing

   if ((fx < 0) || (fy < 0) || (fx >= FIELD_X) || (fy >= FIELD_Y))
      return;

   if (gRevealed[fx][fy] == 0)
   {
      if (gFlagCount <= 0)
         return;

      gRevealed[fx][fy] = 2;
      gFlagCount--;
      game_mode = kGameNormal;
      CheckVictory();
   }
   else if (gRevealed[fx][fy] == 2)
   {
      gRevealed[fx][fy] = 0;
      gFlagCount++;
   }
}
