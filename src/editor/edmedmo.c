/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/edmedmo.c,v 1.20 2000/02/19 13:10:38 toml Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   edmedmo.c

   Here be editor kindsa stuff for water motion structures.  The
   centerpieces are the routines which propagate textures and motion
   through the world rep, and the SDESC.  There's some accessory stuff
   for clearing the database when we load a level.

   See medmotn* for the game-time part of this story (updating water
   texture (u, v)s per frame), and gedmedmo for the connection to the
   brush database.

   See editsave.c for loading and saving of g_aEdMedMoSurface.

   We use motion index 0 as stillness.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#include <stdio.h>
#include <string.h>

#include <lg.h>
#include <matrix.h>
#include <mprintf.h>
#include <cfgdbg.h>

#include <portal.h>
#include <wr.h>
#include <wrdbrend.h>

#include <cvtbrush.h>
#include <lresname.h>
#include <sdesc.h>
#include <sdesbase.h>
#include <isdesced.h>
#include <isdescst.h>
#include <texmem.h>

#include <winui.h>
#include <medmotns.h>
#include <medmotn.h>
#include <edmedmo.h>
#include <family.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


// global weirdnesses for speed in our recursion
static mxs_vector g_seed_pos;
static uchar g_medium;
static uchar g_motion_index;
static bool (*g_bounding_function)(mxs_vector *sample_point);
static short g_texture_above;
static short g_texture_below;

// Here are the texture assignments and RGBs.
sEdMedMoSurface g_aEdMedMoSurface[MAX_CELL_MOTION];

// Here's how we know where we've already been when we're propagating.
// With this limit we could probably traverse Lake Superior.
#define MAX_CELLS_TO_MARK 10000
static int *marked_cell_list;
static int marked_cell_count;


// return values for CellStatus, below
#define CELL_READY_FOR_ROCKING  0
#define CELL_ALREADY_VISITED    1
#define CELL_WRONG_MEDIUM       2
#define CELL_OUTSIDE_VOLUME     3



/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   We mark every cell we visit, not just the ones of the right medium.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void MarkCell(int cell_index)
{
   if (marked_cell_count == MAX_CELLS_TO_MARK)
      Warning(("edmedmo: tried to mark more than %d cells\n",
               marked_cell_count));
   else {
      marked_cell_list[marked_cell_count++] = cell_index;
      WR_CELL(cell_index)->flags |= CELL_TRAVERSED;
   }
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   We unmark the whole bunch at once, after the main work.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void UnmarkAllCells(void)
{
   int i;

   for (i = 0; i < marked_cell_count; ++i)
      WR_CELL(marked_cell_list[i])->flags &= ~CELL_TRAVERSED;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This tells us whether we want to mark a given cell; and if not, why
   not.  Note that the order in which we perform the tests is
   important; if we reach a cell which is both the wrong medium and
   has been traversed, we want to treat it as the wrong medium so we
   can still set the media of its portals.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static int CellStatus(PortalCell *cell, mxs_vector *middle)
{
   if (ConvertFindFinalMedium(cell->medium) != g_medium)
      return CELL_WRONG_MEDIUM;

   if (!g_bounding_function(middle))
      return CELL_OUTSIDE_VOLUME;

   if (cell->flags & CELL_TRAVERSED)
      return CELL_ALREADY_VISITED;

   return CELL_READY_FOR_ROCKING;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This is the recursive core of our cell marking.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static int SetCells(int cell_index, int prev_cell_index)
{
   int i;
   mxs_vector middle;
   int status;
   PortalCell *cell = WR_CELL(cell_index);
   int num_vertices = cell->num_vertices;
   mxs_vector *vertex = cell->vpool;
   int num_portals = cell->num_portal_polys;
   int num_solid_polys = cell->num_polys - num_portals;
   int num_render_portals = cell->num_render_polys - num_solid_polys;
   PortalPolygonCore *portal = cell->portal_poly_list;
   PortalPolygonRenderInfo *portal_render
      = cell->render_list + num_solid_polys;
   int portals_set = 0;

   // The middle of the cell is the average of the vertices.
   // This is meant for good-nuffness more than perfection.
   mx_zero_vec(&middle);

   for (i = 0; i < num_vertices; i++) {
      mx_addeq_vec(&middle, vertex);
      vertex++;
   }
   mx_scaleeq_vec(&middle, 1.0 / (float) num_vertices);

   status = CellStatus(cell, &middle);
   MarkCell(cell_index);

   if (status == CELL_READY_FOR_ROCKING) {
      cell->motion_index = g_motion_index;

      portals_set = 1;

      for (i = 0; i < num_portals; i++) {
         if (i < num_render_portals) {
            portal_render->texture_id = g_texture_below;
            portal->motion_index = g_motion_index;
         }

         portals_set += SetCells(portal->destination, cell_index);
         portal++;
         portal_render++;
      }
   }

   // If this cell is, say, air next to water, it's still got a portal
   // into this cell, and that surface should move.
   if (status == CELL_WRONG_MEDIUM) {
      portals_set = 1;

      for (i = 0; i < num_portals; i++) {
         if (portal->destination == prev_cell_index) {
            if (i < num_render_portals) {
               portal_render->texture_id = g_texture_above;
               portal->motion_index = g_motion_index;
            }
         }
         portal++;
         portal_render++;
      }
   }

   return portals_set;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Go look for water textures prefix "prefix".  fill in in_hnd,out_hnd
   returns TRUE if they work, else FALSE if one or both wasnt found

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
BOOL GetWaterTexIDs(char *prefix, short *in_hnd, short *out_hnd)
{
   int handles[2]; 
   if (family_find_water(prefix,handles,2))
   {
      *in_hnd = (short)handles[0]; 
      *out_hnd = (short)handles[1]; 
      return TRUE; 
   }  
   return FALSE; 
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This is called for each volume we're dealing with (in practic, each
   flow brush).

   It sets the medium for all cells which
    * are flagged by our bounding function;
    * can be reached from our seed point; and
    * have the given medium, or one will turn into it (this lets us
      flood temporary media like door brushes).

   We return the number of cells marked.  This is 0 if the coordinates
   do not map to a cell, or the one they map to is outside the
   bounding box or has the wrong medium.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
int EdMedMoSetCells(mxs_vector *seed_pos,
                    uchar medium, uchar motion_index,
                    bool (*bounding_function)(mxs_vector *sample_point))
{
   int seed_status;
   int cell_index;
   int portals_set;
   PortalCell *cell;
   Location loc;

   MakeLocationFromVector(&loc, seed_pos);
   cell_index = CellFromLoc(&loc);
   if (cell_index == CELL_INVALID) {
      ConfigSpew("FlowSpew",
                 ("EdMedMoSetCells: seed not in WR at (%g %g %g)\n",
                  seed_pos->x, seed_pos->y, seed_pos->z));
      return 0;
   }

   g_seed_pos = *seed_pos;
   g_medium = medium;
   g_motion_index = motion_index;
   g_bounding_function = bounding_function;

   // defaults, just in case...
   g_texture_above = WATERIN_IDX;
   g_texture_below = WATEROUT_IDX;

   if (g_aEdMedMoSurface[motion_index].texture_name_base[0] != 0) {
      if (!GetWaterTexIDs(g_aEdMedMoSurface[motion_index].texture_name_base,
                          &g_texture_above, &g_texture_below)) {
      ConfigSpew("FlowSpew",
                 ("Cannot find water textures from %s for flow group %d\n",
                  g_aEdMedMoSurface[motion_index].texture_name_base,
                  motion_index));
      }
   }

   // We want our seed to be inside the world, with a different motion
   // index from ours, and with the given medium.  We treat the seed
   // cell differently from the ones we spread out to in that it's
   // compared using the seed point rather than the average point of
   // its vertices.
   cell = WR_CELL(cell_index);
   seed_status = CellStatus(cell, seed_pos);

   if (seed_status == CELL_READY_FOR_ROCKING) {
      marked_cell_list = Malloc(sizeof(int) * MAX_CELLS_TO_MARK);
      marked_cell_count = 0;

      portals_set = SetCells(cell_index, CELL_INVALID);

      UnmarkAllCells();
      Free(marked_cell_list);
   } else {
      portals_set = 0;
      switch (seed_status) {
         case CELL_WRONG_MEDIUM:
            ConfigSpew("FlowSpew",
                       ("EdMedMoSetCells: seed cell is wrong medium: %d\n",
                        cell->medium));
            break;

         case CELL_ALREADY_VISITED:
            ConfigSpew("FlowSpew",
                       ("EdMedMoSetCells: seed cell already marked?\n"));
            break;

         case CELL_OUTSIDE_VOLUME:
            ConfigSpew("FlowSpew",
                       ("EdMedMoSetCells: seed not in brush at (%g %g %g)\n",
                        seed_pos->x, seed_pos->y, seed_pos->z));
            break;
      }
   }

   return portals_set;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   BRUTE FORCE!  BRUTE!  FORCE!

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void EdMedMoClearMediumMotionInPortals(void)
{
   int i, p;
   int num_portals;
   PortalPolygonCore *portal;
   PortalCell *cell;

   for (i = 0; i < wr_num_cells; i++) {
      cell = WR_CELL(i);

      portal = cell->portal_poly_list;
      num_portals = cell->num_portal_polys;

      for (p = 1; p < num_portals; p++) {
         portal->motion_index = 0;
         portal++;
      }
   }
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Nuke them from orbit.  It's the only way.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void EdMedMoClearMediumMotion(void)
{
   int i;

   memset(&g_aMedMoCellMotion[0], 0,
          sizeof(g_aMedMoCellMotion[0]) * MAX_CELL_MOTION);

   memset(&portal_cell_motion[0], 0,
          sizeof(portal_cell_motion[0]) * MAX_CELL_MOTION);

   memset(&g_aEdMedMoSurface[0], 0,
          sizeof(g_aEdMedMoSurface[0]) * MAX_CELL_MOTION);

   for (i = 0; i < MAX_CELL_MOTION; ++i) {
      // By default, we expect all motion to be horizontal.
      portal_cell_motion[i].major_axis = MEDIUM_AXIS_Z;
      portal_cell_motion[i].in_motion = TRUE;
      g_aEdMedMoSurface[i].texture_above = WATERIN_IDX;
      g_aEdMedMoSurface[i].texture_below = WATEROUT_IDX;
   }
}


// internal SDESC struct
typedef struct sMotionEdit {
   int index;
   float x, y, z, theta;
   float dx, dy, dz, dtheta;
   uint major_axis;
   char texture_name_base[kMedmoSurfaceFilenameLength];
   BOOL in_motion;
} sMotionEdit;

static sFieldDesc gMotionEdit[] = {
   { "anchor x", kFieldTypeFloat, FieldLocation(sMotionEdit, x) },
   { "anchor y", kFieldTypeFloat, FieldLocation(sMotionEdit, y) },
   { "anchor z", kFieldTypeFloat, FieldLocation(sMotionEdit, z) },
   { "angle (0-360)", kFieldTypeFloat, FieldLocation(sMotionEdit, theta) },

   { "x change/sec.", kFieldTypeFloat, FieldLocation(sMotionEdit, dx) },
   { "y change/sec.", kFieldTypeFloat, FieldLocation(sMotionEdit, dy) },
   { "z change/sec.", kFieldTypeFloat, FieldLocation(sMotionEdit, dz) },
   { "angle change/sec.", kFieldTypeFloat, FieldLocation(sMotionEdit, dtheta)},

   { "axis: 0=x 1=y 2=z", kFieldTypeInt, 
     FieldLocation(sMotionEdit, major_axis)},

   { "texture name", kFieldTypeString, 
     FieldLocation(sMotionEdit, texture_name_base) },


   { "used?", kFieldTypeBool, FieldLocation(sMotionEdit, in_motion)}
};


// desc job
static sStructDesc Desc_Desc
  = StructDescBuild(sMotionEdit, kStructFlagNone, gMotionEdit);

// the structure we edit
static sMotionEdit popup_sMotionEdit;

// temporary storage so we can cancel editing within SDESC
static sMotionEdit temp_sMotionEdit;


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This sets up our sMotionEdit for editing.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void SetSDESCFromMotionStructures(int index, sMotionEdit *data)
{
   data->index = index;

   data->x = portal_cell_motion[index].center.x;
   data->y = portal_cell_motion[index].center.y;
   data->z = portal_cell_motion[index].center.z;
   data->theta
      = (((float) portal_cell_motion[index].angle * 360.0) / 65536.0);

   data->dx = g_aMedMoCellMotion[index].center_change.x;
   data->dy = g_aMedMoCellMotion[index].center_change.y;
   data->dz = g_aMedMoCellMotion[index].center_change.z;
   data->dtheta
      = (((float) g_aMedMoCellMotion[index].angle_change * 360.0) / 65536.0);

   data->major_axis = (uint) portal_cell_motion[index].major_axis;

   strncpy(data->texture_name_base, 
           g_aEdMedMoSurface[index].texture_name_base,
           kMedmoSurfaceFilenameLength);


   data->in_motion = (BOOL) portal_cell_motion[index].in_motion;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This applies the result of the user's changes to the actual data
   structures.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void SetMotionStructuresFromSDESC(sMotionEdit *data)
{
   int index = data->index;

   portal_cell_motion[index].center.x = data->x;
   portal_cell_motion[index].center.y = data->y;
   portal_cell_motion[index].center.z = data->z;
   portal_cell_motion[index].angle
      = (mxs_ang)((data->theta * 65536.0) / 360.0);

   g_aMedMoCellMotion[index].center_change.x = data->dx;
   g_aMedMoCellMotion[index].center_change.y = data->dy;
   g_aMedMoCellMotion[index].center_change.z = data->dz;
   g_aMedMoCellMotion[index].angle_change
      = (mxs_ang)((data->dtheta * 65536.0) / 360.0);

   data->texture_name_base[kMedmoSurfaceFilenameLength-1]='\0';
   strncpy(g_aEdMedMoSurface[index].texture_name_base,data->texture_name_base,
           kMedmoSurfaceFilenameLength);

   if (!GetWaterTexIDs(data->texture_name_base,
                       &g_aEdMedMoSurface[index].texture_above,
                       &g_aEdMedMoSurface[index].texture_below))
   {
      Warning(("No such water texture as %s\n",data->texture_name_base)); 
   }


   if (data->major_axis > 2)
      portal_cell_motion[index].major_axis = 2;
   else
      portal_cell_motion[index].major_axis = data->major_axis;

   portal_cell_motion[index].in_motion = data->in_motion;
}


// This is from gedmedmo.c.
extern void GEdMedMoMarkWaterAllBrushes(void);

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This is called from the wide world of SDESC.  No matter the event
   we update the textures in the world.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void LGAPI SDESCCallback(sStructEditEvent *event, StructEditCBData data)
{
   sMotionEdit *result = (sMotionEdit *) data;

   switch (event->kind) {
      case kStructEdOK:
         {
            SetMotionStructuresFromSDESC(result);
            break;
         }
      case kStructEdCancel:
         {
            *result = temp_sMotionEdit;
            SetMotionStructuresFromSDESC(result);
            break;
         }
      case kStructEdApply:
         {
            SetMotionStructuresFromSDESC(result);
            break;
         }
   }
   GEdMedMoMarkWaterAllBrushes();
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Our SDESC pop-up lets you change the anchor and delta of any medium
   motion index.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
BOOL EdMedMoSDESC(int motion_index)
{
   IStructEditor *sed;
   sStructEditorDesc editdesc;

   sprintf(editdesc.title, "flow group %d", motion_index);
   editdesc.flags = kStructEditNoApplyButton;

   SetSDESCFromMotionStructures(motion_index, &popup_sMotionEdit);
   SetSDESCFromMotionStructures(motion_index, &temp_sMotionEdit);

   sed = CreateStructEditor(&editdesc, &Desc_Desc, &popup_sMotionEdit);
   if (sed) 
   {
      IStructEditor_SetCallback(sed, SDESCCallback, &popup_sMotionEdit);
      IStructEditor_Go(sed,kStructEdModeless);
      SafeRelease(sed);
      return TRUE;
   } else
      return FALSE;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Admittedly, this is a lot of fuss for traversing an array.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void EdMedMoSurfaceIterInit(tEdMedMoSurfaceIter *i)
{
   *i = 0;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This iterator finds those sEdMedMoSurfaces which have nonempty
   texture_name_base fields.  NULL means there's no more.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
sEdMedMoSurface *EdMedMoSurfaceIterNext(tEdMedMoSurfaceIter *i)
{
   while (*i < MAX_CELL_MOTION) {
      if (g_aEdMedMoSurface[*i].texture_name_base[0]) {
         *i += 1;
         return &g_aEdMedMoSurface[*i];
      }
      *i += 1;
   }

   return NULL;
}
