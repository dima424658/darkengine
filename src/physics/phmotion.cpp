/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmotion.cpp,v 1.5 1998/06/19 14:02:15 CCAROLLO Exp $
//
// Physics motion class   
//

#include <lg.h>
#include <matrixs.h>
#include <dynarray.h>

#include <playrobj.h>

#include <physapi.h>
#include <phcore.h>
#include <phmods.h>
#include <phmod.h>

#include <phmotion.h>
#include <phmodata.h>

// Must be last header
#include <dbmem.h>


#define PLAYER_SUB_MODELS (3)


////////////////////////////////////////////////////////////////////////////////

cPlayerMotion::cPlayerMotion()
{
   m_curOffset.SetSize(PLAYER_SUB_MODELS);
   m_targOffset.SetSize(PLAYER_SUB_MODELS);

   for (int i=0; i<PLAYER_SUB_MODELS; i++)
   {
      mx_zero_vec(&m_curOffset[i]);
      mx_zero_vec(&m_targOffset[i]);
   }

   Activate(kMoNormal);
   SetRest(kMoNormal);
}

////////////////////////////////////////

cPlayerMotion::~cPlayerMotion()
{
   m_curOffset.SetSize(0);
   m_targOffset.SetSize(0);
}

////////////////////////////////////////////////////////////////////////////////

void cPlayerMotion::Activate(ePlayerMotion motion)
{
   if (((m_activeMotion == kMoDisable) && (motion != kMoEnable)) || // disabled, no re-enabling
       ((motion == m_activeMotion)))                                // not the same motion
      return;

   m_activeMotion = motion;
   for (int i=0; i<PLAYER_SUB_MODELS; i++)
      mx_zero_vec(&m_targOffset[i]);

   if ((motion >= kMoDisable) || (motion < kMoNormal))
      return;

   AssertMsg(PlayerMotionTable[motion].motion == motion, "Player motion table out of sync!");

   m_timeDuration = PlayerMotionTable[motion].duration;
   m_timeHoldLength = PlayerMotionTable[motion].hold_len;
   m_timeActive = 0.0;

   for (i=0; i<PlayerMotionTable[motion].numOffsets; i++)
      m_targOffset[PlayerMotionTable[motion].offset[i].submod] = PlayerMotionTable[motion].offset[i].vec;
}

////////////////////////////////////////

ePlayerMotion cPlayerMotion::GetActive()
{
   return m_activeMotion;
}

////////////////////////////////////////

void cPlayerMotion::ActivateList(ePlayerMotion *motions, int size)
{
   m_motionList.SetSize(0);

   for (int i=size-1; i>=1; i--)
      m_motionList.Append(motions[i]);

   // force an activation
   m_activeMotion = kMoInvalid;
   Activate(motions[0]);
}

////////////////////////////////////////

void cPlayerMotion::SetRest(ePlayerMotion restMotion)
{
   m_restMotion = restMotion;
}

////////////////////////////////////////

ePlayerMotion cPlayerMotion::GetRest()
{
   return m_restMotion;
}

////////////////////////////////////////////////////////////////////////////////

void cPlayerMotion::SetOffset(short submod, mxs_vector *offset)
{
   if (submod<PLAYER_SUB_MODELS)
      m_curOffset[submod] = *offset;
   m_timeActive = 0.0;
}

////////////////////////////////////////

void cPlayerMotion::GetOffset(short submod, mxs_vector *offset)
{
   if (submod<PLAYER_SUB_MODELS)
      *offset = m_curOffset[submod];
   else 
      mx_zero_vec(offset);
}

////////////////////////////////////////////////////////////////////////////////

void cPlayerMotion::Update(mxs_real dt)
{
   mxs_real   sum_offsets;
   mxs_vector offset_delta;
   int i;

   m_timeActive += (dt / 1000);

   sum_offsets = 0;
   for (i=0; i<PLAYER_SUB_MODELS; i++)
   {
      if ((m_timeDuration > 0.0) && (m_timeActive < m_timeDuration))
      {
         mx_sub_vec(&offset_delta, &m_targOffset[i], &m_curOffset[i]);
         mx_scale_addeq_vec(&m_curOffset[i], &offset_delta, m_timeActive / m_timeDuration);
         sum_offsets += mx_mag2_vec(&offset_delta); 
      }
      else
         m_curOffset[i] = m_targOffset[i];
   }

   if (m_timeHoldLength > 0.0) 
   {
      if (m_timeActive > (m_timeDuration + m_timeHoldLength))
      {
         if (m_motionList.Size() == 0)
           Activate(m_restMotion);
         else
         {
            Activate(m_motionList[m_motionList.Size() - 1]);
            m_motionList.DeleteItem(m_motionList.Size() - 1);
         }
      }
   }
   else
   {
      if ((m_motionList.Size() > 0) && (sum_offsets < 0.01) && (m_timeHoldLength == 0))
      {
         Activate(m_motionList[m_motionList.Size() - 1]);
         m_motionList.FastDeleteItem(m_motionList.Size() - 1);
      }
   }
}









