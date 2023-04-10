/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////
//
// $Header: r:/t2repos/thief2/src/framewrk/resdata.h,v 1.6 2000/01/31 09:47:09 adurant Exp $
//
#pragma once

//
// Interface for application-specific data about resources.
//
// Use these functions to get at the app data from this resource.
//

#ifndef __RESDATA_H
#define __RESDATA_H

//////////
//
// Internal definitions
//
// Do not use these directly! These are exposed here solely so that we can
// use fast macros to get at the application data!
//

#include <_animtxt.h>

typedef struct _appdataelem {
   IRes *pRes;
   int texidx;
   ectsAnimInfo *animdata;
   int next_free;
} appdataelem;

// The maximum number of resources that can have application data associated
// with them. This number is arbitrary, and should be tuned.

#define MAX_APP_RESOURCES 2048


EXTERN appdataelem appdata[MAX_APP_RESOURCES];

EXTERN int GetAppDataElem(IRes *pRes);

//////////
//
// API
//
// Use these functions and macros to get at the application data for
// resources.
//

// Setup and shutdown of the whole AppData mechanism:
EXTERN void InitAppData();
EXTERN void ShutdownAppData();

// Macros to obtain and set the texture and animation indeces for a
// given resource:
#define GetResTexIdx(pRes) (appdata[GetAppDataElem(pRes)].texidx)
#define SetResTexIdx(pRes,val) (appdata[GetAppDataElem(pRes)].texidx = val)
#define GetResAnimData(pRes) (appdata[GetAppDataElem(pRes)].animdata)
#define SetResAnimData(pRes,val) (appdata[GetAppDataElem(pRes)].animdata = val)

// WARNING: dont use this unless you are super special - use Release below
// Should be called just before doing the final release of a resource:
EXTERN void FreeAppRes(IRes *pRes);
// this is moronic... how do you know if you are about to do a final release
// hello?

// call this INSTEAD OF FreeAppRes
// it will do the release for you
// returns whether or not it was final _APPSIDE_ release
// if it wasnt, it doesnt do a FreeAppRes
// sneaky, eh?
EXTERN BOOL ReleaseAppRes(IRes *pRes);

#endif // __RESDATA_H
