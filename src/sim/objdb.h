/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/objdb.h,v 1.2 2000/01/29 13:41:17 adurant Exp $
#pragma once

#ifndef __OBJDB_H
#define __OBJDB_H

#include <dbasemsg.h>
#include <osysbase.h>

//@@@ TODO: Change this for phase III.
#define OBJ_BELONGS_TO_FILETYPE(o,f) ( (((f) & kFiletypeGAM) && OBJ_IS_ABSTRACT(o)) \
                            || (((f) & (kFiletypeMAP|kFiletypeMIS)) && OBJ_IS_CONCRETE(o)))


#endif // __OBJDB_H
