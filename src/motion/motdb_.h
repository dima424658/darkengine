/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/motdb_.h,v 1.2 2000/01/31 09:50:56 adurant Exp $
#pragma once

// utilities for building motion database

#ifndef __MOTDB__H
#define __MOTDB__H

#include <label.h>
#include <motdbtyp.h>
#include <fcytagdb.h>


EXTERN void MotDBCreate(int nActors);
EXTERN void MotDBRegisterTag(Label *pName, sTagInfo *pInfo);
EXTERN void MotDBAddSchema(sMotSchemaDesc *pSchema);

#endif // __MOTDB__H
