/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Commands associated with cut-scene recording/playback

#ifndef SHIP

#include <dpccs.h>

#include <appagg.h>
#include <comtools.h>
#include <mprintf.h>

#include <aiapisns.h>
#include <aicnvman.h>
#include <aiman.h>
#include <iobjsys.h>
#include <viewmgr.h>

#include <dpccmapi.h>

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

EXTERN void DPCCameraAttach(char *pObjName)
{
   AutoAppIPtr_(ObjectSystem, pObjSys);
   ObjID objID;

   AutoAppIPtr(DPCCamera);
   if (strlen(pObjName) == 0)
      pDPCCamera->Attach("Player");
   else if ((objID = pObjSys->GetObjectNamed(pObjName)) == OBJ_NULL)
      mprintf("No object named %s\n", pObjName);
   else
      pDPCCamera->Attach(pObjName);
}

EXTERN void SetCameraEditStateRecord(void)
{
   AutoAppIPtr(DPCCamera);
   pDPCCamera->SetEditState(kCameraRecord);
}

EXTERN void SetCameraEditStateEdit(void)
{
   AutoAppIPtr(DPCCamera);
   pDPCCamera->SetEditState(kCameraEdit);
}

EXTERN void SetCameraEditStatePlayback(void)
{
   AutoAppIPtr(DPCCamera);
   pDPCCamera->SetEditState(kCameraPlayback);
}

EXTERN void SetCameraEditSpeed(float speed)
{
   AutoAppIPtr(DPCCamera);
   pDPCCamera->SetCameraSpeed(speed);
}

EXTERN void MakeScenes(void)
{
   AutoAppIPtr(DPCCamera);
   pDPCCamera->MakeScenes();
}

void ConversationEndListener(ObjID conversationID)
{
   AutoAppIPtr(DPCCamera);
   pDPCCamera->SetEditMode(kCameraNormal);
}

EXTERN void CutSceneStart(ObjID conversationID)
{
   AutoAppIPtr(DPCCamera);
   AutoAppIPtr(ObjectSystem);
   if (!pObjectSystem->Exists(conversationID))
   {
      Warning(("No such conversation object %d\n", conversationID));
      return;
   }
   pDPCCamera->SetScene(pObjectSystem->GetName(conversationID));
   pDPCCamera->SetEditMode(pDPCCamera->GetEditState());
   AutoAppIPtr(AIManager);
   g_fAIAwareOfPlayer = FALSE;
   pAIManager->StartConversation(conversationID);
   (pAIManager->GetConversationManager())->ListenConversationEnd(&ConversationEndListener);
}

EXTERN void CutSceneEdit(ObjID conversationID)
{
   AutoAppIPtr(DPCCamera);
   pDPCCamera->SetEditState(kCameraEdit);
   CutSceneStart(conversationID);
}

EXTERN void CutSceneRecord(ObjID conversationID)
{
   AutoAppIPtr(DPCCamera);
   pDPCCamera->SetEditState(kCameraRecord);
   CutSceneStart(conversationID);
}

EXTERN void CutScenePlayback(ObjID conversationID)
{
   AutoAppIPtr(DPCCamera);
   pDPCCamera->SetEditState(kCameraPlayback);
   CutSceneStart(conversationID);
}

#endif
