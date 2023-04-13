/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/script/sndscrpt.cpp,v 1.36 1999/08/05 17:34:28 Justin Exp $
//
// sndscrpt.cpp

#include <lg.h>
#include <mprintf.h>
#include <appagg.h>
#include <lazyagg.h>

#include <iobjsys.h>
#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <appsfx.h>
#include <objpos.h>
#include <osystype.h>
#include <psnd.h>
#include <schbase.h>
#include <schema.h>
#include <config.h>
#include <cfgdbg.h>
#include <esnd.h>
#include <ctagset.h>
#include <string.h>
#include <objedit.h>
#include <objremap.h>
#include <speech.h>

#include <sndscrpt.h>
#include <sndscrp_.h>
#include <vocore.h>

#include <tagfile.h>
#include <vernum.h>

// Include this last, please
#include <dbmem.h>

// Our special spew that works in playtest
#ifdef PLAYTEST
#define sndSpew(x) if (!config_is_defined("ScriptSoundSpew")) ; else mprintf x
#else
#define sndSpew(x)
#endif 

///////////////////////////////////////////////////////////////////////////////
//
// Message implementations
//

// We keep track of all sounds started by scripts, and when they end
// we use our callback to run the SoundDone or SchemaDone message on
// whichever object's script started the sound.

enum eSndPlayType
{
   kPlayNone,
   kPlaySound,
   kPlaySchema,
   kPlaySchemaEnv,
};

#define MAX_TAG_SIZE 128

// The handles used by the schema system come directly from appsfx.
// So we can depend on them to be unique no matter what calls they
// come from.
class cSoundRecord
{
public:
   int          m_iHandle;
   mxs_vector   m_Coordinates;
   ObjID        m_CallbackObjID;
   ObjID        m_TargetObjID;         // the sound was played _on_ this object
   Label        m_Name;                // either the sound name or the schema name
   BOOL         m_bIsSchema;

   // horrifying replay system
   eSndPlayType m_replay;
   eEnvSoundLoc m_eLoc;
   int          m_iFlags;
   BOOL         m_bAmbient;
   ObjID        m_Obj2;
   char         m_envTagsString[MAX_TAG_SIZE];
   
   cSoundRecord(int iHandle, 
                mxs_vector *pCoordinates,
                ObjID CallbackObjID,
                ObjID TargetObjID,
                Label *Name,
                BOOL bIsSchema)
      : m_iHandle(iHandle),
        m_Coordinates(*pCoordinates),
        m_CallbackObjID(CallbackObjID),
        m_TargetObjID(TargetObjID),
        m_Name(*Name),
        m_bIsSchema(bIsSchema)
   {
   }

   cSoundRecord()
   {
   }
};

static BOOL kill_on_load=FALSE;

// This is used to keep track of both active raw sounds and active
// schemas.
static cDynArray<cSoundRecord *> g_ActiveSounds;

static LazyAggMember(IObjectSystem) pObjSys;
static LazyAggMember(IScriptMan) pScriptMan;

///////////////////////
// init/term

void ScriptSoundInit(void)
{
   if (config_is_defined("sndscrpt_kill_restart"))
      kill_on_load=TRUE;
}

void ScriptSoundTerm(void)
{
}

///////////////////////
// handle generators

// these are later in the file, sorry
static void ActiveSchemaDoneCallback(int iHandle, int iSchemaID, void *pIgnoreUserData);
static void SFXParamsSetup(sfx_parm *pParams, int iFlags, eSoundNetwork net);

static void SchemaParamsSetup(sSchemaCallParams *pParams,
                              ObjID SourceObjID,
                              mxs_vector *Coordinates,
                              ulong iFlags,
                              eSoundNetwork net)
{
   pParams->flags = SCH_SET_CALLBACK | SCH_FORCE_PLAY | iFlags;
   if (net == kSoundNetworkAmbient)
      pParams->flags |= SCH_NETWORK;
   else if (net == kSoundNoNetworkSpatial)
      pParams->flags |= SCH_NO_NETWORK;
   pParams->sourceID = SourceObjID;
   pParams->callback = ActiveSchemaDoneCallback;
   pParams->pSourceLoc = Coordinates;
}

static int SndScrptGen(Label *lname, int iFlags, ObjID call, ObjID targ, ObjID obj2,
                       mxs_vector *crd, cTagSet *pTags, BOOL amb, eEnvSoundLoc loc, eSndPlayType play, eSoundNetwork net)
{
   sSchemaCallParams CallParams;
   int iHandle=SFX_NO_HND;
   switch (play)
   {
      case kPlayNone:
         break;
      case kPlaySound:
         sfx_parm parm;
         SFXParamsSetup(&parm, iFlags, net);         
         if (amb)
            iHandle = g_pPropSnd->GenerateSound(lname->text, &parm);
         else
            iHandle = g_pPropSnd->GenerateSound(targ, OBJ_NULL, lname->text, 1.0, &parm, kGSF_ForcePlay);
         break;
      case kPlaySchema:
         SchemaParamsSetup(&CallParams, targ, crd, iFlags, net);
         iHandle = SchemaPlay(lname, &CallParams);
         break;
      case kPlaySchemaEnv:
         SchemaParamsSetup(&CallParams, targ, crd, iFlags, net);         
         switch (loc)
         {
            case kEnvSoundOnObj:
               iHandle = ESndPlayObj(pTags, targ, obj2, &CallParams);
               break;
            case kEnvSoundAtObjLoc:
               iHandle = ESndPlayLoc(pTags, targ, obj2, crd, &CallParams);   
               break; 
            case kEnvSoundAmbient:
               iHandle = ESndPlay(pTags, targ, obj2, &CallParams);
               break;
         }
         break;
   }
   return iHandle;
}

///////////////////////
// save/load

#define kSndScriptSaveVer 1
static TagFileTag sound_tag = {"SndScript"};
static TagVersion sound_ver = { kSndScriptSaveVer, 0 };

void ScriptSoundSave(ITagFile *file)
{  // go through and write out the data we need
   HRESULT result=file->OpenBlock(&sound_tag,&sound_ver);
   BOOL failed=FALSE;   
   if (result!=S_OK)
      return;
   int cnt=g_ActiveSounds.Size();
   file->Write((char *)&cnt,sizeof(int));
   for (int i = 0; i < g_ActiveSounds.Size(); ++i)
      if (file->Write((char *)g_ActiveSounds[i],sizeof(cSoundRecord))!=sizeof(cSoundRecord))
      {
         failed=TRUE;
         Warning(("ActiveSounds failed to write\n"));
         break;
      }
   sndSpew(("sndscrpt Saved %d records\n",cnt));
   file->CloseBlock();
}

BOOL SndScriptRestartSavedSounds(cSoundRecord *sndDat)
{
   BOOL worked=FALSE;
   cTagSet tmp; // here comes the HACK HACK HACK
   
   sndDat->m_CallbackObjID = ObjRemapOnLoad(sndDat->m_CallbackObjID);
   sndDat->m_TargetObjID   = ObjRemapOnLoad(sndDat->m_TargetObjID);
   if (sndDat->m_replay==kPlaySchemaEnv)
   {  // only used for schemaEnv, so really
      tmp.FromString(sndDat->m_envTagsString);
      sndDat->m_Obj2 = ObjRemapOnLoad(sndDat->m_Obj2);
   }
      
   int hnd = 
      SndScrptGen(&sndDat->m_Name,sndDat->m_iFlags,
                  sndDat->m_CallbackObjID,sndDat->m_TargetObjID,sndDat->m_Obj2,
                  &sndDat->m_Coordinates,&tmp,sndDat->m_bAmbient,sndDat->m_eLoc,
                  sndDat->m_replay,kSoundNetDefault);
   if (sndDat->m_replay==kPlaySound)
      worked=(hnd!=SFX_NO_HND);
   else
      worked=(hnd!=SCH_HANDLE_NULL);
   if (worked)
   {
      sndDat->m_iHandle=hnd;
      g_ActiveSounds.Append(sndDat);
      sndSpew(("Restarted %s (callback %s targ %s) type %d\n",sndDat->m_Name.text,
               ObjWarnName(sndDat->m_CallbackObjID),
               ObjWarnName(sndDat->m_TargetObjID),
               sndDat->m_replay));
   }
   return worked;
}

void SndScriptSendSavedMsg(cSoundRecord *sndDat)
{
   // need to ObjRemap here, i think....
   sndDat->m_CallbackObjID = ObjRemapOnLoad(sndDat->m_CallbackObjID);
   sndDat->m_TargetObjID   = ObjRemapOnLoad(sndDat->m_TargetObjID);
   sndSpew(("savedispatch call %s targ %s - %s (a %s)\n",
            ObjWarnName(sndDat->m_CallbackObjID),
            ObjWarnName(sndDat->m_TargetObjID),
            sndDat->m_Name.text,
            sndDat->m_bIsSchema?"Schema":"Sound"
            ));
   if (sndDat->m_bIsSchema)
   {
      sSchemaDoneMsg *pMsg
         = new sSchemaDoneMsg(sndDat->m_CallbackObjID,
                              sndDat->m_TargetObjID,
                              sndDat->m_Coordinates,
                              sndDat->m_Name.text);
      pScriptMan->PostMessage(pMsg);
      SafeRelease(pMsg);
   }
   else
   {
      sSoundDoneMsg *pMsg
         = new sSoundDoneMsg(sndDat->m_CallbackObjID,
                             sndDat->m_TargetObjID,
                             sndDat->m_Coordinates,
                             sndDat->m_Name.text);
      pScriptMan->PostMessage(pMsg);
      SafeRelease(pMsg);
   }
}

static cDynArray<cSoundRecord *> _LoadSounds;

void ScriptSoundLoad(ITagFile *file)
{  // go through and postmessage the kills
   TagVersion local_ver=sound_ver;
   HRESULT result=file->OpenBlock(&sound_tag,&local_ver);
   BOOL failed=FALSE;
   int cnt=-1;
   
   if (result==S_OK)
   {
      if (file->Read((char *)&cnt,sizeof(int))==sizeof(int))
         for (int i = 0; i < cnt; ++i)
         {
            cSoundRecord *tmpSoundRec = new cSoundRecord();
            if (file->Read((char *)tmpSoundRec,sizeof(cSoundRecord))!=sizeof(cSoundRecord))
            {
               failed=TRUE;
               Warning(("ActiveSounds failed to read\n"));
               delete tmpSoundRec;
               break;
            }
            else
               _LoadSounds.Append(tmpSoundRec);
         }
      else
         Warning(("ActiveSounds couldnt get a count\n"));
      file->CloseBlock();
   }
   sndSpew(("sndscrpt loaded %d records\n",cnt));
}

void ScriptSoundPostLoad(void)
{
   for (int i=0; i<_LoadSounds.Size(); i++)
   {
      if (kill_on_load)
      {
         SndScriptSendSavedMsg(_LoadSounds[i]);
         delete _LoadSounds[i];
      }
      else
      {
         if (!SndScriptRestartSavedSounds(_LoadSounds[i]))
         {
            SndScriptSendSavedMsg(_LoadSounds[i]);
            delete _LoadSounds[i];
         }
      }
   }
   _LoadSounds.SetSize(0);
}

// if the data is real, call postload
void ScriptSoundEnterModeHack(void)
{
   ScriptSoundPostLoad();
}

void ScriptSoundReset(void)
{
   if (g_ActiveSounds.Size())
   {
#ifndef SHIP
      mprintf("Reseting Script Sounds but there are active sounds - TELL DOUG\n");
#endif
   }
}

///////////////////////
// callbacks

// When a scripted schema finishes, we do this.
static void ActiveSchemaDoneCallback(int iHandle, int iSchemaID, 
                                     void *pIgnoreUserData)
{
   if (g_ActiveSounds.Size() == 0)
      Warning(("No scripted schemas playing in callback; handle is %d\n", 
               iHandle));

   sndSpew(("Schema %s is done (handle %d)\n", ObjWarnName(iSchemaID), iHandle));

   for (int i = 0; i < g_ActiveSounds.Size(); ++i) {
      if (g_ActiveSounds[i]->m_iHandle == iHandle) {
#ifdef PLAYTEST
         if (g_ActiveSounds[i]->m_bIsSchema == FALSE)
            Warning(("Scripted schema callback found raw schema %d."
                     "  Yeah, Whatever.\n", g_ActiveSounds[i]->m_iHandle));
#endif // ~PLAYTEST

         sndSpew(("  found!\n"));

         sSchemaDoneMsg *pMsg
            = new sSchemaDoneMsg(g_ActiveSounds[i]->m_CallbackObjID,
                                 g_ActiveSounds[i]->m_TargetObjID,
                                 g_ActiveSounds[i]->m_Coordinates,
                                 g_ActiveSounds[i]->m_Name.text);
         pScriptMan->PostMessage(pMsg);
         SafeRelease(pMsg);

         delete g_ActiveSounds[i];
         g_ActiveSounds.DeleteItem(i);

         return;
      }
   }

   sndSpew(("  not found!\n"));
#ifdef PLAYTEST
   for (int k=0; k<g_ActiveSounds.Size(); k++)
      sndSpew(("    %s, handle %d\n", ObjWarnName(g_ActiveSounds[k]->m_TargetObjID), g_ActiveSounds[k]->m_iHandle));
#endif

   Warning(("Scripted schema handle not found in callback: %d\n", iHandle));
}


static void ActiveSoundDoneCallback(int iHandle, void *pIgnoreUserData)
{
   if (g_ActiveSounds.Size() == 0)
      Warning(("No scripted sounds playing in callback; handle is %d\n", 
               iHandle));

   for (int i = 0; i < g_ActiveSounds.Size(); ++i) {
      if (g_ActiveSounds[i]->m_iHandle == iHandle) {
#ifdef PLAYTEST
         if (g_ActiveSounds[i]->m_bIsSchema == TRUE)
            Warning(("Scripted raw sound callback found schema %d."
                     "  Yeah, Whatever.\n", g_ActiveSounds[i]->m_iHandle));
#endif // ~PLAYTEST

         sSoundDoneMsg *pMsg
            = new sSoundDoneMsg(g_ActiveSounds[i]->m_CallbackObjID,
                                g_ActiveSounds[i]->m_TargetObjID,
                                g_ActiveSounds[i]->m_Coordinates,
                                g_ActiveSounds[i]->m_Name.text);
         pScriptMan->PostMessage(pMsg);
         SafeRelease(pMsg);

         delete g_ActiveSounds[i];
         g_ActiveSounds.DeleteItem(i);

         return;
      }
   }

   Warning(("Scripted sound handle not found in callback: %d\n", iHandle));
}


// This is a lobotomized version of SchemaParamsSetup in schema.cpp.
// When we go back and add support for panning, custom fades, etc., we
// can pattern it after that.
static void SFXParamsSetup(sfx_parm *pParams, int iFlags, eSoundNetwork net)
{
   pParams->flag = iFlags;
   if (net == kSoundNetworkAmbient)
      pParams->flag |= SFXFLG_NET_AMB;
   else if (net == kSoundNoNetworkSpatial)
      pParams->flag |= SFXFLG_NO_NET;
   pParams->group = 0;
   pParams->end_callback = &ActiveSoundDoneCallback;
   pParams->loop_callback = NULL;
   pParams->user_data = 0;
   pParams->radius = 0;
   pParams->pan = 0;
   pParams->gain = -1;
   pParams->delay = 0;
   pParams->fade = 0;
   pParams->pri = 0;
   pParams->num_loops = SFX_LOOP_INFINITE;
}


// This is the stuff held in common by the three sound playing
// routines.
static bool PlayAndRememberSound(const char *SoundName,
                                 ObjID CallObjID,
                                 ObjID TargObjID,
                                 mxs_vector *pCoord,
                                 BOOL bAmbient, 
                                 eSoundSpecial Special = kSoundNormal,
                                 eSoundNetwork Network = kSoundNetDefault)
{
   Label SoundLabel;
   memcpy(&SoundLabel.text, SoundName, 15);
   SoundLabel.text[15] = 0;

   // start our sound
   int iFlags;
   switch (Special)
   {
      case kSoundLoop:
         iFlags = SFXFLG_LOOP;
         break;
      default:
         iFlags = 0;
         break;
   }
   int iHandle = SndScrptGen(&SoundLabel,iFlags,CallObjID,TargObjID,OBJ_NULL,
                             pCoord,NULL,bAmbient,(eEnvSoundLoc)0,kPlaySound,
                             Network);

   if (iHandle != SFX_NO_HND)
   {
      cSoundRecord *t = new cSoundRecord(iHandle, pCoord, CallObjID, TargObjID, &SoundLabel, FALSE);
      
      // @REPLAY - needs iFlags, bAmibent
      t->m_bAmbient = bAmbient;
      t->m_iFlags   = iFlags;
      t->m_replay   = kPlaySound;
      
      sndSpew(("Script on %s added sound %s\n", ObjWarnName(CallObjID), SoundName));
      
#ifdef PLAYTEST
      for (int k = 0; k < g_ActiveSounds.Size(); k++)
         Assert_(g_ActiveSounds[k]->m_iHandle != iHandle);
#endif

      g_ActiveSounds.Append(t);
      return TRUE;
   } else {
      sndSpew(("Script on %s failed to play sound %s\n", ObjWarnName(CallObjID), SoundName));
      return FALSE;
   }
}


static bool PlayAndRememberSchema(ObjID SchemaID,
                                  ObjID CallObjID,
                                  ObjID TargObjID,
                                  mxs_vector *pCoord,
                                  ulong iFlags,
                                  eSoundNetwork Network)
{
   if (!SchemaID) {
#ifdef PLAYTEST      
      mprintf("Script on %s requested nonexistent schema!\n",ObjWarnName(CallObjID));
#endif      
      return FALSE;
   }

   const char* name = pObjSys->GetName(SchemaID);

   Label SchemaLabel;   
   strncpy(SchemaLabel.text,name,sizeof(Label));
   SchemaLabel.text[sizeof(Label)-1] = '\0';

   int iHandle = SndScrptGen(&SchemaLabel,iFlags,CallObjID,TargObjID,OBJ_NULL,
                             pCoord,NULL,0,(eEnvSoundLoc)0,kPlaySchema,
                             Network);

   if (iHandle != SCH_HANDLE_NULL)
   {
      cSoundRecord *t = new cSoundRecord(iHandle, pCoord, CallObjID, TargObjID, &SchemaLabel, TRUE);

      // @REPLAY - iFlags
      t->m_iFlags   = iFlags;
      t->m_replay   = kPlaySchema;
      
      sndSpew(("Script on %s adding schema %s\n", 
               ObjWarnName(CallObjID), SchemaLabel.text)); 
#ifdef PLAYTEST
      for (int k = 0; k < g_ActiveSounds.Size(); k++)
         Assert_(g_ActiveSounds[k]->m_iHandle != iHandle);
#endif

      g_ActiveSounds.Append(t);
      return TRUE;
   } else {
      sndSpew(("Script on %s failed to play schema %s\n", 
               ObjWarnName(CallObjID), SchemaLabel.text)); 
      return FALSE;
   }
}



static bool PlayAndRememberEnvSchema(ObjID CallObjID,
                                     ObjID SrcObjID,
                                     ObjID Obj2,
                                     cTagSet *pTagSet,
                                     mxs_vector *pCoord,
                                     ulong iFlags,
                                     eEnvSoundLoc loc = kEnvSoundAmbient,
                                     eSoundNetwork Network = kSoundNetDefault)
{
   int iHandle = SndScrptGen(NULL,iFlags,CallObjID,SrcObjID,Obj2,pCoord,pTagSet,FALSE,loc,kPlaySchemaEnv,Network);

   if (iHandle != SCH_HANDLE_NULL) 
   {
      const char* name = pObjSys->GetName(SchemaGetIDFromHandle(iHandle));
      Label label;
      strncpy(label.text,name,sizeof(label.text));
      label.text[sizeof(label.text)-1] = '\0';

      cSoundRecord *t = new cSoundRecord(iHandle,pCoord,CallObjID,SrcObjID,&label,TRUE);

      // @REPLAY - iFlags, loc, Obj2, tags
      t->m_iFlags   = iFlags;
      t->m_eLoc     = loc;
      t->m_replay   = kPlaySchemaEnv;
      t->m_Obj2     = Obj2;
      cStr tmp;
      pTagSet->ToString(&tmp);
      strncpy(t->m_envTagsString,(const char *)tmp,MAX_TAG_SIZE);

#ifdef PLAYTEST
      for (int k = 0; k < g_ActiveSounds.Size(); k++)
         Assert_(g_ActiveSounds[k]->m_iHandle != iHandle);
#endif

      g_ActiveSounds.Append(t);
      sndSpew(("Script on %s Obj %s Handle %d\n",label.text,ObjWarnName(SrcObjID),iHandle));
      return TRUE;
   }
   else 
   {
#ifdef PLAYTEST
      if (pTagSet && pTagSet->Size())
      {
         cStr tmpStr;
         pTagSet->ToString(&tmpStr);
         sndSpew(("Script on %s failed to play env schema (tags %s)\n", ObjWarnName(CallObjID), tmpStr));
      }
      else
         sndSpew(("Script on %s failed to play env schema\n", ObjWarnName(CallObjID)));         
#endif      
      return FALSE;
   }
}


// We can halt all sounds on a given object, or get more specific,
// requiring a given sound or schema name and even a callback object.
static int HaltActiveSounds(ObjID TargetObjID,
                            const char *pszName,
                            ObjID CallbackObjID,
                            BOOL bIsSchema)
{
   cSoundRecord *pRecord;
   int iNumHalted = 0;
   BOOL bAnyName = FALSE;
   BOOL bAnyCallbackObjID = FALSE;

#ifdef PLAYTEST
   if (strlen(pszName) > 15)
      mprintf("WARNING: HaltActiveSounds: %s longer than 15 characters.\n", pszName);
#endif // PLAYTEST

   if (*pszName == 0)
      bAnyName = TRUE;

   if (CallbackObjID == OBJ_NULL)
      bAnyCallbackObjID = TRUE;

   // Copy the array as halts will mutate the global array
   cDynArray<cSoundRecord *> activeSounds(g_ActiveSounds);

#ifdef PLAYTEST
   sndSpew(("Script going to halt for %s\n", ObjWarnName(TargetObjID)));
   sndSpew(("Soundlist (size %d)\n", activeSounds.Size()));
   for (int k=0; k<activeSounds.Size(); k++)
      sndSpew(("  on %s, handle %d\n", ObjWarnName(activeSounds[k]->m_TargetObjID), activeSounds[k]->m_iHandle));
#endif

   for (int i = 0; i < activeSounds.Size(); ++i) {
      pRecord = activeSounds[i];

      if ((pRecord->m_bIsSchema == bIsSchema) &&
          (pRecord->m_TargetObjID == TargetObjID) &&
          (bAnyName || (strcmp(pszName, pRecord->m_Name.text) == 0)) &&
          (bAnyCallbackObjID || (pRecord->m_CallbackObjID == CallbackObjID)))
      {
         sndSpew(("Script Halting %s%s for obj %s (cback obj %s) handle %d\n",
                  pszName[0]=='\0'?"Unknown":pszName,
                  bIsSchema?" (a Schema)":" (a Sound)",
                  ObjWarnName(TargetObjID),ObjWarnName(CallbackObjID),pRecord->m_iHandle));

         if (bIsSchema)
            SchemaPlayHalt(pRecord->m_iHandle);
         else
            g_pPropSnd->SoundHalt(pRecord->m_iHandle);

         ++iNumHalted;
      }
   }

   return iNumHalted;
}


IMPLEMENT_SCRMSG_PERSISTENT(sSoundDoneMsg)
{
   PersistenceHeader(sScrMsg, kSoundDoneMsgVer);
   Persistent(coordinates);
   Persistent(targetObject);
   Persistent(name);

   return TRUE;
}


IMPLEMENT_SCRMSG_PERSISTENT(sSchemaDoneMsg)
{
   PersistenceHeader(sScrMsg, kSchemaDoneMsgVer);
   Persistent(coordinates);
   Persistent(targetObject);
   Persistent(name);

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDebugScrSrv
//


DECLARE_SCRIPT_SERVICE_IMPL(cSoundScrSrv, Sound)
{
public:

   ////////////////////////////////////
   //
   // four routines for playing sounds...
   //

   // If we do not have a target object or set of coordinates, we
   // assume our sound is coming from the object playing the script.
   STDMETHOD_ (boolean, Play)(object CallbackObjID, 
                              const string& SoundName,
                              eSoundSpecial Special = kSoundNormal,
                              eSoundNetwork Network = kSoundNetDefault)

   {
      ObjID ScriptID = ScriptObjID(CallbackObjID);
      Position *Pos = ObjPosGet(ScriptID);
      mxs_vector Coordinates;

      if (Pos == 0)
         mx_zero_vec(&Coordinates);
      else
         Coordinates = Pos->loc.vec;

      return PlayAndRememberSound(SoundName, ScriptID, ScriptID,
                                  &Coordinates, FALSE, Special, Network);
   }

   // We'll record the coordinates of the target object.  Naturally,
   // this doesn't keep it from moving before the sound finishes.
   STDMETHOD_ (boolean, Play)(object CallbackObjID, 
                              const string& SoundName,
                              object TargetObj,
                              eSoundSpecial Special = kSoundNormal,
                              eSoundNetwork Network = kSoundNetDefault)
   {
      ObjID ScriptID = ScriptObjID(CallbackObjID);
      ObjID TargetID = ScriptObjID(TargetObj);
      Position *Pos = ObjPosGet(TargetID);
      mxs_vector Coordinates;

      if (Pos == 0)
         mx_zero_vec(&Coordinates);
      else
         Coordinates = Pos->loc.vec;

      return PlayAndRememberSound(SoundName, ScriptID, TargetID,
                                  &Coordinates, FALSE, Special, Network);
   }

   // We set the target object to OBJ_NULL if we're playing to
   // coordinates.
   STDMETHOD_ (boolean, Play)(object CallbackObjID, 
                              const string& SoundName,
                              vector& Location,
                              eSoundSpecial Special = kSoundNormal,
                              eSoundNetwork Network = kSoundNetDefault)
   {
      ObjID ScriptID = ScriptObjID(CallbackObjID);
      return PlayAndRememberSound(SoundName, ScriptID, OBJ_NULL,
                                  &Location, FALSE, Special, Network);
   }

   STDMETHOD_ (boolean, PlayAmbient)(object CallbackObjID, 
                                     const string& SoundName,
                                     eSoundSpecial Special = kSoundNormal,
                                     eSoundNetwork Network = kSoundNetDefault)
   {
      ObjID ScriptID = ScriptObjID(CallbackObjID);
      mxs_vector Coordinates;
      mx_zero_vec(&Coordinates);

      return PlayAndRememberSound(SoundName, ScriptID, ScriptID,
                                  &Coordinates, TRUE, Special, Network);
   }

   ////////////////////////////////////
   //
   // four routines for playing schemas
   //
   STDMETHOD_ (boolean, PlaySchema)(object CallbackObjID,
                                    object Schema,
                                    eSoundNetwork Network = kSoundNetDefault)
   {
      ObjID ScriptID = ScriptObjID(CallbackObjID);
      Position *Pos = ObjPosGet(ScriptID);
      mxs_vector Coordinates;

      if (Pos == 0)
         mx_zero_vec(&Coordinates);
      else
         Coordinates = Pos->loc.vec;

      return PlayAndRememberSchema(Schema, ScriptID, ScriptID, 
                                   &Coordinates, SCH_SET_OBJ, Network);
   }

   STDMETHOD_ (boolean, PlaySchema)(object CallbackObjID, 
                                    object Schema,
                                    object TargetObj,
                                    eSoundNetwork Network = kSoundNetDefault)
   {
      ObjID ScriptID = ScriptObjID(CallbackObjID);
      ObjID TargetID = ScriptObjID(TargetObj);
      Position *Pos = ObjPosGet(TargetID);
      mxs_vector Coordinates;

      if (Pos == 0)
         mx_zero_vec(&Coordinates);
      else
         Coordinates = Pos->loc.vec;

      return PlayAndRememberSchema(Schema, ScriptID, TargetID,
                                   &Coordinates, SCH_SET_OBJ, Network);
   }

   STDMETHOD_ (boolean, PlaySchema)(object CallbackObjID, 
                                    object Schema,
                                    vector& Location,
                                    eSoundNetwork Network = kSoundNetDefault)
   {
      ObjID ScriptID = ScriptObjID(CallbackObjID);
      return PlayAndRememberSchema(Schema, ScriptID, OBJ_NULL, 
                                   &Location, SCH_SET_LOC, Network);
   }

   STDMETHOD_ (boolean, PlaySchemaAmbient)(object CallbackObjID,
                                           object Schema,
                                           eSoundNetwork Network = kSoundNetDefault)
   {
      ObjID ScriptID = ScriptObjID(CallbackObjID);
      mxs_vector Coordinates;
      mx_zero_vec(&Coordinates);

      return PlayAndRememberSchema(Schema, ScriptID, ScriptID, 
                                   &Coordinates, 0, Network);
   }


   STDMETHOD_(boolean, PlayEnvSchema)(object CallbackObject,
                                      const string& Tags,
                                      object SourceObject, object Agent,
                                      eEnvSoundLoc loc = kEnvSoundAmbient,
                                      eSoundNetwork Network = kSoundNetDefault)
   {
      cTagSet TagSet(Tags);

      ObjID ScriptID = ScriptObjID(CallbackObject);
      ObjID SourceID = ScriptObjID(SourceObject);
      if (SourceID == OBJ_NULL)
         SourceID = ScriptID; 
      Position *Pos = ObjPosGet(SourceID);
      mxs_vector Coordinates;

      if (Pos)
         Coordinates = Pos->loc.vec;
      else
         mx_zero_vec(&Coordinates);

      ulong flags = (loc == kEnvSoundOnObj) ? SCH_SET_OBJ : 0; 

      return PlayAndRememberEnvSchema(ScriptID,
                                      SourceID,
                                      Agent,
                                      &TagSet,
                                      &Coordinates,
                                      flags,
                                      loc,
                                      Network);
   }



   STDMETHOD_(boolean, PlayVoiceOver)(object cb_obj, object schema)
   {  
      AutoAppIPtr(VoiceOverSys); 

      int iHandle = pVoiceOverSys->Play(schema); 
      
      if (iHandle != SCH_HANDLE_NULL) 
         return TRUE;
      else 
      {
         ConfigSpew("ScriptSoundSpew",("Script on object %d failed to play schema %d")); 

         return FALSE;
      }

   }

   ////////////////////////////////////
   //
   // two routines to undo the stuff the other ones do
   //

   STDMETHOD_(integer, Halt)(object TargetObject,
                             const string& SoundName,
                             object CallbackObject)
   {
      return HaltActiveSounds(TargetObject, SoundName, CallbackObject, FALSE);
   }

   STDMETHOD_(boolean, HaltSchema)(object TargetObject,
                                   const string& SchemaName,
                                   object CallbackObject)
   {
      return HaltActiveSounds(TargetObject, SchemaName, CallbackObject, TRUE);
   }


   STDMETHOD(HaltSpeech)(object speakerObj)
   {
      SpeechHalt(speakerObj);
      return S_OK;
   }

   STDMETHOD_(boolean,PreLoad)(const string& SpeechName)
   {
      return SFX_PreLoad(SpeechName);
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cSoundScrSrv, Sound);

///////////////////////////////////////////////////////////////////////////////
