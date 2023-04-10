#ifndef __RESRECT_H
#define __RESRECT_H

/*
 * $Source: x:/prj/tech/libsrc/res/RCS/resrect.h $
 * $Revision: 1.2 $
 * $Author: TOML $
 * $Date: 1996/09/14 16:10:01 $
 *
 */
#ifdef __cplusplus
extern "C"
{
   struct _RectElem;
#endif // __cplusplus


   typedef struct _RectElem
   {
      Rect area;                                 // bounding rectangle
      Rect anchor;                               // bounding rectangle's anchor area
   } RectElem;

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __RESRECT_H

