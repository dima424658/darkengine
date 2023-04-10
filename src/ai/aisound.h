/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aisound.h,v 1.13 1999/05/10 10:45:56 TOML Exp $
//
//

#ifndef __AISOUND_H
#define __AISOUND_H

#include <aiapisnd.h>
#include <aibascmp.h>
#include <aisndtyp.h>
#include <ctagset.h>

#pragma once
#pragma pack(4)

F_DECLARE_INTERFACE(IAIConverse);

class IMotionCoordinator;
struct sAIMotionRequest;

struct sAISoundConceptDesc;

EXTERN ObjID g_AISoundDebugObj;

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitSoundEnactor(IAIManager *);
BOOL AITermSoundEnactor();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISoundEnactor
//

typedef cAIComponentBase<IAISoundEnactor, &IID_IAISoundEnactor> cAISoundEnactorBase;

class cAISoundEnactor : public cAISoundEnactorBase
{
public:   
   cAISoundEnactor();

   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();
   STDMETHOD_(void, End)();
   STDMETHOD_(void, OnModeChange)(eAIMode previous, eAIMode mode);

   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   //
   // Halt th current concept
   //
   STDMETHOD_(void, HaltCurrent)();

   //
   // Attempt to play a sound concept
   //
   STDMETHOD (RequestConcept)(eAISoundConcept, const cTagSet *);
   STDMETHOD (RequestConcept)(const char * pszConcept, const cTagSet *);

   // @HACK for E3 to allow scripts to stop all speech from this AI
   BOOL m_fSpeechStop;

private:
   HRESULT DoRequestConcept(sAISoundConceptDesc *, const cTagSet *);

#ifdef CONCEPT_PLAYBACK_RESTRICT
   unsigned m_ConceptTimes[kAISC_CoreNum];
#endif
   cAITimer m_ReacquireTimer;
   
   enum eFlags
   {
      kPlayedSpotPlayer = 0x01
   };
   
   eAISoundConcept m_LastConceptPlayed;
   cTagSet         m_LastTagsPlayed;
   
   unsigned m_flags;
   
   IAIConverse * m_pConverse;
};

///////////////////////////////////////

inline cAISoundEnactor::cAISoundEnactor()
 : m_ReacquireTimer(AICustomTime(3*60*1000)),
   m_flags(0),
   m_LastConceptPlayed(kAISC_CoreBase),
   m_pConverse(NULL)
{
   m_fSpeechStop = FALSE;
#ifdef CONCEPT_PLAYBACK_RESTRICT
   memset(&m_ConceptTimes, 0, sizeof(m_ConceptTimes));
#endif
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AISOUND_H */
