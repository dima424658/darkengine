/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/grnull.c $
 * $Revision: 1.2 $
 * $Author: KEVIN $
 * $Date: 1996/04/30 11:56:51 $
 *
 * Null-function placeholder.
 *
 * This file is part of the dev2d library.
 *
 */

#include <dbg.h>

void gr_null (void) {}

void gr_scream (void)
{
   Warning(("gr_scream(): 2d operation not supported.\n"));
}
