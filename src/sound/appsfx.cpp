/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/appsfx.cpp,v 1.127 2000/03/24 22:57:27 adurant Exp $
// remedial sound system for DromEd/Dark/AIR/So on

#include <dynarray.h>
#include <math.h>
#include <arqapi.h>

#include <timer.h>

#include <appagg.h>

#include <matrix.h>
#include <lgsound.h>
#include <config.h>
#include <mprintf.h>

#include <storeapi.h>
#include <resapi.h>
#include <sndrstyp.h>

#include <appsfx.h>
#include <appsfxi.h>
#include <wrtype.h>
#include <objpos.h>
#include <auxsnd.h>
#include <prikind.h>

#include <rooms.h>
#include <roomsys.h>
#include <roomeax.h>
#include <roomprop.h>
#include <playrobj.h>
#include <osysbase.h>
#include <timelog.h>
#include <simtime.h>  // for horror of dynamic channels

#include <songutil.h>

#ifdef EDITOR
#include <songedtr.h>
#endif

#include <objmedia.h>
#include <physapi.h>

////////////
// profiling and timers

// define this to get mono-prints of timing
//   info, and to get play_sfx timings to work
//#define APPSFX_TIMINGS

#ifdef APPSFX_TIMINGS
#define PROFILE_ON
#endif

// We ask the mixer for more channels than the user requests
// so that some channels are always available to metagame.
// There's no speed overhead since in-game allocation is self-
// limiting.
#define SFX_BONUSCHANNELS 6

#include <timings.h>

#include <dbmem.h>      // last header, yada yada

//#define MIXER_COUNTS
//#define MPRINT_TRACK
//#define STRESS_ASYNCH
//#define DYNAMIC_CHANNELS

#ifdef MPRINT_TRACK
#define snd_mprintf(x) mprintf x
static int GetRefCount(IRes *p) { int x=0; if (p) { x=p->AddRef(); p->Release(); } return x-1; }
#else
#define snd_mprintf(x)
#define GetRefCount(p) (666)
#endif

DECLARE_TIMER(StartPlayTimer, Average);
DECLARE_TIMER(SndMixerTimer, Average);
DECLARE_TIMER(SfxUpdateTimer, Average);

///////////////////////////////////////
// data structures, static globals

typedef struct {
   uchar  type;      // sound effect type, set by user/app
   char   ID;        // location in fxlist
   ushort pad;       // i hate everything
   uint   flags;     // for quick determination of what to do in updates
   ObjID  attach;    // object attached to
   mxs_vector loc;   // location for non-attached sounds, or bounce loc
   ISndSample *pSnd; // actual sound effect we are playing
   IRes* pRes;       // handle to resource being used
   char  *name;      // for streaming sounds, i guess, for now, maybe
   int master_gain;  // master volume for the sample, as opposed to attenuated/cur
   int distance;     // distance to sound origination point (-1 == stright line)
   sfx_parm parm;    // current parameters locally for the sample
   ulong  play_time; // when did we first get sample
   char* pStreamBuffer;  // Buffer used when streaming.  NULL if not streamed.
   mxs_real blocking_factor;  // current blocking factor
} _sfx;              // maybe name should be for all, extension generates type


int                  sfx_use_channels           = 8; // SFX_CHANNELS;
static _sfx          fxlist[SFX_MAX_COUNT];
static ISndMixer*    pSndMixer                  = NULL;
static sfx_parm      def_parms                  = {0,0,0,0,0,-1,0,0,0,-1,NULL,NULL};
static float         attenuation_factor         = 55;
static float         pan_factor                 = 0.4;
static eSnd3DMethod  sfx3DMethod                = kSnd3DMethodSoftware;
static int           max_simul_hardware_3d_snds = 0;  // @TODO: pick which ones we use less idiotically
static BOOL          in_kill;
static BOOL          sfxCheckChannels           = TRUE;
static int           sgLastVolumeSetting        = 0; // So we can restore volume between close and init.
static int           sgLastRoomType;
static float         sgSourceReverbMix          = 0.6;
static float         sgGainScale                = 1.0;
static int           sgMixerTimeout             = -1;
static BOOL          rev_stereo                 = FALSE;
static char          dummy_sound[64]            = {'\0',};
static BOOL          sound_always_delay         = FALSE;
static BOOL          sfx_loud_cap               = FALSE;
static cDynArray<_sfx> g_DeferredCallbackList;
static BOOL          g_deferCallbacks;
static BOOL          g_allowAsynch              = TRUE;
static BOOL          g_asynchAll                = TRUE;
static BOOL          sgLogRoomTransitions       = FALSE;
static BOOL          sgSpewMixerAttribs         = FALSE;
static int           sgVol2D;
static int           sgVol3D;
static int           sgVol2DHW;
static int           sgVol3DHW;
static int           sgVolMusic;
static BOOL          sgVolsChanged;
static BOOL          use2DHWmod; //add in sgVol2DHW
static BOOL          use3DHWmod; //add in sgVol3DHW

///////////////////////////////////////////////////////////
// Function callback definitions.
BOOL (*appsfxModeCallback)(sfx_mode mode, int handle, sfx_parm *parm) = NULL;

#define SFX_GAME_BASE_INDEX     0
#define SFX_METAGAME_BASE_INDEX SFX_MAX_MODE_COUNT
static int          base_index; // this is a dumb name

#define _sfx_get_hnd_from_ptr(fx) (fx-fxlist)

///////////////////////////////////////////////////////////
// Forward declarations of the internal functions
static BOOL _sfx_update(_sfx *fx, BOOL init, ulong cur_time);
static void _sfx_free(_sfx *fx, BOOL makeCallback);
static void SFX_Store_Config();

// simple system to set flags fields for app called types
// this array is [BF][type], and takes a BF and user type and tells
// what flags that implies.  
#define SFX_BF_RAW 0
#define SFX_BF_OBJ 1
#define SFX_BF_VEC 2
uchar base_flags[3][3]=
{
   {SFXFLG_STATIC,SFXFLG_STATIC,SFXFLG_STATIC},
   {SFXFLG_STATIC,SFXFLG_STATIC|SFXFLG_OBJ,SFXFLG_POS|SFXFLG_OBJ},
   {SFXFLG_STATIC,SFXFLG_STATIC,SFXFLG_POS},
};

#define set_flags(fx,which,app) \
    (fx->flags=base_flags[which][fx->type]|(app&SFXFLG_APP_MASK))
 
BOOL _sfx_priority_callback( ISndMixer    *pMixer,
                             ISndSample   *pSample,
                             void         *pCBData );

#ifdef DBG_ON
void DbgCheckInUseChannels(void);  // check to make sure we are all cleaned up
#else
#define DbgCheckInUseChannels()
#endif

ISearchPath *pSoundPath = 0;

static int  g_SFX_Device = SFXDEVICE_Software;

///////////////////////////////////////
// have some horror

#ifdef DYNAMIC_CHANNELS
#define FRAME_HISTORY_LEN     (0x20)
#define FRAME_HISTORY_MASK    (0x1f)
#define FRAME_HISTORY_SHIFT      (5)
static int sfx_time_log[FRAME_HISTORY_LEN];
static int sfx_time_total=0;
static int sfx_time_index=0;
#define IDEAL_MIN_FRAME_RATE    (16)
#define IDEAL_MIN_FRAME_MS    (1000/IDEAL_MIN_FRAME_RATE)
#define DYNAMIC_MODIFIER(fr)    ((IDEAL_MIN_FRAME_RATE-fr)/4)  // 0 - 3 channels 
#endif DYNAMIC_CHANNELS

///////////////////////////////////////
// startup/shutdown

void SFXClose(void)
{
   // Store config variables.
   SFX_Store_Config ();

   if (pSndMixer)
   {
      // Store current master volume setting.
      sgLastVolumeSetting = SFX_GetMasterVolume();

      int i;
      pSndMixer->StopAllSamples();
      pSndMixer->Update();      // trigger those callbacks
      for (i=0; i<SFX_MAX_COUNT; i++)
         if (fxlist[i].type!=SFX_NONE)
            _sfx_free(&fxlist[i], TRUE);
      
#ifndef SHIP
      for (i=0; i<SFX_MAX_COUNT; i++)
         if (fxlist[i].pSnd)
         {
            Warning(("Some sounds were not fully cleaned up\n"));
            break;
         }
#endif
      
      // Shutdown reverb if in use.
      if ( pSndMixer->Have3DReverb () )
         pSndMixer->Shutdown3DReverb();

      // Shut down song system.
#ifdef EDITOR
      SongEd_Shutdown();
#endif
      SongUtilShutdown();

      TIMELOG_DUMP( "sndlog.txt" );
      int ref = pSndMixer->Release();
      AssertMsg1(!ref, "Sound mixer reference count is not 0. (%d)\n", ref);
   }
   DbgCheckInUseChannels();
   pSndMixer=NULL;

   if (pSoundPath) {
      pSoundPath->Release();
      pSoundPath = NULL;
   }
}


#ifndef SHIP
#define IfLoud_SFX_Quick_List(all) do { if (sfx_loud_cap)  SFX_Quick_List(all); } while (0)
#else
#define IfLoud_SFX_Quick_List(all)
#endif   

// refresh any config vars we are allowed to
// doesnt allow channels yet
void SFX_Refresh_Config(void)
{

#ifndef SHIP
   if (config_is_defined("all_one_sound"))
   {
      config_get_raw("all_one_sound",dummy_sound,64);
      if (dummy_sound[0]=='\0')
         strcpy(dummy_sound,"alarm");
   }
   sound_always_delay   = config_is_defined ("sfx_always_delay");
   sfx_loud_cap         = config_is_defined ("sfx_loud_cap");
   sgSpewMixerAttribs   = config_is_defined ("sfx_spew_mixer_attribs");
#endif

   config_get_int       ("sfx_mixer_timeout",   &sgMixerTimeout);
   config_get_float     ("attenuation_factor",  &attenuation_factor);
   config_get_float     ("pan_factor",          &pan_factor);
   config_get_int       ("sfx_device",          &g_SFX_Device);
   config_get_int       ("max_sfx_3d",          &max_simul_hardware_3d_snds);
   config_get_float     ("sfx_source_reverb_mix", &sgSourceReverbMix);
   config_get_float     ("sfx_gain_scale",      &sgGainScale);

   config_get_int       ("sfx_vol_2d",          &sgVol2D);
   config_get_int       ("sfx_vol_3d",          &sgVol3D);
   // volume offsets to add only when hw acceleration is on
   sgVol2DHW = -300;
   sgVol3DHW = 0;
   config_get_int       ("sfx_vol_2d_hw",       &sgVol2DHW);
   config_get_int       ("sfx_vol_3d_hw",       &sgVol3DHW);
   config_get_int       ("sfx_vol_music",       &sgVolMusic);

   g_allowAsynch        = ! (config_is_defined  ("sfx_no_asynch"));
   g_asynchAll          = ! (config_is_defined  ("sfx_no_asynch_all"));
   sfxCheckChannels     = ! (config_is_defined  ("sfx_preset_3d_channels"));
   sgLogRoomTransitions = config_is_defined     ("sfx_log_room_transitions");

#ifdef DYNAMIC_CHANNELS   
   for (int i=0; i<FRAME_HISTORY_LEN; i++)
      sfx_time_log[i]=IDEAL_MIN_FRAME_MS;
   sfx_time_total=i*IDEAL_MIN_FRAME_MS;
#endif   
}

//
// Save standard config variables.
//
void SFX_Store_Config()
{
   config_set_int       ("sfx_device",          g_SFX_Device);
   config_set_int       ("sfx_vol_2d",          sgVol2D);
   config_set_int       ("sfx_vol_3d",          sgVol3D);
   config_set_int       ("sfx_vol_music",       sgVolMusic);
}

BOOL SFXInit(void)
{
   sSndAttribs attribs;
   int mixer_rate=22050;
   BOOL createOk;
   sSndEnvironment snd3DEnv;
   char methodName[256];

   if (pSndMixer)
      return TRUE;

   sgLastRoomType          = -1;
   sgGainScale             = 1.0F;
   sgVol2D                 = 0;
   sgVol3D                 = 0;
   sgVolMusic              = 0;
   sgVolsChanged           = TRUE;
   use2DHWmod              = FALSE;
   use3DHWmod              = FALSE;

   SFX_Refresh_Config();  // refresh reloadable config parms

   if (config_is_defined("no_sound") || (g_SFX_Device == SFXDEVICE_NoSound))
      return TRUE;

   if (config_is_defined("reverse_stereo"))
      rev_stereo = TRUE;

   for (int i = 0; i < SFX_MAX_COUNT; i++) {
      fxlist[i].type=SFX_NONE;
      fxlist[i].pSnd=NULL;
      fxlist[i].pRes=NULL;
      fxlist[i].ID = i;
   }

   base_index = SFX_METAGAME_BASE_INDEX;

   config_get_int("mixer_rate", &mixer_rate);
   
   // generic parms for mixer
   attribs.dataType = kSndDataPCM;  // PCM data
   attribs.sampleRate = mixer_rate; // 22kHz
   attribs.nChannels = 2;           // Stereo

   // the following have no effect in Init
   attribs.bytesPerBlock = 0;
   attribs.samplesPerBlock = 0;
   attribs.numSamples = 0;
   
   if (config_is_defined("force_8_bit_sound"))
      attribs.bitsPerSample = 8;
   else
      attribs.bitsPerSample = 16;

   // Intialize channels in use.
   sfx_use_channels = 12;
   config_get_int("default_sfx_channels", &sfx_use_channels);
   config_get_int("sfx_channels", &sfx_use_channels);

   

#ifndef SHIP
   config_get_int("sfx_channels_override",&sfx_use_channels);
#endif

   //
   // Set up pSoundPath to point to the sound files
   //
   AutoAppIPtr(ResMan);
   pSoundPath = pResMan->NewSearchPath();
   pSoundPath->AddPathTrees("snd2\\", FALSE);
   pSoundPath->AddPathTrees("snd\\", FALSE);
   pSoundPath->Ready();

   //
   // pick 3D sound method - default is dark's built-in pan/vol
   //
   config_get_raw( "snd3d", methodName, sizeof(methodName) );
   if ( ! strcmpi(methodName, "a3d") )
      g_SFX_Device = SFXDEVICE_A3D;

#ifdef USE_QSOUND
   else if ( ! strcmpi(methodName, "qmixer") )
      g_SFX_Device = SFXDEVICE_QMIXER;
#endif

   createOk = FALSE;

   switch (g_SFX_Device)
   {
      case SFXDEVICE_Software:
      {
         sfx3DMethod = kSnd3DMethodNone;
         createOk = SndCreateMixer(&pSndMixer,NULL);
         use2DHWmod = FALSE;
         use3DHWmod = FALSE;
         break;
      }

      case SFXDEVICE_A3D:
      {
         sfx3DMethod = kSnd3DMethodHardware;
         // this really just means "use 3d audio HW acceleration", not just
         //   A3d devices
         //createOk = SndCreateA3DMixer(&pSndMixer,NULL);
         createOk = SndCreateMixer(&pSndMixer,NULL);
         // factor in volume offsets which are used to balance volumes
         //  when hardware acceleration is enabled
         // Don't factor in these offsets.  Just tell the vol get to do it.
         // AMSD 3/24/00
         //sgVol2D += sgVol2DHW;
         //sgVol3D += sgVol3DHW;
         use2DHWmod = TRUE;
         use3DHWmod = TRUE;
         break;
      }

#ifdef USE_QSOUND
      case SFXDEVICE_QMIXER:
      {
         sfx3DMethod = kSnd3DMethodSoftware;
         createOk = SndCreateQSMixer(&pSndMixer, NULL);
         use2DHWmod = FALSE;
         use3DHWmod = FALSE;
         break;
      }
#endif

      default:
      {
         Warning(("Unknown mixer type: %d\n", g_SFX_Device));
         use2DHWmod = FALSE;
         use3DHWmod = FALSE;
         break;
      }
   }

   if ( createOk ) {
      TIMELOG_INIT( 16384 );
      if (sgMixerTimeout != -1)
      {
         pSndMixer->SetTimeout(sgMixerTimeout);
      }

      SongUtilInit(pSndMixer);
      SongUtilSetVolume(sgVolMusic);
#ifdef EDITOR
      SongEd_Init();
#endif

      pSndMixer->Set3DMethod(  sfx3DMethod );
      if (pSndMixer->Init(NULL,sfx_use_channels + SFX_BONUSCHANNELS,&attribs)==kSndOk)
      {
#ifndef SHIP
         // Set config var to make sure mixer initializes to expected format.
         if (sgSpewMixerAttribs)
         {
            sSndAttribs actualAttribs;
            pSndMixer->GetAttribs (&actualAttribs);
            mprintf ("Primary sound buffer format : %d bits, %d Hz, ",
            actualAttribs.bitsPerSample, actualAttribs.sampleRate);
            switch (actualAttribs.nChannels)
            {
               case 1:
                  mprintf ("mono.\n");
                  break;
               case 2:
                  mprintf ("stereo.\n");
                  break;
               default:
                  mprintf ("%d channels (STRANGE!)\n", actualAttribs.nChannels);
                  break;
            }
         }
#endif
         // Restore old volume, if any (0 if not).
         SFX_SetMasterVolume (sgLastVolumeSetting);
         pSndMixer->RegisterPriorityCallback(  _sfx_priority_callback, NULL );
         if ( sfx3DMethod != kSnd3DMethodNone ) {
            // Init reverb.
            int useReverb = 1;
            config_get_int ("sfx_eax", &useReverb);
            // if ( ! (config_is_defined ("sfx_no_reverb")) )
            if (useReverb)
               pSndMixer->Init3DReverb();

            // Init 3D Environment.
            snd3DEnv.dopplerFactor = 1.0;

            // The sound library does meters, dark does feet
            snd3DEnv.distanceFactor = 0.3048;

            // We do our own attenuation for consistency between hw and sw.
            snd3DEnv.rolloffFactor = 0.0;
            config_get_float( "snd3d_distance", &(snd3DEnv.distanceFactor) );
            config_get_float( "snd3d_rolloff", &(snd3DEnv.rolloffFactor) );
            pSndMixer->Set3DEnvironment(  &snd3DEnv );
         }
         return TRUE;      // seems to have worked
      }
   }
   pSndMixer=NULL;
   SFXClose();
   Warning(("Sound failed to correctly init\n"));
   return FALSE;
}

BOOL SFXReset(void)
{
   g_deferCallbacks = TRUE;

   SFXClose();
   BOOL retval = SFXInit();

   g_deferCallbacks = FALSE;

   // If we sucessfully reset, we can pass on our callbacks.  If not,
   // then we should keep them around until we sucessfully reset.
   if (retval)
   {
      for (int i=0; i<g_DeferredCallbackList.Size(); i++)
      {
         _sfx fx = g_DeferredCallbackList[i];
         fx.parm.end_callback(fx.ID, fx.parm.user_data);
      }
      g_DeferredCallbackList.SetSize(0);
   }

   return retval;
}

BOOL SFX_SetSoundDevice(int device, BOOL reset)
{
   g_SFX_Device = device;

   if (reset)
      return SFXReset();
   else
      return TRUE;
}

int SFX_GetSoundDevice()
{
   return g_SFX_Device;
}

BOOL SFXActive(void)
{
   return pSndMixer != NULL; 
}

///////////////////////////////////////
// internal creation/destruction of samples

static int hndInUse = 0;
static int chanInUse = 0;
static int cur3dSndCnt = 0;

int SFX_Get(void)
{
   int i;
   if (pSndMixer!=NULL)  // lets not "get" effects if we have no mixer
      for (i=0; i<SFX_MAX_MODE_COUNT; ++i)
         if (fxlist[i + base_index].type==SFX_NONE)
         {
            hndInUse++;
            return i + base_index;
         }
   IfLoud_SFX_Quick_List(FALSE);
   return SFX_NO_HND;
}

#ifdef DBG_ON
  // check to make sure we are all cleaned up
void DbgCheckInUseChannels(void)
{
   if (hndInUse) { Warning(("appsfx: HndInUse %d on close\n",hndInUse)); hndInUse=0; }
   if (chanInUse) { Warning(("appsfx: chanInUse %d on close\n",chanInUse)); chanInUse=0; }
   if (cur3dSndCnt) { Warning(("appsfx: cur3dSndCnt %d on close\n",cur3dSndCnt)); cur3dSndCnt=0; }   
}
#endif

///////////////////////////////////////
// internal convienience functions

// just set the _sfx vector location to be where us is
static void _sfx_fill_vec_from_obj(_sfx *us)
{  
   ObjPos* pos = ObjPosGet(us->attach);
   if (pos!=NULL)
      mx_copy_vec(&us->loc,&pos->loc.vec);
   else
      Warning(("fillvecfromobj: nonexistant objid %d\n",us->attach));
}

// load up new from type/parm/name correctly
void _sfx_basic_setup(int which, _sfx *newfx, int type, sfx_parm *parm, char *sfx_name)
{
   if (parm)
      newfx->parm=*parm;
   else
      newfx->parm=def_parms;
   newfx->attach=0;
   newfx->pSnd=NULL;
   newfx->pRes=NULL;
   newfx->pStreamBuffer = NULL;

#ifndef SHIP   
   if (dummy_sound[0]!='\0')
       newfx->name=dummy_sound;
   else
#endif
      newfx->name=sfx_name;
   
   // real issue is now setting type and flags correctly
   newfx->type=type;  // easy for now, eh?

   // Set gain.  Gain scale is normally 1, so we test this to avoid float->int expense.
   if ( 1.0F == sgGainScale )
      newfx->master_gain = newfx->parm.gain;
   else
      newfx->master_gain = (int) ((float)newfx->parm.gain * sgGainScale);

   newfx->distance=newfx->parm.distance;

   set_flags(newfx,which,newfx->parm.flag);
   newfx->play_time = tm_get_millisec_unrecorded();
   if (newfx->parm.delay>0)
      newfx->flags |= SFXFLG_DELAY;
#ifndef SHIP
   if (sound_always_delay)
      newfx->flags |= SFXFLG_DELAY;
#endif
}

static void _sfx_cleanup_fx(_sfx *fx, BOOL make_callback)
{
   snd_mprintf(("Cleanup %d: type %d IRes %x (%d) snd %x name %s%s (%d %x %d)\n",
                _sfx_get_hnd_from_ptr(fx),fx->type,fx->pRes,GetRefCount(fx->pRes),fx->pSnd,fx->name,(fx->flags&SFXFLG_ASYNCHWAIT)?" InAsychQueue":"",
                make_callback&&fx->parm.end_callback,fx->ID,fx->parm.user_data));

   if (fx->pRes!=NULL)
   {
      if (fx->flags&SFXFLG_ASYNCHWAIT)
      {
         void * pSound;
         fx->pRes->GetAsyncResult(&pSound);
         snd_mprintf(("Cleanup %d: %s was in async queue\n",_sfx_get_hnd_from_ptr(fx),fx->name));
      }

      // Unlock only if non-stream (since streams never get locked).
      if ( ! (fx->flags & SFXFLG_STREAM) )
         fx->pRes->Unlock();

      fx->pRes->Release();
      fx->pRes = NULL;
   }

   fx->type=SFX_NONE;
   fx->pSnd=NULL;

   // Clean up streaming buffer if any.
   if ( NULL != fx->pStreamBuffer )
   {
      delete [] (fx->pStreamBuffer);
      fx->pStreamBuffer = NULL;
   }

   if (fx->flags&SFXFLG_IS3D)
   {
      if (cur3dSndCnt<=0)
         Warning(("3d Sound Underflow in cleanup %d\n",_sfx_get_hnd_from_ptr(fx)));
      cur3dSndCnt--;
   }
   if ((fx->flags&SFXFLG_NOCALLBACK)==0)
   {
      if (make_callback && fx->parm.end_callback)
      {
         snd_mprintf(("making callback flags %x\n",fx->flags));
         if (g_deferCallbacks)
            g_DeferredCallbackList.Append(*fx);
         else
            fx->parm.end_callback(fx->ID, fx->parm.user_data);
      }
   }
   else
      snd_mprintf(("Abort callback due to NOCALLBACK\n"));
   hndInUse--;
}

// called when a sound stops
static void _sfx_end_callback(ISndSample *pSample, void *pCBD)
{
   _sfx *fx = (_sfx *) pCBD;

   if (fx)
      _sfx_cleanup_fx(fx, TRUE);

   pSample->Release();
   chanInUse--;
}

// This is called at the end of each iteration of a looped sample,
// including the last, shortly _before_ that iteration ends.
static void _sfx_loop_callback(ISndSource *pSndSource, uint32 *pCBD)
{
   _sfx *fx;
   long *pTmp;

   // skip first param (arg count), next param is sfx ptr
   pTmp = (long *) pCBD;
   fx = (_sfx *) pTmp[1];

   if (fx && fx->parm.loop_callback)
      fx->parm.loop_callback(fx->ID, fx->parm.user_data);
}

// software free
static void _sfx_free(_sfx *fx, BOOL makeCallback)
{
   if (fx->type==SFX_NONE)
   {
      Warning(("Trying to free non-alloced sample %d\n",
               _sfx_get_hnd_from_ptr(fx)));
      return;
   }

   if (fx->pSnd)
      fx->pSnd->Stop();
   else
      _sfx_cleanup_fx(fx, makeCallback);
}

// actually goes and gets the ISndSample, fills in the real data
// for now, it also looks up the resource, cause why not
static bool _sfx_start_play(_sfx *fx, BOOL after_delay)
{
   AutoAppIPtr(ResMan);
   AUTO_TIMER(StartPlayTimer);
   char *tmpname=fx->name;
   // The actual sound data:
   void *pSound = NULL;
   bool bHaveDataSource = false;

   // if we're delaying, then skip start
   if (fx->flags & SFXFLG_DELAY)
      return TRUE;

   //   fx->name=NULL;     // so we dont mistakenly refer to it later
   // We prep and lock the sound together (and will unlock and release
   // together later), because we can't allow the memory to go away while
   // the sound may be playing...

   if (fx->pRes==NULL)
   {
      fx->pRes = pResMan->Bind(tmpname, RESTYPE_SOUND, pSoundPath);
      snd_mprintf(("Binding %d: %s (%d)\n",_sfx_get_hnd_from_ptr(fx),tmpname,GetRefCount(fx->pRes)));
   }

   if (fx->pRes)
   {
      if (fx->flags & SFXFLG_STREAM)
         bHaveDataSource = true;
      else
      {
         // @SHIP: if we get pumpevents going in metagame, remove the base_index == below
         if (g_allowAsynch&&((fx->flags&SFXFLG_ASYNCHWAIT)==0)&&
             ((g_asynchAll&&(base_index==SFX_GAME_BASE_INDEX))||(fx->flags&SFXFLG_ASYNCH)))
         {
            fx->pRes->AsyncLock(kPriorityNormal);  // does this return anything real?
            snd_mprintf(("AsynchLock %d: %s (%d)\n",_sfx_get_hnd_from_ptr(fx),tmpname,GetRefCount(fx->pRes)));
            fx->flags|=SFXFLG_ASYNCHWAIT;
            fx->flags&=~SFXFLG_ASYNCH;
#ifdef STRESS_ASYNCH         
            return TRUE; // to always delay a frame
#endif         
         }

         if (fx->flags&SFXFLG_ASYNCHWAIT)
         {
            if (fx->pRes->IsAsyncFulfilled())  // can check this outside, too
            {
               fx->pRes->GetAsyncResult(&pSound);
               snd_mprintf(("AsynchFulfill %d: %s (%d)\n",_sfx_get_hnd_from_ptr(fx),tmpname,GetRefCount(fx->pRes)));
               fx->flags&=~SFXFLG_ASYNCHWAIT;
            }
            else
               return TRUE; // still waiting
         }
         else
         {
            snd_mprintf(("NormalLock %d: %s (%d)\n",_sfx_get_hnd_from_ptr(fx),tmpname,GetRefCount(fx->pRes)));;
            pSound = fx->pRes->Lock();
         }

         // Check for successful lock.
         if (pSound)
            bHaveDataSource = true;
         else
         {
            Warning(("sfx_start_play: couldn't Lock %s\n", tmpname));
            fx->pRes->Release();
            fx->pRes = NULL;
         }
      }
   }
   else
      Warning(("sfx_start_play: couldn't Prep %s\n", tmpname));

   if (bHaveDataSource)
   {
#ifndef SHIP
      // Allow config var to exercise bad mixer.
      int forceFailCount = -1;
      fx->pSnd = NULL;
      config_get_int( "sfx_break_mixer", &forceFailCount );
      if (forceFailCount == -1 || SFX_Channel_Count() < forceFailCount )
#endif
      {
      // Do we want callbacks for looping?
      if (fx->parm.flag & SFXFLG_LOOP)
      {
         fx->pSnd = CreateMemSoundSourceLooped(pSndMixer,
                                              pSound,
                                              fx->pRes->GetSize(),
                                              _sfx_end_callback, 
                                              fx,
                                              &fx->parm.pSndSource,
                                              fx->parm.num_loops,
                                              _sfx_loop_callback, 
                                              fx);
      }                                        
      else if (fx->parm.flag & SFXFLG_STREAM)
      {
         fx->pSnd = CreateNRezStreamedSoundSource (pSndMixer,
                                           fx->pRes,
                                           &(fx->pStreamBuffer),
                                           _sfx_end_callback,
                                           fx,
                                           &fx->parm.pSndSource);
      }
      else
      {
         fx->pSnd = CreateMemSoundSource(pSndMixer,
                                        pSound,
                                        fx->pRes->GetSize(),
                                        _sfx_end_callback,
                                        fx);
      }
      }

      if (fx->pSnd)
      {
         NAME_SAMPLE( fx->pSnd, fx->name );
         if ( (fx->flags & SFXFLG_POS) && (sfx3DMethod != kSnd3DMethodNone) )
         {  // if we want 3D, make sure we arent using too many hardware channels
            eSnd3DMethod useMethod=sfx3DMethod;
            if (sfx3DMethod==kSnd3DMethodHardware)
            {
               long HW3dcnt=1;
               if (sfxCheckChannels)
                  pSndMixer->FreeHWChannelCount(NULL, &HW3dcnt);
               if (HW3dcnt>0 &&
                   ( (max_simul_hardware_3d_snds==0) ||
                     (cur3dSndCnt < max_simul_hardware_3d_snds) ) )
               {
                  cur3dSndCnt++;
                  fx->flags |= SFXFLG_IS3D;

                  // Set reverb mix.
                  if ( pSndMixer->Have3DReverb() )
                     fx->pSnd->Set3DReverbMix(sgSourceReverbMix);
               }
               else
               {  // dont set method hardware, cause the driver just uses HW anyway
                  useMethod=kSnd3DMethodNone;
                  Warning(("Out of channels for 3d sounds for %d (%s)\n",
                           _sfx_get_hnd_from_ptr(fx),fx->name));
               }
            }
            fx->pSnd->Set3DMethod( useMethod );
            fx->pSnd->Set3DMode( kSnd3DModeNormal );
         }
         
         _sfx_update(fx,TRUE,tm_get_millisec_unrecorded());
         fx->flags |= SFXFLG_HAPPY;
         // give sample back ptr to us so it can clear happy if play fails
         fx->pSnd->SetData((long) fx );
         fx->pSnd->SetGroup((uint32) fx->parm.group);
         fx->pSnd->Play();

         chanInUse++;  // since we will always hit the appsfx callback now
         
         if ( fx->flags&SFXFLG_HAPPY )
         {
            snd_mprintf(("Play %d: type %d pRes %x (%d) snd %x name %s\n",
                         _sfx_get_hnd_from_ptr(fx),fx->type,fx->pRes,GetRefCount(fx->pRes),fx->pSnd,fx->name));
            return TRUE;
         }
         else
         {
            snd_mprintf(("_sfx_start_play: couldnt play sound sample for %s (aftrdlay = %d)\n",tmpname,after_delay));
            // if this is the frame we called into appsfx
            // i.e. the frame that SFX_PlayRaw or whatever was called
            // and we are failing right away
            // we are going to return handle -1 to the app
            // so lets not make the app callback
            // since, really, we are gonna tell the app we arent playing
            if (!after_delay)
            {
               fx->flags|=SFXFLG_NOCALLBACK;
               snd_mprintf(("tried to set callback flags, %x\n",fx->flags));
            }
            IfLoud_SFX_Quick_List(FALSE);
         }
      } 
      else
      {
#if defined(DBG_ON) || defined(MPRINT_TRACK) || defined(PLAYTEST)
         mprintf("_sfx_start_play: couldnt create %s\n",tmpname);
#endif         
         IfLoud_SFX_Quick_List(FALSE);         
      }
   }
#if defined(DBG_ON) || defined(MPRINT_TRACK)
   else
      mprintf("_sfx_start_play: couldnt Prep and Lock %s\n",tmpname);
#endif
   TLOG1( "failed to alloc chan to [%d]", fx->ID );
   if (after_delay==FALSE)
   {
      snd_mprintf(("sfx_free fx flg %x ad %d\n",fx->flags,after_delay));
   }
   _sfx_free(fx,after_delay);
   return FALSE;
}

int SFX_Attenuate(int master_gain, int dist)
{
   return master_gain - (dist * attenuation_factor);
}

int SFX_MaxDist(int gain)
{
   int max_dist;

   max_dist = (int)((5000 + gain) / attenuation_factor);

   TLOG2( "Max dist %d for gain %d", max_dist, gain );
   return max_dist;
}

const char *SFX_Name(int hnd)
{
   return fxlist[hnd].name;
}

const sfx_parm *SFX_Parm(int hnd)
{
   return &fxlist[hnd].parm;
}

///////////////////////////////////////
// playback/update loop subsystems

#define NEAH_SND  (6.0)
#define PAN_RANGE (4000) // even though 10k is max, it seems to be too much...
#define SND_GAIN_MIN (-5000) // likewise

static mxs_vector head_pos;
static mxs_angvec head_ang;

static BOOL _sfx_check_delay(_sfx *fx, BOOL *init, ulong cur_time)
{
   if (fx->flags&SFXFLG_DELAY)
      if (cur_time>=fx->play_time+fx->parm.delay)
      {  // times up, go start the sample
         fx->flags = fx->flags&~SFXFLG_DELAY;
         if (!_sfx_start_play(fx,TRUE))
            return FALSE;
         *init = TRUE;
      }
      else
         return FALSE;
   if (g_allowAsynch)
      if (fx->flags&SFXFLG_ASYNCHWAIT)
         if (fx->pRes->IsAsyncFulfilled())
         {
            if (!_sfx_start_play(fx,TRUE))
               return FALSE;
            else
               *init = TRUE;
         }
      else
         return FALSE;
   return TRUE;
}

// returns whether or not it asked for a remix
static BOOL _sfx_update(_sfx *fx, BOOL init, ulong cur_time)
{
   bool change=FALSE;
   int master_gain;
   BOOL fade = FALSE;
   sSndVector sndPos;
  
   if (!fx->pSnd)
   {
      Warning(("_sfx_update with no sound (pSnd NULL) handle %d\n",
                _sfx_get_hnd_from_ptr(fx)));
      return FALSE;
   }

   master_gain = fx->master_gain;
   // This fade stuff is done manually rather than through the sound library
   // so that it interacts with our 3d stuff properly. This will all presumably
   // go away when you we use the new sound library stuff.
   if (fx->parm.fade>0)
   {
      if (cur_time<fx->play_time+fx->parm.fade)
      {
         // fade in
         master_gain += (int)((float)(fx->play_time+fx->parm.fade-cur_time)/(float)fx->parm.fade*
            (float)(SND_GAIN_MIN-fx->master_gain));
         fade = TRUE;
      }
      else
      {
         ulong remaining_time = fx->pSnd->PlayTimeRemaining();
         if (remaining_time<fx->parm.fade)
         {
            // fade out
            master_gain += (int)((1.-(float)remaining_time/(float)fx->parm.fade)*
               (float)(SND_GAIN_MIN-fx->master_gain));
            fade = TRUE;
         }
      }
   }
   if (!(fx->flags&SFXFLG_STATIC))
   {  // if we have dynamic update, lets go
      if (fx->flags&SFXFLG_OBJ)   // get new object position
         _sfx_fill_vec_from_obj(fx);
      if (fx->flags&SFXFLG_POS)
      {
         // Calculate gain.  Now same for both hardware and software.
         float dist = fx->distance;
         int gain;

         if (dist < 0) {
            dist = mx_dist_vec(&head_pos,&fx->loc);
         }
         // gain check first
         gain = SFX_Attenuate(master_gain, (int)dist);
         if (gain!=fx->parm.gain)
         {
            fx->parm.gain=gain;
            change=TRUE;
         }

         // Calculate stereo mix.
         if ( sfx3DMethod == kSnd3DMethodNone )
         {
            // calc pan
            {
               mxs_matrix locframe;
               mxs_vector delta;
               mxs_real   headon;
               mxs_real   headon_abs;
               int pan;

               // calc our heading matrix and the normal from head to sound
               mx_ang2mat(&locframe, &head_ang);
               mx_sub_vec(&delta, &fx->loc, &head_pos);
               mx_normeq_vec(&delta);

               // determine how directly facing the sound we are
               headon = mx_dot_vec(&delta, &locframe.vec[1]);
               headon_abs = fabs(headon) * pan_factor;
               pan = ((int)(10000 - 10000 * cos(1.570796 * headon_abs)));

               // reverse if on other side
               if (headon > 0)
                  pan = -pan;

               // reverse if forced to
               if (rev_stereo)
                  pan = -pan;

               // only update if changed
               if (pan != fx->parm.pan)
               {
                  fx->parm.pan = pan;
                  change = TRUE;
               }
            }
         }
         else
         {
            // Stereo mix in hardware is done using Set3DPosition.
            sndPos.x = fx->loc.x;
            sndPos.y = fx->loc.y;
            sndPos.z = fx->loc.z;
            fx->pSnd->Set3DPosition(&sndPos);
         }
      }
   }
   else if (fade)
   {
      fx->parm.gain = master_gain;
      change = TRUE;
   }

   // Apply independent volume controls.
   if (sgVolsChanged)
   {
      change = TRUE;
   }

   if ( fx->blocking_factor != fx->parm.blocking_factor ) {
      // blocking factor has changed
      fx->blocking_factor = fx->parm.blocking_factor;
      change = TRUE;
   }

   if (change||init)
   {
      // Adjust 2d and 3d independent volumes.
      int net_gain = fx->parm.gain;

      if (fx->flags & SFXFLG_POS)
         net_gain += SFX_Get3dVolume();
      else
         net_gain += SFX_Get2dVolume();

      // Clip volume to valid range.
      if (net_gain < kSndMinVolume)
         net_gain = kSndMinVolume;
      if (net_gain > kSndMaxVolume)
         net_gain = kSndMaxVolume;

      fx->pSnd->SetVolume(net_gain);
      if (fx->pSnd->Get3DMethod() == kSnd3DMethodNone )
         fx->pSnd->SetPan(fx->parm.pan);

      if ( (fx->flags & SFXFLG_POS) && (sfx3DMethod == kSnd3DMethodHardware) && pSndMixer->Have3DOcclusion() ) {
         // set the occlusion here
         long occlusionLevel;
         float modBF;   // modified blocking factor
         static float bfBreakOut = 0.6;

         // do a simple two-piece linear mapping
         //  I do this because sounds become silent well before attenuation reaches kSndMinVolume
         //  0 ... BF_BREAK_IN ... 1    ->   0 ... bfBreakOut ... 1
#define BF_BREAK_IN  0.9
#define bfBreakOut  0.5
         modBF = fx->parm.blocking_factor;
         if ( modBF < BF_BREAK_IN ) {
            modBF *= (bfBreakOut / BF_BREAK_IN);
         } else {
            modBF = bfBreakOut + ( ((modBF - BF_BREAK_IN) / (1.0 - BF_BREAK_IN))
                                       *  (1.0 - bfBreakOut) );
         }
         occlusionLevel = modBF * (kSndMinVolume - net_gain);
         fx->pSnd->Set3DOcclusion( occlusionLevel );
      }
      return TRUE;
   }
   return FALSE;
}

#ifdef MIXER_COUNTS
static int sfx_cnts[256];
static int sfx_remix[256];
static int sfx_total=0;
static int sfx_remixes=0;
static int sfx_ptr=0;
#endif

void SFX_Frame(mxs_vector *pos, mxs_angvec *ang)
{
   static ulong cur_time;     // current time, updated each frame
   mxs_matrix orient;
   BOOL init;
   int i;

#ifdef DYNAMIC_CHANNELS
   sfx_time_total-=sfx_time_log[sfx_time_index];
   sfx_time_log[sfx_time_index]=GetSimFrameTime();
   sfx_time_total+=sfx_time_log[sfx_time_index];
   sfx_time_index=(sfx_time_index+1)&FRAME_HISTORY_MASK;
#endif   

#ifdef MIXER_COUNTS
   sfx_total-=sfx_cnts[sfx_ptr];
   sfx_remixes-=sfx_remix[sfx_ptr];
   sfx_cnts[sfx_ptr]=0;
#endif // MIXER_COUNTS

   if (pSndMixer==NULL) return;

   // update our global time 
   cur_time = tm_get_millisec_unrecorded();

   // @TODO: really this is an editor check only
   //  in game mode, we know we have a view position, so we should punt the ifs
   if (pos) head_pos=*pos;
   if (ang) head_ang=*ang;

// @TODO: this should only be done if camera is at player
//        otherwise, we should go decide to get Location of camera, not player

   // here is where we would like to know if we are/are not the player
   // we also only want to do this in game mode... i guess i could SimTimePassing
   if ( IsSimTimePassing() && pSndMixer->Have3DReverb() )   // just use (1) for test
   {
      // for now, assume we are the player
      cRoom *player_room = g_pRooms->GetObjRoom(PlayerObject());
      int room_type = player_room==NULL?-1:
                       g_pRoomEAX->GetAcoustics(player_room->GetRoomID());
      if ( room_type == -1 ) {
         if ( PhysGetObjMediaState(PlayerObject()) == kMS_Liquid_Submerged ) {
            room_type=kREVERB_UnderWater; // blub blub blub
         }
      }
      if (room_type==-1)
         room_type=GetMissionSecretEAXVar();
      if (room_type==-1)
         room_type=GetGameSysSecretEAXVar();
      if (room_type==-1)
         room_type=kREVERB_Generic; // well, who knows

      // Only set reverb when player enters a new room.
      if (room_type!=sgLastRoomType)
      {
#ifndef SHIP
         if (sgLogRoomTransitions)
         {
#ifdef EDITOR
            mprintf("new RT %s\n", g_RoomTypes[room_type]);
#else
            mprintf("new RT %d\n",room_type);
#endif
         }
#endif

         sgLastRoomType=room_type;

         if ( room_type!=-1 )
         {
            ReverbSettings revSet;
            revSet.flags = kREVERB_FlagType;
            revSet.type = room_type;
            pSndMixer->Set3DReverbSettings(&revSet);
         }
      }
   }
   
   // lets do some 3d whackitude
   TIMER_Start(SndMixerTimer);
   if ( sfx3DMethod != kSnd3DMethodNone ) {
      // update listener's position & orientation
      pSndMixer->Set3DPosition( (sSndVector *) &head_pos);
      mx_ang2mat( &orient, &head_ang );
      pSndMixer->Set3DOrientation(  (sSndVector *) &(orient.vec[0]), (sSndVector *) &(orient.vec[2]) );
   }

   pSndMixer->Update();
   TIMER_MarkStop(SndMixerTimer);

   TIMER_Start(SfxUpdateTimer);
   // go through all of our modes SFX's and update them
   for (i = 0; i < SFX_MAX_MODE_COUNT; ++i)   
      if (fxlist[i+base_index].type!=SFX_NONE)
      {
         init = FALSE;
#ifdef MIXER_COUNTS
         if (_sfx_check_delay(&fxlist[i+base_index], &init, cur_time))
            sfx_remix[sfx_ptr]+=_sfx_update(&fxlist[i+base_index], init, cur_time)?1:0;
         sfx_cnts[sfx_ptr]++;
#else
         if (_sfx_check_delay(&fxlist[i+base_index], &init, cur_time))
            _sfx_update(&fxlist[i+base_index], init, cur_time);
#endif
      }

#ifdef MIXER_COUNTS
   sfx_total+=sfx_cnts[sfx_ptr];
   sfx_remixes+=sfx_remix[sfx_ptr];
   sfx_ptr=(sfx_ptr+1)&0xff;
   if (sfx_ptr==0)
      mprintf("sfx: Cur %d, avg %g (remix %g)\n", sfx_cnts[(sfx_ptr-1)&0xff], (float)sfx_total/256.0, (float)sfx_remixes/256.0);
#endif

   // Don't constantly adjust 2d/3d volume.
   sgVolsChanged = FALSE;

   TIMER_MarkStop(SfxUpdateTimer);
}

static void FinishAsynchOps(void)
{
   BOOL init;
   int i, start_time=tm_get_millisec_unrecorded();
   
   AutoAppIPtr(AsyncReadQueue);
   pAsyncReadQueue->FulfillAll();
   
   for (i=0; i<SFX_MAX_COUNT; i++)
      if (fxlist[i].type!=SFX_NONE)
         if (fxlist[i].flags&SFXFLG_ASYNCHWAIT)
            while (!_sfx_check_delay(&fxlist[i], &init, 0))
               if (start_time+500>tm_get_millisec_unrecorded())
               {
#ifndef SHIP
                  mprintf("AsynchOp timeout\n");
#endif                  
                  break;
               }
}

///////////////////////////////////////
// Volume is attenuation from full volume, in millibels
void SFX_SetMasterVolume(int vol)
{
   if (pSndMixer)
      pSndMixer->SetMasterVolume(vol);
}


int SFX_GetMasterVolume()
{
   if (pSndMixer)
      return pSndMixer->GetMasterVolume();
   return 0;
}

///////////////////////////////////////
// These are the independently controllable volumes.
void SFX_Set2dVolume (int vol)
{
   sgVol2D              = vol;
   sgVolsChanged        = TRUE;
}

int SFX_Get2dVolume ()
{
   if (use2DHWmod)
      return sgVol2D + sgVol2DHW;
   else
      return sgVol2D;
}

void SFX_Set3dVolume (int vol)
{
   sgVol3D              = vol;
   sgVolsChanged        = TRUE;
}

int SFX_Get3dVolume ()
{
   if (use3DHWmod)
      return sgVol3D + sgVol3DHW; 
   else
      return sgVol3D;
}

void SFX_SetMusicVolume (int vol)
{
   // Turn off music at lowest volume.
   if( vol < -5000 )
   {
      SongUtilSetVolume( vol );
      SongUtilDisableMusic();
   }
   else
   {
      SongUtilEnableMusic();
      SongUtilSetVolume( vol );
   }

   sgVolMusic = vol;
}

int SFX_GetMusicVolume ()
{
   return ( SongUtilGetVolume() );
}

// if you are need of this, say, to do some group volume stuff
ISndMixer *SFX_GetMixer(void)
{
   if (pSndMixer)
   {
      pSndMixer->AddRef();
      return pSndMixer;
   }    
   else
      return NULL;
}

int SFX_GetNumChannels ()
{
   return sfx_use_channels;
}

///////////////////////////////////////

void SFX_SetReverseStereo(BOOL state)
{
   rev_stereo = state;
}

BOOL SFX_StereoReversed()
{
   return rev_stereo;
}

///////////////////////////////////////

void SFX_Slam_Dist(int hnd, int distance, BOOL force_update)
{
   AssertMsg(fxlist[hnd].distance >= 0, "Attempt to distance override on absolute sample");
   fxlist[hnd].distance = distance;
   fxlist[hnd].parm.blocking_factor = 0.0;

   if (force_update && fxlist[hnd].pSnd)
      _sfx_update(&fxlist[hnd],TRUE,tm_get_millisec_unrecorded());
}

///////////////////////////////////////

void SFX_Slam_Dist_BF(int hnd, int distance, mxs_real blocking_factor, BOOL force_update)
{
   AssertMsg(fxlist[hnd].distance >= 0, "Attempt to distance override on absolute sample");
   fxlist[hnd].distance = distance;
   fxlist[hnd].parm.blocking_factor = blocking_factor;

   if (force_update && fxlist[hnd].pSnd)
      _sfx_update(&fxlist[hnd],TRUE,tm_get_millisec_unrecorded());
}

///////////////////////////////////////

// This not only sets the mode between game and metagame, but pauses
// or clobbers sounds, as appropriate.
void SFX_SetMode(sfx_mode mode)
{
   int old_index = base_index;
   int i;

   FinishAsynchOps();
   switch (mode)
   {
      // We're switching from the metagame to the game.
      case SFX_mode_game:
      {
         for (i = 0; i < SFX_MAX_MODE_COUNT; ++i)
         {
            _sfx *fx=&fxlist[i + SFX_GAME_BASE_INDEX];
            if ((fx->type != SFX_NONE)&&(fx->pSnd))
               if ((appsfxModeCallback==NULL) || (*appsfxModeCallback)(mode,i,&fx->parm))
                  fx->pSnd->Resume();
            _sfx *mfx=&fxlist[i + SFX_METAGAME_BASE_INDEX];
            if ((mfx->type != SFX_NONE)&&(mfx->pSnd))
               _sfx_free(mfx, TRUE);
         }
         base_index = SFX_GAME_BASE_INDEX;
         break;
      }

      // We're switching from the game to the metagame.
      case SFX_mode_metagame:
      {
         for (i = 0; i < SFX_MAX_MODE_COUNT; ++i)
         {
            _sfx *fx=&fxlist[i + SFX_GAME_BASE_INDEX];
            if ((fx->type != SFX_NONE)&&(fx->pSnd))
               if ((appsfxModeCallback==NULL) || (*appsfxModeCallback)(mode,i,&fx->parm))
                  fx->pSnd->Pause();
         }
         base_index = SFX_METAGAME_BASE_INDEX;
         break;
      }
   }
   snd_mprintf(("SFX_SetMode: setting mode to  %s\n", (mode == SFX_mode_game)? "GAME" : "METAGAME"));
#ifndef SHIP
   if (old_index == base_index)
      Warning(("SFX_SetMode: setting mode to same as before: %s\n",
               (mode == SFX_mode_game)? "GAME" : "METAGAME"));
#endif // ~SHIP
}


///////////////////////////////////////

EXTERN void SFX_KillAll(sfx_mode mode)
{
   in_kill = TRUE;
   
   if (pSndMixer&&mode==SFX_mode_all)
   {
      pSndMixer->StopAllSamples();
      pSndMixer->Update();      // trigger those callbacks
      for (int i = 0; i < SFX_MAX_COUNT; i++)
         if (fxlist[i].type != SFX_NONE)
            _sfx_free(&fxlist[i], TRUE);
   }
   else
   {
      int base=(mode==SFX_mode_metagame)?SFX_METAGAME_BASE_INDEX:SFX_GAME_BASE_INDEX;
      for (int i=0; i<SFX_MAX_MODE_COUNT; i++)
      {
         _sfx *fx=&fxlist[i + base];
         if (fx->type!=SFX_NONE)
            _sfx_free(fx,TRUE);
      }
#ifdef MPRINT_TRACK
      int nbase=(mode!=SFX_mode_metagame)?SFX_METAGAME_BASE_INDEX:SFX_GAME_BASE_INDEX;
      for (int ni=0; ni<SFX_MAX_MODE_COUNT; ni++)
         if (fxlist[ni + nbase].type!=SFX_NONE)
            mprintf("Still using %d\n",ni+nbase);
#endif
   }

   in_kill = FALSE;
}

///////////////////////////////////////


int SFX_Channel_Count()
{
   int i, cnt;
   for (i=0, cnt=0; i<SFX_MAX_COUNT; i++)
      if (fxlist[i].type!=SFX_NONE)
         cnt++;
   //mprintf("appsfx %d channels\n",cnt);

   // Hack: add music channel.
   cnt++;

   return cnt;
}

///////////////////////////////////////
// lets try and preload a sound

BOOL SFX_PreLoad(const char *pName)
{
   IRes *pRes;
   AutoAppIPtr(ResMan);   
   pRes = pResMan->Bind(pName, RESTYPE_SOUND, pSoundPath);
   if (pRes)
   {
      pRes->Lock();
      pRes->Unlock();
      pRes->Release();
      return TRUE;
   }
   return FALSE;
}

//////////////////////
// hack for "frame rate attention paying horror"
// if frame rate is low, pretend we are using more channels

BOOL SFX_IsChannelAvail()
{
#ifdef DYNAMIC_CHANNELS
   int modifier=0;  // how many "channels" is our low frame rate worth
   if (sfx_use_channels>4)
   {
      int avg_frame=(sfx_time_total>>FRAME_HISTORY_SHIFT);
      if (avg_frame>IDEAL_MIN_FRAME_MS)
      {
         int pain=1000/avg_frame;
         modifier=DYNAMIC_MODIFIER(pain);
//         mprintf("%1.1d",modifier);
      }
   }
   return (SFX_Channel_Count() + modifier < sfx_use_channels);   
#else   
   return (SFX_Channel_Count() < sfx_use_channels);
#endif   
}

///////////////////////////////////////
// app side creation/destruction of samples

int SFX_Play_Raw(int type, sfx_parm *parm, char *sfx_name)
{
   if (in_kill)
      return SFX_NO_HND;

   int hnd=SFX_Get();
   if (hnd!=SFX_NO_HND)
   {
      _sfx *newfx=&fxlist[hnd];
      _sfx_basic_setup(SFX_BF_RAW,newfx,type,parm,sfx_name);
      if (!_sfx_start_play(newfx,FALSE)) {
         TLOG1( "SFX_Play_Raw [%d] - play failed", hnd );
         hnd=SFX_NO_HND;
      }
   }
   return hnd;
}

int SFX_Play_Obj(int type, sfx_parm *parm, char *sfx_name, int objid)
{
   int hnd=SFX_Get();
#ifndef SHIP
   if (!ObjExists(objid))
   {
      mprintf("PlayObj: called with %d, an invalid obj (sfxtype %d)\n",objid,type);
      return SFX_NO_HND;
   }
#endif
   if (hnd!=SFX_NO_HND)
   {
      _sfx *newfx=&fxlist[hnd];
      _sfx_basic_setup(SFX_BF_OBJ,newfx,type,parm,sfx_name);
      newfx->attach=objid;
      if (!_sfx_start_play(newfx,FALSE)) {
         TLOG1( "SFX_Play_Obj [%d] - play failed", hnd );
         hnd=SFX_NO_HND;
      }
   }
#ifdef STRESS_ASYNCH
   static int harold=0;
   if (((harold++)&0x3)==0)
      SFX_Play_Obj(type,NULL,sfx_name,objid);
#endif   
   return hnd;
}

int SFX_Play_Vec(int type, sfx_parm *parm, char *sfx_name, mxs_vector *vector)
{
   int hnd=SFX_Get();
   if (hnd!=SFX_NO_HND)
   {
      _sfx *newfx=&fxlist[hnd];
      _sfx_basic_setup(SFX_BF_VEC,newfx,type,parm,sfx_name);
      mx_copy_vec(&newfx->loc,vector);
      newfx->attach=0;
      if (!_sfx_start_play(newfx,FALSE)) {
         TLOG1( "SFX_Play_Vec [%d] - play failed", hnd );
         hnd=SFX_NO_HND;
      }
   }
   return hnd;
}

// kill off fxlist[hnd]
void SFX_Kill_Hnd(int hnd)
{
   _sfx_free(&fxlist[hnd], TRUE);
}

// kill all sounds which refer to this object
void SFX_Kill_Obj(int objid)
{
   int i;

   for (i=0; i<SFX_MAX_COUNT; i++)
      if ((fxlist[i].attach==objid) && (fxlist[i].type != SFX_NONE))
         SFX_Kill_Hnd(i);
}

// callback invoked when a play command fails to get a channel
//   return TRUE if app has freed a channel, FALSE if not
static BOOL
_sfx_priority_callback( ISndMixer    *pMixer,
                        ISndSample   *pSample,
                        void         *pCBData )
{
   _sfx  *pSFX;

   pSFX = (_sfx *) pSample->GetData();
   // signal that this sample couldn't be played
   pSFX->flags &= ~SFXFLG_HAPPY;

   snd_mprintf(("priority unhappy\n"));

   TLOG1( "failed to alloc chan to [%d]", pSFX->ID );

   // TBD: put in a priority scheme which kills off a low priority sound
   //   if this is a high priority sound - return TRUE in that case
   return FALSE;
}

///////////////////////
// EAX control

void SFX_Enable_EAX (void)
{
   // Test for null and initialize.
   if (NULL != pSndMixer)
   {
      pSndMixer->Init3DReverb();

      if (pSndMixer->Have3DReverb())
      {
         // Force current room to initialize reverb if in game mode.
         sgLastRoomType = -1;

         // Set to generic 50%.  This makes the options panel's left/right thing
         // sound cool.
         ReverbSettings revSet;
         revSet.flags = kREVERB_FlagType | kREVERB_FlagLevel;
         revSet.type = kREVERB_Generic;
         revSet.level = 0.5;
         pSndMixer->Set3DReverbSettings(&revSet);
      }
   }
}

void SFX_Disable_EAX (void)
{
   // Test for null and shutdown.
   if (NULL != pSndMixer)
      pSndMixer->Shutdown3DReverb();
}

BOOL SFX_Is_EAX_Enabled (void)
{
   // Test for null and query.
   if (NULL != pSndMixer)
      return ( pSndMixer->Have3DReverb() );

   // If no mixer, we don't have EAX.
   return FALSE;
}

BOOL SFX_Is_EAX_Available (void)
{
   // Test for null and query.
   if (NULL != pSndMixer)
      return ( pSndMixer->CanDo3DReverb() );

   // If no mixer, we can't do EAX.
   return FALSE;
}

BOOL SFX_Use_Occlusion( int sfxHandle )
{
   _sfx *pSFX = &(fxlist[sfxHandle]);

   if ( (pSndMixer != NULL) && (pSFX != NULL) ) {
      return (pSFX->flags & SFXFLG_POS) && (sfx3DMethod == kSnd3DMethodHardware) && pSndMixer->Have3DOcclusion();
   }

   return FALSE;
}


// what follows are utility functions for the editor
// basically the command interpreter, and the status dump/state check code
#ifndef SHIP
void SFX_Dump_Single(_sfx *fx)
{
   mprintf("Effect %x: type %d (flags %x): (pRes %x) Name %s\n",
           _sfx_get_hnd_from_ptr(fx),fx->type,fx->flags,fx->pRes,
           (fx->pRes==NULL)?"NoName":fx->pRes->GetName());
   mprintf("  Snd %x, attach %x, location %g %g %g\n",fx->pSnd,fx->attach,
           fx->loc.x,fx->loc.y,fx->loc.z);
   mprintf("  [%s %s] Parm pan %d gain %d (misc %d %d %d)\n",
           (fx->flags&SFXFLG_IS3D)?"Our3d":"Our2d",
           fx->pSnd==NULL?"NoSnd":
              fx->pSnd->Get3DMethod()==kSnd3DMethodHardware?"L3dHW":
              fx->pSnd->Get3DMethod()==kSnd3DMethodSoftware?"L3dSW":"Lib2d",
           fx->parm.pan,fx->parm.gain,fx->parm.flag,fx->parm.pri,fx->parm.group);
}

void SFX_Quick_List(BOOL dump_all)
{
   int i;
   for (i=0; i<SFX_MAX_COUNT; i++)
      if (dump_all||fxlist[i].type!=SFX_NONE)
      {
         _sfx *fx=&fxlist[i];
         mprintf("sfx %x: %s (res %x): gainpan %d %d [%d %x]\n",_sfx_get_hnd_from_ptr(fx),
                 (fx->pRes==NULL)?"NoName":fx->pRes->GetName(),fx->pRes,
                 fx->parm.pan,fx->parm.gain,fx->type,fx->flags);
      }
}

void SFX_Dump_List(BOOL dump_all)
{
   int i;
   for (i=0; i<SFX_MAX_COUNT; i++)
      if (dump_all||fxlist[i].type!=SFX_NONE)
         SFX_Dump_Single(&fxlist[i]);
}

#ifdef APPSFX_TIMINGS
void SFX_Dump_Timings(void)
{
   mprintf("Dumping and Clearing timing info for Sound\n");
   TIMER_OutputResult(StartPlayTimer);
   TIMER_OutputResult(SndMixerTimer);
   TIMER_OutputResult(SfxUpdateTimer);
   TIMER_Clear(StartPlayTimer);
   TIMER_Clear(SndMixerTimer);
   TIMER_Clear(SfxUpdateTimer);
}
#endif

void SFX_command(char *sfx_name)
{
   if (strnicmp(sfx_name,"show",4)==0)
      SFX_Dump_List((bool)(stricmp(sfx_name,"show_all")==0));
   else if (strnicmp(sfx_name,"quick_show",10)==0)
      SFX_Quick_List(FALSE);
   else if (strnicmp(sfx_name,"refresh",7)==0)
      SFX_Refresh_Config();
#ifdef APPSFX_TIMINGS
   else if (strnicmp(sfx_name,"timings",7)==0)
      SFX_Dump_Timings();
#endif
   else if (stricmp(sfx_name, "startup")==0)
      SFXInit();
   else if (stricmp(sfx_name, "shutdown")==0)
      SFXClose();
   else
      SFX_Play_Raw(SFX_STATIC, NULL, sfx_name);
}
#endif // SHIP
