/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkdrprp.h,v 1.3 2000/01/31 09:55:39 adurant Exp $
#pragma once

#ifndef __SHKDRPRP_H
#define __SHKDRPRP_H

#include <propface.h>

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

#endif