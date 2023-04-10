//		GIF.C		Reads .GIF files
//		Rex E. Bradford, borrowed heavily from Jaemz' readgif.c in ipw lib
/*
 * $Source: x:/prj/tech/libsrc/gfile/RCS/gif.c $
 * $Revision: 1.5 $
 * $Author: TOML $
 * $Date: 1996/10/16 16:07:57 $
 */

#include <stdio.h>
#include <string.h>

#include <lg.h>
#include <gfile.h>

typedef struct {
	char sig[6];
	ushort screenwidth;
	ushort screendepth;
	uchar flags;
	uchar background;
	uchar aspect;
} GifHeader;

typedef struct {
	ushort left;
	ushort top;
	ushort width;
	ushort depth;
	uchar flags;
} ImageBlock;

static ImageBlock imageBlock;
static uchar *buffer;

static bool unpackimage(FILE *fp, int bits);
static void putextension(FILE *fp);
#define NO_CODE		-1

//	-------------------------------------------------------
//		READ ROUTINE
//	-------------------------------------------------------
//
//	GifRead() reads in a GIF file.

bool GifRead(GfileInfo *pgfi, FILE *fp)
{
	GifHeader gifHeader;
	int sizePall;
	int c;
	bool ret;

//	Read header, if not valid GIF file return

	fread(&gifHeader, 1, sizeof(GifHeader), fp);
	if (memcmp(gifHeader.sig, "GIF", 3))
		return FALSE;

//	Get palette if there is one

	pgfi->ppall = NULL;
	if (gifHeader.flags & 0x80)
		{
		sizePall = (1 << ((gifHeader.flags & 7) + 1));
		pgfi->ppall = Calloc(3 * 256);
		fread(pgfi->ppall, 1, 3 * sizePall, fp);
		}

//	Step through the blocks, looking for 1st image block

	while (((c = fgetc(fp)) == ',') || (c == '!') || (c == 0))
		{
		/* if it's an image block... */
		if (c == ',')
			{
			/* get the start of the image block */
			fread(&imageBlock, 1, sizeof(ImageBlock), fp);

			pgfi->bm.type = BMT_FLAT8;
			pgfi->bm.align = 0;
			pgfi->bm.flags = 0;
			pgfi->bm.w = imageBlock.width;
			pgfi->bm.h = imageBlock.depth;
			pgfi->bm.row = pgfi->bm.w;
			buffer = pgfi->bm.bits = Malloc((long) pgfi->bm.row * (long) pgfi->bm.h);

			/* get the local colour map if there is one */
			if (imageBlock.flags & 0x80)
				{
				sizePall = 1<<((imageBlock.flags & 0x0007) + 1);
				if (pgfi->ppall == NULL)
					pgfi->ppall = Calloc(3 * 256);
				fread(pgfi->ppall, 1, 3 * 256, fp);
				}

			/* get the initial code size */
			if ((c = fgetc(fp)) == EOF)
				goto BADRET;

			/* unpack the image */
			ret = unpackimage(fp,c);
			if (ret)
				return TRUE;
			goto BADRET;
			}
		/* otherwise, it's an extension */
		else if (c == '!')
			putextension(fp);
		}

	return TRUE;

//	If error, free up properly

BADRET:

	GfileFree(pgfi);
	return FALSE;
}

//	------------------------------------------------------------------
//		INTERNAL READER
//	------------------------------------------------------------------
//
//	unpackimage() unpacks an LZW compressed image

static bool unpackimage(FILE *fp,int bits)
{
	short int bits2;  	    /* Bits plus 1 */
	short int codesize;       /* Current code size in bits */
	short int codesize2;      /* Next codesize */
	short int nextcode;       /* Next available table entry */
	short int thiscode;       /* Code being expanded */
	short int oldtoken;       /* Last symbol decoded */
	short int currentcode;    /* Code just read */
	short int oldcode;        /* Code read before this one */
	short int bitsleft;       /* Number of bits left in *p */
	short int blocksize;      /* Bytes in next block */
	short int line=0;	    /* next line to write */
	short int byte=0;         /* next byte to write */
	short int pass=0;	    /* pass number for interlaced pictures */

	uchar *p;	         /* Pointer to current byte in read buffer */
	uchar *q;            /* Pointer past last byte in read buffer */
	uchar b[255];        /* Read buffer */
	uchar *u;            /* Stack pointer into firstcodestack */
	uchar *linebuffer;   /* place to store the current line */

	/* Jesus Christ, allocate these */
	uchar *firstcodestack;  /* Stack for first codes */
	uchar *lastcodestack;   /* Statck for previous code */
	short int *codestack;  /* Stack for links */

	static short int wordmasktable[] = {  \
		0x0000,0x0001,0x0003,0x0007,
		0x000f,0x001f,0x003f,0x007f,
		0x00ff,0x01ff,0x03ff,0x07ff,
		0x0fff,0x1fff,0x3fff,0x7fff};

	static int inctable[] = { 8,8,4,2,0 }; /* interlace increments */
	static int startable[] = { 0,4,2,1,0 };  /* interlace starts */

	/* Allocate code table space */

	firstcodestack = Malloc(4096);
	lastcodestack = Malloc(4096);
	codestack = Malloc(2*4096);

	p=q=b;
	bitsleft = 8;

	if (bits < 2 || bits > 8) return FALSE;
	bits2 = 1 << bits;
	nextcode = bits2 + 2;
	codesize2 = 1 << (codesize = bits + 1);
	oldcode = oldtoken = NO_CODE;

	if ((linebuffer = Malloc(imageBlock.width)) == NULL)
		return FALSE;

	/* loop until something breaks */

	while(1)
		{
		if (bitsleft == 8)
			{
			if (++p >= q &&
			(((blocksize = fgetc(fp)) < 1) ||
			(q= (p=b) + fread(b,1,blocksize,fp)) < (b+blocksize)))
				{
				Free(linebuffer);
				return FALSE;
				}
			bitsleft = 0;
			}

		thiscode = *p;
		if ((currentcode=(codesize+bitsleft)) <= 8)
			{
			*p >>= codesize;
			bitsleft = currentcode;
			}
		else
			{
			if (++p >= q &&
			  (((blocksize = fgetc(fp)) < 1) ||
			  (q=(p=b)+fread(b,1,blocksize,fp)) < (b+blocksize)))
				{
				Free(linebuffer);
				return FALSE;
				}
			thiscode |= *p << (8 - bitsleft);
			if (currentcode <= 16)
				*p >>= (bitsleft=currentcode-8);
			else
				{
				if (++p >= q &&
				  (((blocksize = fgetc(fp)) < 1) ||
				  (q=(p=b) + fread(b,1,blocksize,fp)) < (b+blocksize)))
					{
					Free(linebuffer);
   				return FALSE;
					}
				thiscode |= *p << (16 - bitsleft);
				*p >>= (bitsleft = currentcode - 16);
				}
			}
		thiscode &= wordmasktable[codesize];
		currentcode = thiscode;

		if (thiscode == (bits2+1)) break;	/* found EOI */
		if (thiscode > nextcode)
			{
			Free(linebuffer);
			return FALSE;
			}

		if (thiscode == bits2)
			{
			nextcode = bits2 + 2;
			codesize2 = 1 << (codesize = (bits + 1));
			oldtoken = oldcode = NO_CODE;
			continue;
			}

		u = firstcodestack;

		if (thiscode==nextcode)
			{
			if (oldcode==NO_CODE)
				{
				Free(linebuffer);
            return FALSE;
				}
			*u++ = oldtoken;
			thiscode = oldcode;
			}

		while (thiscode >= bits2)
			{
			*u++ = lastcodestack[thiscode];
			thiscode = codestack[thiscode];
			}

		oldtoken = thiscode;
		do {
			linebuffer[byte++]=thiscode;

			if (byte >= imageBlock.width)
				{
            // Hmmm, I think this means write out the line
				if (line < imageBlock.depth)
					memcpy(buffer + (long) line * (long) imageBlock.width,
						linebuffer, imageBlock.width);
				byte=0;

				/* check for interlaced image */
				if (imageBlock.flags & 0x40)
					{
					line+=inctable[pass];
					if(line >= imageBlock.depth)
					    line=startable[++pass];
					}
				else
					++line;
				}

			if (u <= firstcodestack) break;
			thiscode = *--u;
			} while(1);

		if (nextcode < 4096 && oldcode != NO_CODE)
			{
			codestack[nextcode] = oldcode;
			lastcodestack[nextcode] = oldtoken;
			if (++nextcode >= codesize2 && codesize < 12)
			    codesize2 = 1 << ++codesize;
			}
		oldcode = currentcode;
		}

	Free(linebuffer);
	Free(firstcodestack);
	Free(lastcodestack);
	Free(codestack);

	return TRUE;
}

//	---------------------------------------------------------------
//
//	putextension() is called when the GIF decoder encounters an extension

static void putextension(FILE *fp)
{
	int n,i;
	int header_size;
   int tail;

	switch (fgetc(fp))
		{
		case 0x0001:		/* plain text descriptor */
			header_size = 13;
			tail = 1;
			break;
		case 0x00f9:		/* graphic control block */
			header_size = 6;
			tail = 0;
			break;
		case 0x00fe:		/* comment extension */
			header_size = 0;
			tail = 1;
			break;
		case 0x00ff:		/* application extension */
			header_size = 12;
			tail = 1;
			break;
		default:		/* something else */
			header_size = 0;
			tail = 0;
			n=fgetc(fp);
			for(i=0;i<n;++i) fgetc(fp);
			break;
		}

	for (i = 0; i< header_size; i++) fgetc(fp);

	if (tail == 1)
		{
		do {
			n=fgetc(fp);
			for(i=0;i<n;++i) fgetc(fp);
			} while(n > 0 && n != EOF);
 	 	}
}

