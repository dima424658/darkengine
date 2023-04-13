/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/script/litscrpt.h,v 1.2 1999/01/27 20:33:55 XEMU Exp $
//
// litscrpt.h

#pragma once

#ifndef __LITSCRPT_H
#define __LITSCRPT_H

#include <animlgte.h>   // enums for animation modes

#include <scrptmsg.h>
#include <objtype.h>
#include <objscrt.h>


///////////////////////////////////////////////////////////////////////////////
//
// SERVICES
//

///////////////////////////////////////
//
// Light service
//
DECLARE_SCRIPT_SERVICE(Light, 0x16c)
{
   STDMETHOD_(void, Set)(const object& obj, int mode,
                         float min_brightness, float max_brightness) PURE;
   STDMETHOD_(void, SetMode)(const object& obj, int mode) PURE;
   STDMETHOD_(void, Activate)(const object& obj) PURE;
   STDMETHOD_(void, Deactivate)(const object& obj) PURE;
   STDMETHOD_(void, Subscribe)(const object& obj) PURE;
   STDMETHOD_(void, Unsubscribe)(const object& obj) PURE;
   STDMETHOD_(int, GetMode)(const object& obj) PURE;
};


///////////////////////////////////////////////////////////////////////////////
//
// MESSAGES
//


///////////////////////////////////////////////////////////////////////////////
//
// Base scripts
//


///////////////////////////////////////////////////////////////////////////////

#endif // __SNDSCRPT_H
