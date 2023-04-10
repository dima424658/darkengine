/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/vesa.h $
 * $Revision: 1.2 $
 * $Author: KEVIN $
 * $Date: 1996/05/20 11:27:57 $
 *
 * Public vesa vbe structures and flags
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __VESA_H
#define __VESA_H

#ifdef __cplusplus
extern "C" {
#endif
//
// We begin with the the equates for the VBE functions themselves
// together with the version number with which each is associated
//

#define VBE_DETECT            0x4F00      // 1.0
#define VBE_GET_MODE_INFO     0x4F01      // 1.0
#define VBE_SET_MODE          0x4F02      // 1.0
#define VBE_GET_MODE          0x4F03      // 1.0
#define VBE_STATE_FUNC        0x4F04      // 1.0
#define    VBE_STATE_SIZE     0x0         // 1.0
#define    VBE_SAVE_STATE     0x1         // 1.0
#define    VBE_RESTORE_STATE  0x2         // 1.0
#define VBE_BANK_FUNC         0x4F05      // 1.0
#define    VBE_SET_BANK       0x0         // 1.0
#define    VBE_GET_BANK       0x1         // 1.0
#define VBE_SCAN_LINE_FUNC    0x4F06      // 1.1
#define    VBE_SET_PIXEL_W    0x0         // 1.1
#define    VBE_GET_SCAN_LEN   0x1         // 1.1
#define    VBE_SET_BYTE_W     0x2         // 2.0
#define    VBE_GET_MAX_LEN    0x3         // 2.0
#define VBE_DISP_START_FUNC   0x4F07      // 1.1
#define    VBE_SET_START      0x0         // 1.1
#define    VBE_GET_START      0x1         // 1.1
#define VBE_DAC_WIDTH_FUNC    0x4F08      // 1.2
#define    VBE_SET_DAC_W      0x0         // 1.2
#define    VBE_GET_DAC_W      0x1         // 1.2
#define VBE_PAL_FUNC          0x4F09      // 2.0
#define    VBE_SET_PAL        0x0         // 2.0
#define    VBE_GET_PAL        0x1         // 2.0
#define    VBE_SET_2ND_PAL    0x2         // 2.0
#define    VBE_GET_2ND_PAL    0x3         // 2.0
#define VBE_GET_FUNCS         0x4F0A      // 2.0
#define    VBE_GET_PMINFO     0x0         // 2.0
#define    VBE_GET_SETBANK    0x1         // 2.0
#define    VBE_GET_SETDISP    0x2         // 2.0
#define    VBE_GET_SETPAL     0x3         // 2.0

#define VBE_RET_OK            0x004f

// Flags for the save/restore state function

#define VBE_STATE_HARDWARE    0x0001
#define VBE_STATE_BIOS        0x0002
#define VBE_STATE_DAC         0x0004
#define VBE_STATE_SVGA        0x0008
#define VBE_STATE_ALL         0x000F

// Flags for the mode setting function

#define VBE_DONT_CLEAR        0x8000      // don't clear display memory
#define VBE_DONT_CLEAR_SHIFT  0x0F        // shift 15 for don't clear bit
#define VBE_LINEAR_BUFFER     0x4000      // enable linear frame buffer
#define VBE_LINEAR_BUFF_SHIFT 0x0E        // shift 14 for linear buff bit
#define VBE_RAW_MODE_MASK     ~(VBE_DONT_CLEAR | VBE_LINEAR_BUFFER)

// The following can be OR'd with either VBE_SET_START or
// VBE_SET_PAL to make sure these two are set only on retrace.

#define VBE_WAIT_VRT          0x0080      // VBE 2.0
#define VBE_WAIT_VRT_SHIFT    0x7         // bit position of wait bit

// Have the default width of the dac palette registers

#define DEFAULT_DAC_WIDTH        6        // usually have 6 bits

#pragma pack(1)

//
// Now for the structure definitions
//

// First the vbe information block

typedef struct {
   ulong VESASignature;          /* 'VESA' 4 byte signature          */
   short VESAVersion;            /* VBE version number               */
   ulong	OEMStringPtr;          	/* Pointer to OEM string            */
   ulong	capabilities;           /* Capabilities of video card       */
   ushort VideoModePtrOff;       /* Pointer to supported modes offset  */
   ushort VideoModePtrSeg;       /* segment of video mode list pointer */
   short TotalMemory;            /* Number of 64kb memory blocks     */

   /* VBE 2.0 extension information */

   short	OemSoftwareRev;			/* OEM Software revision number		*/
   ulong	OemVendorNamePtr;		   /* Pointer to Vendor Name string	*/
   ulong	OemProductNamePtr;		/* Pointer to Product Name string	*/
   ulong OemProductRevPtr;		   /* Pointer to Product Revision str	*/
   char  reserved[222];          /* Pad to 256 byte block size       */
   char	OemDATA[256];			   /* Scratch pad for OEM data			*/
} VBE_infoBlock;

// And next the block returned by the VBE_MODE_INFO function

typedef struct {
   short ModeAttributes;         /* Mode attributes                  */
   char  WinAAttributes;         /* Window A attributes              */
   char  WinBAttributes;         /* Window B attributes              */
   short WinGranularity;         /* Window granularity in k          */
   short WinSize;                /* Window size in k                 */
   short WinASegment;            /* Window A segment                 */
   short WinBSegment;            /* Window B segment                 */
   ulong	WinFuncPtr;				   /* Pointer to window function       */
   short BytesPerScanLine;       /* Bytes per scanline               */
   short XResolution;            /* Horizontal resolution            */
   short YResolution;            /* Vertical resolution              */
   char  XCharSize;              /* Character cell width             */
   char  YCharSize;              /* Character cell height            */
   char  NumberOfPlanes;         /* Number of memory planes          */
   char  BitsPerPixel;           /* Bits per pixel                   */
   char  NumberOfBanks;          /* Number of CGA style banks        */
   char  MemoryModel;            /* Memory model type                */
   char  BankSize;               /* Size of CGA style banks          */
   // VBE 1.1 extensions
   char  NumberOfImagePages;     /* Number of images pages           */
   char  res1;                   /* Reserved                         */
   // VBE 1.2 extensions
   char  RedMaskSize;            /* Size of direct color red mask    */
   char  RedFieldPosition;       /* Bit posn of lsb of red mask      */
   char  GreenMaskSize;          /* Size of direct color green mask  */
   char  GreenFieldPosition;     /* Bit posn of lsb of green mask    */
   char  BlueMaskSize;           /* Size of direct color blue mask   */
   char  BlueFieldPosition;      /* Bit posn of lsb of blue mask     */
   char  RsvdMaskSize;           /* Size of direct color res mask    */
   char  RsvdFieldPosition;      /* Bit posn of lsb of res mask      */
   char  DirectColorModeInfo;    /* Direct color mode attributes     */
   // VBE 2.0 extensions
   uchar	*PhysBasePtr;			   /* Physical address for linear buf	*/
   ulong	OffScreenMemOffset;		/* Pointer to start of offscreen mem*/
   short OffScreenMemSize;		   /* Amount of offscreen mem in 1K's	*/
   char  res2[206];        		/* Pad to 256 byte block size       */
} VBE_modeInfo;

//
// The VBE 2.0 protected mode information block
//

typedef struct {
   short setWindow;              /* offset of set window call */
   short setDisplayStart;        /* offset of set display start call */
   short setPalette;             /* offset of set primary palette */
   short IOPrivInfo;             /* offset of I/O,memory priveledge info */
// code and tables located here
} VBE_pmInfo;

//
// The VBE 2.0 palette format (the same as used by Windows but different
// from the one used by normal VGA BIOS)
//

typedef struct {
	uchar	blue;	                  /* Blue component of color	 */
	uchar	green;                  /* Green component of color */
	uchar	red;	                  /* Blue component of color	 */
	uchar	alpha;	               /* Alpha or alignment byte	 */
} VBE_palette;

#pragma pack()

//
// Next we have the defines for the bit fields in some of the above fields
//

// Capabilities field flags

#define VBE_8_BIT_DAC         0x000000001 // DAC width can be made 8 bit
// 2.0 extensions 
#define VBE_NON_VGA           0x000000002 // Controller is non-VGA
#define VBE_BLANK_RAM_DAC     0x000000004 // Program DAC with blank bit

// First the mode attributes (bit 1 reserved)

#define VBE_MODE_SUPPORTED    0x000000001 // video mode is supported
#define VBE_BIOS_TTY_OUTPUT   0x000000004 // tty output supported by BIOS
#define VBE_COLOR_MODE        0x000000008 // mode is a color mode
#define VBE_GRAPHICS_MODE     0x000000010 // mode is a graphics mode
// 2.0 extensions
#define VBE_NON_VGA_MODE      0x000000020 // mode is not VGA compatible
#define VBE_NON_BANK_MODE     0x000000040 // mode does not support banking
#define VBE_MODE_LINEAR       0x000000080 // linear mode is supported

// WinA and WinB attributes

#define VBE_WIN_SUPPORTED     0x000000001
#define VBE_WIN_READABLE      0x000000002
#define VBE_WIN_WRITABLE      0x000000003
#define VBE_WIN_ALL_ATTR      0x000000007 // can read/write window

// Direct color mode info

#define VBE_RAMP_CHANGES      0x000000001 // indicates color ramp is programmable
#define VBE_COLOR_RSVD_AVAIL  0x000000002 // indicates that rsvd bits avail

typedef enum {
   vbeMemTXT = 0,                         // Text mode memory model
   vbeMemCGA = 1,                         // CGA style mode
   vbeMemHGC = 2,                         // Hercules graphics style mode
   vbeMemPL  = 3,                         // 16 color VGA style planar mode
   vbeMemPK  = 4,                         // Packed pixel mode
   vbeMemX   = 5,                         // Non-chain 4, 256 color (ModeX)
   vbeMemRGB = 6,                         // Direct color RGB mode
   vbeMemYUV = 7                          // Direct color YUV mode
} VBE_memModels;

#define VESA_ID_MAJ           0x000000001
#define VESA_ID_MIN           0x000000000

// Lastly some defines for the flags in SetVBEMode since we must have
// backword compatibility.

#define SET_MODE_CLEAR        0x000000001 // same as TRUE
#define SET_MODE_SAVE         0x000000000 // same as FALSE
#define SET_MODE_LIN_CLEAR    0x000000003
#define SET_MODE_LIN_SAVE     0x000000002

#define SET_MODE_CLEAR_BIT    0x000000001
#define SET_MODE_LIN_BIT      0x000000002

//
// Now for the function prototypes and externs
//

extern short VBE_version;
extern bool  VBE_modeLinear;

struct grs_sys_info;

int   VBEInit(struct grs_sys_info *info);
int   VBESetScanLineLength(int w,int *hp);
int   VBEGetScreenHeight();
int   VBEGetRowBytes();
int   VBESetPaletteDac(int dacWidth);
int   VBEGetPaletteDac();
void  VBEForceDefaultDac();

void  vbe_close(struct grs_sys_info *info);
int   vbe_set_mode(int mode,int flags);
int   vbe_set_width(int w);
int   vbe_get_width();
void  vbe_set_palette(int start,int n,uchar *palData);
void  vbe_get_palette(int start,int n,uchar *palData);
void  vbe_set_focus(int x,int y);
void  vbe_get_focus(int *xPtr,int *yPtr);

#ifdef __cplusplus
};
#endif
#endif // __VESA_H

