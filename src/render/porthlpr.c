/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   porthlpr.c

   Both temporary and permanent commands for connecting Portal to
   the editor are stored here, along with a few accessory functions.
   Temporary hacks should be commented as such.  If there aren't at
   least a few of those, we should add some.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

#include <matrixs.h>
#include <mprintf.h>

#include <status.h>
#include <portal.h>
#include <porthlpr.h>
#include <viewmgr.h>

// why do we include all this header stuff in here
#include <medmotns.h>
#include <medmotn.h>
#include <fxdisk.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// do we need this around? 
// the motion we're currently changing
int medium_motion_index = 0;

// Mat! what is going on?

#ifdef EDITOR

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Call this with -1 to clear out all them wireframes.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void show_one_cell(int cell_id)
{
   int i;

   // hey, lets fault if given max cell - those were > not >=
   if ((cell_id < -1) || (cell_id >= wr_num_cells))
   {
      mprintf("from -1 to %d, please\n", wr_num_cells - 1);
      Status("Invalid cell id");
      return;
   }

   for (i = 0; i < wr_num_cells; ++i)
      WR_CELL(i)->flags &= ~CELL_RENDER_WIREFRAME;

   if (cell_id != -1)
      WR_CELL(cell_id)->flags |= CELL_RENDER_WIREFRAME;
}

void teleport_to_cell(int cell_id)
{
   // hey, lets fault if given max cell - those were > not >=
   if ((cell_id < -1) || (cell_id >= wr_num_cells)) {
      mprintf("from -1 to %d, please\n", wr_num_cells - 1);
      Status("Invalid cell id");
      return;
   }
   vm_cur_loc_set(&WR_CELL(cell_id)->sphere_center);
}

#endif
