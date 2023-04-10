/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/headmove.c,v 1.29 2000/02/19 12:36:57 toml Exp $
//

#include <math.h>

#include <mouse.h>
#include <2d.h>
#include <config.h>

#include <mprintf.h>

#include <osystype.h>
#include <wr.h>
#include <objpos.h>
#include <camera.h>
#include <headmove.h>
#include <physapi.h>
#include <playrobj.h>
#include <matrix.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// convert from percentage of screen mouse moved to head rate
// per 100 ms...
static int mouse_head_def_speed=0x6000;   // @OPTPANEL: should be on options panel slider
static int mouse_head_def_dead_zone=0;
static int mouse_head_speed=0; // these get set in headmoveInit();
static int mouse_head_dead_zone=0;

static float mouse_rel_x;
static float mouse_rel_y;

#define HEADRATE  (mouse_head_speed)
#define DEAD_ZONE (mouse_head_dead_zone)
#define KBD_TILT_TOP_CLAMP 50.0
#define KBD_TILT_BOTTOM_CLAMP (-1 * KBD_TILT_TOP_CLAMP)

static float keyboard_turn;
static int keyboard_cur_vel;
static float keyboard_tilt;
static int keyboard_tilt_cur_vel;

void headmoveSetKeyboardInput(float turn_rate)
{
  keyboard_turn = turn_rate;
}

void headmoveSetKeyboardTilt(float tilt_rate)
{
  if (tilt_rate > KBD_TILT_TOP_CLAMP)
    keyboard_tilt = KBD_TILT_TOP_CLAMP;
  else
    if (tilt_rate < KBD_TILT_BOTTOM_CLAMP)
      keyboard_tilt = KBD_TILT_BOTTOM_CLAMP;
    else
      keyboard_tilt = tilt_rate;
}  


void headmoveKeyboardReset(void)
{
   keyboard_turn = 0;
   keyboard_cur_vel = 0;
}

void headmoveKeyboardTiltReset(void)
{
  keyboard_tilt = 0;
  keyboard_tilt_cur_vel = 0;
}

// fixangs per second  (actually per 1024/1000 of a second)
#define KBD_ACCELERATE_CHANGE_DIR (kbd_accelerate_change_dir * 0x100)
#define KBD_DECELERATE (kbd_decelerate * 0x100)
#define KBD_ACCELERATE (kbd_accelerate * 0x100)
#define KBD_TURN_RATE  (kbd_turn_rate * 0x100)
// going to use ACCEL/DECEL for tilt as we do for turn (for the moment).
#define KBD_TILT_RATE  (kbd_tilt_rate * 0x100)

// to make these nicer in config files, they're pre-divided by 0x100
static int kbd_decelerate = 0x300;   // 768
static int kbd_accelerate = 0x200;   // 512
static int kbd_accelerate_change_dir = 0x400;  // 1024
static int kbd_turn_rate  = 0x30;   // 48
static int kbd_tilt_rate = 0x30; // 48


//code mostly copied from headmoveComputeKeyboardTurn
int headmoveComputeKeyboardTilt(int ms)
{
  float scale;
  int desired, final, tilt;

  // fast out if player never uses keyboard lookup/down
  if (keyboard_tilt == 0 && keyboard_tilt_cur_vel == 0)
    return 0;
  
  // compute desired turn rate
  desired = KBD_TILT_RATE * -keyboard_tilt;

  if (desired == keyboard_tilt_cur_vel) 
    {
      final = keyboard_tilt_cur_vel;
    }
  else
    {
      int rate;
      if (desired == 0)
	rate = KBD_DECELERATE;
      else if ((keyboard_tilt_cur_vel > 0 && desired < 0) ||
	       (keyboard_tilt_cur_vel < 0 && desired > 0))
	rate = KBD_ACCELERATE_CHANGE_DIR;
      else
	rate = KBD_ACCELERATE;

      // apply 'rate' change for this long
      if (desired < keyboard_cur_vel)
	{
	  final = keyboard_tilt_cur_vel - rate*ms/1024;
	  if (final < desired)
	    final = desired; //clamp at desired rate
	}
      else
	{
	  final = keyboard_tilt_cur_vel + rate*ms/1024;
	  if (final > desired) final = desired; // clamp at desired rate
	}
      
    }
  //now apply turn rate for ms time--to integrate correctly,
  //use average of start and final
   
  tilt = ((keyboard_tilt_cur_vel + final)/2*ms/1024);
  keyboard_tilt_cur_vel = final;
  
  scale = PlayerCamera()->zoom;
  return tilt/scale;

}

int headmoveComputeKeyboardTurn(int ms)
{
   float scale;
   int desired, final, turn;

   // fast out if player never uses keyboard turning
   if (keyboard_turn == 0 && keyboard_cur_vel == 0)
      return 0;

   // compute desired turn rate
   desired = KBD_TURN_RATE * -keyboard_turn;

   if (desired == keyboard_cur_vel) {
      final = keyboard_cur_vel;
   } else {
      int rate;
      if (desired == 0)
         rate = KBD_DECELERATE;
      else if ((keyboard_cur_vel > 0 && desired < 0) ||
               (keyboard_cur_vel < 0 && desired > 0))
         rate = KBD_ACCELERATE_CHANGE_DIR;
      else
         rate = KBD_ACCELERATE;

      // apply 'rate' change for this long
      if (desired < keyboard_cur_vel) {
         final = keyboard_cur_vel - rate*ms/1024;
         if (final < desired) final = desired;   // clamp at desired rate
      } else {
         final = keyboard_cur_vel + rate*ms/1024;
         if (final > desired) final = desired;   // clamp at desired rate
      }
   }
   // now apply turn rate for ms time--to integrate correctly,
   // use average of start and final

   turn = ((keyboard_cur_vel + final)/2*ms/1024);

   keyboard_cur_vel = final;

   scale = PlayerCamera()->zoom;
   return turn / scale;
}

// simple "look around w/the mouse" code
bool headmoveLookAround(Camera *cam, int ms)
{
   bool ret;
   mxs_angvec ang;
   short centerx, centery;

   centerx = grd_visible_canvas->bm.w / 2;
   centery = grd_visible_canvas->bm.h / 2;

   if ((cam == PlayerCamera()) && (CameraGetMode(cam) == VIEW_CAM) && (PhysObjHasPhysics(CameraGetObjID(cam))))
     return FALSE;
   
   // get the head offset
   // danger! hack
   if ((cam == PlayerCamera()) && (CameraGetMode(cam) == REMOTE_CAM) && (PhysObjHasPhysics(CameraGetObjID(cam))))
     PhysGetModRotation(CameraGetObjID(cam), &ang);
   else
     PhysGetSubModRotation(PlayerObject(), PLAYER_HEAD, &ang);

   // factor in the keyboard turn rate
   ang.tz += headmoveComputeKeyboardTurn(ms);
   // factor in the keyboard tilt rate
   ang.ty += headmoveComputeKeyboardTilt(ms);

   // buzzard: try disabling frame-rate relative, because the player will
   // get a large or smaller delta due to moving muose farther in one frame
   ms = 50;

   // was once 30 and 20, i think
   if ((fabs(mouse_rel_x)<=DEAD_ZONE)&&(fabs(mouse_rel_y)<=DEAD_ZONE)) {
      ret = FALSE;                   // "dead zone"
   } else {
      // calc. the new offset --- TODO: make this screen size independant????
      ang.tz -= (HEADRATE*mouse_rel_x*ms)/(100*3*centerx);   // since ms/100 is unit based scale
      ang.ty += (HEADRATE*mouse_rel_y*ms)/(100*2*centery);   //   of these sort of motions
      ret = TRUE;
   }

   // set the new head offset
   PosPropLock++;
   PhysicsListenerLock = TRUE;
   if ((cam == PlayerCamera()) && (CameraGetMode(cam) == REMOTE_CAM) && (PhysObjHasPhysics(CameraGetObjID(cam))))
     PhysSetModRotation(CameraGetObjID(cam), &ang);
   else
     PhysSetSubModRotation(PlayerObject(), PLAYER_HEAD, &ang);
   PhysicsListenerLock = FALSE;
   PosPropLock--;

   return ret;
}

// move src towards dst by diff_frac (0-1)
static mxs_ang smoothAngElem(mxs_ang dst, mxs_ang src, float diff_frac)
{
   fixang delta_ang=src-dst;
   fix    math_ang;

   if (delta_ang>=0x8000)  math_ang=(int)delta_ang-0x10000;
   else                    math_ang=delta_ang;
   dst += (fix)(math_ang*diff_frac);
   return dst;
}

// actual floating point frac
static void smoothAng(mxs_angvec *new, mxs_angvec *dst, mxs_angvec *src, float diff_frac)
{
   int i;
   for (i=0; i<3; i++)
      new->el[i]=smoothAngElem(dst->el[i],src->el[i],diff_frac);
}

// simple "relax the head back toward 000" code
// TODO: make this nonlinear and cool
void headmoveRelax(Camera *cam, float rate)
{      // from dst towards src by frac into new
   extern void smoothAng(mxs_angvec *new, mxs_angvec *dst, mxs_angvec *src, float diff_frac);
   mxs_angvec ang, zerovec={0,0,0};
   
   CameraGetAngOffset(cam, &ang);        // get the current head offset
   if (((CameraGetMode(cam) == REMOTE_CAM) || (CameraGetMode(cam) == VIEW_CAM))
      && (PhysObjHasPhysics(CameraGetObjID(cam))))
     {
       //ok remote mode, dont rotate around z axis.
       zerovec.tz = ang.tz;
     }
   if (rate!=1.0)
      smoothAng(&ang,&ang,&zerovec,rate);
   else
      ang=zerovec;
   CameraSetAngOffset(cam, &ang);        // set the new head offset
}

static long last_look=-1, relax_clock=-1;
static int  relax_mode=RELAX_MODE_ONMOVE;
static int  relax_parm=RELAX_DEF_PAUSE;
static int  relax_time=RELAX_DEF_TIME;

void headmoveReset(Camera *cam, bool instant)
{
   if (instant)
      headmoveRelax(cam,1.0);
   else
      if (relax_clock==-1)
         relax_clock=0;
}

void headmoveSetup(int rmode, int param, int rtime)
{
   relax_mode=rmode;
   relax_parm=param;
   relax_time=rtime;
}

void headmoveInit(void)
{
   if (config_is_defined("relax_on_move"))
      headmoveSetup(RELAX_MODE_ONMOVE,RELAX_DEF_PAUSE,RELAX_DEF_TIME);
   else if (config_is_defined("relax_instant"))
      headmoveSetup(RELAX_MODE_INSTANT,RELAX_DEF_PAUSE,RELAX_DEF_TIME);
   else
      headmoveSetup(RELAX_MODE_NEVER,RELAX_DEF_PAUSE,RELAX_DEF_TIME);
   config_get_int("head_speed",&mouse_head_def_speed);
   config_get_int("head_zone",&mouse_head_def_dead_zone);
   headmoveSetMouseParams(-1,-1);
   mouse_put_xy((short)(grd_visible_canvas->bm.w/2),(short)(grd_visible_canvas->bm.h/2));

   config_get_int("kbd_turn_accelerate", &kbd_accelerate);
   config_get_int("kbd_turn_decelerate", &kbd_decelerate);
   config_get_int("kbd_turn_reverse_acc", &kbd_accelerate_change_dir);
   config_get_int("kbd_turn_rate", &kbd_turn_rate);
}


void headmoveSetRelPosX (float x)
{
   mouse_rel_x = x;
}


void headmoveSetRelPosY (float y)
{
   mouse_rel_y = y;
}


void headmoveSetMouseParams(int speed, int zone)
{
   if(speed>=0)
      mouse_head_speed=speed;
   else
      mouse_head_speed=mouse_head_def_speed;
   if(zone>=0)
      mouse_head_dead_zone=zone;
   else
      mouse_head_dead_zone=mouse_head_def_dead_zone;
}

// To allow external head-moving systems that the head is being used
void headmoveTouch(void)
{
   relax_clock = last_look = -1;
}   

// Actual call to make in from the app
void headmoveCheck(Camera *cam, int ms)
{
   ObjID obj = CameraGetObjID(cam);
   mxs_vector vel, angvel;
   BOOL no_head=FALSE;

   // for now, if we are obj null, lets just look at the player...
   if (obj==OBJ_NULL)
   {
      no_head=TRUE;
      obj=PlayerObject();
   }

   PhysGetVelocity(obj, &vel);
   PhysGetRotationalVelocity(obj, &angvel);

   if (no_head) return;

   if (!headmoveLookAround(cam,ms))
   {  // didnt look around, maybe we should relax the view
      if ((relax_clock==-1)&&(relax_mode!=RELAX_MODE_NEVER))
      {
         if (relax_mode==RELAX_MODE_ONMOVE)
         {
            if ((fabs(mx_mag2_vec(&vel))>2.0)||
                (fabs(angvel.z)>2.0))
               relax_clock=0;
         }
         else
            if (last_look>relax_parm)
               relax_clock=0;
      }
      if (relax_clock>=0)   // do some relaxing
         if (relax_clock+ms>=relax_time)
         {
            headmoveRelax(cam,1.0);
            relax_clock=-2;      // we are tan and relaxed
         }           // -2 means do nothing until we have looked again
         else
         {
            float proportion=(float)ms/(float)(relax_time-relax_clock);
            headmoveRelax(cam,proportion);
            relax_clock+=ms;
         }
      last_look+=ms;
   }
   else
      relax_clock=last_look=-1;     // we are looking around
}

static Position   last_pos;
static mxs_angvec last_head;
static BOOL       last_valid=FALSE;

int   headmove_ang_disp=0;     // net angular displacement of the head
float headmove_vec2_disp=0;    // net displacement squared of the head in space
uchar headbody_axis=(1<<2);  // which axis are insta-connected
//uchar headbody_axis=0;  // which axis are insta-connected

static int ang_abs(fixang ang1, fixang ang2)
{
   fixang delta_ang=ang1-ang2;
   if (delta_ang>=0x8000)
      return 0x10000-((int)delta_ang);  // pre-absed for your pleasure, or something
   else
      return (int)delta_ang;   // we are <0x7fff, so we are positive already
}

void playerHeadControl(void)
{
  ObjID obj = PlayerObject();
  Camera *cam=PlayerCamera();
  ObjPos *cur_pos;
  mxs_angvec cur_headang;  
  mxs_angvec camang;
  int i;


  if (CameraIsRemote(cam))
    return;
  
  cur_pos = ObjPosGet(obj);
  
  CameraGetAngOffset(PlayerCamera(),&camang);
  mx_mk_angvec(&cur_headang, 0, 0, 0);
  for (i=0; i<3; i++)
    {
      if (headbody_axis&(1<<i))
	{
	  cur_pos->fac.el[i]+=camang.el[i];
	  camang.el[i]=0;
	}
      else
	{
	  cur_pos->fac.el[i] = 0;
	  cur_headang.el[i] = camang.el[i];
	}
    }
  PosPropLock++;
  PhysicsListenerLock = TRUE;
  CameraSetAngOffset(PlayerCamera(),&camang);
  PhysSetModRotation(obj, &cur_pos->fac);
  PhysSetSubModRotation(obj, PLAYER_HEAD, &cur_headang);
  ObjRotate(obj,&cur_pos->fac);
  PhysicsListenerLock = FALSE;
  PosPropLock--; 

   headmove_ang_disp=0;  // need to zero this no matter what, since we add to it if we have real data
   if (last_valid)
   {
      mxs_vector tmp;
      mx_sub_vec(&tmp,&last_pos.loc.vec,&cur_pos->loc.vec);
      headmove_vec2_disp=mx_mag2_vec(&tmp);
      headmove_ang_disp=0;
      for (i=0; i<3; i++)
      {
         if ((headbody_axis&(1<<i))==0)
            headmove_ang_disp+=ang_abs(last_head.el[i],camang.el[i]);
         headmove_ang_disp+=ang_abs(last_pos.fac.el[i],cur_pos->fac.el[i]);
      }
   }
   else
      headmove_vec2_disp=0;
   last_head=camang;
   last_pos=*cur_pos;
   last_valid=TRUE;
}
