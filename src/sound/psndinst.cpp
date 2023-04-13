/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sound/psndinst.cpp,v 1.79 2000/03/23 18:33:20 patmac Exp $
//
// Sound propagation functions
//

//#define PROFILE_ON

//#define PSI_TIMING
//#define FSP_TIMING

#include <lg.h>
#include <r3d.h>
#include <2d.h>

#include <math.h>

#include <config.h>
#include <cfgdbg.h>
#include <timings.h>
#include <objedit.h>

#include <matrixs.h>
#include <objpos.h>
#include <rendprop.h>  // SetRenderType & ObjSetHasRefs
#include <playrobj.h>
#include <ged_line.h>
#include <physapi.h>
#include <ghostmvr.h>

#include <comtools.h>
#include <iobjsys.h>
#include <osysbase.h>
#include <appagg.h>
#include <appsfx.h>
#include <schema.h>
#include <schbase.h>
#include <objmedia.h>
#include <iobjnet.h>
#include <sndnet.h>

#include <hashpp.h>
#include <hshpptem.h>

#include <roomsys.h>
#include <room.h>
#include <rooms.h>
#include <roomutil.h>
#include <doorprop.h>

#include <psnd.h>
#include <psndinst.h>

#include <aiapi.h>

#include <roomprop.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>


DECLARE_TIMER(SFX_Play, Average);

////////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
template cSoundInfoTable;
template cSoundPropTable;
#endif

////////////////////////////////////////////////////////////////////////////////

const mxs_real kMaxPathDifference = 10.0;
const mxs_real kSoundConstant = 0.5;

////////////////////////////////////////////////////////////////////////////////

cPropSndInst::cPropSndInst()
 : m_nActiveSoundObjects(0)
{
   m_InfoTable.Clear();
   m_InfoList.SetSize(0);

   for (int i=0; i<kMaxActiveSounds; i++)
   {
      m_ActiveObject[i] = OBJ_NULL;
      m_ActiveSound[i] = NULL;
   }

   m_deferredHaltList.SetSize(0);

   #ifndef SHIP
   m_DrawSoundPath = FALSE;
   #endif

   m_pAIManager = AppGetObj(IAIManager);
}

////////////////////////////////////////

cPropSndInst::~cPropSndInst()
{
   AutoAppIPtr_(ObjectSystem, pObjSys);

   ClearSoundInfo();

   for (int i=0; i<kMaxActiveSounds; i++)
   {
      if (m_ActiveObject[i] != OBJ_NULL)
      {
         if (pObjSys->Exists(m_ActiveObject[i]))
            pObjSys->Destroy(m_ActiveObject[i]);
         m_ActiveObject[i] = OBJ_NULL;
      }
      m_ActiveSound[i] = NULL;
   }
   SafeRelease(m_pAIManager);

   m_deferredHaltList.SetSize(0);
}

////////////////////////////////////////

void cPropSndInst::Init(int handle, ObjID objID, const mxs_vector &loc, ObjID schemaID,
                        const char *sampleName, int flags, float atten_factor, sfx_parm *parms, void *data)
{
   AutoAppIPtr(ObjectSystem);

   m_SrcObject  = objID;
   m_SampleName = sampleName;
   m_pParms     = parms;
   m_Data       = data;
   m_Handle     = handle;
   m_SchemaID   = schemaID;
   m_Flags      = flags;

   m_attenFactor = atten_factor;

#ifndef SHIP
   m_SoundSpew       = config_is_defined("SoundSpew");
   m_SoundGhostSpew  = config_is_defined("SoundGhostSpew");
   m_SoundFullSpew   = config_is_defined("SoundFullSpew");
   m_SoundFullAISpew = config_is_defined("SoundFullAISpew");
   m_SoundHearSpew   = config_is_defined("SoundHearSpew");
   m_SoundPathSpew   = config_is_defined("SoundPathSpew");
#endif

   mx_copy_vec(&m_SrcPoint, (mxs_vector *) &loc);

   if (parms->radius > 0)
   {
      m_MaxDistance = parms->radius;
      m_ScaleDistance = SFX_MaxDist(parms->gain);
   }
   else
   {
      m_MaxDistance   = SFX_MaxDist(parms->gain) * atten_factor;
      m_ScaleDistance = parms->radius;
   }

   sDoorProp *pDoorProp;

   if ((pDoorProp = GetDoorProperty(objID)) != NULL)
   {
      m_roomNoBlock1 = pDoorProp->room1;
      m_roomNoBlock2 = pDoorProp->room2;
   }
   else
   {
      m_roomNoBlock1 = -1;
      m_roomNoBlock2 = -1;
   }

   // Make sure we have reserved our active sound objects
   for (int i=0; i<kMaxActiveSounds; i++)
   {
      if (m_ActiveObject[i] == OBJ_NULL)
      {
         m_ActiveObject[i] = pObjectSystem->Create(ROOT_ARCHETYPE, kObjectConcrete);

         pObjectSystem->SetObjTransience(m_ActiveObject[i], TRUE);
         ObjSetRenderType(m_ActiveObject[i], kRenderNotAtAll);
         ObjSetHasRefs(m_ActiveObject[i], FALSE);
      }
   }
}

////////////////////////////////////////

void cPropSndInst::Cleanup()
{
   AutoAppIPtr_(ObjectSystem, pObjSys);

   // Order all sounds to halt
   g_pPropSnd->SoundHaltAll();

   // Destroy all active sounds
   for (int i=0; i<kMaxActiveSounds; i++)
   {
      if (m_ActiveObject[i] != OBJ_NULL)
      {
         if (pObjSys->Exists(m_ActiveObject[i]))
            pObjSys->Destroy(m_ActiveObject[i]);
         m_ActiveObject[i] = OBJ_NULL;
      }
   }
}

////////////////////////////////////////

// Keep in mind that the only reason this is a list is because when schema
// asks for a sound to be played it may result in it being played in several
// locations if the sound has taken multiple paths to the player.  Because
// we merge the sound down to one pretty much immediately, we're almost always
// dealing with a one-element list, which isn't much of a list at all, really.

cPlayerSoundList *cPropSndInst::GeneratePlayerSoundList(int handle)
{
   cPlayerSoundList *pPlayerList = NULL;
   cPlayerSoundList *pCurPlayerList = NULL;
   cSoundInfo       *pSoundInfo;
   cSoundInfo       *pSoundInfoNext;

   // Get the player's sound list
   if (PlayerObjectExists() && m_InfoTable.Lookup(PlayerObject(), (cSoundInfoList **)&pPlayerList))
   {
      // Allocate list for current sound
      pCurPlayerList = new cPlayerSoundList;

#if 0
      mprintf("Waiting sounds:\n");
      pSoundInfo = pPlayerList->GetFirst();
      while (pSoundInfo)
      {
         mprintf("  [%d] %s\n", pSoundInfo->Handle, pSoundInfo->SampleName);
         pSoundInfo = pSoundInfo->GetNext();
      }
#endif

      // Pull out all the current sounds
      pSoundInfo = pPlayerList->GetFirst();
      while (pSoundInfo != NULL)
      {
         pSoundInfoNext = pSoundInfo->GetNext();

         if (pSoundInfo->Handle == handle)
         {
            pPlayerList->Remove(pSoundInfo);
            pCurPlayerList->Append(pSoundInfo);
         }

         pSoundInfo = pSoundInfoNext;
      }

      // Test for no sounds
      pSoundInfo = pCurPlayerList->GetFirst();
      if (pSoundInfo != NULL)
      {
         // Merge our sounds down to just one
         MergeSounds(pCurPlayerList);

         pSoundInfo = pCurPlayerList->GetFirst();

         #ifndef SHIP
         if (m_SoundSpew)
         {
            mprintf(" [Player] hears \"%s\" at %g %g %g (%d dist) flags %x\n", pSoundInfo->SampleName,
                    pSoundInfo->Position.x, pSoundInfo->Position.y, pSoundInfo->Position.z,
                    (int)pSoundInfo->Distance, pSoundInfo->flags);
         }
         #endif
      }
      else
      {
         delete pCurPlayerList;
         pCurPlayerList = NULL;
      }
   }

   return pCurPlayerList;
}

//////////////////////////////////////

void cPropSndInst::SetupPlayerSoundListCallbacks(cPlayerSoundList *pCurPlayerList)
{
   cSoundInfo* pSoundInfo = pCurPlayerList->GetFirst();

   // Make copy of schema info we're interested in
   pCurPlayerList->SchemaCallback = m_pParms->end_callback;
   pCurPlayerList->SchemaLoopCallback = m_pParms->loop_callback;
   pCurPlayerList->SchemaUserData = m_pParms->user_data;

   Assert_(pCurPlayerList->SchemaCallback != EndSampleCallback);

   // Point our params to us
   pSoundInfo->AppsfxParms.end_callback  = EndSampleCallback;
   pSoundInfo->AppsfxParms.loop_callback  = LoopSampleCallback;
   pSoundInfo->AppsfxParms.user_data = (void *) (pSoundInfo->Handle);
   //   pSoundInfo->AppsfxParms.user_data = (void *)handle;
}

//////////////////////////////////////////

BOOL cPropSndInst::PlaySoundList(cPlayerSoundList *pCurPlayerList)
{
   BOOL retVal = TRUE;
   cSoundInfo *pSoundInfo = pCurPlayerList->GetFirst();

   // Determine if this is an active sound
   if (ActiveSound(pSoundInfo) && (m_nActiveSoundObjects < kMaxActiveSounds))
   {
      #ifndef SHIP
      if (m_SoundSpew)
         mprintf("    becomes active sound\n");
      #endif

      // Find unused active sound object
      int j;
      for (j = 0; j<kMaxActiveSounds && (m_ActiveSound[j] != NULL); j++);

      AssertMsg(j != kMaxActiveSounds, "Unable to find available active sound object");

      // Point the sound struct at its active sound object
      pSoundInfo->flags &= ~kSIF_Deferred;
      pSoundInfo->ActiveObject = m_ActiveObject[j];
      m_ActiveSound[j] = pSoundInfo;
      m_nActiveSoundObjects++;

      // Position the object
      ObjTranslate(pSoundInfo->ActiveObject, &pSoundInfo->Position);
      pSoundInfo->AppsfxParms.distance = (int)(pSoundInfo->Distance + 0.5);

      AUTO_TIMER(SFX_Play);

      // Play sound on object
      pSoundInfo->SFXHandle = SFX_Play_Obj(SFX_3D, &pSoundInfo->AppsfxParms,
                                           pSoundInfo->SampleName, pSoundInfo->ActiveObject);
   }
   else
   {
      pSoundInfo->AppsfxParms.distance = (int)(pSoundInfo->Distance + 0.5);

      AUTO_TIMER(SFX_Play);

      // Fire and forget
      pSoundInfo->SFXHandle = SFX_Play_Vec(SFX_3D, &pSoundInfo->AppsfxParms,
                                           pSoundInfo->SampleName, &pSoundInfo->Position);
   }

   // With pioritization, this should never fail, but just in case
   if (pSoundInfo->SFXHandle == SFX_NO_HND)
   {
#ifndef SHIP
      mprintf("\"%s\" on %s failed to play!\n", pSoundInfo->SampleName, ObjWarnName(pSoundInfo->SrcObject));
#endif

      // Clear from active sound list
      if (pSoundInfo->ActiveObject != OBJ_NULL)
         EndActiveSound(pSoundInfo);

      retVal = FALSE;

      #ifndef SHIP
      if (m_SoundSpew)
         mprintf("    Yikes, couldn't play!\n");
      #endif
   }
   else
   {
      #ifndef SHIP
      if (m_SoundSpew)
         mprintf("    handle = %d, sfxhandle = %d\n", pSoundInfo->Handle, pSoundInfo->SFXHandle);
      #endif
   }

   return retVal;
}


////////////////////////////////////////

void EndSampleCallback(int hSound, void *data)
{
   cPlayerSoundList *pPlayerSoundList;
   cSoundInfo       *pSoundInfo;
   int handle = (int)data;

   if (g_pPropSnd == NULL)
      return;

   pPlayerSoundList = g_pPropSnd->GetSound(handle);

   if (pPlayerSoundList == NULL)
   {
#ifndef SHIP
      mprintf("End sample %d, unable to find handle (hSound = %d)\n", handle, hSound);
#endif
      return;
   }

   // Get the sound info for the sample
   pSoundInfo = pPlayerSoundList->GetFirst();

   AssertMsg(pSoundInfo != NULL, "Empty sound list?");
   AssertMsg(pSoundInfo->GetNext() == NULL, "Multiple sounds in list?");
   AssertMsg3(pSoundInfo->SFXHandle == hSound, "Invalid handle for ended sample %s (sfx = %d, hnd = %d)",
              pSoundInfo->SampleName, pSoundInfo->SFXHandle, hSound);

   // If bumping a sound, do nothing
   if (pSoundInfo->flags & kSIF_Intercept)
   {
      pSoundInfo->SFXHandle = SFX_NO_HND;
      pSoundInfo->flags &= ~kSIF_Intercept;
      return;
   }

   #ifndef SHIP
   if (config_is_defined("SoundSpew"))
      mprintf("Sample %s ended (handle %d, sfxhandle = %d)\n", pSoundInfo->SampleName, hSound, pSoundInfo->SFXHandle);
   #endif

   // If it's an active sound inform that it's being deleted
   if (!(pSoundInfo->flags & kSIF_Deferred))
       g_pPropSnd->EndActiveSound(pSoundInfo);

   // Callback to schema system
   if ( pPlayerSoundList->SchemaCallback != NULL )
      pPlayerSoundList->SchemaCallback(handle, pPlayerSoundList->SchemaUserData);

   // Delete it
   delete pPlayerSoundList->Remove(pSoundInfo);

   // Delete the list
   delete pPlayerSoundList;

   g_pPropSnd->RemoveSound(handle);
}


////////////////////////////////////////

void LoopSampleCallback(int hSound, void *data)
{
   cPlayerSoundList *pPlayerSoundList;
   cSoundInfo       *pSoundInfo;
   int handle = (int)data;

   if (g_pPropSnd == NULL)
      return;

   pPlayerSoundList = g_pPropSnd->GetSound(handle);

   if (pPlayerSoundList == NULL)
   {
      Warning(("Loop sample %d, unable to find handle\n", handle));
      return;
   }

   // Get the sound info for the sample
   pSoundInfo = pPlayerSoundList->GetFirst();

   #ifndef SHIP
   if (config_is_defined("SoundFullSpew"))
      mprintf("Sample looped (handle %d, sfxhandle = %d)\n", hSound,
              pSoundInfo->SFXHandle);
   #endif

   AssertMsg(pSoundInfo != NULL, "Empty sound list?");
   AssertMsg(pSoundInfo->GetNext() == NULL, "Multiple sounds in list?");
   AssertMsg3(pSoundInfo->SFXHandle == hSound,
              "Invalid handle for looped sample %s (sfx = %d, hnd = %d)",
              pSoundInfo->SampleName, pSoundInfo->SFXHandle, hSound);

   // Callback to schema system
   if (pPlayerSoundList->SchemaLoopCallback != NULL)
      pPlayerSoundList->SchemaLoopCallback(handle, pPlayerSoundList->SchemaUserData);
}

////////////////////////////////////////

void cPropSndInst::ApplySoundInfo()
{
   cDynArray<int> handleProcessedList;

   cSoundInfoList *pSoundInfoList;
   cSoundInfo     *pSoundInfo;

   UpdateActiveSounds();
   UpdateDeferredSounds();

   // Always do the player first, so the sound will be on the sound list when it
   // gets to the other players -- that way, they'll be able to set NetSent on it
   // if necessary.

   if (PlayerObjectExists() && (m_InfoTable.Lookup(PlayerObject(), &pSoundInfoList)))
   {
      ResolveActiveSounds(PlayerObject());
      ResolveDeferredSounds(PlayerObject());
   }

   for (int i=0; i<m_InfoList.Size(); i++)
   {
      // already done the player
      if (m_InfoList[i] == PlayerObject())
         continue;

      if (m_InfoTable.Lookup(m_InfoList[i], &pSoundInfoList))
      {
         pSoundInfo = pSoundInfoList->GetFirst();
         while (pSoundInfo != NULL)
         {
            // @HACK: cache this property somewhere
            if (GetPropertyNamed("AI")->IsRelevant(pSoundInfo->Object))
            {
               // AI HOOKUP HERE
               //
               // The pSoundInfo is the thing that you want to look at, the
               // structure is defined in psndinfo.h.  m_InfoList[i] is the
               // AI that is currently hearing the sound.
               //
               if (m_pAIManager)
                  m_pAIManager->NotifySound(pSoundInfo);

               #ifndef SHIP
               if (m_SoundFullAISpew && (pSoundInfo->SrcObject != pSoundInfo->Object))
               {
                  mprintf(" [%d] hears \"%s\" (%d) at %g %g %g (%d dist)\n", m_InfoList[i],
                          pSoundInfo->SampleName, pSoundInfo->SchemaID,
                          pSoundInfo->Position.x, pSoundInfo->Position.y, pSoundInfo->Position.z,
                          (int)pSoundInfo->Distance);
               }
               #endif
            }
            else
            if ((GetGhostType(pSoundInfo->Object) & kGhostListenTypes) == kGhostListenTypes)
            {
               int j;

               #ifndef SHIP
               //if (m_SoundGhostSpew)
               //   mprintf(" Ghost %d hears %s (flags %x)\n", pSoundInfo->Object, pSoundInfo->SampleName,
               //           pSoundInfo->flags);
               #endif

               if (g_pPropSnd->ShouldNetworkSound(&pSoundInfo->AppsfxParms, pSoundInfo->flags,
                                                  pSoundInfo->flags & kSIF_OnObject, pSoundInfo->SrcObject))
               {
                  BOOL found = FALSE;

                  // See if we've already dealt with this sound this frame, and ignore it if so.
                  // We may validly have duplicates if we've both generated and re-propagated
                  // before we get to this code, which happens fairly often when the sound is nearby.
                  for (j=0; j<handleProcessedList.Size(); j++)
                  {
                     if (handleProcessedList[j] == pSoundInfo->Handle)
                     {
                        found = TRUE;
                        break;
                     }
                  }

                  if (!found)
                  {
                     handleProcessedList.Append(pSoundInfo->Handle);

                     #ifndef SHIP
                     if (m_SoundGhostSpew)
                     {
                        mprintf(" [%d] remote, so playing \"%s\" remotely (handle %d)\n",
                                pSoundInfo->Object, pSoundInfo->SampleName, pSoundInfo->Handle);
                     }
                     #endif

                     // Tell other players to play this sound
                     if (pSoundInfo->flags & kSIF_OnObject)
                     {
                        SoundNetGenerateSoundObj(pSoundInfo->Handle, pSoundInfo->SrcObject, pSoundInfo->SchemaID,
                                                 pSoundInfo->SampleName, pSoundInfo->AttenFactor,
                                                 &pSoundInfo->AppsfxParms);
                     }
                     else
                     {
                        SoundNetGenerateSoundVec(pSoundInfo->Handle, &pSoundInfo->SrcPoint,
                                                 pSoundInfo->SchemaID, pSoundInfo->SampleName,
                                                 pSoundInfo->AttenFactor, &pSoundInfo->AppsfxParms);
                     }

                     // We want to make sure that we only send this sound message once, even if
                     // we're re-propagating it.
                     // There are a number of cases that need to be dealt with:
                     //
                     // - Oneshot, out of range of the player: Should propagate a single time
                     //   only, and then fail to play.  No reprop to be dealt with.
                     //
                     // - Oneshot, in range of the player: Will become an active sound and reprop
                     //   fairly often until the sample ends.  We send the sound message immediately.
                     //
                     // - Looping, out of range of the player: Will drop onto the deferred list and
                     //   be propagated regularly.  As soon as it's in range of any player, it's
                     //   broadcast to everyone.  It will then either play or be deferred on the
                     //   remote machines.  In either case, the host machine is done with it.
                     //
                     // - Looping, in range of the player: Sent immediately to everyone.  They will
                     //   either play it if it's in range, or defer it.  Again, the host machine is
                     //   done with it.
                     //
                     // All of these cases are simply handled by marking each locally-generated sound,
                     // whether deferred or not, as being NetSent when they're broadcast to the remote
                     // machines.  I believe that will cause the proper behavior everywhere.

                  }
                  else
                  {
                     #ifndef SHIP
                     if (m_SoundGhostSpew)
                        mprintf("  dup sound, not sending\n");
                     #endif
                  }
               }

               // Look through all sounds we're keeping track of, and mark them as NetSent, so
               // when they're reprop'ed, they know to set the flag.

               cPlayerSoundList *pPlayerSoundList;
               cSoundInfo       *pSound;

               for (j=0; j<kMaxActiveSounds; j++)
               {
                  if ((pPlayerSoundList = g_pPropSnd->GetSound(j)) != NULL)
                  {
                     pSound = pPlayerSoundList->GetFirst();
                     if (pSound->Handle == pSoundInfo->Handle)
                        pSound->flags |= kSIF_NetSent;
                  }
               }

            }

            delete pSoundInfoList->Remove(pSoundInfo);
            pSoundInfo = pSoundInfoList->GetFirst();
         }

         delete pSoundInfoList;
         m_InfoTable.Delete(m_InfoList[i]);

         m_InfoList.DeleteItem(i);

         i--;
      }
   }
}

////////////////////////////////////////

void cPropSndInst::ClearSoundInfo()
{
   cSoundInfoList *pSoundInfoList;
   cSoundInfo     *pSoundInfo;

   for (int i=0; i<m_InfoList.Size(); i++)
   {
      if (m_InfoTable.Lookup(m_InfoList[i], &pSoundInfoList))
      {
         pSoundInfo = pSoundInfoList->GetFirst();
         while (pSoundInfo != NULL)
         {
            pSoundInfoList->Remove(pSoundInfo);
            delete pSoundInfo;
            pSoundInfo = pSoundInfoList->GetFirst();
         }
         delete pSoundInfoList;
      }
   }

   m_InfoTable.Clear();
   m_InfoList.SetSize(0);
}

////////////////////////////////////////////////////////////////////////////////

void cPropSndInst::AddActiveSound(cSoundInfo *pSoundInfo)
{
// @OBB: i removed an ifndef SHIP, since we need to set J for below
   int j;
   for (j=0; j<kMaxActiveSounds && (m_ActiveSound[j] != NULL); j++);

   AssertMsg(j != kMaxActiveSounds, "Unable to find available active sound object");

   // Point the sound struct at its active sound object
   pSoundInfo->flags &= ~kSIF_Deferred;
   m_ActiveSound[j] = pSoundInfo;
   m_nActiveSoundObjects++;
}

void cPropSndInst::EndActiveSound(cSoundInfo *pActiveSound)
{
   cPlayerSoundList *pPlayerList;
   cSoundInfo       *pPlayerSoundInfo;
   cSoundInfo       *pPlayerSoundInfoNext;
   int i, j;

   // Find it in array
   for (i=0; i<kMaxActiveSounds; i++)
   {
      if (m_ActiveSound[i] == pActiveSound)
         break;
   }

   if (i != kMaxActiveSounds)
   {
      // Remove soundinfo from player list, just in case it's there
      if (PlayerObjectExists() && m_InfoTable.Lookup(PlayerObject(), (cSoundInfoList **)&pPlayerList))
      {
         pPlayerSoundInfo = pPlayerList->GetFirst();
         while (pPlayerSoundInfo != NULL)
         {
            pPlayerSoundInfoNext = pPlayerSoundInfo->GetNext();

            if (pPlayerSoundInfo->Handle == m_ActiveSound[i]->Handle)
               delete pPlayerList->Remove(pPlayerSoundInfo);

            pPlayerSoundInfo = pPlayerSoundInfoNext;
         }

         // If the player list is empty, delete it
         if (pPlayerList->GetFirst() == NULL)
         {
            delete pPlayerList;
            m_InfoTable.Delete(PlayerObject());

            for (j=0; j<m_InfoList.Size(); j++)
            {
               if (m_InfoList[j] == PlayerObject())
                  break;
            }

            if (j != m_InfoList.Size())
               m_InfoList.DeleteItem(j);
            else
               Warning(("EndActiveSound::player list empty, couldn't find in infolist\n"));
         }
      }

      // Clear pointer and decrement count
      m_ActiveSound[i] = NULL;
      m_nActiveSoundObjects--;
   }
   else
      Warning(("Unable to find active sound to end\n"));
}

////////////////////////////////////////////////////////////////////////////////

void cPropSndInst::BumpActiveSound(cPlayerSoundList *pPlayerList)
{
   cSoundInfo* pSoundInfo = pPlayerList->GetFirst();

   for (int i=0; i<kMaxActiveSounds; i++)
   {
      if (m_ActiveSound[i] == pPlayerList->GetFirst())
      {
         // Set our "intercept" flag on this sample
         pSoundInfo->flags |= kSIF_Intercept;
         SFX_Kill_Hnd(pSoundInfo->SFXHandle);

         // Since we're taking it off the active list, make sure it's on the main list
         BOOL found = FALSE;
         for (int j=0; j<kMaxActiveSounds && !found; j++)
            found = (g_pPropSnd->GetSound(j) == pPlayerList);

         AssertMsg2(found, "Attempt to bump %s (handle %d), but not on SoundList!",
                    pSoundInfo->SampleName, pSoundInfo->Handle);

         // And clear off active list
         pSoundInfo->FramesUntilUpdate = kDeferredUpdateFrequency;
         pSoundInfo->ActiveObject = OBJ_NULL;
         pSoundInfo->flags |= kSIF_Deferred;

         #ifndef SHIP
         if (m_SoundSpew)
            mprintf("Bumping \"%s\" (handle %d) onto deferred list\n",
                    pSoundInfo->SampleName, pSoundInfo->Handle);
         #endif

         m_ActiveSound[i] = NULL;
         m_nActiveSoundObjects--;

         return;
      }
   }

   CriticalMsg2("Attempt to bump sound %s (handle %d), but not on ActiveList!",
                pSoundInfo->SampleName, pSoundInfo->Handle);
}

////////////////////////////////////////////////////////////////////////////////

BOOL cPropSndInst::MergeSounds(cSoundInfoList *pSoundInfoList)
{
   cSoundInfo *pSoundInfo;
   cSoundInfo *pSoundInfoNext;
   cSoundInfo *pMergeSoundInfo;
   mxs_real    min_dist;
   mxs_vector  glob;
   mxs_matrix  locframe;
   mxs_vector  sum_locdelta;
   mxs_real    weight_accum = 0;

   pSoundInfo = pSoundInfoList->GetFirst();

   // If empty or just one, don't merge
   if ((pSoundInfo == NULL) || (pSoundInfo->GetNext() == NULL))
      return FALSE;

   // Find the smallest distance
   min_dist = pSoundInfo->Distance;
   while (pSoundInfo != NULL)
   {
      if (pSoundInfo->Distance < min_dist)
         min_dist = pSoundInfo->Distance;
       pSoundInfo = pSoundInfo->GetNext();
   }

   // Weed out too-far sounds
   pSoundInfo = pSoundInfoList->GetFirst();
   while (pSoundInfo != NULL)
   {
      pSoundInfoNext = pSoundInfo->GetNext();

      if (pSoundInfo->Distance > (min_dist + kMaxPathDifference))
         delete pSoundInfoList->Remove(pSoundInfo);

      pSoundInfo = pSoundInfoNext;
   }

   // Point back to start and bail if down to only one
   pSoundInfo = pSoundInfoList->GetFirst();
   if (pSoundInfo->GetNext() == NULL)
      return FALSE;

   // Make new sound structure
   pMergeSoundInfo = new cSoundInfo(*pSoundInfo);

   #ifndef SHIP
   if (m_SoundFullSpew)
      mprintf("Merging sounds from %s\n", ObjWarnName(pSoundInfo->SrcObject));
   #endif

   // Set up for merge
   mx_ang2mat(&locframe, &ObjPosGet(PlayerObject())->fac);

   mx_zero_vec(&sum_locdelta);

   while (pSoundInfo != NULL)
   {
      mxs_vector locdelta;
      mxs_vector delta;
      mxs_real   weight;

      // Weight factor is square of amout from max path difference
      weight = min_dist + kMaxPathDifference - pSoundInfo->Distance;
      weight = weight * weight;

      if (weight == 0)
         weight = 0.5;

      // Get vector to sound
      mx_sub_vec(&delta, &pSoundInfo->Position, &ObjPosGet(PlayerObject())->loc.vec);
      if (mx_mag2_vec(&delta) > 0.0001)
         mx_normeq_vec(&delta);

      // Put in head-relative coords
      mx_mat_mul_vec(&locdelta, &locframe, &delta);

      // Flatten and re-normalize
      locdelta.z = 0;
      if (mx_mag2_vec(&locdelta) > 0.0001)
         mx_normeq_vec(&locdelta);

      // Scale by weight
      mx_scale_addeq_vec(&sum_locdelta, &locdelta, weight);

      // Accumulate for later divide
      weight_accum += weight;

      // Bye bye
      delete pSoundInfoList->Remove(pSoundInfo);
      pSoundInfo = pSoundInfoList->GetFirst();
   }

   pMergeSoundInfo->Distance = min_dist;

   // Average back down
   AssertMsg(weight_accum > 0, "Negative accumulated sound weight?");
   mx_scaleeq_vec(&sum_locdelta, 1 / weight_accum);

   // And place far enough away that a frame's worth of movement is irrelevant
   if (mx_mag2_vec(&sum_locdelta) > 0.0001)
      mx_normeq_vec(&sum_locdelta);
   mx_scaleeq_vec(&sum_locdelta, 100);

   // Convert back to world coords
   mx_inveq_mat(&locframe);
   mx_mat_mul_vec(&glob, &locframe, &sum_locdelta);
   mx_addeq_vec(&glob, &ObjPosGet(PlayerObject())->loc.vec);

   mx_copy_vec(&pMergeSoundInfo->Position, &glob);

   pSoundInfoList->Append(pMergeSoundInfo);

   return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

// This function is called once per frame.  It repropagates any active sounds that
// are ready to be repropagated.

void cPropSndInst::UpdateActiveSounds()
{
   cPlayerSoundList *pPlayerList;
   cSoundInfo       *pSoundInfo;

   for (int i=0; i<kMaxActiveSounds; i++)
   {
      if ((m_ActiveSound[i] != NULL) && !(m_ActiveSound[i]->flags & kSIF_Static))
      {
         if (--m_ActiveSound[i]->FramesUntilUpdate <= 0)
         {
            pSoundInfo = m_ActiveSound[i];

            pSoundInfo->flags &= ~kSIF_HasBeenUpdated;

            #ifndef SHIP
            if (m_SoundFullSpew)
            {
               mprintf("Re-propagating \"%s\"", pSoundInfo->SampleName);
               if (pSoundInfo->flags & kSIF_OnObject)
                  mprintf(" from %s", ObjWarnName(pSoundInfo->SrcObject));
               mprintf(" flags %x\n", pSoundInfo->flags);
            }
            #endif

            // Remove the sound to be re-propagated from the object list
            if (PlayerObjectExists() &&
                m_InfoTable.Lookup(PlayerObject(), (cSoundInfoList **)&pPlayerList))
            {
               cSoundInfo *pPlayerSoundInfo;
               cSoundInfo *pPlayerSoundInfoNext;

               pPlayerSoundInfo = pPlayerList->GetFirst();
               while (pPlayerSoundInfo != NULL)
               {
                  pPlayerSoundInfoNext = pPlayerSoundInfo->GetNext();

                  Assert_(pPlayerSoundInfo != pSoundInfo);

                  if (pPlayerSoundInfo->Handle == pSoundInfo->Handle)
                     delete pPlayerList->Remove(pPlayerSoundInfo);

                  pPlayerSoundInfo = pPlayerSoundInfoNext;
               }
            }

            int gen_flags = ((pSoundInfo->flags & kSIF_Networked) ? kGSF_Networked : kGSF_None) |
                            ((pSoundInfo->flags & kSIF_NetSent) ? kGSF_NetSent : kGSF_None);

            // Re-propagate sound
            if (pSoundInfo->flags & kSIF_OnObject)
            {
               g_pPropSnd->GenerateSoundHandle(pSoundInfo->Handle,
                                               pSoundInfo->SrcObject,
                                               pSoundInfo->SchemaID,
                                               pSoundInfo->SampleName,
                                               pSoundInfo->AttenFactor,
                                               &pSoundInfo->AppsfxParms,
                                               gen_flags, pSoundInfo->Data);
            }
            else
            {
               g_pPropSnd->GenerateSoundHandle(pSoundInfo->Handle,
                                               pSoundInfo->SrcPoint,
                                               pSoundInfo->SrcObject,
                                               pSoundInfo->SchemaID,
                                               pSoundInfo->SampleName,
                                               pSoundInfo->AttenFactor,
                                               &pSoundInfo->AppsfxParms,
                                               gen_flags, pSoundInfo->Data);
            }

            mxs_real min_dist = mx_dist_vec(&ObjPosGet(PlayerObject())->loc.vec,
                                            &ObjPosGet(pSoundInfo->ActiveObject)->loc.vec);

            if (pSoundInfo->flags & kSIF_OnObject)
            {
               if (ObjPosGet(pSoundInfo->SrcObject) != NULL)
               {
                  mxs_real new_dist = mx_dist_vec(&ObjPosGet(PlayerObject())->loc.vec,
                                                  &ObjPosGet(pSoundInfo->SrcObject)->loc.vec);
                  if (new_dist < min_dist)
                     min_dist = new_dist;
               }
            }
            else
            {
               mxs_real new_dist = mx_dist_vec(&ObjPosGet(PlayerObject())->loc.vec,
                                               &pSoundInfo->SrcPoint);

               if (new_dist < min_dist)
                  min_dist = new_dist;
            }

            pSoundInfo->FramesUntilUpdate = min_dist / 10.0;

            if (pSoundInfo->FramesUntilUpdate > kMinUpdateFrequency)
               pSoundInfo->FramesUntilUpdate = kMinUpdateFrequency;

            if (pSoundInfo->FramesUntilUpdate < kMaxUpdateFrequency)
               pSoundInfo->FramesUntilUpdate = kMaxUpdateFrequency;

            #ifndef SHIP
            if (m_SoundFullSpew)
               mprintf("fuu = %d (for handle %d)\n", pSoundInfo->FramesUntilUpdate, pSoundInfo->Handle);
            #endif
         }
      }
   }
}

////////////////////////////////////////////////////////////////////////////////

void cPropSndInst::UpdateDeferredSounds()
{
   cPlayerSoundList *pPlayerSoundList;
   cSoundInfo       *pDeferredSound;

   for (int i=0; i<kMaxActiveSounds; i++)
   {
      // Grab any sound in this slot
      if ((pPlayerSoundList = g_pPropSnd->GetSound(i)) != NULL)
      {
         pDeferredSound = pPlayerSoundList->GetFirst();

         // Only deal with deferred sounds here
         if (pDeferredSound->flags & kSIF_Deferred)
         {
            if (--pDeferredSound->FramesUntilUpdate <= 0)
            {
               pDeferredSound->flags &= ~kSIF_HasBeenUpdated;

               if (pDeferredSound->flags & kSIF_Static)
               {
                  // We don't re-propagate statics, they're always good
                  if (PlayerObjectExists())
                  {
                     #ifndef SHIP
                     if (m_SoundFullSpew)
                        mprintf("Re-triggering deferred static \"%s\"\n", pDeferredSound->SampleName);
                     #endif

                     cPlayerSoundList *pPlayerList;

                     // Create a new sound list if there isn't one waiting
                     if (!m_InfoTable.Lookup(PlayerObject(), (cSoundInfoList **)&pPlayerList))
                        pPlayerList = new cPlayerSoundList;

                     pPlayerList->SchemaCallback = pPlayerSoundList->SchemaCallback;
                     pPlayerList->SchemaLoopCallback = pPlayerSoundList->SchemaLoopCallback;
                     pPlayerList->SchemaUserData = pPlayerList->SchemaUserData;

                     cSoundInfo *pNewSoundInfo = new cSoundInfo;

                     pNewSoundInfo->flags = pDeferredSound->flags;
                     pNewSoundInfo->Object = PlayerObject();
                     pNewSoundInfo->Handle = pDeferredSound->Handle;

                     strncpy(pNewSoundInfo->SampleName, pDeferredSound->SampleName, 16);
                     pNewSoundInfo->AppsfxParms = pDeferredSound->AppsfxParms;

                     pPlayerList->Append(pNewSoundInfo);
                  }
               }
               else
               {
                  #ifndef SHIP
                  if (m_SoundFullSpew)
                  {
                     mprintf("Re-propagating deferred \"%s\"", pDeferredSound->SampleName);
                     if (pDeferredSound->flags & kSIF_OnObject)
                        mprintf(" from %s\n", ObjWarnName(pDeferredSound->SrcObject));
                     else
                        mprintf("\n");
                  }
                  #endif

                  int gen_flags = ((pDeferredSound->flags & kSIF_Networked) ? kGSF_Networked : kGSF_None) |
                                  ((pDeferredSound->flags & kSIF_NetSent) ? kGSF_NetSent : kGSF_None);

                  // Re-propagate sound
                  if (pDeferredSound->flags & kSIF_OnObject)
                  {
                     g_pPropSnd->GenerateSoundHandle(pDeferredSound->Handle,
                                                     pDeferredSound->SrcObject,
                                                     pDeferredSound->SchemaID,
                                                     pDeferredSound->SampleName,
                                                     pDeferredSound->AttenFactor,
                                                     &pDeferredSound->AppsfxParms,
                                                     gen_flags, pDeferredSound->Data);
                  }
                  else
                  {
                     g_pPropSnd->GenerateSoundHandle(pDeferredSound->Handle,
                                                     pDeferredSound->SrcPoint,
                                                     pDeferredSound->SrcObject,
                                                     pDeferredSound->SchemaID,
                                                     pDeferredSound->SampleName,
                                                     pDeferredSound->AttenFactor,
                                                     &pDeferredSound->AppsfxParms,
                                                     gen_flags, pDeferredSound->Data);
                  }
               }

               pDeferredSound->FramesUntilUpdate = kDeferredUpdateFrequency;
            }
         }
      }
   }
}

////////////////////////////////////////////////////////////////////////////////

BOOL cPropSndInst::ActiveSound(cSoundInfo *pSoundInfo)
{
   // @TODO: Heuristic for whether this sound should be updated each frame

   return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

// This function is called once per frame for the player object after repropagation.
// It updates 3D location for active sounds.

void cPropSndInst::ResolveActiveSounds(ObjID hearingObj)
{
   cSoundInfoList *pSoundInfoList;
   cSoundInfoList AccumList;
   cSoundInfo *pActiveSound;
   cSoundInfo *pSoundInfo;
   cSoundInfo *pSoundInfoNext;
   int i;

   for (i=0; i<kMaxActiveSounds; i++)
   {
      if ((m_ActiveSound[i] != NULL) && !(m_ActiveSound[i]->flags & kSIF_Static))
      {
         // Because bumping a sound could screw with m_InfoTable, we look
         // this up every pass through the loop, so be sure we have a fresh
         // pointer.
         if (m_InfoTable.Lookup(hearingObj, &pSoundInfoList))
         {
            pActiveSound = m_ActiveSound[i];

            AssertMsg(AccumList.GetFirst() == NULL, "Sound accumulator not empty on resolve start?");
            AssertMsg(!(pActiveSound->flags & kSIF_Deferred), "Deferred sound on active list!");

            // If our sound is ready to be updated
            if (!(pActiveSound->flags & kSIF_HasBeenUpdated))
            {
               pActiveSound->flags |= kSIF_HasBeenUpdated;

               // Find the objects in the list with the same handle
               pSoundInfo = pSoundInfoList->GetFirst();
               while (pSoundInfo != NULL)
               {
                  pSoundInfoNext = pSoundInfo->GetNext();

                  // Ignore any deferred sounds
                  if (!(pSoundInfo->flags & kSIF_Deferred))
                  {
                     if (pActiveSound->Handle == pSoundInfo->Handle)
                     {
                        pSoundInfoList->Remove(pSoundInfo);
                        AccumList.Append(pSoundInfo);
                     }
                  }

                  pSoundInfo = pSoundInfoNext;
               }

               if (AccumList.GetFirst() != NULL)
               {
                  // Merge all sounds from that object into one
                  MergeSounds(&AccumList);
                  pSoundInfo = AccumList.GetFirst();

                  // Update the virtual sound's position
                  ObjTranslate(pActiveSound->ActiveObject, &pSoundInfo->Position);

                  if ( SFX_Use_Occlusion(pActiveSound->SFXHandle) ) {
                     // EAX Occlusion is available to model attenuation due to closed doors, so
                     //   pass the real distance and the blocking factor
                     SFX_Slam_Dist_BF(pActiveSound->SFXHandle, (int)(pSoundInfo->RealDistance + 0.5),
                                      pSoundInfo->BlockingFactor, FALSE);
                  } else {
                     // pass the munged distance which combines real distance and blocking factor
                     SFX_Slam_Dist(pActiveSound->SFXHandle, (int)(pSoundInfo->Distance + 0.5), FALSE);
                  }

                  pActiveSound->Distance = pSoundInfo->Distance;
                  // Note: SFX_Attenuate does NOT reuqire an SFX channel.
                  pActiveSound->Volume = SFX_Attenuate(pActiveSound->AppsfxParms.gain, pActiveSound->Distance);

                  #ifndef SHIP
                  if (m_SoundFullSpew)
                     mprintf("   Handle %d (%s, on %s) is slammed to %d dist\n",
                             pActiveSound->Handle, pActiveSound->SampleName, ObjWarnName(pActiveSound->SrcObject),
                             (int)(pSoundInfo->Distance + 0.5));
                  #endif

                  // And remove it from the list
                  delete AccumList.Remove(pSoundInfo);
               }
               else
               {
                  // The sound doesn't appear on on our sound list, so it must have
                  // gone out of range.

                  #ifndef SHIP
                  if (m_SoundSpew)
                     mprintf("\"%s\" (handle %d) out of range, either bumping or killing\n",
                             pActiveSound->SampleName, pActiveSound->Handle);
                  #endif

                  // Find our sound list
                  cPlayerSoundList *pSoundList;
                  BOOL found = FALSE;

                  for (int j=0; j<kMaxActiveSounds; j++)
                  {
                     if ((pSoundList = g_pPropSnd->GetSound(j)) != NULL)
                     {
                        if (pSoundList->GetFirst() == pActiveSound)
                        {
                           found = TRUE;
                           break;
                        }
                     }
                  }

                  if (found)
                  {
                     // Either kill the sound or move it onto the deferred list
                     if (pActiveSound->AppsfxParms.flag & SFXFLG_LOOP)
                        BumpActiveSound(pSoundList);
                     else
                        SFX_Kill_Hnd(pActiveSound->SFXHandle);
                  }
                  else
                  {
                     Warning(("Missing active sound \"%s\" (handle %d) is not on the sound list",
                              pActiveSound->SampleName, pActiveSound->Handle));
                  }
               }
            }
         }
      }
   }
}

////////////////////////////////////////////////////////////////////////////////

void cPropSndInst::ResolveDeferredSounds(ObjID hearingObj)
{
   cPlayerSoundList *pPlayerSoundList;
   cSoundInfoList *pSoundInfoList;
   cSoundInfoList AccumList;
   cSoundInfo *pDeferredSound;
   cSoundInfo *pSoundInfo;
   cSoundInfo *pSoundInfoNext;
   int i;

   for (i=0; i<kMaxActiveSounds; i++)
   {
      AssertMsg(AccumList.GetFirst() == NULL, "Sound accumulator not empty on resolve start?");

      // Grab any sound in this slot
      if ((pPlayerSoundList = g_pPropSnd->GetSound(i)) != NULL)
      {
         pDeferredSound = pPlayerSoundList->GetFirst();

         // Only deal with deferred sounds here
         if (pDeferredSound->flags & kSIF_Deferred)
         {
            // Only deal with sounds that have been repropagated, but not updated
            if (!(pDeferredSound->flags & kSIF_HasBeenUpdated))
            {
               pDeferredSound->flags |= kSIF_HasBeenUpdated;

               // Because bumping a sound could screw with m_InfoTable, we look
               // this up every pass through the loop, so be sure we have a fresh
               // pointer.
               if (m_InfoTable.Lookup(hearingObj, &pSoundInfoList))
               {
                  // Find the objects in the list with the same handle
                  pSoundInfo = pSoundInfoList->GetFirst();
                  while (pSoundInfo != NULL)
                  {
                     pSoundInfoNext = pSoundInfo->GetNext();

                     if (pDeferredSound->Handle == pSoundInfo->Handle)
                     {
                        pSoundInfoList->Remove(pSoundInfo);
                        AccumList.Append(pSoundInfo);
                     }

                     pSoundInfo = pSoundInfoNext;
                  }

                  if (AccumList.GetFirst() != NULL)
                  {
                     // Merge all sounds from that object into one
                     MergeSounds(&AccumList);
                     pSoundInfo = AccumList.Remove(AccumList.GetFirst());

                     pSoundInfo->flags |= kSIF_Deferred;
                     pSoundInfo->SFXHandle = -1;

                     delete pPlayerSoundList->Remove(pPlayerSoundList->GetFirst());
                     pPlayerSoundList->Append(pSoundInfo);

                     // Try to start up the sound again
                     if (g_pPropSnd->Prioritize(pPlayerSoundList) == PRIORITY_RES_OK)
                     {
                        #ifndef SHIP
                        if (m_SoundSpew)
                           mprintf("Restarting \"%s\" (handle %d)\n",
                                   pSoundInfo->SampleName, pSoundInfo->Handle);
                        #endif

                        if (pSoundInfo->flags & kSIF_Static)
                        {
                           pSoundInfo->SFXHandle = SFX_Play_Raw(SFX_STATIC, &pSoundInfo->AppsfxParms, pSoundInfo->SampleName);

                           int j;
                           for (j=0; j<kMaxActiveSounds && (m_ActiveSound[j] != NULL); j++);

                           if (pSoundInfo->SFXHandle != SFX_NO_HND)
                           {
                              #ifndef SHIP
                              if (m_SoundSpew)
                                 mprintf("  static restart succeeded\n");
                              #endif

                              pSoundInfo->flags &= ~kSIF_Deferred;
                              m_ActiveSound[j] = pSoundInfo;
                              m_nActiveSoundObjects++;
                           }
                        }
                        else
                        {
                           if (!PlaySoundList(pPlayerSoundList))
                           {
                              pSoundInfo->flags |= kSIF_Deferred;
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }
}

////////////////////////////////////////////////////////////////////////////////

#ifndef SHIP
void cPropSndInst::DrawActiveSounds()
{
   int ocol = gr_get_fcolor();
   r3s_point pt;
   cSoundInfo *pSoundInfo;

   for (int i=0; i<kMaxActiveSounds; i++)
   {
      if ((pSoundInfo = m_ActiveSound[i]) == NULL)
         continue;

      r3_start_block();
      r3_transform_block(1, &pt, &ObjPosGet(pSoundInfo->ActiveObject)->loc.vec);
      r3_end_block();

      if (pt.ccodes == 0)
      {
         gr_set_fcolor(241);
         g2_circle(pt.grp.sx, pt.grp.sy, fix_div(4, grd_bm.w));
         gr_set_fcolor(0);
         g2_circle(pt.grp.sx, pt.grp.sy, fix_div(5, grd_bm.w));
         g2_circle(pt.grp.sx, pt.grp.sy, fix_div(3, grd_bm.w));
      }
   }

   gr_set_fcolor(ocol);
}

void cPropSndInst::SpewActiveSounds()
{
   static char buff[256];
   int i;

   mprintf("\nACTIVE SOUND LIST\n");
   for (i=0; i<kMaxActiveSounds; i++)
   {
      if (m_ActiveSound[i] == NULL)
         mprintf(" [%d] - FREE -\n", i);
      else
      {
         sprintf(buff, " [%d] %13s   handle %2d   obj %3d   dist %5g", i,
                 m_ActiveSound[i]->SampleName, m_ActiveSound[i]->Handle,
                 m_ActiveSound[i]->SrcObject, m_ActiveSound[i]->Distance);
         mprintf("%s\n", buff);
      }
   }
}
#endif

////////////////////////////////////////////////////////////////////////////////

DECLARE_TIMER(FSP_Total, Average);
DECLARE_TIMER(FSP_PathResolve, Average);
DECLARE_TIMER(FSP_DistDirCalc, Average);

#define MAX_PORTALS 64
#define MAX_INLINE_DIST_SQ 25.0

void cPropSndInst::BackCastPath(sAnchorPt *anchor_list, cRoomPortal **portal_list, int i, const mxs_vector &L, const mxs_vector &src_pt)
{
   mxs_vector proj_pt;
   mxs_vector dir;
   int j, k;

   // Now we need to cast backward, to see if we lifted the last
   // anchor point off its corner.  This is kind of nasty.

   // Find last anchor, and the one before it
   int last_proj_portal = -1;
   int last_anchor = -1;

   BOOL can_raycast;

   mxs_vector head_pt = L;
   mxs_vector last_anchor_pt;

   for (j=i-1; j>=0; j--)
   {
      if (anchor_list[j].valid)
      {
         if (last_proj_portal < 0)
            last_proj_portal = j;
         else
            if (last_anchor < 0)
            {
               last_anchor = j;
               break;
            }
      }
   }

   // If we found a single portal, use the source object
   if (last_anchor < 0)
      last_anchor_pt = src_pt;
   else
      last_anchor_pt = anchor_list[last_anchor].pos;

   for (j=i-1; j>=0; j--)
   {
      if ((j == last_proj_portal) || (last_proj_portal == -1))
         mx_sub_vec(&dir, &last_anchor_pt, &head_pt);
      else
         mx_sub_vec(&dir, &anchor_list[last_proj_portal].pos, &head_pt);

      can_raycast = portal_list[j]->Raycast(head_pt, dir);

      // If we generate a new anchor point, add to active list,
      // and move head_pt.
      if (!can_raycast)
      {
         portal_list[j]->GetRaycastProj(head_pt, dir, &proj_pt);

         // New anchor point, add to list, and move head_pt
         if (!anchor_list[j].valid)
         {
            anchor_list[j].pos = head_pt = proj_pt;
            anchor_list[j].valid = TRUE;
         }
         else
         // We either moved last_proj_portal's point, or hit it
         {
            Assert_(j == last_proj_portal);

            if (mx_dist2_vec(&anchor_list[j].pos, &proj_pt) < MAX_INLINE_DIST_SQ)
            {
               // We hit it, so we're done backing up
               return;
            }
            else
            {
               // We missed it, so update the position, update head_pt,
               // and keep going.
               anchor_list[j].pos = head_pt = proj_pt;

               // Update last_anchor and last_proj_portal
               last_proj_portal = last_anchor;

               last_anchor = -1;
               for (k=last_proj_portal-1; k>=0; k--)
               {
                  if (anchor_list[k].valid)
                  {
                     last_anchor = k;
                     break;
                  }
               }

               if (last_anchor < 0)
                  last_anchor_pt = src_pt;
               else
                  last_anchor_pt = anchor_list[last_anchor].pos;
            }
         }
      }
      else
      // If we can now raycast through the portal of the adjacent
      // anchor, we've lifted it off.
      if (anchor_list[j].valid)
      {
         Assert_(j == last_proj_portal);

         anchor_list[j].valid = FALSE;

         // If this was the only portal, we're done
         if (last_anchor < 0)
            return;

         // Update last_anchor and last_proj_portal
         last_proj_portal = last_anchor;

         last_anchor = -1;
         for (k=last_proj_portal-1; k>=0; k--)
         {
            if (anchor_list[k].valid)
            {
               last_anchor = k;
               break;
            }
         }

         if (last_anchor < 0)
            last_anchor_pt = src_pt;
         else
            last_anchor_pt = anchor_list[last_anchor].pos;
      }
   }
}

////////////////////////////////////////

#ifndef SHIP
static int hear_count = 0;
static int hear_depth_accum = 0;
static int min_depth = 1000;
static int max_depth = 0;
#endif

void cPropSndInst::FindSoundPath(const mxs_vector &src_pt, const mxs_vector &hear_pt, const cBFRoomInfo *room_table, int end_room,
                                 mxs_real *dist, mxs_vector *position, mxs_real *realDist, mxs_real *blockingFactor)
{
   #ifdef FSP_TIMING
   AUTO_TIMER(FSP_Total);

   TIMER_Start(FSP_PathResolve);
   #endif

   static cRoomPortal *portal_list[MAX_PORTALS];
   static sAnchorPt anchor_list[MAX_PORTALS];

   int portal_list_size = 0;

   mxs_vector L;
   mxs_vector H;
   mxs_vector dir;
   mxs_vector proj_pt;
   int i;

   // Initialization
   memset(anchor_list, 0, sizeof(sAnchorPt) * MAX_PORTALS);

   int cur_room = end_room;
   int prev_portal;

   float blocking = 1.0;
   static int prev_room;
   const cRoom *pTheRoom;
   float fTransmission;

   pTheRoom = g_pRooms->GetRoom(cur_room);

   // Get transmission property for current room
   if (g_pLoudRoomProperty->Get(pTheRoom->GetObjID(), &fTransmission))
      blocking *= fTransmission;

   // Count portals and accumulate blocking
   while (room_table[cur_room].GetPreviousPortal() >= 0)
   {
      prev_room = room_table[cur_room].GetPreviousRoom();

      if (((cur_room != m_roomNoBlock1)  || (prev_room != m_roomNoBlock2)) &&
          ((prev_room != m_roomNoBlock1) || (cur_room != m_roomNoBlock2)))
      {
         pTheRoom = g_pRooms->GetRoom(room_table[cur_room].GetPreviousRoom());

         // Get transmission property for adjacent room
         if (g_pLoudRoomProperty->Get(pTheRoom->GetObjID(), &fTransmission))
            blocking *= fTransmission;

         blocking *= (1.0 - g_pPropSnd->GetBlockingFactor(cur_room, room_table[cur_room].GetPreviousRoom()));
      }

      portal_list_size++;
      cur_room = room_table[cur_room].GetPreviousRoom();
   }

   AssertMsg1(portal_list_size < MAX_PORTALS, "FindSoundPath: too many portals (%d)", portal_list_size);

   #ifndef SHIP
   if (m_SoundHearSpew)
   {
      int room_count = portal_list_size + 1;

      hear_count++;
      hear_depth_accum += room_count;
      if (room_count < min_depth)
         min_depth = room_count;
      if (room_count > max_depth)
         max_depth = room_count;

      mprintf(" avg depth = %g\n", ((float)hear_depth_accum) / ((float)hear_count));
      mprintf(" min depth = %d\n", min_depth);
      mprintf(" max depth = %d\n", max_depth);
      mprintf("\n");
   }
   #endif

   // Fill in portal list, backward, so it's in the right direction for us
   for (i=1; end_room >= 0; end_room = room_table[end_room].GetPreviousRoom())
   {
      prev_portal = room_table[end_room].GetPreviousPortal();

      if (prev_portal >= 0)
      {
         portal_list[portal_list_size - i] = g_pRooms->GetRoom(end_room)->GetPortal(prev_portal);
         i++;
      }
   }

   L = src_pt;

   for (i=0; i<portal_list_size; i++)
   {
      // Cast from our L point through the current portal to the center
      // of the next portal
      if ((i + 1) == portal_list_size)
        H = hear_pt;
      else
        H = portal_list[i+1]->GetCenter();

      mx_sub_vec(&dir, &H, &L);

      if (!portal_list[i]->Raycast(L, dir))
      {
         // Failed raycast, so project onto portal edge
         portal_list[i]->GetRaycastProj(L, dir, &proj_pt);

         // Add to anchor list
         anchor_list[i].pos = proj_pt;
         anchor_list[i].valid = TRUE;

         // Move L to the new anchor point
         L = proj_pt;

         BackCastPath(anchor_list, portal_list, i, L, src_pt);
      }
   }

   BackCastPath(anchor_list, portal_list, i, hear_pt, src_pt);

   #ifdef FSP_TIMING
   TIMER_MarkStop(FSP_PathResolve);
   TIMER_Start(FSP_DistDirCalc);
   #endif

   // Add up segment distances
   int last_anchor = -1;

   #ifndef SHIP
   if (m_SoundPathSpew)
      mprintf("start = %g %g %g\n", src_pt.x, src_pt.y, src_pt.z);
   #endif

   *dist = 0.0;
   for (i=0; i<portal_list_size; i++)
   {
      if (anchor_list[i].valid)
      {
         #ifndef SHIP
         if (m_SoundPathSpew)
            mprintf("anchor[%d] = %g %g %g\n", i, anchor_list[i].pos.x, anchor_list[i].pos.y, anchor_list[i].pos.z);
         #endif

         if (last_anchor < 0)
            *dist += mx_dist_vec(&src_pt, &anchor_list[i].pos);
         else
            *dist += mx_dist_vec(&anchor_list[last_anchor].pos, &anchor_list[i].pos);

         last_anchor = i;
      }
   }

   #ifndef SHIP
   if (m_SoundPathSpew)
   {
      mprintf("end = %g %g %g\n", hear_pt.x, hear_pt.y, hear_pt.z);
      mprintf("\n");
   }
   #endif

   #ifndef SHIP
   #ifdef EDITOR
   if (PlayerObjectExists() && m_DrawSoundPath &&
       (ObjPosGet(PlayerObject())->loc.vec.x == hear_pt.x) &&
       (ObjPosGet(PlayerObject())->loc.vec.y == hear_pt.y))
   {
      ged_line_load_channel = LINE_CH_SOUND;

      mxs_vector from = src_pt;

      for (i=0; i<portal_list_size; i++)
      {
         if (anchor_list[i].valid)
         {
            gedLineAddRGB(&from, &anchor_list[i].pos, LINE_FLG_DIR, 255, 255, 0);
            from = anchor_list[i].pos;
         }
      }

      gedLineAddRGB(&from, &hear_pt, LINE_FLG_DIR, 255, 255, 0);
   }
   #endif
   #endif

   if (last_anchor < 0)
   {
      *dist += mx_dist_vec(&src_pt, &hear_pt);
      *position = src_pt;
   }
   else
   {
      *dist += mx_dist_vec(&anchor_list[last_anchor].pos, &hear_pt);
      *position = anchor_list[last_anchor].pos;
   }

   *realDist = *dist;
   blocking = 1.0 - blocking;
   // blocking of 0 means no blocking, 1 means completely blocked

   if (*dist < m_MaxDistance)
      *dist += (m_MaxDistance - *dist) * blocking;

   *blockingFactor = blocking;

   #ifdef FSP_TIMING
   TIMER_MarkStop(FSP_DistDirCalc);
   #endif
}

////////////////////////////////////////////////////////////////////////////////

DECLARE_TIMER(PSI_EnterCallback, Average);
DECLARE_TIMER(PSI_PortalsCallback, Average);

#define  sq(x)  ((x)*(x))

BOOL cPropSndInstHigh::EnterCallback(const cRoom *room, const cRoomPortal *enterPortal,
                                     const mxs_vector &enterPt, mxs_real dist)
{
   #ifdef PSI_TIMING
   AUTO_TIMER(PSI_EnterCallback);
   #endif

   if (dist > m_MaxDistance)
      return FALSE;

   cSoundInfoList *pSoundInfoList;
   cSoundInfo     *pSoundInfo;
   mxs_vector      objPos;
   const ObjID    *obj_list;
   int             num_objs;

   mxs_real        obj_dist, blocking_factor, real_dist, old_obj_dist;
   mxs_vector      speaker_pos;

   // Get the list of applicable objects
   room->GetObjList(&obj_list, &num_objs, g_SndWatchHandle);

   // For each object in the room
   while (num_objs--)
   {
      if (PlayerObjectExists() && (*obj_list == PlayerObject()))
         PhysGetSubModLocation(*obj_list, PLAYER_HEAD, &objPos);
      else
         objPos = ObjPosGet(*obj_list)->loc.vec;

      // @TODO: make this lo-res for AIs
      FindSoundPath(m_SrcPoint, objPos, m_RoomPropAgent->GetRoomBrushInfo(), room->GetRoomID(), &obj_dist, &speaker_pos, &real_dist, &blocking_factor);

      if (PortalsCallback(NULL, NULL, obj_dist) < 0)
         goto next_high_sound;

      old_obj_dist = obj_dist;
      if (m_ScaleDistance > 0)
      {
         if (m_pParms->flag & SFXFLG_SHARP)
         {
            mxs_real scale_pct = obj_dist / m_MaxDistance;
            obj_dist = pow(scale_pct, 4) * m_ScaleDistance;
         }
         else
            obj_dist = (obj_dist / m_MaxDistance) * m_ScaleDistance;
      }

      obj_dist /= m_attenFactor;

      real_dist *= (obj_dist / old_obj_dist);

      if (PlayerObjectExists() &&
          (*obj_list == PlayerObject()) &&
          (PhysGetObjMediaState(*obj_list) == kMS_Liquid_Submerged))
      {
         if (m_ScaleDistance > 0)
            obj_dist += (m_ScaleDistance - obj_dist) * 0.5;
         else
            obj_dist += (m_MaxDistance - obj_dist) * 0.5;
      }

      // Create the sound structure
      pSoundInfo = new cSoundInfo;

      strcpy(pSoundInfo->SampleName, m_SampleName);
      pSoundInfo->SchemaID = m_SchemaID;
      pSoundInfo->SrcObject = m_SrcObject;
      pSoundInfo->Object = *obj_list;
      pSoundInfo->Handle = m_Handle;
      pSoundInfo->flags = kSIF_HasBeenUpdated | m_Flags;
      pSoundInfo->ActiveObject = OBJ_NULL;
      pSoundInfo->FramesUntilUpdate = 1;
      pSoundInfo->AppsfxParms = *m_pParms;
      pSoundInfo->Data = m_Data;
      pSoundInfo->AttenFactor = m_attenFactor;

      pSoundInfo->SrcPoint = m_SrcPoint;

      pSoundInfo->Distance = obj_dist;
      pSoundInfo->Volume = SFX_Attenuate(m_pParms->gain, pSoundInfo->Distance);
      pSoundInfo->BlockingFactor = blocking_factor;
      pSoundInfo->RealDistance = real_dist;

      // Extrapolate speaker_pos
      if (PlayerObjectExists() && (*obj_list == PlayerObject()))
      {
         mxs_vector snd_dir;

         mx_sub_vec(&snd_dir, &speaker_pos, &objPos);
         if (mx_mag2_vec(&snd_dir) > 0.0001)
            mx_normeq_vec(&snd_dir);
         mx_scale_add_vec(&pSoundInfo->Position, &objPos, &snd_dir, obj_dist);
      }
      else
         pSoundInfo->Position = speaker_pos;

      // Get the sound list for the object
      if (!m_InfoTable.Lookup(*obj_list, &pSoundInfoList))
      {
         // Allocate a new sound list
         if (PlayerObjectExists() && (*obj_list == PlayerObject()))
            pSoundInfoList = new cPlayerSoundList;
         else
            pSoundInfoList = new cSoundInfoList;

         // Mark the object as hearing a sound
         m_InfoList.Append(*obj_list);
      }

      // Add sound structure to list
      pSoundInfoList->Append(pSoundInfo);

      // @TBD: Need to re-hash here always?
      m_InfoTable.Set(*obj_list, pSoundInfoList);

next_high_sound:
      obj_list++;
   }

   return TRUE;
}

////////////////////////////////////////

void cPropSndInstHigh::ExitCallback(const cRoom *room)
{
}

////////////////////////////////////////

mxs_real cPropSndInstHigh::PortalsCallback(const cRoomPortal *enterPortal,
                                           const cRoomPortal *exitPortal, mxs_real dist)
{
   #ifdef PSI_TIMING
   AUTO_TIMER(PSI_PortalsCallback);
   #endif

   short room1;
   short room2;

   // Adjust distance for blocked exit portal
   if (exitPortal != NULL)
   {
      room1 = exitPortal->GetNearRoom()->GetRoomID();
      room2 = exitPortal->GetFarRoom()->GetRoomID();

      if (((room1 != m_roomNoBlock1) || (room2 != m_roomNoBlock2)) &&
          ((room2 != m_roomNoBlock1) || (room1 != m_roomNoBlock2)))
      {
         if (dist > m_MaxDistance)
            return -1.0;

         dist += (m_MaxDistance - dist) *
            g_pPropSnd->GetBlockingFactor(exitPortal->GetNearRoom()->GetRoomID(),
                                          exitPortal->GetFarRoom()->GetRoomID());
      }
   }

   if (dist > m_MaxDistance)
      return -1.0;

   return dist;
}

////////////////////////////////////////////////////////////////////////////////





