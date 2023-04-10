/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkgolf.cpp,v 1.12 1999/08/05 17:45:34 Justin Exp $

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
#include <matrix.h>

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
}

#include <dbmem.h>

//////////////////////////////
//////////////////////////////
// @TBD:
//   green slope?
//   sandtrap physics hacks
//   lip out for too fast past hole
//   real art for golfer
//   better windsock?
//
// BIG ISSUE
//   subframe click resolution

////////////////
// our layout

static Rect base_rect = {{ 13, 11},{153,287}}; // +1 due to bug in canvas-iz-ation
#define COURSE_W        (140)
#define SCREEN_H        (276)
#define COURSE_H        (252)

#define SWING_BTN_H     (12)
#define CLUB_TOP_Y      (COURSE_H+12)

#define CLUB_CNT        (5)   // 5 clubs, each 28 pixels across
#define CLUB_BTN_W      (28)

#define PIX_TO_METER    (2.2)

////////////////
// overall game state

typedef enum eGolfGameMode
 { kGameInit, kGameNewHole, kGameAim, kGameSwinging, kGamePhysics, kGameHoleDone, kGameOver, kGameScores };

static eGolfGameMode  gCurGameMode = kGameInit;
static int            gLastModeChange = 0;

////////////////
// Pixel <-> terrain mapping
#define PIXEL_NULL         (0)
#define PIXEL_TEE         (32)
#define PIXEL_TEEAREA    (102)
#define PIXEL_GREEN      (100)
#define PIXEL_HOLE       (183)
#define PIXEL_SAND       (200)
#define PIXEL_WATER      (131)
#define PIXEL_FAIRWAY     (86)
#define PIXEL_ROUGH      (107)
#define PIXEL_OOB        (111)
#define PIXEL_PATH      (0xd6) // @TBD: not in the art yet, fix this when it is

#define PIXEL_WINDSOCK    (14) // where to place the windsock center...
#define PIXEL_SCORE_3L     (8)
#define PIXEL_SCORE_1L    (11)

/////////////
// terrain system
#define TERRAIN_NULL       (0)
#define TERRAIN_TEE        (1)
#define TERRAIN_TEEAREA    (2)
#define TERRAIN_GREEN      (3)
#define TERRAIN_HOLE       (4)
#define TERRAIN_SAND       (5)
#define TERRAIN_WATER      (6)
#define TERRAIN_FAIRWAY    (7)
#define TERRAIN_ROUGH      (8)
#define TERRAIN_OOB        (9)
#define TERRAIN_PATH       (10)
#define NUM_TERRAIN        (TERRAIN_PATH+1)

#define TERR_FLG_NONE      (0)
#define TERR_FLG_ADJUST    (1<<0)    // if we decide to do cup-lip-roll's w/a flag
#define TERR_FLG_PENALTY   (1<<1)    // penalty stroke, return to exit point
#define TERR_FLG_SAND      (1<<2)    // halve power if not an SW

// default globals...
#define AIR_DRAG       (0.36)  // who the hell knows...
#define MIN_ROLL_VEL   (2.80)  // below this, stop rolling
#define MAX_HOLE_VEL   (8.00)  // max speed at which we can roll into the hole

typedef struct {
   float bounce;       // how bouncy is this terrain
   float drag;         // drag while rolling on this terrain
   float swing_vary;   // how much to auto-adjust swings from this terrain
   float power;        // what to multiply power by on this terrain (need a flag for "the ok club")
   short pixel;        // what pixel color this is
   short flags;        // if we want to do lip out and such this way...
} sGolfTerr;

static sGolfTerr gGolfTerr[]=
{
   {0.00,   3, 0.1,   1, PIXEL_NULL,    TERR_FLG_PENALTY},
   {0.10,   1,   0,   1, PIXEL_TEE,     TERR_FLG_NONE   },
   {0.10,   1,   0,   1, PIXEL_TEEAREA, TERR_FLG_NONE   },
   {0.15, 0.5,   0,   1, PIXEL_GREEN,   TERR_FLG_NONE   },
   {0.00, 1.0,   0,   1, PIXEL_HOLE,    TERR_FLG_ADJUST },
   {0.05,   4, 0.2,   1, PIXEL_SAND,    TERR_FLG_SAND   },  // sand power *0.5 in code
   {0.00,   9,   0,   1, PIXEL_WATER,   TERR_FLG_PENALTY},
   {0.15, 0.7,   0,   1, PIXEL_FAIRWAY, TERR_FLG_NONE   },
   {0.10, 1.5, 0.1, 0.8, PIXEL_ROUGH,   TERR_FLG_NONE   },
   {0.00,   9,   0, 0.7, PIXEL_OOB,     TERR_FLG_PENALTY},
   {0.60,   1, 0.1,   1, PIXEL_PATH,    TERR_FLG_NONE   },
};

//////////////////////////////
// high scores

sFileVarDesc gGolfScoreDesc =
{
   kCampaignVar,
   "GolfScr",
   "Golf High Scores",
   FILEVAR_TYPE(sMiniGameScores),
   {1,0},
   {1,0},
   "shock",
};

cFileVar<sMiniGameScores,&gGolfScoreDesc> gGolfScoreTable;

////////////////
// misc string state
#define GetNamedString(buf,name)           ShockStringFetch(buf,sizeof(buf),name,"minigame")
#define GetNamedStringNum(buf,name,num)    ShockStringFetch(buf,sizeof(buf),name,"minigame",num)

static char gGolfSunkShotMsg[64];
static BOOL gHaventHackedThisHole;  // have we hacked a restart yet this hole

////////////////
// misc env constants

////////////////
// iRes's
IRes *gGolfTerrain=NULL;            // should we random generate, shouldnt we?
IRes *gGolfGolfer=NULL;
IRes *gGolfPin=NULL;

#define COLOR_WHITE     0
#define COLOR_GREEN     1
#define COLOR_GREY      2
#define COLOR_BLACK     3
#define COLOR_RED       4
#define COLOR_DRKBLUE   5
#define COLOR_LGTBLUE   6
#define NUM_COLORS      7
static int colors[NUM_COLORS];
uchar colRGB[]={255,255,255,0,200,0,128,128,128,0,0,0,200,0,0,0,0,80,0,0,200};

////////////////
// state of the world

static Point targ_pt;      // target point in the world for shot
static Point tee_pt;       // where is the tee on this hole
static Point hole_pt;      // where is the hole
static Point self_pt;      // where is the golfer
static Point ball_pt;      // where is the ball
static Point wind_pt;      // where to draw the windsock
static Point score_pt;     // get the score point
static BOOL  score_3l;     // 3line or 1line score

//////////////////////////
// physics info

// the "physics", ahem
typedef struct {
   mxs_vector pos;
   mxs_vector vel;
   mxs_vector acc;
   mxs_vector wind;        // current wind vel
   mxs_vector init_wind;   // initial wind vector
   int        last_gust;   // simtime of last gust
   float      dt;          // time run during this simulation
} sGolfPhysics;

static sGolfPhysics gGolfPhys;

////////////////
// Clubs
#define CLUB_DRIVER    0
#define CLUB_SHORTIRON 1
#define CLUB_LONGIRON  2
#define CLUB_SANDWEDGE 3
#define CLUB_PUTTER    4
#define NUM_CLUBS     (CLUB_PUTTER+1)

typedef struct {
   float      vel;         // xyvel, really
   float      z_vec;       // assuming unit vector in xy
   float      meter_speed; // speed of the swing meter
   float      expected_d;  // expected length of full power shot in meters
} sGolfClub;

// @TBD: this is way too long, but oh well, for now we need it w/no subframe accuracy
#define METER_FULL_MS  (800.0) // 3/4 seconds to pull back, div by meter_speed

// ms to reach full!
#define ClubSpeed()   ((float)METER_FULL_MS/gGolfClubs[cur_club].meter_speed)

// once physics is tuned, this needs massive tuning
// the current 500m driver is a little dodgy
// the "expected_d" numbers are _TOTALLY MADE UP_, do not believe them!
static sGolfClub gGolfClubs[NUM_CLUBS]=
{
   {102, 0.27, 1.20, 265},   // Driver     (2W)
   { 79, 0.51, 1.00, 210},   // Long Iron  (3I)
   { 58, 0.78, 1.00, 150},   // Short Iron (7I)
   { 32, 1.20, 0.85,  78},   // SandWedge  (SW)
   { 23, 0.00, 0.65,  66},   // Putter      (P)
};

/////////////////////
// player score
#define NUM_HOLES 9
static int par_per_hole[NUM_HOLES];    // parsed out of a string var
static int score_per_hole[NUM_HOLES];
static int total_shots;
static int cur_shots;
static int cur_hole;
static int cur_course;
static int cur_club;

////////////////
// scoring may not be as easy here... what with the card and all...

int  _par_through_hole(int hole)
{
   int par=0;
   for (int i=0; i<hole; i++)
      par+=par_per_hole[i];
   return par;
}

void _make_score_string(char *buf, int *scores, int hole_cnt)
{
   char *p=buf;
   for (int i=0; i<hole_cnt; i++)
      if (scores[i]>9)
         *p++='X';
      else
         *p++='0'+scores[i];
   *p='\0';      // full string needs to end in \0, of course
}

void _add_high_score(void)
{
   char buf[128], form_buf[64];
   GetNamedString(form_buf,"gf_score_long");
   sprintf(buf,form_buf,total_shots-_par_through_hole(NUM_HOLES),total_shots);
   _make_score_string(buf+strlen(buf),score_per_hole,NUM_HOLES);
   ShockMiniGameAddHighScore(&gGolfScoreTable,10000-total_shots,buf);
}  // need to sort in reverse order, obviously... hence the 10000- hack

void _draw_scores(Point pt)
{
   Point drawpt;
   char buf[128];
   drawpt.x=pt.x; // +base_rect.ul.x;  // or something
   drawpt.y=pt.y; // +base_rect.ul.y;
   ShockMiniGameDisplayHighScores(&gGolfScoreTable, drawpt.x+9, drawpt.y+15);
   drawpt.y=pt.y+base_rect.lr.y-base_rect.ul.y;
   gr_font_string(gShockFont, gMiniGameScoreMsg, drawpt.x+23, drawpt.y-51);
   GetNamedString(buf,"gf_show_par");
   strcat(buf," "); // duh, i suck
   _make_score_string(buf+strlen(buf),par_per_hole,NUM_HOLES);
   gr_font_string(gShockFont, buf, drawpt.x+27, drawpt.y-30);
}

void _display_running_score(Point pt)
{
   char form_buf[64], buf[64];
   int use_hole=(gCurGameMode==kGameHoleDone)?cur_hole:cur_hole-1;
   if (score_3l)
      GetNamedString(form_buf,"gf_status_3line");
   else
      GetNamedString(form_buf,"gf_status");
   sprintf(buf,form_buf,cur_hole,cur_shots,total_shots-_par_through_hole(use_hole));

   gr_set_fcolor(colors[COLOR_WHITE]);
   gr_font_string(gShockFontMono, buf, pt.x+score_pt.x, pt.y+score_pt.y);

   if (gGolfSunkShotMsg[0]!='\0')
      gr_font_string(gShockFontMono, gGolfSunkShotMsg, pt.x+2, pt.y+COURSE_H-13);
}

//////////////
// course access functions

grs_bitmap *GetCourse(void)
{
   if (gGolfTerrain==NULL)
   {
      Warning(("Get Course w/No Course\n"));
      return NULL;
   }
   return (grs_bitmap *)gGolfTerrain->Lock();
}

void UnGetCourse(void)
{
   gGolfTerrain->Unlock();
}

#define GetTerrAtXY(bm,x,y) (bm->bits[(bm->w*y)+x])

int PixelToTerrain(int pix_col)
{
   for (int i=0; i<NUM_TERRAIN; i++)
      if (gGolfTerr[i].pixel==pix_col)
         return i;
   return TERRAIN_NULL;
}

// return Terrain for point on course (relative to bm)
int ExamineCourse(int x, int y)
{
   grs_bitmap *bm=GetCourse();
   if (bm==NULL) return TERRAIN_NULL;
   int val=GetTerrAtXY(bm,x,y);
   UnGetCourse();
   return PixelToTerrain(val);
}

#define ExamineCoursePt(pt) (ExamineCourse(pt.x,pt.y))

////////////////
// create the world - do we just draw holes?  or random generate?
// for now, we assume it is drawn, color defines meaning

////////////////
// swing system
// here is the "gameplay", namely the swing reader...
// w/o subframe click timing, this will really really suck.. prob should do this in draw...

typedef enum eGolfSwingFSM { kGolfFSMWait, kGolfFSMWindup, kGolfFSMSwing, kGolfFSMDone };

eGolfSwingFSM swing_state=kGolfFSMWait;

static float  swing_power=1.0;
static float  swing_aim=0.0;
static float  swing_meter=0.0;
static int    swing_start=0;    // in ms...
static int    swing_frame=0;    // last frame time - for when we do own mouse, i guess

// for swingmeter craziness
#if 0
   bool downEvent;
   bool button;
   mouse_check_btn(0,&button);
   downEvent = (button&&!m_lastButton);
   m_lastButton = button;
#endif

#define SWING_LEFT_X 18
#define SWING_WIDTH  104
#define MAX_SWING_METER  ((float)(COURSE_W-SWING_LEFT_X)/(float)(SWING_WIDTH))
#define MIN_SWING_METER  (-((float)(SWING_LEFT_X)/(float)(SWING_WIDTH)))

// returns if we are done
BOOL _SwingButton(int tm)
{
   switch (swing_state)
   {
      case kGolfFSMWait:   // waiting for user to really start
         swing_start=tm;
         swing_state=kGolfFSMWindup;
         swing_meter=0;
         break;
      case kGolfFSMWindup: // ok, pulling back...
         swing_power=(float)(tm-swing_start)/ClubSpeed();
         if (swing_power>MAX_SWING_METER) swing_power=MAX_SWING_METER;
         swing_start=tm;  // now we are moving down...
         swing_state=kGolfFSMSwing;
         break;
      case kGolfFSMSwing:  // coming back down
         if (swing_meter<0.75)  // dumb way to ignore click at top of swing near 1.2
         {  // power is 0-1, sub off 0-1 we have gone back down
            swing_aim=swing_power-((float)(tm-swing_start)/ClubSpeed());
            swing_state=kGolfFSMDone;
         }
         if (swing_aim<MIN_SWING_METER) swing_aim=MIN_SWING_METER;
         return TRUE;
         break;
      case kGolfFSMDone:   // fire away...
         // just ignore these, not clear we should ever get them, really
         break;
   }
   return FALSE;
}

// all we care about is button state, and our own state, of course...
// returns if we are done
BOOL _CheckSwing(Point pt)
{
   BOOL are_done=FALSE;
   if (gCurGameMode==kGameSwinging)
   {
      int tm=tm_get_millisec();
      
      // update meter / fsm
      if (swing_state==kGolfFSMWindup)
      {
         swing_meter=((float)(tm-swing_start))/ClubSpeed();
         if (swing_meter>MAX_SWING_METER)
         {
            swing_power=MAX_SWING_METER;
            swing_start=tm;
            swing_state=kGolfFSMSwing;
         }
      }
      else if (swing_state==kGolfFSMSwing)
      {
         swing_meter=swing_power-((tm-swing_start)/ClubSpeed());
         if (swing_meter<MIN_SWING_METER)
         {
            swing_aim=MIN_SWING_METER;
            swing_state=kGolfFSMDone;
            are_done=TRUE;
         }
      }
   }

   // now the draw itself...
   pt.x+=1;
   pt.y+=COURSE_H+1;  // get to top of swing button
   int bty=pt.y+SWING_BTN_H-2;
   
   gr_set_fcolor(colors[COLOR_WHITE]);
   gr_rect(pt.x,pt.y,pt.x+COURSE_W-2,bty);

   gr_set_fcolor(colors[COLOR_BLACK]);
   gr_vline(pt.x+SWING_LEFT_X,pt.y,bty);

   gr_set_fcolor(colors[COLOR_RED]);   
   gr_vline(pt.x+SWING_LEFT_X+SWING_WIDTH,pt.y,bty);

   // now draw in the current state of the meter, if we have one...
   if (swing_state!=kGolfFSMWait)
   {
      if (swing_state>kGolfFSMWindup)
      {
         gr_set_fcolor(colors[COLOR_DRKBLUE]);
         gr_rect(pt.x+SWING_LEFT_X,pt.y,pt.x+SWING_LEFT_X+(swing_power*SWING_WIDTH),bty);
         gr_set_fcolor(colors[COLOR_BLACK]);  // draw power
         gr_vline(pt.x+SWING_LEFT_X+(swing_power*SWING_WIDTH),pt.y,bty);
      }

      float use_meter=(swing_state!=kGolfFSMDone)?swing_meter:swing_aim;
      if (use_meter>MAX_SWING_METER)
         use_meter=MAX_SWING_METER;
      else if (use_meter<MIN_SWING_METER)
         use_meter=MIN_SWING_METER;

      int meter_color=swing_state==kGolfFSMWindup?COLOR_DRKBLUE:COLOR_LGTBLUE;
      gr_set_fcolor(colors[meter_color]);
      if (swing_meter>0)
         gr_rect(pt.x+SWING_LEFT_X,pt.y,pt.x+SWING_LEFT_X+(use_meter*SWING_WIDTH),bty);
      else
         gr_rect(pt.x+SWING_LEFT_X+(use_meter*SWING_WIDTH),pt.y,pt.x+SWING_LEFT_X,bty);
   }
   return are_done;
}

void _StartSwing(void)
{
   swing_state=kGolfFSMWait;
}

BOOL BallAtHole(void)
{
   for (int i=-1; i<=1; i++)
      for (int j=-1; j<=1; j++)      
         if (ExamineCourse(ball_pt.x+i,ball_pt.y+j)==TERRAIN_HOLE)
            return TRUE;
   return FALSE;
}

// we are done! ball is over! go home! etc...
void FinishBallPhys(void)
{
   if (BallAtHole())
   {  
      char buf[64], form_buf[64];
      int off=cur_shots-par_per_hole[cur_hole-1];
      if (cur_shots==1) // hole in one
         GetNamedString(buf,"gf_hole_one");
      else if ((off>=-2)&&(off<=2))
         GetNamedStringNum(buf,"gf_sink_",off+3);
      else
      {
         if (off>0)
            GetNamedString(form_buf,"gf_over");
         else
            GetNamedString(form_buf,"gf_under");
         sprintf(buf,form_buf,off);
      }
      strcpy(gGolfSunkShotMsg,buf);
      total_shots+=cur_shots;
      score_per_hole[cur_hole-1]=cur_shots;
      gCurGameMode=kGameHoleDone;
   }
   else
   {  // hmmm, are we in penalty terrain
      int hit_pt_terr=ExamineCoursePt(ball_pt);
      if (gGolfTerr[hit_pt_terr].flags&TERR_FLG_PENALTY)
      {  // back ball up towards the golfer till you hit != WATER
         float bx=ball_pt.x, by=ball_pt.y, tx=self_pt.x, ty=self_pt.y;
         float dx=tx-bx, dy=ty-by;
         float dist=sqrt(dx*dx+dy*dy);
         dx/=dist; dy/=dist;
         while (fabs(bx-tx)+fabs(by-ty)>3)
         {
            bx+=dx; by+=dy;
            int terr=ExamineCourse((int)bx,(int)by);
            if ((gGolfTerr[terr].flags&TERR_FLG_PENALTY)==0)
               break;
         }
         ball_pt.x=(int)bx;
         ball_pt.y=(int)by;
         cur_shots++;         // the penalty shot itself
         if (hit_pt_terr==TERRAIN_WATER)
            GetNamedString(gGolfSunkShotMsg,"gf_water");
         else
            GetNamedString(gGolfSunkShotMsg,"gf_oob");
      }
      self_pt=ball_pt;        // should move the golfer, huh?
      gCurGameMode=kGameAim;
      targ_pt=hole_pt;
      cur_shots++;            // misc scoring updates
   }
}

// for now, be total morons...
void SimulateBallPhysics(float dt)
{
   sGolfPhysics *p=&gGolfPhys;

   if (dt>0.25) dt=0.25;  // @TBD: currently our "drag" "equation" breaks horribly if dt is too big
   p->dt+=dt;

   // core "integrator"
   mx_scale_addeq_vec(&p->vel,&p->acc,dt);
   mx_scale_addeq_vec(&p->pos,&p->vel,dt);
#ifdef DBG_ON
   if (!config_is_defined("golf_no_wind"))
#endif      
      if (p->acc.z!=0)  // we are in the air
         mx_scale_addeq_vec(&p->pos,&p->wind,dt*0.20);  // wind is 50%

   // now, put the ball where the pos is...
   ball_pt.x=p->pos.x/PIX_TO_METER;
   ball_pt.y=p->pos.y/PIX_TO_METER;
   
   // check for bounce/landing/so on
   if (p->pos.z<0) // have hit the ground
   {
      p->pos.z=0;
      p->vel.z*=-gGolfTerr[ExamineCoursePt(ball_pt)].bounce;
      p->vel.x*=1.3;
      p->vel.y*=1.3;
      if (p->vel.z<4.0)  // not moving too fast
         p->vel.z=0;
      //
      if (p->vel.z==0)
      {
         p->acc.z=0;  // now we are rolling
         // zero the whole thing, for now?
      }
   }

   // @TBD: this is horrific.  drag could, in theory, send something backwards
   // HAVE TO FIX THIS...

   float acc_damp=0.15;
   
   // drag "model"   @TBD: do real physics-y stuff here, not these hacks!
   if (p->acc.z==0)  // we are rolling
   {
      float drag=gGolfTerr[ExamineCoursePt(ball_pt)].drag;
      p->vel.x*=(1.0-drag*dt);   // @TBD this is an idiotic drag equation!
      p->vel.y*=(1.0-drag*dt);   // argh argh argh
      if (fabs(p->vel.x)+fabs(p->vel.y)<MIN_ROLL_VEL) // if rolling and going slow...
         FinishBallPhys();
      acc_damp=0.4;
   }
   else
   {
      p->vel.x*=(1.0-AIR_DRAG*dt);    // well, this might be better
      p->vel.y*=(1.0-AIR_DRAG*dt);
   }

   // and lets crank down the acceleration in attempt to avoid infinite roll
   p->acc.x*=1.0-(acc_damp*dt);  // ???? why not... woo woo
   p->acc.y*=1.0-(acc_damp*dt);  //      ?

   // if we hit the hole, if going slow enough, you win
   if (p->pos.z==0)  // we are on the ground, at least for this frame
      if (ExamineCoursePt(ball_pt)==TERRAIN_HOLE)
      {
         float spd=sqrt(p->vel.x*p->vel.x+p->vel.y*p->vel.y);
         if (spd<MAX_HOLE_VEL)
            FinishBallPhys();
         else
         {
#ifdef DBG_ON      
            mprintf("LipOut! speed %g from (%g %g)\n",spd,p->vel.x,p->vel.y);
#endif
            float new_ang=RandFloat()*2*3.14159265358979323846;
            p->vel.x=cos(new_ang)*spd*0.6;
            p->vel.y=sin(new_ang)*spd*0.6;
         }
      }
   
#ifdef DBG_ON
   if (config_is_defined("golf_physics"))
      mprintf("%d B %d %d, p %5g %5g %5g vel %5g %5g %5g acc %5g %5g %5g\n",
              (int)(1000*p->dt),ball_pt.x,ball_pt.y,p->pos.x,p->pos.y,p->pos.z,
              p->vel.x,p->vel.y,p->vel.z,p->acc.x,p->acc.y,p->acc.z);
#endif
   
   if (p->dt>15.0) // no more than 15s, safety pup says
      FinishBallPhys();

   BOOL in_bounds_x=FALSE, in_bounds_y=FALSE;
   if (ball_pt.x<0)
      ball_pt.x=0; 
   else if (ball_pt.x>=COURSE_W)
      ball_pt.x=COURSE_W-1;
   else
      in_bounds_x=TRUE;
   
   if (ball_pt.y<0)
      ball_pt.y=0; 
   else if (ball_pt.y>=COURSE_H)
      ball_pt.y=COURSE_H-1;
   else
      in_bounds_y=TRUE;

   if (!(in_bounds_x&&in_bounds_y))
      FinishBallPhys();    // if not in bounds in x and y
}                          //  you hit edge of the world....   


#ifdef DBG_ON
float golf_dbg_vel, golf_dbg_z_vec;
#endif

void StartBallPhysics(void)
{
   sGolfPhysics *p=&gGolfPhys;
   float use_power=swing_power, use_aim=swing_aim;
   mxs_vector aim_vec;

#ifdef DBG_ON   
   if (config_is_defined("golf_perfect_swing"))
    { use_power=1.0; use_aim=0.0; }
#endif   
   
   // get the target vector
   p->vel.x=(targ_pt.x-ball_pt.x);         // get into mks
   p->vel.y=(targ_pt.y-ball_pt.y);
   p->vel.z=0;
   mx_normeq_vec(&p->vel);
   aim_vec.x=p->vel.y; aim_vec.y=-p->vel.x; aim_vec.z=0; // aim left/right is 90degrees off
   float use_vec=gGolfClubs[cur_club].z_vec;    // post norm, add the z_vec
   float use_vel=gGolfClubs[cur_club].vel;
#ifdef DBG_ON
   if (config_is_defined("golf_debug_clubs"))
    { use_vec=golf_dbg_z_vec; use_vel=golf_dbg_vel; }
#endif   
   
   p->vel.z=use_vec;
   mx_scaleeq_vec(&p->vel,use_vel);

   // multiply vel by swing_power
   if (use_power>1)
   {  // extra power, but worse aim
      float swing_break=((Rand()%65)-32);   // -32 to 32
      if (use_power<1.08) swing_break/=2;   // only do full screw up at max power...
      use_aim+=(use_aim/12.0)*swing_break;  // so, screw with it, basically
      use_power=1+(use_power-1)*1.2;        // and non-linear power up top...
   }
   use_power*=gGolfTerr[ExamineCoursePt(ball_pt)].power;
   if (gGolfTerr[ExamineCoursePt(ball_pt)].flags&TERR_FLG_SAND)
      if (cur_club!=CLUB_SANDWEDGE)
         use_power*=0.4;
   mx_scaleeq_vec(&p->vel,use_power);    // swing power multiply

   p->pos.x=(ball_pt.x*PIX_TO_METER);               // get into mks
   p->pos.y=(ball_pt.y*PIX_TO_METER);
   p->pos.z=0;

   // perturb by the swing performance
   // set up the acceleration
   p->acc.x=p->acc.y=0;
   p->acc.z=-9.8;

   // @TBD: make this much more useful
   // and multiple swing_aim by Rand of gGolfTerr[ExamineCoursePt(ball_pt)].swing_vary
   float final_aim;
   if (fabs(use_aim)>0.09)
      final_aim=use_aim*18;
   else
      final_aim=use_aim*12;
   if (cur_club==CLUB_PUTTER) final_aim*=0.35;  // better aim with putter...
   mx_scaleeq_vec(&aim_vec,-final_aim);  // woo woo
   mx_addeq_vec(&p->acc,&aim_vec);  // who the hell knows....

   // a new swing
   p->dt=0;

   // go into physics mode
   gCurGameMode=kGamePhysics;   

#ifdef DBG_ON
   if (config_is_defined("golf_swing"))
   {
      char buf[256];
      sprintf(buf,"swing %4.4g %4.4g, use %4.4g %4.4g, final %4.4g (acc %3.3g %3.3g %3.3g)\n",
              swing_power,swing_aim,use_power,use_aim,final_aim,p->acc.x,p->acc.y,p->acc.z);
      mprintf(buf);
   }
   
   if (config_is_defined("golf_physics"))
      mprintf("Setup c %d t %d %d b %d %d u %g %g s %g %g\n p %g %g %g vel %g %g %g acc %g %g %g\n",
              cur_club,targ_pt.x,targ_pt.y,ball_pt.x,ball_pt.y,use_power,use_aim,swing_power,swing_aim,
              p->pos.x,p->pos.y,p->pos.z,p->vel.x,p->vel.y,p->vel.z,p->acc.x,p->acc.y,p->acc.z);
#endif   
}

// call on each new hole to pick initial wind values
void InitWind(void)
{
   sGolfPhysics *p=&gGolfPhys;
   float wind_ang=RandFloat()*2*3.14159265358979323846;
   float wind_mag=10+(RandFloat()*15);
   p->wind.z=0;
   p->wind.x=cos(wind_ang)*wind_mag;
   p->wind.y=sin(wind_ang)*wind_mag;
   p->init_wind=p->wind;
}

#define GUST_DUR 500

// simulate the wind during hole play
void SimulateWind(void)
{
   sGolfPhysics *p=&gGolfPhys;
   float base=0.95;
   int tm=GetSimTime();
   if (p->last_gust+4000<tm)
   {  // we can gust again, if we want
      if ((Rand()&0x7f)==0)
         p->last_gust=tm;
   }
   if (p->last_gust+GUST_DUR>=tm)
   {
      base=0.74;
   }
   p->wind.x*=base+(RandFloat()*(1-base)*2);
   p->wind.y*=base+(RandFloat()*(1-base)*2);
   if (p->last_gust+GUST_DUR<tm)
   {
      mxs_vector diff;
      mx_sub_vec(&diff,&p->init_wind,&p->wind);
      mx_scaleeq_vec(&diff,RandFloat()*0.5);  // go 0-1/2way back to initial...
      mx_addeq_vec(&p->wind,&diff);
   }
}

// @TBD: draw windsock as little triangle in lower left (right above swing meter...)
// i guess we are gonna need a gr_poly here
// or i guess we could do 2 or 3 lines or something

#define _sock_pt(c,a)     fix_make(pt.a+wind_pt.a,0)+fix_from_float(corners[c].a)
#define _sock_line(c1,c2) gr_fix_line(_sock_pt(c1,x),_sock_pt(c1,y),_sock_pt(c2,x),_sock_pt(c2,y))

// if we were cool, wed have a "distance to pin" under the windsock too
// in a readable font, as well
void _draw_windsock_n_distance(Point pt)
{
   mxs_vector corners[4];
   corners[0].x=gGolfPhys.wind.x*0.8;
   corners[0].y=gGolfPhys.wind.y*0.8;
   corners[1].x=-corners[0].x/3; corners[1].y=-corners[0].y/3;
   corners[2].x=-corners[1].y;   corners[2].y=corners[1].x;
   corners[3].x=-corners[2].x;   corners[3].y=-corners[2].y;
   gr_set_fcolor(colors[COLOR_WHITE]);
   _sock_line(0,1);
   _sock_line(2,3);
   _sock_line(2,0);
   _sock_line(3,0);

   // needs to go under the sock, someday...
   char form_buf[64], buf[64];
   GetNamedString(form_buf,"gf_dist");
   int dx=hole_pt.x-self_pt.x, dy=hole_pt.y-self_pt.y;
   int dist=(int)sqrt((float)(dx*dx+dy*dy));     // get distance to hole...
   sprintf(buf,form_buf,(int)(dist*PIX_TO_METER));
   gr_set_fcolor(colors[COLOR_WHITE]);
   gr_font_string(gShockFontMono, buf, pt.x+wind_pt.x+2-14, pt.y+wind_pt.y+2+10);
   GetNamedString(buf,"gf_show_par");
   sprintf(buf+strlen(buf),"%d",par_per_hole[cur_hole-1]);
   gr_font_string(gShockFontMono, buf, pt.x+wind_pt.x+2-14, pt.y+wind_pt.y+2+20);
   
}

////////////////////////
// actually start out a hole...
Point FindPixel(int pixel)
{
   grs_bitmap *bm=GetCourse();
   Point pt;
   pt.x=pt.y=-1;
   if (bm==NULL) return pt;
   for (int j=0; j<COURSE_H; j++)
      for (int i=0; i<COURSE_W; i++)
         if (GetTerrAtXY(bm,i,j)==pixel)
         {
            pt.x=i;
            pt.y=j;
            break;
         }
   UnGetCourse();
   return pt;
}

Point FindTerr(int terr)
{
   return FindPixel(gGolfTerr[terr].pixel);
}

void ReloadHole(void)
{
   SafeFreeHnd(&gGolfTerrain);  // get rid of the last hole
   char buf[32];
   sprintf(buf,"gfhol%1.1d%2.2d",cur_course,cur_hole);
   gGolfTerrain=LoadPCX(buf);
}

// hey, weve made it to the next hole, so start it up
// load the hole by course/hole id, then go find the tee and such
void GolfStartHole(void)
{
   ReloadHole();
   ball_pt=self_pt=tee_pt=FindTerr(TERRAIN_TEE);
   targ_pt=hole_pt=FindTerr(TERRAIN_HOLE);
   wind_pt=FindPixel(PIXEL_WINDSOCK);
   if (wind_pt.x==-1)
    { wind_pt.x=COURSE_W-25; wind_pt.y=COURSE_H-45; }
   score_pt=FindPixel(PIXEL_SCORE_3L);
   if (score_pt.x==-1)
   {
      score_pt=FindPixel(PIXEL_SCORE_1L);
      score_3l=FALSE;
   }
   else
      score_3l=TRUE;
   if (score_pt.x==-1)
    { score_pt.x=2; score_pt.y=2; }
   InitWind();  // actually set up the wind vectors
   gCurGameMode=kGameAim;
   cur_shots=1;     // reset shot counter
   cur_club=CLUB_DRIVER; // always start w/driver
   gGolfSunkShotMsg[0]='\0';
   gHaventHackedThisHole=TRUE;
#ifdef DBG_ON
   if (config_is_defined("golf_hole_dist"))
   {
      int dx=hole_pt.x-self_pt.x, dy=hole_pt.y-self_pt.y;
      int dist=(int)sqrt((float)(dx*dx+dy*dy));     // get distance to hole...
      mprintf("Hole %d dist %d\n",(int)(dist*PIX_TO_METER));
   }
#endif   
}

void SetupRound(int course)
{
   memset(score_per_hole,0,sizeof(score_per_hole));
   total_shots=cur_shots=0;
   cur_course=course;
   cur_hole=1;
   gCurGameMode=kGameNewHole;
   // go get par scores
   char buf[64];
   GetNamedStringNum(buf,"gf_par_c_",course);
   for (int i=0; i<NUM_HOLES; i++)
      par_per_hole[i]=buf[i]-'0';  // get par table....
}

///////////////
// for now, draw just draws the bitmap, then puts the player Dot and aim arrow over it
void GolfDraw(Point pt)
{
   Point ipt=pt;
   ShockMiniGameSetupCanvas(ipt,base_rect,-1);
   pt.x=pt.y=0;
   
   if (gCurGameMode==kGameScores)
      _draw_scores(pt);

   if (gCurGameMode==kGameNewHole)
      GolfStartHole();

   if (gCurGameMode==kGamePhysics||gCurGameMode==kGameAim||gCurGameMode==kGameSwinging)
      SimulateWind();

   if (gCurGameMode==kGamePhysics) // run the ball simulation
      SimulateBallPhysics(GetSimFrameTime()/1000.0);

   if ((gCurGameMode!=kGameScores)&&(gCurGameMode!=kGameInit))
   {
      Point draw_self, drawpt;

      // get to the stupid rect we should just get as a subcanvas in the first place, duh!
      //      pt.x+=base_rect.ul.x;
      //      pt.y+=base_rect.ul.y;
      
      // draw the terrain
      DrawByHandle(gGolfTerrain,pt);
      
      // draw the golfer
      draw_self.x=pt.x+self_pt.x;
      draw_self.y=pt.y+self_pt.y;
      draw_self.y-=15;             // move up so the bitmap is right
      DrawByHandle(gGolfGolfer,draw_self);                           
      draw_self.y+=15;             // move back so line later is right

      // draw the pin if the ball is far enough away
      if ((abs(ball_pt.x-hole_pt.x)+abs(ball_pt.y-hole_pt.y))>50)
      {
         drawpt.x=pt.x+hole_pt.x;
         drawpt.y=pt.y+hole_pt.y-7;
         DrawByHandle(gGolfPin,drawpt);
      }

      // show/update the swing meter      
      if (gCurGameMode==kGameSwinging||gCurGameMode==kGamePhysics)
         if (_CheckSwing(pt))
            StartBallPhysics();
      
      gr_set_fcolor(colors[COLOR_WHITE]);

      // show currently selected club
      drawpt.x=pt.x+cur_club*CLUB_BTN_W;
      drawpt.y=pt.y+CLUB_TOP_Y;
      gr_box(drawpt.x+2,drawpt.y+1,drawpt.x+CLUB_BTN_W-1,drawpt.y+12);

      // show the windsock, n such
      _draw_windsock_n_distance(pt);

      // and of course, show current situation
      _display_running_score(pt);
      
      // @TBD: draw the ball (w/Z) - this should be _NONLINEAR_ in z!
      float ball_rad=(gGolfPhys.pos.z/12.5); // for now, who the hell knows
      // really, these should use the gGolfPhys if we are in the air...
      fix ballx=fix_make(pt.x+ball_pt.x,0), bally=fix_make(pt.y+ball_pt.y,0);
      fix rad=0x80+(0x20*ball_rad); // I TOTALLY DONT GET THE rad PARAMETER
      rad=(int)(rad*64.0/14.0);     // this math was done back when we were full screen canvas!
      gr_disk(ballx,bally,rad);     // i dare you to read the comment in g2.h and get it
                                    // especially the (r/320)'s everywhere...

      // for now, to better see what is going on...
      BOOL draw_aim_line=gCurGameMode==kGameAim;
#ifdef DBG_ON
      if (config_is_defined("golf_aim_phys"))
         draw_aim_line|=gCurGameMode==kGamePhysics;
#endif
      if (draw_aim_line)
      {
         int dx=targ_pt.x-self_pt.x, dy=targ_pt.y-self_pt.y;
         float mag=sqrt(dx*dx+dy*dy);
         float diff=(gGolfClubs[cur_club].expected_d/PIX_TO_METER)/mag;
         dx*=diff; dy*=diff;
         gr_int_line(draw_self.x,draw_self.y,draw_self.x+dx,draw_self.y+dy);
      }
   }

   ShockMiniGameBlitCanvas(ipt,base_rect);
}

///////////////
// only used for UI, we poll for swing, i think
void GolfDragDrop(Point pt, BOOL start)
{
   if (gCurGameMode!=kGameSwinging)
      if (start)
         return;  // ignore downs except during swing meter

   // clear the message
   gGolfSunkShotMsg[0]='\0';

   switch (gCurGameMode)
   {
      case kGameAim:
         if (pt.y-base_rect.ul.y>COURSE_H)
         {
            int y_pos=pt.y-base_rect.ul.y;
            int x_pos=pt.x-base_rect.ul.x;
            if (y_pos>CLUB_TOP_Y)
            {
               cur_club=x_pos/CLUB_BTN_W;
               if (cur_club>NUM_CLUBS-1) cur_club=NUM_CLUBS-1;
            }
            else
            {
               _StartSwing();
               gCurGameMode=kGameSwinging;  // start swing...
            }
         }
         else
         {
            targ_pt.x=pt.x-base_rect.ul.x;
            targ_pt.y=pt.y-base_rect.ul.y;
         }
         break;

      case kGameSwinging:
         if ((pt.y-base_rect.ul.y>CLUB_TOP_Y)&&(swing_state==kGolfFSMWait))
         {  // abort swing attempt if you click in the club window
            gCurGameMode=kGameAim;
         }
         else
         {
            // @TBD: THIS SUCKS!
            // first off, need subframe
            // second off, this is up events, not down
            // third off, we should do this in the fsm, and draw a real final line
            // THIS IS UNSHIPPABLE
            _SwingButton(tm_get_millisec());
            if (swing_state==kGolfFSMDone)
               StartBallPhysics();
         }
         break;
         
      case kGameHoleDone:
         // should time out
         if (++cur_hole>NUM_HOLES)
         {
            _add_high_score();
            gCurGameMode=kGameScores;     // course done - ahh, do something...
            gLastModeChange=GetSimTime();
         }
         else
            gCurGameMode=kGameNewHole;  // go to next hole
         break;

      case kGameScores:
         if (gLastModeChange+3000<GetSimTime())
            SetupRound(0);
         break;
   }
}

void GolfRightMouse(Point pt)
{
#ifndef DBG_ON   
   if (ShockMiniGameIsHacked())
      if (gHaventHackedThisHole)
#endif
      {
         GolfStartHole();   // allow restart of hole, for now (maybe for hacked?)
         gHaventHackedThisHole=FALSE;
      }
}

#define MIX_COLOR(r,g,b) (grd_bpp==15)?((r>>3)<<10)|((g>>3)<<5)|(b>>3):((r>>3)<<11)|((g>>2)<<5)|(b>>3)

//////////////////////////////
// color hack

static void pick_g2_colors(void)
{
   for (int i=0; i<NUM_COLORS; i++)
      colors[i]=MIX_COLOR(colRGB[i*3+0],colRGB[i*3+1],colRGB[i*3+2]);
}

//////////////////////////////
// init/term

void GolfInit(void)
{
   if (gCurGameMode==kGameInit)
      SetupRound(0);   // which course
   else
      ReloadHole();
   //   gGolfGolfer=LoadPCX("gfgolfer");
   gGolfGolfer=LoadPCX("gfgolf2");
   gGolfPin=LoadPCX("gfflag");
   pick_g2_colors();
}

void GolfTerm(void)
{
   SafeFreeHnd(&gGolfTerrain);
   SafeFreeHnd(&gGolfGolfer);
   SafeFreeHnd(&gGolfPin);
}
