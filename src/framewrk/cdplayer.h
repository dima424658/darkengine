/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/cdplayer.h,v 1.2 2000/01/29 13:20:23 adurant Exp $
#pragma once

#ifndef __CDPLAYR_H
#define __CDPLAYR_H

F_DECLARE_INTERFACE(ICDPlayer); 

#undef INTERFACE
#define INTERFACE ICDPlayer

DECLARE_INTERFACE_(ICDPlayer,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   STDMETHOD(Init)(void) PURE;

   // basic low-level CD APIs
   STDMETHOD(CDPlayTrack)(int track) PURE;
   STDMETHOD(CDStopPlay)(void) PURE;

   // higher-level music control
   STDMETHOD(CDSetBGMTrack)(int track) PURE;
   STDMETHOD(CDPlayTransition)(int track, uint flags) PURE;
   STDMETHOD(Update)(void) PURE;
};

EXTERN void CDPlayerCreate();
#undef INTERFACE

#endif