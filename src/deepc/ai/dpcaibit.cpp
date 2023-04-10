/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <dpcaibit.h>

#include <appagg.h>
#include <comtools.h>
#include <matrix.h>

#include <stimbase.h>
#include <stimsens.h>
#include <stimul8r.h>

#include <objpos.h>
#include <physapi.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////
//
// Bite.
//

void cAIBiteAction::Bite(void)
{
   AutoAppIPtr(StimSensors); 
   StimSensorID sensid = pStimSensors->LookupSensor(m_targetObjID, m_stimID); 
   
   if (sensid != SENSORID_NULL)
   {
      AutoAppIPtr(Stimulator); 
      sStimEventData data = { m_stimID, m_intensity, 0.0, sensid, 0, GetSimTime() }; 
      sStimEvent event(&data); 
      
      pStimulator->StimulateSensor(sensid, &event); 
   }
   SetState(kAIBiteActionBitten);
}

////////////////////////////////////////
//
// Leap up
// 

void cAIBiteAction::Leap(void)
{
   mxs_vector launchVec;
   ObjPos *pPos = ObjPosGet(m_pAIState->GetID());
   
   mx_rot_z_vec(&launchVec, &cMxsVector(m_leapSpeedX, 0, m_leapSpeedZ), pPos->fac.tz);
   PhysSetVelocity(m_pAIState->GetID(), &launchVec);
   SetState(kAIBiteActionLeaping);
}

///////////////////////////////////////
//
// Update the action.
//

STDMETHODIMP_(eAIResult) cAIBiteAction::Update()
{
   if (((m_state == kAIBiteActionBitten) || (m_state == kAIBiteActionLeaping)) && PhysObjOnGround(m_pAIState->GetID()))
   {
      result = kAIR_Success;
   }
   else
   {
      result = kAIR_NoResult;
   }
   return result;
}

///////////////////////////////////////
//
// Enact the action.
//

STDMETHODIMP_(eAIResult) cAIBiteAction::Enact(ulong deltaTime)
{
   switch (m_state)
   {
      case kAIBiteActionStart:
      {
         Leap();
      }
      case kAIBiteActionLeaping:
      {
         cMxsVector      dest;
         GetObjLocation(m_targetObjID, &dest);
         const float targDistSq = m_pAIState->DistSq(dest);
         if (targDistSq <= (m_biteDist*m_biteDist))
         {
            Bite();
         }
         break;
      }
      case kAIBiteActionBitten:
      {
         break;
      }
   }
   return result;
}

///////////////////////////////////////////////////////////////////////////////

