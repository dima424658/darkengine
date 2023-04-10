/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkminig.cpp,v 1.20 2000/02/19 13:25:43 toml Exp $

// alarm icon
#include <2d.h>

#include <resapi.h>
#include <appagg.h>
#include <minmax.h>
#include <mprintf.h>

#include <playrobj.h>
#include <simtime.h>
#include <schema.h>
#include <random.h>
#include <rand.h>
#include <math.h>

#include <shkovrly.h>
#include <shkutils.h>
#include <shkovcst.h>
#include <shkmfddm.h>
#include <shkobjst.h>
#include <shkprop.h>
#include <shkiftul.h>
#include <shkplayr.h>
#include <shkminig.h>
#include <shkhplug.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
}

static Rect full_rect = {{LMFD_X, LMFD_Y}, {LMFD_X + LMFD_W, LMFD_Y + LMFD_H}}; 
static Rect close_rect = {{163,8},{163 + 20, 8 + 21}};

static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

static IRes *gGameHnd;

typedef void (*GamePointFunc)(Point);
typedef void (*GamePointBoolFunc)(Point, BOOL);
typedef void (*GameFunc)(void);

extern GameFunc gInitFuncs[];
extern GameFunc gShutdownFuncs[];
extern GamePointFunc gDrawFuncs[];
extern GamePointFunc gMouseFuncs[];
extern GamePointFunc gRightMouseFuncs[];
extern GamePointBoolFunc gDragDropFuncs[];

static eMFDGame gGameCurrent = kGameNone;
//--------------------------------------------------------------------------------------
void SetGame(eMFDGame which)
{
   // call shutdown
   if (gGameCurrent != kGameNone)
   {
      if (gShutdownFuncs[gGameCurrent] != NULL)
         gShutdownFuncs[gGameCurrent]();
   }
   gGameCurrent = which;
   if (gInitFuncs[gGameCurrent] != NULL)
      gInitFuncs[gGameCurrent]();
}
//--------------------------------------------------------------------------------------
void ShockMFDGameInit(int which)
{
   int i;
   gGameHnd= LoadPCX("gameback");
   gGameCurrent = kGameNone;

   close_handles[0] = LoadPCX("CloseOff"); 
   close_handles[1] = LoadPCX("CloseOn"); 
   close_bitmaps[0] = (grs_bitmap *) close_handles[0]->Lock();
   close_bitmaps[1] = (grs_bitmap *) close_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      close_bitmaps[i] = close_bitmaps[0];
   }

   SetLeftMFDRect(which, full_rect);
}

//--------------------------------------------------------------------------------------
void ShockMFDGameTerm(void)
{
   SafeFreeHnd(&gGameHnd);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);

   if (gGameCurrent != kGameNone)
   {
      if (gShutdownFuncs[gGameCurrent] != NULL)
         gShutdownFuncs[gGameCurrent]();
   }
}

//--------------------------------------------------------------------------------------
void ShockMFDGameDraw(void)
{
   Rect r = ShockOverlayGetRect(kOverlayMFDGame);

   DrawByHandle(gGameHnd,r.ul);

   if (gDrawFuncs[gGameCurrent] != NULL)
      gDrawFuncs[gGameCurrent](r.ul);

   LGadDrawBox(VB(&close_button),NULL);
}

//--------------------------------------------------------------------------------------
bool ShockMFDGameHandleMouse(Point pos) 
{
   //Point usepos;
   //Rect r = ShockOverlayGetRect(kOverlayMFDGame);

   //usepos.x = pos.x - r.ul.x;
   //usepos.y = pos.y - r.ul.y;

   if (gMouseFuncs[gGameCurrent] != NULL)
      gMouseFuncs[gGameCurrent](pos);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   uiDefer(DeferOverlayClose,(void *)kOverlayMFDGame);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r = ShockOverlayGetRect(kOverlayMFDGame);

   // set up the continue button
   close_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   close_elem.draw_data = close_bitmaps;
   close_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&close_button, LGadCurrentRoot(), close_rect.ul.x + r.ul.x, close_rect.ul.y + r.ul.y,
      RectWidth(&close_rect), RectHeight(&close_rect), &close_elem, close_cb, 0);
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   LGadDestroyBox(VB(&close_button),FALSE);
}
//--------------------------------------------------------------------------------------
void ShockMFDGameStateChange(int which)
{
   if (ShockOverlayCheck(which))
   {
      // just got put up
      BuildInterfaceButtons();

      ObjID o;
      o = ShockOverlayGetObj();
      if (gPropHackDiff->IsRelevant(o) && !(ObjGetObjState(o) == kObjStateHacked))
      {
         ShockHRMPlugSetMode(0, o);
         ShockOverlayChange(kOverlayHRMPlug, kOverlayModeOn);
      }

      SetGame(kGameIndex);
   }
   else
   {
      // just got taken down
      ShockOverlayChange(kOverlayHRMPlug, kOverlayModeOff);

      DestroyInterfaceButtons();
   }
}
//--------------------------------------------------------------------------------------
bool ShockMFDGameRightMouse(Point pos)
{
   if (gRightMouseFuncs[gGameCurrent] != NULL)
      gRightMouseFuncs[gGameCurrent](pos);
   return(TRUE);
}
//--------------------------------------------------------------------------------------

BOOL ShockMiniGameIsHacked(void)
{
   ObjID o;
   o = ShockOverlayGetObj();
   if (ObjGetObjState(o) == kObjStateHacked)
      return(TRUE);
   else
      return(FALSE);
}

//--------------------------------------------------------------------------------------
bool ShockMFDGameDragDrop(Point pos, BOOL start)
{
   if (gDragDropFuncs[gGameCurrent] != NULL)
      gDragDropFuncs[gGameCurrent](pos,start);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
sOverlayFunc OverlayMFDGame = { 
   ShockMFDGameDraw, // draw
   ShockMFDGameInit, // init
   ShockMFDGameTerm, // term
   ShockMFDGameHandleMouse, // mouse
   ShockMFDGameRightMouse, // dclick (really use)
   ShockMFDGameDragDrop, // dragdrop
   NULL, // key
   NULL, // bitmap
   "", // upschema
   "", // downschema
   ShockMFDGameStateChange, // state
   NULL, // transparency
   0, // distance
   TRUE, // needmouse
};


//--------------------------------------------------------------------------------------
// THE MINIGAME INDEX MINIGAME
//--------------------------------------------------------------------------------------
#define INDEX_X 16
#define INDEX_Y 14
#define INDEX_DY  15
static void IndexDraw(Point pt)
{
   Point drawpt;
   drawpt.x = pt.x + INDEX_X;
   drawpt.y = pt.y + INDEX_Y;

   int games;
   int i;
   char temp[255];

   static int dim_color[3] = { 100, 100, 100};

   if (!gPropMiniGames->Get(PlayerObject(), &games))
      return;

   for (i=1; i < kGameMax; i++)
   {
      ShockStringFetch(temp,sizeof(temp),"GameName","minigame",i);
      if (temp[0]=='\0')
         strcpy(temp,"Test Game");
      if (games & (1 << i))
         gr_font_string(gShockFont,temp,drawpt.x,drawpt.y);
      drawpt.y = drawpt.y + INDEX_DY;
   }
}

static void IndexMouse(Point pt)
{
   int which;
   which = ((pt.y - INDEX_Y) / INDEX_DY) + 1;

   //mprintf("IndexMouse, %d, %d  which = %d\n",pt.x,pt.y,which);
   if ((which < 0) || (which >= kGameMax)) 
      return;

   int games;
   if (gPropMiniGames->Get(PlayerObject(), &games))
   {
      //mprintf("games = %x, which = %x\n",games,(1 << which));
      if (games & (1 << which))
         SetGame((eMFDGame)which);
   }
}

static void IndexDragDrop(Point pos, BOOL start)
{
   //mprintf("index dragdrop: %d, %d  mode %d\n",pos.x,pos.y,start);
}
//--------------------------------------------------------------------------------------
// SLOT MACHINE MINIGAME
//--------------------------------------------------------------------------------------

void SlotsInit(void)
{
}

void SlotsTerm(void)
{
}

void SlotsDraw(Point pt)
{
}

void SlotsMouse(Point pt)
{
}

///////////////////////////
// high score "system"

// for getting our strings out
#define GetNamedString(buf,name)    ShockStringFetch(buf,sizeof(buf),name,"minigame")

char gMiniGameScoreMsg[HIGH_SCORE_MSG_LEN];

void ShockMiniGameAddHighScore(sMiniGameScores *pScores, int score, char *buf)
{
   if (score>=pScores->scores[MAX_HIGH_SCORES-1].pts)
   {
      for (int i=MAX_HIGH_SCORES-2; i>=0; i--)
         if (pScores->scores[i].pts<=score)
            pScores->scores[i+1]=pScores->scores[i];
         else
            break;
      pScores->scores[i+1].pts=score;
      strncpy(pScores->scores[i+1].msg,buf,HIGH_SCORE_MSG_LEN-2);
      pScores->scores[i+1].msg[HIGH_SCORE_MSG_LEN-1]='\0';
      if (i==-1)
         GetNamedString(gMiniGameScoreMsg,"minig_topscore");
      else
      {
         char buf[64];
         GetNamedString(buf,"minig_newscore");
         sprintf(gMiniGameScoreMsg,buf,i+2);
      }
   }
   else
      GetNamedString(gMiniGameScoreMsg,"minig_youlose");
}

static char *_get_next_break(char *str)
{
   char *pN, *p1, *p;
   p1=strchr(str,1);     // 1 is softspace
   pN=strchr(str,'\n');  // \n embedded CR
   if (pN&&p1)      // have both, take closest
      p=min(p1,pN);
   else   
      if (pN)       // if only one, take it
         p=pN;
      else          // well, p1 might be NULL, but that means both are 
         p=p1;
   return p;        // return it
}

void ShockMiniGameDisplayHighScores(sMiniGameScores *pScores, int x, int y)
{
   int j, lines_per=1, wid=140-18; // 140 is est width, 18 for the number, see below
   char buf[256];
   
   // first go through and find the longest line...
   for (j=0; j<MAX_HIGH_SCORES; j++)
   {
      strcpy(buf,pScores->scores[j].msg);
      gr_font_string_wrap(gShockFont,buf,wid);
      char *s=buf, *p;
      int lines=0;
      do {
         p=_get_next_break(s);
         if (p!=NULL)
         {
            s=p+1;
            lines++;
         }
      } while (p!=NULL);
      if (*s!='\0')
         lines++;
      if (lines>lines_per)
         lines_per=lines;
   }
   

   // figure out how many to draw
   int scores_to_show=HIGH_SCORE_MAX_LINES/lines_per;
   if (scores_to_show>MAX_HIGH_SCORES)
      scores_to_show=MAX_HIGH_SCORES;

   // now display, rewrapping, if lines_per > 1
   for (j=0; j<scores_to_show; j++)
   {
      sprintf(buf,"%d: ",j+1);
      gr_font_string(gShockFont, buf, x, y);
      x+=18;        // move over
      if (lines_per==1)
      {
         gr_font_string(gShockFont, pScores->scores[j].msg, x, y);
         x-=18; y+=11; // move back and then down
      }
      else
      {
         strcpy(buf,pScores->scores[j].msg);
         gr_font_string_wrap(gShockFont,buf,wid);
         char *s=buf, *p;
         int lines=0;
         do {
            p=_get_next_break(s);
            if (p!=NULL)
            {
               *p='\0';
               gr_font_string(gShockFont, s, x, y);
               y+=11;
               s=p+1;
               lines++;
            }
         } while (p!=NULL);
         if (*s!='\0')
         {
            gr_font_string(gShockFont, s, x, y);
            y+=11;
            lines++;
         }
         x-=18;
         while (lines<lines_per)
         {
            y+=11;
            lines++;
         }
      }
   }
}

////////////////////////////////////////////////////////////////
// subcanvas controller

static grs_canvas  tmp_minig_canvas;
static grs_bitmap *p_tmp_minig_bitmap=NULL;

// pass clear_color -1 if no clear
// the bitmap returned by this _must come back_ in blit canvas
BOOL ShockMiniGameSetupCanvas(Point pt, Rect base, int clear_color)
{
   grs_canvas *cnv=&tmp_minig_canvas;

   if (p_tmp_minig_bitmap)
      Warning(("Hey! trying to use minigamecanvas twice\n"));
   p_tmp_minig_bitmap = gr_alloc_bitmap(BMT_FLAT16, 0, RectWidth(&base),RectHeight(&base));
   gr_make_canvas(p_tmp_minig_bitmap,cnv);
   gr_push_canvas(cnv);
   if (clear_color!=-1)
      gr_clear(clear_color);
   return TRUE;
}

void ShockMiniGameBlitCanvas(Point pt, Rect base)
{
   gr_pop_canvas();                                   // pop the canvas, then blit it
   gr_bitmap(p_tmp_minig_bitmap,pt.x + base.ul.x, pt.y + base.ul.y);
   gr_free(p_tmp_minig_bitmap);                       // clear up our alloc
   p_tmp_minig_bitmap=NULL;
}

////////////////////////////////////////////////////////////////
// if you want to flip to screensaver mode

#define MAX_SS_LEN 16
static int      cur_len=0, cur_ptr=0;
static float    cur_spd[2], cur_ang[2];  // 0,1 is 0, 1,2 is 1
static float    cur_ss[MAX_SS_LEN][4];
static int      cur_col[MAX_SS_LEN];

#define ss_float_to_fix(x) ((int)(x*65536.0))
#define ss_pt(x,w)         (ss_float_to_fix(cur_ss[x][w]))
#define ss_line(i)         gr_fix_line(ss_pt(i,0),ss_pt(i,1),ss_pt(i,2),ss_pt(i,3))
#define FltPI              (3.1415925358979323846)
#define Flt2PI             (FltPI*2)
#define MIN_SPD            (3.0)
#define MAX_SPD            (12.0)
#define InitPos(m)         (20.0+(RandFloat()*((m/2)-20)))

// 5 bit targets for these
#define MIX_COLOR(r,g,b) ((grd_bpp==15)?((r<<10)|(g<<5)|(b)):((r<<11)|((g<<1)<<5)|(b)))
#define UNMIX_COLOR(x,r,g,b) \
   do {if (grd_bpp==15) { r=((x>>10)&0x1f); g=((x>>5)&0x1f); b=(x&0x1f); } \
       else { r=((x>>11)&0x1f); g=((x>>5)&0x3f)>>1; b=(x&0x1f); } } while (0)

static void AngReflect(float *ang, float axis)
{
   if ((Rand()&3)==0) // just do random around this axis
      *ang=axis+RandFloat()*FltPI-(FltPI/2);
   else               // reflect across it
      *ang=axis+(axis-(*ang));
   do {
      if ((*ang)<0)      *ang+=Flt2PI;
      if ((*ang)>=Flt2PI) *ang-=Flt2PI;
   } while (((*ang)<0)||((*ang)>=Flt2PI));
}

static void FillFromPrev(int w)
{
   int c[3], prev=(w==0)?MAX_SS_LEN-1:w-1;
   float clip_hi, ref[2][2]={{FltPI,0},{3*FltPI/2,FltPI/2}};

   cur_ss[w][0]=cur_ss[prev][0]+cos(cur_ang[0])*cur_spd[0];
   cur_ss[w][1]=cur_ss[prev][1]+sin(cur_ang[0])*cur_spd[0];

   cur_ss[w][2]=cur_ss[prev][2]+cos(cur_ang[1])*cur_spd[1];
   cur_ss[w][3]=cur_ss[prev][3]+sin(cur_ang[1])*cur_spd[1];

   for (int i=0; i<4; i++)
   {
      float r_axis=-1;
      if ((i&1)==0) // x_coor
         clip_hi=grd_canvas->bm.w;
      else
         clip_hi=grd_canvas->bm.h;
      if (cur_ss[w][i]>=clip_hi)
       { cur_ss[w][i]=clip_hi-1.1; r_axis=ref[i>>1][0]; }
      else if (cur_ss[w][i]<=0)
       { cur_ss[w][i]=1.1;         r_axis=ref[i>>1][1]; }
      if (r_axis!=-1)
         AngReflect(&cur_ang[i>>1],r_axis);
   }

   UNMIX_COLOR(cur_col[prev],c[0],c[1],c[2]);
   for (int j=0; j<3; j++)
    { c[j]+=(Rand()%5)-2; if (c[j]<0) c[j]=0; else if (c[j]>31) c[j]=31; }
   cur_col[w]=MIX_COLOR(c[0],c[1],c[2]);
}

static void InitSS(void)
{
   int w=grd_canvas->bm.w, h=grd_canvas->bm.h;
   cur_len=3+(Rand()%10);
   cur_ss[0][0]=InitPos(w);       cur_ss[0][1]=InitPos(h);
   cur_ss[0][2]=(w/2)+InitPos(w); cur_ss[0][3]=(h/2)+InitPos(h);
   cur_ang[0]=RandFloat()*Flt2PI; cur_ang[1]=RandFloat()*Flt2PI;
   cur_spd[0]=4+RandFloat()*5;    cur_spd[1]=4+RandFloat()*5;
   cur_col[0]=MIX_COLOR((Rand()&0x1f),(Rand()&0x1f),(Rand()&0x1f));
   for (int i=1; i<cur_len; i++)
      FillFromPrev(i);
   cur_ptr=cur_len-1;
}

void ShockMiniGameScreenSaver(Point pt, Rect base)
{
   ShockMiniGameSetupCanvas(pt,base,0);       // setup our sitchiation
   if (cur_len==0) InitSS();                  // need we init?
   cur_ptr=(cur_ptr+1)%MAX_SS_LEN;            // update the saver
   FillFromPrev(cur_ptr);
   for (int i=0; i<2; i++)                    // update control vars
   {  
      cur_spd[i]+=(RandFloat()-0.5);
      if (cur_spd[i]<MIN_SPD) cur_spd[i]=MIN_SPD;
      else if (cur_spd[i]>MAX_SPD) cur_spd[i]=MAX_SPD;
      cur_ang[i]+=(RandFloat()-0.5)/5.0;
   }
   for (int j=0; j<cur_len; j++)              // now do the draw
   {
      int idx=(MAX_SS_LEN+cur_ptr-j)%MAX_SS_LEN;
      gr_set_fcolor(cur_col[idx]);
      ss_line(idx);
   }
   ShockMiniGameBlitCanvas(pt,base);          // and, of course, the blit
}

////////////////////////////////////////////////////////////////
// the actual dumb arrays

extern void PingInit(void);
extern void PingTerm(void);
extern void PingDraw(Point pt);
extern void PingMouse(Point pt);

extern void SwineInit(void);
extern void SwineTerm(void);
extern void SwineDraw(Point pt);
extern void SwineMouse(Point pt);
extern void SwineRightMouse(Point pt);

extern void OverWorldInit(void);
extern void OverWorldTerm(void);
extern void OverWorldDraw(Point pt);
extern void OverWorldMouse(Point pt);
extern void OverWorldRightMouse(Point pt);

extern void KaBaconInit(void);
extern void KaBaconTerm(void);
extern void KaBaconDraw(Point pt);
extern void KaBaconMouse(Point pt);

extern void AbyssInit(void);
extern void AbyssTerm(void);
extern void AbyssDraw(Point pt);
extern void AbyssMouse(Point pt);
extern void AbyssRightMouse(Point pt);

extern void TicTacToeInit(void);
extern void TicTacToeTerm(void);
extern void TicTacToeDraw(Point pt);
extern void TicTacToeMouse(Point pt);

extern void HoggerInit(void);
extern void HoggerTerm(void);
extern void HoggerDraw(Point pt);
extern void HoggerMouse(Point pt);

extern void RacerInit(void);
extern void RacerTerm(void);
extern void RacerDraw(Point pt);
extern void RacerMouse(Point pt);
extern void RacerRMouse(Point pt);

extern void TetrisInit();
extern void TetrisTerm();
extern void TetrisDraw(Point pt);
extern void TetrisMouse(Point pt);
extern void TetrisRightMouse(Point pt);

extern void DonkeyHogInit();
extern void DonkeyHogTerm();
extern void DonkeyHogDraw(Point pt);
extern void DonkeyHogMouse(Point pt);
extern void DonkeyHogRightMouse(Point pt);

extern void GolfInit(void);
extern void GolfTerm(void);
extern void GolfDraw(Point pt);
extern void GolfRightMouse(Point pt);
extern void GolfDragDrop(Point pt, BOOL start);

//--------------------------------------------------------------------------------------
GameFunc gInitFuncs[] = { 
   NULL, SlotsInit, PingInit, SwineInit, OverWorldInit, 
   KaBaconInit, AbyssInit, HoggerInit, TicTacToeInit, 
   RacerInit, TetrisInit, DonkeyHogInit, GolfInit,
};
GameFunc gShutdownFuncs[]= { 
   NULL , SlotsTerm, PingTerm, SwineTerm, OverWorldTerm, 
   KaBaconTerm, AbyssTerm, HoggerTerm, TicTacToeTerm, 
   RacerTerm, TetrisTerm, DonkeyHogTerm, GolfTerm, 
};
GamePointFunc gDrawFuncs[] = { 
   IndexDraw, SlotsDraw, PingDraw, SwineDraw, OverWorldDraw, 
   KaBaconDraw, AbyssDraw, HoggerDraw, TicTacToeDraw, 
   RacerDraw, TetrisDraw, DonkeyHogDraw, GolfDraw,
};
GamePointFunc gMouseFuncs[] = { 
   IndexMouse, SlotsMouse, PingMouse, SwineMouse, OverWorldMouse, 
   KaBaconMouse, AbyssMouse, HoggerMouse, TicTacToeMouse, 
   RacerMouse, TetrisMouse, DonkeyHogMouse, NULL,
};
GamePointFunc gRightMouseFuncs[] = { 
   NULL, NULL, NULL, SwineRightMouse, OverWorldRightMouse, 
   NULL, AbyssRightMouse, NULL, NULL, 
   RacerRMouse, TetrisRightMouse, DonkeyHogMouse, GolfRightMouse,
};
GamePointBoolFunc gDragDropFuncs[] = {
   IndexDragDrop, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, GolfDragDrop,
};
