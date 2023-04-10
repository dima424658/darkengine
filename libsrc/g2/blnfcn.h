/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/blnfcn.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/11 10:01:00 $
 *
 * prototypes and for the blend system
 *
 * This file is part of the g2 libarary.
 *
 */

#ifndef __BLNFCN_H
#define __BLNFCN_H
/* prototypes for blend table maintenance, TRUE means success, FALSE not */
extern bool gr_free_blend(void);
/* tab_cnt is how many blend steps, note cnt<=0 is equivalent to calling
   free blend */
extern bool gr_init_blend(int log_blend_levels);
/* returns TRUE if a memory error occured. */
extern int gr_source_blend_enable();
extern void gr_source_blend_disable();
extern bool grd_source_blend_enabled;
#endif /* !__BLNFCN */
