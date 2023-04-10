/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/portold/animlit.c,v 1.2 1997/06/14 06:00:23 MAT Exp $
///////////////////////////////////////////////////////////////
// animated lighting--now, with lightmaps!

#include <lg.h>
#include <stdlib.h>
#include <ctype.h>
#include <animlit.h>
#include <fix.h>



  // max number of lights, at 8 bytes apiece
#define MAX_LIGHTS 1024

  // max number of animating lights; of course, non-animating
  // lights should probably not go through this system, unless
  // you just use it with your own system for modifying lights
#define MAX_ANIM_LIGHTS  MAX_LIGHTS

  // max number of light-vs-cell interactions
#define MAX_LIGHT_REGIONS  8192

  // max number of light-vs-vertex interactions
static unsigned char light_raw_data[1 << 17];   // 128K

typedef struct
{
   uint16  cell_id;
   uint8 num_verts;
   uint8 surf_offset;
} CellLightInfo;

//static CellLightInfo cell_info[MAX_LIGHT_REGIONS];

typedef struct
{
     // generic database stuff
   uint32 light_raw_offset;   // offset into light_raw_data
   uint16 num_cells;          // number of cells we affect
   uint16 first_cell;         // offset into cell_info of first one

     // current light animation info
   uint16 current_brightness;
   uint16 default_brightness;
   uint16 anim;
   uint16 padding;
} LightInfo;

static LightInfo light_info[MAX_LIGHTS];

static int num_lights;
static int num_cells, data_used;

//   Do a lighting delta for a light by a particular brightness
//   This is an internal function.

//  Turn lighting to a particular brightness
//  This is mostly an internal function

void LightClear(void)
{
//   max_light_anims = 0;
   num_lights = 0;
   num_cells = 0;
   data_used = 0;
}



void LightSetBrightness(int t, int b)
{
   if (light_info[t].current_brightness == b)
      return;
#if 0
   if (light_info[t].current_brightness)
      LightAdjust(t, -light_info[t].current_brightness);

   LightAdjust(t, b);
#endif

   light_info[t].current_brightness = b;
}


////////////////////////////////////////////////////////////////
//
// Here on down is the old vertex-based animated lighting.
// None of this code is active.
//


//  Vertex-based lighting animation


//
// The basic idea is that for each animating light,
// we keep a list of all the vertices it affected,
// and what the brightness of that was.
//
// If we want to support dynamically loaded cells
// (which the rest of portal supports), we probably
// need to make this data dynamically loaded.  That
// means inverting it so each cell stores it's associated
// lighting information, which isn't worth my effort
// right now.
//
// Once we have production levels, we should collect
// stats and see if we should compress the data more
// (e.g. do we have lots of runs of 0).  It might also
// turn out that we could supercompress it by using
// less precision (maybe only 4 or 5 bits), and then
// delta encoding that (producing even more 0s), or
// some such.

  // I think lg.h secretly includes some old version of sean.h,
  // so we don't need these.
#if 0
typedef unsigned  char uint8;
typedef unsigned short uint16;
typedef unsigned  long uint32;

typedef   signed  char  int8;
typedef          short  int16;
typedef           long  int32;
#endif


#if 0
//  Turn lighting off

void LightTurnOff(int t)
{
   LightSetBrightness(t, 0);
}


//  Turn lighting on

void LightTurnOn(int t)
{
   LightSetBrightness(t, light_info[t].default_brightness);
}


//  Set the brightness for a light without changing its current state

void LightSet(int t, int b)
{
   light_info[t].default_brightness = b;
}


//  Turn lighting on setting the brightness

void LightTurnOnSet(int t, int b)
{
   LightSet(t,b);
   LightTurnOn(t);
}


//  Check if light is on (non-zero) or off.  This doesn't
//  check the current against default or anything like that.

bool IsLightOn(int t)
{
   return light_info[t].current_brightness != 0;
}

//  Toggle lighting

void LightToggle(int t)
{
   if (IsLightOn(t))
      LightTurnOff(t);
   else
      LightTurnOn(t);
}

//
//
//
////////////////////////////////////////////////////
//
//
//    animating light system
//
//  for each light currently in the animation system,
//  we do wacky automatic processing.
//
//  the basic moves a light can make are:
//
//    1    turn on in X time (X can be 0)
//    0    turn off in X time (X can be 0)
//    b    set a new brightness value X
//    w    delay for time X
//    r    execute next instruction with random range from this X to it's X
//    c    branch to beginning of script
//    s    skip next instruction X/256 % of the time
//    j    jump relative by X instructions (from the j)
//    .    end animation
//     
//  'r' does not work as a prefix on 's'
//
//  you can make a random jump table with 'rj' which then
//  jump into a collection of jumps
//
//  So we have a simple light scripting language
//  which defines how a light behaves.
//
//  Time is in "ticks", which are 70 times/second
//  Actually, they're in whatever the app makes them

enum
{
   LA_NOP,
   LA_DELAY,
   LA_FADE
};

typedef struct
{
   uchar *script;
   uint16 iptr;
   uint16 light;
   uint16 state;
   uint16 delay;
   fix cur_bright;
   fix delta_bright;
} LightAnimInfo;

//LightAnimInfo anim_info[MAX_ANIM_LIGHTS];
//static int max_light_anims=0;

int LightAnimAlloc(int light)
{
   // if we already have an animation playing, screw it
   // this shouldn't happen because SetScript should free it

   if (light_info[light].anim)
      return 0;

   if (max_light_anims < MAX_LIGHTS) {
      int a = ++max_light_anims;
      anim_info[a].light = light;
      anim_info[a].state = LA_DELAY;
      anim_info[a].delay = 1;
      anim_info[a].iptr  = 0;

      light_info[light].anim = a;
      return a;
   }
   return 0;
}

void LightAnimFree(int a)
{
   light_info[anim_info[a].light].anim = 0;
   if (a < max_light_anims) {
      anim_info[a] = anim_info[max_light_anims];
      light_info[anim_info[a].light].anim = a;
   }
   --max_light_anims;
}

void LightAnimSetScript(int t, uchar *script)
{
   int a;
   // check if this light already has a script

   if (light_info[t].anim)
      LightAnimFree(light_info[t].anim);

   if (!script)
      return;

   a = LightAnimAlloc(t);
   if (!a)
      return;    // so sad!
   anim_info[a].script = script;
}

fix LightDefault(int t)
{
   return fix_make(light_info[t].default_brightness, 0x8000);
}

bool LightAnimPlayScript(int a, int ticks)
{
   uchar *script = anim_info[a].script;
   int i = anim_info[a].iptr;
   int j = anim_info[a].light;
   int random_base = -1, parm;
   bool turn_off = FALSE;

   while (ticks) {
      if (anim_info[a].delay) {
         int t = anim_info[a].delay;
         if (ticks < t)
            t = ticks;

         if (anim_info[a].state == LA_FADE)
            anim_info[a].cur_bright += anim_info[a].delta_bright * t;

         ticks -= t;

         anim_info[a].delay -= t;
         if (anim_info[a].delay)
            goto done;
      }

      // compute parameter to next element
      parm = script[i+1];
      if (random_base != -1) {
         parm = (rand() >> 2) % (parm - random_base) + random_base;
         random_base = -1;
      }

      switch (tolower(script[i])) {
         case 'b':
            LightSet(j, parm);
            break;
         case 'w':
            anim_info[a].state = LA_DELAY;
            anim_info[a].delay = parm;
            break;
         case 'r':
            random_base = parm;
            break;
         case 0:    // go off the end, start over!
         case 'c':
            i = 0;
            goto no_skip;
         case 's':
            if (((rand() >> 3) & 255) < script[i+1])
               i += 2;
            break;
         case 'j':
            i = i + 2 * ((signed char) script[i+1]);
            goto no_skip;
         case '1':
            if (parm) {
               anim_info[a].state = LA_FADE;
               anim_info[a].delta_bright =
                 (LightDefault(j) - anim_info[a].cur_bright) / parm;
               anim_info[a].delay = parm;
            } else {
               anim_info[a].cur_bright = LightDefault(j);
            }
            break;
         case '0':
            if (parm) {
               anim_info[a].state = LA_FADE;
               anim_info[a].delta_bright =
                    (0 - anim_info[a].cur_bright) / parm;
               anim_info[a].delay = parm;
            } else {
               anim_info[a].cur_bright = 0;
            }
            break;
         case '.':
            turn_off = TRUE;
            goto done;
      }

      i += 2;

     no_skip:
      ;
   }
done:
   anim_info[a].iptr = i;
   LightSetBrightness(anim_info[a].light, fix_int(anim_info[a].cur_bright));
   return turn_off;
}

void LightAnimPlayScripts(int ticks)
{
   int i;
   for (i=1; i <= max_light_anims;) {
      if (LightAnimPlayScript(i, ticks))
         LightAnimFree(i);
      else
         ++i;
   }
}


//
//
//
//
////////////////////////////////////////////////////
//
//   Interface to build the lighting data stuff   //
//
//  The calling convention is this:
//
//     LightDefineStart();
//        LightCellStart(cell_number);
//           LightAtVertex(int vertex, uchar lighting);
//        LightCellEnd();
//     lt = LightDefineEnd();
//
//  Within any LightCell block, all vertex numbers
//  passed in must be ascending.  They do not have to
//  be sequential, however, they are treated as if
//  they are (gaps are filled with 0s).
//
//  Empty cells (i.e. do a LightCellStart, but never do any
//  LightAtVertex) do not cost any storage, since we allocate
//  cell storage only on the first call to LightAtVertex
//

static int skipped;
static int added;

void LightDefineStart(void)
{
   light_info[num_lights].light_raw_offset = data_used;
   light_info[num_lights].first_cell = num_cells;
   light_info[num_lights].num_cells = 0;
   light_info[num_lights].current_brightness = 0;
   light_info[num_lights].default_brightness = 0;
   light_info[num_lights].anim = 0;

   skipped = added = 0;
}

int LightDefineEnd(void)
{
   return num_lights++;
}

static int current_loc, current_cell;
static int last_vertex;

void LightCellStart(int cell)
{
   current_loc = -1;
   current_cell = cell;
}

void LightCellEnd(void)
{
}


void LightAdjust(int t, int b)
{
   int i,r,n,z,k, c;
   uchar a, *data;
   bool negative;
   r = light_info[t].first_cell;
   n = light_info[t].num_cells;

   z = light_info[t].light_raw_offset;

   negative = b < 0;
   if (negative) b = -b;

   while (n--) {
      k = cell_info[r].num_verts;
      i = cell_info[r].surf_offset;
      data = portal_get_lighting_data(cell_info[r].cell_id) + i;
      if (!negative)
         while (k--) {
            a = light_raw_data[z++];
            if (a) {
               c = (int) *data + ((a * b) >> 8);
               *data = (c < 255 ? c : 255);
            }
            ++data;
         }
      else
         while (k--) {
            a = light_raw_data[z++];
            if (a) {
               c = (int) *data - ((a * b) >> 8);
               *data = (c > 0 ? c : 0);
            }
            ++data;
         }
      ++r;
   }
}


void LightAtVertex(int vertex, uchar lighting)
{
   if (current_loc == -1) {
      // allocate cell

      current_loc = num_cells++;
      ++light_info[num_lights].num_cells;

      cell_info[current_loc].surf_offset = vertex;
      cell_info[current_loc].num_verts = 0;
      cell_info[current_loc].cell_id = current_cell;

   } else if (vertex != last_vertex + 1) {
      int i;
#if 0
      if (vertex <= last_vertex) {
         mprintf("Vertices out of order in LightAtVertex\n");
         return;
      }
#endif
      cell_info[current_loc].num_verts += vertex - last_vertex - 1;
      skipped += vertex - last_vertex - 1;
      for (i = last_vertex + 1; i < vertex; ++i)
         light_raw_data[data_used++] = 0;
   }

   ++cell_info[current_loc].num_verts;
   light_raw_data[data_used++] = lighting;
   last_vertex = vertex;
   ++added;
}
#endif

