///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/actmovie/RCS/imutil.h $
// $Author: TOML $
// $Date: 1996/11/18 12:39:16 $
// $Revision: 1.2 $
//

// Display utility functions stolen from Microsoft

#ifndef __IMUTIL__
#define __IMUTIL__

#include <strmif.h>
#include <amvideo.h>
#include "mtype.h"
#include <dev2d.h>

DWORD CountSetBits(const DWORD Field);
DWORD CountPrefixBits(const DWORD Field);
BOOL CheckBitFields(const VIDEOINFO *pInput);

    // Used to manage BITMAPINFOHEADERs and the display format

BOOL CheckHeaderValidity(const VIDEOINFO *pInput);
BOOL CheckPaletteHeader(const VIDEOINFO *pInput);
BOOL IsPalettised();
WORD GetDisplayDepth();

    // Provide simple video format type checking

HRESULT CheckMediaType(const CMediaType *pmtIn);
HRESULT CheckVideoType(const VIDEOINFO *pInput);
HRESULT UpdateFormat(VIDEOINFO *pVideoInfo);
const DWORD *GetBitMasks(const VIDEOINFO *pVideoInfo);

BOOL GetColourMask(DWORD *pMaskRed,
                   DWORD *pMaskGreen,
                   DWORD *pMaskBlue);

extern const DWORD bits555[3];
extern const DWORD bits565[3];
extern const DWORD bits888[3];

DWORD GetBitmapSize(const BITMAPINFOHEADER *pHeader);
WORD GetBitCount(const GUID *pSubtype);
uchar BitmapTypeFromBMIHeader(const BITMAPINFOHEADER *pHeader);
uint BitmapFlagsFromBMIHeader(const BITMAPINFOHEADER *pHeader);
DWORD gr_bitmap_type_count(grs_bitmap *bitmap);

/*
  Our own blitter because the 2d stretch blit is so slow.
  Will expand the source image by some integral amount in both dimensions
  so as to most nearly match (without exceeding) the target rectangle.
*/

#define AM_BLIT_V_FLIP 0x01     // invert the image vertically
#define AM_BLIT_V_GRATE 0x02    // insert blank horizontal lines

void AMBlit(BYTE *Src, ULONG SrcW, ULONG SrcH,
            BYTE *Tgt, ULONG TgtW, ULONG TgtH, int rowBytesTarget, RECT *TgtRect,
            uchar bitDepth, uchar flags);

#endif // __IMUTIL__

