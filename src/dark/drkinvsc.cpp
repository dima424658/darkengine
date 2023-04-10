/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkinvsc.cpp,v 1.3 2000/02/19 12:27:19 toml Exp $
//

#include <lg.h>

// script headers
#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>
#include <objscrpt.h>
#include <plyrmode.h>

// drk inv setup
#include <drkinv.h>
#include <drkinvsc.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

DECLARE_SCRIPT_SERVICE_IMPL(cDarkInvSrv, DrkInv)
{
public:
   // change the current inventory capabilities...
   STDMETHOD_(void,CapabilityControl)(eDrkInvCap cap_change, eDrkInvControl control)
   {
      drkInvCapControl(cap_change,control);
   }

   // set the speed factor for the player (based on what they are holding)
   STDMETHOD_(void,AddSpeedControl)(const char *name, float speed_fac, float rot_fac)
   {
      AddSpeedScale(name, speed_fac, rot_fac);
   }

   STDMETHOD_(void,RemoveSpeedControl)(const char *name)
   {
      RemoveSpeedScale(name);
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cDarkInvSrv, DrkInv);
