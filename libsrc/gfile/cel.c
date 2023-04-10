//		CEL.C		Reads 1st frame from .CEL or .FLC or .FLI file
//		Rex E. Bradford (REX)
//
//	The header files: "pjstypes.h" and "pjfli.h" come from the
//	Autodesk developer's kit, and the .fli/.flc format is more fully
//	described in an accompanying document entitled:
//		"Animation Support Libraries: File Formats Reference"
/*
* $Header: r:/prj/lib/src/gfile/rcs/cel.c 1.3 1994/05/26 18:00:36 rex Exp $
* $Log: cel.c $
 * Revision 1.3  1994/05/26  18:00:36  rex
 * Allocated bitmap buffer larger than need be to avoid crash, also took
 * out warning of condition that should be met but isn't always
 * 
 * Revision 1.2  1993/10/26  13:21:50  rex
 * Fixed bug in CelExpandWordDelta()
 * 
 * Revision 1.1  1993/09/27  19:21:41  rex
 * Initial revision
 * 
*/

#include <string.h>

#include <lg.h>
#include <gfile.h>
#include <pjstypes.h>
#include <pjfli.h>

bool CelReadFrame(GfileInfo *pgfi, FILE *fp);
void CelGetPall(GfileInfo *pgfi, uchar *buff, int shifter);
void CelExpandBrun(GfileInfo *pgfi, uchar *ps);
void CelExpandByteDelta(GfileInfo *pgfi, uchar *ps);
void CelExpandWordDelta(GfileInfo *pgfi, uchar *ps);

//	------------------------------------------------------
//
//	CelRead() reads in a .CEL file.

bool CelRead(GfileInfo *pgfi, FILE *fp)
{
	Fli_head celHead;

//	Read header, check for error

	fread(&celHead, sizeof(celHead), 1, fp);
	if ((celHead.type != FLIHR_MAGIC) && (celHead.type != FLIH_MAGIC))
		{
		Warning(("CelRead: invalid CEL/FLC/FLI magic type: %d\n", celHead.type));
		return FALSE;
		}

//	Assume no palette

	pgfi->ppall = NULL;

//	Seek to first frame, set up bitmap, read it

	fseek(fp, celHead.frame1_oset, SEEK_SET);

	pgfi->bm.type = BMT_FLAT8;
	pgfi->bm.align = 0;
	pgfi->bm.flags = 0;
	pgfi->bm.w = celHead.width;
	pgfi->bm.h = celHead.height;
	pgfi->bm.row = pgfi->bm.w;
	pgfi->bm.bits = Malloc((long) pgfi->bm.row * (long) pgfi->bm.h);
	if (pgfi->bm.bits == NULL)
		goto BADRET;

	if (!CelReadFrame(pgfi, fp))
		goto BADRET;

	return TRUE;

//	Error, clean up

BADRET:

	GfileFree(pgfi);
	return FALSE;
}

//	----------------------------------------------------------
//
//	CelReadFrame() reads in a frame from the .cel file.

static bool CelReadFrame(GfileInfo *pgfi, FILE *fp)
{
	Fli_frame frameInfo;
	Chunk_id chunk;
	int ichunk;
	long nextOffset;
	bool frameDone;
	long fullLen;
	uchar *buff;

//	Buffer not allocated yet

	buff = NULL;

READFRAME:

//	Read frame info header, check for errors, report if verbose

	fread(&frameInfo, sizeof(frameInfo), 1, fp);
	if (frameInfo.type != FCID_FRAME)
		{
		Warning(("CelReadFrame: missing frame chunk\n"));
		goto BADRET;
		}

//	Allocate buffer

	if (buff == NULL)
		{
		fullLen = (long) pgfi->bm.row * (long) pgfi->bm.h;
		buff = Malloc(max(fullLen, frameInfo.size));
		if (buff == NULL)
			return FALSE;
		}

//	If no frame chunks, add time to previous frame & retry

	if (frameInfo.chunks == 0)
		{
		fseek(fp, frameInfo.size - sizeof(frameInfo), SEEK_CUR);
		goto READFRAME;
		}

//	Else process all chunks in frame

	else
		{
		frameDone = FALSE;

		for (ichunk = 0; ichunk < frameInfo.chunks; ichunk++)
			{
			fread(&chunk, sizeof(chunk), 1, fp);
			nextOffset = ftell(fp) + chunk.size - sizeof(chunk);
			switch (chunk.type)
				{
				case FLI_COLOR256:
					fread(buff, chunk.size - sizeof(chunk), 1, fp);
					if (pgfi->ppall == NULL)
						pgfi->ppall = Malloc(3 * 256);
					if (pgfi->ppall == NULL)
						goto BADRET;
					CelGetPall(pgfi, buff, 0);
					break;

				case FLI_SS2:
					fread(buff, chunk.size - sizeof(chunk), 1, fp);
					CelExpandWordDelta(pgfi, buff);
					frameDone = TRUE;
					break;

				case FLI_COLOR:
					fread(buff, chunk.size - sizeof(chunk), 1, fp);
					if (pgfi->ppall == NULL)
						pgfi->ppall = Malloc(3 * 256);
					if (pgfi->ppall == NULL)
						goto BADRET;
					CelGetPall(pgfi, buff, 2);
					break;

				case FLI_LC:
					fread(buff, chunk.size - sizeof(chunk), 1, fp);
					CelExpandByteDelta(pgfi, buff);
					frameDone = TRUE;
					break;

				case FLI_COLOR_0:
					memset(pgfi->bm.bits, 0, fullLen);
					frameDone = TRUE;
					break;

				case FLI_BRUN:
					fread(buff, chunk.size - sizeof(chunk), 1, fp);
					CelExpandBrun(pgfi, buff);
					frameDone = TRUE;
					break;

				case FLI_COPY:
#ifdef OMIT		// amazingly, this relation isn't always true!
					if ((chunk.size - sizeof(chunk)) != fullLen)
						{
						Warning(("CelReadFrame: invalid FLI_COPY chunk size,\n\
chunk.size - sizeof(chunk) = %d, fullLen = %d\n",
chunk.size - sizeof(chunk), fullLen));
						goto BADRET;
						}
#endif
					fread(pgfi->bm.bits, fullLen, 1, fp);
					frameDone = TRUE;
					break;

				default:
					break;
				}

			fseek(fp, nextOffset, SEEK_SET);
			}
		}

//	Make sure got some data in that chunk, bump current frame & return

	if (!frameDone)
		{
		Warning(("CelReadFrame: no frame data!\n"));
		goto BADRET;
		}

	Free(buff);
	buff = NULL;
	return TRUE;

//	Bad return

BADRET:

	if (buff)
		{
		Free(buff);
		buff = NULL;
		}
	return FALSE;
}

//	----------------------------------------------------------
//		INTERNAL ROUTINES
//	----------------------------------------------------------
//
//	CelGetPall() gets pallette

static void CelGetPall(GfileInfo *pgfi, uchar *buff, int shifter)
{
	uchar *pd;
	short index;
	short numPackets;
	short ipacket;
	short count;
	short i;

	pd = pgfi->ppall;
	index = 0;

//	First word is # color packets

	numPackets = *(short *)buff;
	buff += 2;

	for (ipacket = 0; ipacket < numPackets; ipacket++)
		{
//	Each packet is index,count,rgb[]

		index += *buff++;
		count = *buff++;
		if (count == 0)
			count = 256;

		if ((index != 0) || (count != 256))
			{
			Warning(("CelGetPall: can't handle partial pallettes\n"));
			}

		for (i = 0; i < (count * 3); i++)
			*pd++ = *buff++ << shifter;
		}
}

//	----------------------------------------------------------
//
//	CelExpandBrun() expands a FLI_BRUN chunk.

static void CelExpandBrun(GfileInfo *pgfi, uchar *ps)
{
	uchar *pd;
	short y;
	short pixels;
	signed char npix;
	short i;

//	Init destination

	pd = pgfi->bm.bits;

//	Byte-Run-length is always full-screen

	for (y = 0; y < pgfi->bm.h; y++)
		{
		ps++;									// skip packet count;
		pixels = 0;							// init pixel count

//	Get pixels till hit end of line

		while (pixels < pgfi->bm.w)
			{
			npix = *(signed char *)ps;	// get signed count
			ps++;
			if (npix < 0)					// if negative count, copy
				{
				npix = -npix;
				for (i = 0; i < npix; i++)
					*pd++ = *ps++;
				}
			else								// if positive count, run
				{
				for (i = 0; i < npix; i++)
					*pd++ = *ps;
				ps++;
				}
			pixels += npix;				// bump pixel count
			}

//	At end of line, check for errors

		if (pixels > pgfi->bm.w)
			{
			Warning(("CelExpandBrun: decompress error\n"));
			return;
			}
		}
}

//	----------------------------------------------------------
//
//	CelExpandByteDelta() expands a FLI_LC chunk.

static void CelExpandByteDelta(GfileInfo *pgfi, uchar *ps)
{
	uchar *pd;
	short nskip;
	short nlines;
	short y;
	short xpos;
	short i;
	short numPackets;
	short ipacket;
	signed char npix;

//	Init dest

	pd = pgfi->bm.bits;

//	First word is # lines to skip

	nskip = *(short *)ps;
	ps += 2;

	for (y = 0; y < nskip; y++)
		{
		for (i = 0; i < pgfi->bm.w; i++)
			*pd++ = 0;
		}

//	Next word is # lines in the chunk

	nlines = *(short *)ps;
	ps += 2;

//	Iterate thru lines

	for (y = 0; y < nlines; y++)
		{

//	First byte is packet count

		numPackets = *ps++;
		xpos = 0;

//	Loop thru packets

		for (ipacket = 0; ipacket < numPackets; ipacket++)
			{

//	First byte is # pixels to skip

			nskip = *ps++;
			for (i = 0; i < nskip; i++)
				*pd++ = 0;
			xpos += nskip;

//	Next byte is count

			npix = *(signed char *)ps;
			ps++;

//	If count negative, pixel run

			if (npix < 0)
				{
				npix = -npix;
				for (i = 0; i < npix; i++)
					*pd++ = *ps;
				ps++;
				}

//	Else if count positive, pixel copy

			else
				{
				for (i = 0; i < npix; i++)
					*pd++ = *ps++;
				}

//	Update pixel pos

			xpos += npix;
			}

//	At end of packets, check for errors & fill in rest of line with skip

		if (xpos > pgfi->bm.w)
			{
			Warning(("CelExpandByteDelta: decompress error\n"));
			return;
			}
		else if (xpos < pgfi->bm.w)
			{
			for (i = xpos; i < pgfi->bm.w; i++)
				*pd++ = 0;
			}
		}
}

//	----------------------------------------------------------
//
//	CelExpandWordDelta() expands a FLI_SS2 chunk.

static void CelExpandWordDelta(GfileInfo *pgfi, uchar *ps)
{
	uchar *pd;
	short nlines;
	short nskip;
	short y;
	short word;
	short xpos;
	short i,j;
	short numPackets;
	short ipacket;
	signed char nwords;
	short lastByte;

//	Init dest

	pd = pgfi->bm.bits;

//	First word is # lines in chunk

	nlines = *(short *)ps;
	ps += 2;

//	Iterate thru lines

	for (y = 0; y < nlines; y++)
		{

//	Check for special options

		while (TRUE)
			{
			word = *(short *)ps;
			ps += 2;
			if ((word & 0xC000) == 0xC000)	// 11xxxxxx: skip lines
				{
				for (i = 0; i < -word; i++)
					for (j = 0; j < pgfi->bm.w; j++)
						*pd++ = 0;
				}
			else if ((word & 0xC000) == 0x8000)	// 10xxxxxx: single byte patch
				{
				lastByte = word & 0xFF;
				numPackets = *(short *)ps;
				ps += 2;
				break;
				}
			else if ((word & 0xC000) == 0x4000)
				{
				Error(1, "CelExpandWordDelta: 0x4000 case illegal\n");
				}
			else										// 00xxxxxx: num packets (following)
				{
				numPackets = word;
				break;
				}
			}

//	Get packets

		xpos = 0;
		for (ipacket = 0; ipacket < numPackets; ipacket++)
			{

//	First byte is pixels to skip

			nskip = *ps++;
			for (i = 0; i < nskip; i++)
				*pd++ = 0;
			xpos += nskip;

//	Next byte is count of words

			nwords = *(signed char *)ps;
			ps++;

//	If count negative, word pixel run

			if (nwords < 0)
				{
				nwords = -nwords;
				for (i = 0; i < nwords; i++)
					{
					*pd++ = *ps;
					*pd++ = *(ps + 1);
					}
				ps += 2;
				}

//	Else if count positive, word pixel copy

			else
				{
				for (i = 0; i < nwords; i++)
					{
					*pd++ = *ps++;
					*pd++ = *ps++;
					}
				}

//	Update pixel pos

			xpos += (nwords << 1);
			}

//	At end of packets, check for errors & fill in rest of line with skip

		if (xpos > pgfi->bm.w)
			{
			Warning(("CelExpandWordDelta: decompress error\n"));
			return;
			}
		else if (xpos < pgfi->bm.w)
			{
			for (i = xpos; i < pgfi->bm.w; i++)
				*pd++ = 0;
			}
		if (lastByte >= 0)
			*(pd - 1) = lastByte;
		}
}