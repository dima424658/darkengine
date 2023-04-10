/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipthqry.h,v 1.1 1998/11/03 16:24:48 TOML Exp $
//
//
//

#ifndef __AIPTHQRY_H
#define __AIPTHQRY_H

#include <comtools.h>

F_DECLARE_INTERFACE(IAIPathQuery);
struct mxs_vector;

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAIPathQuery
//

DECLARE_INTERFACE_(IAIPathQuery, IUnknown)
{
   //
   // Find the edge of the database. Returns TRUE if hit it.
   //
   STDMETHOD_(BOOL, CastForBoundary)(const mxs_vector & end, 
                                     mxs_vector * pHitLoc, 
                                     mxs_vector * pBoundVec) PURE;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIPTHQRY_H */
