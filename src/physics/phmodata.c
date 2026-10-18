/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmodata.c,v 1.15 2000/02/19 12:32:18 toml Exp $
//
// Physics motion data
//

#include <phmodata.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

sPlayerMotionData PlayerMotionTable[] = 
{
   // Name, duration, hold length, num submodels affected, { submod, offset }, { submod, offset }, ...

   // Stand
   { kMoNormal,       0.8f, 0.0f, 1, {0, {0,  0,  0} } },
   { kMoStrideLeft,   0.6f, 0.01f, 1, {0, {0, -0.1f,  -0.4f} } },
   { kMoStrideRight,  0.6f, 0.01f, 1, {0, {0, 0.1f,  -0.4f} } },

   // Crouch
   { kMoCrouch,       0.8f, 0.0f, 2, {0, {0,  0, -2.02f}, {2, {0, 0, -1} } } },
   { kMoCrawlLeft,    0.6f, 0.01f, 2, {0, {0, -0.15f,  -2.5f}, {2, {0, 0, -1} } } },
   { kMoCrawlRight,   0.6f, 0.01f, 2, {0, {0, 0.15f,  -2.5f}, {2, {0, 0, -1} } } },

   // BodyCarry
   { kMoWithBody,     0.8f, 0.0f, 1, {0, {0,  0,  -0.8f} } },
   { kMoWithBodyLeft, 0.6f, 0.01f, 1, {0, {0, -0.5f, -1.5f} } },
   { kMoWithBodyRight,0.6f, 0.01f, 1, {0, {0, 0.15f, -1.1f} } },

   // General
   { kMoJumpLand,     0.0f, 0.1f, 1, {0, {0,  0, -.5f} } },
   { kMoWeaponSwing,  0.6f, 0.01f, 1, {0, {0.8f,  0,  0} } },
   { kMoWeaponSwingCrouch,  0.6f, 0.01f, 2, {0, {0.8f,  0,  -2.02f} , {2, {0.8f, 0, -1} } } },

   // Leaning
   { kMoLeanLeft,     1.5f, 0.0f, 1, {0, {0,  2.2f, 0} } },
   { kMoLeanRight,    1.5f, 0.0f, 1, {0, {0, -2.2f, 0} } },
   { kMoLeanForward,  1.5f, 0.0f, 1, {0, {2.2f, 0, 0} } },

   // Crouch-leaning
   { kMoCrouchLeanLeft,    1.5f, 0.0f, 2, {0, {0, 1.7f, -2}, {2, {0, 0, -1} } } },
   { kMoCrouchLeanRight,   1.5f, 0.0f, 2, {0, {0, -1.7f, -2}, {2, {0, 0, -1} } } },
   { kMoCrouchLeanForward, 1.5f, 0.0f, 2, {0, {1.7f, 0, -2}, {2, {0, 0, -1} } } },
};
