/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Source: r:/t2repos/thief2/src/motion/mputil.h,v $
// $Author: adurant $
// $Date: 2000/01/31 09:51:06 $
// $Revision: 1.5 $
//
// Multiped utilities
#pragma once

#ifndef __MPUTIL_H
#define __MPUTIL_H

#include <multiped.h>

typedef void MpFile;
typedef BOOL (* MpFileFunc)(MpFile* file, void* buf, int len); 
extern void MpWriteMultiped(multiped *mp, MpFileFunc write, MpFile *file);
extern void MpReadMultiped(multiped *mp, MpFileFunc read, MpFile *file);

#endif // __MPUTIL_H
