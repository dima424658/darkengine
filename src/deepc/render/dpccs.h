/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef _DPCCS_H
#define _DPCCS_H

// Commands associated with cut-scene recording/playback
//

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // !OBJTYPE_H

EXTERN void CameraPositionSpew(void);
EXTERN void DPCCameraAttach(char *pObjName);
EXTERN void SetCameraEditStateRecord(void);
EXTERN void SetCameraEditStateEdit(void);
EXTERN void SetCameraEditStatePlayback(void);
EXTERN void SetCameraEditSpeed(float speed);
EXTERN void MakeScenes(void);
EXTERN void CutSceneStart(ObjID conversationID);
EXTERN void CutSceneEdit(ObjID conversationID);
EXTERN void CutSceneRecord(ObjID conversationID);
EXTERN void CutScenePlayback(ObjID conversationID);

#endif // _DPCCS_H
