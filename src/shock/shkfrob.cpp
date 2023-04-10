/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkfrob.cpp,v 1.10 2000/02/19 13:20:20 toml Exp $

#include <shkfrob.h>

#include <property.h>
#include <propbase.h>

#include <sdesc.h>
#include <sdestype.h>
#include <sdesbase.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//-----------------------------------------------------
ILabelProperty *g_pPropFrobSound;

static sPropertyDesc FrobSoundDesc = 
{
   PROP_FROB_SOUND, 0,
  NULL, 0, 0, // constraints, versions
  { "Script", "Use sound" },
};

void FrobSoundPropertyInit(void)
{
   g_pPropFrobSound = CreateLabelProperty(&FrobSoundDesc, kPropertyImplDense);
}

BOOL FrobSoundIsRelevant(ObjID objID)
{
   return g_pPropFrobSound->IsRelevant(objID);
}

// get action sound
Label *FrobSoundGet(ObjID objID)
{
   Label *pFrobSound;

   if (g_pPropFrobSound->Get(objID, &pFrobSound))
      return pFrobSound;
   else
      return NULL;
}

//-----------------------------------------------------
// Audio log sound property
ILabelProperty *g_pPropAudioLog;

static sPropertyDesc AudioLogDesc = 
{
   PROP_AUDIO_LOG, 0,
  NULL, 0, 0, // constraints, versions
  { "Script", "Audio Data" },
};

void AudioLogPropertyInit(void)
{
   g_pPropAudioLog = CreateLabelProperty(&AudioLogDesc, kPropertyImplSparse);
}

//-----------------------------------------------------
ILabelProperty *g_pPropApparID;

static sPropertyDesc ApparIDDesc = 
{
   PROP_APPAR_ID, 0,
  NULL, 0, 0, // constraints, versions
  { "Script", "Apparition" },
};

void ApparIDPropertyInit(void)
{
   g_pPropApparID = CreateLabelProperty(&ApparIDDesc, kPropertyImplSparse);
}

//-----------------------------------------------------
ILabelProperty *g_pPropFailSound;

static sPropertyDesc FailSoundDesc = 
{
   PROP_FAIL_SOUND, 0,
  NULL, 0, 0, // constraints, versions
  { "Script", "Failure Sound" },
};

void FailSoundPropertyInit(void)
{
   g_pPropFailSound = CreateLabelProperty(&FailSoundDesc, kPropertyImplSparse);
}

//-----------------------------------------------------
// hack lock & key

IIntProperty *g_pPropHackLock;
IIntProperty *g_pPropHackKey;

static sPropertyDesc HackLockDesc = 
{
   PROP_HACK_LOCK, 0,
  NULL, 0, 0, // constraints, versions
  { "Script", "Hack Lock" },
};
static sPropertyDesc HackKeyDesc = 
{
   PROP_HACK_KEY, 0,
  NULL, 0, 0, // constraints, versions
  { "Script", "Hack Key" },
};

void HackLockPropertyInit(void)
{
   g_pPropHackLock = CreateIntProperty(&HackLockDesc, kPropertyImplSparse);
}

void HackKeyPropertyInit(void)
{
   g_pPropHackKey = CreateIntProperty(&HackKeyDesc, kPropertyImplSparse);
}

BOOL HackLockIsRelevant(ObjID objID)
{
   return g_pPropHackLock->IsRelevant(objID);
}

BOOL HackKeyIsRelevant(ObjID objID)
{
   return g_pPropHackKey->IsRelevant(objID);
}

int HackLockGet(ObjID objID)
{
   int val;

   if (g_pPropHackLock->Get(objID, &val))
      return val;
   else
      return NULL;
}

int HackKeyGet(ObjID objID)
{
   int val;

   if (g_pPropHackKey->Get(objID, &val))
      return val;
   else
      return NULL;
}
//-----------------------------------------------------
// Book-o-tron data to display
ILabelProperty *g_pPropBookData;

static sPropertyDesc BookDataDesc = 
{
  PROP_BOOK_DATA, 0,
  NULL, 0, 0, // constraints, versions
  { "Script", "Book Data" },
};

void BookDataPropertyInit(void)
{
   g_pPropBookData = CreateLabelProperty(&BookDataDesc, kPropertyImplSparse);
}
//-----------------------------------------------------
IIntProperty *g_pPropKeypadCode;

static sPropertyDesc KeypadCodeDesc = 
{
   PROP_KEYPAD_CODE, 0,
  NULL, 0, 0, // constraints, versions
  { "Script", "Keypad Code" },
};

void KeypadCodePropertyInit(void)
{
   g_pPropKeypadCode = CreateIntProperty(&KeypadCodeDesc, kPropertyImplSparse);
}

//-----------------------------------------------------
void ShockFrobPropertiesInit(void)
{
   FrobSoundPropertyInit();
   AudioLogPropertyInit();
   ApparIDPropertyInit();
   FailSoundPropertyInit();
   HackLockPropertyInit();
   HackKeyPropertyInit();
   BookDataPropertyInit();
   KeypadCodePropertyInit();
}