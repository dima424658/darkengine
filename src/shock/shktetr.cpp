/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shktetr.cpp,v 1.3 2000/02/19 13:26:25 toml Exp $

// alarm icon
#include <2d.h>

#include <resapi.h>
#include <appagg.h>
#include <minmax.h>
#include <mprintf.h>

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
#include <random.h>

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

#define GetNamedString(buf,name)    ShockStringFetch(buf,sizeof(buf),name,"minigame")

//--------------------------------------------------------------------------------------
// TETRIS
//--------------------------------------------------------------------------------------

Rect tetris_rect = {{13,11},{152,286}};
#define MFD_VIEW_WID   (RectWidth(&tetris_rect))
#define MFD_VIEW_HGT   (RectHeight(&tetris_rect))
#define MFD_VIEW_MID    (MFD_VIEW_WID / 2)

#define BLOCK_X 12
#define BLOCK_Y 12

#define SIZE_X ((152-13)/BLOCK_X)
#define SIZE_Y ((286-11)/BLOCK_Y)

#define LEVELS 10
// 128 pixels per sec after 5 mins

char szTetPlayfield[(SIZE_X+2)*(SIZE_Y+1)];

char szTetPices[5][(4*4)+1] = 
{
{
   ".0.."
   ".0.."
   ".0.."
   ".0.."
},
{
   ".1.."
   ".11."
   ".1.."
   "...."
},
{
   ".2.."
   ".2.."
   ".22."
   "...."
},
{
   "..3."
   "..3."
   ".33."
   "...."
},
{
   "...."
   ".44."
   ".44."
   "...."
},
};

struct sCol {
   uchar r,g,b;
   int nColor;
};

sCol TetrColor[] = {
   {  0,  0,255,0},
   {  0,255,  0,0},
   {255,255,  0,0},
   {  0,255,255,0},
   {255,  0,  0,0},
   {  0,  0,128,0},
   {  0,128,  0,0},
   {128,128,  0,0},
   {  0,128,128,0},
   {128,  0,  0,0},
};

sFileVarDesc gTetrisScoreDesc =
{
   kCampaignVar,
   "StackerScr",
   "Pig Stacker High Scores",
   FILEVAR_TYPE(sMiniGameScores),
   {1,0},
   {1,0},
   "shock",
};

cFileVar<sMiniGameScores,&gTetrisScoreDesc> gTetrisScoreTable;

static void 
store_high_score(int score, int lvl)
{
   char buf[64], form_buf[64];

   GetNamedString(form_buf,"TetrisScoreForm");
   sprintf(buf,form_buf,score,lvl);
   ShockMiniGameAddHighScore(&gTetrisScoreTable,score,buf);
}

// Pixels/sec.
float vTetLevelSpeed[LEVELS] = 
{
   32.0f+(22.4f*0.0f),
   32.0f+(22.4f*1.0f),
   32.0f+(22.4f*2.0f),
   32.0f+(22.4f*3.0f),
   32.0f+(22.4f*4.0f),
   32.0f+(22.4f*5.0f),
   32.0f+(22.4f*6.0f),
   32.0f+(22.4f*7.0f),
   32.0f+(22.4f*8.0f),
   32.0f+(22.4f*9.0f),
};

typedef enum 
{
   TET_RUNNING,
   TET_GAME_OVER,
} TetModes;

typedef struct {
   TetModes GameMode;
   int nNext;
   int x,y;
   int nPiceDir;
   int nDir;
   int nStuck;
   int nScore;
   int nLevel;
   int nDrop;
   float fTime,fFrames;
   char szPice[(4*4)+1];
} tetris_state;

tetris_state gTetris;
// -----------------
// mfd tetris

void
TetrisRotate(char *szPice)
{
   char szTmp[4*4];
   int x,y;
   memcpy(&szTmp,szPice,sizeof(szTmp));
   for(y=0;y<4;y++)
   {
      for(x=0;x<4;x++)
      {
         szPice[x+(y*4)] = szTmp[((3-x)*4)+y];
      }
   }

}   

void TetrisGetPice(char *szDest,int nPice)
{
   strcpy(szDest,szTetPices[nPice]);
}   

void
TetrisNewPice()
{
   TetrisGetPice(gTetris.szPice,gTetris.nNext);
   gTetris.nNext = RandRangeLong(5);
   gTetris.x = ((SIZE_X/2)-2)*BLOCK_X;
   gTetris.y = (-BLOCK_Y)*4;
   gTetris.nDir = 1;
}   

void 
TetrisInit()
{
   int x,y,i;

   gTetris.GameMode = TET_RUNNING;
   gTetris.fTime = 0.0f;
   gTetris.fFrames = 0.0f;
   gTetris.nScore = 0;
   gTetris.nDrop = 0;
   gTetris.nLevel = 1;

   for(i=0;i<sizeof(TetrColor)/sizeof(sCol);i++)
   {
      TetrColor[i].nColor = guiScreenColor(guiRGB(TetrColor[i].r,TetrColor[i].g,TetrColor[i].b));
   }

   char *szFoo = szTetPlayfield;
   for(y=0;y<SIZE_Y;y++)
   {
      *(szFoo++) = 'X';
      for(x=0;x<SIZE_X;x++)
         *(szFoo++) = '.';
      *(szFoo++) = 'X';
   }
   memset(szFoo,'X',SIZE_X+2);

   gTetris.nNext = RandRangeLong(5);
   TetrisNewPice();

}   

void 
TetrisTerm()
{
   
}   

void
TetrisRect(Point pt,int x,int y,int w,int h)
{
   Point P1,P2;
   P1.x = x;
   P1.y = y;
   P2.x = x+w;
   P2.y = y+h;

   if(P1.x < 0)
      P1.x = 0;
   if(P1.y < 0)
      P1.y = 0;
   if(P2.x < 0)
      P2.x = 0;
   if(P2.y < 0)
      P2.y = 0;
   if(P1.x == 0 && P2.x == 0 && P1.y == 0 && P2.y == 0)
      return;

   P1.x +=  pt.x+tetris_rect.ul.x;
   P1.y +=  pt.y+tetris_rect.ul.y;
   P2.x +=  pt.x+tetris_rect.ul.x;
   P2.y +=  pt.y+tetris_rect.ul.y;

   gr_rect(P1.x,P1.y,P2.x,P2.y);
}   

int
TetrisColl(int nModX,int nModY,char *szPice)
{
   int x,y;
   for(y=0;y<4;y++)
   {
      for(x=0;x<4;x++)
      {
         if(nModY+y < 0)
         {
            szPice++;
            continue;
         }

         if(*(szPice++) != '.' && szTetPlayfield[(nModX+x)+((nModY+y)*(SIZE_X+2))] != '.')
         {
            return 1;
         }
      }
   }
   return 0;
}   

void
TetrisPlace(int nModX,int nModY,char *szPice)
{
   int x,y;
   for(y=0;y<4;y++)
   {
      for(x=0;x<4;x++)
      {
         if(nModY+y < 0)
         {
            if(gTetris.GameMode != TET_GAME_OVER)
            {
               store_high_score(gTetris.nScore,gTetris.nLevel);
            }
            gTetris.GameMode = TET_GAME_OVER;
            szPice++;
            continue;
         }

         if(*(szPice++) != '.')
         {
            szTetPlayfield[(nModX+x)+((nModY+y)*(SIZE_X+2))] = szPice[-1];
         }
      }
   }

   if(gTetris.GameMode == TET_GAME_OVER)
      return;
  
   gTetris.nScore+=gTetris.nLevel;

   szPice = szTetPlayfield;

   int nScore = 10*gTetris.nLevel;

   for(y=0;y<SIZE_Y;y++)
   {
      int nFull = 1;
      for(x=0;x<(SIZE_X+2);x++)
      {
         if(*(szPice++) == '.')
         {
            nFull = 0;
         }
      }
      if(nFull)
      {
         gTetris.nScore += nScore;
         nScore *= 2;
         memmove(szTetPlayfield+(SIZE_X+2),szTetPlayfield,(SIZE_X+2)*y);
         szTetPlayfield[0] = 'X';
         for(x=1;x<(SIZE_X+1);x++)
         {
            szTetPlayfield[x] = '.';
         }
         szTetPlayfield[x] = 'X';
      }
   }
}   

void
TetrisMove(Point pt)
{
   short mx, my;
   mouse_get_xy(&mx, &my);
   
   if((gTetris.x%BLOCK_X) == 0)
   {
      int nMouse = ((mx-(BLOCK_X*2))-tetris_rect.ul.x)/BLOCK_X;
      int nPice = gTetris.x/BLOCK_X;
      
      if(nMouse == nPice)
         gTetris.nDir = 0;
      if(nMouse < nPice)
         gTetris.nDir = -1;
      if(nMouse > nPice)
         gTetris.nDir = 1;

      int nModX,nModY;
      nModX = (gTetris.x / BLOCK_X)+1;
      nModY = (gTetris.y / BLOCK_Y);

      if(TetrisColl(nModX+gTetris.nDir,nModY,gTetris.szPice))
      {
         gTetris.nDir = 0;
      }

      if((gTetris.y % BLOCK_Y) != 0)
      {
         if(TetrisColl(nModX+gTetris.nDir,nModY+1,gTetris.szPice))
         {
            gTetris.nDir = 0;
         }
      }
   }

   gTetris.x += gTetris.nDir;

   if((gTetris.y % BLOCK_Y) == 0)
   {
      int nModX,nModY;
      nModX = (gTetris.x / BLOCK_X)+1;
      nModY = (gTetris.y / BLOCK_Y);

      if(TetrisColl(nModX,nModY+1,gTetris.szPice))
      {
         gTetris.nStuck++;
      }
      else
      {
         gTetris.nStuck=0;
         gTetris.y++;
      }
   }
   else
   {
      gTetris.nStuck=0;
      gTetris.y++;
   }

   if(gTetris.nStuck > 16 && (gTetris.x % BLOCK_X) == 0)
   {
      gTetris.nDrop = 0;
      TetrisPlace((gTetris.x/BLOCK_X)+1,gTetris.y/BLOCK_Y,gTetris.szPice);
      TetrisNewPice();
   }

}

void 
TetrisDraw(Point pt)
{
   gTetris.fTime += GetSimFrameTime()/1000.0f;

   switch(gTetris.GameMode)
   {
      case TET_RUNNING:
      {

         gTetris.nLevel = gTetris.fTime/60.0f;
         if(gTetris.nLevel >= 10)
            gTetris.nLevel = 9;

         gTetris.nLevel++;

         gTetris.fFrames += vTetLevelSpeed[gTetris.nLevel-1]*GetSimFrameTime()/1000.0f;

         if(gTetris.nDrop)
         {
            while(gTetris.nDrop)
            {
               TetrisMove(pt);
            }
         }
         else
         {
            while(gTetris.fFrames >= 1.0f)
            {
               TetrisMove(pt);
               gTetris.fFrames -= 1.0f;
            }
         }
         break;
      }
   }

   int x,y;

   for(y=0;y<SIZE_Y;y++)
   {
      for(x=0;x<SIZE_X;x++)
      {
         char cPice = szTetPlayfield[x+(y*(SIZE_X+2))+1];
         if(cPice != '.')
         {
            gr_set_fcolor(TetrColor[cPice-'0'].nColor);
            TetrisRect(pt,x*BLOCK_X,y*BLOCK_Y,BLOCK_X-1,BLOCK_Y-1);
         }
      }
   }
   
   for(y=0;y<4;y++)
   {
      for(x=0;x<4;x++)
      {
         char cPice = gTetris.szPice[x+(y*4)];
         if(cPice != '.')
         {
            gr_set_fcolor(TetrColor[(cPice-'0')+5].nColor);
            TetrisRect(pt,gTetris.x+(x*BLOCK_X),gTetris.y+(y*BLOCK_Y),BLOCK_X-1,BLOCK_Y-1);
         }
      }
   }

   switch(gTetris.GameMode)
   {
      case TET_GAME_OVER:
      {
         Point drawpt = 
         {
           pt.x+tetris_rect.ul.x+30, pt.y+tetris_rect.ul.y+25
         };
         ShockMiniGameDisplayHighScores(&gTetrisScoreTable,drawpt.x,drawpt.y);

         if(((int)gTetris.fTime) & 1)
            break;

         char szFmt[64];
         Point NewPt = {pt.x+tetris_rect.ul.x, pt.y+tetris_rect.ul.y};
         GetNamedString(szFmt,"TetrisGameOver");

         int nWidth = gr_font_string_width(gShockFont,szFmt);

         NewPt.x += (MFD_VIEW_WID-nWidth)/2;
         NewPt.y += MFD_VIEW_HGT/2;

         gr_font_string(gShockFont,szFmt,NewPt.x,NewPt.y);
         
         break;
      }
   }

   char szFmt[64],szLevel[64];
   GetNamedString(szFmt,"TetrisScore");
   sprintf(szLevel,szFmt,gTetris.nLevel,gTetris.nScore);
   gr_font_string(gShockFont,szLevel,pt.x+tetris_rect.ul.x,pt.y+tetris_rect.ul.y);

}   

int
TetrisRotCheck()
{
   int nModX,nModY;
   nModX = (gTetris.x / BLOCK_X)+1;
   nModY = (gTetris.y / BLOCK_Y);

   if(TetrisColl(nModX,nModY,gTetris.szPice))
   {
      return 1;
   }
   
   if((gTetris.x % BLOCK_X) != 0)
   {
      if(TetrisColl(nModX+1,nModY,gTetris.szPice))
      {
         return 1;
      }
   }

   if((gTetris.y % BLOCK_Y) != 0)
   {
      if(TetrisColl(nModX,nModY+1,gTetris.szPice))
      {
         return 1;
      }
      if((gTetris.x % BLOCK_X) != 0)
      {
         if(TetrisColl(nModX+1,nModY+1,gTetris.szPice))
         {
            return 1;
         }
      }
   }

   return 0;
}
void 
TetrisMouse(Point pt)
{
   switch(gTetris.GameMode)
   {
      case TET_RUNNING:
      {
         TetrisRotate(gTetris.szPice);

         if(TetrisRotCheck())
         {
            TetrisRotate(gTetris.szPice);
            TetrisRotate(gTetris.szPice);
            TetrisRotate(gTetris.szPice);
         }
         break;
      }
      case TET_GAME_OVER:
      {
         TetrisInit();
         gTetris.GameMode = TET_RUNNING;
         break;
      }
         
   }
}   

void 
TetrisRightMouse(Point pt)
{
   gTetris.nDrop = 1;
}   
