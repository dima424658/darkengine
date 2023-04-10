/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/grnull.h $
 * $Revision: 1.2 $
 * $Author: KEVIN $
 * $Date: 1996/04/30 11:57:22 $
 *
 * Null-function placeholder prototype.
 *
 * This file is part of the dev2d library.
 *
 * $Log: grnull.h $
 * Revision 1.2  1996/04/30  11:57:22  KEVIN
 * Added gr_scream(), which spews a warning when called.
 * 
 * Revision 1.1  1996/04/10  16:40:30  KEVIN
 * Initial revision
 * 
 * Revision 1.1  1993/02/04  17:36:36  kaboom
 * Initial revision
 * 
 */

#ifndef __GRNULL_H
#define __GRNULL_H

#ifdef __cplusplus
extern "C" {
#endif

extern void gr_null (void);
extern void gr_scream (void);
#define gr_warn gr_scream

#ifdef __cplusplus
};
#endif
#endif /* !__GRNULL_H */
