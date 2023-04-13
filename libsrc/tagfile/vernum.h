/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/tagfile/RCS/vernum.h 1.1 1998/09/28 13:50:47 CMONTER Exp $

#ifndef VERNUM_H
#define VERNUM_H

typedef struct VersionNum
{
   uint major;
   uint minor;
} VersionNum; 

#define VersionNumsEqual(v1,v2) ((v1)->major == (v2)->major \
                                 && (v1)->minor == (v2)->minor)

   // strcmp()-like api
EXTERN int VersionNumsCompare(const VersionNum* v1, const VersionNum* v2);

EXTERN const char* VersionNum2String(const VersionNum* v);

#endif // VERNUM_H
