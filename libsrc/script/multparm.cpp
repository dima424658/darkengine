/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/multparm.cpp,v 1.2 1997/12/22 16:19:21 TOML Exp $
//
//
//

#include <multparm.h>

cMultiParm::uScratch cMultiParm::gm_scratch;

void cMultiParm::VecToTempString() const
{
   sprintf(gm_scratch.szBuf, "%f, %f, %f", pVec->x, pVec->y, pVec->z);
}
