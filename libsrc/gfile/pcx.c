//	PCX.C		Reads & process .PCX files
//	Rex E. Bradford (REX)
//
//	This file implements a .PCX file reader.  The Gfile access routines
//	call the function: PcxRead() in this file.

/*
* $Header: x:/prj/tech/libsrc/gfile/RCS/pcx.c 1.9 1997/08/15 10:31:19 JAEMZ Exp $
* $Log: pcx.c $
 * Revision 1.9  1997/08/15  10:31:19  JAEMZ
 * *** empty log message ***
 * 
 * Revision 1.8  1996/05/09  12:22:34  TONY
 * Ignore manufacturer field when reading, Windows 95 Paint can
 * put unspec'ed values there.
 * 
 * Revision 1.7  1994/09/01  18:05:56  rex
 * Fixed bug related to when rowbytes is set
 * 
 * Revision 1.6  1994/08/22  17:40:02  rex
 * Added 24/bit support
 * 
 * Revision 1.5  1994/02/01  12:17:57  rex
 * Added better warnings and hack to fix a single stupid pcx file with odd width
 * 
 * Revision 1.4  1993/10/27  20:15:39  jak
 * pcx code sets pcxheader's 'palette_type' to 2, which makes pstyler think
 * the file is greyscale when it's not.  So now it sets it to 0.
 * 
 * Revision 1.3  1993/10/19  15:12:08  xemu
 * got rid of warnigns
 * 
 * Revision 1.2  1993/10/19  15:09:27  xemu
 * added writiing stuff, changed the structure names some
 * 
 * Revision 1.1  1993/09/27  16:38:52  rex
 * Initial revision
 * 
*/

#include <string.h>

#include <stdlib.h>
#include <stdio.h>
#include <lg.h>
#include <gfile.h>

#define  RGB_RED     0
#define  RGB_GREEN   1
#define  RGB_BLUE 2
#define  RGB_SIZE 3

typedef unsigned long 	rgb;

rgb get_palrgb(uchar *paldata,int n);
rgb get_rgb(uchar *paldata, uchar *data,int xres, uint x,uint y);

/* Write a 24 bit PCX file, based on code in Supercharged Bitmap Graphics */
/* or write an 8 bit PCX file */

typedef struct {
   char manufacturer;
   char version;
   char encoding;              
   char bits_per_pixel;       // better be 8 for us
   short int xmin,ymin;       // usually zero unless zany
   short int xmax,ymax;       // xmax-xmin+1 = w, ymax-ymin+1 = h
   short int hres;            // unreliable      
   short int vres;            // unreliable
   char palette[48];
   char reserved;
   char colour_planes;        // 3 for rgb, 1 for 8 bit
   short int bytes_per_line;  // use this for width when unpacking
   short int palette_type;    // everyone punts this, ignore, should mean monochrome or color
   char filler[58];
}  PCXHEAD;

typedef PCXHEAD PcxHeader;

#define READBUFF_SIZE 4096
static uchar *readBuff;
static short readBuffIndex;

int _writepcxline(uchar *p,FILE *fp,int bytes);

bool PcxReadHeader(FILE *fp, PcxHeader *phead);
void PcxReadPall(FILE *fp, GfileInfo *pgfi);
bool PcxReadBody(FILE *fp, grs_bitmap *pbm);
bool PcxReadRow(FILE *fp, uchar *p, short npixels);

//	-------------------------------------------------------
//		READ ROUTINE
//	-------------------------------------------------------
//
//	PcxRead() reads in a PCX file.

bool PcxRead(GfileInfo *pgfi, FILE *fp)
{
	PcxHeader pcxHeader;

//	Read header, if not valid PCX file return

	if (!PcxReadHeader(fp, &pcxHeader))
		return FALSE;

//	No palette so far

	pgfi->ppall = NULL;

//	Fill in bitmap

	pgfi->bm.align = 0;
	pgfi->bm.flags = 0;
	pgfi->bm.w = (pcxHeader.xmax - pcxHeader.xmin) + 1;
	pgfi->bm.h = (pcxHeader.ymax - pcxHeader.ymin) + 1;
	if (pcxHeader.colour_planes == 1)
		{
		pgfi->bm.type = BMT_FLAT8;
		pgfi->bm.row = pgfi->bm.w;
		}
	else if (pcxHeader.colour_planes == 3)
		{
		pgfi->bm.type = BMT_FLAT24;
		pgfi->bm.row = pgfi->bm.w * 3;
		}
	else
		{
		Warning(("PCX file has %d planes!\n", pcxHeader.colour_planes));
		return FALSE;
		}

//	Allocate bitmap bits

	pgfi->bm.bits = Malloc((long) pgfi->bm.row * (long) pgfi->bm.h);
	if (pgfi->bm.bits == NULL)
		{
		Warning(("Can't allocate bitmap bits in pcx reader\n"));
		goto BADRET;
		}

//	Allocate palette and read it

	if (pcxHeader.colour_planes == 1)
		{
		pgfi->ppall = Malloc(256 * 3);
		if (pgfi->ppall == NULL)
			{
			Warning(("Can't allocate palette in pcx reader\n"));
			goto BADRET;
			}
		PcxReadPall(fp, pgfi);
		}

//	Allocate read buffer

	readBuff = Malloc(READBUFF_SIZE);
	if (readBuff == NULL)
		{
		Warning(("Can't allocate buffer in pcx reader\n"));
		goto BADRET;
		}
	readBuffIndex = READBUFF_SIZE;

//	Read body and return

	if (!PcxReadBody(fp, &pgfi->bm))
		{
		Warning(("Unable to read PCX file!\n"));
		Free(readBuff);
		goto BADRET;
		}
	else
		{
		Free(readBuff);
		return TRUE;
		}

//	If error, free up properly

BADRET:

	GfileFree(pgfi);
	return FALSE;
}

//	-----------------------------------------------------------------
//		WRITE ROUTINE
//	-----------------------------------------------------------------

/* Writes a 24 bit image for 24 bit images and an 8 bit for 8 bit */
bool PcxWrite(GfileInfo *pgfi, FILE *fp)
{
// (char *pathname,Image *im)
   PCXHEAD pcx;
   int i,bytes;
   uchar *p;
//   uchar r,g,b;

   pcx.manufacturer = 10;
   pcx.version = 5;
   pcx.encoding = 1;
   pcx.bits_per_pixel = 8;
   pcx.xmin = 0;
   pcx.ymin = 0;
   pcx.xmax = pgfi->bm.w - 1;
   pcx.ymax = pgfi->bm.h - 1;
   pcx.hres = pcx.xmax - pcx.xmin + 1;    
   pcx.vres = pcx.ymax - pcx.ymin + 1;
   pcx.colour_planes = (pgfi->bm.type == BMT_FLAT8) ? 1 : 3;
   pcx.bytes_per_line = pcx.hres;
   pcx.palette_type = 0;

   fwrite(&pcx,1,sizeof(PCXHEAD),fp);

   bytes=pcx.bytes_per_line;

   if (pcx.colour_planes==3) {
      /* We know this is an RGB image, 24 bit */
      Warning(("Hey!  Gfile doesn't know from 24 bit color!"));
   }
   else {
      /* This is an 8 bit image */
      /* Good thing too, since we can't do 24 bit right.  Fortunately, neither can the read
         part of gfile -- Rob F. 10/18/93 */
      p = pgfi->bm.bits;
//      p = &(im->data[0]);

      for(i=0;i<pcx.vres;++i) {
         if(_writepcxline(p,fp,bytes) != bytes) {
            return FALSE;
         }
         p += pcx.hres;
      }

      fputc(12,fp); // flag for people to know the palette is next

      fwrite(pgfi->ppall, 256 * 3, 1, fp);
   }

   fclose(fp);
   return TRUE;
}

//	--------------------------------------------------------
//		INTERNAL ROUTINES
//	--------------------------------------------------------
//
//	PcxReadHeader() reads the header from a PCX file.

#define IGNORE_MANUFACTURER      // Windows 95 Paint writes an 'illegal'
                                 // value here...

bool PcxReadHeader(FILE *fp, PcxHeader *phead)
{
	fseek(fp, 0L, SEEK_SET);
	fread(phead, sizeof(PcxHeader), 1, fp);

#ifndef IGNORE_MANUFACTURER
	if (phead->manufacturer != 10)
		{
		Warning(("Bad PCX header: Manufacturer id\n"));
		return FALSE;
		}
#endif
	if (phead->version != 5)
		{
		Warning(("Bad PCX header: not version 3.0\n"));
		return FALSE;
		}
	if (phead->encoding != 1)
		{
		Warning(("Bad PCX header: not compressed\n"));
		return FALSE;
		}

	if ((phead->xmin != 0) || (phead->ymin != 0))
		{
		Warning(("Bad PCX header: picture not at 0,0\n"));
		return FALSE;
		}
	if (phead->bits_per_pixel != 8)
		{
		Warning(("PCX file must have 8 bits per pixel\n"));
		return FALSE;
		}

	return TRUE;
}

//	-------------------------------------------------------
//
//	PcxReadPall() reads in the pallette.

void PcxReadPall(FILE *fp, GfileInfo *pgfi)
{
	uchar pallPresent;

	fseek(fp, -769L, SEEK_END);
	fread(&pallPresent, 1, 1, fp);
	if (pallPresent == 12)
		fread(pgfi->ppall, 256 * 3, 1, fp);
	else
		Warning(("Bad PCX trailer: pallette not found\n"));
}

//	-------------------------------------------------------
//
//	PcxReadBody() reads in the body.

static short readBuffIndex;

bool PcxReadBody(FILE *fp, grs_bitmap *pbm)
{
	uchar *p,*buff24,*pd;
	int y,i,plane;

//	Seek to start of color data

	fseek(fp, 128L, SEEK_SET);

//	If 24-bit, alloc buffer

	if (pbm->type == BMT_FLAT24)
		{
		buff24 = Malloc(pbm->row);
		if (buff24 == NULL)
			{
			Warning(("PCX reader can't alloc memory\n"));
			return FALSE;
			}
		}

//	Iterate across rows

	p = pbm->bits;
	for (y = 0; y < pbm->h; y++)
		{

//	If 8-bit, read row directly into bitmap

		if (pbm->type == BMT_FLAT8)
			{
			if (!PcxReadRow(fp, p, pbm->w))
				{
				Warning(("PCX decompression error on line: %d\n", y));
				return FALSE;
				}
			}

//	If 24-bit, read 3 planes into row buffer, then interleave into bitmap

		else
			{
			for (plane = 0; plane < 3; plane++)
				{
				if (!PcxReadRow(fp, buff24 + (plane * pbm->w), pbm->w))
					{
					Warning(("PCX decompression error on line: %d\n", y));
					return FALSE;
					}
				}
			for (i = 0, pd = p; i < pbm->w; i++)
				{
				*pd++ = buff24[i];
				*pd++ = buff24[pbm->w + i];
				*pd++ = buff24[(pbm->w * 2) + i];
				}
			}

//	Advance bitmap ptr to next row

		p += pbm->row;
		}

//	If 24-bit, free row buffer

	if (pbm->type == BMT_FLAT24)
		Free(buff24);

	return TRUE;
}

//	-------------------------------------------------
//
//	PcxReadRow() reads a compressed row.

bool PcxReadRow(FILE *fp, uchar *p, short npixels)
{
	uchar count,value;
	uchar *pstart;
	uchar *pend;
	
	pstart = p;
	pend = (pstart + npixels);

	while (p < pend)
		{
		if (readBuffIndex >= READBUFF_SIZE)
			{
			fread(readBuff, READBUFF_SIZE, 1, fp);
			readBuffIndex = 0;
			}
		value = readBuff[readBuffIndex++];
		if ((value & 0xC0) != 0xC0)
			{
			*p++ = value;
			}
		else
			{
			count = value & 0x3F;
			if (readBuffIndex >= READBUFF_SIZE)
				{
				fread(readBuff, READBUFF_SIZE, 1, fp);
				readBuffIndex = 0;
				}
			value = readBuff[readBuffIndex++];
			if ((p + count) > pend)
				{
				// Hack to make some stupid pcx file work.  Is DeluxePaint
				// broken, or is this a feature of the pcx format?
				if (((p + count) == (pend + 1)) && (npixels & 1))
					{
					memset(p, value, count - 1);
					p += count - 1;
					}
				else
					{
					Warning(("PCX decompression error, xpixel: %d\n", p - pstart));
					return FALSE;
					}
				}
			else
				{
				memset(p, value, count);
				p += count;
				}
			}
		}

	return TRUE;
}


/* write and encode into a PCX line */
int _writepcxline(uchar *p,FILE *fp,int bytes)
{
   int n=0,i;
   uchar c;

   do {
      c = p[n++];
      i = 1;
      while ( (c == p[n]) && (i<0x3f) && (n<bytes) ) {
         ++i;
         ++n;
      }
      if ( (i>1) || (c>0x3f) ) {
         fputc(0xc0 + i,fp);
      }
      fputc(c,fp);
   } while(n < bytes);
   return(n);
}

