/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/sndnet.cpp,v 1.10 1999/08/05 18:25:03 Justin Exp $
//
// This file deals with broadcasting sounds that occur in the simulation.
// It is really only intended for use with psnd, with which it is fairly
// closely coupled.
//
// The underlying notion is that, when a sound gets to psnd, that checks
// whether the sound should be networked. If so, then it uses this module
// to broadcast the sound to everyone else. This makes sure that, on the
// client machines, networking is suppressed.
//
// We have one minor hideousness: there are two versions of most of
// these messages, a "normal" version and a "small" version. This is due
// to the fact that sfx_parm is pretty large, but usually just full of
// default values, so we usually try to just send the few interesting
// values. We only send the full structure when something looks
// non-default.
//

#include <lg.h>
#include <netmsg.h>
#include <config.h>
#include <objedit.h>

#include <hashpp.h>
#include <hshpptem.h>
#include <dwordset.h>

#include <sndnet.h>
#include <psndapi.h>

// We re-derive some numbers from the schema system, rather than
// transmitting them:
#include <schbase.h>
#include <schsamps.h>
#include <schprop.h>

// Must be last:
#include <dbmem.h>

//////////////////////////////
//
// UTILITIES and STRUCTURES
//

//////////
//
// Map from remote sound handles to local ones
//
// In this mapping, the remote handle is assumed to be the "host", who
// owns this sound; the local one is a "client", which should halt when 
// the host does.
//

// An "owned sound". We compress this into 32 bits for convenience:
typedef int tOwnedSndHandle;
static inline tOwnedSndHandle OwnHandle(uint owner, uint handle)
{
   return (owner << 16) + handle;
}

// The hash table from host sound handles to local ones:
typedef cHashTable<tOwnedSndHandle, int, cHashFunctions> tSoundHandleHash;
static tSoundHandleHash HandleMap;
// The table to keep track of which sounds are "clients":
static cDWORDSet ClientSet;

// The method that gets called when a local sound finishes:
static void soundNetEndCallback(int localHandle, void *pClientData)
{
   tOwnedSndHandle ownHandle = (tOwnedSndHandle) pClientData;
   // Remove this entry from the map table:
   HandleMap.Delete(ownHandle);
   ClientSet.Delete(localHandle);
}

// Add an entry to the mapping. If there isn't actually a local sound,
// don't bother.
static void soundNetAddMapping(int remoteHandle, int localHandle, ObjID owner)
{
   if (localHandle != kPSndFail) {
      tOwnedSndHandle ownHandle = OwnHandle(owner, remoteHandle);
      HandleMap.Set(ownHandle, localHandle);
      ClientSet.Add(localHandle);
   }
}

//////////
//
// Simplified sfx_parm, for networking
//

// A cut-down version of sfx_parm, with only the stuff we want to network:
typedef struct {
   int    pan;       // using standard sndlib pan (-10k to 10k)
   ushort flag;      // flag overrides, not all are app setable
   uchar  pri;       // priority for the sound
   uchar  group;     // what group the sound effect goes into
   int    gain;      // currently in snd.lib decibel format, maybe not a good idea
   int    delay;     // initial delay
   ulong  fade;      // fade in and out time
   int    radius;    // forced sound radius
   int    distance;  // distance to sound origination point (-1 == straight line)
   int    num_loops; // 
} sfx_net_parm;      //    since we really want local vol overrides? who knows
// 32


// An even smaller version of sfx_parm, for the typical case with
// mostly defaults:
typedef struct {
   uchar  group;
   uchar  pad;
   ushort flag;
   short gain;
   short radius;
} sfx_small_net_parm;
// 8

// Copy parms into the appropriate network version. Returns TRUE iff
// we should use the full net_parm, or FALSE iff we should use the
// small_net_parm.
// @TBD: is it *ever* reasonable for gain, radius, or distance to have
// a non-short value?
BOOL fillNetParms(sfx_parm *parm, 
                  sfx_net_parm *net_parm,
                  sfx_small_net_parm *small_net_parm)
{
   if ((parm->pan != 0) ||
       (parm->pri != 128) || // Why 128? I dunno, but it seems normal
       (parm->gain != ((short) parm->gain)) ||
       (parm->fade != 0) ||
       (parm->radius != ((short) parm->radius)) ||
       (parm->num_loops != 0))
   {
      // Okay, it's not a complete ordinary parm, so use the full struct:
      net_parm->pan = parm->pan;
      net_parm->flag = parm->flag;
      net_parm->pri = parm->pri;
      net_parm->group = parm->group;
      net_parm->gain = parm->gain;
      net_parm->delay = parm->delay;
      net_parm->fade = parm->fade;
      net_parm->radius = parm->radius;
      net_parm->distance = parm->distance;
      net_parm->num_loops = parm->num_loops;

      /*
      mprintf("sfx_net_parm: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
              parm->pan,
              parm->flag,
              parm->pri,
              parm->group,
              parm->gain,
              parm->delay,
              parm->fade,
              parm->radius,
              parm->distance,
              parm->num_loops);
      */

      return TRUE;
   } else {
      // It's boring and normal, so just use the cut-down version:
      small_net_parm->group = parm->group;
      small_net_parm->flag = parm->flag;
      small_net_parm->gain = parm->gain;
      small_net_parm->radius = parm->radius;
      return FALSE;
   }
}

// Copy parms from the network version:
void fillParms(sfx_parm *parm, sfx_net_parm *net_parm)
{
   // Copy the relevant fields:
   parm->pan = net_parm->pan;
   parm->flag = net_parm->flag;
   parm->pri = net_parm->pri;
   parm->group = net_parm->group;
   parm->gain = net_parm->gain;
   parm->delay = net_parm->delay;
   parm->fade = net_parm->fade;
   parm->radius = net_parm->radius;
   parm->distance = net_parm->distance;
   parm->num_loops = net_parm->num_loops;

   // NULL out the fields we don't use on the client side:
   parm->loop_callback = NULL;
   parm->user_data = NULL;
   parm->pSndSource = NULL;

   // And set the local-relevant ones:
   parm->end_callback = soundNetEndCallback;
}

// Fill parms from the small version:
void fillParmsSmall(sfx_parm *parm, sfx_small_net_parm *net_parm)
{
   // Copy the relevant fields:
   parm->pan = 0;
   parm->flag = net_parm->flag;
   parm->pri = 128;
   parm->group = net_parm->group;
   parm->gain = net_parm->gain;
   parm->delay = 0;
   parm->fade = 0;
   parm->radius = net_parm->radius;
   parm->distance = -1;
   parm->num_loops = 0;

   // NULL out the fields we don't use on the client side:
   parm->loop_callback = NULL;
   parm->user_data = NULL;
   parm->pSndSource = NULL;

   // And set the local-relevant ones:
   parm->end_callback = soundNetEndCallback;
}

//////////////////////////////
//
// THE MESSAGES
//
// ASSUMPTIONS:
// -- The attenuation factor is taken directly from the schema, and
//    doesn't need to be transmitted.
// -- The sampleNums within a schema are statically determined, and
//    identical on all machines, so we can transmit the sample number
//    in lieu of the sample's name.
// -- The delay parm is taken directly from the schema, and doesn't
//    need to be transmitted.
// -- The distance parm is garbage at this point, and doesn't need
//    to be transmitted.
//

//////////
//
// A sound attached to a specific object
//

static cNetMsg *g_pSoundObjMsg = NULL;
static cNetMsg *g_pSmallSoundObjMsg = NULL;

static void makeSoundObj(int handle,
                         ObjID objID, 
                         ObjID schemaID, 
                         uchar sampleNum,
                         sfx_parm *parm,
                         ObjID owner)
{
   parm->flag |= SFXFLG_NO_NET;
   parm->user_data = (void *) OwnHandle(owner, handle);
   sSchemaPlayParams *pPlayParams = SchemaPlayParamsGet(schemaID);
   if (pPlayParams == NULL) {
      // Weird, but we're not going to try to network this one...
      Warning(("Networked schema without playParams!\n"));
      return;
   }
   parm->delay = pPlayParams->initialDelay;
   const char *sampleName = SchemaSampleGet(schemaID, sampleNum);
   // Re-derive the attenuation factor, rather than transmitting it,
   // since it's simply based on the schema:
   float atten_factor = SchemaAttFacGet(schemaID);

#ifndef SHIP
   if (config_is_defined("SoundSpew"))
   {
      mprintf("SndNet: playing %s\n", sampleName);
   }
#endif

   // Play the sound, and record it:
   int localHandle = GenerateSoundObj(objID,
                                      schemaID,
                                      sampleName,
                                      atten_factor,
                                      parm, TRUE,
                                      NULL);
   soundNetAddMapping(handle, localHandle, owner);
}

static void handleSoundObj(int handle,
                           ObjID objID, 
                           ObjID schemaID, 
                           uchar sampleNum,
                           sfx_net_parm *net_parms,
                           ObjID owner)
{
   // Set up for the sound:
   sfx_parm parm;
   fillParms(&parm, net_parms);
   makeSoundObj(handle, objID, schemaID, sampleNum, &parm, owner);
}

static void handleSmallSoundObj(int handle,
                                ObjID objID, 
                                ObjID schemaID, 
                                uchar sampleNum,
                                sfx_small_net_parm *net_parms,
                                ObjID owner)
{
   // Set up for the sound:
   sfx_parm parm;
   fillParmsSmall(&parm, net_parms);
   makeSoundObj(handle, objID, schemaID, sampleNum, &parm, owner);
}

static sNetMsgDesc sSoundObjDesc =
{
   kNMF_Broadcast | kNMF_AppendSenderID,
   "SoundObj",
   "Object-attached sound",
   NULL,
   handleSoundObj,
   {{kNMPT_Int, kNMPF_None, "Handle"},
    {kNMPT_GlobalObjID, kNMPF_None, "Object"},
    {kNMPT_ObjID, kNMPF_None, "Schema"},
    {kNMPT_UByte, kNMPF_None, "Sample number"},
    {kNMPT_Block, kNMPF_None, "SFX Parms", sizeof(sfx_net_parm)},
    {kNMPT_End}}
};

static sNetMsgDesc sSmallSoundObjDesc =
{
   kNMF_Broadcast | kNMF_AppendSenderID,
   "SmSoundObj",
   "Simple Object-attached sound",
   NULL,
   handleSmallSoundObj,
   {{kNMPT_Int, kNMPF_None, "Handle"},
    {kNMPT_GlobalObjID, kNMPF_None, "Object"},
    {kNMPT_ObjID, kNMPF_None, "Schema"},
    {kNMPT_UByte, kNMPF_None, "Sample number"},
    {kNMPT_Block, kNMPF_None, "SFX Parms", sizeof(sfx_small_net_parm)},
    {kNMPT_End}}
};

EXTERN int SchemaGetCurrSample();

void SoundNetGenerateSoundObj(int handle,
                              ObjID objID, 
                              ObjID schemaID, 
                              const char *sampleName, 
                              float atten_factor, 
                              sfx_parm *parms)
{
   sfx_net_parm net_parms;
   sfx_small_net_parm small_parms;

   // Actually get the sample index from the schema system
   int sampleNum = SchemaSampleGetNamed(schemaID, sampleName);
   AssertMsg2(sampleNum >= 0, "Bad sample index for schema %s, name %s", 
              ObjWarnName(schemaID), sampleName);

   // Confirm that we don't need to transmit the attenuation factor:
   AssertMsg1(atten_factor == SchemaAttFacGet(schemaID),
              "SoundObj: Attenuation factor doesn't match schema %d!",
              schemaID);
   // Confirm that we don't need to transmit the delay:
   sSchemaPlayParams *pPlayParams = SchemaPlayParamsGet(schemaID);
   if (pPlayParams == NULL) {
      // Weird, but we're not going to try to network this one...
      Warning(("Networked schema without playParams!\n"));
      return;
   }
   AssertMsg1(parms->delay == pPlayParams->initialDelay,
              "SoundObj: Delay doesn't match schema %d!",
              schemaID);

   if (fillNetParms(parms, &net_parms, &small_parms)) {
      g_pSoundObjMsg->Send(OBJ_NULL, handle, objID, schemaID,
                           sampleNum, &net_parms);
   } else {
      g_pSmallSoundObjMsg->Send(OBJ_NULL, handle, objID, schemaID,
                                sampleNum, &small_parms);
   }
}

//////////
//
// A sound with a vector
//

static cNetMsg *g_pSoundVecMsg = NULL;
static cNetMsg *g_pSmallSoundVecMsg = NULL;

static void makeSoundVec(int handle,
                         mxs_vector *vec,
                         ObjID schemaID, 
                         uchar sampleNum,
                         sfx_parm *parm,
                         ObjID owner)
{
   parm->flag |= SFXFLG_NO_NET;
   parm->user_data = (void *) OwnHandle(owner, handle);
   sSchemaPlayParams *pPlayParams = SchemaPlayParamsGet(schemaID);
   if (pPlayParams == NULL) {
      // Weird, but we're not going to try to network this one...
      Warning(("Networked schema without playParams!\n"));
      return;
   }
   parm->delay = pPlayParams->initialDelay;
   const char *sampleName = SchemaSampleGet(schemaID, sampleNum);
   // Re-derive the attenuation factor, rather than transmitting it,
   // since it's simply based on the schema:
   float atten_factor = SchemaAttFacGet(schemaID);

#ifndef SHIP
   if (config_is_defined("SoundSpew"))
   {
      mprintf("SndNet: playing %s\n", sampleName);
   }
#endif

   // Play the sound, and record it:
   int localHandle = GenerateSoundVec(vec,
                                      OBJ_NULL,
                                      schemaID,
                                      sampleName,
                                      atten_factor,
                                      parm, TRUE,
                                      NULL);
   soundNetAddMapping(handle, localHandle, owner);
}

static void handleSoundVec(int handle,
                           mxs_vector *vec,
                           ObjID schemaID, 
                           uchar sampleNum,
                           sfx_net_parm *net_parms,
                           ObjID owner)
{
   // Set up for the sound:
   sfx_parm parm;
   fillParms(&parm, net_parms);
   makeSoundVec(handle, vec, schemaID, sampleNum, &parm, owner);
}

static void handleSmallSoundVec(int handle,
                                mxs_vector *vec,
                                ObjID schemaID, 
                                uchar sampleNum,
                                sfx_small_net_parm *net_parms,
                                ObjID owner)
{
   // Set up for the sound:
   sfx_parm parm;
   fillParmsSmall(&parm, net_parms);
   makeSoundVec(handle, vec, schemaID, sampleNum, &parm, owner);
}

static sNetMsgDesc sSoundVecDesc =
{
   kNMF_Broadcast | kNMF_AppendSenderID,
   "SoundVec",
   "Vector-attached sound",
   NULL,
   handleSoundVec,
   {{kNMPT_Int, kNMPF_None, "Handle"},
    {kNMPT_Vector, kNMPF_None, "Vector"},
    {kNMPT_ObjID, kNMPF_None, "Schema"},
    {kNMPT_UByte, kNMPF_None, "Sample number"},
    {kNMPT_Block, kNMPF_None, "SFX Parms", sizeof(sfx_net_parm)},
    {kNMPT_End}}
};

static sNetMsgDesc sSmallSoundVecDesc =
{
   kNMF_Broadcast | kNMF_AppendSenderID,
   "SmSoundVec",
   "Simple Vector-attached sound",
   NULL,
   handleSmallSoundVec,
   {{kNMPT_Int, kNMPF_None, "Handle"},
    {kNMPT_Vector, kNMPF_None, "Vector"},
    {kNMPT_ObjID, kNMPF_None, "Schema"},
    {kNMPT_UByte, kNMPF_None, "Sample number"},
    {kNMPT_Block, kNMPF_None, "SFX Parms", sizeof(sfx_small_net_parm)},
    {kNMPT_End}}
};

void SoundNetGenerateSoundVec(int handle,
                              mxs_vector *vec,
                              ObjID schemaID, 
                              const char *sampleName, 
                              float atten_factor, 
                              sfx_parm *parms)
{
   sfx_net_parm net_parms;
   sfx_small_net_parm small_parms;

   // Actually get the sample index from the schema system
   int sampleNum = SchemaSampleGetNamed(schemaID, sampleName);
   AssertMsg2(sampleNum >= 0, "Bad sample index for schema %s, name %s", 
              ObjWarnName(schemaID), sampleName);

   // Confirm that we don't need to transmit the attenuation factor:
   AssertMsg1(atten_factor == SchemaAttFacGet(schemaID),
              "SoundVec: Attenuation factor doesn't match schema %d!",
              schemaID);
   // Confirm that we don't need to transmit the delay:
   sSchemaPlayParams *pPlayParams = SchemaPlayParamsGet(schemaID);
   if (pPlayParams == NULL) {
      // Weird, but we're not going to try to network this one...
      Warning(("Networked schema without playParams!\n"));
      return;
   }
   AssertMsg1(parms->delay == pPlayParams->initialDelay,
              "SoundVec: Delay doesn't match schema %d!",
              schemaID);

   if (fillNetParms(parms, &net_parms, &small_parms)) {
      g_pSoundVecMsg->Send(OBJ_NULL, handle, vec, schemaID,
                           sampleNum, &net_parms);
   } else {
      g_pSmallSoundVecMsg->Send(OBJ_NULL, handle, vec, schemaID,
                                sampleNum, &small_parms);
   }
}

//////////
//
// A plain sound (usually not networked, but sometimes)
//

static cNetMsg *g_pSoundMsg = NULL;
static cNetMsg *g_pSmallSoundMsg = NULL;

static void makeSound(int handle,
                      const char *sampleName, 
                      sfx_parm *parm,
                      ObjID owner)
{
   parm->flag ^= SFXFLG_NET_AMB;
   parm->user_data = (void *) OwnHandle(owner, handle);

   // Play the sound, and record it:
   int localHandle = GenerateSound(sampleName, parm);
   soundNetAddMapping(handle, localHandle, owner);
}

static void handleSound(int handle,
                        const char *sampleName, 
                        sfx_net_parm *net_parms,
                        ObjID owner)
{
   // Set up for the sound:
   sfx_parm parm;
   fillParms(&parm, net_parms);
   makeSound(handle, sampleName, &parm, owner);
}

static void handleSmallSound(int handle,
                             const char *sampleName, 
                             sfx_small_net_parm *net_parms,
                             ObjID owner)
{
   // Set up for the sound:
   sfx_parm parm;
   fillParmsSmall(&parm, net_parms);
   makeSound(handle, sampleName, &parm, owner);
}

static sNetMsgDesc sSoundDesc =
{
   kNMF_Broadcast | kNMF_AppendSenderID,
   "Sound",
   "Plain sound",
   NULL,
   handleSound,
   {{kNMPT_Int, kNMPF_None, "Handle"},
    {kNMPT_String, kNMPF_None, "Sample name"},
    {kNMPT_Block, kNMPF_None, "SFX Parms", sizeof(sfx_net_parm)},
    {kNMPT_End}}
};

static sNetMsgDesc sSmallSoundDesc =
{
   kNMF_Broadcast | kNMF_AppendSenderID,
   "SmSound",
   "Simple Plain sound",
   NULL,
   handleSmallSound,
   {{kNMPT_Int, kNMPF_None, "Handle"},
    {kNMPT_String, kNMPF_None, "Sample name"},
    {kNMPT_Block, kNMPF_None, "SFX Parms", sizeof(sfx_small_net_parm)},
    {kNMPT_End}}
};

void SoundNetGenerateSound(int handle,
                           const char *sampleName, 
                           sfx_parm *parms)
{
   sfx_net_parm net_parms;
   sfx_small_net_parm small_parms;
   if (fillNetParms(parms, &net_parms, &small_parms)) {
      g_pSoundMsg->Send(OBJ_NULL, handle, sampleName, &net_parms);
   } else {
      g_pSmallSoundMsg->Send(OBJ_NULL, handle, sampleName, &small_parms);
   }
}

//////////
//
// Stop a sound prematurely
//

static cNetMsg *g_pSoundHaltMsg = NULL;

static void handleSoundHalt(int handle, ObjID owner)
{
   int localHandle;
   tOwnedSndHandle ownHandle = OwnHandle(owner, handle);
   if (HandleMap.Lookup(ownHandle, &localHandle)) {
      SoundHalt(localHandle);
      HandleMap.Delete(ownHandle);
      ClientSet.Delete(localHandle);
   }
}

static sNetMsgDesc sSoundHaltDesc =
{
   kNMF_Broadcast | kNMF_AppendSenderID,
   "HaltSound",
   "Halt Sound",
   NULL,
   handleSoundHalt,
   {{kNMPT_Int, kNMPF_None, "Handle"},
    {kNMPT_End}}
};

void SoundNetHalt(int handle)
{
   // If we have a mapping for this sound, then we don't own it, so we'd
   // better not tell others to shut it down.
   if (!ClientSet.InSet(handle)) {
      g_pSoundHaltMsg->Send(OBJ_NULL, handle);
   }
}


//////////////////////////////
//
// STARTUP and SHUTDOWN
//

// Prepare the sound-related network msgs
void SoundNetInit()
{
   g_pSoundObjMsg = new cNetMsg(&sSoundObjDesc);
   g_pSmallSoundObjMsg = new cNetMsg(&sSmallSoundObjDesc);
   g_pSoundVecMsg = new cNetMsg(&sSoundVecDesc);
   g_pSmallSoundVecMsg = new cNetMsg(&sSmallSoundVecDesc);
   g_pSoundMsg = new cNetMsg(&sSoundDesc);
   g_pSmallSoundMsg = new cNetMsg(&sSmallSoundDesc);
   g_pSoundHaltMsg = new cNetMsg(&sSoundHaltDesc);
}

// Delete the messages
void SoundNetTerm()
{
   delete g_pSoundObjMsg;
   delete g_pSmallSoundObjMsg;
   delete g_pSoundVecMsg;
   delete g_pSmallSoundVecMsg;
   delete g_pSoundMsg;
   delete g_pSmallSoundMsg;
   delete g_pSoundHaltMsg;
}
