/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/lazyagg.h,v 1.1 1997/12/31 00:08:21 mahk Exp $
#pragma once  
#ifndef __LAZYAGG_H
#define __LAZYAGG_H

#include <comtools.h>
#include <appagg.h>

////////////////////////////////////////////////////////////
// A "Lazy" Aggregate member.  
// AppGetObj's itself the first time it's needed.  
// 
// Never releases itself 
//

template<class IFACE, const GUID* guid> 
class cLazyAggMember
{

protected:
   IFACE* iptr; 

public:
   cLazyAggMember() :iptr(NULL) {}; 

   operator IFACE* () 
   { 
      if (iptr == NULL) 
         iptr = (IFACE*)_AppGetAggregated(IID_TO_REFIID(*guid)); 
      return iptr; 
   }

   IFACE* operator ->() 
   { 
      return operator IFACE*(); 
   };

};


#define LazyAggMember(iface) cLazyAggMember<iface,&IID_##iface> 

#endif // __LAZYAGG_H



