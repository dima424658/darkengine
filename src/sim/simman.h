/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/simman.h,v 1.3 1998/09/30 14:12:39 mahk Exp $
#pragma once  
#ifndef __SIMMAN_H
#define __SIMMAN_H
#include <comtools.h>
#include <idispatc.h>


////////////////////////////////////////////////////////////
// THE SIMULATION MANAGER (Agg Member)
//
// This is the channel through which "simulation" messages are passed.
// Someday, it may be responsible for the "sim state" as well. 
// 

//
// THE SIM MANAGER INTERFACE, DERIVED FROM IMessageDispatch
//

F_DECLARE_INTERFACE(ITagFile); 
F_DECLARE_INTERFACE(ISimManager); 

#undef INTERFACE
#define INTERFACE ISimManager
DECLARE_INTERFACE_(ISimManager,IMessageDispatch)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_DISPATCH_PURE(); 

   //
   // Init the sim, or resume it, whichever is appropriate
   //
   STDMETHOD(StartSim)(THIS) PURE;

   //
   // Suspend the sim if it's started, otherwise do nothing 
   //
   STDMETHOD(SuspendSim)(THIS) PURE;

   //
   // Stop the sim 
   //
   STDMETHOD(StopSim)(THIS) PURE; 
   
   //
   // Last Sim message sent 
   // 
   STDMETHOD_(ulong,LastMsg)(THIS) PURE;

   //
   // Do database stuff 
   //
   STDMETHOD(DatabaseMsg)(THIS_ ulong msg, ITagFile* file) PURE;
   
   

}; 

#define ISimManager_StartSim(p)           COMCall0(p, StartSim) 
#define ISimManager_SuspendSim(p)         COMCall0(p, SuspendSim) 
#define ISimManager_StopSim(p)            COMCall0(p, StopSim) 

#define ISimManager_LastMsg(p)            COMCall0(p, LastMsg) 
#define ISimManager_DatabaseMsg(p, a, b)  COMCall0(p, DatabaseMsg, a, b) 


EXTERN void SimManagerCreate(void); 


#endif // __SIMMAN_H


