//		GFILE.H		Generic graphics file access
//		Rex E. Bradford (REX)
/*
* $Header: x:/prj/tech/libsrc/gfile/RCS/gfile.h 1.6 1998/07/01 18:27:10 PATMAC Exp $
* $Log: gfile.h $
 * Revision 1.6  1998/07/01  18:27:10  PATMAC
 * Add extern "C"
 * 
 * Revision 1.5  1994/12/07  14:00:24  mahk
 * Added GfileFindParm
 * 
 * Revision 1.4  1993/10/19  15:10:42  xemu
 * added GfileWrite
 * 
 * Revision 1.3  1993/10/07  09:44:39  rex
 * Added BMP type
 * 
 * Revision 1.2  1993/09/27  19:21:25  rex
 * Added GFILE_CEL type
 * 
 * Revision 1.1  1993/09/27  16:38:49  rex
 * Initial revision
 * 
 * Revision 1.1  1993/09/27  15:58:02  rex
 * Initial revision
 * 
*/

#ifndef GFILE_H
#define GFILE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef STDIO_H
#include <stdio.h>
#endif
#ifndef TYPES_H
#include <types.h>
#endif
#ifndef RECT_H
#include <rect.h>
#endif
#ifndef __2D_H 
#include <2d.h>
#endif
#ifndef DATAPATH_H
#include <datapath.h>
#endif

typedef enum {
	GFILE_PCX,				// .pcx file (org. PC Paintbrush)
	GFILE_GIF,				// .gif file (org. Compuserve?)
	GFILE_CEL,				// .cel, .flc, or .fli (Autodesk Animator/Studio)
	GFILE_IFF,				// .iff (NOT SUPPORTED YET!)
	GFILE_BMP,				// .bmp file (Windows 3.x)
} GfileType;

typedef struct {
	short index;				// index at which to start writing
	short numcols;				// number of colors to write
	uchar rgb[3 * 256];		// 0-256 rgb entries
} PallInfo;

typedef struct {
	GfileType type;		// GFILE_XXX
	grs_bitmap bm;			// bitmap
	uchar *ppall;			// ptr to pall, or NULL
} GfileInfo;

//	Function prototypes

bool GfileRead(GfileInfo *pgfi, char *filename, Datapath *pdp);
bool GfileWrite(GfileInfo *pgfi, char *filename, Datapath *pdp);
void GfileFree(GfileInfo *pgfi);
void GfileGetPall(GfileInfo *pgfi, PallInfo *ppall);

void GfileGetImage(grs_bitmap *pbm, Rect *parea, uchar *pbits);
bool GfileFindImage(grs_bitmap *pbm, Point currLoc, Rect *parea, uchar bordCol);
int GfileFindAnchorRect(grs_bitmap *pbm, Rect *parea, uchar bordCol, Rect *panrect);
int GfileFindParm(grs_bitmap *pbm, Rect *parea, uchar bordCol, int* parm);


#ifdef __cplusplus
}
#endif
#endif

