/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/cdscript.h,v 1.2 2000/01/29 13:39:51 adurant Exp $
#pragma once

#ifndef __CDSCRIPT_H
#define __CDSCRIPT_H

DECLARE_SCRIPT_SERVICE(CD, 0x226)
{
   STDMETHOD(SetBGM)(int track) PURE;
   STDMETHOD(SetTrack)(int track, uint flags) PURE;
};

#endif
