/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkttt.cpp,v 1.2 2000/02/19 13:26:30 toml Exp $
#include <2d.h>

#include <resapi.h>
#include <appagg.h>
#include <minmax.h>
#include <mprintf.h>
#include <rand.h>
#include <limits.h>

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
#include <shkmmax.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
}



static IRes *gBackHnd;

Rect base_rect = {{13,11},{152,286}};
#define MFD_VIEW_WID   (RectWidth(&base_rect))
#define MFD_VIEW_HGT   (RectHeight(&base_rect))
#define MFD_VIEW_MID    (MFD_VIEW_WID / 2)

int tictactoe_evaluator(void* pos);

//----------------------------
//----------------------------
// mfd Tic-Tac-Toe
//----------------------------
//----------------------------

typedef struct {
   uchar owner[9];
} tictactoe;

typedef struct {
   uchar game_mode;
   tictactoe board;
   uchar whomoves;
   uchar whoplayer;
} ttt_state;

static ttt_state gState;

#define NOBODY 0
#define X 1
#define O 2
#define OTHERPLAYER(w) (X+O-(w))

#define TTT_SQ_WID 40
#define TTT_SQ_HGT 40

#define TTT_ULX ((MFD_VIEW_WID-(3*TTT_SQ_WID))/2)
// this leaves a little room at the top for text, which
// we secretly declare is 6 pixels
#define TTT_MESS_HGT 6
#define TTT_ULY (TTT_MESS_HGT+(MFD_VIEW_HGT-TTT_MESS_HGT-(3*TTT_SQ_HGT))/2)
#define TTT_PUZ_WID (3*TTT_SQ_WID)
#define TTT_PUZ_HGT (3*TTT_SQ_HGT)
#define TTT_LRX (TTT_ULX+TTT_PUZ_WID)
#define TTT_LRY (TTT_ULY+TTT_PUZ_HGT)

static void tictactoe_drawwin(ttt_state* st, Point offset);

// ----------------------
// TIC-TAC-TOE:
//   static evaluator, move generator
// ----------------------

#pragma disable_message(202)
int winnerval(uchar owner)
{
   if(owner==X) return INT_MAX;
   else if(owner==O) return INT_MIN;
   else return 0;
}

bool tictactoe_over(tictactoe* st)
{
   int i,val;

   val=tictactoe_evaluator(st);
   if(val==winnerval(X)||val==winnerval(O)) return TRUE;

   for(i=0;i<9;i++) {
      if(st->owner[i]==NOBODY) return FALSE;
   }
   return TRUE;
}

static char corners_ttt[]={0,2,6,8};
static char initmove_ttt[]={0,1,4};

int ttt_fullness(tictactoe* st)
{
   int i, ret=0;

   for(i=0;i<9;i++) {
      if(st->owner[i]!=NOBODY)
	 ret++;
   }
   return ret;
}

char move_to_index(char move,tictactoe* st)
{
   int i;
   bool empty;

   empty=(ttt_fullness(st)==0);
   if(empty) return initmove_ttt[move];

   for(i=0;i<9 && move>=0;i++) {
      if(st->owner[i]==NOBODY) {
	 if(move==0)
	    return i;
	 move--;
      }
   }
   Warning(("TTT: Invalid move!\n"));
   return -1;
}


int tictactoe_evaluator(void* pos)
{
   tictactoe* t=(tictactoe *)pos;
   uchar win;

   win=t->owner[0];
   if(win!=NOBODY) {
      if(t->owner[1]==win && t->owner[2]==win)
	 return winnerval(win);
      if(t->owner[3]==win && t->owner[6]==win)
	 return winnerval(win);
   }

   win=t->owner[8];
   if(win!=NOBODY) {
      if(t->owner[6]==win && t->owner[7]==win)
	 return winnerval(win);
      if(t->owner[2]==win && t->owner[5]==win)
	 return winnerval(win);
   }

   win=t->owner[4];
   if(win!=NOBODY) {
      if(t->owner[3]==win && t->owner[5]==win)
	 return winnerval(win);
      if(t->owner[1]==win && t->owner[7]==win)
	 return winnerval(win);
      if(t->owner[0]==win && t->owner[8]==win)
	 return winnerval(win);
      if(t->owner[6]==win && t->owner[2]==win)
	 return winnerval(win);
   }

   return winnerval(NOBODY);
}

// note that this procedure duplicates a lot of the work done by
// tictactoe_evaluator: we do not consolidate them because we don't
// want to slow down the evaluator.
void tictactoe_drawwin(ttt_state* st, Point offset)
{
   uchar win, realwin;
   int i;
   Point p1,p2;
   char buf[80];
   tictactoe* t=&(st->board);

   p1.x=-1;

   for(i=0;i<3;i++) {
      win=t->owner[i];
      if(t->owner[i+3]==win && t->owner[i+6]==win) {
	 realwin=win;
	 p1.x=TTT_ULX+(TTT_SQ_WID*i)+(TTT_SQ_WID/2); p1.y=TTT_ULY;
	 p2.x=p1.x; p2.y=TTT_LRY;
      }         
   }
   for(i=0;i<9;i+=3) {
      win=t->owner[i];
      if(t->owner[i+1]==win && t->owner[i+2]==win) {
	 realwin=win;
	 p1.x=TTT_ULX; p1.y=TTT_ULY+(TTT_SQ_HGT*i/3)+(TTT_SQ_HGT/2);
	 p2.x=TTT_LRX; p2.y=p1.y;
      }
   }
   win=t->owner[0];
   if(t->owner[4]==win && t->owner[8]==win) {
      realwin=win;
      p1.x=TTT_ULX; p1.y=TTT_ULY;
      p2.x=TTT_LRX; p2.y=TTT_LRY;
   }
   win=t->owner[6];
   if(t->owner[4]==win && t->owner[2]==win) {
      realwin=win;
      p1.x=TTT_ULX; p1.y=TTT_LRY;
      p2.x=TTT_LRX; p2.y=TTT_ULY;
   }
   if(p1.x>0) {
      gr_int_line(offset.x + p1.x,offset.y + p1.y,offset.x + p2.x,offset.y + p2.y);
      char t1[255],t2[255];
      if (realwin == st->whoplayer)
         ShockStringFetch(t1,sizeof(t1),"YouHave","minigame");
      else
         ShockStringFetch(t1,sizeof(t1),"ComputerHas","minigame");
      ShockStringFetch(t2,sizeof(t2),"Won","minigame");
      sprintf(buf,"%s %s",t1,t2);
      gr_font_string(gShockFont,buf,offset.x + (MFD_VIEW_WID-gr_font_string_width(gShockFont,buf))/2,
         offset.y + 10);
      //lg_sprintf(buf,"%S%S",realwin==st->whoplayer?REF_STR_YouHave:REF_STR_ComputerHas,REF_STR_Won);
      //draw_shadowed_text(buf,MFD_VIEW_WID-gr_string_width(buf)-1,1);
   }
}


bool tictactoe_generator(void* pos, int index, bool minimizer_moves)
{
   int i;
   tictactoe* t=(tictactoe *)pos;
   bool empty=TRUE;
   uchar mover=minimizer_moves?O:X;

   int realindex=index;

   if(tictactoe_evaluator(pos)!=winnerval(NOBODY)) return FALSE;  // already have a winner => no children

#define NO_SYMMETRIES
#ifdef NO_SYMMETRIES
   for(i=0;empty && i<9;i++) {
      if(t->owner[i]!=NOBODY)
	 empty=FALSE;
   }

   // don't bother with symmetries of starting moves
   if(empty) {
      switch(index) {
	 case 0: t->owner[0]=mover; return TRUE;
	 case 1: t->owner[1]=mover; return TRUE;
	 case 2: t->owner[4]=mover; return TRUE;
	 default: return FALSE;
      }
   }
#endif

   for(i=0;i<9;i++) {
      if(t->owner[i]==NOBODY) {
	 if(index==0) {
	    t->owner[i]=mover;
	    return TRUE;
	 }
	 index--;
      }
   }
   return FALSE;
}
#pragma enable_message(202)

void TicTacToeMouse(Point pt)
{
   ttt_state *st=&gState;
   Point pos = MakePoint(pt.x-base_rect.ul.x-TTT_ULX, pt.y-base_rect.ul.y-TTT_ULY);

   //mprintf("ttt mouse pt %d, %d   board pos %d, %d\n",pt.x,pt.y,pos.x,pos.y);
   //if(!(me->action & MOUSE_LDOWN)) return FALSE;
   if(st->whomoves != st->whoplayer) return;
   if(tictactoe_over(&(st->board))) 
   {
      // restart
      SetGame(kGameTicTacToe); 
      return;
   }
   if(pos.x<0||pos.y<0) return;

   pos.x/=TTT_SQ_WID;
   pos.y/=TTT_SQ_HGT;

   if(pos.x>=TTT_PUZ_WID || pos.y>=TTT_PUZ_HGT)
      return;

   if(st->board.owner[pos.x+3*pos.y] != NOBODY)
      return;

   st->board.owner[pos.x+3*pos.y]=st->whoplayer;

   if(!tictactoe_over(&(st->board))) {
      st->whomoves=OTHERPLAYER(st->whoplayer);

      minimax_setup(&(st->board),sizeof(tictactoe),9,st->whomoves==O,
	 tictactoe_evaluator, tictactoe_generator, NULL);
   }

   //mfd_notify_func(MFD_GAMES_FUNC, MFD_INFO_SLOT, FALSE, MFD_ACTIVE, TRUE);

   return;
}

void TicTacToeInit()
{
   gBackHnd = LoadPCX("pongback");

   // reset the state
   memset(&gState,0,sizeof(ttt_state));

   ttt_state* state=&gState;

   state->whomoves=X;
   state->whoplayer=(rand()&1)?X:O;

   if(state->whoplayer!=state->whomoves) {
      // fake straight to a corner move
      state->board.owner[corners_ttt[rand()&3]]=state->whomoves;
      state->whomoves=state->whoplayer;      
   }

   fstack_init();
}

void TicTacToeTerm()
{
   SafeFreeHnd(&gBackHnd);
}

void TicTacToeDraw(Point pt)
{
   int val;
   static long timeformove=0, dt, timeout;
   char whichmove;
   ttt_state* st=&gState;
   int loops=0;

   int x,y;
   bool over;
   uchar owner;
   char temp[255];

   Point offset;

   //draw_res_bm(REF_IMG_bmBlankMFD, 0, 0);
   offset.x = pt.x + base_rect.ul.x;
   offset.y = pt.y + base_rect.ul.y;
   DrawByHandle(gBackHnd, offset);

   //gr_set_fcolor(FindColor(255,0,0));

   over=tictactoe_over(&(st->board));
   if (!over) 
   {
      if(st->whomoves==st->whoplayer)
         ShockStringFetch(temp,sizeof(temp),"YourMove","minigame");
      else
         ShockStringFetch(temp,sizeof(temp),"Thinking","minigame");
      gr_font_string(gShockFont,temp,offset.x + (MFD_VIEW_WID-gr_string_width(temp))/2,
         offset.y + 1);
      //draw_shadowed_text(get_temp_string(bm),(MFD_VIEW_WID-gr_string_width(get_temp_string(bm)))/2,1);
   }

   // Hmm, this used to be gr_uvline.... maybe insufficent to just make it clipped.
   gr_vline(offset.x + TTT_ULX+TTT_SQ_WID,offset.y + TTT_ULY,offset.y + TTT_ULY+TTT_PUZ_HGT);
   gr_vline(offset.x + TTT_ULX+2*TTT_SQ_WID,offset.y + TTT_ULY,offset.y + TTT_ULY+TTT_PUZ_HGT);
   gr_int_line(offset.x + TTT_ULX,offset.y + TTT_ULY+TTT_SQ_HGT,
      offset.x + TTT_ULX+TTT_PUZ_WID,offset.y + TTT_ULY+TTT_SQ_HGT);
   gr_int_line(offset.x + TTT_ULX,offset.y + TTT_ULY+2*TTT_SQ_HGT,
      offset.x + TTT_ULX+TTT_PUZ_WID,offset.y + TTT_ULY+2*TTT_SQ_HGT);
   for(y=0;y<3;y++) 
   {
      for(x=0;x<3;x++) 
      {
	      owner=st->board.owner[x+3*y];
         IRes *bmp;
         Point pt;
	      if(owner==NOBODY)
	         continue;

	      if(owner==st->whoplayer)
	         bmp = LoadPCX("tttplayr"); // bm=REF_IMG_ttt_Player;
	      else
	         bmp = LoadPCX("tttshodn"); // =REF_IMG_ttt_Shodan;

         pt.x = offset.x + 1+TTT_ULX+TTT_SQ_WID*x;
         pt.y = offset.y + 1+TTT_ULY+TTT_SQ_HGT*y;
	      DrawByHandle(bmp,pt);
         SafeFreeHnd(&bmp);
      }
      if(over) 
      {
	      //gr_set_fcolor(FindColor(0,0,255));
	      tictactoe_drawwin(st,offset);
      }
   }


   // run the "AI"
   if(st->whomoves==st->whoplayer) 
      return;
   if(tictactoe_over(&(st->board))) 
      return;

   while (loops < 10000) 
   {
      minimax_step();
      loops++;
      if (minimax_done())
	      loops = 10000;
   }

   if(minimax_done()) 
   {
      minimax_get_result(&val,&whichmove);
      st->board.owner[move_to_index(whichmove,&(st->board))]=OTHERPLAYER(st->whoplayer);
      st->whomoves=st->whoplayer;
   }
}

