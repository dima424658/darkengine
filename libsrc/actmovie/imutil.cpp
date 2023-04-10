///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/actmovie/RCS/imutil.cpp $
// $Author: TOML $
// $Date: 1996/12/13 15:57:38 $
// $Revision: 1.7 $
//

#ifdef _WIN32

#include <imutil.h>
#include <lgassert.h>
#include <lglog.h>
#include <limits.h>
#include <uuids.h>
#include <astring.h>
#include <mprintf.h>

// These are bit field masks for true colour devices

const DWORD bits555[] = {0x007C00,0x0003E0,0x00001F};
const DWORD bits565[] = {0x00F800,0x0007E0,0x00001F};
const DWORD bits888[] = {0xFF0000,0x00FF00,0x0000FF};

// We assume throughout this code that any bitfields masks are allowed no
// more than eight bits to store a colour component. This checks that the
// bit count assumption is enforced and also makes sure that all the bits
// set are contiguous. We return a boolean TRUE if the field checks out ok

BOOL CheckBitFields(const VIDEOINFO *pInput)
{
    DWORD *pBitFields = (DWORD *) pInput->dwBitMasks;

    for (INT iColour = iRED;iColour <= iBLUE;iColour++) {

        // First of all work out how many bits are set

        DWORD SetBits = CountSetBits(pBitFields[iColour]);
        if (SetBits > iMAXBITS || SetBits == 0) {
            LogMsg1("Bit fields for component %d invalid",iColour);
            return FALSE;
        }

        // Next work out the number of zero bits prefix
        DWORD PrefixBits = CountPrefixBits(pBitFields[iColour]);

        // This is going to see if all the bits set are contiguous (as they
        // should be). We know how much to shift them right by from the
        // count of prefix bits. The number of bits set defines a mask, we
        // invert this (ones complement) and AND it with the shifted bit
        // fields. If the result is NON zero then there are bit(s) sticking
        // out the left hand end which means they are not contiguous

        DWORD TestField = pBitFields[iColour] >> PrefixBits;
        DWORD Mask = ULONG_MAX << SetBits;
        if (TestField & Mask) {
            LogMsg1("Bit fields for component %d not contiguous",iColour);
            return FALSE;
        }
    }
    return TRUE;
}


// This counts the number of bits set in the input field

DWORD CountSetBits(const DWORD Field)
{
    // This is a relatively well known bit counting algorithm

    DWORD Count = 0;
    DWORD init = Field;

    // Until the input is exhausted, count the number of bits

    while (init) {
        init = init & (init - 1);  // Turn off the bottommost bit
        Count++;
    }
    return Count;
}


// This counts the number of zero bits upto the first one set NOTE the input
// field should have been previously checked to ensure there is at least one
// set although if we don't find one set we return the impossible value 32

DWORD CountPrefixBits(const DWORD Field)
{
    DWORD Mask = 1;
    DWORD Count = 0;

    while (TRUE) {
        if (Field & Mask) {
            return Count;
        }
        Count++;

        Assert_(Mask != 0x80000000);
        if (Mask == 0x80000000) {
            return Count;
        }
        Mask <<= 1;
    }
}


// This is called to check the BITMAPINFOHEADER for the input type. There are
// many implicit dependancies between the fields in a header structure which
// if we validate now make for easier manipulation in subsequent handling. We
// also check that the BITMAPINFOHEADER matches it's specification such that
// fields likes the number of planes is one, that it's structure size is set
// correctly and that the bitmap dimensions have not been set as negative

BOOL CheckHeaderValidity(const VIDEOINFO *pInput)
{
    // Check the bitmap dimensions are not negative

    if (pInput->bmiHeader.biWidth <= 0 || pInput->bmiHeader.biHeight <= 0) {
        LogMsg("Invalid bitmap dimensions");
        return FALSE;
    }

    // Check the compression is either BI_RGB or BI_BITFIELDS

    if (pInput->bmiHeader.biCompression != BI_RGB) {
        if (pInput->bmiHeader.biCompression != BI_BITFIELDS) {
            LogMsg("Invalid compression format");
            return FALSE;
        }
    }

    // If BI_BITFIELDS compression format check the colour depth

    if (pInput->bmiHeader.biCompression == BI_BITFIELDS) {
        if (pInput->bmiHeader.biBitCount != 16) {
            if (pInput->bmiHeader.biBitCount != 32) {
                LogMsg("BI_BITFIELDS not 16/32 bit depth");
                return FALSE;
            }
        }
    }

    // Check the assumptions about the layout of the bit fields

    if (pInput->bmiHeader.biCompression == BI_BITFIELDS) {
        if (CheckBitFields(pInput) == FALSE) {
            LogMsg("Bit fields are not valid");
            return FALSE;
        }
    }

    // Are the number of planes equal to one

    if (pInput->bmiHeader.biPlanes != 1) {
        LogMsg("Number of planes not one");
        return FALSE;
    }

    // Check the image size is consistent (it can be zero)

    if (pInput->bmiHeader.biSizeImage != GetBitmapSize(&pInput->bmiHeader)) {
        if (pInput->bmiHeader.biSizeImage) {
            LogMsg("Image size incorrectly set");
            return FALSE;
        }
    }

    // Check the size of the structure

    if (pInput->bmiHeader.biSize != sizeof(BITMAPINFOHEADER)) {
        LogMsg("Size of BITMAPINFOHEADER wrong");
        return FALSE;
    }
    return CheckPaletteHeader(pInput);
}


// This runs a few simple tests against the palette fields in the input to
// see if it looks vaguely correct. The tests look at the number of palette
// colours present, the number considered important and the biCompression
// field which should always be BI_RGB as no other formats are meaningful

BOOL CheckPaletteHeader(const VIDEOINFO *pInput)
{
    // The checks here are for palettised videos only

    if (PALETTISED(pInput) == FALSE) {
        if (pInput->bmiHeader.biClrUsed) {
            LogMsg("Invalid palette entries");
            return FALSE;
        }
        return TRUE;
    }

    // Compression type of BI_BITFIELDS is meaningless for palette video

    if (pInput->bmiHeader.biCompression != BI_RGB) {
        LogMsg("Palettised video must be BI_RGB");
        return FALSE;
    }

    // Check the number of palette colours is correct

    if (pInput->bmiHeader.biClrUsed > PALETTE_ENTRIES(pInput)) {
        LogMsg("Too many colours in palette");
        return FALSE;
    }

    // The number of important colours shouldn't exceed the number used

    if (pInput->bmiHeader.biClrImportant > pInput->bmiHeader.biClrUsed) {
        LogMsg("Too many important colours");
        return FALSE;
    }
    return TRUE;
}

// Initialise the optional fields in a VIDEOINFO. These are mainly to do with
// the source and destination rectangles and palette information such as the
// number of colours present. It simplifies our code just a little if we don't
// have to keep checking for all the different valid permutations in a header
// every time we want to do anything with it (an example would be creating a
// palette). We set the base class media type before calling this function so
// that the media types between the pins match after a connection is made

HRESULT UpdateFormat(VIDEOINFO *pVideoInfo)
{
    Assert_(pVideoInfo);

    BITMAPINFOHEADER *pbmi = HEADER(pVideoInfo);
    SetRectEmpty(&pVideoInfo->rcSource);
    SetRectEmpty(&pVideoInfo->rcTarget);

    // Set the number of colours explicitly

    if (PALETTISED(pVideoInfo)) {
        if (pVideoInfo->bmiHeader.biClrUsed == 0) {
            pVideoInfo->bmiHeader.biClrUsed = PALETTE_ENTRIES(pVideoInfo);
        }
    }

    // The number of important colours shouldn't exceed the number used, on
    // some displays the number of important colours is not initialised when
    // retrieving the display type so we set the colours used correctly

    if (pVideoInfo->bmiHeader.biClrImportant > pVideoInfo->bmiHeader.biClrUsed) {
        pVideoInfo->bmiHeader.biClrImportant = PALETTE_ENTRIES(pVideoInfo);
    }

    // Change the image size field to be explicit

    if (pVideoInfo->bmiHeader.biSizeImage == 0) {
        pVideoInfo->bmiHeader.biSizeImage = GetBitmapSize(&pVideoInfo->bmiHeader);
    }
    return NOERROR;
}

// Return the bit masks for the true colour VIDEOINFO provided

const DWORD *GetBitMasks(const VIDEOINFO *pVideoInfo)
{
    static DWORD FailMasks[] = {0,0,0};

    if (pVideoInfo->bmiHeader.biCompression == BI_BITFIELDS) {
        return pVideoInfo->dwBitMasks;
    }

    Assert_(pVideoInfo->bmiHeader.biCompression == BI_RGB);

    switch (pVideoInfo->bmiHeader.biBitCount) {
        case 16: return bits555;
        case 24: return bits888;
        case 32: return bits888;
        default: return FailMasks;
    }
}

DWORD GetBitmapSize(const BITMAPINFOHEADER *pHeader)
{
    return DIBSIZE(*pHeader);
}

// This maps bitmap subtypes into a bits per pixel value and also a name

const struct {
    const GUID *pSubtype;
    WORD BitCount;
    TCHAR *pName;
} BitCountMap[] = { &MEDIASUBTYPE_RGB1,        1,   TEXT("RGB Monochrome"),
                    &MEDIASUBTYPE_RGB4,        4,   TEXT("RGB VGA"),
                    &MEDIASUBTYPE_RGB8,        8,   TEXT("RGB 8"),
                    &MEDIASUBTYPE_RGB565,      16,  TEXT("RGB 565 (16 bit)"),
                    &MEDIASUBTYPE_RGB555,      16,  TEXT("RGB 555 (16 bit)"),
                    &MEDIASUBTYPE_RGB24,       24,  TEXT("RGB 24"),
                    &MEDIASUBTYPE_RGB32,       32,  TEXT("RGB 32"),
                    &MEDIASUBTYPE_Overlay,     0,   TEXT("Overlay"),
                    &GUID_NULL,                0,   TEXT("UNKNOWN") };


// Given a video bitmap subtype we return the number of bits per pixel it uses
// We return a WORD bit count as thats what the BITMAPINFOHEADER uses. If the
// GUID subtype is not found in the table we return an invalid USHRT_MAX

WORD GetBitCount(const GUID *pSubtype)
{
    Assert_(pSubtype);
    const GUID *pMediaSubtype;
    INT iPosition = 0;

    // Scan the mapping list seeing if the source GUID matches any known
    // bitmap subtypes, the list is terminated by a GUID_NULL entry

    while (TRUE) {
        pMediaSubtype = BitCountMap[iPosition].pSubtype;
        if (IsEqualGUID(*pMediaSubtype,GUID_NULL)) {
            return USHRT_MAX;
        }
        if (IsEqualGUID(*pMediaSubtype,*pSubtype)) {
            return BitCountMap[iPosition].BitCount;
        }
        iPosition++;
    }
}

uchar BitmapTypeFromBMIHeader(const BITMAPINFOHEADER *pBMIHeader)
{
   switch (pBMIHeader->biBitCount)
   {
      case 8:
         return BMT_FLAT8;
      case 16:
         return BMT_FLAT16;
      default:
         LogMsg("BitmapTypeFromBMIHeader: unsupported type");
         return 0;
   }
}

uint BitmapFlagsFromBMIHeader(const BITMAPINFOHEADER *pBMIHeader)
{
   switch (pBMIHeader->biBitCount)
   {
      case 16:
         return BMF_RGB_565;
      default:
         LogMsg("BitmapFlagsFromBMIHeader: unsupported type");
         return 0;
   }
}

DWORD gr_bitmap_type_count(grs_bitmap *bitmap)
{
   switch(bitmap->type)
   {
      case BMT_MONO:
         return 1;
      case BMT_FLAT8: case BMT_BANK8: case BMT_RSD8 : case BMT_TLUC8:
         return 8;
      case BMT_BANK16: case BMT_FLAT16:
         return 16;
      case BMT_BANK24: case BMT_FLAT24:
         return 24;
      default:
         LogMsg("Warning: unrecognised type");
         return 0;
   }
}

void AMBlitFlipped16to16  (BYTE * pSource, int widthSource, int heightSource,
                           BYTE * pDest,   int widthDest,   int heightDest,   int rowBytesDest,
                           int xDest, int yDest)
{
    // Adjust widths for depth
    widthSource *= 2;
    widthDest   *= 2;
    xDest       *= 2;

    // Clip
    const unsigned widthBlit  = max(min(widthDest  - xDest, widthSource),  0);
          unsigned heightBlit = max(min(heightDest - yDest, heightSource), 0);

    // Start at bottom in source, (x, y) in dest
    pSource += (heightBlit - 1) * widthSource;
    pDest += (yDest * widthDest) + xDest;

    while (heightBlit)
    {
        memcpya((void *) pDest,
                (void *) pSource,
                (int)    widthBlit,
                (void *) pDest);
        heightBlit--;
        pSource -= widthSource;
        pDest += rowBytesDest;
    }

}

void AMBlitFlipped16to16x2(BYTE * pSource, int widthSource, int heightSource,
                           BYTE * pDest,   int widthDest,   int heightDest,   int rowBytesDest,
                           int xDest, int yDest, uchar flags)
{
    BYTE expandBuf[1280*2];

    // Adjust widths for depth
    widthSource *= 2;
    widthDest   *= 2;
    xDest       *= 2;

    // Clip (broken for height right now (toml 10-01-96))
    const unsigned widthBlit  = max(min(widthDest  - xDest, widthSource * 2),  0);
          unsigned heightBlit = max(min(heightDest - yDest, heightSource), 0);

    // Start at bottom in source, (x, y) in dest
    pSource += (heightBlit - 1) * widthSource;
    pDest += (yDest * widthDest) + xDest;

    while (heightBlit)
    {
        // Horizontal double
        {
        register WORD *  pDoubleScan;
        register WORD *  pDoubleLimit;
        register DWORD * pDoubledPixel;
        register DWORD   dwPixel;

        pDoubledPixel = (DWORD *) expandBuf;
        pDoubleScan = (WORD *) pSource;
        pDoubleLimit = (WORD *) pSource + widthSource;

        while (pDoubleScan < pDoubleLimit)
        {
            dwPixel = *pDoubleScan;
            *pDoubledPixel = dwPixel | (dwPixel << 16);
            pDoubleScan++;
            pDoubledPixel++;
        }
        }

        // Vertical double
        memcpya((void *) pDest,
                (void *) expandBuf,
                (int)    widthBlit,
                (void *) pDest);

        pDest += rowBytesDest;

        if (!(flags & AM_BLIT_V_GRATE))
        {
            memcpya((void *) pDest,
                    (void *) expandBuf,
                    (int)    widthBlit,
                    (void *) pDest);
        }

        heightBlit--;
        pSource -= widthSource;
        pDest += rowBytesDest;
    }

}

#define SRC_I(x, y) (&Src[(x)*SrcW*PixelSize+(y)*PixelSize])
#define TGT_I(x, y) (&Tgt[(x)*TgtW*PixelSize+(y)*PixelSize])

void AMBlit(BYTE *Src, ULONG SrcW, ULONG SrcH,
            BYTE *Tgt, ULONG TgtW, ULONG TgtH, int rowBytesDest,
            RECT *TgtRect, uchar bitDepth, uchar flags)
{
   ULONG expand = max(1,
                min((TgtRect->right-TgtRect->left+1)/SrcW,
                (TgtRect->bottom-TgtRect->top+1)/SrcH));

   if (bitDepth == 16 && (flags & AM_BLIT_V_FLIP))
   {
        if (expand == 2)
        {
            AMBlitFlipped16to16x2(Src, (int)SrcW, (int)SrcH,
                                  Tgt, (int)TgtW, (int)TgtH, rowBytesDest,
                                  TgtRect->left, TgtRect->top, flags);
            return;
        }
        else if (expand == 1)
        {
            AMBlitFlipped16to16(Src, (int)SrcW, (int)SrcH,
                                Tgt, (int)TgtW, (int)TgtH, rowBytesDest,
                                TgtRect->left, TgtRect->top);
            return;
        }
   }

   LONG PixelSize;      // size of a pixel in bytes
   ULONG TgtInc;        // increment for target in bytes (based on expand)
   LONG SrcRowInc;      // increment for source/row in bytes
   BYTE *pSrc, *pTgt;   // pointers to source/target data
   BYTE *pSrcLastRow;   // pointer to the last source row we process
   BYTE *pSrcRowStart;  // pointer to the start of current source row
   ULONG TgtRow;        // index to current target row
   ULONG TgtRowSize;    // size of target row in bytes
   int i, j, k;
   bool finished = FALSE;  // TRUE if we're on the last source row

   PixelSize = bitDepth/8;

   if (flags&AM_BLIT_V_FLIP)
   {
      // start at the top of the source
      pSrc = SRC_I(SrcH-1, 0);
      SrcRowInc = -SrcW*PixelSize;
      pSrcLastRow = SRC_I(0, 0);
   }
   else
   {
      pSrc = SRC_I(0, 0);
      SrcRowInc = SrcW*PixelSize;
      pSrcLastRow = SRC_I(SrcH-1, 0);
   }
   TgtInc = PixelSize;
   TgtRow = TgtRect->top;
   TgtRowSize = (TgtRect->right-TgtRect->left+1)*expand*PixelSize;
   while (!finished)
   {
      finished = (pSrc == pSrcLastRow);
      pSrcRowStart = pSrc;
      pTgt = TGT_I(TgtRow, TgtRect->left);
      for (i=0; i<SrcW; i++)
      {
         for (j=0; j<expand; j++)
         {
            for (k=0; k<PixelSize; k++)
            {
               pTgt[k] = pSrc[k];
            }
            pTgt += TgtInc;
         }
         pSrc += PixelSize;
      }
      if (!(flags&AM_BLIT_V_GRATE))
         for (j=0; j<expand-1; j++)
         {
            memcpya((void*)TGT_I(TgtRow+j+1, TgtRect->left),
                    (void*)TGT_I(TgtRow, TgtRect->left),
                    (int)TgtRowSize,
                    (void*)TGT_I(TgtRow, TgtRect->left));
         }
      pSrc = pSrcRowStart+SrcRowInc;
      TgtRow += expand;
   }
}


#endif
