/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkcs.cpp,v 1.3 1999/03/02 15:16:04 mahk Exp $
// Commands associated with cut-scene recording/playback

#ifndef SHIP

#include <shkcs.h>

#include <appagg.h>
#include <comtools.h>
#include <mprintf.h>

#include <aiapisns.h>
#include <aicnvman.h>
#include <aiman.h>
#include <iobjsys.h>
#include <viewmgr.h>

#include <shkcmapi.h>

// Include these absolutely last
#include <dbmem.h>

EXTERN void CameraPositionSpew(void)
{
#ifdef EDITOR
   mxs_vector *pLoc;
   mxs_angvec *pAng;

   vm_spotlight_loc(&pLoc, &pAng);
   mprintf("%g %g %g, %f %f %f", pLoc->x, pLoc->y, pLoc->z, pAng->tx, pAng->ty, pAng->tz);
#endif 
}

EXTERN void ShockCameraAttach(char *pObjName)
{
   AutoAppIPtr_(ObjectSystem, pObjSys);
   ObjID objID;

   AutoAppIPtr(ShockCamera);
   if (strlen(pObjName) == 0)
      pShockCamera->Attach("Player");
   else if ((objID = pObjSys->GetObjectNamed(pObjName)) == OBJ_NULL)
      mprintf("No object named %s\n", pObjName);
   else
      pShockCamera->Attach(pObjName);
}

EXTERN void SetCameraEditStateRecord(void)
{
   AutoAppIPtr(ShockCamera);
   pShockCamera->SetEditState(kCameraRecord);
}

EXTERN void SetCameraEditStateEdit(void)
{
   AutoAppIPtr(ShockCamera);
   pShockCamera->SetEditState(kCameraEdit);
}

EXTERN void SetCameraEditStatePlayback(void)
{
   AutoAppIPtr(ShockCamera);
   pShockCamera->SetEditState(kCameraPlayback);
}

EXTERN void SetCameraEditSpeed(float speed)
{
   AutoAppIPtr(ShockCamera);
   pShockCamera->SetCameraSpeed(speed);
}

EXTERN void MakeScenes(void)
{
   AutoAppIPtr(ShockCamera);
   pShockCamera->MakeScenes();
}

void ConversationEndListener(ObjID conversationID)
{
   AutoAppIPtr(ShockCamera);
   pShockCamera->SetEditMode(kCameraNormal);
}

EXTERN void CutSceneStart(ObjID conversationID)
{
   AutoAppIPtr(ShockCamera);
   AutoAppIPtr(ObjectSystem);
   if (!pObjectSystem->Exists(conversationID))
   {
      Warning(("No such conversation object %d\n", conversationID));
      return;
   }
   pShockCamera->SetScene(pObjectSystem->GetName(conversationID));
   pShockCamera->SetEditMode(pShockCamera->GetEditState());
   AutoAppIPtr(AIManager);
   g_fAIAwareOfPlayer = FALSE;
   pAIManager->StartConversation(conversationID);
   (pAIManager->GetConversationManager())->ListenConversationEnd(&ConversationEndListener);
}

EXTERN void CutSceneEdit(ObjID conversationID)
{
   AutoAppIPtr(ShockCamera);
   pShockCamera->SetEditState(kCameraEdit);
   CutSceneStart(conversationID);
}

EXTERN void CutSceneRecord(ObjID conversationID)
{
   AutoAppIPtr(ShockCamera);
   pShockCamera->SetEditState(kCameraRecord);
   CutSceneStart(conversationID);
}

EXTERN void CutScenePlayback(ObjID conversationID)
{
   AutoAppIPtr(ShockCamera);
   pShockCamera->SetEditState(kCameraPlayback);
   CutSceneStart(conversationID);
}

#endif
