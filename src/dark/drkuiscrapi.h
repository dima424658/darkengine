#pragma once

#ifndef __DRKUISCRAPI_H
#define __DRKUISCRAPI_H

#include <scrptbas.h>
#include <objscrt.h>

#define DEFAULT_TIMEOUT -1001

DECLARE_SCRIPT_SERVICE(DarkUI,0x19f)
{
   STDMETHOD(TextMessage)(const char* message, int color = 0,
      int timeout = DEFAULT_TIMEOUT) PURE;
   STDMETHOD(ReadBook)(const char* text, const char* art) PURE;
   STDMETHOD_(object,InvItem)() PURE;
   STDMETHOD_(object,InvWeapon)() PURE;
   STDMETHOD(InvSelect)(object obj) PURE;
   STDMETHOD_(boolean,IsCommandBound)(const string & cmd) PURE;
   STDMETHOD_(string,DescribeKeyBinding)(const string & cmd) PURE;
};

#endif
