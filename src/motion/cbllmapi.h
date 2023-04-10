/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/cbllmapi.h,v 1.3 2000/01/31 09:50:03 adurant Exp $
// This module provides the abstract interface to a cerebellum, to be used
// by higher-level systems 
#pragma once

#ifndef __CBLLMAPI_H
#define __CBLLMAPI_H

#include <objtype.h>
#include <cbllmtyp.h>

#ifdef __cplusplus

class ICerebellum
{
public:
   virtual ~ICerebellum() {}
   virtual void PlayMotion(const char *name, eManeuverTransitionType trans=kMnvrTrans_Immediate, eManeuverPriority priority=kMnvrPri_Script) = 0;
   virtual void Update(ulong deltaTime) = 0;
};

EXTERN ICerebellum *CbllmGetFromObj(ObjID obj);

#endif // __cplusplus

#endif // __CBLLMAPI_H
