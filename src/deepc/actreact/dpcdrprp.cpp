/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <dpcdrprp.h>

#include <property.h>
#include <propbase.h>

#include <sdesc.h>
#include <sdestype.h>
#include <sdesbase.h>

////////////////////////////////////////////////////////////////
ILabelProperty *g_pPropDoorOpenSound;

static sPropertyDesc DoorOpenSoundDesc = 
{
   PROP_DOOR_OPEN_SOUND, 
   0,
   NULL, 
   1,
   0,
   {"Door", "Door Open Sound"},
};

void DoorOpenSoundPropertyInit(void)
{
   g_pPropDoorOpenSound = CreateLabelProperty(&DoorOpenSoundDesc, kPropertyImplDense);
}

BOOL DoorOpenSoundIsRelevant(ObjID objID)
{
   return g_pPropDoorOpenSound->IsRelevant(objID);
}

// get action sound
Label *DoorOpenSoundGet(ObjID objID)
{
   Label *pDoorOpenSound;

   if (g_pPropDoorOpenSound->Get(objID, &pDoorOpenSound))
      return pDoorOpenSound;
   else
      return NULL;
}

////////////////////////////////////////////////////////////////
ILabelProperty *g_pPropDoorCloseSound;

static sPropertyDesc DoorCloseSoundDesc = 
{
   PROP_DOOR_CLOSE_SOUND,
   0,
   NULL, 
   1,
   0,
   {"Door", "Door Close Sound"},
};

void DoorCloseSoundPropertyInit(void)
{
   g_pPropDoorCloseSound = CreateLabelProperty(&DoorCloseSoundDesc, kPropertyImplDense);
}

BOOL DoorCloseSoundIsRelevant(ObjID objID)
{
   return g_pPropDoorCloseSound->IsRelevant(objID);
}

// get action sound
Label *DoorCloseSoundGet(ObjID objID)
{
   Label *pDoorCloseSound;

   if (g_pPropDoorCloseSound->Get(objID, &pDoorCloseSound))
      return pDoorCloseSound;
   else
      return NULL;
}

////////////////////////////////////////////////////////////////
IIntProperty *g_pPropDoorTimer;

static sPropertyDesc DoorTimerDesc = 
{
   PROP_DOOR_TIMER,
   0,
   NULL, 
   1,
   0,
   {"Door", "Door Timer Duration"},
};

void DoorTimerPropertyInit(void)
{
   g_pPropDoorTimer = CreateIntProperty(&DoorTimerDesc, kPropertyImplSparse);
}

BOOL DoorTimerIsRelevant(ObjID objID)
{
   return g_pPropDoorTimer->IsRelevant(objID);
}

// get action sound
int DoorTimerGet(ObjID objID)
{
   int pDoorTimer;

   if (g_pPropDoorTimer->Get(objID, &pDoorTimer))
      return pDoorTimer;
   else
      return NULL;
}
////////////////////////////////////////////////////////////////
