/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkping.cpp,v 1.4 2000/02/19 13:25:54 toml Exp $

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
// PING
//--------------------------------------------------------------------------------------

Rect pong_rect = {{13,11},{152,286}};
#define MFD_VIEW_WID   (RectWidth(&pong_rect))
#define MFD_VIEW_HGT   (RectHeight(&pong_rect))
#define MFD_VIEW_MID    (MFD_VIEW_WID / 2)

typedef struct {
   int game_mode;
   int ball_pos_x, ball_pos_y;
   int ball_dir_x, ball_dir_y;
   int p_pos, p_spd;
   int c_pos, c_spd;
   int c_score;
   int p_score;
   int last_point;
   int game_won;
} pong_state;

pong_state gPong;
// -----------------
// mfd pong

#define PLY_PADDLE_YRAD 2
#define PLY_PADDLE_XRAD 6
#define CMP_PADDLE_YRAD 2
#define CMP_PADDLE_XRAD 6
#define PONG_BALL_YRAD  2
#define PONG_BALL_XRAD  3
#define PONG_BORDER     3
#define PONG_SWEET_SPOT 2


// -------------------
// generic paddle-hits-ball code

#define PONG_X_DIR_MAX 7

int get_new_x_dir(int paddle_loc,int ball_loc)
{
   int x_dir=((ball_loc-paddle_loc)/2);
   if (x_dir<0)
      if (x_dir>=-PONG_SWEET_SPOT) x_dir=0;
      else if (x_dir<-PONG_X_DIR_MAX) x_dir=-PONG_X_DIR_MAX;
      else x_dir++;
   else if (x_dir>0)
      if (x_dir<=PONG_SWEET_SPOT) x_dir=0;
      else if (x_dir>PONG_X_DIR_MAX) x_dir=PONG_X_DIR_MAX;
      else x_dir--;
   return x_dir;
}

// this function returns non-zero
// if the ball went off the bottom past the paddle
int generic_ball_and_paddle(void *game_state)
{
  pong_state *work_ps = (pong_state *) game_state;

   // now deal with moving the ball, assume it is currently a valid state
   work_ps->ball_pos_x+=work_ps->ball_dir_x;
   work_ps->ball_pos_y+=work_ps->ball_dir_y;
   work_ps->p_pos+=work_ps->p_spd;
   if (work_ps->ball_pos_x<PONG_BALL_XRAD)
   {
      work_ps->ball_pos_x=PONG_BALL_XRAD+(PONG_BALL_XRAD-work_ps->ball_pos_x);
      work_ps->ball_dir_x=-work_ps->ball_dir_x;
   }
   if (work_ps->ball_pos_x>MFD_VIEW_WID-1-PONG_BALL_XRAD)
   {
      work_ps->ball_pos_x=MFD_VIEW_WID-1-PONG_BALL_XRAD-(PONG_BALL_XRAD-(MFD_VIEW_WID-1-work_ps->ball_pos_x));
      work_ps->ball_dir_x=-work_ps->ball_dir_x;
   }
   if (work_ps->ball_dir_y>=0)
    // lets see whats up with the player
   {
     if (work_ps->ball_pos_y-PONG_BALL_YRAD>MFD_VIEW_HGT-PONG_BORDER)
	   {  // out the bottom
        //mprintf("out bottom?  %d - %d  > %d - %d\n",work_ps->ball_pos_y,PONG_BALL_YRAD,MFD_VIEW_HGT,PONG_BORDER);
	     return 1;
	   }
     else if (work_ps->ball_pos_y>MFD_VIEW_HGT-PONG_BORDER-CMP_PADDLE_YRAD*2-PONG_BALL_YRAD)
	{  // was the player there to deflect it
	  if ((work_ps->ball_pos_x+PONG_BALL_XRAD>=work_ps->p_pos-PLY_PADDLE_XRAD)&&
	      (work_ps->ball_pos_x-PONG_BALL_XRAD<=work_ps->p_pos+PLY_PADDLE_XRAD))
	      {  // we got it, larry
		int nxdir=get_new_x_dir(work_ps->p_pos,work_ps->ball_pos_x);
		work_ps->ball_dir_y=-work_ps->ball_dir_y;       // hmm, does dirx work yet
		work_ps->ball_dir_x=(nxdir-work_ps->ball_dir_x)>>1;      // average of reflection and new angle??? is this good?
		  // minus looks good if it hits the edge of the paddle
		  // plus looks good if it hits the middle of the paddle
		  //   hmm... so need average of reflection and new, but
		  //   reflection has different meanings depending where it hit
      SchemaPlay((Label *)"pong_hit",NULL);
	      }
	   }
   }
   return 0;
}

static long score_time=0;

void games_run_pong(pong_state *work_ps)
{
   int c_des_spd=0;    // desired speed for the computer player
   if ((work_ps->ball_dir_x|work_ps->ball_dir_y)==0)
   {  // create new ball
      int serve_speed = float((work_ps->c_score+work_ps->p_score) * 0.8) + 2.3 ;
      work_ps->ball_pos_y=MFD_VIEW_HGT/2;
//      work_ps->ball_dir_x=(rand()%3)-1;       // -1 -> 1 for x
      work_ps->ball_dir_x=0;
      work_ps->ball_dir_y=((rand()%3)-1)*serve_speed;
      if (work_ps->ball_dir_y==0) work_ps->ball_dir_y=-serve_speed; // -2 -> 2, but not 0, for y
      if (work_ps->ball_dir_y < 0)
	 work_ps->ball_pos_x=work_ps->c_pos;
      else
	 work_ps->ball_pos_x=work_ps->p_pos;
      score_time=0;
   }

   // the brutally powerful AI, thats right, AI, think about it
   if (work_ps->ball_dir_y<0) // moving towards computer
   {
      if (work_ps->c_pos<work_ps->ball_pos_x-CMP_PADDLE_XRAD) // we are too far left
	 c_des_spd=(((work_ps->ball_pos_x-work_ps->c_pos)>>3)+2);
      else if (work_ps->c_pos>work_ps->ball_pos_x+CMP_PADDLE_XRAD)
	 c_des_spd=-(((work_ps->c_pos-work_ps->ball_pos_x)>>3)+2);
   }                              
   if (work_ps->c_spd>c_des_spd)
      work_ps->c_spd--;
   else if (work_ps->c_spd<c_des_spd)
      work_ps->c_spd++;

   if (generic_ball_and_paddle(work_ps)) {
       // the player missed.
       // what a loser

      SchemaPlay((Label *)"pong_miss",NULL);
     work_ps->ball_dir_x=work_ps->ball_dir_y=0;
     work_ps->last_point=0;
     if (++work_ps->c_score==0x7) work_ps->game_won=1;
     score_time=GetSimTime();
     return;
   }

   work_ps->c_pos+=work_ps->c_spd;
   if (work_ps->ball_dir_y<0) // moving towards computer
   {
	   if (work_ps->ball_pos_y+PONG_BALL_YRAD<PONG_BORDER)
	   {  // out the top, point for the player
	 //play_digi_fx(SFX_INVENT_WARE,1);
      SchemaPlay((Label *)"pong_score",NULL);
	 work_ps->ball_dir_x=work_ps->ball_dir_y=0;
	 work_ps->last_point=1;
	 if (++work_ps->p_score==0x7) work_ps->game_won=1;
	 score_time=GetSimTime();
      }
	   else if (work_ps->ball_pos_y<PONG_BORDER+CMP_PADDLE_YRAD*2+PONG_BALL_YRAD)
	   {  // was the computer there to deflect it
	      if ((work_ps->ball_pos_x+PONG_BALL_XRAD>=work_ps->c_pos-CMP_PADDLE_XRAD)&&
		  (work_ps->ball_pos_x-PONG_BALL_XRAD<=work_ps->c_pos+CMP_PADDLE_XRAD))
	      {  // we got it, larry
	    int nxdir=get_new_x_dir(work_ps->c_pos,work_ps->ball_pos_x);
		 work_ps->ball_dir_y=-work_ps->ball_dir_y;       // hmm, does dirx work yet
	    work_ps->ball_dir_x=(nxdir-work_ps->ball_dir_x)>>1;      // average of reflection and new angle??? is this good?
	    work_ps->ball_dir_x += rand()%3 -1;
      SchemaPlay((Label *)"pong_hit",NULL);
	      }
	   }
   }

}

#define PONG_FUDGE 10

bool games_handle_pong(Point pos)
{
   pong_state *cur_ps=&gPong; // (pong_state *)GAME_DATA;

   ubyte spd = min(8,abs(pos.x - cur_ps->p_pos)/PLY_PADDLE_XRAD+1);
   if(pos.x+PONG_FUDGE<0 || pos.y+PONG_FUDGE<0 ||
      pos.x>=RectWidth(&pong_rect)+PONG_FUDGE || pos.y>=RectHeight(&pong_rect)+PONG_FUDGE)
   {
      cur_ps->p_spd=0;
      return TRUE;
   }
   if (cur_ps->p_pos == pos.x)
      cur_ps->p_spd = 0;
   else if (cur_ps->p_pos > pos.x)
      cur_ps->p_spd = -spd;
   else
      cur_ps->p_spd = spd;
   return TRUE;
}

IRes *gPongBack;
IRes *gPongPaddle;
IRes *gPongBall;

void PingInit()
{
   pong_state *cur_ws= &gPong; // (pong_state *)game_state;
   cur_ws->ball_dir_x=cur_ws->ball_dir_y=cur_ws->c_spd=cur_ws->p_spd=cur_ws->p_score=cur_ws->c_score=cur_ws->game_won=0;
   cur_ws->c_pos=cur_ws->p_pos=MFD_VIEW_MID;      // move paddles to middle

   //if (&cur_ws->game_mode!=(&GAME_MODE)) Warning(("Pointer Chaos %x %x\n",&cur_ws->game_mode,&GAME_MODE));

   score_time=0;

   gPongBack = LoadPCX("pongback");
   gPongBall = LoadPCX("pongball");
   gPongPaddle = LoadPCX("pongpadd");
}

void PingTerm()
{
   SafeFreeHnd(&gPongBack);
   SafeFreeHnd(&gPongBall);
   SafeFreeHnd(&gPongPaddle);
}

#define NORMAL_DISPLAY  0
#define SCORE_DISPLAY   1
#define WIN_DISPLAY     2

#define WIN_PAUSE       4000 // (4*CIT_CYCLE)
#define SCORE_PAUSE     2000 // (2*CIT_CYCLE) 

#define PONG_CYCLE      (1000/30)

/*
void ss_rect(Point pt, int a,int b,int c,int d)
{ 
   gr_rect(a + pt.x, b + pt.y, pt.x + c, pt.y + d);
}
*/

void ss_string(Point pt, char *name, short x, short y)
{
   char temp[255];
   ShockStringFetch(temp,sizeof(temp),name,"minigame");
   gr_font_string(gShockFont,temp,pt.x + x, pt.y + y);
}

void PingDraw(Point dpt)
{
   pong_state *cur_ps= &gPong; // (pong_state *)GAME_DATA;
   int game_use=NORMAL_DISPLAY;

   //if (tac == 0 && score_time > 0)
     // score_time = 0;

   // is not true
   if (cur_ps->game_won)
   {
      if (score_time+WIN_PAUSE > GetSimTime())
	 game_use=WIN_DISPLAY;
      else
//       { games_init_pong(cur_ps); return;}
      { 
         SetGame(kGameIndex); 
         return;
      }
   }
   else if (score_time+SCORE_PAUSE>GetSimTime()) {
      game_use=SCORE_DISPLAY;
   }
   else
   {
      extern bool games_handle_pong(Point pos);
      static tSimTime frac = 0;

      tSimTime diff = GetSimFrameTime() + frac;
      for(; diff>=PONG_CYCLE; diff-=PONG_CYCLE) {
         Point pos;
	      games_run_pong(cur_ps);
	      mouse_get_xy(&pos.x,&pos.y);
         pos.x = pos.x - dpt.x;
         pos.y = pos.y - dpt.y;
	      games_handle_pong(pos);
	      if (score_time > 0 || cur_ps->game_won)
		 break;
      }
      // carry over sim time
      frac = diff;
   }
   //if (!full_game_3d)
     // draw_res_bm(REF_IMG_bmBlankMFD, 0, 0);
   Point pt;
   pt.x = dpt.x + pong_rect.ul.x;
   pt.y = dpt.y + pong_rect.ul.y;
   DrawByHandle(gPongBack,pt);

   //gr_set_fcolor(ORANGE_YELLOW_BASE);
   //ss_rect(pt, cur_ps->c_pos-CMP_PADDLE_XRAD,PONG_BORDER,cur_ps->c_pos+CMP_PADDLE_XRAD,PONG_BORDER+CMP_PADDLE_YRAD*2);
   Point draw;
   draw.x = pt.x + cur_ps->c_pos;
   draw.y = pt.y + PONG_BORDER; // cur_ps->c_pos;
   DrawByHandleCenter(gPongPaddle, draw);

   //gr_set_fcolor(ORANGE_YELLOW_BASE);
   //ss_rect(pt, cur_ps->p_pos-PLY_PADDLE_XRAD,MFD_VIEW_HGT-PONG_BORDER-PLY_PADDLE_YRAD*2,cur_ps->p_pos+PLY_PADDLE_XRAD,MFD_VIEW_HGT-PONG_BORDER);
   draw.x = pt.x + cur_ps->p_pos;
   draw.y = pt.y + MFD_VIEW_HGT-PONG_BORDER - PLY_PADDLE_YRAD*2;
   DrawByHandleCenter(gPongPaddle,draw);

   //gr_set_fcolor(GRAY_8_BASE);
   //ss_rect(pt, cur_ps->ball_pos_x-PONG_BALL_XRAD,cur_ps->ball_pos_y-PONG_BALL_YRAD,cur_ps->ball_pos_x+PONG_BALL_XRAD,cur_ps->ball_pos_y+PONG_BALL_YRAD);
   draw.x = pt.x + cur_ps->ball_pos_x;
   draw.y = pt.y + cur_ps->ball_pos_y;
   //mprintf("ball at %d, %d\n",cur_ps->ball_pos_x,cur_ps->ball_pos_y);
   DrawByHandleCenter(gPongBall, draw);

   if (game_use!=NORMAL_DISPLAY)
   {
      char tmp[2]="V";
      if (cur_ps->last_point)
   	   ss_string(pt, "YouHave",MFD_VIEW_MID-18,45);
      else
	      ss_string(pt, "ComputerHas",MFD_VIEW_MID-25,45);
      if (cur_ps->game_won)
	      ss_string(pt, "Won",MFD_VIEW_MID-8,60);
      else
	      ss_string(pt, "Scored",MFD_VIEW_MID-16,60);

      char temp[255],text[255];
      ShockStringFetch(temp,sizeof(temp),"YouScore","minigame");
      sprintf(text,temp,cur_ps->p_score);
      gr_font_string(gShockFont,text,pt.x + MFD_VIEW_MID-25,pt.y + 135);

      ShockStringFetch(temp,sizeof(temp),"ComputerScore","minigame");
      sprintf(text,temp,cur_ps->c_score);
      gr_font_string(gShockFont,text,pt.x + MFD_VIEW_MID-25,pt.y + 150);

   }
   //mfd_add_rect(0,0,MFD_VIEW_WID,MFD_VIEW_HGT);

   // autoreexpose
   //mfd_notify_func(MFD_GAMES_FUNC, MFD_INFO_SLOT, FALSE, MFD_ACTIVE, FALSE);

   return;
}

void PingMouse(Point pt)
{
}
