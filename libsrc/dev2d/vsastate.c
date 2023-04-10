/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/vsastate.c $
 * $Revision: 1.2 $
 * $Author: TOML $
 * $Date: 1996/10/16 16:06:53 $
 *
 * Routine for getting the current video state
 */

#ifndef _WIN32

#include <conio.h>
#include <string.h>

#include <dpmidat.h>
#include <grd.h>
#include <mode.h>
#include <state.h>
#include <vesa.h>
#include <vga.h>
#include <vgareg.h>

//
// SaveVESATextState:
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

int SaveVESATextState(VideoState *curState)
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

} // SaveVESATextState

//
// DetermineVesaLevel:
//
// Short routine to determine if we have a good enough vesa to use
// vesa calls to set things.  Returns -1 if no vesa, 0 if version < 1.2,
// 1 if version 1.2 and 2 if better (2.0 or bigger).
//

int DetermineVesaLevel()
{
   dpmis_block dpmiBlock;
   int buffSize;
   int vesaLevel;

// First we need to determine if vesa is there and if so,
// get its version number.

   buffSize = (sizeof(VBE_infoBlock) + 15) >> 4;
   if (dpmi_alloc_dos_mem(&dpmiBlock,buffSize) != 0) return -1;

   dpmi_reg_data.eax = VBE_DETECT;
   dpmi_reg_data.edx = 0;
   dpmi_reg_data.es  = ((ulong)dpmiBlock.p) >> 4;

// Now make the call to find out if anyone there

   if (
         dpmi_real_interrupt(0x10,&dpmi_reg_data) != 0 ||
         (dpmi_reg_data.eax & 0xffff) != VBE_RET_OK    ||
         ((VBE_infoBlock *)dpmiBlock.p)->VESASignature != 'ASEV'
      )
   {
      vesaLevel = -1;
   }
   else
   {
      VBE_version = ((VBE_infoBlock *)dpmiBlock.p)->VESAVersion;
      if (((VBE_infoBlock *)dpmiBlock.p)->VESAVersion < 0x101)
         vesaLevel = 0;
      else if (((VBE_infoBlock *)dpmiBlock.p)->VESAVersion >= 0x200)
         vesaLevel = 2;
      else
         vesaLevel = 1;
   } // have no VESA vbe of appropriate type

   dpmi_free_dos_mem(&dpmiBlock);
   return vesaLevel;

} // DetermineVesaLevel

//
// SaveGenGraphicsState:
//
// Determine if the currently set graphics state is one of ourse
// i.e. grd_mode is set, or not.  Save the relevant data so that
// we can restore the state later.
//
// What are the possibilities?
// 1) 2d mode: not vesa
// 2) 2d mode: is vesa
// 3) standard vga mode
// 4) random vesa mode
//
// Now if have vesa of 1.1 or greater can check for graphInfo.dacWidth (not really unless 1.2)
// and also set the bytesPerScanLine, and can get the focus.  If 2.0 or greater
// then can also use the vesa set palette function call.  So we need to
// save the vesa version number along with all the data.  Ok, so if it
// is a 2d mode we set some bit and make the 2d set mode call otherwise
// we make the vbe or standard bios call.  Then do other stuff as needed.
//

int SaveGenGraphicsState(VideoState *curState)
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

// Now get the vesa level (0-2)
   curState->graphInfo.vesaLevel = DetermineVesaLevel();

// Set our mode number depending on the various flags

   if (grd_mode != -1)
   {
      curState->mode = grd_mode;
      if (curState->graphInfo.vesaLevel >= 2)
      {
         dpmi_reg_data.eax = VBE_GET_MODE;
         dpmi_real_interrupt(0x10,&dpmi_reg_data);
         curState->mode |= (dpmi_reg_data.ebx & VBE_LINEAR_BUFFER);
      } // have possibility of linear frame buffer
   } // have 2d mode, so store value
   else if (curState->graphInfo.vesaLevel < 0)
   {
      dpmi_reg_data.eax = 0x0f00;
      dpmi_real_interrupt(0x10,&dpmi_reg_data);
      curState->mode = (dpmi_reg_data.eax & 0x00ff);
   } // have only vga mode
   else
   {
      dpmi_reg_data.eax = VBE_GET_MODE;
      dpmi_real_interrupt(0x10,&dpmi_reg_data);

   // Ignore the DONT_CLEAR_BIT which may be returned, we will always
   // clear when resetting the mode.  But keep around the linear frame
   // buffer bit.

      curState->mode =
         (dpmi_reg_data.ebx & 0xffff) & ~VBE_DONT_CLEAR;

   } // can use vesa to get mode

// Next get the palette. We use the bios if vesaLevel < 2, otherwise vbe.

   if (curState->graphInfo.vesaLevel < 2)
   {
      vga_get_pal(0,256,&curState->pal[0].red);
   } // use the 2d get palette routine
   else
   {
      vbe_get_palette(0,256,&curState->pal[0].red);
   } // use the vbe to save the palette

// Next look for the pixelsPerScanLine and graphInfo.dacWidth, setting to 0
// if don't have level 1 or better.  We don't set the focus since
// it seems that it does not make much sense as we keep track of
// the current focus and current visible canvas in the 2d.  So if we
// were to restore to a state which had a non-trivial focus, there
// would not yet be set the screen structure and so yuck...

   if (curState->graphInfo.vesaLevel > 0)
   {
      curState->graphInfo.pixelsPerScanLine = vbe_get_width();
      curState->graphInfo.dacWidth = VBEGetPaletteDac();

//    VBEGetFocus(&curState->graphInfo.xFocus,&curState->graphInfo.yFocus);
      curState->graphInfo.xFocus = 0;
      curState->graphInfo.yFocus = 0;
   } // have scan line and dac width calls
   else
   {
      curState->graphInfo.pixelsPerScanLine = 0;    // set to something bad
      curState->graphInfo.dacWidth = DEFAULT_DAC_WIDTH;
      curState->graphInfo.xFocus = 0;
      curState->graphInfo.yFocus = 0;
   } // have old vesa or only vga

// Indicate that we were successful in getting the information

   return 1;

} // SaveGenGraphicsState

//
// SaveGenState:
//
// Small control routine to dispatch to one of the text or
// graphics state savers.  Just reads one of the standard
// VGA registers to determine the type.  Return 1 (TRUE)
// if all ok, 0 (FALSE) if something went wrong.
//

int save_gen_state(VideoState *curState)
{
// The very first thing to do is determine if current state
// is graphics or text state.  This we do by looking at a
// standard VGA register, which should always be set correctly.

   outp(GRX_ADR,GR_MISC);

   if (inp(GRX_DATA) & GR6_MODE)
   {
      return SaveGenGraphicsState(curState);
   } // have graphics state
   else
   {
      return SaveVESATextState(curState);
   } // have text state

} // SaveGenState

//
// RestoreVESATextState:
//
// From the passed structure reset the text state including
// all the palette registers and the palette itself.  Also
// load in the 8x8 font if we were in a 50 row mode.
// Return 1 if all ok, 0 if something went wrong.
//

int RestoreVESATextState(VideoState *newState)
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

} // RestoreVESATextState

//
// RestoreGenGraphicsState:
//
// Use the newState data to reload a graphics state, and if
// one of ours call the mode setter to set the 2d internal
// variables.  1 (TRUE) means all ok, 0 (FALSE) means something
// went wrong.
//

int RestoreGenGraphicsState(VideoState *newState)
{
   if (newState->graphInfo.is2dMode)
   {
      if (newState->mode & VBE_LINEAR_BUFFER)
      {
         gr_set_mode(newState->mode & 0x00ff,3);
      } // set mode with linear buffer
      else
      {
         gr_set_mode(newState->mode & 0x00ff,1);
      } // set mode normally
   } // have 2d mode
   else
   {
      if (newState->graphInfo.vesaLevel < 0)
      {
         dpmi_reg_data.eax = newState->mode & 0x00ff;
         dpmi_real_interrupt(0x10,&dpmi_reg_data);
      } // set mode using vga bios
      else
      {
         dpmi_reg_data.eax = VBE_SET_MODE;
         dpmi_reg_data.ebx = newState->mode & 0xffff;
         if (
               dpmi_real_interrupt(0x10,&dpmi_reg_data) != 0 ||
               (dpmi_reg_data.eax & 0xffff) != VBE_RET_OK
            )
         {
            return 0;
         } // have an error, return
      } // set mode using vesa vbe
   } // not a 2d mode

// Try to set the dac width and scan line length if possible
   if (newState->graphInfo.vesaLevel > 0)
   {
      if (
            vbe_set_width(newState->graphInfo.pixelsPerScanLine)
               < newState->graphInfo.pixelsPerScanLine
         )
      {
         return 0;
      } // if is less then we failed

      if (newState->graphInfo.dacWidth > DEFAULT_DAC_WIDTH)
      {
         if (VBESetPaletteDac(newState->graphInfo.dacWidth) < newState->graphInfo.dacWidth)
         {
            return 0;
         } // if less then we failed
      } // need to set the dac width

   // We currently ignore completely the Focus since it isn't clear
   // what should happen if there is not yet a screen defined

   // VBESetFocus(newState->graphInfo.xFocus,newState->graphInfo.yFocus);

   } // can try to set stuff

// Finally set the palette

   if (newState->graphInfo.vesaLevel < 2)
   {
      vga_set_pal(0,256,&newState->pal[0].red);
   } // use our old palette setter
   else
   {
      vbe_set_palette(0,256,&newState->pal[0].red);
   } // use the new palette setter

// If possible and should, set the dac width

   return 1;

} // RestoreGenGraphicsState

//
// RestoreGenState:
//
// Small routine to switch between the text and graphics
// state restorers.  Just looks at the type field of the
// VideoState structure passed in to determine which to call.
// Return 1 (TRUE) if all ok, 0 (FALSE) if something went wrong.
//

int restore_gen_state(VideoState *newState)
{
   if (newState->type == GRD_STATE_TEXT)
   {
      return RestoreVESATextState(newState);
   } // have a text mode
   else if (newState->type == GRD_STATE_GRAPHICS)
   {
      return RestoreGenGraphicsState(newState);
   } // have a graphics mode
   else
   {
      return 0;
   } // there is a problem!
} // RestoreGenState

#endif
