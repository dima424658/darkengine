/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/objquer_.h,v 1.3 2000/01/29 13:24:00 adurant Exp $
#pragma once

#ifndef __OBJQUER__H
#define __OBJQUER__H
#include <objquery.h>
#include <objtype.h>

//
// Base object query.  Does COM stuff for you.  
//



class cBaseObjectQuery : public IObjectQuery 
{
#ifndef SHIP
   static int num_outstanding;
   static void delta(int dx) { num_outstanding += dx;};
#else
   static void delta(int dx) { } ;
#endif

public:
   DECLARE_UNAGGREGATABLE();
   cBaseObjectQuery() { delta(1);};
   
   virtual ~cBaseObjectQuery() { delta(-1);};

   
};



#endif // __OBJQUER__H


