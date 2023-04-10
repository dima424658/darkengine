/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/buffer.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:50:48 $
 *
 * Prototypes for 2d temporary storage management.
 *
 * This file is part of the dev2d library.
 */

#ifndef __BUFFER_H
#define __BUFFER_H

#include <memall.h>
#include <tmpalloc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define gd_alloc_temp temp_malloc
#define gd_free_temp temp_free

#ifdef __cplusplus
};
#endif
#endif
