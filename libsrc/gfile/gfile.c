//	GFILE.C		Read & process a graphics file
//	Rex E. Bradford (REX)
//
//	This file implements a generic graphics file for read-only.
//	Various specific files (gfpcx.c, for instance), implement
//	stuff specific to that format.  Currently, this set of routines
//	assumes 1 picture per file.
//
//	Static arrays and variables are used to hold information
//	on the current pcx file, so only one can be open at
//	a time.
/*
* $Header: r:/prj/lib/src/gfile/rcs/gfile.c 1.9 1994/12/07 14:00:01 mahk Exp $
* $Log: gfile.c $
 * Revision 1.9  1994/12/07  14:00:01  mahk
 * Added GfileFindParm
 * 
 * Revision 1.8  1994/02/25  15:48:00  rex
 * Fixed area finder for when rectangle extends to rightmost or bottommost pixel
 * 
 * Revision 1.7  1994/02/21  12:43:06  rex
 * Fixed bug in bordered area finder
 * 
 * Revision 1.6  1993/10/19  15:09:56  xemu
 * added GfileWrite
 * 
 * Revision 1.5  1993/10/07  09:44:47  rex
 * Added BMP type
 * 
 * Revision 1.4  1993/09/27  19:28:08  rex
 * Fixed bug in extension lookup
 * 
 * Revision 1.3  1993/09/27  19:21:12  rex
 * Added .cel type, changed extension lookup code
 * 
 * Revision 1.2  1993/09/27  18:00:40  rex
 * Added call to GifRead
 * 
 * Revision 1.1  1993/09/27  16:38:45  rex
 * Initial revision
 * 
*/

#include <string.h>

#include <lg.h>
#include <gfile.h>
#include <fname.h>

//	Read() functions in other files

bool PcxRead(GfileInfo *pgfi, FILE *fp);
bool GifRead(GfileInfo *pgfi, FILE *fp);
bool CelRead(GfileInfo *pgfi, FILE *fp);
bool BmpRead(GfileInfo *pgfi, FILE *fp);

//	Write() functions in other files

bool PcxWrite(GfileInfo *pgfi, FILE *fp);

//	-------------------------------------------------------
//		ACCESS ROUTINES
//	-------------------------------------------------------
//
//	GfileRead() grabs the image out of a graphics file.
//
//		pgfi     = ptr to file info struct
//		filename = ptr to filename
//		pdp      = ptr to datapath
//
//	Returns: TRUE if successful, FALSE if problem

bool GfileRead(GfileInfo *pgfi, char *filename, Datapath *pdp)
{
static char *exts[] = {"PCX","GIF","CEL","FLC","FLI","BMP",NULL};
static GfileType gftypes[] = {GFILE_PCX,GFILE_GIF,GFILE_CEL,GFILE_CEL,
	GFILE_CEL,GFILE_BMP};

	int itype;
	FILE *fp;
	bool ret;
	Fname fname;

//	Extract file extension, get type

	FnameExtract(&fname, filename);
	itype = 0;
	while (exts[itype])
		{
		if (strcmpi(fname.ext, exts[itype]) == 0)
			{
			pgfi->type = gftypes[itype];
			break;
			}
		++itype;
		}
	if (exts[itype] == NULL)
		{
		Warning(("GfileOpen: unknown extension on file: %s\n", filename));
		return FALSE;
		}

//	Open file

	fp = DatapathOpen(pdp, filename, "rb");
	if (fp == NULL)
		return FALSE;

//	If opened successfully, read it

	switch (pgfi->type)
		{
		case GFILE_PCX:
			ret = PcxRead(pgfi, fp);
			break;

		case GFILE_GIF:
			ret = GifRead(pgfi, fp);
			break;

		case GFILE_CEL:
			ret = CelRead(pgfi, fp);
			break;

		case GFILE_BMP:
			ret = BmpRead(pgfi, fp);
			break;

		default:
			Warning(("GfileUse: unknown format %d\n", pgfi->type));
			ret = FALSE;
			break;
		}

//	Close file and return

	fclose(fp);
	return(ret);
}

//	-------------------------------------------------------
//		ACCESS ROUTINES
//	-------------------------------------------------------
//
//	GfileWrite() writes the image out of a graphics file.
//
//		pgfi     = ptr to file info struct
//		filename = ptr to filename
//		pdp      = ptr to datapath
//
//	Returns: TRUE if successful, FALSE if problem

bool GfileWrite(GfileInfo *pgfi, char *filename, Datapath *pdp)
{
static char *exts[] = {"PCX","GIF","CEL","FLC","FLI","BMP",NULL};
static GfileType gftypes[] = {GFILE_PCX,GFILE_GIF,GFILE_CEL,GFILE_CEL,
	GFILE_CEL,GFILE_BMP};

	int itype;
	FILE *fp;
	bool ret;
	Fname fname;

//	Extract file extension, get type

	FnameExtract(&fname, filename);
	itype = 0;
	while (exts[itype])
		{
		if (strcmpi(fname.ext, exts[itype]) == 0)
			{
			pgfi->type = gftypes[itype];
			break;
			}
		++itype;
		}
	if (exts[itype] == NULL)
		{
		Warning(("GfileOpen: unknown extension on file: %s\n", filename));
		return FALSE;
		}

//	Open file

	fp = DatapathOpen(pdp, filename, "wb");
	if (fp == NULL)
		return FALSE;

//	If opened successfully, read it

	switch (pgfi->type)
		{
		case GFILE_PCX:
			ret = PcxWrite(pgfi, fp);
			break;

		default:
			Warning(("GfileUse: unknown format %d\n", pgfi->type));
			ret = FALSE;
			break;
		}

//	Close file and return

	fclose(fp);
	return(ret);
}

//	--------------------------------------------------------
//
//	GfileFree() frees up info in gfile struct.
//
//		pgfi = ptr to file info struct

void GfileFree(GfileInfo *pgfi)
{
	if (pgfi->bm.bits)
		Free(pgfi->bm.bits);
	if (pgfi->ppall)
		Free(pgfi->ppall);
	memset(pgfi, 0, sizeof(GfileInfo));
}

//	--------------------------------------------------------
//
//	GfileGetPall() gets partial pallette from a gfile.

void GfileGetPall(GfileInfo *pgfi, PallInfo *ppall)
{
	memcpy(ppall->rgb, pgfi->ppall + (ppall->index * 3), ppall->numcols * 3);
}

//	-------------------------------------------------------
//
//	GfileGetImage() gets image from a graphics file.
//
//		pbm   = ptr to bitmap
//		parea = ptr to area to extract
//		pbits = buffer to extract into (must be at least width * height bytes)

void GfileGetImage(grs_bitmap *pbm, Rect *parea, uchar *pbits)
{
	uchar *psrc;
	int width;
	int y;

	psrc = pbm->bits + (parea->ul.y * pbm->row) + parea->ul.x;
	width = parea->lr.x - parea->ul.x;

	for (y = parea->ul.y; y < parea->lr.y; y++)
		{
		memcpy(pbits, psrc, width);
		psrc += pbm->row;
		pbits += width;
		}
}

//	--------------------------------------------------------
//
//	GfileFindImage() searches for next image against a background.
//	It looks for the upper-left corner of a rectangle set to
//	the border color, and assumes an image is inside the rectangle.
//
//		pbm     = ptr to bitmap
//		currLoc = point to begin search at
//		parea   = area found (filled in)
//		bordCol = border color

bool GfileFindImage(grs_bitmap *pbm, Point currLoc, Rect *parea, uchar bordCol)
{
	uchar *pbase;
	uchar *p;
	short x;

//	Look while current y is inside picture

	while (currLoc.y < (pbm->h - 1))
		{

//	Compute ptr to start of current row

		pbase = pbm->bits + (currLoc.y * pbm->row);

//	Start search across from current x location

		for (x = currLoc.x + 1; x < (pbm->w - 1); x++)
			{

//	Look for 3-pixel corner (current, pixel to right, pixel below)

			p = pbase + x;
			if ((*p == bordCol) && (*(p+1) == bordCol) &&
				(*(p + pbm->w) == bordCol))
					{

//	If found, set image area u.l. to be inside corner

					parea->ul.x = x + 1;
					parea->ul.y = currLoc.y + 1;

//	And then start looking to right for border color to end

					for (parea->lr.x = (x + 1); parea->lr.x < pbm->w; parea->lr.x++)
						{
						if (*(pbase + parea->lr.x) != bordCol)
							break;
						}
					parea->lr.x--;

//	Also look down for border color to end

					for (parea->lr.y = (currLoc.y + 1); parea->lr.y < pbm->h;
						parea->lr.y++)
						{
						if (*(pbm->bits + (parea->lr.y * pbm->row) + x) != bordCol)
							break;
						}
					parea->lr.y--;

					return TRUE;
					}
			}

//	Didn't see anything (else) on this line, start at beginning of next line

		currLoc.y++;
		currLoc.x = -1;
		}

//	Hey, no more in the picture!

	return FALSE;
}

//	--------------------------------------------------------
//
//	GfileFindAnchorRect() searches for an anchor rectangle in
//	the current image's border.  The current image must be bordered,
//	and the bottom and right border edges are searched for strips
//	not in the border color.
//
//		pbm     = ptr to bitmap
//		parea   = ptr to image area
//		bordCol = border color
//		panrect = ptr to anchor rect (filled in by this routine unless
//						not present)
//
//	Returns:
//
//		0 if no anchor rect
//		1 if anchor rect (can be just point)
//		-1 if error (for instance, bottom side has one but not right)

int GfileFindAnchorRect(grs_bitmap *pbm, Rect *parea, uchar bordCol, Rect *panrect)
{
	uchar *p;
	int x,y;
	int foundx,foundy;

//	Search bottom edge

	foundx = 0;
	p = pbm->bits + (parea->lr.y * pbm->row) + parea->ul.x;
	for (x = 0; x < RectWidth(parea); x++)
		{
		if (*p++ == bordCol)
			{
			switch (foundx)
				{
				case 0:
				case 2:
					break;
				case 1:
					panrect->lr.x = x;
					foundx = 2;
					break;
				}
			}
		else
			{
			switch (foundx)
				{
				case 0:
					panrect->ul.x = x;
					foundx = 1;
					break;
				case 1:
					break;
				case 2:
					return(-1);
				}
			}
		}

//	Search bottom edge

	foundy = 0;
	p = pbm->bits + (parea->ul.y * pbm->row) + parea->lr.x;
	for (y = 0; y < RectHeight(parea); y++, p += pbm->row)
		{
		if (*p == bordCol)
			{
			switch (foundy)
				{
				case 0:
				case 2:
					break;
				case 1:
					panrect->lr.y = y;
					foundy = 2;
					break;
				}
			}
		else
			{
			switch (foundy)
				{
				case 0:
					panrect->ul.y = y;
					foundy = 1;
					break;
				case 1:
					break;
				case 2:
					return(-1);
				}
			}
		}

//	Return code

	if ((foundx == 0) && (foundy == 0))
		return(0);
	if (foundx == 1)
		panrect->lr.x = RectWidth(parea);
	if (foundy == 1)
		panrect->lr.y = RectHeight(parea);
	if (foundx && (!foundy))
		return(-1);
	if (foundy && (!foundx))
		return(-1);

	return(1);
}



//	--------------------------------------------------------
//
//	GfileFindParm() looks for an integer "parameter" for the image.
//                 For now, this is the color of the bottom-right pixel of the 
//                 bounding box
//
//		pbm     = ptr to bitmap
//		parea   = ptr to image area
//		bordCol = border color
//		parm =   ptr to parm to be filled by this routine
//
//
//		0 if no parm
//		1 if parm found
//		-1 if error 

#pragma off(unreferenced)

int GfileFindParm(grs_bitmap *pbm, Rect *parea, uchar bordCol, int* parm)
{
	uchar *p;

	p = pbm->bits + (parea->lr.y * pbm->row) + parea->lr.x;
   *parm = *p;
	return(1);
}

#pragma on(unreferenced)
