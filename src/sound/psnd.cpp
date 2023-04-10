/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sound/psnd.cpp,v 1.49 2000/03/22 19:02:42 patmac Exp $
//
// Sound propagation functions
//

//#define PROFILE_ON

#include <lg.h>
#include <config.h>
#include <cfgdbg.h>
#include <timings.h>
#include <mprintf.h>

#include <playrobj.h>
#include <objpos.h>
#include <objedit.h>
#include <appsfx.h>
#include <schprop.h>
#include <iobjnet.h>
#include <netman.h>
#include <lazyagg.h>

#include <hashpp.h>
#include <hshpptem.h>

#include <roomsys.h>
#include <rooms.h>
#include <psnd.h>
#include <psndapi.h>

#include <rendprop.h>

#include <sndnet.h>
#include <sndframe.h>

// Must be last header
#include <dbmem.h>

static LazyAggMember(IObjectNetworking) gpObjNet;
static LazyAggMember(IObjectSystem)     gpObjSys;
static LazyAggMember(INetManager)       gpNetMan;

DECLARE_TIMER(GenSnd, Average);
DECLARE_TIMER(GenSndObj, Average);
DECLARE_TIMER(GenSndVec, Average);
DECLARE_TIMER(GenSndStatic, Average);

////////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
template cSoundBlockingTable;
#endif

////////////////////////////////////////////////////////////////////////////////

cPropSnd  *g_pPropSnd = NULL;
int        g_SndWatchHandle = -1;

// TBD : Set actual values.
// TBD : Should be in module more closely related to the types themselves?
// TBD : Maybe make function to initialize with (possibly dynamic) values.
int gSoundTypeCaps[] = { 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 };

////////////////////////////////////////////////////////////////////////////////

#ifndef SHIP
int num_sounds = 0;
int num_too_far = 0;
#endif

cPropSnd::cPropSnd()
{
   m_SoundInstance = &m_SoundInstHigh;

   for (int i=0; i<kMaxActiveSounds; i++)
      m_SoundList[i] = NULL;

   m_SoundBlockingTable.Clear();

   m_NoSound = config_is_defined("no_sound");

}

////////////////////////////////////////

cPropSnd::~cPropSnd()
{
   SoundHaltAll();

   while (AreSounds())
   {
      #ifndef SHIP
      if (config_is_defined("SoundSpew"))
         mprintf("Waiting for active sounds to finish\n");
      #endif
   }

   m_SoundBlockingTable.Clear();

   #ifndef SHIP
   if (config_is_defined("too_far_sound_count") && (num_sounds > 0))
      mprintf("%d sounds generated, %d too far away (%g%%)\n", num_sounds, num_too_far, 
              100.0 * (float)num_too_far / (float)num_sounds);
   #endif
}

////////////////////////////////////////

//
// These two functions initialize the sound instance, propagate the sound,
// and return a handle, which is the address of the list of sounds that the
// player hears (if the sound is audible to the player)
//

int cPropSnd::GenerateSound(ObjID objID, ObjID schemaID, const char *sampleName, float atten_factor, 
                            sfx_parm *parms, int flags, void *data)
{
   int handle;

   if (ObjPosGet(objID) == NULL)
   {
      Warning(("GenerateSound::Attempt to play on obj w/o position (%s)!\n",ObjEditName(objID)));
      return kPSndFail;
   }

   if (ShouldPlaySound(parms, flags, TRUE, objID))
   {
      // I'm not generating a Warning here because we expect some things (like AI projectiles) 
      // to actually do this. 
      if (!ObjHasRefs(objID))
         return kPSndFail;

      if ((handle = GetSoundHandle()) == -1)
      {
         Warning(("No free handles for GenerateSound\n"));
         return kPSndFail;
      }
  
      return GenerateSoundHandle(handle, objID, schemaID, sampleName, atten_factor, 
                                 parms, kGSF_WantHandle | flags, data);
   }
   else
   {
#ifndef SHIP
      if (config_is_defined("SoundGhostSpew"))
      {
         mprintf("Failing to play %s,\n", sampleName);
         mprintf("  parms->flag = 0x%x\n", parms->flag);
         mprintf("  flags = 0x%x\n", flags);
#ifdef NEW_NETWORK_ENABLED
         mprintf("  proxy(%d) = %d\n", objID, gpObjNet->ObjIsProxy(objID));
#endif
      }
#endif
      return kPSndFail;
   }
}


int cPropSnd::GenerateSoundHandle(int handle, ObjID objID, ObjID schemaID, const char *sampleName, 
                                  float atten_factor, sfx_parm *parms, int flags, void *data)
{
   AUTO_TIMER(GenSnd);
   AUTO_TIMER(GenSndObj);

   AutoAppIPtr_(ObjectSystem, pObjSys);

   if (!pObjSys->Exists(objID))
   {
#ifndef SHIP
      mprintf("WARNING: Attempt to play sound %s on nonexistant object %d!\n", sampleName, objID);
#endif
      return kPSndFail;
   }

   cRoom *start_room = g_pRooms->GetObjRoom(objID);

   // Deal with starting outside the room database
   if (start_room == NULL)
   {
      #ifndef SHIP
      if (!config_is_defined("QuietSoundRooms"))
         mprintf("\"%s\" on %s (%g %g %g) not in room!\n",
                 sampleName, ObjWarnName(objID), ObjPosGet(objID)->loc.vec.x, 
                 ObjPosGet(objID)->loc.vec.y, ObjPosGet(objID)->loc.vec.z);
      #endif

      return kPSndFail;
   }

   // we could point this at a low-definition version, if we ever cared, which we don't
   m_SoundInstance = &m_SoundInstHigh;

   // @TODO: need some way to say whether this is the inital sound or not!

   int init_flags = kSIF_OnObject | ((flags & kGSF_NetSent) ? kSIF_NetSent : kSIF_None) |
                                    ((flags & kGSF_Networked) ? kSIF_Networked : kSIF_None) |
                                    ((flags & kGSF_ForcePlay) ? kSIF_ForcePlay : kSIF_None);

   // Init the sound instance
   m_SoundInstance->Init(handle, objID, ObjPosGet(objID)->loc.vec, schemaID, sampleName, 
                         init_flags, atten_factor, parms, data);

   #ifndef SHIP
   if (config_is_defined("too_far_sound_count"))
   {
      float dist = 0;
      config_get_float("too_far_sound_count", &dist);

      if (mx_dist_vec(&ObjPosGet(PlayerObject())->loc.vec, &ObjPosGet(objID)->loc.vec) > dist)
         num_too_far++;
      num_sounds++;
   }
   #endif

   // Do the propagation
   m_PropAgent.SetFunctions(m_SoundInstance);
   m_PropAgent.PropagateBF(ObjPosGet(objID)->loc.vec, g_pRooms->GetObjRoom(objID));

   if ((flags & kGSF_WantHandle) && !m_NoSound)
   {
      cPlayerSoundList* pPlayerList = GeneratePlayerSoundList(handle);
      if (pPlayerList == NULL)
      {
         #ifndef SHIP
         if (config_is_defined("SoundFullSpew"))
            mprintf("GenSoundHandle: %s on %s failed to propagate to the player\n",
                    sampleName, ObjWarnName(objID));
         #endif            

         // Make a cSoundInfo and defer it, if we're a looped sample
         if (parms->flag & SFXFLG_LOOP)
         {
            cPlayerSoundList *pPlayerSoundList = new cPlayerSoundList;
            cSoundInfo *pSoundInfo = new cSoundInfo;

            pPlayerSoundList->Append(pSoundInfo);

            pPlayerSoundList->SchemaCallback = parms->end_callback;
            pPlayerSoundList->SchemaUserData = parms->user_data;

            parms->distance = -1;
            parms->end_callback = EndSampleCallback;
            parms->user_data = (void *)handle;

            // TODO : Fill with real values?
            parms->pri = 128;

            pSoundInfo->SchemaID = schemaID;
            pSoundInfo->SrcObject = objID;
            pSoundInfo->SrcPoint = ObjPosGet(objID)->loc.vec;
            pSoundInfo->Object = PlayerObject();
            strncpy (pSoundInfo->SampleName, sampleName, 16);
            pSoundInfo->AppsfxParms = *parms;
            pSoundInfo->AttenFactor = atten_factor;

            pSoundInfo->flags |= kSIF_OnObject|kSIF_Deferred;
            pSoundInfo->Handle = handle;

            RegisterSound(handle, pPlayerSoundList);

            #ifndef SHIP
            if (config_is_defined("SoundFullSpew"))
               mprintf("  looping sample, so deferred (handle %d)\n", handle);
            #endif
            
            return handle;
         }
         else
         {
            #ifndef SHIP
            if (config_is_defined("SoundFullSpew"))
               mprintf("  non-looping sample, so failing\n");
            #endif
   
            return kPSndFail;
         }
      }

      #ifndef SHIP
      if (config_is_defined("SoundSpew"))
         mprintf( "Playing object sound on %s.\n", ObjWarnName(objID) );
      #endif

      SNDFrameAddObj( objID );

      m_SoundInstance->SetupPlayerSoundListCallbacks(pPlayerList);

      switch (Prioritize(pPlayerList))
      {
         case PRIORITY_RES_OK:
            if (!m_SoundInstance->PlaySoundList(pPlayerList))
            {
               delete pPlayerList->Remove(pPlayerList->GetFirst());
               delete pPlayerList;
               return kPSndFail;
            }
            break;
         case PRIORITY_RES_DEFERRED:
            if (pPlayerList->GetFirst())
               pPlayerList->GetFirst()->flags |= kSIF_Deferred;
            break;
         case PRIORITY_RES_FAIL:
            delete pPlayerList->Remove(pPlayerList->GetFirst());
            delete pPlayerList;
            return kPSndFail;
      }

      RegisterSound(handle, pPlayerList);

      return handle;
   }
   else
   {
      if (flags & kGSF_WantHandle)
         return kPSndFail;
      else
         return handle;
   }
}

int cPropSnd::GenerateSound(mxs_vector &vec, ObjID src_obj, ObjID schemaID, const char *sampleName, 
                            float atten_factor,sfx_parm *parms, int flags, void *data)
{
   int handle;

   if (ShouldPlaySound(parms, flags, FALSE, src_obj))
   {
      if ((handle = GetSoundHandle()) == -1)
      {
         Warning(("No free handles for GenerateSound\n"));
         return kPSndFail;
      }

      return GenerateSoundHandle(handle, vec, src_obj, schemaID, sampleName, atten_factor, 
                                 parms, kGSF_WantHandle | flags, data);
   }
   else
   {
#ifndef SHIP
      if (config_is_defined("SoundGhostSpew"))
      {
         mprintf("Failing to play %s,\n", sampleName);
         mprintf("  parms->flag = 0x%x\n", parms->flag);
         mprintf("  flags = 0x%x\n", flags);
         mprintf("  exists(%d) = %d, defaulthost = %d\n", src_obj, gpObjSys->Exists(src_obj), 
                 gpNetMan->AmDefaultHost());
#ifdef NEW_NETWORK_ENABLED
         mprintf("  proxy(%d) = %d\n", src_obj, gpObjNet->ObjIsProxy(src_obj));
#endif
      }
#endif
      return kPSndFail;
   }
} 

int cPropSnd::GenerateSoundHandle(int handle, mxs_vector &vec, ObjID src_obj, ObjID schemaID, 
                                  const char *sampleName, float atten_factor,
                                  sfx_parm *parms, int flags, void *data)
{
   AUTO_TIMER(GenSnd);
   AUTO_TIMER(GenSndVec);

   cRoom *start_room = g_pRooms->GetObjRoom(src_obj);

   if ((start_room == NULL) || (!start_room->PointInside(vec)))
      start_room = g_pRooms->RoomFromPoint(vec);

   // Deal with starting outside the room database
   if (start_room == NULL)
   {
      #ifndef SHIP
      if (!config_is_defined("QuietSoundRooms"))
         mprintf("\"%s\" on %s (vec %g %g %g) not in room!\n",
                 sampleName, ObjWarnName(src_obj), vec.x, vec.y, vec.z);
      #endif

      return kPSndFail;
   }

   m_SoundInstance = &m_SoundInstHigh;

   int init_flags = kSIF_None | ((flags & kGSF_NetSent) ? kSIF_NetSent : kSIF_None) |
                                ((flags & kGSF_Networked) ? kSIF_Networked : kSIF_None) |
                                ((flags & kGSF_ForcePlay) ? kSIF_ForcePlay : kSIF_None);

   // Init sound instance
   m_SoundInstance->Init(handle, src_obj, vec, schemaID, sampleName, init_flags, atten_factor, parms, data);

   #ifndef SHIP
   if (config_is_defined("too_far_sound_count"))
   {
      float dist = 0;
      config_get_float("too_far_sound_count", &dist);

      if (mx_dist_vec(&ObjPosGet(PlayerObject())->loc.vec, &vec) > dist)
         num_too_far++;
      num_sounds++;
   }
   #endif

   // Do the propagation
   m_PropAgent.SetFunctions(m_SoundInstance);
   m_PropAgent.PropagateBF(vec, start_room);

   if ((flags & kGSF_WantHandle) && !m_NoSound)
   {
      cPlayerSoundList* pPlayerList = GeneratePlayerSoundList(handle);
      if (pPlayerList == NULL)
      {
         #ifndef SHIP
         if (config_is_defined("SoundFullSpew"))
            mprintf("GenSoundHandle: %s on %s failed to propagate to the player\n",
                    sampleName, ObjWarnName(src_obj));
         #endif            

         // Make a cSoundInfo and defer it, if we're a looped sample
         if (parms->flag & SFXFLG_LOOP)
         {
            cPlayerSoundList *pPlayerSoundList = new cPlayerSoundList;
            cSoundInfo *pSoundInfo = new cSoundInfo;

            pPlayerSoundList->Append(pSoundInfo);

            pPlayerSoundList->SchemaCallback = parms->end_callback;
            pPlayerSoundList->SchemaUserData = parms->user_data;

            parms->distance = -1;
            parms->end_callback = EndSampleCallback;
            parms->user_data = (void *)handle;

            // TODO : Fill with real values?
            parms->pri = 128;

            pSoundInfo->SrcObject = src_obj;
            pSoundInfo->SrcPoint = vec;
            pSoundInfo->Object = PlayerObject();
            strncpy (pSoundInfo->SampleName, sampleName, 16);
            pSoundInfo->AppsfxParms = *parms;

            pSoundInfo->flags |= kSIF_Deferred;
            pSoundInfo->Handle = handle;

            RegisterSound(handle, pPlayerSoundList);

            #ifndef SHIP
            if (config_is_defined("SoundSpew"))
               mprintf("  looping sample, so deferred (handle %d)\n", handle);
            #endif
            
            return handle;
         }
         else
         {
            #ifndef SHIP
            if (config_is_defined("SoundFullSpew"))
               mprintf("  non-looping sample, so failing\n");
            #endif
   
            return kPSndFail;
         }

         return kPSndFail;
      }

      m_SoundInstance->SetupPlayerSoundListCallbacks(pPlayerList);
      
      #ifndef SHIP
      if (config_is_defined("SoundSpew"))
         mprintf( "Playing vector sound on %s.\n", ObjWarnName(src_obj) );
      #endif

      SNDFrameAddObj( src_obj );

      switch (Prioritize (pPlayerList))
      {
         case PRIORITY_RES_OK:
            if (!(m_SoundInstance->PlaySoundList(pPlayerList)))
            {
               delete pPlayerList->Remove(pPlayerList->GetFirst());
               delete pPlayerList;
               return kPSndFail;
            }
            break;
         case PRIORITY_RES_DEFERRED:
            if (pPlayerList->GetFirst())
               pPlayerList->GetFirst()->flags |= kSIF_Deferred;
            break;
         case PRIORITY_RES_FAIL:
            delete pPlayerList->Remove(pPlayerList->GetFirst());
            delete pPlayerList;
            return kPSndFail;
      }

      RegisterSound (handle, pPlayerList);  

      return handle;
   }
   else
   {
      if (flags & kGSF_WantHandle)
         return kPSndFail;
      else
         return handle;
   }
}

int cPropSnd::GenerateSound(const char *sampleName, sfx_parm *parms)
{
   AUTO_TIMER(GenSnd);
   AUTO_TIMER(GenSndStatic);

   cPlayerSoundList *pPlayerList;
   cSoundInfo       *pSoundInfo;

   int handle;

   if ((handle = GetSoundHandle()) == -1)
   {
      Warning(("No free handles for GenerateSound\n"));
      return kPSndFail;
   }

#ifdef NEW_NETWORK_ENABLED
   if (parms->flag & SFXFLG_NET_AMB)
   {
      // Tell the other players to play this sound
      SoundNetGenerateSound(handle, 
                            sampleName,
                            parms);
   }
#endif

   // Create a dummy sound list, so we can be part of the psnd world
   pPlayerList = new cPlayerSoundList;
   pSoundInfo = new cSoundInfo;

   pPlayerList->Append(pSoundInfo);

   pPlayerList->SchemaCallback = parms->end_callback;
   pPlayerList->SchemaUserData = parms->user_data;

   parms->distance = -1;
   parms->end_callback = EndSampleCallback;
   parms->user_data = (void *)handle;

   pSoundInfo->Object = PlayerObject();
   pSoundInfo->Handle = handle;

   strncpy (pSoundInfo->SampleName, sampleName, 16);
   pSoundInfo->AppsfxParms = *parms;

   pSoundInfo->flags |= kSIF_Static;

   switch (Prioritize(pPlayerList))
   {
      case PRIORITY_RES_OK:
         pSoundInfo->SFXHandle = SFX_Play_Raw(SFX_STATIC, parms, (char *)sampleName);

         if (pSoundInfo->SFXHandle == SFX_NO_HND)
         {
            Warning(("\"%s\" failed to play\n", sampleName));

            delete pPlayerList->Remove(pSoundInfo);
            delete pPlayerList;
            return kPSndFail;
         }
         else
         {
            // Find unused active sound object
            m_SoundInstance->AddActiveSound(pSoundInfo);

            #ifndef SHIP
            if (config_is_defined("SoundSpew"))
               mprintf(" \"%s\" is statically started (handle = %d, sfxhandle = %d)\n", sampleName, handle, pSoundInfo->SFXHandle);
            #endif
         }
         break;
      case PRIORITY_RES_DEFERRED:
         pSoundInfo->flags |= kSIF_Deferred;
         break;
      case PRIORITY_RES_FAIL:
         delete pPlayerList->Remove(pSoundInfo);
         delete pPlayerList;
         return kPSndFail;
   }

   RegisterSound(handle, pPlayerList);

   return handle;
}

////////////////////////////////////////////////////////////////////////////////

void cPropSnd::SoundHalt(int handle)
{
   cPlayerSoundList *pPlayerList;
   cSoundInfo       *pSoundInfo;

   if (m_NoSound)
      return;

   // Make sure the handle is valid
   if((pPlayerList = GetSound(handle)) == NULL)
   {
      Warning (("Called SoundHalt with handle %d, which is NULL\n", handle));
#ifndef SHIP
      SpewSounds();
#endif
      return;
   }

#ifdef NEW_NETWORK_ENABLED
   SoundNetHalt(handle);
#endif
   
   // Tell the sample to stop playing
   pSoundInfo = pPlayerList->GetFirst();

   if (pSoundInfo->flags & kSIF_Deferred)
   {
      // Spoof an end sample callback to schema
      if (pPlayerList->SchemaCallback != NULL)
         pPlayerList->SchemaCallback(handle, pPlayerList->SchemaUserData);

      // We're deferred, so we're not really playing, so just clean up
      delete pPlayerList->Remove(pSoundInfo);
      delete pPlayerList;

      RemoveSound(handle);
   }
   else
      SFX_Kill_Hnd(pSoundInfo->SFXHandle);
}

////////////////////////////////////////

void cPropSnd::SoundHaltObj(ObjID objID)
{
   cDynArray<int> handleKillList;
   sSoundInfo *pSoundInfo;
   int i;

   for (i=0; i<kMaxActiveSounds; i++)
   {
      if (m_SoundList[i])
      {
         pSoundInfo = m_SoundList[i]->GetFirst();

         if ((pSoundInfo->SrcObject == objID) && (pSoundInfo->flags & kSIF_OnObject))
            handleKillList.Append(m_SoundList[i]->GetFirst()->Handle);
      }
   }

   for (i=0; i<handleKillList.Size(); i++)
      SoundHalt(handleKillList[i]);
}

////////////////////////////////////////

void cPropSnd::SoundHaltAll()
{
   for (int i=0; i<kMaxActiveSounds; i++)
   {
      if (m_SoundList[i] != NULL)
         SoundHalt(i);
   }
}

////////////////////////////////////////

BOOL cPropSnd::AreSounds()
{
   for (int i=0; i<kMaxActiveSounds; i++)
   {
      if (m_SoundList[i] != NULL)
         return TRUE;
   }

   return FALSE;
}

////////////////////////////////////////////////////////////////////////////////

int cPropSnd::GetSoundHandle()
{
   for (int i=0; i<kMaxActiveSounds; i++)
   {
      if (m_SoundList[i] == NULL)
         return i;
   }

   return -1;
}

////////////////////////////////////////

int cPropSnd::FindSound(cPlayerSoundList *pSoundList)
{
   for (int i=0; i<kMaxActiveSounds; i++)
   {
      if (m_SoundList[i] == pSoundList)
         return i;
   }

   return -1;
}

////////////////////////////////////////////////////////////////////////////////

void cPropSnd::BlockAdjacentRooms(short roomID1, short roomID2, mxs_real factor)
{
   if ((roomID1 > g_pRooms->GetNumRooms()) || (roomID2 > g_pRooms->GetNumRooms()))
      return;

   cRoom *pRoom1 = g_pRooms->GetRoom(roomID1);
   cRoom *pRoom2 = g_pRooms->GetRoom(roomID2);
   short  r1, r2;
   int  i, j;

   for (i=0; i<pRoom1->GetNumPortals(); i++)
   {
      for (j=0; j<pRoom2->GetNumPortals(); j++)
      {
         r1 = pRoom1->GetPortal(i)->GetFarRoom()->GetRoomID();
         r2 = pRoom2->GetPortal(j)->GetFarRoom()->GetRoomID();

         if (r1 == r2)
         {
            if (factor >= 0)
            {
               SetBlockingFactor(roomID1, r1, factor, FALSE);
               SetBlockingFactor(roomID2, r1, factor, FALSE);
            }
            else
            {
               RemoveBlockingFactor(roomID1, r1, FALSE);
               RemoveBlockingFactor(roomID2, r1, FALSE);
            }
         }
      }
   }
}

////////////////////////////////////////

void cPropSnd::SetBlockingFactor(short roomID1, short roomID2, mxs_real factor, BOOL blockAdjacent)
{
   long key;

   if (blockAdjacent)
      BlockAdjacentRooms(roomID1, roomID2, factor);

   key = roomID1 << 16 | roomID2;
   m_SoundBlockingTable.Set(key, factor);

   key = roomID2 << 16 | roomID1;
   m_SoundBlockingTable.Set(key, factor);
}

////////////////////////////////////////

void cPropSnd::RemoveBlockingFactor(short roomID1, short roomID2, BOOL blockAdjacent)
{
   long key;

   if (blockAdjacent)
      BlockAdjacentRooms(roomID1, roomID2, -1);

   key = roomID1 << 16 | roomID2;
   m_SoundBlockingTable.Delete(key);

   key = roomID2 << 16 | roomID1;
   m_SoundBlockingTable.Delete(key);
}

////////////////////////////////////////

mxs_real cPropSnd::GetBlockingFactor(short roomID1, short roomID2)
{
   long     key;
   mxs_real value;

   key = roomID1 << 16 | roomID2;
   if (m_SoundBlockingTable.Lookup(key, &value))
      return value;
   else
      return 0.0;
}

/////////////////////////////////////////

BOOL cPropSnd::ShouldPlaySound(const sfx_parm *pParms, int flags, BOOL onObject, ObjID srcObject) const
{
#ifdef NEW_NETWORK_ENABLED
   // Always want to play:
   // - a sound that's non-networked
   // - a sound a network message has told us to play
   // - a sound that we've been forced to play
   if ((pParms->flag & SFXFLG_NO_NET) || 
       (flags & kGSF_Networked) || 
       (flags & kGSF_ForcePlay))
      return TRUE;

   if (onObject)
   {
      // Object based sounds are played if we own the object or 
      // the object is local only (== !Proxy)
      return !gpObjNet->ObjIsProxy(srcObject);
   }
   else
   {
      // Vector based sounds are played if the object exists and
      // we own it or it's local only (== !Proxy), or if the object doesn't exist and we're the
      // default host
      return ((srcObject == OBJ_NULL || !gpObjSys->Exists(srcObject)) && gpNetMan->AmDefaultHost()) ||
              (gpObjSys->Exists(srcObject) && !gpObjNet->ObjIsProxy(srcObject));
   }
#else
   // TBD - is this the right thing for no-network? patmc
   return TRUE;
#endif
}

/////////////////////////////////////////

BOOL cPropSnd::ShouldNetworkSound(const sfx_parm *pParms, int flags, BOOL onObject, ObjID srcObject) const
{
#ifdef NEW_NETWORK_ENABLED
   // Never want to network:
   // - a sound we've already networked
   // - a sound that a network message has caused us to play
   // - a sound that is marked as non-networked
   // - a sound from a LocalOnly object
   if ((flags & kSIF_NetSent) ||
       (flags & kSIF_Networked) ||
       (pParms->flag & SFXFLG_NO_NET) ||
       (gpObjSys->Exists(srcObject) && gpObjNet->ObjLocalOnly(srcObject)))
      return FALSE;

   // After that initial filter, always network sounds that we've
   // been forced to play 
   if (flags & kSIF_ForcePlay)
      return TRUE;

   if (onObject)
   {
      // Object-based sounds are networked if we own the object
      return gpObjNet->ObjHostedHere(srcObject);
   }
   else
   {
      // Vector based sounds are networked if the object exists and we own it,
      // or it doesn't exist be we're the default host
      return ((gpObjSys->Exists(srcObject) && gpObjNet->ObjHostedHere(srcObject))) ||
             (((srcObject == OBJ_NULL) || !gpObjSys->Exists(srcObject)) && gpNetMan->AmDefaultHost());
   }
#else
   return FALSE;
#endif
}

/////////////////////////////////////////


// Prioritize determines the prioritized status of a sound that
// would like to be played.  If the new sound bumps an existing
// sound, then it does the bumping.

PrioritizationResult cPropSnd::Prioritize(const cPlayerSoundList* pPlayerList)
{
   const cSoundInfo *pSoundInfo = pPlayerList->GetFirst();
   int thisPri, bumpPri;
   PrioritizationResult retVal  = PRIORITY_RES_OK;
   uchar thisSoundType          = pSoundInfo->AppsfxParms.group;
   uchar thisSoundPriority      = pSoundInfo->AppsfxParms.pri;

   // Check for cap on type.
   bool typeCapReached = TypeCapReached (thisSoundType);

   // Check for any available channels.
   bool channelAvail = SFX_IsChannelAvail();

   #ifndef SHIP
   if (config_is_defined("SoundListSpew"))
   {
      mprintf(" channel avail = %d\n", SFX_IsChannelAvail());
      mprintf(" %d channels playing\n", SFX_Channel_Count());
      SpewSounds();
   }
   #endif

   // Basic concept: Fail if either we have reached the cap or if channels are full
   // and our priority is too low to matter.  We're OK if we haven't reached cap and
   // a channel is avail.  Bump another channel and return OK if we haven't reached
   // the cap and there is a lower priority sound playing.
   if ((!channelAvail) || typeCapReached)
   {
      // Select a bump candidate pased on the existing channels' effective priorities.
      // Notice that if both occur (i.e. !channelAvail && typeCapReached), we use
      // typeCapReached as candidate seletor.  This should guarantee that we never
      // exceed type caps.
      cPlayerSoundList* pBumpCandidate;
      if (typeCapReached)
      {
         #ifndef SHIP
         if (config_is_defined("SoundPriSpew"))
            mprintf("SoundPri: Type cap reached on type %d\n", thisSoundType);
         #endif

         pBumpCandidate = LowestPrioritySoundOfType(thisSoundType, &bumpPri);
      }
      else
      {
         #ifndef SHIP
         if (config_is_defined("SoundPriSpew"))
            mprintf("SoundPri: No channels available\n");
         #endif

         pBumpCandidate = LowestPrioritySound(&bumpPri);
      }

      // Under no conditions should pBumpCandidate be NULL at this point.
      AssertMsg (pBumpCandidate != NULL, "In PSound, Prioritize() Could not find a bump candidate.");

      // Calculate this sound's priority.
      thisPri = EffectivePriority(pPlayerList);

      #ifndef SHIP
      if (config_is_defined("SoundPriSpew"))
         mprintf("SoundPri: New sound priority: %d, bump candidate priority: %d\n", thisPri, bumpPri);
      #endif

      // If this sound is lower or equal priority, we either fail or defer, depending on
      // the type of sound.  This is so that schema is informed or faked
      // properly.
      if (thisPri <= bumpPri)
      {
         if (pSoundInfo->AppsfxParms.flag & SFXFLG_LOOP)
         {
            #ifndef SHIP
            if (config_is_defined("SoundPriSpew"))
               mprintf("SoundPri: New < Candidate: Deferring %s (handle %d).\n", pSoundInfo->SampleName, pSoundInfo->Handle);
            #endif

            retVal = PRIORITY_RES_DEFERRED;
         }
         else
         {
            #ifndef SHIP
            if (config_is_defined("SoundPriSpew"))
               mprintf("SoundPri: New < Candidate: Failing new non-looping sound.\n");
            #endif

            retVal = PRIORITY_RES_FAIL;
         }
      }
      else
      {
         // If it's a sample-looped sound, we have to bump and defer.
         if (pBumpCandidate->GetFirst()->AppsfxParms.flag & SFXFLG_LOOP)
         {
            #ifndef SHIP
            if (config_is_defined("SoundPriSpew"))
               mprintf("SoundPri: Bumping %s (sfxhandle %d) onto deferred list\n", 
                       pBumpCandidate->GetFirst()->SampleName, 
                       pBumpCandidate->GetFirst()->SFXHandle);
            #endif

            BumpActiveSound(pBumpCandidate);
            Assert_(pBumpCandidate->GetFirst()->SFXHandle == -1);
         }
         // All other (non sample-looped) sounds can just get bumped.
         else
         {
            #ifndef SHIP
            if (config_is_defined("SoundPriSpew"))
               mprintf("SoundPri: Bumping %s (sfxhandle %d)\n", 
                       pBumpCandidate->GetFirst()->SampleName,
                       pBumpCandidate->GetFirst()->SFXHandle);
            #endif

            SFX_Kill_Hnd(pBumpCandidate->GetFirst()->SFXHandle);
         }
      }
   }

   return retVal;
}

///////////////////////////////////////

BOOL cPropSnd::TypeCapReached(uchar type)
{
   cPlayerSoundList *pSoundList;
   int soundTypeCount = 0;

   // Count sounds that have the same category.
   for (int i=0; i<kMaxActiveSounds; i++)
   {
      pSoundList = GetSound(i);

      if (pSoundList && !(pSoundList->GetFirst()->flags & kSIF_Deferred) &&
          (m_SoundList[i]->GetFirst()->AppsfxParms.group == type))
         soundTypeCount++;
   }

#ifndef SHIP
   Assert_((type >= 0) && (type < (sizeof(gSoundTypeCaps)) / (sizeof(gSoundTypeCaps[0]))));

   if (soundTypeCount > gSoundTypeCaps[type])
   {
      if (config_is_defined("SoundPriSpew"))
         Warning (("Prioritization: Sound type max has been exceeded on type %d\n", (int) type));
      type = 0;
   }
#endif

   // Check against sound type caps.
   return (soundTypeCount > gSoundTypeCaps[type]);
}

///////////////////////////////////////

cPlayerSoundList* cPropSnd::LowestPrioritySoundOfType (uchar type, int* itsEffectivePriority)
{
   cPlayerSoundList* retVal = NULL;
   int iterEffectivePriority;

   // Loop through all sounds.
   for ( int i=0; i<kMaxActiveSounds; i++ )
   {
      // Only look for sounds which match this one's type.
      if ( m_SoundList[i] != NULL )
      {
         cSoundInfo* pSoundInfo = m_SoundList[i]->GetFirst();
         if ( (!(pSoundInfo->flags & kSIF_Deferred)) &&
              (pSoundInfo->AppsfxParms.group == type) )
         {
            // Calculate the iterator sound's effective priority.
            iterEffectivePriority = EffectivePriority (m_SoundList[i]);
         
            // If it's the first one found, then it's the lowest.
            if (retVal == NULL)
            {
               retVal = m_SoundList[i];
               *itsEffectivePriority = iterEffectivePriority;
            }
            // Compare against previous lowest and replace if it's lower.
            else if (iterEffectivePriority < *itsEffectivePriority)
            {
               retVal = m_SoundList[i];
               *itsEffectivePriority = iterEffectivePriority;
            }
         }
      }
   }

   return retVal;
}

////////////////////////////////////////

cPlayerSoundList* cPropSnd::LowestPrioritySound (int* itsEffectivePriority)
{
   cPlayerSoundList* retVal = NULL;
   int iterEffectivePriority;

   // Loop through all sounds.
   for (int i=0; i<kMaxActiveSounds; i++)
   {
      if ( m_SoundList[i] != NULL )
      {
         cSoundInfo* pSoundInfo = m_SoundList[i]->GetFirst();
         if ( !(pSoundInfo->flags & kSIF_Deferred))
         {
            // Calculate the iterator sound's effective priority.
            iterEffectivePriority = EffectivePriority (m_SoundList[i]);
      
            // If it's the first one found, then it's the lowest.
            if (retVal == NULL)
            {
               retVal = m_SoundList[i];
               *itsEffectivePriority = iterEffectivePriority;
            }
            // Compare against previous lowest and replace if it's lower.
            else if (iterEffectivePriority < *itsEffectivePriority)
            {
               retVal = m_SoundList[i];
               *itsEffectivePriority = iterEffectivePriority;
            }
         }
      }
   }
   return retVal;
}

////////////////////////////////////////

// This is the algorithm for combining whatever it is about a sound that determines
// its effective (i.e. net) priority value.  Currently based on effective volume and
// schema priority value.

int cPropSnd::EffectivePriority(const cPlayerSoundList* pPlayerList)
{
   const cSoundInfo *pSoundInfo = pPlayerList->GetFirst();
   int retVal;

   int priority = pSoundInfo->AppsfxParms.pri;
   int volume = pSoundInfo->Volume;

   // Do tests for bad values of volume and / or priority
   if (priority < 0)
   {
      Warning(("Prioritization : Given a priority of %d, Out of range [0..255].", priority));
      priority = 0;
   }

   if ((volume < -10000) || (volume > 0))
   {
      Warning (("Prioritization : Given a volume of %d, Out of range [-10000..0].", (int) pSoundInfo->Volume));
      volume = -5000;
   }

   // Prioritization function:
   //
   // Effective priority is (mathematically) a function of 2 variables : schema priority
   // and effective volume.  Upon discussion with Eric the sound guy, it makes sense for
   // this function to yield guaranteed max and min effective priorities from maximum and
   // minimum schema priorities, respectively.  If the schema priority is not one of these,
   // it can approach, but never reach, either max or min.  "Normal" (or middle) schema
   // priority, is linear with volume, yielding a range from a notch above absolute
   // minimum to a notch below maximum.  Lower than normal schema priority gives a function
   // that is linear with volume, but ranging from a notch above minimum to some value in
   // middle, scalar with the schema priority.  Higher than normal schema priority gives a function
   // linear with volume, ranging from somewhere in the middle to a notch below maximum.
   // This results in a function of two variables that is continuous over both dimensions
   // except for when schema priority is max or min.
   // Summarily:
   //   ep(p, v)=       max_ep if p == max_p
   //                   min_ep if p == min_p
   //                   m(p)*v + b(p) // (eq. for a line as a function of p)
   //
   float fNormalizedEP;
   float fMinPri, fPriRange;

   // Force max or min.
   if (priority == SCH_PRIORITY_MAX)
      return EFF_PRI_MAX;
   if (priority == SCH_PRIORITY_MIN)
      return EFF_PRI_MIN;

   // Normal priority is purely linear with volume.
   if (priority == SCH_PRIORITY_NORMAL)
      // Simple linear with volume.
      fNormalizedEP = ((float) (volume + 10000)) * ( 1.0F / 10000.0F ); // Normalize to [0..1]
   else
   {
      // Smaller slope for non-normal priority.
      if (priority < SCH_PRIORITY_NORMAL)
      {
         fMinPri = 0.0F;
         fPriRange = (float)priority / (float) SCH_PRIORITY_NORMAL;
      }
      else
      {
         fMinPri = ((float)priority - (float) SCH_PRIORITY_NORMAL) / (float) SCH_PRIORITY_NORMAL;
         fPriRange = 1.0 - fMinPri;
      }

      fNormalizedEP = ((float) (volume + 10000)) * (1.0F / 10000.0F) *            // Normalize to [0..1]
                       fPriRange +                                                              // Find spot on ramp.
                       fMinPri;                                                                 // Add base of ramp.
   }

   // Adjust to discrete values.
   retVal = (int) ( fNormalizedEP *
                    (float) (EFF_PRI_MAX - EFF_PRI_MIN) +                                     // Scale to full range.
                    (float) EFF_PRI_MIN );                                                    // Add in base level.

   // Clip to range (MIN..MAX)
   if (retVal >= EFF_PRI_MAX)
      retVal = EFF_PRI_MAX - 1;
   else if (retVal <= EFF_PRI_MIN)
      retVal = EFF_PRI_MIN + 1;

   return retVal;
}

////////////////////////////////////////

void cPropSnd::BumpActiveSound(cPlayerSoundList *pPlayerList)
{
   m_SoundInstance->BumpActiveSound(pPlayerList);
}

////////////////////////////////////////

#ifndef SHIP
void cPropSnd::SpewActiveSounds()
{
   Assert_(m_SoundInstance);
   char buff[256];

   mprintf("Sound List:\n");

   for (int i=0; i<kMaxActiveSounds; i++)
   {
      cPlayerSoundList *pPlayerList;

      if ((pPlayerList = GetSound(i)) != NULL)
      {
         cSoundInfo *pSoundInfo = pPlayerList->GetFirst();

         sprintf(buff, " [%2d] %13s  sfxhandle %2d  obj %4d  group %2d  dist %5g  %s", i, 
                 pSoundInfo->SampleName, pSoundInfo->SFXHandle, 
                 pSoundInfo->SrcObject, pSoundInfo->AppsfxParms.group, pSoundInfo->Distance,
                 pSoundInfo->flags & kSIF_Deferred ? "(deferred)" : "");
         mprintf("%s\n", buff);
      }
   }
}
#endif

//////////////////////////////////////////////////////////////////////////////

