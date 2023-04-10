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
   { kMoNormal,       0.8, 0.0, 1, {0, {0,  0,  0} } },
   { kMoStrideLeft,   0.6, 0.01, 1, {0, {0, -0.1,  -0.4} } },
   { kMoStrideRight,  0.6, 0.01, 1, {0, {0, 0.1,  -0.4} } },

   // Crouch
   { kMoCrouch,       0.8, 0.0, 2, {0, {0,  0, -2.02}, {2, {0, 0, -1} } } },
   { kMoCrawlLeft,    0.6, 0.01, 2, {0, {0, -0.15,  -2.5}, {2, {0, 0, -1} } } },
   { kMoCrawlRight,   0.6, 0.01, 2, {0, {0, 0.15,  -2.5}, {2, {0, 0, -1} } } },

   // BodyCarry
   { kMoWithBody,     0.8, 0.0, 1, {0, {0,  0,  -0.8} } },
   { kMoWithBodyLeft, 0.6, 0.01, 1, {0, {0, -0.5, -1.5} } },
   { kMoWithBodyRight,0.6, 0.01, 1, {0, {0, 0.15, -1.1} } },

   // General
   { kMoJumpLand,     0.0, 0.1, 1, {0, {0,  0, -.5} } },
   { kMoWeaponSwing,  0.6, 0.01, 1, {0, {0.8,  0,  0} } },
   { kMoWeaponSwingCrouch,  0.6, 0.01, 2, {0, {0.8,  0,  -2.02} , {2, {0.8, 0, -1} } } },

   // Leaning
   { kMoLeanLeft,     1.5, 0.0, 1, {0, {0,  2.2, 0} } },
   { kMoLeanRight,    1.5, 0.0, 1, {0, {0, -2.2, 0} } },
   { kMoLeanForward,  1.5, 0.0, 1, {0, {2.2, 0, 0} } },

   // Crouch-leaning
   { kMoCrouchLeanLeft,    1.5, 0.0, 2, {0, {0, 1.7, -2}, {2, {0, 0, -1} } } },
   { kMoCrouchLeanRight,   1.5, 0.0, 2, {0, {0, -1.7, -2}, {2, {0, 0, -1} } } },
   { kMoCrouchLeanForward, 1.5, 0.0, 2, {0, {1.7, 0, -2}, {2, {0, 0, -1} } } },
};
