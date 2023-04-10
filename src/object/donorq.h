/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/donorq.h,v 1.2 2000/01/29 13:23:02 adurant Exp $
#pragma once
#ifndef __DONORQ_H
#define __DONORQ_H
#include <comtools.h>
#include <objquery.h>
#include <traittyp.h>

////////////////////////////////////////////////////////////
// DONOR QUERY INTERFACE
//
// An object query with some other useful stuff
//

typedef tMetaPropertyPriority tDonorPriority;

F_DECLARE_INTERFACE(IDonorQuery);

#undef INTERFACE
#define INTERFACE IDonorQuery

DECLARE_INTERFACE_(IDonorQuery,IObjectQuery)
{
   DECLARE_UNKNOWN_PURE();

   //
   // IObjectQuery Methods
   //
   STDMETHOD_(BOOL,Done)(THIS) PURE;
   STDMETHOD_(ObjID,Object)(THIS) PURE; 
   STDMETHOD(Next)(THIS) PURE;

   //
   // Get the priority of the current donor.
   //
   STDMETHOD_(tDonorPriority,Priority)(THIS) PURE;

};

#undef INTERFACE


#endif // __DONORQ_H
