/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/imottab.h,v 1.4 2000/01/29 13:22:10 adurant Exp $
#pragma once

#ifndef __IMOTTAB_H
#define __IMOTTAB_H

#include <comtools.h>

#ifdef __cplusplus
#define CONSTFUNC const
#else
#define CONSTFUNC
#endif 

#undef INTERFACE 
#define INTERFACE IMotionTable
DECLARE_INTERFACE_(IMotionTable,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   // get information about static motion array.  
   // this should only be used for motion loading purposes.
   // and should go away or change once the world is happily dynamic.
   STDMETHOD_(int,NumPossibleMotions)(THIS) CONSTFUNC PURE;
   STDMETHOD_(char*,GetNameFromHardIndex)(THIS_ int index) CONSTFUNC PURE;
   STDMETHOD_(void,AddMotionFromHardIndex)(THIS_ int index, int motionNum) PURE;

   // get information about a motion given its motion number
   STDMETHOD_(int,GetType)(THIS_ const int motionNum) CONSTFUNC PURE;
   STDMETHOD_(BOOL,NeckIsFixed)(THIS_ const int motionNum) CONSTFUNC PURE;
   STDMETHOD_(char*,GetName)(THIS_ const int motionNum) PURE;
   STDMETHOD_(int,GetBlendType)(THIS_ const int motionNum) CONSTFUNC PURE;
   STDMETHOD_(int,GetNumMotions)(THIS) CONSTFUNC PURE;

   // get the motion number for a motion based on other information
   STDMETHOD_(int,NameGetNum)(THIS_ const char *name) CONSTFUNC PURE;
   STDMETHOD_(int,TypeGetAllNums)(THIS_ const int type, int *pMotNums, int *maxMots) CONSTFUNC PURE;
   STDMETHOD_(int,TypeGetNumFast)(THIS_ const int type) CONSTFUNC PURE;

};

#endif // __IMOTTAB_H



