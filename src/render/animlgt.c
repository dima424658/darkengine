/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/animlgt.c,v 1.35 2000/02/02 21:19:57 bfarquha Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   animlgt.c

   Here's the animated static lights; that is, lights for which you
   can change the brightness at runtime--including setting it to
   zero--but not the position.  All such lights are generated from the
   AnimLightProp property.

   Each cell in the world rep has a list of the animated lights which
   reach it, and each polygon has a bitmask matching its cell's list.
   When we change a light's intensity, we set a bit corresponding to
   that light in every cell the light reaches.  That work is internal
   to this module.  Do not try this at home.

   Portal knows nothing about light timing.  It has a uchar* telling
   it how bright each animated light is in the current frame; this is
   used for building surfaces.  And it has a callback for updating
   timings each frame, into which it passes the time change in
   milliseconds since the previous frame.  Cell marking also takes
   place within that callback.  There is no requirement that
   lights be set through the callback; it's just there so Portal
   won't have to know what system controls what light, and to make
   frame-by-frame updates part of the routine.

   The surface building code is in portal\portsurf.c.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#include <stdio.h>
#include <stdlib.h>

#include <lg.h>
#include <mprintf.h>

#include <wr.h>
#include <portal.h>

#ifdef EDITOR
#include <editbr.h>
#include <brinfo.h>
#endif

#include <objlight.h>
#include <mlight.h>
#include <objtype.h>
#include <sdesbase.h>
#include <sdesedit.h>
#include <sdesc.h>
#include <sdesedst.h>

#include <litprop.h> //zb
#include <litdbg.h>

#include <animlgt.h>
#include <animlgts.h>
#include <rand.h>

#include <dbmem.h>

EXTERN int num_light;
EXTERN int num_dyn;
EXTERN mls_multi_light light_data[];
EXTERN mls_multi_light light_this[];

// These are in Portal.c.  It's a little bit curious that we're
// duplicating the constant.
#define MAX_LIT_CELLS 512
EXTERN int lit_cell[MAX_LIT_CELLS];
EXTERN int num_lit;
EXTERN int num_culled;

int g_iCurAnimLightToCell = 0;
sAnimLightToCell g_aAnimLightToCell[MAX_ANIM_LIGHT_TO_CELLS];

// A RAND_COHERENT light changes up to some amount per frame, as a
// fraction of the total range of intensities the light can assume.
// (Actually, it changes +/- half this amount.)
#define RAND_VARIATION .6

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This ain't much since our allocation is all static.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void AnimLightClear(void)
{
   g_iCurAnimLightToCell = 0;
   num_lit = 0;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This should be called exactly once, exactly early on.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void AnimLightInit(void)
{
   portal_anim_light_intensity = (uchar *)Malloc(sizeof(uchar)
                                               * MAX_ANIM_LIGHTS);
   AnimLightClear();
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This should be called exactly once, when we're closing our app.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void AnimLightCleanup(void)
{
   Free(portal_anim_light_intensity);
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

  This flags all the cells reached by a given animated light, to
  force surface rebuilding, and clears the light's refresh flag.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void AnimLightFlagCells(sAnimLight *light, ObjID obj )
{
   PortalCell *cell;
   int c, num_cells;
   sAnimLightToCell *light_to_cell;
   int index = light->light_data_index;
   float intensity = light->brightness;

   if (index == -1)
      return;

#ifndef SHIP
   if (light->max_brightness == 0)
      mprintf("This animated light has a maximum brightness of 0!");
#endif // ~SHIP

   // The first of the arrays we set here is for object lighting, and
   // wants a float from 0 to 1, while the second is for surface
   // building and wants a uchar.  Our starting values are from 0 to
   // 1024.
#ifdef RGB_LIGHTING //zb
   {
      float hue,saturation;
      rgb_vector colector;
      int r,g,b;
#ifdef SHIP
      ObjColorGet(obj, &hue, &saturation);
#else
      if (!ObjColorGet(obj, &hue, &saturation))
         if (config_is_defined("warn_nocolor_lights"))
            mprintf("No color for light %d\n",obj);
#endif
      portal_convert_hsb_to_rgb(&r,&g,&b,hue,saturation);
      colector.x = (r * intensity) / 255.0f;
      colector.y = (g * intensity) / 255.0f;
      colector.z = (b * intensity) / 255.0f;
      portal_set_normalized_color(r,g,b);

      set_object_light(index, intensity, &colector);
   }
#else
   set_object_light(index, intensity);
#endif

   portal_anim_light_intensity[index]
      = (uchar) (intensity * (255.0 / light->max_brightness));

   num_cells = light->num_cells_reached;
   light_to_cell = &g_aAnimLightToCell[light->first_light_to_cell];

   watch_light_mprint(obj,("Hits %d cells, index %d, intensity %d, posinpal %d\n",
         num_cells, index, portal_anim_light_intensity[index],
         light_to_cell->pos_in_cell_palette));

   for (c = 0; c < num_cells; ++c)
   {
      watch_light_mprint(obj,("%d.",light_to_cell->cell_index));
      if (light_to_cell->cell_index < wr_num_cells)
      {
         cell = WR_CELL(light_to_cell->cell_index);

         // mark this cell as having been touched by our light
         cell->changed_anim_light_bitmask
            |= (1 << (light_to_cell->pos_in_cell_palette));
      }
      light_to_cell++;
   }
   watch_light_mprint(obj,("\n"));

   light->refresh = FALSE;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   When we add a new light to a cell, all the lights already reaching
   that cell now have different indices in its palette of animated
   lights.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void UpdatePositionsInCellPalettes(int cell_index)
{
   int i;

   for (i = 0; i < g_iCurAnimLightToCell; i++)
      if (g_aAnimLightToCell[i].cell_index == cell_index)
         g_aAnimLightToCell[i].pos_in_cell_palette++;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Each light reaches some number of cells; each such connection is
   stored in an sAnimLightToCell structure.  This should be called
   immediately after the appropriate light has been lit in the world
   rep since it depends on a cell list generated by that operation.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void AnimLightSetCellList(sAnimLight *light, uint light_data_index)
{
   int i;
   int num_reached = num_lit - num_culled;

   if ((g_iCurAnimLightToCell + num_reached) >= MAX_ANIM_LIGHT_TO_CELLS) {
#ifndef SHIP
      mprintf("Ran out of pairings from anim lights to cells!\n");
#endif
      return;
   }

   light->num_cells_reached = num_reached;
   light->first_light_to_cell = g_iCurAnimLightToCell;
   light->light_data_index = light_data_index;
   light->refresh = TRUE;

   for (i = 0; i < num_reached; ++i) {
      int cell_index = lit_cell[i];

      PortalCell *cell = WR_CELL(cell_index);

      cell->num_anim_lights++;

      if (cell->anim_light_index_list) {
         cell->anim_light_index_list
            = (ushort *) Realloc(cell->anim_light_index_list,
                                 sizeof(ushort) * cell->num_anim_lights);

         // make room for the new light at the head of the cell's list
         memmove(&cell->anim_light_index_list[1],
                 &cell->anim_light_index_list[0],
                 sizeof(ushort) * (cell->num_anim_lights - 1));

         UpdatePositionsInCellPalettes(cell_index);
      } else
         cell->anim_light_index_list = (ushort *) Malloc(sizeof(ushort));

      cell->anim_light_index_list[0] = light_data_index;

      g_aAnimLightToCell[g_iCurAnimLightToCell + i].cell_index = cell_index;
      g_aAnimLightToCell[g_iCurAnimLightToCell + i].pos_in_cell_palette = 0;
   }

   g_iCurAnimLightToCell += num_reached;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This advances the light's timer, and if necessary flags the light
   as having changed, so that the surfaces it reaches must be rebuilt.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
#define FLICKER_FACTOR  0.5
BOOL AnimLightUpdateTimer(sAnimLight *light, long time_change)
{
   float brightness, old_brightness, weight, range, change;
   long time;
   BOOL changed_state = FALSE;

   if (light->inactive)
      return FALSE;

   // Any really silly setting gets clamped to about 16 fps.
   if (light->time_rising_ms < 5)
      light->time_rising_ms = 63;
   if (light->time_falling_ms < 5)
      light->time_falling_ms = 63;

   time = light->countdown_ms - time_change;

   // advance until we've accounted for the full time change
   while (time < 0) {
      changed_state = TRUE;

      light->is_rising ^= TRUE;

      // have we reached a deactivating limit case?
      if (((light->mode == ANIM_LIGHT_MODE_SMOOTH_BRIGHTEN)
        && (!light->is_rising))
       || ((light->mode == ANIM_LIGHT_MODE_SMOOTH_DIM)
        && (light->is_rising))) {
         light->inactive = TRUE;

         // throw away all excess frame time in these cases to prevent
         // further animation past the limits
         time = 0;
      }

      if (light->mode == ANIM_LIGHT_MODE_FLICKER)
      {
         float randfactor;
         int interval;
         if (light->is_rising)
            interval = light->time_rising_ms;
         else
            interval = light->time_falling_ms;
         randfactor = (1.0 - FLICKER_FACTOR) + ((FLICKER_FACTOR * 2) * (float)Rand() / (float) RAND_MAX);
         interval = interval * randfactor;
         //mprintf("randfactor %f, interval %d\n",randfactor,interval);
         time += interval;
      }
      else
      {
         if (light->is_rising)
            time += light->time_rising_ms;
         else
            time += light->time_falling_ms;
      }
   }

   light->countdown_ms = time;
   old_brightness = light->brightness;

   switch (light->mode) {
      case ANIM_LIGHT_MODE_FLIP:
      case ANIM_LIGHT_MODE_FLICKER:
         if (light->is_rising)
            brightness = light->min_brightness;
         else
            brightness = light->max_brightness;
         break;

      case ANIM_LIGHT_MODE_SMOOTH:
      case ANIM_LIGHT_MODE_SMOOTH_BRIGHTEN:
      case ANIM_LIGHT_MODE_SMOOTH_DIM:
         range = light->max_brightness - light->min_brightness;
         if (light->is_rising) {
            weight = (float) time / (float) light->time_rising_ms;
            brightness = light->min_brightness + (1 - weight) * range;
         } else {
            weight = (float) time / (float) light->time_falling_ms;
            brightness = light->min_brightness + weight * range;
         }
         break;

      case ANIM_LIGHT_MODE_RANDOM:
         if (changed_state) {
            range = light->max_brightness - light->min_brightness;
            brightness = light->min_brightness
               + (float) Rand() * (1.0 / (float) RAND_MAX) * range;
         } else
            brightness = old_brightness;
         break;

      case ANIM_LIGHT_MODE_MINIMUM:
         brightness = light->min_brightness;
         break;

      case ANIM_LIGHT_MODE_MAXIMUM:
         brightness = light->max_brightness;
         break;

      case ANIM_LIGHT_MODE_EXTINGUISH:
         brightness = 0.0;
         break;

      case ANIM_LIGHT_MODE_RAND_COHERENT:
         if (changed_state) {
            range = light->max_brightness - light->min_brightness;

            change = (float) (Rand() - 16384) * (1.0 / (float) RAND_MAX)
                   * (range * RAND_VARIATION);

            brightness = old_brightness + change;

            if ((brightness > light->max_brightness && change > 0)
             || (brightness < light->min_brightness && change < 0))
               brightness = old_brightness - (change * .5);
         } else
            brightness = old_brightness;
         break;

      default:
         break;
   }

   if (brightness != old_brightness) {
      light->refresh = TRUE;
      light->brightness = brightness;
   }

   return changed_state;
}
