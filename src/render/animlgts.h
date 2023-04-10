/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/animlgts.h,v 1.8 2000/01/29 13:38:27 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   animlgts.h

   structures for animated lightmaps

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _ANIMLGTS_H_
#define _ANIMLGTS_H_

#include <wr.h>

#include <animlgte.h>   // enums put in own file for scripting 

// An animated light knows its magnitude and which cells it reaches.
// For each cell, it also knows its position in the cell's light
// palette.  These are stored in the 
typedef struct sAnimLight
{
   BOOL refresh;

   // connection to world rep
   ushort first_light_to_cell;  // index into array of cells/palette indexes
   ushort num_cells_reached;
   short light_data_index;     // index into light_data array in objlight

   // control of fluctuation
   short mode;
   long time_rising_ms; // how many milliseconds to reach max intensity?
   long time_falling_ms;// milliseconds to reach min intensity?
   float min_brightness;
   float max_brightness;

   // current state
   float brightness;
   BOOL is_rising;
   long countdown_ms;   // timer
   BOOL inactive;       // if off, don't update countdown_ms or intensity
} sAnimLight;


// For each animated light, we need to know all the cells it reaches
// and its position in each cell's list of animated lights.  This data
// is all stored in gpAnimLightToCell.  Each sPortalAnimLight has an
// index into this array.
typedef struct sAnimLightToCell {
   ushort cell_index;
   uchar pos_in_cell_palette;
   uchar cheezefood;            // artificial filler
} sAnimLightToCell;


#endif // ~_ANIMLGTS_H_

