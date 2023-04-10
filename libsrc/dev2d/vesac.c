/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/vesac.c $
 * $Revision: 1.3 $
 * $Author: TOML $
 * $Date: 1996/10/16 16:06:55 $
 *
 * Public vesa vbe routines
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef _WIN32

#include <stdlib.h>
#include <string.h>
#include <memall.h>
#include <dpmidat.h>
#include <dbg.h>

#include <grd.h>
#include <vesa.h>
#include <valloc.h>
#include <vga.h>
#include <vgareg.h>
#include <mode.h>
#include <idevice.h>
#include <devtab.h>
#include <bitmap.h>

//
// NOTE:
//
// These routines are mostly interfaces into the VESA VBE, but
// there is also some 2d level code mixed in.  So this module
// is not a generic vbe interface!
//

//
// First some data to keep locally
//

short VBE_version = -1;                      // need to know the version number
uchar bankShift;                             // power of 2 to multiply bank by
ushort gsSel;                                // selector for the video memory
int bytesPerScanLine;                        // the number of bytes in the scan line
int curDacDepth = 6;                         // this is the default depth for the dac
bool useBlankInPalSet;                       // whether to use the blanking bit when setting pal
bool dacTakes8Bits;                          // determines whether we can use 8 bit palette entries
bool lineLengthFuncGood = TRUE;              // assume that we can use the line length function
bool VBE_modeLinear;                         // bool for whether the current linear mode is linear

void *setBankFuncPtr = NULL;                 // points to function we get from vbe
void *setDispStartFuncPtr = NULL;            // points to function we get from vbe
void *setPaletteFuncPtr = NULL;              // points to function we get from vbe

static uchar *videoLinearBase = NULL;        // the base of the extended linear region
static VBE_pmInfo* pmInfoPtr;                // pointer to the protected mode info routines/tables

extern void SetBankProtA(int bank);          // protected mode version, window A only
extern void SetBankRealA(int bank);          // real mode version, window A only
extern void SetBankProtAB(int bank);         // protected mode version, window A and B
extern void SetBankRealAB(int bank);         // real mode version, window A and B

#define SIZE_OF_LINEAR_BUFF   0x800000       // make it 8Mb large

void CallDebugger();
#pragma aux CallDebugger = "int 3";

//
// VBEReadFunc:
//
// This is a VBE 2.0 routine for allocating and copying the code
// for the specified vbe function.  This gives a protected mode
// version of the routine which can be called directly, making it
// much faster than having to go through a real mode interrupt.
//

void *VBEReadFunc(int funcNum)
{
   if (!pmInfoPtr)
   {
      dpmi_reg_data.eax = VBE_GET_FUNCS;
      dpmi_reg_data.ebx = VBE_GET_PMINFO;
      if (
            dpmi_real_interrupt(0x10,&dpmi_reg_data) != 0 ||
            (dpmi_reg_data.eax & 0xffff) != VBE_RET_OK
         )
      {
         return NULL;
      } // something bad happened
      else
      {
         int pmInfoSize = (dpmi_reg_data.ecx & 0xffff);
         pmInfoPtr = (VBE_pmInfo *)Malloc(pmInfoSize);
         if (pmInfoPtr)
         {
            uchar *srcPtr =
               (uchar *)(
                  (dpmi_reg_data.es << 4) +
                  (dpmi_reg_data.edi & 0xffff)
               );
            memcpy(pmInfoPtr,srcPtr,pmInfoSize);
         } // successfully allocated
         else
         {
            return NULL;
         } // allocation failed
      } // try to allocate and copy
   } // need to get and store the info

   switch(funcNum)
   {
      case VBE_GET_SETBANK:
      {
         return ((uchar *)pmInfoPtr + pmInfoPtr->setWindow);
      } // set bank routine
      case VBE_GET_SETDISP:
      {
         return ((uchar *)pmInfoPtr + pmInfoPtr->setDisplayStart);
      } // set display start
      case VBE_GET_SETPAL:
      {
         return ((uchar *)pmInfoPtr + pmInfoPtr->setPalette);
      } // set palette
      default:
      {
         return NULL;
      }
   } // switch on funcNum

} // VBEReadFunc

//
// CreateVideoSeg:
//
// This routine uses the dpmi functions to generate a segment
// for the video memory.  I believe this is needed for some of
// the protected mode vbe routines.  Return 0 if it could not
// generate the descriptor or set it correctly.
//

ushort CreateVideoSeg(uchar *linearAddr,ulong size)
{
   short desc;

// First allocate one descriptor from the list
   if ((desc = dpmi_alloc_desc(1)) < 0) return 0;

// Now set the base
   if (dpmi_set_base(desc,(ulong)linearAddr) != 0)
   {
      dpmi_free_desc(desc);
      return 0;
   } // have a problem, so leave

// Next the limit
   if (dpmi_set_limit(desc,size) != 0)
   {
      dpmi_free_desc(desc);
      return 0;
   } // have a problem, so leave

// All is ok, just return the descriptor.
   return desc;

} // CreateVideoSeg

//
// CheckSetLineLengthFunc:
//
// It seems that many of the vbe's out there do not support function
// 4F06 = VBE_SCAN_LINE_FUNC, though they claim that they should.  So
// we need to check and set something to indicate that we should not
// use these functions.  We check also against the resolution of the
// current mode since the numbers returned must be at least as large
// as those.
//

void CheckSetLineLengthFunc(short w,short h)
{
   memset(&dpmi_reg_data,0,sizeof(dpmi_reg_data));

   dpmi_reg_data.eax = VBE_SCAN_LINE_FUNC;
   dpmi_reg_data.ebx = VBE_GET_SCAN_LEN;

   if (
         dpmi_real_interrupt(0x10,&dpmi_reg_data) != 0   ||
         (dpmi_reg_data.eax & 0xffff) != VBE_RET_OK      ||
         (dpmi_reg_data.ebx & 0xffff) == 0               ||
         (short)(dpmi_reg_data.ecx & 0xffff) <  w        ||
         (short)(dpmi_reg_data.edx & 0xffff) <  h
      )
   {
      lineLengthFuncGood = FALSE;
   } // call doesn't work
   else
   {
      lineLengthFuncGood = TRUE;
   } // seems ok???

} // CheckSetLineLengthFunc

//
// GetModeScanLineInfo:
//
// Routine to get the scan line info by looking at the mode info
// of the current mode.  Return TRUE if things went ok, FALSE
// otherwise.
//

bool GetModeScanLineInfo(int *numPixels,int *numBytes)
{
   VBE_modeInfo* modeInfoPtr; // pointer to storage area
   dpmis_block dpmiBlock;     // storage area for the mode info

   if (
         dpmi_alloc_dos_mem(
            &dpmiBlock, (sizeof(*modeInfoPtr) + 15) >> 4
         ) != 0
      )
   {
      return FALSE;
   } // can't alloc mem

   modeInfoPtr = (VBE_modeInfo *)dpmiBlock.p;

// Get the current mode number
   dpmi_reg_data.eax = VBE_GET_MODE;
   if (
         dpmi_real_interrupt(0x10,&dpmi_reg_data) != 0 ||
         (dpmi_reg_data.eax & 0xffff) != VBE_RET_OK
      )
   {
      dpmi_free_dos_mem(&dpmiBlock);
      return FALSE;
   } // had a problem

// Now set up to get the mode info
   dpmi_reg_data.eax = VBE_GET_MODE_INFO;
   dpmi_reg_data.ecx = dpmi_reg_data.ebx;    // previous call put mode into ebx
   dpmi_reg_data.edi = 0;
   dpmi_reg_data.es  = ((ulong)modeInfoPtr) >> 4;

// Make the call to the vbe to get the mode data
   if (
         dpmi_real_interrupt(0x10,&dpmi_reg_data) != 0 ||
         (dpmi_reg_data.eax & 0xffff) != VBE_RET_OK
      )
   {
      dpmi_free_dos_mem(&dpmiBlock);
      return FALSE;
   } // return if can't get the mode info

// Ok, here have the mode info, let's play...

   *numBytes  = modeInfoPtr->BytesPerScanLine;
   *numPixels = modeInfoPtr->XResolution;

   dpmi_free_dos_mem(&dpmiBlock);

   return TRUE;

} // GetModeScanLineInfo

//
// VBESetScanLineLength:
//
// Routine to set the scan line length of the current video mode.
// It takes a paremeter in pixel length.  Returns the true pixel
// width to which it was set, or zero if there was an error.
// Also sets the global bytesPerScanLine and sets the passed pointer
// to the maximum number of scan lines possible given this width.
// NOTE: This may not work if we have other than 8 bits per pixel!
//

int VBESetScanLineLength(int w,int *hp)
{
   if (lineLengthFuncGood)
   {
      dpmi_reg_data.eax = VBE_SCAN_LINE_FUNC;
      dpmi_reg_data.ebx = VBE_SET_PIXEL_W;
      dpmi_reg_data.ecx = w;
      if (
            dpmi_real_interrupt(0x10,&dpmi_reg_data) != 0 ||
            (dpmi_reg_data.eax & 0xffff) != VBE_RET_OK
         )
      {
         return 0;
      }
      else
      {
         bytesPerScanLine = (dpmi_reg_data.ebx & 0xffff);
         grd_screen_row = bytesPerScanLine;
         *hp = (dpmi_reg_data.edx & 0xffff);
         return (dpmi_reg_data.ecx & 0xffff);
      } // went ok
   } // seems we can call the vbe for this
   else
   {
      int numPixels;

      if (!GetModeScanLineInfo(&numPixels,&bytesPerScanLine))
      {
         return 0;
      }

   // It is assumed that VBEInit has been called so that grd_info.memory
   // has already been set.

      *hp = (grd_info.memory * 1024) / bytesPerScanLine;

      return bytesPerScanLine;

   } // can't call, something wrong

} // VBESetScanLineLength

//
// VBESetPaletteDac:
//
// Routine to set the dac to n bits.  Returns the depth it was
// really set to or 6 if there was an error (since this is default).
//

int VBESetPaletteDac(int dacWidth)
{
   if (VBE_version < 0x102)
      return DEFAULT_DAC_WIDTH;

   dpmi_reg_data.eax = VBE_DAC_WIDTH_FUNC;
   dpmi_reg_data.ebx = VBE_SET_DAC_W | (dacWidth << 8);

   if (
         dpmi_real_interrupt(0x10,&dpmi_reg_data) != 0 ||
         (dpmi_reg_data.eax & 0xffff) != VBE_RET_OK
      )
   {
      return (curDacDepth = DEFAULT_DAC_WIDTH);        // assume is still 6
   } // have a problem

   curDacDepth = (dpmi_reg_data.ebx & 0xff00) >> 8;

// Test if something strange going on

   if (curDacDepth != dacWidth && curDacDepth != DEFAULT_DAC_WIDTH)
   {
      if (dacWidth != DEFAULT_DAC_WIDTH)
         return VBESetPaletteDac(DEFAULT_DAC_WIDTH);
      else
         return (curDacDepth = DEFAULT_DAC_WIDTH);   // something weird, assume 6 is ok
   } // neither dacWidth or 6 ???

// Otherwise return what things were set to

   return curDacDepth;

} // VBESetPaletteDac

//
// VBEGetPaletteDac:
//
// Routine to just get the current palette dac width.  Returns
// 0 if there was some kind of error.
//

int VBEGetPaletteDac()
{
   if (VBE_version < 0x102)
      return DEFAULT_DAC_WIDTH;

   dpmi_reg_data.eax = VBE_DAC_WIDTH_FUNC;
   dpmi_reg_data.ebx = VBE_GET_DAC_W;

   if (
         dpmi_real_interrupt(0x10,&dpmi_reg_data) != 0 ||
         (dpmi_reg_data.eax & 0xffff) != VBE_RET_OK
      )
   {
      return DEFAULT_DAC_WIDTH;
   } // have a problem

// Value really set to in returned bh register

   return (dpmi_reg_data.ebx & 0xff00) >> 8;

} // VBEGetPaletteDac

//
// VBEForceDefaultDac:
//
// Routine to force us to use the 6 bit colors of normal
// 18 bit dacs, even though the vbe says we have a 24
// bit dac.  This is to bypass problems with some cards
// which claim they support it, but don't.
//

void VBEForceDefaultDac()
{
   VBESetPaletteDac(DEFAULT_DAC_WIDTH);
   dacTakes8Bits = FALSE;
} // VBEForceDefaultDac

// These are two palette conversion/copy functions needed for
// transfering 2d style palettes to VBE 2.0 palettes which are
// needed for the palette setting routines of VBE 2.0.  We need
// in particular to shift by 2 in the correct direction if we
// do not have 8 bit dac.  Also the VBE palettes are blue,
// green, red, alpha, while the 2d palettes are stored red,
// green, blue.  Fun fun fun...

void Copy2DtoVBEpal(uchar *dstPtr,uchar *srcPtr,int n);
#pragma aux Copy2DtoVBEpal =  \
   "cmp dword ptr [curDacDepth],6"  \
   "je  copyTop2"             \
"copyTop1:"                   \
   "mov  bl,[edx+0]"          \
   "mov  [eax+2],bl"          \
   "mov  bl,[edx+1]"          \
   "mov  [eax+1],bl"          \
   "mov  bl,[edx+2]"          \
   "mov  [eax+0],bl"          \
   "add  eax,4"               \
   "add  edx,3"               \
   "dec  ecx"                 \
   "jg   copyTop1"            \
   "jmp  copyEnd"             \
"copyTop2:"                   \
   "mov  bl,[edx+0]"          \
   "shr  bl,2"                \
   "mov  [eax+2],bl"          \
   "mov  bl,[edx+1]"          \
   "shr  bl,2"                \
   "mov  [eax+1],bl"          \
   "mov  bl,[edx+2]"          \
   "shr  bl,2"                \
   "mov  [eax+0],bl"          \
   "add  eax,4"               \
   "add  edx,3"               \
   "dec  ecx"                 \
   "jg   copyTop2"            \
"copyEnd:"                    \
   parm [eax] [edx] [ecx]     \
   modify exact [eax ebx ecx edx];

void CopyVBEto2Dpal(uchar *dstPtr,uchar *srcPtr,int n);
#pragma aux CopyVBEto2Dpal =  \
   "cmp dword ptr [curDacDepth],6"  \
   "je  copyTop2"             \
"copyTop1:"                   \
   "mov  bl,[edx+2]"          \
   "mov  [eax+0],bl"          \
   "mov  bl,[edx+1]"          \
   "mov  [eax+1],bl"          \
   "mov  bl,[edx+0]"          \
   "mov  [eax+2],bl"          \
   "add  eax,3"               \
   "add  edx,4"               \
   "dec  ecx"                 \
   "jg   copyTop1"            \
   "jmp  copyEnd"             \
"copyTop2:"                   \
   "mov  bl,[edx+2]"          \
   "shl  bl,2"                \
   "mov  [eax+0],bl"          \
   "mov  bl,[edx+1]"          \
   "shl  bl,2"                \
   "mov  [eax+1],bl"          \
   "mov  bl,[edx+0]"          \
   "shl  bl,2"                \
   "mov  [eax+2],bl"          \
   "add  eax,3"               \
   "add  edx,4"               \
   "dec  ecx"                 \
   "jg   copyTop2"            \
"copyEnd:"                    \
   parm [eax] [edx] [ecx]     \
   modify exact [eax ebx ecx edx];

void CallPaletteFunc(int start,int n,VBE_palette* palPtr,int func);
#pragma aux CallPaletteFunc parm [edx] [ecx] [edi] [ebx] modify exact [ebx];

//
// VBESetPalette:
//
// Set the palette using the VBE 2.0 function.  This is sort of a
// pain since the palettes are VGA type palettes in RGB format,
// while the vbe format is in blue,green,red,alpha format.
// NOTE: We call the vesa vbe palette setter even if we are in
// a vga mode since we might have the dac set to 8 bits and then
// the normal vga setter will not work (though this should never
// happen since we switch back to 6 bits in the vga case).
//

void vbe_set_palette(int start,int n,uchar *palData)
{
   if (grd_mode < GRM_FIRST_SVGA_MODE)
   {
      vga_set_pal(start,n,palData);
   }
   else if (setPaletteFuncPtr)
   {
      VBE_palette locPal[256];            // allocate 1K on stack for palette

   // Make a version of the palette suitable for the function call.

      Copy2DtoVBEpal((uchar *)&locPal[0],palData,n);

   // Actually make the function call to set the palette

      CallPaletteFunc(start,n,locPal,VBE_SET_PAL);

   } // have protected mode function to set palette
   else
   {
      dpmis_block dpmiBlock;

   // First allocate room in low memory for the palette data, returning
   // if can not allocate.  Need 256*(paletteSize) bytes = 16*size paragraphs.

      if (dpmi_alloc_dos_mem(&dpmiBlock,(16*sizeof(VBE_palette))) != 0) return;

   // Now copy the palette we have to the low memory area, being careful to
   // make the alpha byte each time.  Make sure to copy from the start
   // of the palette and copy the whole thing.  No, let's just copy what
   // we need and read from that point.

      Copy2DtoVBEpal(dpmiBlock.p,palData,n);

   // Now make the call to the vbe routine, using interrupt if no protected
   // mode routine.

      dpmi_reg_data.eax = VBE_PAL_FUNC;
      dpmi_reg_data.ebx = VBE_SET_PAL;
      if (useBlankInPalSet) dpmi_reg_data.ebx |= VBE_WAIT_VRT;
      dpmi_reg_data.ecx = n;
      dpmi_reg_data.edx = start;
      dpmi_reg_data.es  = ((ulong)dpmiBlock.p >> 4);  // es:di contains pointer
      dpmi_reg_data.edi = 0;                          // we have copied only data we want

   // I guess we have no error condition, which is bad, so just call
   // the real interrupt and that is that.

      dpmi_real_interrupt(0x10,&dpmi_reg_data);    // hopefully set the palette

   // Finally rid ourselves of this low memory block

      dpmi_free_dos_mem(&dpmiBlock);

   } // need to deal with vesa set mode function interrupt

} // VBESetPalette

//
// VBEGetPalette:
//
// Routine to get the palette returned by the VBE 2.0 get palette
// function and stick it into the 2d structure passed in.
//

void vbe_get_palette(int start,int n,uchar *palData)
{
   if (grd_mode < GRM_FIRST_SVGA_MODE)
   {
      vga_get_pal(start,n,palData);
   } // use normal vga stuff
   else if (setPaletteFuncPtr)
   {
      VBE_palette locPal[256];            // allocate 1K on stack for palette

   // Get the palette into our local copy by making the protected
   // mode call.

      CallPaletteFunc(start,n,locPal,VBE_GET_PAL);

   // Assume this only copied n values starting at start into locPal
   // and then copy these into our 2d palette

      CopyVBEto2Dpal(palData,(uchar *)&locPal[0],n);

   } // have protected mode routine to set the palette
   else
   {
      dpmis_block dpmiBlock;

   // First allocate room in low memory for the palette data, returning
   // if can not allocate.  Need 256*(paletteSize) bytes = 16*size paragraphs.

      if (dpmi_alloc_dos_mem(&dpmiBlock,(16*sizeof(VBE_palette))) != 0) return;

   // Now make the call to the vbe routine to get the data

      dpmi_reg_data.eax = VBE_PAL_FUNC;
      dpmi_reg_data.ebx = VBE_GET_PAL;
      dpmi_reg_data.ecx = 256;
      dpmi_reg_data.edx = 0;
      dpmi_reg_data.es  = ((ulong)dpmiBlock.p >> 4);      // es:di contains pointer
      dpmi_reg_data.edi = 0;

   // Now make the function call, leaving with nothing if error
      if (
            dpmi_real_interrupt(0x10,&dpmi_reg_data) != 0 ||
            (dpmi_reg_data.eax & 0xffff) != VBE_RET_OK
         )
      {
         dpmi_free_dos_mem(&dpmiBlock);
         return;
      } // no good, don't set

   // At this point we have the data we need in dpmiBlock.p, so
   // need to copy from the correct place to our destination.

      CopyVBEto2Dpal(palData,(uchar *)((VBE_palette *)dpmiBlock.p + start),n);

      dpmi_free_dos_mem(&dpmiBlock);               // get rid of dos mem

   } // need to make a vesa call to set the palette

} // VBEGetPalette

//
// SetModeIntern:
//
// This is an internal routine to set up the various 2d pointers
// and such which need to be set during mode switching.  The mode
// is the internal 2d mode number, while the second is a flag
// for whether we are setting a linear mode.  Return -1 if there
// was some kind of error else return 0.
//

int SetModeIntern(int mode,bool isLinear)
{
// Need to set the flag indicating linear mode bit
   VBE_modeLinear = isLinear;

// Delete the protected mode information we obtained and set the
// function pointers to NULL.  Also clear the gsSel to be made
// again for the next mode.

   if (pmInfoPtr)
   {
      Free(pmInfoPtr);
      pmInfoPtr = NULL;
   }

   if (gsSel)
   {
      dpmi_free_desc(gsSel);
      gsSel = 0;
   }

// Determine if we can use the damn set line length function since it
// seems that some vbes just don't work as they are supposed to.  This
// sets a global which we look at when calling vbe func 4F06.

   CheckSetLineLengthFunc(
      grd_mode_info[mode].w,
      grd_mode_info[mode].h
   );

// Look to see if we have an 8 bit dac capability, and if so, set the
// dac to that depth.  This needed since every mode change resets the
// dac to 6 bit depth by default.

   if (dacTakes8Bits)
   {
      VBESetPaletteDac(8);
   } // can handle 8 bit

// Next set up the selector to the video memory if necessary
   if (VBE_version >= 0x200)
   {
      if (isLinear)
      {
         gsSel = CreateVideoSeg(videoLinearBase,SIZE_OF_LINEAR_BUFF - 1);
      } // have linear mode
      else
      {
         gsSel = CreateVideoSeg((uchar *)((ulong)VGA_BASE),0xFFFF);
      } // have normal mode

      if (gsSel == 0) return -1;

   } // need to make the selector

// We also want to make sure the scan line length is what it should
// be since we do not know if perhaps it will be what it was previously
// set to when last in this mode.  This will also set the bytesPerScanLine
// and the 2d global grd_screen_row.

   {
      int maxVert;
      VBESetScanLineLength(grd_mode_info[mode].w,&maxVert);
   } // make sure the width is what we want

// Now set up the tables so that we know which type of functions
// to use for all of our primitives.

   switch(grd_mode_info[mode].bitDepth)
   {
      case 4:
      {
         return -1;
         break;
      }
      case 8:
      {
         if (isLinear)
         {
            grd_screen_bmt = BMT_FLAT8;
         } // linear mode
         else
         {
            grd_screen_bmt = BMT_BANK8;
         } // non-linear mode
         break;
      }
      case 15:
      case 16:
      {
         if (isLinear)
         {
            grd_screen_bmt = BMT_FLAT16;
         } // linear mode
         else
         {
            grd_screen_bmt = BMT_BANK16;
         } // non-linear mode
         break;
      }
      case 24:
      {
         if (isLinear)
         {
            grd_screen_bmt = BMT_FLAT24;
         } // linear mode
         else
         {
            grd_screen_bmt = BMT_BANK24;
         } // non-linear mode
         break;
      }
      case 32:
      {
         return -1;
         break;
      }
   } // switch

// Now set the grd_mode_cap entries

   if (isLinear)
   {
      grd_mode_cap.vbase = videoLinearBase;
      grd_valloc_mode = 0;    // indicates use the vbase
   } // have linear buffer
   else
   {
      grd_mode_cap.vbase = (uchar *)((ulong)VGA_BASE);
      grd_valloc_mode = 1;    // indicates ignore the vbase
   } // have normal bank buffer

// Now allocate and read in the functions for setting the
// bank, the display start, and the palette if we can.

   if (VBE_version >= 0x200)
   {
      setBankFuncPtr      = VBEReadFunc(VBE_GET_SETBANK);
      setDispStartFuncPtr = VBEReadFunc(VBE_GET_SETDISP);
      setPaletteFuncPtr   = VBEReadFunc(VBE_GET_SETPAL);
   } // read in functions
   else
   {
      setBankFuncPtr      = NULL;
      setDispStartFuncPtr = NULL;
      setPaletteFuncPtr   = NULL;
   } // no functions, set to null

// We store in a global the amount to shift a bank before calling
// the bank setting function.

   bankShift = grd_mode_info[mode].bankShift;

// The most time critical routine depending on the mode is the bank
// switching routine which is often called in the middle of draw
// routines.  In VBE 2.0 and greater we have protected mode functions
// to call, so we use those otherwise we have to make the BIOS call
// to set the bank.  The final question is whether we need to set the
// bank on both vesa windows, or just winA.

   if (grd_mode_info[mode].flags & GRI_USE_WINA)
   {
      if (VBE_version >= 0x200 && setBankFuncPtr)
      {
         ((void **)vesa_device_table)[GDC_SET_BANK] =
            &SetBankProtA;
      } // have protected mode bank switcher
      else
      {
         ((void **)vesa_device_table)[GDC_SET_BANK] =
            &SetBankRealA;
      } // no protected mode bank switcher
   } // we can use just window A
   else
   {
      if (VBE_version >= 0x200 && setBankFuncPtr)
      {
         ((void **)vesa_device_table)[GDC_SET_BANK] =
            &SetBankProtAB;
      } // have protected mode bank switcher
      else
      {
         ((void **)vesa_device_table)[GDC_SET_BANK] =
            &SetBankRealAB;
      } // no protected mode bank switcher
   } // seems we need to use both windows.

// Finally return 0 to indicate no errors
   return 0;

} // SetModeIntern

//
// vbe_set_mode:
//
// Routine to set the video mode and update the various pointers
// we need to update in the process.  Return 0 if all ok, -1 if
// mode set failed for any reason.  flags has the following meanging:
// 0 (or FALSE) means save screen
// 1 (or TRUE)  means clear screen
// 2 means save screen, linear mode
// 3 means clear screen, linear mode
// If a linear mode is requested, but it cannot be set, then we
// return indicating failure.
//

int vbe_set_mode(int mode,int flags)
{
   short modeFlags = 0;

// Before doing anything, clear out all of the memory that we have
// access to.  Do only if in a linear mode since only there do we
// have easy access to everything.  This seems to be necessary for
// the mach64 888000-GX or whatever chip.

   if (VBE_modeLinear)
   {
      memset(videoLinearBase,0,grd_info.memory << 10);
   } // try clearing all the memory in card

// Check if it really is a vga mode
   if (mode < GRM_FIRST_SVGA_MODE)
   {
      VBE_modeLinear = FALSE;             // going to non-linear mode
      curDacDepth = DEFAULT_DAC_WIDTH;    // and make sure we have the correct width
      return vga_set_mode(mode,flags);
   } // really is an svga mode

// Check that this mode is supported
   if (!(grd_mode_info[mode].flags & GRM_IS_SUPPORTED))
   {
      return -1;
   } // mode not supported

// Now set up the mode flags correctly given our incomming flags
   if (!(flags & SET_MODE_CLEAR_BIT))
   {
      modeFlags |= (1 << VBE_DONT_CLEAR_SHIFT);
   } // indicate we don't clear
   if (flags & SET_MODE_LIN_BIT)
   {
      modeFlags |= (1 << VBE_LINEAR_BUFF_SHIFT);
   } // indicate we want a linear mode

// Now actually go to the new mode, returning if it somehow fails
   dpmi_reg_data.eax = VBE_SET_MODE;
   dpmi_reg_data.ebx =
      (modeFlags | grd_mode_info[mode].mode_vesa);

// This is the call to actually set the mode

   if (
         dpmi_real_interrupt(0x10,&dpmi_reg_data) != 0 ||
         (dpmi_reg_data.eax & 0xffff) != VBE_RET_OK
      )
   {
      return -1;
   } // could not set the mode somehow

// Make sure that we return the dac width to 6 since that is what happens when
// the mode switch is actually made.

   curDacDepth = DEFAULT_DAC_WIDTH;    // and make sure we have the correct width

// Finally call the routine to set the internal variables dependant on the mode

   return SetModeIntern(mode,(modeFlags & VBE_LINEAR_BUFFER) ? TRUE : FALSE);

} // vbe_set_mode

//
// vbe_get_mode:
//
// Return the 2d video mode which is current and set the VBE_modeLinear
// global to make sure it is correct.  Return -1 if some kind of
// error or if can't find this mode.
//

int vbe_get_mode()
{
   grs_mode_info *modeInfoPtr = &grd_mode_info[0];
   grs_mode_info *lastInfoPtr = &grd_mode_info[GRD_MODES];
   short vbeMode;
   int locMode;

// Make vbe call to get the current mode number
   dpmi_reg_data.eax = VBE_GET_MODE;
   if (
         dpmi_real_interrupt(0x10,&dpmi_reg_data) != 0 ||
         (dpmi_reg_data.eax & 0xffff) != VBE_RET_OK
      )
   {
      return -1;
   } // had a problem

// Retrieve the vbe mode sent to the set mode routine
   vbeMode = (dpmi_reg_data.ebx & 0xffff);

// Set the global VBE_modeLinear flag and get the raw vbe mode
   VBE_modeLinear = (vbeMode & VBE_LINEAR_BUFFER);
   vbeMode &= VBE_RAW_MODE_MASK;

// Now search for our mode number in the list of modes

   for(locMode = -1; modeInfoPtr < lastInfoPtr; modeInfoPtr++)
   {
      if (modeInfoPtr->mode_vesa == vbeMode)
      {
         locMode = modeInfoPtr->mode_2d;
         break;
      } // found it
   } // for modeInfoPtr

// Simply return what we found (or if not found, return -1)
   return locMode;

} // vbe_get_mode

//
// VBESetWidth:
//
// Try to set the requested width by calling the more detailed
// routine written above.  Return the value which is the
// true width to which it was set.
//

int vbe_set_width(int w)
{
   if (grd_mode < GRM_FIRST_SVGA_MODE)
   { // don't support vga mode width changes.
      grd_screen_max_height = grd_cap->h;
      return grd_cap->w;
   }

   return VBESetScanLineLength(w,&grd_screen_max_height);

} // vbe_set_width

//
// VBEGetWidth:
//
// Return the current pixel width of the logical scan line.
//

int vbe_get_width()
{
   if (lineLengthFuncGood)
   {
      dpmi_reg_data.eax = VBE_SCAN_LINE_FUNC;
      dpmi_reg_data.ebx = VBE_GET_SCAN_LEN;

      if (
            dpmi_real_interrupt(0x10,&dpmi_reg_data) != 0 ||
            (dpmi_reg_data.eax & 0xffff) != VBE_RET_OK
         )
      {
         return -1;
      } // call no good
      else
      {
         return (dpmi_reg_data.ecx & 0xffff);
      } // call worked
   } // can use normal 1.1 vesa call
   else
   {
      int numPixels;
      int numBytes;

      if (!GetModeScanLineInfo(&numPixels,&numBytes))
      {
         return -1;
      } // an error regardless

      return numPixels;

   } // things are broken, use other info

} // vbe_get_width

//
// VBEGetRowBytes:
//
// Need a routine which returns the number of bytes to offset by
// to get to the next logical scan line.
//

int VBEGetRowBytes()
{
   if (lineLengthFuncGood)
   {
      dpmi_reg_data.eax = VBE_SCAN_LINE_FUNC;
      dpmi_reg_data.ebx = VBE_GET_SCAN_LEN;

      if (
            dpmi_real_interrupt(0x10,&dpmi_reg_data) != 0 ||
            (dpmi_reg_data.eax & 0xffff) != VBE_RET_OK
         )
      {
         return -1;
      } // call no good
      else
      {
         return (dpmi_reg_data.ebx & 0xffff);
      } // call worked
   } // have decent vbe
   else
   {
      int numPixels;
      int numBytes;

      if (!GetModeScanLineInfo(&numPixels,&numBytes))
      {
         return -1;
      } // an error regardless

      return numBytes;

   } // have shitty vbe

} // VBEGetRowBytes

//
// VBEGetScreenHeight
//
// Routine to determine the maximum number of scan lines given
// the current scan line length.
//

int VBEGetScreenHeight()
{
   if (lineLengthFuncGood)
   {
      dpmi_reg_data.eax = VBE_SCAN_LINE_FUNC;
      dpmi_reg_data.ebx = VBE_GET_SCAN_LEN;

      if (
            dpmi_real_interrupt(0x10,&dpmi_reg_data) != 0 ||
            (dpmi_reg_data.eax & 0xffff) != VBE_RET_OK
         )
      {
         return -1;
      } // call no good
      else
      {
         return (dpmi_reg_data.edx & 0xffff);
      } // call worked
   } // have decent vbe
   else
   {
      int numPixels;

      if (!GetModeScanLineInfo(&numPixels,&bytesPerScanLine))
      {
         return -1;
      } // an error regardless

   // It is assumed that VBEInit has been called, so grd_info.memory is set
      return (grd_info.memory * 1024) / bytesPerScanLine;

   } // have shitty vbe

} // VBEGetScreenHeight

//
// vbe_get_focus:
//
// Routine to just return the x,y offset in pixels from the top left of
// video space of the start of the visible screen.
//

void vbe_get_focus(int *xPtr,int *yPtr)
{
   dpmi_reg_data.eax = VBE_DISP_START_FUNC;
   dpmi_reg_data.ebx = VBE_GET_START;

   if (
         dpmi_real_interrupt(0x10,&dpmi_reg_data) != 0 ||
         (dpmi_reg_data.eax & 0xffff) != VBE_RET_OK
      )
   {
      *xPtr = -1;
      *yPtr = -1;
   } // indicate failure of sorts
   else
   {
      *xPtr = (dpmi_reg_data.ecx & 0xffff);
      *yPtr = (dpmi_reg_data.edx & 0xffff);
   } // success
} // vbe_get_focus

//
// vbe_set_focus:
//
// Call the internal assembly language routine to change the focus
// and then set up the 2d variables that are necessary to change.
// NOTE: We set some internal 2d variables here, so this isn't a pure
// vbe function!!!!
//

extern void vga_set_focus(int x,int y);

void vbe_set_focus(int x,int y)
{
   if (grd_mode < GRM_FIRST_SVGA_MODE && grd_mode >= 0)
   {
      vga_set_focus(x,y);
   } // call the vga version
   else
   {
      void SetFocusIntern(int x,int y,int waitVRT);

   // Start by setting the focus internally, indicate we
   // do not wait for the vertical retrace before setting.

      SetFocusIntern(x,y,0);

      grd_screen->x = x;
      grd_screen->y = y;

   // Also set the start of the screen bits
      grd_visible_canvas->bm.bits =
         grd_screen_canvas->bm.bits + y*bytesPerScanLine + x;

   } // use the vbe version (this only for 8 bits per pixel mode!)

} // vbe_set_focus

//
// VBEInit:
//
// Call the VBE detection routine, and if detected, read in the
// modes it has and compare against the information in the
// local mode info array.  Also fill in the granularity and
// the linear buffer existance bit.  Return FALSE if not detected
// or detected but no modes, and TRUE otherwise.
//

int VBEInit(grs_sys_info *info)
{
   dpmis_block dpmiBlockInfo;    // storage area for the general info
   dpmis_block dpmiBlockMode;    // storage area for the mode info
   VBE_infoBlock* infoBlock;     // pointer to low mem where buffer lives
   VBE_modeInfo*  modeInfo;      // for each mode we ask VBE for info
   short *srcModePtr;            // pointer to modes returned
   short *dstModePtr;            // pointer to mode list in grs_sys_info
   short *endListPtr;            // pointer to end of destination mode list
   short *firstVesaPtr;          // pointer to the first vesa mode we enter in our list
   grs_mode_info *grsModeInfoPtr; // running pointer into grs_mode_info
   int count;                    // don't go running off into memory if card is bad

// First set the pointers in the blocks to 0 to indicate nothing allocated
   dpmiBlockInfo.p = NULL;
   dpmiBlockMode.p = NULL;

// Try to allocate the memory for the main info block
   if (
         dpmi_alloc_dos_mem(
            &dpmiBlockInfo,
            (sizeof(*infoBlock) + 15) >> 4
         ) != 0
      )
   {
      goto VBEInit_exit;
   } // can't allocate anything

// Now make the info call to the video bios to see if we actually
// have vesa capability.

   dpmi_reg_data.eax = VBE_DETECT;                 // indicate we query for vbe
   dpmi_reg_data.edi = 0;                          // es:di points to block to fill
   dpmi_reg_data.es  = ((ulong)dpmiBlockInfo.p) >> 4;  // get the segment

// Indicate we would like the vbe2 information if available when
// we initialize things so fill signiture with "VBE2" before hand.
   ((VBE_infoBlock *)dpmiBlockInfo.p)->VESASignature = '2EBV';

// Make the actual interrupt call, filling the structure.  If something
// bad happens then simply return with a fail.

   if (
         dpmi_real_interrupt(0x10,&dpmi_reg_data) != 0 ||
         (dpmi_reg_data.eax & 0xffff) != VBE_RET_OK
      )
   {
      goto VBEInit_exit;
   } // interrupt no good

// Now parse what we have, so first give it some structure and really
// check for vesa existance by checking the signature.  If ok get the
// version and get the pointer to the mode list.

   infoBlock = (VBE_infoBlock *)dpmiBlockInfo.p;
   if (infoBlock->VESASignature != 'ASEV') return FALSE;
   VBE_version = infoBlock->VESAVersion;           // store the version for others

   if (VBE_version < 0x101)
      goto VBEInit_exit;                           // we don't deal with old VBEs

   useBlankInPalSet = (infoBlock->capabilities & VBE_BLANK_RAM_DAC);
   dacTakes8Bits = (infoBlock->capabilities & VBE_8_BIT_DAC);
   info->id_maj = VESA_ID_MAJ;
   info->id_min = VESA_ID_MIN;
   info->memory = infoBlock->TotalMemory << 6;     // get memory in Kb (not 64Kb blocks)
   srcModePtr = (short *)((infoBlock->VideoModePtrSeg << 4) + infoBlock->VideoModePtrOff);

// Get ready for the mode info by allocating seperate space for it, again in low memory

   if (
         dpmi_alloc_dos_mem(
            &dpmiBlockMode,
            (sizeof(*modeInfo) + 15) >> 4
         ) != 0
      )
   {
      goto VBEInit_exit;
   } // can't allocate anything


// If we have VBE 2.0 or higher, we use the vesa interface for
// changing the palette, otherwise we program the vga registers
// using vga_set/get_pal routines.  NOTE: Eventually need to check
// for VGA compatibility to be sure it is ok to call the vga functions.

   if (VBE_version >= 0x200)
   {
      ((void **)vesa_device_table)[GDC_SET_PAL] = &vbe_set_palette;
      ((void **)vesa_device_table)[GDC_GET_PAL] = &vbe_get_palette;
   } // use VBE set/get palette
   else
   {
      ((void **)vesa_device_table)[GDC_SET_PAL] = &vga_set_pal;
      ((void **)vesa_device_table)[GDC_GET_PAL] = &vga_get_pal;
   } // use VGA set/get palette

// Now run through list of modes until either fill up our local list
// or we come to the end of the system list.  For each mode in the
// source list, determine if it is a mode the 2d supports by first
// looking for a mode with the same resolution and bit depth.  If
// found then store the current vesa mode number in the appropriate
// spot in the mode list.  Then fill in the granularity and the bit
// indicating whether the mode supports a linear frame buffer.

   dstModePtr = info->modes;                       // point to start
   endListPtr = dstModePtr + GRD_MODES;            // and the end
   count = 0;                                      // count how many modes we check

// First we need to store the vga video modes we support
   *dstModePtr++ = GRM_320x200x8;
   *dstModePtr++ = GRM_320x200x8X;
   *dstModePtr++ = GRM_320x400x8;
   *dstModePtr++ = GRM_320x240x8;
   *dstModePtr++ = GRM_320x480x8;
   firstVesaPtr  = dstModePtr;

// Unfortunately, have to work around bad implementations of vesa vbe on
// certain cards.  Some do not end correctly with -1 and some do
// not have only vbe modes in the list they give us.
#define COUNT_LIMIT 0x40

// Loop through all of the modes in the returned list

   while
      (
         *srcModePtr != -1 &&
         dstModePtr < endListPtr &&
         count < COUNT_LIMIT
      )
   {
   // Increment the counter for modes checked
      count++;

   // This is something to save us from bad cards which put wrong
   // values into their mode list.  Though it also bypasses the
   // vbe 2.0 required mode 81FF which gives access to all of the
   // video memory.

      if (*srcModePtr < 0x100 || *srcModePtr >= 0x200)
      {
         srcModePtr++;
         continue;
      } // go to loop bottom

   // Set up to get mode information from vbe

      dpmi_reg_data.eax = VBE_GET_MODE_INFO;          // call the get mode info func
      dpmi_reg_data.ecx = *srcModePtr;                // indicate the mode to check on
      dpmi_reg_data.edi = 0;                          // ed:di points to area
      dpmi_reg_data.es  = ((ulong)dpmiBlockMode.p) >> 4;  // get the segment

   // Make the call to the vbe to get the mode data
      if (
            dpmi_real_interrupt(0x10,&dpmi_reg_data) != 0 ||
            (dpmi_reg_data.eax & 0xffff) != VBE_RET_OK
         )
      {
         if (dstModePtr > firstVesaPtr)
         {
            goto VBEInit_done;
         } // have at least one vesa mode, so don't fail
         else
         {
            goto VBEInit_exit;
         } // we must fail since no vesa modes
      } // interrupt no good

   // Give our data some structure and continue if we have graphics mode
      modeInfo = (VBE_modeInfo *)dpmiBlockMode.p;

      if (modeInfo->ModeAttributes & VBE_GRAPHICS_MODE)
      {
         grsModeInfoPtr = grd_mode_info + GRD_MODES - 1;  // point at end of list

      // First look for the mode in our list of modes
         while(
            grsModeInfoPtr >= grd_mode_info &&
            (
               grsModeInfoPtr->w != modeInfo->XResolution ||
               grsModeInfoPtr->h != modeInfo->YResolution ||
               grsModeInfoPtr->bitDepth != modeInfo->BitsPerPixel
            )
         )
         {
            grsModeInfoPtr--;
         } // while loop

      // Check if mode found and we support it
         if (
            grsModeInfoPtr >= grd_mode_info &&
            (grsModeInfoPtr->flags & GRM_IS_SUPPORTED)
         )
         {
         // Save the real vesa number in our list
            grsModeInfoPtr->mode_vesa = *srcModePtr;

            if (modeInfo->ModeAttributes & VBE_MODE_SUPPORTED)
            {
               uchar bankShift = 0;
               while ((64 >> bankShift) != modeInfo->WinGranularity)
                  bankShift++;

            // Will multiply each bank number by 2^bankShift before calling
            // the vbe bank setting function
               grsModeInfoPtr->bankShift = bankShift;

            // Determine if it will be ok to switch banks only for window A
               if ((modeInfo->WinAAttributes & VBE_WIN_ALL_ATTR) == VBE_WIN_ALL_ATTR)
               {
                  grsModeInfoPtr->flags |= GRI_USE_WINA;
               } // can use just win A

            // Finally deal with the possibility of a linear buffer mode
               if (
                     VBE_version >= 0x200 &&
                     (modeInfo->ModeAttributes & VBE_MODE_LINEAR)
                  )
               {
                  if (videoLinearBase == NULL)
                  {
                     if (
                           dpmi_map_physical_mem(
                              modeInfo->PhysBasePtr,  // this is the physical address
                              SIZE_OF_LINEAR_BUFF,    // make it 8Mb large
                              &videoLinearBase        // and store the address here
                           ) == 0
                        )
                     {
                        grsModeInfoPtr->flags |= GRM_IS_LINEAR;
                     } // success making the linear address
                  } // need to setup the linear address
                  else
                  {
                     grsModeInfoPtr->flags |= GRM_IS_LINEAR;
                  } // already mapped physical address
               } // the mode supports a linear mode

            // Finally enter the mode in our 2d mode list and inc pointer
               *dstModePtr++ = grsModeInfoPtr->mode_2d;

            } // hardware supports mode
            else
            {
               grsModeInfoPtr->flags &= ~GRM_IS_SUPPORTED;
            } // hardware can't support mode
         } // graphics mode found in our list
      } // have graphics mode

      srcModePtr++;                                   // inc for next loop

   } // main while loop

// Now terminate list with the value -1
VBEInit_done:
   *dstModePtr = -1;
   dpmi_free_dos_mem(&dpmiBlockInfo);
   dpmi_free_dos_mem(&dpmiBlockMode);

// Finally go through all the modes in the modes list, setting to
// 0 the bit for supported if we in fact do not support that mode.

   grsModeInfoPtr = grd_mode_info + GRM_FIRST_SVGA_MODE;
   while(grsModeInfoPtr < grd_mode_info + GRD_MODES)
   {
      if (!grsModeInfoPtr->mode_vesa)
         grsModeInfoPtr->flags &= ~GRM_IS_SUPPORTED;
      grsModeInfoPtr++;
   } // loop through the list

   return TRUE;

VBEInit_exit:
   if (dpmiBlockInfo.p)
      dpmi_free_dos_mem(&dpmiBlockInfo);
   if (dpmiBlockMode.p)
      dpmi_free_dos_mem(&dpmiBlockMode);
   info->modes[0] = -1;
   VBE_version = -1;
   return FALSE;

} // VBEInit

//
// vbe_close:
//
// This routine shuts down the interface to the vesa driver
//

#pragma off(unreferenced)
void vbe_close(grs_sys_info *info)
{
   dpmi_unmap_linear_address(videoLinearBase);
   videoLinearBase = NULL;

   VBE_version = 0;

   if (pmInfoPtr) Free(pmInfoPtr);
   pmInfoPtr = NULL;

   setBankFuncPtr = NULL;
   setDispStartFuncPtr = NULL;
   setPaletteFuncPtr = NULL;

   if (gsSel) dpmi_free_desc(gsSel);
   gsSel = 0;
} // vbe_close
#pragma on(unreferenced)

#endif /* !_WIN32 */
