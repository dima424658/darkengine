/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aidetect.cpp,v 1.21 2000/02/28 18:32:54 adurant Exp $
//

// #define PROFILE_ON 1

#include <lg.h>
#include <mprintf.h>

#include <appagg.h>

#include <mtagvals.h>

#include <aiactmot.h>
#include <aiapibhv.h>
#include <aibasabl.h>
#include <aigoal.h>
#include <aiutils.h>
#include <aiactloc.h>
#include <aidetect.h>
#include <aiapiiai.h>
#include <aiman.h>
#include <aisndtyp.h>
#include <aisound.h>
#include <aisuslnk.h>
#include <aitagtyp.h>
#include <aiteams.h>
#include <aiwr.h>
#include <susprop.h>
#include <rendprop.h>
#include <iobjsys.h>
#include <objdef.h>
#include <objquery.h>

//#define DARK
#ifdef THIEF
#include <drkstats.h>
#endif

#include <objpos.h>
#include <portal.h>

#include <aiprops.h>

// Must be last header
#include <dbmem.h>



// From aibassns.cpp:
extern IBoolProperty * g_pAIOnlyNoticesPlayerProperty;
#define AIGetOnlyNoticesPlayer(obj) AIGetProperty(g_pAIOnlyNoticesPlayerProperty, (obj), (BOOL)FALSE)

extern IBoolProperty * g_pAINoticesBodiesProperty;
#define AIGetNoticesBodies(obj) AIGetProperty(g_pAINoticesBodiesProperty, (obj), (BOOL)TRUE)


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDetect
//

STDMETHODIMP_(const char *) cAIDetect::GetName()
{
   return "Detect ability";
}

///////////////////////////////////////

// constructor and destructor
cAIDetect::cAIDetect()
   : m_SeenBody(FALSE), m_LastSeen(OBJ_NULL), m_Timer(kAIT_2Sec)
{
}

STDMETHODIMP_(void) cAIDetect::Init()
{
   // YO!! if you want to try working on this, uncomment this line
   SetNotifications(kAICN_ActionProgress);
   m_Timer.Reset();
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIDetect::Save(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Detect, 0, 0, pTagFile))
   {
      SaveNotifications(pTagFile);

      m_Timer.Save(pTagFile);
      AITagMove(pTagFile, &m_SeenBody);
      AITagMove(pTagFile, &m_LastSeen);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIDetect::Load(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Detect, 0, 0, pTagFile))
   {
      LoadNotifications(pTagFile);

      m_Timer.Load(pTagFile);
      AITagMove(pTagFile, &m_SeenBody);
      AITagMove(pTagFile, &m_LastSeen);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

DECLARE_TIMER(cAIDetect_OnActionProgress, Average);

// see if it is time for a new action
STDMETHODIMP_(void) cAIDetect::OnActionProgress(IAIAction * pAction)
{  // If we're in a good position to interrupt...
   AUTO_TIMER(cAIDetect_OnActionProgress);

   if (pAction->GetResult() > kAIR_NoResult)
      if (m_Timer.Expired())
      {  // lets check again
         if (SearchForBodies())
         {
            m_SeenBody=TRUE;  // for now, each AI can see one body ever
         }
         SearchForSuspicious();
         m_Timer.Reset();
      }
}

///////////////////////////////////////

// actually do the work

DECLARE_TIMER(cAIDetect_SearchForBodies, Average);

BOOL cAIDetect::SearchForBodies(void)
{
   AUTO_TIMER(cAIDetect_SearchForBodies);

   BOOL found_any=FALSE;
   BOOL all_robot=TRUE;

#ifndef THIEF
   //since only Thief2 has robots at the moment (at least for bodies), then
   //clearly the bodies aren't robots if not Thief2
   all_robot = FALSE;
#endif

   tAIIter iAIIter;
   ObjPos *ourLoc=ObjPosGet(GetID());
   BOOL bOnlyNoticesPlayer;
   BOOL bNoticesBodies;

   if (m_SeenBody || m_pAIState->GetMode() != kAIM_Normal)
      return FALSE;

   mxs_matrix tempmatrix;
   mx_ang2mat(&tempmatrix,&ourLoc->fac);
   const mxs_vector ourvecfac = tempmatrix.vec[0];

   bOnlyNoticesPlayer = AIGetOnlyNoticesPlayer(GetID());
   bNoticesBodies = AIGetNoticesBodies(GetID());

   /// mprintf("Body Count\n");
   AutoAppIPtr(AIManager);
   IAI *pAI=pAIManager->GetFirst(&iAIIter);
   while (pAI)
   {
      ObjID targ=pAI->GetObjID();
      const cAIState *pAIState=((IInternalAI *)pAI)->GetState();
      if (ObjHasRefs(targ) && pAIState->IsDead())
      {
         if ((GetID() != targ) &&
             (AITeamCompare(GetID(), targ) == kAI_Teammates) &&
             !bOnlyNoticesPlayer &&
             bNoticesBodies)
         {
            #define kBodyLookDistSq sq(15.0)
            #define kBodyLighting 0.15

            //make sure they are in front of us, roughly.
            #define kBodyDotMinimum 0.0
            ObjPos *themLoc=ObjPosGet(targ);
            float distSq;
            mxs_vector relfac;
            mx_sub_vec(&relfac,&themLoc->loc.vec,&ourLoc->loc.vec);

            // check distSqance
            distSq=m_pAIState->DistSq(themLoc->loc.vec);

            if ((distSq < kBodyLookDistSq) && (mx_dot_vec(&relfac,&ourvecfac) >= kBodyDotMinimum))
            {  // need to check lighting and raycast
               Location hit;
               if (PortalRaycast(&themLoc->loc,&ourLoc->loc,&hit,0))
               {  // should check verse objects
                  float light = AIGetObjectLighting(targ);

                  if (light > kBodyLighting)
                  {
                     m_LastSeen=targ;
                     found_any=TRUE;
#ifdef THIEF
                     //if not a robot, then clearly we found a non robot
                     if (!DarkStatCheckBit(targ,kDarkStatBitRobot))
                        all_robot=FALSE;
#endif
                  }
               }
            }
         }
      }
      pAI->Release();
      pAI=pAIManager->GetNext(&iAIIter);
   }
   pAIManager->GetDone(&iAIIter);
   if (found_any)
   {
#ifdef THIEF
      if (!DarkStatCheckBit(m_LastSeen,kDarkStatBitFoundBody))
      {
         DarkStatInc(kDarkStatBodiesFound);
         DarkStatSetBit(m_LastSeen,kDarkStatBitFoundBody,TRUE);
      }
#endif
      m_pAI->NotifyFoundBody(m_LastSeen);
      if (m_pAI->AccessSoundEnactor())
//real people take priority over robots (well, maybe not for Karras), but
//in general.  So if not every corpse is a robot, scream bloody murder.
//only if every corpse is a robot should we scream oily vandalism.
      if (all_robot==FALSE)
         m_pAI->AccessSoundEnactor()->RequestConcept(kAISC_FoundBody);
      else
         m_pAI->AccessSoundEnactor()->RequestConcept(kAISC_FoundRobot);
      }

   return found_any;
}

DECLARE_TIMER(cAIDetect_SearchForSuspicious, Average);

BOOL cAIDetect::SearchForSuspicious(void)
{
   AUTO_TIMER(cAIDetect_SearchForSuspicious);

   BOOL found_any=FALSE;

   ObjPos *ourLoc=ObjPosGet(GetID());
   ObjID m_LastSuspicious = OBJ_NULL;

   mxs_matrix tempmatrix;
   mx_ang2mat(&tempmatrix,&ourLoc->fac);
   const mxs_vector ourvecfac = tempmatrix.vec[0];


   if (m_pAIState->GetMode() != kAIM_Normal)
      return FALSE;

//   AutoAppIPtr(ObjectSystem);

//   IObjectQuery* objquery = pObjectSystem->Iter(kObjectConcrete);

   sPropertyObjIter iter;

   StartSuspiciousIter(&iter);

   ObjID targ;
   BOOL iscurrentsuspicious;

//   NextSuspiciousIter(&iter,&targ,&iscurrentsuspicious);

//   for (; !objquery->Done(); objquery->Next())
//     {
//       ObjID targ = objquery->Object();


   for (; (NextSuspiciousIter(&iter,&targ,&iscurrentsuspicious));)
   {
     if (!OBJ_IS_CONCRETE(targ)) continue;  //only concrete objs.

       if ((iscurrentsuspicious && (ObjHasRefs(targ)) &&
           (!AIIsSuspiciousOfObj(GetID(),targ)))
         ||
           ((!iscurrentsuspicious && (ObjHasRefs(targ)) &&
            (AIIsSuspiciousOfObj(GetID(),targ)))))
         //is it a suspicious object? (and I'm not already suspicious of it)
         //or, alternatively, it isn't a suspicious object, and I AM
         //suspicious of it... (which means I shouldn't be suspicious of it
         //anymore).
         {
#define kSuspiciousLookDistSq sq(15.0)
#define kSuspiciousLighting 0.15
#define kSuspiciousDotMinimum 0.0
          ObjPos *themLoc=ObjPosGet(targ);

          if (!themLoc) continue;

          float distSq;
          float minlightlevel;

          if (!GetSuspiciousLightLevel(targ,&minlightlevel))
             minlightlevel = kSuspiciousLighting;

          mxs_vector relfac;
          mx_sub_vec(&relfac,&themLoc->loc.vec,&ourLoc->loc.vec);

          // check distSqance
          distSq=m_pAIState->DistSq(themLoc->loc.vec);
          //close enough, and facing the right way.
          if ((distSq < kSuspiciousLookDistSq) &&
              (mx_dot_vec(&relfac,&ourvecfac) >= kSuspiciousDotMinimum))
            {  // need to check lighting and raycast
              Location hit;
              if (PortalRaycast(&themLoc->loc,&ourLoc->loc,&hit,0))
                {  // should check verse objects
                  float light = AIGetObjectLighting(targ);

                  if (light > minlightlevel)
                    {
                      if (!AIIsSuspiciousOfObj(GetID(),targ))
                        {
                          m_LastSuspicious=targ;
                          //ok, remember that I'm suspicious of it.
                          AICreateSuspiciousLink(GetID(),targ);
                          found_any = TRUE;
                        }
                      else
                        {
                          //we were suspicious of it, but shouldn't
                          //be now... so become unsuspicious of it.
                          AIClearSuspiciousLinks(GetID(),targ);
                        }
                    }
                }
            }//end dist check
        } //end if suspicious/not suspicious
    } //end for
//   objquery->Release();

   StopSuspiciousIter(&iter);

   if (found_any)
     m_pAI->NotifyFoundSuspicious(m_LastSuspicious);

   return found_any;
}
