/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propstat.h,v 1.2 1998/10/01 18:40:37 mahk Exp $
#pragma once  
#ifndef __PROPSTAT_H
#define __PROPSTAT_H

////////////////////////////////////////////////////////////
// PROPERTY STATISTICS API 
//

typedef struct sPropTimeStats sPropTimeStats; 

#undef INTERFACE 
#define INTERFACE IPropertyStats
DECLARE_INTERFACE_(IPropertyStats,IUnknown)
{
   //
   // Standard IUnknown Stuff
   //
   DECLARE_UNKNOWN_PURE(); 

   //
   // Our stat gathering methods 
   //
   STDMETHOD_(sPropTimeStats*,GetTimeStats)(THIS) PURE; 

}; 


#endif // __PROPSTAT_H
