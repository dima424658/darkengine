/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/objquery.h,v 1.4 2000/01/29 13:23:59 adurant Exp $
#pragma once

#ifndef __OBJQUERY_H
#define __OBJQUERY_H
#include <comtools.h>
#include <objtype.h>

F_DECLARE_INTERFACE(IObjectQuery);

#undef INTERFACE 
#define INTERFACE IObjectQuery

DECLARE_INTERFACE_(IObjectQuery,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   // 
   // Are there any more objects in the query
   //
   STDMETHOD_(BOOL,Done)(THIS) PURE;

   //
   // Get the Query's current object
   //
   STDMETHOD_(ObjID,Object)(THIS) PURE; 

   //
   // Move to the next object in the query
   //
   STDMETHOD(Next)(THIS) PURE;
}; 

#undef INTERFACE

#define IObjectQuery_Done(p)  COMCall0(p, Done)
#define IObjectQuery_Object(p)   COMCall0(p, Object)
#define IObjectQuery_Next(p)  COMCall0(p, Next)

EXTERN IObjectQuery* CreateEmptyObjectQuery(void);
EXTERN IObjectQuery* CreateSingleObjectQuery(ObjID obj);
EXTERN IObjectQuery* ConcatenateObjectQueries(IObjectQuery** Qvec, int n);

// turn a link query into an object query
F_DECLARE_INTERFACE(ILinkQuery);
EXTERN IObjectQuery* CreateLinkDestObjectQuery(ILinkQuery* Q);

#endif // __OBJQUERY_H
