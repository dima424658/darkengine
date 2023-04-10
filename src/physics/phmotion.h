/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmotion.h,v 1.4 2000/01/29 13:32:59 adurant Exp $
//
// Physics motion class header
//
#pragma once

#ifndef __PHMOTION_H
#define __PHMOTION_H

#include <matrixs.h>
#include <dynarray.h>

#include <phmoapi.h>

////////////////////////////////////////////////////////////////////////////////

class cPlayerMotion
{
public:
   
   /////////////////////////////////////
   //
   // Constructor / Destructor
   //
   cPlayerMotion();
   ~cPlayerMotion();

   /////////////////////////////////////
   //
   // Active motion accessors
   //
   void          Activate(ePlayerMotion motion);
   ePlayerMotion GetActive();

   void          ActivateList(ePlayerMotion *motions, int size);

   void          SetRest(ePlayerMotion restMotion);
   ePlayerMotion GetRest();

   void SetOffset(short submod, mxs_vector *offset);
   void GetOffset(short submod, mxs_vector *offset);

   /////////////////////////////////////
   //
   // Update
   //
   void Update(mxs_real dt);

private:
   
   ePlayerMotion m_activeMotion;
   mxs_real      m_timeDuration;
   mxs_real      m_timeHoldLength;

   ePlayerMotion m_restMotion;

   cDynArray<ePlayerMotion> m_motionList;

   cDynArray<mxs_vector>   m_curOffset;
   cDynArray<mxs_vector>   m_targOffset;

   mxs_real      m_timeActive;

};

////////////////////////////////////////////////////////////////////////////////

#endif // __PHMOTION_H










