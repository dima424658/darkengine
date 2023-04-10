//		Rect.H		Rectangle routines header file
//		Rex E. Bradford (REX)
/*
* $Header: x:/prj/tech/libsrc/dstruct/RCS/rect.h 1.10 1997/03/12 19:14:32 TONY Exp $
* $Log: rect.h $
 * Revision 1.10  1997/03/12  19:14:32  TONY
 * Added RectSetNull(), RectCheckNull() macros.
 * 
 * Revision 1.9  1997/02/21  13:51:43  TOML
 * made structs forward declarable
 * 
 * Revision 1.8  1997/01/09  18:06:33  TOML
 * new msvc
 *
 * Revision 1.7  1996/10/10  13:16:13  TOML
 * msvc
 *
 * Revision 1.6  1996/01/22  15:39:11  DAVET
 *
 * Added cplusplus stuff.
 *
 * Revision 1.5  1994/04/05  11:02:15  rex
 * One should endeavor to spend less time writing diatribes about the compiler,.
 * and more time ensuring the veracity of one's code.
 *
 * Revision 1.4  1994/04/05  04:04:24  dc
 * how about a make point that isnt a function and works better and so on....
 *
 * Revision 1.3  1993/11/08  18:53:28  mahk
 * WHOOOPS
 *
 * Revision 1.2  1993/11/08  18:33:47  mahk
 * Added RECT_FILL and MakePoint
 *
 * Revision 1.1  1993/04/22  13:06:17  rex
 * Initial revision
 *
 * Revision 1.2  1993/04/19  18:35:01  rex
 * Added macro versions of most functions
 *
 * Revision 1.1  1992/08/31  17:03:04  unknown
 * Initial revision
 *
*/


#ifndef RECT_H
#define RECT_H

#ifdef __cplusplus
extern "C"  {
#endif

//	Here are the Point and Rect structs

typedef struct Point {
	short x;
	short y;
} Point;

typedef struct Rect {
	Point ul;
	Point lr;
} Rect;

//	Point macros

#define PointsEqual(p1,p2) (*(long*)(&(p1)) == *(long*)(&(p2)))
#define PointSetNull(p) do {(p).x = -1; (p).y = -1;} while (0);
#define PointCheckNull(p) ((p).x == -1 && (p).y == -1)

//	Rect macros: get width & height

#define RectWidth(pr) ((pr)->lr.x - (pr)->ul.x)
#define RectHeight(pr) ((pr)->lr.y - (pr)->ul.y)
#define RectSetNull(pr) do {(pr)->ul.x = (pr)->ul.y = (pr)->lr.x = (pr)->lr.y = -1;} while (0);
#define RectCheckNull(pr) ((pr)->ul.x == -1 && (pr)->ul.y == -1 && (pr)->lr.x == -1 && (pr)->lr.y == -1)

// Unwrap macros

#define RECT_UNWRAP(pr)  ((pr)->ul.x),((pr)->ul.y),((pr)->lr.x),((pr)->lr.y)
#define PT_UNWRAP(pt)    ((pt).x),((pt).y)

//	These macros are faster, fatter versions of their function counterparts

#define RECT_TEST_SECT(pr1,pr2) ( \
	((pr1)->ul.y < (pr2)->lr.y) && \
	((pr1)->lr.y > (pr2)->ul.y) && \
	((pr1)->ul.x < (pr2)->lr.x) && \
	((pr1)->lr.x > (pr2)->ul.x))

#define RECT_UNION(pr1,pr2,prunion) { \
	(prunion)->ul.x = (pr1)->ul.x < (pr2)->ul.x ? (pr1)->ul.x : (pr2)->ul.x; \
	(prunion)->lr.x = (pr1)->lr.x > (pr2)->lr.x ? (pr1)->lr.x : (pr2)->lr.x; \
	(prunion)->ul.y = (pr1)->ul.y < (pr2)->ul.y ? (pr1)->ul.y : (pr2)->ul.y; \
	(prunion)->lr.y = (pr1)->lr.y > (pr2)->lr.y ? (pr1)->lr.y : (pr2)->lr.y; \
	}

#define RECT_ENCLOSES(pr1,pr2) ( \
	((pr1)->ul.y <= (pr2)->ul.y) && \
	((pr1)->lr.y >= (pr2)->lr.y) && \
	((pr1)->ul.x <= (pr2)->ul.x) && \
	((pr1)->lr.x >= (pr2)->lr.x))

#define RECT_TEST_PT(prect,pt) ( \
	((pt).y >= (prect)->ul.y) && ((pt).y < (prect)->lr.y) && \
	((pt).x >= (prect)->ul.x) && ((pt).x < (prect)->lr.x))

#define RECT_MOVE(prect,pt) { \
	(prect)->ul.x += pt.x; \
	(prect)->ul.y += pt.y; \
	(prect)->lr.x += pt.x; \
	(prect)->lr.y += pt.y; \
	}

#define RECT_OFFSETTED_RECT(pr1,pt,proff) { \
	(proff)->ul.x = (pr1)->ul.x + (pt).x; \
	(proff)->ul.y = (pr1)->ul.y + (pt).y; \
	(proff)->lr.x = (pr1)->lr.x + (pt).x; \
	(proff)->lr.y = (pr1)->lr.y + (pt).y; \
	}

#define RECT_FILL(pr,x,y,x2,y2) \
   { \
      (pr)->ul.x = (x);  \
      (pr)->ul.y = (y);  \
      (pr)->lr.x = (x2); \
      (pr)->lr.y = (y2); \
   }

//	These are the functional versions of the above macros

int RectTestSect(Rect *pr1, Rect *pr2);
void RectUnion(Rect *pr1, Rect *pr2, Rect *prunion);
int RectEncloses(Rect *pr1, Rect *pr2);
int RectTestPt(Rect *prect, Point pt);
void RectMove(Rect *pr, Point delta);
void RectOffsettedRect(Rect *pr, Point delta, Rect *proff);

//	These functions have no macro counterparts
int RectSect(Rect *pr1, Rect *pr2, Rect *prsect);
int RectClipCode(Rect *prect, Point pt);

// guess why this isnt a macro        // hah, you cant
//Point MakePoint(short x, short y);  // Guess what this does.
//#define MakePoint(x,y) (Point)(((ushort)y<<16)+((ushort)x))
// oh, doug is mocked, you cant cast to a non-scaler type

// take this, note ax and bx passed but use whole thing... oooooh
#ifdef __WATCOMC__
Point MakePointInline(ushort x, ushort y);
#pragma aux MakePointInline = \
   "shl     ebx,10H"          \
   "and     eax,0000ffffH"    \
   "add     eax,ebx"          \
   parm [ax] [bx]             \
   modify [eax ebx];
#else
#ifdef BAD
__inline Point MakePointInline(ushort x, ushort y)
{
#pragma warning(disable : 4035)	// disables no return value warning
	__asm
	{
		mov		ax, x
		mov		bx, y
		shl		ebx, 10H
		and		eax, 0000ffffH
		add		eax, ebx
	}
};
#else
__inline Point MakePointInline(ushort x, ushort y)
{
    ulong l = (y << 16) | x;
    return *((Point *)(&l));
}
#endif
#endif
// and this
#define MakePoint(x,y) MakePointInline((ushort)x,(ushort)y)
// curse you
// note i had to specify ax and bx even though i dont care what is really used
// we would like to say [ax bx cx dx] and then have the code use arg1 and arg2
// however, we cant, because there is no way of doing that, because lifeispain
// so the compiler generates things like mov eax,ebx;mov ebx,ecx;code as above
// which is dumb, since we could do the above with bx and cx just as well. ick

#ifdef __cplusplus
}
#endif



#endif
