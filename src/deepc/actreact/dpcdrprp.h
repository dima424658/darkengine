/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCDRPRP_H
#define __DPCDRPRP_H


#ifndef PROPFACE_H
#include <propface.h>
#endif // !PROPFACE_H

#define PROP_DOOR_OPEN_SOUND "DoorOpenSound"
EXTERN void DoorOpenSoundPropertyInit(void);
EXTERN BOOL DoorOpenSoundIsRelevant(ObjID objID);
EXTERN Label *DoorOpenSoundGet(ObjID objID);

#define PROP_DOOR_CLOSE_SOUND "DoorCloseSound"
EXTERN void DoorCloseSoundPropertyInit(void);
EXTERN BOOL DoorCloseSoundIsRelevant(ObjID objID);
EXTERN Label *DoorCloseSoundGet(ObjID objID);

#define PROP_DOOR_TIMER "DoorTimer"
EXTERN void DoorTimerPropertyInit(void);
EXTERN BOOL DoorTimerIsRelevant(ObjID objID);
EXTERN int DoorTimerGet(ObjID objID);

#endif // __DPCDRPRP_H