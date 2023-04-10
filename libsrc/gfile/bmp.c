//		BMP.C		Reads Microsoft Windows 3.x .BMP bitmap file
//		Rex E. Bradford (REX)
//
/*
* $Header: x:/prj/tech/libsrc/gfile/RCS/bmp.c 1.2 1996/11/14 10:59:39 cnorden Exp $
* $Log: bmp.c $
 * Revision 1.2  1996/11/14  10:59:39  cnorden
 * fixes flipped .bmp problem
 * 
 * Revision 1.1  1993/10/07  09:44:35  rex
 * Initial revision
 * 
*/

#include <string.h>

#include <lg.h>
#include <gfile.h>

typedef struct {
	ushort bfType;			// "BM"
	ulong bfSize;			// size in longwords of file
	ushort bfReserved1;	// zero
	ushort bfReserved2;	// zero
	ulong bmOffBits;		// byte offset after header where image begins
} BmpHead;

typedef struct {
	ulong biSize;				// size of this header (40)
	ulong biWidth;				// image width in pixels
	ulong biHeight;			// image height in pixels
	ushort biPlanes;			// num image planes (1)
	ushort biBitCount;		// bits per pixel (1/4/8/24)
	ulong biCompression;		// compression type
	ulong biSizeImage;		// size in bytes of compressed image
	ulong biXPelsPerMeter;	// horz resolution
	ulong biYPelsPerMeter;	// vert resolution
	ulong biClrUsed;			// number of colors used
	ulong biClrImportant;	// number of "important" colors
} BmpInfo;

static bool BmpReadImage(GfileInfo *pgfi, BmpInfo *pInfo, FILE *fp);

//	------------------------------------------------------
//
//	BmpRead() reads in a .BMP file.

bool BmpRead(GfileInfo *pgfi, FILE *fp)
{
	BmpHead bmpHead;
	BmpInfo bmpInfo;
	int numPall,i;
	uchar *spall,*dpall;
	uchar pall[4 * 256];

//	Read header, check for error

	fread(&bmpHead, sizeof(bmpHead), 1, fp);
	if (bmpHead.bfType != 0x4D42)
		{
		Warning(("BmpRead: not a Windows 3.x bitmap file\n"));
		return FALSE;
		}

//	Read info, check for error

	fread(&bmpInfo, sizeof(bmpInfo), 1, fp);
	if ((bmpInfo.biSize != 40) || (bmpInfo.biPlanes != 1))
		{
		Warning(("BmpRead: bad bitmap info struct\n"));
		return FALSE;
		}
	if ((bmpInfo.biBitCount != 4) && (bmpInfo.biBitCount != 8))
		{
		Warning(("BmpRead: can't read file with %d bits per pixel\n",
			bmpInfo.biBitCount));
		return FALSE;
		}

//	For now, bail if compressed

	if (bmpInfo.biCompression)
		{
		Warning(("BmpRead: can't do compressed bmp yet\n"));
		return FALSE;
		}

//	Read palette, grab triplets from quadlets

	pgfi->ppall = Calloc(3 * 256);
	if (pgfi->ppall == NULL)
		goto BADRET;
	numPall = bmpInfo.biClrUsed;
	if (numPall == 0)
		numPall = (bmpInfo.biBitCount == 4) ? 16 : 256;
	fread(pall, numPall * 4, 1, fp);
	for (i = 0, dpall = pgfi->ppall, spall = pall; i < numPall;
		i++, spall += 4)
		{
		*dpall++ = *(spall+2);
		*dpall++ = *(spall+1);
		*dpall++ = *(spall+0);
		}

//	Seek to first frame, set up bitmap, read it

	fseek(fp, bmpHead.bmOffBits, SEEK_SET);

	pgfi->bm.type = BMT_FLAT8;
	pgfi->bm.align = 0;
	pgfi->bm.flags = 0;
	pgfi->bm.w = bmpInfo.biWidth;
	pgfi->bm.h = bmpInfo.biHeight;
	pgfi->bm.row = pgfi->bm.w;
	pgfi->bm.bits = Malloc((long) pgfi->bm.row * (long) pgfi->bm.h);
	if (pgfi->bm.bits == NULL)
		goto BADRET;

	if (!BmpReadImage(pgfi, &bmpInfo, fp))
		goto BADRET;

	return TRUE;

//	Error, clean up

BADRET:

	GfileFree(pgfi);
	return FALSE;
}

//	----------------------------------------------------------
//
//	BmpReadImage() reads in the image from the .bmp file

static bool BmpReadImage(GfileInfo *pgfi, BmpInfo *pInfo, FILE *fp)
{
	int x,y,widthBytes;
	uchar *pbits,*p;
	uchar buff[1024];		// FAILS CATASTROPHICALLY IF 4-BIT > 2048 PIXELS WIDE

//	Uncompressed, 4 bits

	if (pInfo->biBitCount == 4)
		{
		widthBytes = (((pgfi->bm.w + 1) / 2) + 3) & 0xFFFC;
		for (y = 0, pbits = pgfi->bm.bits; y < pgfi->bm.h; y++)
			{
			fread(buff, widthBytes, 1, fp);
			for (x = 0, p = pbits; x < pgfi->bm.w; x++)
				{
				if (x & 1)
					*p++ = buff[x >> 1] & 0x0F;
				else
					*p++ = buff[x >> 1] >> 4;
				}
			pbits += pgfi->bm.row;
			}
		}

//	Uncompressed, 8 bits
// flip the image because, well, bmp's are stored upside-down - cnn

	else
		{
		widthBytes = (pgfi->bm.w + 3) & 0xFFFC;
		for (y = 0, pbits = pgfi->bm.bits + pgfi->bm.row * (pgfi->bm.h-1); y < pgfi->bm.h; y++)
			{
			fread(pbits, pgfi->bm.w, 1, fp);
			if (widthBytes - pgfi->bm.w)
				fread(buff, widthBytes - pgfi->bm.w, 1, fp);
			pbits -= pgfi->bm.row;
			}
		}

	return TRUE;
}

//	----------------------------------------------------------
//		INTERNAL ROUTINES
//	----------------------------------------------------------

