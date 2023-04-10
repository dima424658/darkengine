/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkracer.cpp,v 1.5 2000/02/19 13:26:01 toml Exp $

// alarm icon
#include <2d.h>
#include <math.h>

#include <resapi.h>
#include <appagg.h>
#include <minmax.h>
#include <mprintf.h>
#include <random.h>
#include <mouse.h>
#include <simtime.h>
#include <dynarray.h>

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

#define sq(x)  ((x)*(x))

//--------------------------------------------------------------------------------------
// Swine Hunter (aka Racer 2.0)
//--------------------------------------------------------------------------------------

// Window/frame constants
const int kRacerHeight = 90;
const int kRacerWidth  = 40;

const int kFrameWidth = 135;
const int kFrameHeight = 271;

// Speed constants
const int kCarSpeed = 50;
const int kCarForwardMax = 3 * kFrameHeight / 4;
const int kCarBackMax = kFrameHeight / 4;
const int kShotSpeed = 8;

const int kCarMaxVel = 1.5;

const int kAnimSpeed = 200;

const int kCrashedDelay = 2000;

// Other car constants
const int kOtherCarMinSpeed = 19;
const int kOtherCarMaxSpeed = 35;

const int kOtherCarShooterPct = 5;    // 0-5
const int kOtherCarRazorPct = 15;     // 6-15
const int kOtherCarPigPct = 65;       // 16-65

// Track building  constants
const int kTrackVol = 25;
const int kTrackWidth = 110;

const int kFramesBetweenVol = 10;
const int kFramesBetweenWidth = 20;

// Car controls
const int kDeadZone = 4;

typedef struct
{
   float x;
   float y;
} fPoint;

enum eRacerFlags
{
   kRF_Crashed   = 0x0001,
   kRF_GameOver  = 0x0002,
   kRF_ShotFired = 0x0004,
   kRF_BackFired = 0x0008,
   kRF_Seeking   = 0x1000,
   kRF_Animating = 0x2000,
};

enum eOtherCarTypes
{
   kOCT_Pedestrian,
   kOCT_Pig,
   kOCT_RazorTires,
   kOCT_Shooter,
   kOCT_MaxTypes
};

// Scoring constants
const int kScoreMs = 1000;
const int kScoreTime = 5;
const int kScoreCrash[kOCT_MaxTypes] = { -1000, 500, 750, 600 };

struct sOtherCar
{
   eOtherCarTypes type;
   fPoint velocity;
   fPoint floc;
   Point  loc;
   Point  shot;
   int    speed;
   int    updateAccum;
   int    delayAccum;
   short  flags;
   short  frame;
};

// Structures
struct sRaceState
{
   float centerVel;
   cDynArray<ushort> left;
   cDynArray<ushort> right;

   fPoint velocity;
   fPoint fcar;
   Point car;
   Point shot;
   Point backshot;

   cDynArray<sOtherCar> others;

   int scoreMsAccum;
   int delay;

   int lives;
   int score;

   int flags;
};

// Bitmaps
IRes *gRaceCar;
IRes *gRaceCrash;
IRes *gRaceWall;
IRes *gRaceShot;
IRes *gRaceSShot;
IRes *gRaceBack;
IRes *gRaceOthers[kOCT_MaxTypes][2];


grs_canvas gRacerCanvas;

// All our game state
sRaceState gRaceState;

int gFrameTime;


// high scores

sFileVarDesc gRacerScoreDesc =
{
   kCampaignVar,
   "RacerScr",
   "Racer High Scores",
   FILEVAR_TYPE(sMiniGameScores),
   {1,0},
   {1,0},
   "shock",
};

cFileVar<sMiniGameScores,&gRacerScoreDesc> gRacerScoreTable;

////////////////
// misc string state
#define GetNamedString(buf,name)    ShockStringFetch(buf,sizeof(buf),name,"minigame")

static void _check_n_add_score(int score)
{
   char form_buf[64], buf[64];
   // format string, pass it in
   GetNamedString(form_buf,"rc_score_form");
   sprintf(buf,form_buf,score);
   ShockMiniGameAddHighScore(&gRacerScoreTable,score,buf);
}

////////////////////////////////////////

void BuildWalls(int row)
{
   if (row == 0)
      return;

   // Curve track
   int prev_center = (gRaceState.right[row-1] + gRaceState.left[row-1]) / 2;
   int cur_center;
   int center_delta = 0;

   int rand = RandRangeLong(100);

   if (rand < (kTrackVol / 3))
      center_delta = -2;
   else
   if (rand < kTrackVol)
      center_delta = -1;
   else
   if (rand > 100 - (kTrackVol / 3))
      center_delta = 2;
   else
   if (rand > 100 - kTrackVol)
      center_delta = 1;

   gRaceState.centerVel += center_delta;

   if (prev_center < (kFrameWidth / 3))
      gRaceState.centerVel++;
   if (prev_center > (2 * kFrameWidth / 3))
      gRaceState.centerVel--;

   if (gRaceState.centerVel > 2)
      gRaceState.centerVel = 2;
   if (gRaceState.centerVel < -2)
      gRaceState.centerVel = -2;

   cur_center = prev_center + gRaceState.centerVel;

   // Widen/narrow the track
   int prev_width = gRaceState.right[row-1] - gRaceState.left[row-1]; 
   int cur_width;
   int width_delta = 0;

   rand = RandRangeLong(100);

   if (prev_width < kTrackWidth)
   {
      if (rand > 75)
         width_delta = 2;
      else
      if (rand > 50)
         width_delta = 1;
   }
   else
   if (prev_width > kTrackWidth)
   {
      if (rand > 75)
         width_delta = -2;
      else
      if (rand > 50)
         width_delta = -1;
   }
   else
   {
      if (rand < 25)
         width_delta = -1;
      else
      if (rand > 75)
         width_delta = 1;
   }

   cur_width = prev_width + width_delta;

   // Update walls
   gRaceState.left[row] = max(4, cur_center - (cur_width / 2));
   gRaceState.right[row] = min(kFrameWidth - 4, cur_center + (cur_width / 2));
}

void CreateOtherCar(eOtherCarTypes type)
{
   int new_index = gRaceState.others.Size();

   gRaceState.others.SetSize(new_index + 1);
   sOtherCar *pOtherCar = &gRaceState.others[new_index];

   pOtherCar->type = type;
   pOtherCar->updateAccum = 0;
   pOtherCar->delayAccum = 0;
   pOtherCar->flags = 0;
   pOtherCar->frame = 0;

   int start_at_top;

   switch (type)
   {
      case kOCT_Pedestrian:
      {
         pOtherCar->frame = RandRangeLong(2);

         // Pedestrians always start from the top and head downward
         start_at_top = TRUE;
         break;
      }

      case kOCT_Pig:
      {
         pOtherCar->flags |= kRF_Animating;

         if (RandRangeLong(100) > 50)
            start_at_top = TRUE;
         else
            start_at_top = FALSE;

         break;
      }

      case kOCT_RazorTires:
      {
         pOtherCar->flags |= kRF_Seeking | kRF_Animating;

         if (RandRangeLong(100) > 50)
            start_at_top = TRUE;
         else
            start_at_top = FALSE;

         break;
      }

      case kOCT_Shooter:
      {
         // Shooters always start at bottom and shoot as they pass
         pOtherCar->frame = RandRangeLong(2);

         start_at_top = FALSE;

         break;
      }
   }

   int left;
   int right;

   if (start_at_top)
   {
      pOtherCar->loc.y = kFrameHeight - 5;

      left = gRaceState.left[pOtherCar->loc.y / 3] + 5;
      right = gRaceState.right[pOtherCar->loc.y / 3] - 5;
      pOtherCar->loc.x = left + RandRangeLong(right - left);
         
      pOtherCar->speed = kOtherCarMinSpeed + RandRangeLong(kOtherCarMaxSpeed - kOtherCarMinSpeed);
      
      pOtherCar->velocity.x = 0;
      pOtherCar->velocity.y = -1;
   }
   else
   {
      pOtherCar->loc.y = 5;

      left = gRaceState.left[pOtherCar->loc.y / 3] + 5;
      right = gRaceState.right[pOtherCar->loc.y / 3] - 5;
      pOtherCar->loc.x = left + RandRangeLong(right - left);

      pOtherCar->speed = kOtherCarMinSpeed + RandRangeLong(kOtherCarMaxSpeed - kOtherCarMinSpeed);
      
      pOtherCar->velocity.x = 0;
      pOtherCar->velocity.y = 1;
   }

   pOtherCar->floc.x = (float)pOtherCar->loc.x;
   pOtherCar->floc.y = (float)pOtherCar->loc.y;
}

BOOL UpdateOtherCar(sOtherCar *pOtherCar)
{
   // Update Y position
   if (pOtherCar->flags & kRF_Seeking)
   {
      // Seek towards 
      if (RandRangeLong(100) < 10)
      {
         if (gRaceState.car.y > pOtherCar->loc.y)
            pOtherCar->velocity.y += 0.2;
         else
            pOtherCar->velocity.y -= 0.2;
      }
   }

   // Stay on the road
   int left_dist = pOtherCar->loc.x - gRaceState.left[pOtherCar->loc.y / 3];
   int right_dist = gRaceState.right[pOtherCar->loc.y / 3] - pOtherCar->loc.x;

   if (left_dist < 20)
      pOtherCar->velocity.x += 2.0 - left_dist / 10.0;

   if (right_dist < 20)
      pOtherCar->velocity.x -= 2.0 - right_dist / 10.0;
         
   // Jiggle
   int rand = RandRangeLong(100);

   if (rand < 10)
      pOtherCar->velocity.x -= 0.1;
   else
   if (rand > 100)
      pOtherCar->velocity.x += 0.1;

   // Stay away from other cars
   int closest_car = -2;
   float car_dist = 0;

   for (int i=0; i<gRaceState.others.Size(); i++)
   {
      if (&gRaceState.others[i] == pOtherCar)
         continue;

      float cur_dist = sqrt((float)sq(pOtherCar->loc.x - gRaceState.others[i].loc.x) +
                            (float)sq(pOtherCar->loc.y - gRaceState.others[i].loc.y));

      if ((car_dist == 0) || (cur_dist < car_dist))
      {
         closest_car = i;
         car_dist = cur_dist;
      }
   }

   // Shooters are special
   if (pOtherCar->type == kOCT_Shooter)
   {
      // Try to keep to one side of the road
      if (pOtherCar->frame == 0)
         pOtherCar->velocity.x -= 0.1;
      else
         pOtherCar->velocity.x += 0.1;

      // If we're withing 3, y-wise of the player, shoot!
      if (!(pOtherCar->flags & kRF_ShotFired) && 
          abs(pOtherCar->loc.y - gRaceState.car.y) < 3)
      {
         pOtherCar->shot.x = pOtherCar->loc.x;
         pOtherCar->shot.y = pOtherCar->loc.y;

         pOtherCar->flags |= kRF_ShotFired;
      }
   }

   // Factor in player (repellant or attractive) player
   if (pOtherCar->flags & kRF_Seeking)
   {
      // Seek
      if (RandRangeLong(100) < 10)
      {
         if (pOtherCar->loc.x > gRaceState.car.x)
            pOtherCar->velocity.x -= 0.2;
         else
            pOtherCar->velocity.x += 0.2;
      }
   }
   else
   {
      float cur_dist = sqrt((float)sq(pOtherCar->loc.x - gRaceState.car.x) +
                            (float)sq(pOtherCar->loc.y - gRaceState.car.y));

      if ((car_dist == 0) || (cur_dist < car_dist))
      {
         closest_car = -1;
         car_dist = cur_dist;
      }
   }
   
   if (car_dist < 15)
   {
      // Player is closest
      if (closest_car == -1)
      {
         if (pOtherCar->loc.x > gRaceState.car.x)
            pOtherCar->velocity.x += 1.5 - car_dist / 10.0;
         else
            pOtherCar->velocity.x -= 1.5 - car_dist / 10.0;
      }
      else
      // Other car is closest
      if (closest_car != -2)
      {
         if (pOtherCar->loc.x > gRaceState.others[closest_car].loc.x)
            pOtherCar->velocity.x += 1.5 - car_dist / 10;
         else
            pOtherCar->velocity.x -= 1.5 - car_dist / 10;
      }
   }

   // Cap velocities
   if (pOtherCar->velocity.y > 1)
      pOtherCar->velocity.y = 1;
   else
   if (pOtherCar->velocity.y < -1)
      pOtherCar->velocity.y = -1;

   if (pOtherCar->velocity.x > 1)
      pOtherCar->velocity.x = 1;
   else
   if (pOtherCar->velocity.x < -1)
      pOtherCar->velocity.x = -1;

   pOtherCar->floc.x += pOtherCar->velocity.x;
   pOtherCar->floc.y += pOtherCar->velocity.y;
   
   pOtherCar->loc.x = (int)pOtherCar->floc.x;
   pOtherCar->loc.y = (int)pOtherCar->floc.y;

   // Kill if going off the screen
   if ((pOtherCar->loc.y < 5) || (pOtherCar->loc.y > kFrameHeight - 5))
      return FALSE;

   return TRUE;
}      

void InitCar(int x, int y)
{
   gRaceState.car.x = x;
   gRaceState.car.y = y;

   gRaceState.fcar.x = (float)x;
   gRaceState.fcar.y = (float)y;

   gRaceState.velocity.x = gRaceState.velocity.y = 0;
}

void Crash()
{
   gRaceState.flags |= kRF_Crashed;
   gRaceState.delay = kCrashedDelay;

   gRaceState.lives--;

   if (gRaceState.lives < 0)
   {
      gRaceState.lives = 0;
      gRaceState.flags |= kRF_GameOver;

      _check_n_add_score(gRaceState.score);
   }
}

////////////////////////////////////////

void RacerInit()
{
   int i;

   gRaceState.centerVel = 0.0;

   gRaceState.left.SetSize(kRacerHeight);
   gRaceState.right.SetSize(kRacerHeight);

   gRaceState.left[0] = RandRangeLong(kFrameWidth / 4);
   gRaceState.right[0] = kFrameWidth - RandRangeLong(kFrameWidth / 4);

   for (i=1; i<kRacerHeight; i++)
      BuildWalls(i);
 
   InitCar(kFrameWidth * 0.5, kFrameHeight * 0.2);

   gRaceState.delay = 0;

   gRaceState.lives = 3;
   gRaceState.score = 0;

   gRaceState.flags = 0;

   gRaceCar =   LoadPCX("racecar");
   gRaceCrash = LoadPCX("racecrsh");
   gRaceWall =  LoadPCX("racewall");
   gRaceBack =  LoadPCX("raceback");
   gRaceShot =  LoadPCX("raceshot");
   gRaceSShot = LoadPCX("racessht");

   gRaceOthers[0][0] = LoadPCX("raceped1");
   gRaceOthers[0][1] = LoadPCX("raceped2");

   gRaceOthers[1][0] = LoadPCX("racepig1");
   gRaceOthers[1][1] = LoadPCX("racepig2");

   gRaceOthers[2][0] = LoadPCX("raceraz1");
   gRaceOthers[2][1] = LoadPCX("raceraz2");

   gRaceOthers[3][0] = LoadPCX("racesht1");
   gRaceOthers[3][1] = LoadPCX("racesht2");

   Assert_(kOCT_MaxTypes == 4);

   gFrameTime = 0;
}

void RacerTerm()
{
   gRaceState.left.SetSize(0);
   gRaceState.right.SetSize(0);

   gRaceState.others.SetSize(0);

   SafeFreeHnd(&gRaceCar);
   SafeFreeHnd(&gRaceCrash);
   SafeFreeHnd(&gRaceWall);
   SafeFreeHnd(&gRaceBack);
   SafeFreeHnd(&gRaceShot);
   SafeFreeHnd(&gRaceSShot);

   SafeFreeHnd(&gRaceOthers[0][0]);
   SafeFreeHnd(&gRaceOthers[0][1]);
   SafeFreeHnd(&gRaceOthers[1][0]);
   SafeFreeHnd(&gRaceOthers[1][1]);
   SafeFreeHnd(&gRaceOthers[2][0]);
   SafeFreeHnd(&gRaceOthers[2][1]);
   SafeFreeHnd(&gRaceOthers[3][0]);
   SafeFreeHnd(&gRaceOthers[3][1]);
}

static Rect base_rect = {{ 13, 11},{154,288}};

void SwineHunterDraw(Point pt);
void RacerDraw(Point pt)
{
   // push a canvas here for the current draw rectangle?
   grs_canvas cnv;
   grs_bitmap *bmp;

   bmp = gr_alloc_bitmap(BMT_FLAT16, 0, RectWidth(&base_rect),RectHeight(&base_rect));
   gr_make_canvas(bmp,&cnv);

   // do the actual drawing
   gr_push_canvas(&cnv);
   SwineHunterDraw(pt);
   gr_pop_canvas();

   // blit it
   gr_bitmap(bmp,pt.x + base_rect.ul.x, pt.y + base_rect.ul.y);

   // clear up our alloc
   gr_free(bmp);
}

void SwineHunterDraw(Point pt)
{
   int i, j;

   char buf[256];
   char temp[256];

   // Draw the background
   Point back;
   back.x = 0;
   back.y = 0;
   DrawByHandle(gRaceBack, back);

   // Dead?
   if (gRaceState.flags & kRF_GameOver)
   {
      Point drawpt;

      ShockStringFetch(temp, 80, "sh_gameover", "minigame");
      gr_font_string(gShockFont, temp, 40, 40);
      
      drawpt.x=base_rect.ul.x+17;  // or something
      drawpt.y=base_rect.ul.y+50;
      ShockMiniGameDisplayHighScores(&gRacerScoreTable,drawpt.x,drawpt.y);
      drawpt.x=pt.x+base_rect.ul.x+21;
      drawpt.y=pt.y+base_rect.lr.y-21;
      gr_font_string(gShockFont, gMiniGameScoreMsg, drawpt.x, drawpt.y);
      
      return;
   }

   // Draw walls
   Point wall;
   for (i=0; i<kRacerHeight; i++)
   {
      wall.y = kFrameHeight - i * 3;

      wall.x = gRaceState.left[i];
      DrawByHandleCenter(gRaceWall, wall);

      wall.x = gRaceState.right[i];
      DrawByHandleCenter(gRaceWall, wall);

   }

   // Draw car
   Point carpos;
   carpos.x = gRaceState.car.x;
   carpos.y = kFrameHeight - gRaceState.car.y;

   if (gRaceState.flags & kRF_Crashed)
      DrawByHandleCenter(gRaceCrash, carpos);
   else
      DrawByHandleCenter(gRaceCar, carpos);

   // Draw shot
   if (gRaceState.flags & kRF_ShotFired)
   {
      Point shotpos;
      shotpos.x = gRaceState.shot.x;
      shotpos.y = kFrameHeight - gRaceState.shot.y;

      DrawByHandleCenter(gRaceShot, shotpos);
   }

   // Draw back shot
   if (gRaceState.flags & kRF_BackFired)
   {
      Point shotpos;
      shotpos.x = gRaceState.backshot.x;
      shotpos.y = kFrameHeight - gRaceState.backshot.y;
   
      DrawByHandleCenter(gRaceShot, shotpos);
   }

   // Draw other cars and shots
   for (i=0; i<gRaceState.others.Size(); i++)
   {
      sOtherCar *pOtherCar = &gRaceState.others[i];

      Point otherpos;
      otherpos.x = pOtherCar->loc.x;
      otherpos.y = kFrameHeight - pOtherCar->loc.y;

      if (pOtherCar->flags & kRF_Crashed)
         DrawByHandleCenter(gRaceCrash, otherpos);
      else
      {
         DrawByHandleCenter(gRaceOthers[pOtherCar->type][pOtherCar->frame], otherpos);

         if (pOtherCar->flags & kRF_ShotFired)
         {
            Point shotpos;
            shotpos.x = pOtherCar->shot.x;
            shotpos.y = kFrameHeight - pOtherCar->shot.y;

            DrawByHandleCenter(gRaceSShot, shotpos);
         }
      }
   }
   
   // Draw score/lives
   ShockStringFetch(temp, 80, "sh_score", "minigame");
   sprintf(buf, "%s %d\n", temp, gRaceState.score);
   gr_font_string(gShockFont, buf, 4, 2);

   ShockStringFetch(temp, 80, "sh_lives", "minigame");
   sprintf(buf, "%s %d\n", temp, gRaceState.lives);
   gr_font_string(gShockFont, buf, 90, 2);

   // Delaying?
   if (gRaceState.delay > 0)
   {
      gRaceState.delay -= GetSimFrameTime();

      // Coming out of delay?
      if (gRaceState.delay <= 0)
      {
         if (gRaceState.flags & kRF_Crashed)
         {
            gRaceState.flags &= ~(kRF_Crashed | kRF_ShotFired | kRF_BackFired);

            gRaceState.others.SetSize(0);

            InitCar((gRaceState.left[gRaceState.car.y / 3] + 
                    gRaceState.right[gRaceState.car.y / 3]) / 2, 
                    gRaceState.car.y);
         }
      }
   }
   else
   {
      // Update other cars
      for (i=0; i<gRaceState.others.Size(); i++)
      {
         sOtherCar *pOtherCar = &gRaceState.others[i];

         // Shots
         if (pOtherCar->flags & kRF_ShotFired)
         {
            // the shooter is only firing ai car, so we can tell which way
            // the bullet is heading by looking at the rendering frame
            if (pOtherCar->frame == 0)
               pOtherCar->shot.x += kShotSpeed / 2;
            else
               pOtherCar->shot.x -= kShotSpeed / 2;
         }

         // If crashed, don't update location or animation
         if (pOtherCar->flags & kRF_Crashed)
         {
            // If we're done delaying, destroy
            pOtherCar->delayAccum += GetSimFrameTime();
            if (pOtherCar->delayAccum > kCrashedDelay)
            {
               gRaceState.others.DeleteItem(i);
               i--;
            }
            continue;
         }

         // Animations
         if (pOtherCar->flags & kRF_Animating)
         {
            pOtherCar->delayAccum += GetSimFrameTime();
            while (pOtherCar->delayAccum > kAnimSpeed)
            {
               pOtherCar->delayAccum -= kAnimSpeed;
               pOtherCar->frame ^= 1;
            }
         }

         // Positions
         pOtherCar->updateAccum += GetSimFrameTime();
         while (pOtherCar->updateAccum > pOtherCar->speed)
         {
            pOtherCar->updateAccum -= pOtherCar->speed;

            if (!UpdateOtherCar(&gRaceState.others[i]))
            {
               gRaceState.others.DeleteItem(i);
               i--;
               break;
            }
         }
      }
            
      gFrameTime += GetSimFrameTime();
      while (gFrameTime > kCarSpeed)
      {
         // Update wall positions
         for (i=0; i<kRacerHeight-1; i++)
         {
            gRaceState.left[i] = gRaceState.left[i+1];
            gRaceState.right[i] = gRaceState.right[i+1];
         }

         BuildWalls(kRacerHeight - 1);

         // Update car velocity
         short mx, my;
         mouse_get_xy(&mx, &my);

         mx -= pt.x + base_rect.ul.x;
         my -= pt.y + base_rect.ul.y;

         int x_spd = (abs(mx - carpos.x) > 20) ? 2 : 1;
         int y_spd = (abs(my - carpos.y) > 20) ? 2 : 1;

         if ((carpos.x - mx) > kDeadZone)
            gRaceState.car.x -= x_spd;
         else
         if ((mx - carpos.x) > kDeadZone)
            gRaceState.car.x += x_spd;

         if ((carpos.y - my) > kDeadZone)
            gRaceState.car.y += y_spd;
         else
         if ((my - carpos.y) > kDeadZone)
            gRaceState.car.y -= y_spd;

         if (gRaceState.car.x < 3)
            gRaceState.car.x = 3;
         if (gRaceState.car.x > kFrameWidth - 3)
            gRaceState.car.x = kFrameWidth - 3;
         if (gRaceState.car.y > kCarForwardMax)
            gRaceState.car.y = kCarForwardMax;
         if (gRaceState.car.y < kCarBackMax)
            gRaceState.car.y = kCarBackMax;

         // Update shot position
         if (gRaceState.flags & kRF_ShotFired)
            gRaceState.shot.y += kShotSpeed;
         if (gRaceState.flags & kRF_BackFired)
            gRaceState.backshot.y -= kShotSpeed;

         // Scroll any crashed cars too
         for (i=0; i<gRaceState.others.Size(); i++)
         {
            if (gRaceState.others[i].flags & kRF_Crashed)
               gRaceState.others[i].loc.y-=3;

            if (gRaceState.others[i].loc.y < 5)
            {
               gRaceState.others.DeleteItem(i);
               i--;
            }
               
         }

         // Add new car?
         int new_car_chance = (gRaceState.score / 5000) + 1; // 1 percent for each 5000 points

         if (gRaceState.others.Size() == 0)  // better chance if road is empty
            new_car_chance += 1;

         new_car_chance -= (gRaceState.others.Size() - 1);  // try to avoid clusters of cars

         if (new_car_chance < 0) new_car_chance = 0;
         if (new_car_chance > 10) new_car_chance = 10;

         if (RandRangeLong(100) < new_car_chance)
         {
            int rand = RandRangeLong(100);

            if (rand < kOtherCarShooterPct)
               CreateOtherCar(kOCT_Shooter);
            else
            if (rand < kOtherCarRazorPct)
               CreateOtherCar(kOCT_RazorTires);
            else
            if (rand < kOtherCarPigPct)
               CreateOtherCar(kOCT_Pig);
            else
               CreateOtherCar(kOCT_Pedestrian);
         }

         // Update time-based score
         gRaceState.scoreMsAccum += kCarSpeed;
         while (gRaceState.scoreMsAccum > kScoreMs)
         {
            gRaceState.scoreMsAccum -= kScoreMs;
            gRaceState.score += kScoreTime;
         }

         gFrameTime -= kCarSpeed;
      } 

      // AI collisition detection
      for (i=0; i<gRaceState.others.Size(); i++)
      {
         sOtherCar *pOtherCar = &gRaceState.others[i];

         if (pOtherCar->flags & kRF_ShotFired)
         {
            // Shot/wall
            if ((pOtherCar->shot.x - 3 <= gRaceState.left[pOtherCar->shot.y / 3]) ||
                (pOtherCar->shot.x + 3 >= gRaceState.right[pOtherCar->shot.y / 3]))
            {
               pOtherCar->flags &= ~kRF_ShotFired;
            }
            // Shot/player
            if ((abs(pOtherCar->shot.x - gRaceState.car.x) < 6) &&
                (abs(pOtherCar->shot.y - gRaceState.car.y) < 6))
               Crash();
         }
         
         // no collision detection for crashed cars
         if (pOtherCar->flags & kRF_Crashed)
            continue;

         // Car/wall
         if ((pOtherCar->loc.x - 2 <= gRaceState.left[pOtherCar->loc.y / 3]) ||
             (pOtherCar->loc.x + 2 >= gRaceState.right[pOtherCar->loc.y / 3]))
         {
            pOtherCar->flags |= kRF_Crashed;
            pOtherCar->delayAccum = 0;

            if (kScoreCrash[pOtherCar->type] > 0)
               gRaceState.score += kScoreCrash[pOtherCar->type];
         }

         // Car/car
         for (j=0; j<gRaceState.others.Size(); j++)
         {
            sOtherCar *pOtherOtherCar = &gRaceState.others[j];

            if ((i == j) || (pOtherOtherCar->flags & kRF_Crashed))
               continue;

            if ((abs(pOtherCar->loc.x - pOtherOtherCar->loc.x) <= 12) && 
                (abs(pOtherCar->loc.y - pOtherOtherCar->loc.y) <= 12))
            {
               // We're only going to allow crashing of the pOtherOtherCar here, 
               // because the opposite cases are going to be part of the nested 
               // iteration anyway.

               // Razortires crash everything but pigs.

               if ((pOtherCar->type == kOCT_RazorTires) && (pOtherOtherCar->type != kOCT_Pig))
               {
                  pOtherOtherCar->flags |= kRF_Crashed;
                  pOtherOtherCar->delayAccum = 0;

                  if (kScoreCrash[pOtherCar->type] > 0)
                     gRaceState.score += kScoreCrash[pOtherCar->type];
               }
            }
         }

         // Car/shot
         if ((gRaceState.flags & kRF_ShotFired) &&
             (abs(pOtherCar->loc.x - gRaceState.shot.x) <= 6) &&
             (abs(pOtherCar->loc.y - gRaceState.shot.y) <= 6))
         {
            pOtherCar->flags |= kRF_Crashed;
            pOtherCar->delayAccum = 0;

            gRaceState.score += kScoreCrash[pOtherCar->type];

            gRaceState.flags &= ~kRF_ShotFired;
         }

         if ((gRaceState.flags & kRF_BackFired) &&
             (abs(pOtherCar->loc.x - gRaceState.backshot.x) <= 6) &&
             (abs(pOtherCar->loc.y - gRaceState.backshot.y) <= 6))
         {
            pOtherCar->flags |= kRF_Crashed;
            pOtherCar->delayAccum = 0;

            gRaceState.score += kScoreCrash[pOtherCar->type];

            gRaceState.flags &= ~kRF_BackFired;
         }


      }

      // Check for car/ai collision
      for (i=0; i<gRaceState.others.Size(); i++)
      {
         sOtherCar *pOtherCar = &gRaceState.others[i];

         // You can drive through wrecked cars
         if (pOtherCar->flags & kRF_Crashed)
            continue;

         int x_dist;
         int y_dist;

         x_dist = abs(gRaceState.car.x - pOtherCar->loc.x);
         y_dist = abs(gRaceState.car.y - pOtherCar->loc.y);

         if ((x_dist < 12) && (y_dist < 12))
         {
            // Collision, do different things based on type
            if ((pOtherCar->type == kOCT_Pedestrian) ||
                (pOtherCar->type == kOCT_Shooter))
            {
               // bounce
               if (gRaceState.car.x > pOtherCar->loc.x)
               {
                  gRaceState.car.x++;
                  pOtherCar->velocity.x -= 1.0;
               }
               else
               {
                  gRaceState.car.x--;
                  pOtherCar->velocity.x += 1.0;
               }

               if (gRaceState.car.y > pOtherCar->loc.y)
               {
                  gRaceState.car.y++;
                  pOtherCar->velocity.y -= 1.0;
               }
               else
               {
                  gRaceState.car.y--;
                  pOtherCar->velocity.y += 1.0;
               }
            }
            else
            {
               Crash();
               break;
            }
         }
      }

      // Check for shot/wall collision
      if ((gRaceState.shot.y >= kFrameHeight - 5) ||
          (gRaceState.shot.x - 3 <= gRaceState.left[gRaceState.shot.y / 3]) ||
          (gRaceState.shot.x + 3 >= gRaceState.right[gRaceState.shot.y / 3]))

      {
         gRaceState.flags &= ~kRF_ShotFired;
      }

      if ((gRaceState.backshot.y <= 5) ||
          (gRaceState.backshot.x - 3 <= gRaceState.left[gRaceState.backshot.y / 3]) ||
          (gRaceState.backshot.x + 3 >= gRaceState.right[gRaceState.backshot.y / 3]))

      {
         gRaceState.flags &= ~kRF_BackFired;
      }

      // Check for car/wall collision
      if ((gRaceState.car.x - 3 <= gRaceState.left[gRaceState.car.y / 3]) ||
          (gRaceState.car.x + 3 >= gRaceState.right[gRaceState.car.y / 3]))
      {
         Crash();
      }
   }

   // Cap score 
   if (gRaceState.score < 0)
      gRaceState.score = 0;
}

void RacerMouse(Point pt)
{
   // Clicking restarts the game if you're in GameOver
   if (gRaceState.flags & kRF_GameOver)
   {
      SetGame(kGameRacer);
      return;
   }

   // can only shoot one at a time, and not while crashed
   if (gRaceState.flags & (kRF_ShotFired | kRF_Crashed))
      return;

   gRaceState.shot.x = gRaceState.car.x;
   gRaceState.shot.y = gRaceState.car.y + 5;

   gRaceState.flags |= kRF_ShotFired;

}

void RacerRMouse(Point pt)
{
   if (!ShockMiniGameIsHacked())
      return;

   if (gRaceState.flags & (kRF_BackFired | kRF_Crashed))
      return;

   gRaceState.backshot.x = gRaceState.car.x;
   gRaceState.backshot.y = gRaceState.car.y - 5;

   gRaceState.flags |= kRF_BackFired;
}


