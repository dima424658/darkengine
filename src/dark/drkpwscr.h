/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkpwscr.h,v 1.2 1998/08/08 23:33:40 dc Exp $

#pragma once
#ifndef __DRKPWSCR_H
#define __DRKPWSCR_H

DECLARE_SCRIPT_SERVICE(DrkPowerups, 0x153)
{
   // cause a world flash to trigger
   STDMETHOD_(void,TriggerWorldFlash)(object obj) PURE;

   // try to deploy an arch at src_obj, returns success
   // WILL go ahead and create the arch_obj if there is room
   STDMETHOD_(BOOL,ObjTryDeploy)(object src_object, object deploy_arch) PURE;

   // hack for now to allow cleaning up nearby blood
   STDMETHOD_(void,CleanseBlood)(object water_src_object, float rad) PURE;
};

#endif  // __DRKPWSCR_H
