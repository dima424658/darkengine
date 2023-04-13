/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/schema.cpp,v 1.89 2000/01/14 10:44:35 MAT Exp $

#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <lgassert.h>
#include <cfgdbg.h>
#include <timer.h>
#include <lgsound.h>

#include <matrix.h>
#include <dlist.h>
#include <appagg.h>

#include <wrtype.h>
#include <objpos.h>

#include <status.h>
#include <command.h>
#include <iobjsys.h>
#include <appsfx.h>
#include <winui.h>
#include <schema.h>
#include <schprop.h>
#include <schyacc.h>
#include <schbase.h>
#include <schsamps.h>
#include <schdb.h>
#include <speech.h>
#include <schmsg.h>
#include <schema_.h>
#include <tag.h>
#include <psndapi.h>
#include <esndglue.h>

#include <texprop.h>
#include <playrobj.h>
#include <camera.h>
#include <rand.h>
#include <objedit.h>

#ifdef PLAYTEST
#include <config.h>
#include <mprintf.h>
#endif

#ifndef SHIP
#include <recapi.h>
#include <dynarray.h>
#endif

// Must be last header
#include <dbmem.h>

#define min(x,y) ((x)<(y)?(x):(y))

#ifdef OLD_WAY
// this will be bad if the schema doesn't exist or have a name...
inline const char* OBJ_NAME(ObjID obj)
{
   static char buf[32]; 

   const char* s = pObjSys->GetName(obj); 
   if (!s)
   {
      sprintf(buf,"#%d",obj); 
      return buf; 
   }
   return s ; 
}
#else
#define OBJ_NAME(obj) ObjWarnName(obj)
#endif

#define SCHEMA_LOOP_NEXT_TIME(time, pLoopParams) \
   ((time)+(pLoopParams)->intervalMin+(int)(SchemaRand()* \
    ((pLoopParams)->intervalMax-(pLoopParams)->intervalMin)))

// This is the condition for special case zero delay schemas to loop
// over a single sample using the sound library rather than the schema
// looping code (if we need to count the iterations we handle it
// through a callback, but the sound library still does the looping)
#define SCHEMA_LOOP_SINGLE_SOUND(pLoopParams) \
   (((pLoopParams) != NULL) && \
    ((pLoopParams)->intervalMin == 0) && \
    ((pLoopParams)->intervalMax == 0))

#define SCHEMA_NUM_LOOPS(pLoopParams) \
      (((pLoopParams) && (pLoopParams->flags & SCHEMA_LOOP_COUNT))? \
      pLoopParams->count : 0)

// Maximum distance at which we start a schema playing, squared
#define SCHEMA_MAX_PLAY_DIST 5000

// are we playing back a recording?
#define RECORDER_PLAYBACK() \
   (pRecorder && ((pRecorder->GetMode() == kRecPlayback) || \
                  (pRecorder->GetMode() == kRecPausedPlayback)))

IObjectSystem* pObjSys = NULL;
ITraitManager* pTraitMan = NULL;

#ifndef SHIP
// Lots of recorder stuff to spoof sound callbacks

// The basic idea behind schema recording is to record the decisions
// to start samples and the callbacks that end them. Of course, the
// actual samples played during a recording may be completely out of
// synch with this. However, we should still send the same messages
// and therefore the game state should stay in synch.
static IRecorder *pRecorder = NULL;
// callback handles and data for spoofing
typedef cDynArray<int> cCallbackHandleArray;
typedef cDynArray<void*> cCallbackDataArray;
static cCallbackHandleArray callbackHandles;
static cCallbackDataArray callbackData;
static cCallbackHandleArray loopCallbackHandles;
static cCallbackDataArray loopCallbackData;
static BOOL spoofingCallbacks = FALSE;

// We have separate arrays for callbacks which occur while we're
// stuffing our regular callbacks into the recording stream (that is,
// for when spoofingCallbacks is TRUE).
static cCallbackHandleArray callbackHandles2;
static cCallbackDataArray callbackData2;
static cCallbackHandleArray loopCallbackHandles2;
static cCallbackDataArray loopCallbackData2;

#endif

// one for each sample playing as part of a schema
typedef struct sSchemaPlaySample
{
   int hSound;  // handle to sound
   struct sSchemaPlaySample *next;
#ifndef SHIP
   int hRealSound; // actual 
#endif
} sSchemaPlaySample;

struct sSchemaPlay;

class SchemaPlayList : public cDList<sSchemaPlay,1> 
{
public:
   ~SchemaPlayList()
   {
   }
};

typedef cDListNode<sSchemaPlay,1> SchemaPlayListNode;

// one for each schema that is currently playing
struct sSchemaPlay : public SchemaPlayListNode
{
   ObjID schemaID;
   int flags;
   ObjID sourceID;
   mxs_vector sourceLoc;
   int volume;
   int count;                   // no of samples
   int loopCount;               // number of samples we've played (cumulative)
   sSchemaPlaySample *pSamples; // currently playing samples
   SchemaCallback callback;     // optional callback on schema play end
   SchemaCallback loop_callback;// optional callback
   void *userData;              // for callbacks
   Label msg;
   long msgData[3];
   int nextTime;                // when to start next sample (if looping)
};

SchemaPlayList playingSchemas;

#define MAX_PLAYING_SAMPLES 32 
static struct sSchemaPlaySample playingSamples[MAX_PLAYING_SAMPLES];
static sSchemaPlaySample *pFreePlayingSamples;

static BOOL killingSamples = FALSE;

sSchemaCallParams g_sDefaultSchemaCallParams = {0,};

// forward declarations
void SchemaLoopStartNext(sSchemaPlay *pSchemaPlay);

#ifdef DBG_ON

// sample list debugging
// turn on with config "SampleList"

void SchemaSampleListSpew(sSchemaPlaySample *pSample)
{
   int count = 0;

   while (pSample != NULL)
   {
      ++count;
      pSample = pSample->next;
   }
}

#define SchemaSampleListsSpew(when) if (config_is_defined("SampleList")) SchemaSampleListsSpewFn(when)
#define SchemaListSpew(when) if (config_is_defined("SchemaList")) SchemaListSpewFn(when)

void SchemaSampleListsSpewFn(char *when)
{
   sSchemaPlay *pSchemaPlay;

   mprintf("%s ", when);
   // spew free list
   mprintf("Free list:");
   SchemaSampleListSpew(pFreePlayingSamples);
   // spew active schema sample lists
   pSchemaPlay = playingSchemas.GetFirst();
   while (pSchemaPlay != NULL)
   {
      mprintf(" %s", OBJ_NAME(pSchemaPlay->schemaID));
      SchemaSampleListSpew(pSchemaPlay->pSamples);
      pSchemaPlay = pSchemaPlay->GetNext();
   }
   mprintf("\n");
}

void SchemaListSpewFn(char *when)
{
   sSchemaPlay *pSchemaPlay = playingSchemas.GetFirst();

   mprintf("%s, schema list:", when);
   while (pSchemaPlay != NULL)
   {
      mprintf(" %s", OBJ_NAME(pSchemaPlay->schemaID));
      pSchemaPlay = pSchemaPlay->GetNext();
   }
   mprintf("\n");
}

#else

#define SchemaSampleListsSpew(when)
#define SchemaListSpew(when)

#endif

ulong SchemaGetTime()
{
   return tm_get_millisec_unrecorded();
}

// return rand 0->1 (exclusive)
double SchemaRand()
{
   return (double)UnrecordedRand()/(double)(RAND_MAX+1);
}

// play samples list setup
void SchemaPlaySamplesInit()
{
   int i;

   for (i=0; i<MAX_PLAYING_SAMPLES-1; i++)
      playingSamples[i].next = &playingSamples[i+1];
   playingSamples[MAX_PLAYING_SAMPLES-1].next = NULL;
   pFreePlayingSamples = &playingSamples[0];
}

// remove a sample from a list
// remember: must decrement list length count
// should probably also add the sample to the free list after calling this
void SchemaPlaySampleRemove(sSchemaPlaySample **ppListHead,
                            sSchemaPlaySample *pRemSample)
{
   sSchemaPlaySample *pSample;

   Assert_(ppListHead != NULL);
   if (*ppListHead == pRemSample)
      *ppListHead = pRemSample->next;
   else
   {
      pSample = *ppListHead;
      while ((pSample != NULL) && (pSample->next != pRemSample))
         pSample = pSample->next;
      Assert_(pSample != NULL);
      pSample->next = pRemSample->next;
   }
   pRemSample->next = NULL;
}

// return a sample structure to the free list
void SchemaPlaySampleFree(sSchemaPlaySample *pPlaySample)
{
   Assert_(pPlaySample != NULL);
   pPlaySample->next = pFreePlayingSamples;
   pFreePlayingSamples = pPlaySample;
}

// remove and free play sample structure
void SchemaPlaySampleRemoveAndFree(sSchemaPlay *pSchemaPlay,
                                   sSchemaPlaySample *pRemSample)
{
   Assert_(pRemSample != NULL);
   SchemaPlaySampleRemove(&(pSchemaPlay->pSamples), pRemSample);
   SchemaPlaySampleFree(pRemSample);
   --pSchemaPlay->count;
}

// get a new sample and add to list
// returns TRUE on success, FALSE if out of sample structures
// remember: must increment list count
sSchemaPlaySample *SchemaPlaySampleAdd(sSchemaPlaySample **ppListHead)
{
   sSchemaPlaySample *pSample;
   sSchemaPlaySample *pNewSample;

   Assert_(ppListHead != NULL);
   if (pFreePlayingSamples == NULL)
   {
      Warning(("SchemaPlaySampleAdd: out of play samples\n"));
      return FALSE;
   }
   pNewSample = pFreePlayingSamples;
   SchemaPlaySampleRemove(&pFreePlayingSamples, pNewSample);
   if (*ppListHead == NULL)
      *ppListHead = pNewSample;
   else
   {
      pSample = *ppListHead;
      while (pSample->next != NULL)
         pSample = pSample->next;
      pSample->next = pNewSample;
   }
   return pNewSample;
}

// find and return a sample structure corresponding to given sound handle
sSchemaPlaySample *SchemaPlaySampleFind(sSchemaPlaySample *ppListHead,
                                        int hSound)
{
   sSchemaPlaySample *pSample;

   pSample = ppListHead;
   while (pSample != NULL)
   {
      if (pSample->hSound == hSound)
         return pSample;
      pSample = pSample->next;
   }
   return NULL;
}

// Get schema object from its label
ObjID SchemaObjFromName(const Label *schemaName)
{
   if (pObjSys != NULL)
      return pObjSys->GetObjectNamed(schemaName->text);
   else
      return OBJ_NULL;
}

// get the handle of a free play structure, -1 if none
// initialize structure values to defaults
sSchemaPlay *SchemaPlayGet()
{
   sSchemaPlay *pSchemaPlay;

   if ((pSchemaPlay = new sSchemaPlay) != NULL)
   {
      pSchemaPlay->flags = 0;
      pSchemaPlay->schemaID = OBJ_NULL;
      pSchemaPlay->count = 0;
      pSchemaPlay->loopCount = 0;
      pSchemaPlay->pSamples = NULL;
      pSchemaPlay->nextTime = INT_MAX;
      playingSchemas.Append(pSchemaPlay);
   }
   else
      Warning(("SchemaPlayGet: can't get schema play structure\n"));
   return pSchemaPlay;
}

// Add a play structure for a given object
sSchemaPlay *SchemaPlayAdd(ObjID schemaID, const sSchemaCallParams *pCallParams)
{
   sSchemaPlay *pSchemaPlay;
   int i;

   if (killingSamples)
      return NULL;

   if ((pSchemaPlay = SchemaPlayGet()) == NULL)
   {
      Warning(("SchemaPlayAdd: max playing schemas reached\n"));
      return NULL;
   }
   pSchemaPlay->schemaID = schemaID;
   if (pCallParams == NULL)
      return pSchemaPlay;
   pSchemaPlay->flags = pCallParams->flags;
   pSchemaPlay->sourceID = pCallParams->sourceID;
   if (((pCallParams->flags)&SCH_SET_LOC) && (pCallParams->pSourceLoc != NULL))
      pSchemaPlay->sourceLoc = *(pCallParams->pSourceLoc);
   pSchemaPlay->volume = pCallParams->volume;
   pSchemaPlay->callback = pCallParams->callback;
   pSchemaPlay->loop_callback = pCallParams->loop_callback;
   pSchemaPlay->userData = pCallParams->pData;
   for (i=0; i<3; i++)
      pSchemaPlay->msgData[i] = pCallParams->msgData[i];

   SchemaListSpew("SchemaPlayAdd");
   return pSchemaPlay;
}

// setup next sample start time if either
// if we're starting and we're polyphonic
// if we're ending and we're monophonic

#define SAMPLE_START 0
#define SAMPLE_END   1
void SchemaLoopSetupNextTime(sSchemaPlay *pSchemaPlay, 
                             sSchemaLoopParams *pLoopParams,
                             long currentTime, int start_end)
{
   if (((start_end == SAMPLE_START) && (pLoopParams->flags & SCHEMA_LOOP_POLY))
    || ((start_end == SAMPLE_END) && !(pLoopParams->flags & SCHEMA_LOOP_POLY)))
   {
      // have we started all the samples we need to?
      if ((pLoopParams->flags) & SCHEMA_LOOP_COUNT)
         if (pLoopParams->count <= pSchemaPlay->loopCount)
            return;

      if (SCHEMA_LOOP_SINGLE_SOUND(pLoopParams))
         return;

      pSchemaPlay->nextTime = SCHEMA_LOOP_NEXT_TIME(currentTime, pLoopParams);
      ConfigSpew("SchemaTiming", ("Scheduled next sample for %s for %d\n",
                                  OBJ_NAME(pSchemaPlay->schemaID),
                                  pSchemaPlay->nextTime));
   }
}

// end schema play: make callback and free structure
void SchemaPlayEnd(sSchemaPlay *pSchemaPlay)
{
   // make callback - note this may happen before samples actually stop...
   if ((pSchemaPlay->flags & SCH_SET_CALLBACK)
    && (pSchemaPlay->callback != NULL))
   {
      ConfigSpew("SchemaCallback", 
                 ("SchemaPlayEnd callback: handle %d, schema %s, data %d\n",
                  (int)pSchemaPlay,
                  OBJ_NAME(pSchemaPlay->schemaID),
                  (int)(pSchemaPlay->userData)));
      pSchemaPlay->callback((int)pSchemaPlay, pSchemaPlay->schemaID,
                            pSchemaPlay->userData);
   }

   playingSchemas.Remove(pSchemaPlay);
   delete pSchemaPlay;
}

// stop a schema
// halt all samples playing as part of it
void SchemaPlayHalt(int hSchemaPlay)
{
   sSchemaPlay *pSchemaPlay = (sSchemaPlay*)hSchemaPlay;
   sSchemaPlaySample *pSample;

   if (pSchemaPlay == NULL)
   {
      Warning(("SchemaPlayHalt: called with NULL handle\n"));
      return;
   }

#ifdef DBG_ON
   {
      // find handle in playing list
      sSchemaPlay *pSearch = playingSchemas.GetFirst();

      while ((pSearch != NULL) && (pSearch != pSchemaPlay))
         pSearch = pSearch->GetNext();
      if (pSearch != pSchemaPlay)
      {
         Warning(("SchemaPlayHalt: called with invalid handle (%d)\n", (int)pSchemaPlay));
         return;
      }
   }
#endif

   // stop samples and free play sample structs
   if ((pSample = pSchemaPlay->pSamples) != NULL)
   {
      killingSamples = TRUE;
#ifndef SHIP
      if (RECORDER_PLAYBACK())
      {
         // the actual sound handles in the recording may be different
         // from those recorded in fact, some may already be dead, so
         // we may get warnings from appsfx
         if (pSample->hRealSound != -1)
            SoundHalt(pSample->hRealSound);
      }
      else
#endif
      {
         SoundHalt(pSample->hSound);
      }
         
      while (pSample->next != NULL)
      {
         pSample = pSample->next;
         SoundHalt(pSample->hSound);
      }
      pSample->next = pFreePlayingSamples;
      pFreePlayingSamples = pSchemaPlay->pSamples;
      pSchemaPlay->pSamples = NULL;
      killingSamples = FALSE;
   }
   SchemaPlayEnd(pSchemaPlay);
   SchemaSampleListsSpew("Schema halt");
   SchemaListSpew("SchemaPlayHalt");
}

// deal with the end of a sample
// can end the schema
void SchemaSampleEnd(sSchemaPlay *pSchemaPlay, sSchemaPlaySample *pSample)
{
   sSchemaLoopParams *pLoopParams;

   // check to see if we've finished loop count
   if ((pLoopParams = SchemaLoopParamsGet(pSchemaPlay->schemaID)) != NULL)
      if (((pLoopParams->flags) & SCHEMA_LOOP_COUNT)
       && (pSchemaPlay->count == 0)
       && (pLoopParams->count == pSchemaPlay->loopCount))
         SchemaPlayEnd(pSchemaPlay);

   // if we're actively killing samples, then don't bother setting up
   // new ones--also, the sample will be removed in the halting code...
   if (!killingSamples)
   {
      SchemaPlaySampleRemoveAndFree(pSchemaPlay, pSample);

      // setup next sample or end the schema
      if (pLoopParams != NULL)
         SchemaLoopSetupNextTime(pSchemaPlay, pLoopParams, SchemaGetTime(), 
                                 SAMPLE_END);
      else
         // non looping schema ends
         SchemaPlayEnd(pSchemaPlay);
   }

   SchemaSampleListsSpew("Sample end");
   SchemaListSpew("SchemaSampleEnd");
}


// This actually makes the call out of the schema system when a sound
// ends, or when we play back a recording and spoof the callback from
// the frame update function.
static void SchemaSampleEndCallOut(int hSound, void *data)
{
   sSchemaPlaySample *pSample;
   sSchemaPlay *pSchemaPlay;

   ConfigSpew("SchemaTiming", ("Sample %d (schema %s) ended\n",
              hSound, OBJ_NAME((ObjID)data)));

   // this is kind of a hack: if we're killing samples, then don't
   // bother dealing with the death of one...
   //   if (killingSamples)
   //      return;

   pSchemaPlay = playingSchemas.GetFirst();
   while (pSchemaPlay != NULL)
   {
      if (pSchemaPlay->schemaID == (ObjID)data)
      {
         // try to find play sample structure corresponding to sound handle
         pSample = SchemaPlaySampleFind(pSchemaPlay->pSamples, hSound);
         if (pSample != NULL)
         {
            SchemaSampleEnd(pSchemaPlay, pSample);
            return;
         }
      }
      pSchemaPlay = pSchemaPlay->GetNext();
   }
   Warning(("SchemaSampleEndCallOut: no record of sample %d (schema %s)\n",
            hSound, OBJ_NAME((ObjID)data)));
}


// the sound referred to by this handle has ended
// if we are monophonic, then get ready to start a new sample after
// appropriate delay
static void SchemaSampleEndCallback(int hSound, void *data)
{
#ifndef SHIP
   if (pRecorder) {
      if (!RECORDER_PLAYBACK())
      {
         // if recording, save the callback data
         if (spoofingCallbacks) {
            callbackHandles2.Append(hSound);
            callbackData2.Append(data);
         } else {
            callbackHandles.Append(hSound);
            callbackData.Append(data);
         }
      }
      return;
   }
#endif

   SchemaSampleEndCallOut(hSound, data);
}


static void SchemaSampleLoopCallOut(int hSound, void *data)
{
   sSchemaPlaySample *pSample;
   sSchemaPlay *pSchemaPlay;

   ConfigSpew("SchemaTiming", ("Sample %d (schema %s) looped\n",
                               hSound, OBJ_NAME((ObjID)data)));

   pSchemaPlay = playingSchemas.GetFirst();
   ObjID Schema = (ObjID) data;
   while (pSchemaPlay != NULL)
   {
      if (pSchemaPlay->schemaID == Schema)
      {
         // try to find play sample structure corresponding to sound handle
         pSample = SchemaPlaySampleFind(pSchemaPlay->pSamples, hSound);
         if (pSample != NULL
          && (pSchemaPlay->flags & SCH_SET_LOOP_CALLBACK)
          && (pSchemaPlay->loop_callback != NULL)) {
            pSchemaPlay->loop_callback((int)pSchemaPlay, 
                                       pSchemaPlay->schemaID,
                                       pSchemaPlay->userData);
         }
         return;
      }
      pSchemaPlay = pSchemaPlay->GetNext();
   }
   ConfigSpew("SchemaCallback",
              ("SchemaSampleLoopCallOut: no record of sample %d (schema %s)\n",
              hSound, OBJ_NAME(Schema)));
}


// This is for looping which is performed by the sound library.
// So all this does is make a callback out of the schema system.
static void SchemaSampleLoopCallback(int hSound, void *data)
{
#ifndef SHIP
   if (pRecorder)
   {
      if (!RECORDER_PLAYBACK())
      {
         if (spoofingCallbacks) {
            loopCallbackHandles2.Append(hSound);
            loopCallbackData2.Append(data);
         } else {
            loopCallbackHandles.Append(hSound);
            loopCallbackData.Append(data);
         }
      }
      return;
   }
#endif

   SchemaSampleLoopCallOut(hSound, data);
}


// choose a sample from a schema avoiding samples currently playing (if any)
// can force a sample choice
// return sample number within schema, SCH_RANDOM_SAMPLE if can't choose
int SchemaChooseSample(ObjID schemaID, int sampleNum)
{
   int samplesNum;
   int n;

   samplesNum = SchemaSamplesNum(schemaID);
   if (samplesNum<=0)
   {
#ifdef PLAYTEST
      mprintf("SchemaChooseSample: schema %s has no samples\n",OBJ_NAME(schemaID));
#endif
      return SCH_RANDOM_SAMPLE;
   }
   if (sampleNum>=0)
      if (sampleNum<samplesNum)
         n = sampleNum;
      else
      {
         Warning(("SchemaChooseSample: bad sample num (%d) for schema %s\n",
                  sampleNum, OBJ_NAME(schemaID)));
         n = 0;
      }
   else // choose randomly - consider freqs here
   {
      int count = 0;
      int choose;
      int freqTotal = 0;
      const uchar *pFreqs;
      sSchemaPlayParams *pParams;
      int lastSample;

      // get the last sample if we want to avoid reps
      if (((pParams = SchemaPlayParamsGet(schemaID)) != NULL) &&
          ((pParams->flags)&SCH_NO_REPEAT))
          lastSample = SchemaLastSampleGet(schemaID);
      // sum freqs - choose randomly in that range and then map back to samples
      pFreqs = SchemaFreqsGet(schemaID);
      for (n=0; n<samplesNum; n++)
      {
         if (n != lastSample)
            freqTotal += (int)(pFreqs[n]);
      }
      choose = (int)floor(SchemaRand()*(double)freqTotal);
      n = 0;
      count = 0;
      do
      {
         if (n != lastSample)
            count += pFreqs[n];
         ++n;
      } while ((n<=samplesNum-1) && (count<=choose));
      --n;
   }
   return n;
}

void SchemaMsgSend(Label *pMsgType, int volume, sSchemaPlay *pSchemaPlay)
{
   sSchemaMsg sMsg;
   int i;
   ObjPos *pPos;
   ObjPos pos;

   sMsg.msgType = *pMsgType;
   sMsg.volume = volume;
   sMsg.sourceID = pSchemaPlay->sourceID;
   if ((pSchemaPlay->flags)&SCH_SET_OBJ)
      pPos = ObjPosGet(pSchemaPlay->sourceID);
   else if ((pSchemaPlay->flags)&SCH_SET_LOC)
   {
      pos.loc.vec = pSchemaPlay->sourceLoc;
      pPos = &pos;
   }
   if (pPos == NULL)
      // use the player object's location
      pPos = ObjPosGet(PlayerObject());
   if (pPos == NULL)
   {
      Warning(("SchemaMsgSend: source object has no location\n"));
      sMsg.sourceLoc.x = 0;
      sMsg.sourceLoc.y = 0;
      sMsg.sourceLoc.z = 0;
   }
   else
      sMsg.sourceLoc = pPos->loc.vec;
   if ((pSchemaPlay->flags)&SCH_SET_MSG_DATA)
      for (i=0; i<SCHEMA_MSG_DATA_SIZE; i++)
         sMsg.data[i] = pSchemaPlay->msgData[i];
   SchemaMsgPropagate(&sMsg);
}

void SchemaParamsSetup(ObjID schemaID, sfx_parm *parm)
{
   sSchemaPlayParams *pParams;
   int priority;

   parm->flag = 0;
   parm->group = 0;
   parm->user_data = (void*)schemaID;
   parm->radius = 0;
   parm->num_loops = SFX_LOOP_INFINITE;
   parm->end_callback = &SchemaSampleEndCallback;
   parm->loop_callback = &SchemaSampleLoopCallback;

   if ((pParams = SchemaPlayParamsGet(schemaID)) != NULL)
   {
      // setup parameters
      if ((pParams->flags)&SCH_PAN_POS)
         parm->pan = pParams->pan;
      else if ((pParams->flags)&SCH_PAN_RANGE)
         parm->pan = (int)(((SchemaRand()*2.)-1.)*(double)pParams->pan);
      else
         parm->pan = 0;
      // TODO: make this not be stupid - what volume range will we use?
      parm->gain = pParams->volume;
      parm->delay = pParams->initialDelay;
      parm->fade = pParams->fade;

      // now fill in class flags and such!!!
      parm->group = (pParams->flags&SCH_CLASS_MASK)>>SCH_CLASS_SHIFT;
      if (pParams->flags&SCH_NO_CACHE)
         parm->flag|=SFXFLG_NOCACHE;
      if (pParams->flags&SCH_STREAM)
         parm->flag|=SFXFLG_STREAM;      

#ifdef NEW_NETWORK_ENABLED
      if (pParams->flags & SCH_NET_AMBIENT)
         parm->flag |= SFXFLG_NET_AMB;
      if (pParams->flags & SCH_LOC_SPATIAL)
         parm->flag |= SFXFLG_NO_NET;
#endif
   }
   else
   {
      parm->pan = 0;
      parm->gain = -1;
      parm->delay = 0;
      parm->fade = 0;
      parm->group = 0;
   }

   priority = SchemaPriorityGet(schemaID);
   if ((priority>SCH_PRIORITY_MAX) || (priority < SCH_PRIORITY_MIN))
   {
      Warning(("Schema %s priority out of range (%d)\n", OBJ_NAME(schemaID), priority));
      if (priority<0) priority=0;
      else if (priority>UCHAR_MAX) priority=UCHAR_MAX;
   }
   parm->pri = (uchar)priority;
}

// @HACK: this is to allow sndnet to get at the sample number, which is a
// more concise way of sending the sound than its name. We should think
// about how we actually want to expose this. Do we want to add it as a
// param to PSnd, or should we formalize this peek?
static int g_currSampleNum;
static void SchemaSampleCache(int sampleNum)
{
   g_currSampleNum = sampleNum;
}
EXTERN int SchemaGetCurrSample()
{
   return g_currSampleNum;
}

// Play a sample for a given schema
// Add the sample to the play list if we're a playing schema
// Return the sound handle for the new sample, NULL if can't play it
static int SchemaSamplePlay(ObjID schemaID, int sampleNum,
                            sSchemaPlay *pSchemaPlay, 
                            BOOL sampleLoop, int sampleLoopCount, void *pData)
{
   const char *sampleName;
   int hSound;
   int hRealSound = -1;
   Label *pMsgType;
   sSchemaPlaySample *pNewSample;

   // play the actual sample
   if ((sampleName = SchemaSampleGet(schemaID, sampleNum)) != NULL)
   {
      sfx_parm parm;

      SchemaSampleCache(sampleNum);

      ConfigSpew("SchemaSamplePlay", ("Playing schema %s, sample %d\n", OBJ_NAME(schemaID), sampleNum));
      SchemaParamsSetup(schemaID, &parm);
      if (sampleLoop)
      {
         parm.flag |= SFXFLG_LOOP;
         parm.num_loops = sampleLoopCount;
      }

      // override schema volume
      if (pSchemaPlay->flags&SCH_SET_VOLUME)
         parm.gain = pSchemaPlay->volume;
      else if (pSchemaPlay->flags&SCH_ADD_VOLUME)
         parm.gain += pSchemaPlay->volume;
      else if (pSchemaPlay->flags&SCH_SCALE_VOLUME)
         parm.gain = ((float)parm.gain * (float)pSchemaPlay->volume) / 100.0;
      else if (pSchemaPlay->flags&SCH_RADIUS_VOLUME)
         parm.radius = pSchemaPlay->volume;

      // set the sharp attenuation flag, if requested      
      if (pSchemaPlay->flags & SCH_SHARP_ATTEN)
         parm.flag |= SFXFLG_SHARP;

      // set the networking flags, if appropriate
      if ((pSchemaPlay->flags & SCH_SET_OBJ) ||
          (pSchemaPlay->flags & SCH_SET_LOC))
      {
         // Spatial sound is networked by default
         if (pSchemaPlay->flags & SCH_NO_NETWORK)
            parm.flag |= SFXFLG_NO_NET;
      } else {
         // Ambient sound is non-networked by default
         if (pSchemaPlay->flags & SCH_NETWORK)
            parm.flag |= SFXFLG_NET_AMB;
      }

      float att_fac=SchemaAttFacGet(schemaID);

      int flags = (pSchemaPlay->flags & SCH_FORCE_PLAY) ? kGSF_ForcePlay : kGSF_None;

      // actually play it
      if ((pSchemaPlay->flags&SCH_SET_OBJ)
       && (pSchemaPlay->sourceID != OBJ_NULL))
         hSound = GenerateSoundObj(pSchemaPlay->sourceID, schemaID,
                                   (char *)sampleName, att_fac, &parm, flags, pData);
      else if (pSchemaPlay->flags&SCH_SET_LOC)
         hSound = GenerateSoundVec(&pSchemaPlay->sourceLoc, pSchemaPlay->sourceID, schemaID,
                                   (char *)sampleName, att_fac, &parm, flags, pData);
      else
      {  // Since the sound is not coming from anywhere in particular
         // in this case, we'd have a hard time propagating the data.
#ifndef SHIP
         if (pData)
            Warning(("SchemaSamplePlay: data cannot be used w/no location\n"));
#endif // ~SHIP
         hSound = GenerateSound((char*)sampleName, &parm);
      }

#ifndef SHIP
      if (pRecorder)
      {
         // store the real handle
         if (RECORDER_PLAYBACK())
            hRealSound = hSound;
         // restore/save the recorded handle
         RecStreamAddOrExtract(pRecorder, &hSound, sizeof(int), "sample id");
      }
#endif
      // send out the attached message - note do this whether we can play or not
      if ((pSchemaPlay->flags)&SCH_SET_MSG)
         SchemaMsgSend(&(pSchemaPlay->msg), parm.gain, pSchemaPlay);
      if ((pMsgType = SchemaMessageGet(schemaID)) != NULL)
         SchemaMsgSend(pMsgType, parm.gain, pSchemaPlay);
      // give up if we can't play it
      if (hSound == -1)
      {
         ConfigSpew("SchemaSamplePlay", ("Couldnt Generate for schema %s\n", OBJ_NAME(schemaID)));
         return hSound;
      }
      ConfigSpew("SchemaTiming", ("Started new sample (%d) for schema %s at %d\n", 
                                  hSound, OBJ_NAME(schemaID), SchemaGetTime()));
      if ((pNewSample = SchemaPlaySampleAdd(&(pSchemaPlay->pSamples))) != NULL)
      {
         pNewSample->hSound = hSound;
#ifndef SHIP
         pNewSample->hRealSound = hRealSound;
#endif
         ++pSchemaPlay->count;
         ++pSchemaPlay->loopCount;
      }
      SCHEMA_LAST_SAMPLE_SET(schemaID, sampleNum);
      SchemaSampleListsSpew("Sample start");
   }
   else
   {
      Warning(("SchemaSamplePlay: can't get sample %d for schema %s\n",
               sampleNum, OBJ_NAME(schemaID)));
      SchemaSampleListsSpew("Sample start fail");
      return -1;  // ?????????? or _NULL
   }
   return hSound;
}

// recheck loops every second
#define SCHEMA_RECHECK_DELAY 1000

int SchemaSamplePlayAndSetupNext(ObjID schemaID, int sampleNum, 
                                 sSchemaPlay *pSchemaPlay, 
                                 sSchemaLoopParams *pLoopParams, void *pData)
{
   int hSound;

   // play the damn sample
   hSound = SchemaSamplePlay(schemaID, sampleNum, pSchemaPlay, 
                             SCHEMA_LOOP_SINGLE_SOUND(pLoopParams),
                             SCHEMA_NUM_LOOPS(pLoopParams), pData);
   if (hSound == -1)
   {  // we didn't start a sample for some reason
      if (pLoopParams != NULL)     // If we're playing a loop, schedule a recheck later
         pSchemaPlay->nextTime = SchemaGetTime() + SCHEMA_RECHECK_DELAY;
   }
   else
   {  // setup the start of the next sample
      if (pLoopParams != NULL)
         SchemaLoopSetupNextTime(pSchemaPlay, pLoopParams, SchemaGetTime(), 
                                 SAMPLE_START);
   }
   return hSound;
}

// This function is never called.  Commenting it out.  Note that
// we've changed a couple of things with sound so that in some camera modes
// it will use player box location instead of location of camera.  (REMOTE_CAM
// and VIEW_CAM)  (see sndloop.cpp).  AMSD 12/14/99
//inline BOOL SchemaTooFar(mxs_vector *pSource)
//{
//   return (mx_dist2_vec(pSource, &(PlayerCamera()->pos))>SCHEMA_MAX_PLAY_DIST);
//}

// play the selected sample from the schema
// choose sample if sampleNum==SCH_RANDOM_SAMPLE
// return play handle
int SchemaIDPlay(ObjID schemaID, sSchemaCallParams *pCallParams, void *pData)
{
   sSchemaLoopParams *pLoopParams;
   sSchemaPlay *pSchemaPlay = NULL;
   int sampleNum;

   // setup play stuff
   if ((pSchemaPlay = SchemaPlayAdd(schemaID, pCallParams)) == NULL)
      return SCH_HANDLE_NULL;

   // choose the sample
   if ((pCallParams == NULL) || (!(pCallParams->flags & SCH_SET_SAMPLE)))
      sampleNum = SchemaChooseSample(schemaID, SCH_RANDOM_SAMPLE);
   else
      sampleNum = SchemaChooseSample(schemaID, pCallParams->sampleNum);
   if (sampleNum == SCH_RANDOM_SAMPLE)
   {  // we need to clean up better, since we clearly have created pSchemaPlay
      playingSchemas.Remove(pSchemaPlay);
      delete pSchemaPlay;
#ifdef PLAYTEST
      mprintf("Couldn't find a sample to play for schema %s\n", ObjWarnName(schemaID));
#endif
      return SCH_HANDLE_NULL;
   }

   pLoopParams = SchemaLoopParamsGet(pSchemaPlay->schemaID);
   
   // play the sample and schedule start of next
   if (SchemaSamplePlayAndSetupNext(schemaID, sampleNum, pSchemaPlay,
                                    pLoopParams, pData) == -1)
   {
      ConfigSpew("SchemaPlay", ("Failed PlaySetupNext %s (%slooping)\n", OBJ_NAME(schemaID),
                                pLoopParams==NULL?"non-":""));
      if (pLoopParams == NULL)
      {  // give up if we can't play the sample
         playingSchemas.Remove(pSchemaPlay);
         delete pSchemaPlay;
         pSchemaPlay = NULL;
      }
   }

   ConfigSpew("SchemaPlay", ("Starting schema %s (%x)\n", OBJ_NAME(schemaID),pSchemaPlay));
   SchemaSampleListsSpew("SchemaIDPlay");
   SchemaListSpew("SchemaIDPlay");

   return (int)pSchemaPlay;
}

int SchemaIDPlayObj(ObjID schemaID, ObjID objID, void *pData)
{
   sSchemaCallParams callParams = g_sDefaultSchemaCallParams;

   callParams.flags |= SCH_SET_OBJ;
   callParams.sourceID = objID;
   return SchemaIDPlay(schemaID, &callParams, pData);
}

int SchemaIDPlayLoc(ObjID schemaID, mxs_vector *pLoc, void *pData)
{
   sSchemaCallParams callParams = g_sDefaultSchemaCallParams;

   callParams.flags &= ~SCH_SET_OBJ;
   callParams.flags |= SCH_SET_LOC;
   callParams.pSourceLoc = pLoc;
   return SchemaIDPlay(schemaID, &callParams, pData);
}

int SchemaPlay(const Label *schemaName, sSchemaCallParams *pCallParams,
               void *pData)
{
   ObjID schemaID = SchemaObjFromName(schemaName);

   if (schemaID == OBJ_NULL)
   {
      Warning(("SchemaPlay: no schema named %s\n", &schemaName->text[0]));
      return SCH_HANDLE_NULL;
   }
   return SchemaIDPlay(schemaID, pCallParams, pData);
}

int SchemaPlayObj(const Label *schemaName, ObjID objID, void *pData)
{
   sSchemaCallParams callParams = g_sDefaultSchemaCallParams;

   callParams.flags |= SCH_SET_OBJ;
   callParams.sourceID = objID;
   return SchemaPlay(schemaName, &callParams, pData);
}

int SchemaPlayLoc(const Label *schemaName, mxs_vector *pLoc, void *pData)
{
   sSchemaCallParams callParams = g_sDefaultSchemaCallParams;

   callParams.flags &= ~SCH_SET_OBJ;
   callParams.flags |= SCH_SET_LOC;
   callParams.pSourceLoc = pLoc;
   return SchemaPlay(schemaName, &callParams, pData);
}

#ifdef EDITOR
#define HAVE_SCHEMA_COMMANDS
#endif

#ifdef HAVE_SCHEMA_COMMANDS
int SchemaPlay0(const Label *schemaName)
{
   return SchemaPlay(schemaName, NULL, NULL);
}

void SchemasPlay(char *pszSchemas)
{
   int i = 0;
   char szSchema1[16]; // label length
   char *pSchema2;

   CommandParseStringArgs(pszSchemas, szSchema1, 16, &pSchema2);
   SchemaPlay((Label*)szSchema1, NULL, NULL);
   if (pSchema2)
      SchemaPlay((Label*)pSchema2, NULL, NULL);
}

void SchemaHaltNamed(const Label *schemaName)
{
   sSchemaPlay *pSchemaPlay;
   ObjID schemaID = SchemaObjFromName(schemaName);

   if (schemaID == OBJ_NULL)
   {
      Warning(("SchemaHaltNamed: no schema named %s\n", schemaName));
      return;
   }
   pSchemaPlay = playingSchemas.GetFirst();
   while (pSchemaPlay != NULL)
   {
      if (pSchemaPlay->schemaID == schemaID)
      {
         SchemaPlayHalt((int)pSchemaPlay);
         return;
      }
      pSchemaPlay = pSchemaPlay->GetNext();
   }
   SchemaListSpew("SchemaHaltNamed");
}

void SoundDestroy(void)
{
   SpeechDestroy();
   ESndDestroy();
   SchemasDestroy();
}

#ifdef SCHEMA_LOADING
void SchemaFilesDestroyAndLoad(void)
{
   if (!winui_GetYorN("This will destroy all links to schemas."
                          "  Do it anyway?"))
      return;

   SchemaHaltAll();
   SoundDestroy();
   SchemaFilesLoadAll();
   terrainprop_load();   // for now, until yaccparse knows to deal correctly
}

// This doesn't destroy our schema archetypes, so that links to them can
// stick around.
static void SchemaFilesDestroySpeechAndLoad(void)
{
   SchemaHaltAll();
   SpeechDestroy();
   ESndDestroy();
   SchemaFilesLoadAll();
}

#ifndef SHIP
static void SchemaFilesDump(void)
{
   ESndDump();
}
#endif

#endif

static Command schema_commands[] =
{
   { "play_schema", FUNC_STRING, SchemaPlay0, "Play a named schema", HK_ALL },
   { "play_schemas", FUNC_STRING, SchemasPlay, "Play two schemas", HK_ALL},
   { "destroy_schemas", FUNC_VOID, SchemasDestroy, "Destroy all schemas", HK_ALL },
   { "destroy_speech", FUNC_VOID, SpeechDestroy, "Destroy all speech", HK_ALL },
   { "destroy_sound", FUNC_VOID, SoundDestroy, "Destroy all schemas and speech", HK_ALL },
   { "halt_schema", FUNC_STRING, SchemaHaltNamed, "Halt the first instance of a playing schema", HK_ALL},
   { "halt_schemas", FUNC_VOID, SchemaHaltAll, "Halt all playing schemas", HK_ALL},
#ifdef SCHEMA_LOADING   
   { "zggtvrk_load_schema", FUNC_STRING, SchemaFilesLoad, "Load a schema file", HK_ALL },
   { "zggtvrk_load_schemas", FUNC_VOID, SchemaFilesDestroyAndLoad, "Load all schemas in path (destroy first)", HK_ALL },
   { "reload_schemas", FUNC_VOID, SchemaFilesDestroySpeechAndLoad, "Load all schemas in path (don't destroy)", HK_ALL },
#endif   
#ifndef SHIP
   { "dump_schemas", FUNC_VOID, SchemaFilesDump, "Dump schema tags DB)", HK_ALL },
#endif
};

static void setup_commands()
{
   COMMANDS(schema_commands,HK_ALL);
}
#endif // HAVE_SCHEMA_COMMANDS


void SchemaInit()
{
   if (pObjSys == NULL)
      pObjSys = AppGetObj(IObjectSystem);
   if (pTraitMan == NULL)
      pTraitMan = AppGetObj(ITraitManager);
#ifndef SHIP
   if ( (pRecorder == NULL) && (! config_is_defined ("disable_schema_recording")) )
      pRecorder = AppGetObj(IRecorder);
#endif

#ifdef HAVE_SCHEMA_COMMANDS   
   setup_commands();
#endif
   SchemaPropsInit();
   SchemaPlaySamplesInit();

   SchemaListSpew("SchemaInit");
}

void SchemaShutdown()
{
   SchemaHaltAll();
   SchemaPropsTerm();
   SafeRelease(pObjSys);
   SafeRelease(pTraitMan);
#ifndef SHIP
   SafeRelease(pRecorder);
#endif
}

void SchemaHaltAll()
{
   sSchemaPlay *pSchemaPlay;

   pSchemaPlay = playingSchemas.GetFirst();
   while (pSchemaPlay != NULL)
   {
      SchemaPlayHalt((int)pSchemaPlay);
      pSchemaPlay = playingSchemas.GetFirst();
   }
   SchemaListSpew("SchemaHaltAll");
}

// kill a single sample playing as part of a schema
void SchemaSampleHalt(sSchemaPlay *pSchemaPlay, sSchemaPlaySample *pSample)
{
   killingSamples = TRUE;
   SoundHalt(pSample->hSound);
   SchemaPlaySampleRemoveAndFree(pSchemaPlay, pSample);
   killingSamples = FALSE;
}

// start the next scheduled schema loop
void SchemaLoopStartNext(sSchemaPlay *pSchemaPlay)
{
   sSchemaLoopParams *pSchemaLoopParams
      = SchemaLoopParamsGet(pSchemaPlay->schemaID);

   pSchemaPlay->nextTime = INT_MAX;
   if (pSchemaLoopParams)
   {
      int n;

      // check to see that we don't exceed max number of samples to play
      if (pSchemaPlay->count>pSchemaLoopParams->maxSamples)
         SchemaSampleHalt(pSchemaPlay, pSchemaPlay->pSamples);

      // choose a new sample and play it
      n = SchemaChooseSample(pSchemaPlay->schemaID, SCH_RANDOM_SAMPLE);
      if (n != SCH_RANDOM_SAMPLE)
         SchemaSamplePlayAndSetupNext(pSchemaPlay->schemaID, n, 
                                      pSchemaPlay, pSchemaLoopParams, NULL);
   }
   else
      Warning(("Non-looping schema %s has new sample scheduled?\n", 
               OBJ_NAME(pSchemaPlay->schemaID)));
}


#ifndef SHIP
static void RecordCallbacks(ulong *pNumCallbacks, char *pszLabel,
                            cCallbackHandleArray *pHandles,
                            cCallbackDataArray *pCallbackData)
{
   Assert_(pHandles->Size() == pCallbackData->Size());

   *pNumCallbacks = pHandles->Size();
   pRecorder->AddToStream((void*) pNumCallbacks, sizeof(ulong), pszLabel);
}


static void ExtractCallbacks(ulong *pNumCallbacks, char *pszLabel,
                             cCallbackHandleArray *pHandles,
                             cCallbackDataArray *pCallbackData)
{
   pRecorder->ExtractFromStream((void*) pNumCallbacks, sizeof(ulong), pszLabel);
   pHandles->SetSize(*pNumCallbacks);
   pCallbackData->SetSize(*pNumCallbacks);
}
#endif // ~SHIP


// per frame function
void SchemaFrame()
{
   sSchemaPlay *pSchemaPlay;
   long currentTime = SchemaGetTime();
   BOOL startNew;

#ifndef SHIP
   ulong i;
   ulong numCallbacks;
   ulong numLoopCallbacks;

   if (pRecorder)
   {
      // record or play back our sample end and loop callbacks
      spoofingCallbacks = TRUE;

      if (RECORDER_PLAYBACK())
      {
         ExtractCallbacks(&numCallbacks, "schema callbacks",
                          &callbackHandles, &callbackData);
         ExtractCallbacks(&numLoopCallbacks, "schema loop CBs",
                          &loopCallbackHandles, &loopCallbackData);
      }
      else
      {
         RecordCallbacks(&numCallbacks, "schema callbacks",
                         &callbackHandles, &callbackData);
         RecordCallbacks(&numLoopCallbacks, "schema loop CBs",
                         &loopCallbackHandles, &loopCallbackData);
      }

      // spoof callbacks
      for (i = 0; i < numCallbacks; ++i)
      {
         RecStreamAddOrExtract(pRecorder, &callbackHandles[i], 
                               sizeof(int), "schema callback handle");
         RecStreamAddOrExtract(pRecorder, &callbackData[i], 
                               sizeof(void*), "schema callback data");
         SchemaSampleEndCallOut(callbackHandles[i], callbackData[i]);
      }

      for (i = 0; i < numLoopCallbacks; ++i)
      {
         RecStreamAddOrExtract(pRecorder, &loopCallbackHandles[i], 
                               sizeof(int), "schema loop CB handle");
         RecStreamAddOrExtract(pRecorder, &loopCallbackData[i], 
                               sizeof(void*), "schema loop CB data");
         SchemaSampleLoopCallOut(loopCallbackHandles[i],
                                 loopCallbackData[i]);
      }

      callbackHandles.SetSize(0);
      callbackData.SetSize(0);
      loopCallbackHandles.SetSize(0);
      loopCallbackData.SetSize(0);

      spoofingCallbacks = FALSE;

      //////////////////////////////////////////////////////////////
      // This is the same deal as just above, but it handles any
      // schemas which ended or looped while we were making the first
      // set of callbacks.

      if (RECORDER_PLAYBACK())
      {
         ExtractCallbacks(&numCallbacks, "schema callbacks",
                          &callbackHandles2, &callbackData2);
         ExtractCallbacks(&numLoopCallbacks, "schema loop CBs",
                          &loopCallbackHandles2, &loopCallbackData2);
      }
      else
      {
         RecordCallbacks(&numCallbacks, "schema callbacks",
                         &callbackHandles2, &callbackData2);
         RecordCallbacks(&numLoopCallbacks, "schema loop CBs",
                         &loopCallbackHandles2, &loopCallbackData2);
      }

      // spoof callbacks
      for (i = 0; i < numCallbacks; ++i)
      {
         RecStreamAddOrExtract(pRecorder, &callbackHandles2[i], 
                               sizeof(int), "schema callback handle");
         RecStreamAddOrExtract(pRecorder, &callbackData2[i], 
                               sizeof(void*), "schema callback data");
         SchemaSampleEndCallOut(callbackHandles2[i], callbackData2[i]);
      }

      for (i = 0; i < numLoopCallbacks; ++i)
      {
         RecStreamAddOrExtract(pRecorder, &loopCallbackHandles2[i], 
                               sizeof(int), "schema loop CB handle");
         RecStreamAddOrExtract(pRecorder, &loopCallbackData2[i], 
                               sizeof(void*), "schema loop CB data");
         SchemaSampleLoopCallOut(loopCallbackHandles2[i], 
                                 loopCallbackData2[i]);
      }

      callbackHandles2.SetSize(0);
      callbackData2.SetSize(0);
      loopCallbackHandles2.SetSize(0);
      loopCallbackData2.SetSize(0);
   }
#endif // ~SHIP

   pSchemaPlay = playingSchemas.GetFirst();
   // look through playing schemas and see if we need to start new samples
   while (pSchemaPlay != NULL)
   {
      // check to see if we're scheduled to start a new sample
      startNew = (pSchemaPlay->nextTime < currentTime);
#ifndef SHIP
      if (pRecorder)
      {
         // record/playback decision to start new sample
         RecStreamAddOrExtract(pRecorder, &startNew, sizeof(BOOL), "new sample");
      }
#endif
      if (startNew)
         SchemaLoopStartNext(pSchemaPlay);
      pSchemaPlay = pSchemaPlay->GetNext();
   }
   SchemaSampleListsSpew("Frame");
}

ObjID SchemaGetIDFromHandle(int hSchemaPlay)
{
   if (hSchemaPlay == SCH_HANDLE_NULL)
      return OBJ_NULL;
   sSchemaPlay *pSchemaPlay = (sSchemaPlay*)hSchemaPlay;
   return pSchemaPlay->schemaID;
}

int SchemaGetSFXFromHandle(int hSchemaPlay, int iWhich)
{
   if (hSchemaPlay == SCH_HANDLE_NULL)
      return SFX_NO_HND;

   sSchemaPlay *pSchemaPlay = (sSchemaPlay*)hSchemaPlay;
   sSchemaPlaySample *pSample = pSchemaPlay->pSamples;
   if (!pSample)
      return SFX_NO_HND;

   // It's hard to imagine a client wanting iWhich > 0, but just in
   // case...
   for (int i = 0; i < iWhich; ++i) {
      pSample = pSample->next;
      if (!pSample)
         return SFX_NO_HND;
   }

   return pSample->hSound;
}
