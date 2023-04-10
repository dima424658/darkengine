/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkcs.h,v 1.2 2000/01/31 09:55:28 adurant Exp $
//
// Commands associated with cut-scene recording/playback
//
#pragma once

#include <objtype.h>

EXTERN void CameraPositionSpew(void);
EXTERN void ShockCameraAttach(char *pObjName);
EXTERN void SetCameraEditStateRecord(void);
EXTERN void SetCameraEditStateEdit(void);
EXTERN void SetCameraEditStatePlayback(void);
EXTERN void SetCameraEditSpeed(float speed);
EXTERN void MakeScenes(void);
EXTERN void CutSceneStart(ObjID conversationID);
EXTERN void CutSceneEdit(ObjID conversationID);
EXTERN void CutSceneRecord(ObjID conversationID);
EXTERN void CutScenePlayback(ObjID conversationID);

