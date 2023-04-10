/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/skyint.h,v 1.2 2000/01/31 09:53:19 adurant Exp $
#pragma once

#ifndef _SKYINT_H_
#define _SKYINT_H_


///////////////////////////////////////////
// basic interface for sky stuff:
class ISkyObject
{
public:
   virtual void AppInit() = 0;
   virtual void Init() = 0;
   virtual void Term() = 0;
   virtual void Render() = 0;
};


#endif