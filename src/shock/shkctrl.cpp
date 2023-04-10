/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkctrl.cpp,v 1.8 2000/02/19 12:36:44 toml Exp $

#include <math.h>
#include <kbcook.h>
#include <keydefs.h>

#include <mouse.h>
#include <2d.h>
#include <config.h>
#include <matrix.h>
#include <objpos.h>
#include <portal.h>
#include <camera.h>

#include <physapi.h>
#include <playrobj.h>

#include <shkctrl.h>
#include <shkplayr.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// these are stolen from gamemode.c and why not?
#define KEYS_WALK      case 's': case KEY_UP: case KEY_PAD_CENTER
#define KEYS_RUN       case 'w': case KEY_UP|KB_FLAG_SHIFT: case KEY_PAD_UP: case KEY_PAD_UP|KB_FLAG_SHIFT: case KEY_PAD_CENTER|KB_FLAG_SHIFT
#define KEYS_BACK      case 'x': case KEY_DOWN: case KEY_PAD_DOWN
#define KEYS_SIDELEFT  case 'z': case ',': case KEY_LEFT|KB_FLAG_ALT: case KEY_PAD_LEFT|KB_FLAG_ALT: case KEY_PAD_END
#define KEYS_SIDERIGHT case 'c': case '.': case KEY_RIGHT|KB_FLAG_ALT: case KEY_PAD_RIGHT|KB_FLAG_ALT: case KEY_PAD_PGDN
#define KEYS_TURNLEFT  case 'a': case KEY_LEFT: case KEY_PAD_LEFT
#define KEYS_TURNRIGHT case 'd': case KEY_RIGHT: case KEY_PAD_RIGHT
#define KEYS_FASTLEFT  case 'A': case KEY_LEFT|KB_FLAG_SHIFT: case KEY_PAD_LEFT|KB_FLAG_SHIFT
#define KEYS_FASTRIGHT case 'D': case KEY_RIGHT|KB_FLAG_SHIFT: case KEY_PAD_RIGHT|KB_FLAG_SHIFT
#define KEYS_LOOKUP    case 'r': case KEY_END: case KEY_GREY_MINUS
#define KEYS_LOOKDOWN  case 'v': case KEY_PGDN: case KEY_GREY_PLUS
#define KEYS_LOOKCNTR  case 'f': case KEY_DEL: case KEY_PAD_DEL

// misc magic numbers
#define RAW_SLEW_SPEED 15.0
#define RAW_ROTATE_SPEED 3.14    // 180 degrees/sec

static float g_PlayerSlewSpeedScale =1.0;
static float g_PlayerRotateSpeedScale =1.0;

#define SLEW_SPEED (RAW_SLEW_SPEED*g_PlayerSlewSpeedScale)
#define ROTATE_SPEED (RAW_ROTATE_SPEED*g_PlayerRotateSpeedScale)

#define NET_ROTATE_SPEED      (ROTATE_SPEED / PlayerCamera()->zoom)
#define NET_FAST_ROTATE_SPEED ((3 * ROTATE_SPEED) / (2 * PlayerCamera()->zoom))

bool ShockControlKeyParse(int keycode, ObjID controlObj, float speed)
{
   mxs_vector rotVec = {0, 0, 0};

   if (keycode & KB_FLAG_DOWN)
      switch (keycode & ~KB_FLAG_DOWN)
      {
         KEYS_RUN:       PhysAxisControlVelocity(controlObj, 0, speed*SLEW_SPEED); break;
         KEYS_WALK:      PhysAxisControlVelocity(controlObj, 0, speed*SLEW_SPEED / 2); break;
         KEYS_BACK:      PhysAxisControlVelocity(controlObj, 0, -speed*SLEW_SPEED / 2); break;

         KEYS_SIDELEFT:  PhysAxisControlVelocity(controlObj, 1,  speed*SLEW_SPEED); break;
         KEYS_SIDERIGHT: PhysAxisControlVelocity(controlObj, 1, -speed*SLEW_SPEED); break;

         KEYS_TURNLEFT:  
                         {
                            rotVec.z = speed*NET_ROTATE_SPEED;
                            PhysControlRotationalVelocity(controlObj, &rotVec); 
                         }
                         break;
         KEYS_TURNRIGHT: 
                         {
                            rotVec.z = -speed*NET_ROTATE_SPEED;
                            PhysControlRotationalVelocity(controlObj, &rotVec); 
                         }
                         break;
         KEYS_FASTLEFT:  PhysAxisControlRotationalVelocity(controlObj, 2,  speed*NET_FAST_ROTATE_SPEED); break;
                         {
                            rotVec.z = speed*NET_FAST_ROTATE_SPEED;
                            PhysControlRotationalVelocity(controlObj, &rotVec); 
                         }
                         break;
         KEYS_FASTRIGHT: PhysAxisControlRotationalVelocity(controlObj, 2, -speed*NET_FAST_ROTATE_SPEED); break;
                         {
                            rotVec.z = -speed*NET_FAST_ROTATE_SPEED;
                            PhysControlRotationalVelocity(controlObj, &rotVec); 
                         }
                         break;
         KEYS_LOOKUP:    PhysAxisControlRotationalVelocity(controlObj, 1, -speed*NET_ROTATE_SPEED); break;
         KEYS_LOOKDOWN:  PhysAxisControlRotationalVelocity(controlObj, 1,  speed*NET_ROTATE_SPEED); break;
         KEYS_LOOKCNTR:
         {
            mxs_vector angvel;

            mx_zero_vec(&angvel);
            PhysSetRotationalVelocity(controlObj, &angvel);

            break;
         }
         case 'q':
         {
            PhysAxisControlVelocity(controlObj, 2, speed*SLEW_SPEED);
            break;
         }
         case 'e':
         {
            PhysAxisControlVelocity(controlObj, 2, -speed*SLEW_SPEED);
            break;
         }
         default: 
            return FALSE;
      }
   else
      switch (keycode)     // if not down, then up, no flag to mask out
      {
         KEYS_RUN:
         KEYS_WALK:
         KEYS_BACK:      PhysStopAxisControlVelocity(controlObj, 0); break;

         KEYS_SIDELEFT:
         KEYS_SIDERIGHT: PhysStopAxisControlVelocity(controlObj, 1); break;

         KEYS_TURNLEFT:
         KEYS_TURNRIGHT:

         KEYS_FASTLEFT:
         KEYS_FASTRIGHT:

         KEYS_LOOKUP:
         KEYS_LOOKDOWN:  PhysStopControlRotationalVelocity(controlObj); break;
         case 'q':
         {
            PhysStopAxisControlVelocity(controlObj, 2);
            break;
         }
         case 'e':
         {
            PhysStopAxisControlVelocity(controlObj, 2);
            break;
         }
         default: 
            return FALSE;
      }
   return TRUE;
}

const float kPlayerJumpSpeed = 10;

void ShockPlayerJump(void)
{
   ObjID playerObjID = PlayerObject();
//   AutoAppIPtr(PlayerPsi);

// @TODO: rework psi powers
//   if (pPlayerPsi->IsActive() && (pPlayerPsi->GetPower() == kPsiLevitate))
//      PhysAxisSetVelocity(playerObjID, 2, kPsiLevitateSpeed);
//   else
   {
      mxs_vector velocity;
      
      if (PhysObjOnGround(playerObjID))
      {
         PhysGetVelocity(playerObjID, &velocity);
         velocity.z = kPlayerJumpSpeed;
         PhysSetVelocity(playerObjID, &velocity);
      }
      else if (PhysObjInWater(playerObjID))
      {
         PhysGetVelocity(playerObjID, &velocity);
         velocity.z = kPlayerJumpSpeed/ 4;
         PhysSetVelocity(playerObjID, &velocity);
      }
   }
}
