/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aidetect.h,v 1.4 2000/01/24 12:12:00 adurant Exp $
//

#ifndef __AIDETECT_H
#define __AIDETECT_H

#include <aiapicmp.h>
#include <aibascmp.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDetect (adjunct to senses)
//

class cAIDetect : public cAIComponentBase<IAIComponent, &IID_IAIComponent>
{
public:
   // construct
   cAIDetect();

   // Standard component methods
   STDMETHOD_(const char *, GetName)(void);
   STDMETHOD_(void, Init)(void);
   
   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   // Update the status of the current goal
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);

   
private:
   // helper call that does the real work
   BOOL SearchForBodies(void);

   BOOL SearchForSuspicious(void);

   // our data
   cAITimer m_Timer;
   BOOL     m_SeenBody;
   ObjID    m_LastSeen;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIDETECT_H */
