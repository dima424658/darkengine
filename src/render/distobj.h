/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/distobj.h,v 1.2 2000/01/29 13:38:41 adurant Exp $
#pragma once

#ifndef _DISTOBJ_H_
#define _DISTOBJ_H_


// We are assuming only one implementation of this - the implementation file contains statics!

#include <skyint.h>
#include <lgd3d.h>
#include <r3d.h>


class cDistantArt : public ISkyObject
{
public:
   virtual void AppInit();
   virtual void Init();
   virtual void Term();
   virtual void Render();
};

#endif