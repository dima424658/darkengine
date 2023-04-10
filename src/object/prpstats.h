/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/prpstats.h,v 1.1 1998/10/01 18:40:47 mahk Exp $
#pragma once  
#ifndef __PRPSTATS_H
#define __PRPSTATS_H

enum ePropTimeStats
{
   kPropCopyTime,
   kPropGetDonorTime,
   kPropGetExemplarTime, 
   kPropIterStartTime,
   kPropIterNextTime,
   kPropIterStopTime,
   kPropCreateTime,
   kPropDeleteTime,
   kPropGetTime,
   kPropSetTime,
   kPropRelevantTime, 
   kPropListenerTime,
   kPropTouchTime, 
   kPropRequiresTime,
   kPropImpliesTime,
   kPropRebuildConcretesTime,

   kPropNumTimeStats
}; 

struct sPropTimeStats
{
   ulong time[kPropNumTimeStats]; 
};


#endif // __PRPSTATS_H
