/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/cvtbrush.c,v 1.7 2000/02/19 12:27:53 toml Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   cvtbrush.c

   The sides of groups of cells with persistant media are preserved
   through portalization, since to the portalizer there's a medium
   boundary. They are used where we want the cell database to include
   particular surfaces (as portals, that is).  Once we have our portal
   database, we convert the persistant media to regular media, here.

   We have some autohose medium portals in nonpersistant cells,
   because they lead into persistant ones.  These, too, must die.

   We only use this for doors, so far, and it's a bit hard-coded.
   In fact, we have no way to specify how the persistant cells are
   used, so if we want to generalize this setup we'll have to mark 
   the persistant brushes and propagate that somehow.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#include <config.h>
#include <mprintf.h>

#include <media.h>
#include <portal.h>
#include <wrdbrend.h>
#include <texmem.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This is a helper for ConvertRemovePersistance(), which is a helper
   for ConvertPersistantCells(), both of which are below.

   Given a medium, this tells us what medium it will be when we're done
   converting them all.

   And still, good help is so hard to find these days.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
uchar ConvertFindFinalMedium(uchar medium)
{
      if (medium >= MEDIA_SOLID_PERSIST
       && medium <= MEDIA_WATER_PERSIST)
         return medium - (MEDIA_SOLID_PERSIST - MEDIA_SOLID);
      else
         return medium;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This is a helper for ConvertPersistantCells(), below.

   This makes sure the extraneous portals we've created will not be
   drawn.  We iterate over all the rendered surfaces of a cell, and
   those which are marked as medium boundaries get their flags
   fiddled.  We have to make an exception for those which represent
   _real_ medium boundaries so we can have doors in half-flooded areas
   (and because it's the correct thing to do).

   This is called on both cells with persistant media and the cells
   adjacent to them, and only recurses to adjacent cells if we're in a
   persistant one.  So it's called before the medium is changed so we
   can tell which kind we're in.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void ConvertRemovePersistance(PortalCell *cell, uchar final_medium)
{
   PortalPolygonCore *poly_core;
   PortalPolygonRenderInfo *render_info;
   PortalCell *dest_cell;
   uchar dest_final_medium;
   uchar num_solid_polys, num_rendered_portals;
   int i;

   num_solid_polys = cell->num_polys - cell->num_portal_polys;
   num_rendered_portals = cell->num_render_polys - num_solid_polys;

#ifndef SHIP
   if (config_is_defined("CvtBrushSpew")) {
      mprintf("\nsolid_polys = %d  polys = %d  portal_polys = %d",
              (int) num_solid_polys, 
              (int) cell->num_polys, 
              (int) cell->num_portal_polys);

      mprintf(" rendered_portals = %d render_polys = %d\n",
              (int) num_rendered_portals,
              (int) cell->num_render_polys);
   }
#endif // ~SHIP

   poly_core = cell->portal_poly_list;
   render_info = cell->render_list + num_solid_polys;

   for (i = 0; i < num_rendered_portals; i++) {

      // Where are we going from the portal, and what medium are we
      // headed into?  Accounting for persistant media, of course.
      // If there's a medium difference, there's a clut.
      if (poly_core->clut_id) {
         dest_cell = wr_cell[poly_core->destination];
         dest_final_medium = ConvertFindFinalMedium(dest_cell->medium);

#ifndef SHIP
         if (config_is_defined("CvtBrushSpew"))
            mprintf("medium: %d  dest_medium: %d",
                    final_medium, dest_final_medium);
#endif // ~SHIP

         // If our final medium is the same as that of the cell we reach
         // through this portal, we don't want to render the boundary.
         if (dest_final_medium == final_medium) {
            render_info->texture_id = 0;
            poly_core->flags &= ~(PORTAL_SPLITS_OBJECT | RENDER_DOESNT_LIGHT);

            // If we're in a cell with a persistant medium, the cell
            // on the other side of this portal may have unwanted
            // visible medium boundaries.
            if (cell->medium != final_medium)
               ConvertRemovePersistance(dest_cell, dest_final_medium);
         } else
            if (final_medium == MEDIA_WATER)
               render_info->texture_id = WATEROUT_IDX;
            else 
               render_info->texture_id = WATERIN_IDX;
      }
      poly_core++;
      render_info++;
   }
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   The outer loop handles only those cells with persistant media, so
   it's the place to change their medium flags.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void ConvertPersistantCells(uchar added_flags)
{
   PortalCell *cell;
   int i;
   uchar final_medium;

   for (i = 0; i < wr_num_cells; i++) {
      cell = wr_cell[i];
      final_medium = ConvertFindFinalMedium(cell->medium);

      if (cell->medium != final_medium) {
         ConvertRemovePersistance(cell, final_medium);
         cell->medium = final_medium;
         cell->flags |= added_flags;
      }
   }
}


extern BOOL doing_opt_pass1;
// decide what texture to show at boundaries between media
int get_texture_for_medium_transition(int x, int y)
{
   uchar final_x = ConvertFindFinalMedium((uchar) x);
   uchar final_y = ConvertFindFinalMedium((uchar) y);

   if (final_x == MEDIA_AIR && final_y == MEDIA_WATER)
      return WATERIN_IDX;
   if (final_x == MEDIA_WATER && final_y == MEDIA_AIR)
      return WATEROUT_IDX;

   if (doing_opt_pass1 && x != y) {
      // force this to be a surface so the optimizer
      // will know about it
      return 1;
   }

   return -1;
}


// nowadays, we don't actually use the value from this except
// to see if it's non-zero
int get_clut_for_medium_transition(int x, int y)
{
   uchar final_x = ConvertFindFinalMedium((uchar) x);
   uchar final_y = ConvertFindFinalMedium((uchar) y);

   if (final_x == MEDIA_AIR && final_y == MEDIA_WATER)
      return 1;   // going from air into water
   if (final_x == MEDIA_WATER && final_y == MEDIA_AIR)
      return 2;   // going from water into air

   return 0;
}

