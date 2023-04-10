/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/vgastate.c $
 * $Revision: 1.2 $
 * $Author: TOML $
 * $Date: 1996/10/16 16:06:52 $
 *
 * Routine for getting the current video state
 */

#ifndef _WIN32

#include <conio.h>
#include <string.h>
#include <dpmidat.h>
#include <vgareg.h>
#include <state.h>
#include <mode.h>
#include <grd.h>
#include <vga.h>

//
// SaveVGATextState:
//
// This routine fills in the GraphicsState structure passed
// to it.  First it determines what the mode is by determining
// if it is a text or graphics mode.
//
// If it is a text mode:
// 1) Get the mode by calling bios function 0x0F
// 2) Determine if the font is doubled by calling function
//    0x11 subfunction 0x30 and looking at dl which will be
//    0x31 = 49 = 50 - 1 for 50 line doubled modes.
// 3) Get the palette by calling function 0x10 subfunction 0x17
//    and store it in a 768 byte structure.
// 4) Get the current color page state from 0x10 subfunction 0x1a
// 5) Get the 17 palette registers from 0x10 subfunction 0x09
//
// Return 1 on success or 0 on failure (TRUE/FALSE).
//

int SaveVGATextState(VideoState *curState)
{
   dpmis_block dpmiBlock;

// Indicate our state type
   curState->type = GRD_STATE_TEXT;

// Now get the mode by calling the vga bios
   dpmi_reg_data.eax = 0x0f00;
   dpmi_real_interrupt(0x10,&dpmi_reg_data);
   curState->mode = (dpmi_reg_data.eax & 0xff);

// Clear out the mode flags so we can fill them in
   curState->textInfo.modeFlags = 0;

// Determine if there are 50 text lines on screen, indicating
// an 8x8 font which has to be specially loaded.  0x31 = 50 - 1
   dpmi_reg_data.eax = 0x1130;
   dpmi_real_interrupt(0x10,&dpmi_reg_data);
   if ((dpmi_reg_data.edx & 0xff) == 0x31)
   {
      curState->textInfo.modeFlags |= TEXT_FONT_IS_DOUBLED;
   } // have 50 lines

// Find out how many color sets there are and which one is in use.
// We happen to be able to shift the top byte down by the same
// amount we would need to shift it up or down to get value.

   dpmi_reg_data.eax = 0x101A;
   dpmi_real_interrupt(0x10,&dpmi_reg_data);
   if (dpmi_reg_data.ebx & 0xff)
   {
      curState->textInfo.modeFlags |= TEXT_FONT_HAVE_16_SETS;
   } // have 16 color sets
   curState->textInfo.modeFlags |=
      (dpmi_reg_data.ebx & 0xff00) >> TEXT_FONT_ACTIVE_SHIFT;

// Next get the palette, so need to allocate space in low
// mem to read into, then call read function, then save it
// in our space.

   if (dpmi_alloc_dos_mem(&dpmiBlock,sizeof(curState->pal) >> 4) != 0) return 0;
   dpmi_reg_data.eax = 0x1017;   // function to read palette
   dpmi_reg_data.ebx = 0;        // start at register 0
   dpmi_reg_data.ecx = 256;      // read all 256 of them
   dpmi_reg_data.es  = ((ulong)dpmiBlock.p >> 4);  // segment
   dpmi_reg_data.edx = 0;        // and offset of place to read to

   dpmi_real_interrupt(0x10,&dpmi_reg_data);       // bios makes copy
   memcpy(&curState->pal[0],dpmiBlock.p,sizeof(curState->pal));

// And now get the 17 palette registers
   dpmi_reg_data.eax = 0x1009;   // function to read palette registers
   dpmi_reg_data.es  = ((ulong)dpmiBlock.p >> 4);  // segment
   dpmi_reg_data.edx = 0;        // and offset of place to read to

   dpmi_real_interrupt(0x10,&dpmi_reg_data);       // get the 17 bytes
   memcpy(&curState->textInfo.palRegs[0],dpmiBlock.p,17);   // copy into our buffer

// Finally free up the dos memory we hogged before
   dpmi_free_dos_mem(&dpmiBlock);

   return 1;

} // SaveVGATextState

//
// SaveVGAGraphicsState:
//
// Determine if the currently set graphics state is one of ourse
// i.e. grd_mode is set, or not.  Save the relevant data so that
// we can restore the state later.
//
// What are the possibilities?
// 1) 2d mode: not vesa
// 2) standard vga mode
//

int SaveVGAGraphicsState(VideoState *curState)
{
   curState->type = GRD_STATE_GRAPHICS;

   if (grd_mode != -1)
   {
      curState->graphInfo.is2dMode = MODE_IS_2D;
   } // have set some 2d graphics mode
   else
   {
      curState->graphInfo.is2dMode = MODE_IS_NOT_2D;
   } // no 2d graphics mode set

// Now set the vesa level to -1 since is vga
   curState->graphInfo.vesaLevel = -1;

// Set our mode number depending on the various flags

   if (grd_mode != -1)
   {
      curState->mode = grd_mode;
   } // have 2d mode, so store value
   else
   {
      dpmi_reg_data.eax = 0x0f00;
      dpmi_real_interrupt(0x10,&dpmi_reg_data);
      curState->mode = (dpmi_reg_data.eax & 0x00ff);
   } // have only vga mode

// Next get the palette. We use the bios registers directly.

   vga_get_pal(0,256,&curState->pal[0].red);

// Since only vga we cannot set the various other graphics variables,
// so set them all to the default.

   curState->graphInfo.pixelsPerScanLine = 0;      // set to something bad
   curState->graphInfo.dacWidth = 6;               // default dac width
   curState->graphInfo.xFocus = 0;
   curState->graphInfo.yFocus = 0;

// Indicate that we were successful in getting the information

   return 1;

} // SaveVGAGraphicsState

//
// vga_save_state:
//
// Small control routine to dispatch to one of the text or
// graphics state savers.  Just reads one of the standard
// VGA registers to determine the type.  Return 1 (TRUE)
// if all ok, 0 (FALSE) if something went wrong.
//

int vga_save_state(VideoState *curState)
{
// The very first thing to do is determine if current state
// is graphics or text state.  This we do by looking at a
// standard VGA register, which should always be set correctly.

   outp(GRX_ADR,GR_MISC);

   if (inp(GRX_DATA) & GR6_MODE)
   {
      return SaveVGAGraphicsState(curState);
   } // have graphics state

   return SaveVGATextState(curState);

} // vga_save_state

//
// RestoreVGATextState:
//
// From the passed structure reset the text state including
// all the palette registers and the palette itself.  Also
// load in the 8x8 font if we were in a 50 row mode.
// Return 1 if all ok, 0 if something went wrong.
//

int RestoreVGATextState(VideoState *newState)
{
   dpmis_block dpmiBlock;

// First try to allocate the low memory we will need for transferring
// things around.  We send the number of paragraphs to reserve.
   if (dpmi_alloc_dos_mem(&dpmiBlock,sizeof(newState->pal) >> 4) != 0) return 0;

// Next set the mode using the bios call (AH=0 for mode set, AL=mode)
   dpmi_reg_data.eax = newState->mode & 0xff;
   dpmi_real_interrupt(0x10,&dpmi_reg_data);

// Next check if we are supposed to change the font size
   if (newState->textInfo.modeFlags & TEXT_FONT_IS_DOUBLED)
   {
      dpmi_reg_data.eax = 0x1112;
      dpmi_reg_data.ebx = 0x0;
      dpmi_real_interrupt(0x10,&dpmi_reg_data);
   } // load 8x8 font

// Next load in the palette itself, so use the low mem buffer we have
   memcpy(dpmiBlock.p,&newState->pal[0],sizeof(newState->pal));
   dpmi_reg_data.eax = 0x1012;         // function to write palette
   dpmi_reg_data.ebx = 0;              // start at register 0
   dpmi_reg_data.ecx = 256;            // write all 256 registers
   dpmi_reg_data.es  = ((ulong)dpmiBlock.p >> 4);  // segment
   dpmi_reg_data.edx = 0;              // offset in segment

   dpmi_real_interrupt(0x10,&dpmi_reg_data);       // actually do the write

// Next set the number of colors sets and the active set
   if (newState->textInfo.modeFlags & TEXT_FONT_HAVE_16_SETS)
   {
      dpmi_reg_data.eax = 0x1013;      // function to set color sets
      dpmi_reg_data.ebx = 0x0100;      // BH=1 16 sets, BL=0 set num sets
      dpmi_real_interrupt(0x10,&dpmi_reg_data);    // set num color sets
   } // set to 16 sets
   else
   {
      dpmi_reg_data.eax = 0x1013;      // function to set color sets
      dpmi_reg_data.ebx = 0x0000;      // BH=0 4 sets, BL=0 set num sets
      dpmi_real_interrupt(0x10,&dpmi_reg_data);    // set num color sets
   } // set to 4 sets

// Next set the active color set (0-3) or (0-15)
   dpmi_reg_data.eax = 0x1013;         // function for setting color sets
   dpmi_reg_data.ebx =
      ((newState->textInfo.modeFlags & TEXT_FONT_ACTIVE_SET) << TEXT_FONT_ACTIVE_SHIFT)
      || 0x01;                         // BH=active subset, BL=1 select set

// Now write the 17 palette registers
   memcpy(dpmiBlock.p,&newState->textInfo.palRegs[0],17);   // copy 17 palette regs
   dpmi_reg_data.eax = 0x1002;                     // function to write palette regs
   dpmi_reg_data.es  = ((ulong)dpmiBlock.p >> 4);  // segment of data
   dpmi_reg_data.edx = 0;                          // offset to data
   dpmi_real_interrupt(0x10,&dpmi_reg_data);       // make the bios call

// Finally free up the dos memory we hogged before
   dpmi_free_dos_mem(&dpmiBlock);

   return 1;

} // RestoreVGATextState

//
// RestoreVGAGraphicsState:
//
// Use the newState data to reload a graphics state, and if
// one of ours call the mode setter to set the 2d internal
// variables.  1 (TRUE) means all ok, 0 (FALSE) means something
// went wrong.
//

int RestoreVGAGraphicsState(VideoState *newState)
{
   if (newState->graphInfo.is2dMode)
   {
      gr_set_mode(newState->mode & 0x00ff,1);
   } // have 2d mode
   else
   {
      dpmi_reg_data.eax = newState->mode & 0x00ff;
      dpmi_real_interrupt(0x10,&dpmi_reg_data);
   } // not a 2d mode

// Finally set the palette

   vga_set_pal(0,256,&newState->pal[0].red);

// If possible and should, set the dac width

   return 1;

} // RestoreVGAGraphicsState

//
// vga_restore_state:
//
// Small routine to switch between the text and graphics
// state restorers.  Just looks at the type field of the
// VideoState structure passed in to determine which to call.
// Return 1 (TRUE) if all ok, 0 (FALSE) if something went wrong.
//

int vga_restore_state(VideoState *newState)
{
   if (newState->type == GRD_STATE_TEXT)
   {
      return RestoreVGATextState(newState);
   } // have a text mode
   else if (newState->type == GRD_STATE_GRAPHICS)
   {
      return RestoreVGAGraphicsState(newState);
   } // have a graphics mode

   return 0; // there is a problem!

} // vga_restore_state

#endif

