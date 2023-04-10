/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/damgscrs.h,v 1.4 1998/08/15 23:32:14 mahk Exp $
#pragma once  
#ifndef __DAMGSCRS_H
#define __DAMGSCRS_H

#include <objscrt.h>

DECLARE_SCRIPT_SERVICE(Damage,0xFE)
{
   //
   // Damage an Object
   //
   STDMETHOD(Damage)(object victim, object culprit, integer how_much, integer what_kind = 0) PURE; 

   //
   // Slay an object
   //
   STDMETHOD(Slay)(object victim, object culprit) PURE; 

   //
   // Resurrect an object
   //
   STDMETHOD(Resurrect)(object victim, object culprit = object(0)) PURE; 


};



#endif // __DAMGSCRS_H
