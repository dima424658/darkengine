/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/rsd.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:00:40 $
 *
 * Constants and macros for RSD8 bitmap processing.
 *
 * $Log: rsd.h $
 * Revision 1.1  1996/04/10  17:00:40  KEVIN
 * Initial revision
 * 
 * Revision 1.2  1993/10/26  02:14:30  kevin
 * Changed algorithm so that rsd_src is never advanced
 * past the end of the bitmap.  (It stays pointed to the
 * end when the end is reached.)
 * 
 * Revision 1.1  1992/11/12  13:51:29  kaboom
 * Initial revision
 * 
 */

#ifndef __RSD_H
#define __RSD_H

#ifdef __cplusplus
extern "C" {
#endif

#define RSD_RUN    0
#define RSD_SKIP   1
#define RSD_DUMP   2

/* this is a pretty specific-use macro for getting the next rsd token from an
   rsd input buffer.  the source buffer has to be named rsd_src, the token's
   code goes in rsd_code, and the count for the operation goes in rsd_count.
   after a call of this macro, rsd_src is advanced to the actual data for the
   code (pixel data for dump, run value for run) if there is any, or to the
   beginning of the next token (for skip). */
#define RSD_GET_TOKEN()                               \
{                                                     \
   if (*rsd_src == 0)               /* run */         \
   {                                                  \
      rsd_code = RSD_RUN;                             \
      rsd_count = rsd_src[1];                         \
      rsd_src += 2;                                   \
/*      mprintf ("run %d ",count);  */                    \
   }                                                  \
   else if (*rsd_src < 0x80)        /* dump */        \
   {                                                  \
      rsd_code = RSD_DUMP;                            \
      rsd_count = *rsd_src;                           \
      rsd_src++;                                      \
/*      mprintf ("dump %d ",count);  */                   \
   }                                                  \
   else if (*rsd_src != 0x80)       /* skip */        \
   {                                                  \
      rsd_code = RSD_SKIP;                            \
      rsd_count = *rsd_src & 0x7f;                    \
      rsd_src++;                                      \
/*      mprintf ("skip %d ",count);  */                   \
   }                                                  \
   else                             /* long op */     \
   {                                                  \
      ushort *rsd_usrc = (ushort *)++rsd_src;         \
                                                      \
      if (*rsd_usrc >= 0x8000)                        \
      {                                               \
         if (*rsd_usrc >= 0xc000)   /* long run */    \
         {                                            \
            rsd_code = RSD_RUN;                       \
            rsd_count = *rsd_usrc & 0x3fff;           \
            rsd_src += 2;                             \
/*            mprintf ("run %d ",count);   */             \
         }                                            \
         else                       /* long dump */   \
         {                                            \
            rsd_code = RSD_DUMP;                      \
            rsd_count = *rsd_usrc & 0x7fff;           \
            rsd_src += 2;                             \
/*            mprintf ("dump %d ",count);  */             \
         }                                            \
      }                                               \
      else if (*rsd_usrc != 0)      /* long skip */   \
      {                                               \
         rsd_code = RSD_SKIP;                         \
         rsd_count = *rsd_usrc;                       \
         rsd_src += 2;                                \
/*         mprintf ("skip %d ",count);     */             \
      }                                               \
      else {                                           \
/* subsequent uses of RSD_GET_TOKEN should also return to rsd_done.*/ \
         rsd_code = RSD_SKIP;                         \
         rsd_src-- ;                                  \
         goto rsd_done;                               \
      }                                               \
   }                                                  \
}

#ifdef __cplusplus
};
#endif
#endif
