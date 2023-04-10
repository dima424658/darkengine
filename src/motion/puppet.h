/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/puppet.h,v 1.7 2000/01/31 09:49:57 adurant Exp $
#pragma once

#ifndef __PUPPET_H
#define __PUPPET_H

#include <objtype.h>
#include <property.h> // for listener structs

#ifdef __cplusplus

#include <mclntapi.h>
#include <cbllmapi.h>

typedef struct sPuppetMnvrRequest
{
   eManeuverTransitionType trans;
   sMcMoveParams params;
} sPuppetMnvrRequest;

typedef class cPuppet : public ICerebellum
{
public:
   cPuppet(ObjID objID=OBJ_NULL);
   cPuppet(const cPuppet& );
   ~cPuppet();

   virtual void SetOwner(ObjID obj);
   ObjID GetOwner() { return m_ObjID; };

   virtual void PlayMotion(const char *name, eManeuverTransitionType trans=kMnvrTrans_Immediate, eManeuverPriority=kMnvrPri_Script);
   virtual void Update(ulong deltaTime);

   void ResetMotor();

   void SetManeuverRequest(sPuppetMnvrRequest *pRequest);
   sPuppetMnvrRequest *GetManeuverRequest() { return m_pMnvrRequest; }

private:
   ObjID m_ObjID;
   sPuppetMnvrRequest *m_pMnvrRequest;
   IMotionCoordinator *m_pMCoord;
} cPuppet;


EXTERN ICerebellum *PuppetGetCerebellum(ObjID obj);

#endif // __cplusplus


EXTERN void PuppetsInit();
EXTERN void PuppetsUpdate(ulong dt);
EXTERN void LGAPI PuppetCreatureListener(sPropertyListenMsg* msg, PropListenerData data);

EXTERN BOOL ObjIsPuppet(ObjID obj);

#endif
