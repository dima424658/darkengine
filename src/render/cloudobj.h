/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/cloudobj.h,v 1.2 2000/01/29 13:38:37 adurant Exp $
#pragma once

#ifndef _CLOUDOBJ_H_
#define _CLOUDOBJ_H_


// We are assuming only one implementation of this - the implementation file contains statics!

#include <skyint.h>

class cCloudDeck : public ISkyObject
{
public:
   virtual void AppInit();
   virtual void Init();
   virtual void Term();
   virtual void Render();
};


#endif
