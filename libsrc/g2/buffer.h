/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/buffer.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/11 10:01:08 $
 *
 * Prototypes for 2d temporary storage management.
 *
 * This file is part of the g2 library.
 */

#include <memall.h>
#include <tmpalloc.h>
#define gr_alloc_temp temp_malloc
#define gr_free_temp temp_free
