/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/oldasfx.c,v 1.47 2000/02/19 12:14:26 toml Exp $
// remedial sound system for DromEd/Dark/AIR/So on

#include <math.h>

#include <timer.h>

#include <matrix.h>
#include <lgsound.h>
#include <config.h>
#include <mprintf.h>

#include <lresname.h>
#include <lress.h>

#include <appsfx.h>
#include <wrtype.h>
#include <objpos.h>
#include <auxsnd.h>

#include <osysbase.h>
#include <timelog.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//#define MIXER_COUNTS
//#define MPRINT_TRACK

///////////////////////////////////////
// data structures, static globals

typedef struct {
   uchar  type;      // sound effect type, set by user/app
   char   ID;        // location in fxlist
   ushort flags;     // for quick determination of what to do in updates
   ObjID  attach;    // object attached to
   mxs_vector loc;   // location for non-attached sounds, or bounce loc
   ISndSample *snd;  // actual sound effect we are playing
   rn_hnd rhnd;      // handle to resource being used
   char  *name;      // for streaming sounds, i guess, for now, maybe
   int master_gain;  // master volume for the sample, as opposed to attenuated/cur
   int distance;     // distance to sound origination point (-1 == stright line)
   sfx_parm parm;    // current parameters locally for the sample
   ulong  play_time; // when did we first get sample
} _sfx;              // maybe name should be for all, extension generates type

static _sfx fxlist[SFX_MAX_COUNT];
static ISndMixer *sfxMixer=NULL;
static sfx_parm def_parms={0,0,0,0,-1,0,0,0,-1,NULL,NULL};
static float  attenuation_factor = 55;
static float  pan_factor = 0.3;
static sfx_use_channels = SFX_CHANNELS;
static eSnd3DMethod sfx3DMethod = kSnd3DMethodSoftware;
static int max_simul_hardware_3d_snds = 4;  // @TODO: pick which ones we use less idiotically

#define _sfx_get_hnd_from_ptr(fx) (fx-fxlist)

// some forward declarations of the internal functions
static BOOL _sfx_update(_sfx *fx, BOOL init, ulong cur_time);
static void _sfx_free(_sfx *fx, BOOL makeCallback);

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
 
BOOL
_sfx_priority_callback( ISndMixer    *pMixer,
                        ISndSample   *pSample,
                        void         *pCBData );

#ifdef DBG_ON
void DbgCheckInUseChannels(void);  // check to make sure we are all cleaned up
#else
#define DbgCheckInUseChannels()
#endif

///////////////////////////////////////
// startup/shutdown

void SFXClose(void)
{
   if (sfxMixer)
   {
      int i;
      ISndMixer_StopAllSamples(sfxMixer);
      ISndMixer_Update(sfxMixer);      // trigger those callbacks
      for (i=0; i<SFX_MAX_COUNT; i++)
         if (fxlist[i].type!=SFX_NONE)
            _sfx_free(&fxlist[i], TRUE);
      TIMELOG_DUMP( "sndlog.txt" );
      ISndMixer_Release(sfxMixer);
   }
   DbgCheckInUseChannels();
   sfxMixer=NULL;
}

static BOOL rev_stereo = FALSE;

BOOL SFXInit(void)
{
   sSndAttribs attribs;
   int i, mixer_rate=22050;
   BOOL createOk;
   sSndEnvironment snd3DEnv;
   char methodName[256];

   if (sfxMixer)
      return TRUE; 

   if (config_is_defined("no_sound"))
      return FALSE;

   if (config_is_defined("reverse_stereo"))
      rev_stereo = TRUE;

   for (i=0; i<SFX_MAX_COUNT; i++)
    { fxlist[i].type=SFX_NONE; fxlist[i].snd=NULL; fxlist[i].ID = i;}

   config_get_int("mixer_rate", &mixer_rate);
   
   // generic parms for mixer
   attribs.dataType = kSndDataPCM;  // PCM data
   attribs.sampleRate = mixer_rate;      // 22kHz
   attribs.nChannels = 2;           // Stereo

   // the following have no effect in Init
   attribs.bytesPerBlock = 0;
   attribs.samplesPerBlock = 0;
   attribs.numSamples = 0;
   
   if (config_is_defined("force_8_bit_sound"))
      attribs.bitsPerSample = 8;
   else
      attribs.bitsPerSample = 16;

   config_get_float("attenuation_factor", &attenuation_factor);
   config_get_float("pan_factor", &pan_factor);
   config_get_int("sfx_channels", &sfx_use_channels);
   config_get_int("max_sfx_3d", &max_simul_hardware_3d_snds);

   //
   // pick 3D sound method - default is dark's built-in pan/vol
   //
   sfx3DMethod = kSnd3DMethodSoftware;
   config_get_raw( "snd3d", methodName, sizeof(methodName) );
   if ( !strcmpi( methodName, "qsound" ) ) {
      createOk = SndCreateQSMixer(&sfxMixer,NULL);
   } else if ( !strcmpi( methodName, "a3d" ) ) {
      sfx3DMethod = kSnd3DMethodHardware;
      createOk = SndCreateA3DMixer(&sfxMixer,NULL);
   } else {
      createOk = SndCreateMixer(&sfxMixer,NULL);
      if ( !strcmpi( methodName, "panvol" ) ) {
         sfx3DMethod = kSnd3DMethodPanVol;
      } else if ( strcmpi( methodName, "ds" ) ) {
         sfx3DMethod = kSnd3DMethodNone;
      }
   }

   if ( createOk ) {
      TIMELOG_INIT( 16384 );
      ISndMixer_Set3DMethod( sfxMixer, sfx3DMethod );
      if (ISndMixer_Init(sfxMixer,NULL,sfx_use_channels,&attribs)==kSndOk) {
         ISndMixer_RegisterPriorityCallback( sfxMixer, _sfx_priority_callback, NULL );
         if ( sfx3DMethod != kSnd3DMethodNone ) {
            snd3DEnv.dopplerFactor = 1.0;
            // The sound library does meters, dark does feet
            snd3DEnv.distanceFactor = 0.3048;
            snd3DEnv.rolloffFactor = 0.3;
            config_get_float( "snd3d_distance", &(snd3DEnv.distanceFactor) );
            config_get_float( "snd3d_rolloff", &(snd3DEnv.rolloffFactor) );
            ISndMixer_Set3DEnvironment( sfxMixer, &snd3DEnv );
         }
         return TRUE;      // seems to have worked
      }
   }
   sfxMixer=NULL;
   SFXClose();
   Warning(("Sound failed to correctly init\n"));
   return FALSE;
}

BOOL SFXActive(void)
{
   return sfxMixer != NULL; 
}

#ifdef THIS_WAS_SUPPORTED
// may have to redo this with a hard Close, then custom Init, i guess
bool SFXMixerConfig(void)
#endif

///////////////////////////////////////
// internal creation/destruction of samples

static int hndInUse = 0;
static int chanInUse = 0;
static int cur3dSndCnt = 0;

int SFX_Get(void)
{
   int i;
   if (sfxMixer!=NULL)  // lets not "get" effects if we have no mixer
      for (i=0; i<SFX_MAX_COUNT; i++)
         if (fxlist[i].type==SFX_NONE) {
            hndInUse++;
            return i;
         }
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
   newfx->snd=NULL;
   newfx->rhnd=NO_HND;
   newfx->name=sfx_name;
   // real issue is now setting type and flags correctly
   newfx->type=type;  // easy for now, eh?
   newfx->master_gain=newfx->parm.gain;
   newfx->distance=newfx->parm.distance;
   set_flags(newfx,which,newfx->parm.flag);
   newfx->play_time = tm_get_millisec_unrecorded();
   if (newfx->parm.delay>0)
      newfx->flags |= SFXFLG_DELAY;
}

static void _sfx_cleanup_fx(_sfx *fx, BOOL make_callback)
{
#ifdef MPRINT_TRACK
   mprintf("Cleanup %d: type %d rhnd %d snd %x name %s (%d %x %d)\n",
           _sfx_get_hnd_from_ptr(fx),fx->type,fx->rhnd,fx->snd,fx->name,
           make_callback&&fx->parm.callback,fx->ID,fx->parm.user_data);
#endif   
   if (fx->rhnd!=LRES_NO_HND)
   {
      ResNameFreeHandle(fx->rhnd);
      fx->rhnd=LRES_NO_HND;  // safety code, so we dont accidentally reuse
   }
   fx->type=SFX_NONE;
   fx->snd=NULL;
   if (fx->flags&SFXFLG_IS3D)
   {
      if (cur3dSndCnt<=0) Warning(("3d Sound Underflow in cleanup %d\n",_sfx_get_hnd_from_ptr(fx)));
      cur3dSndCnt--;
   }
   if (make_callback && fx->parm.end_callback)
      fx->parm.end_callback(fx->ID, fx->parm.user_data);
   hndInUse--;
}

// called when a sound stops
static void _sfx_end_callback(ISndSample *pSample, void *pCBD)
{
   _sfx *fx = (_sfx *) pCBD;

   if (fx)
      _sfx_cleanup_fx(fx, TRUE);
   ISndSample_Release(pSample);
   chanInUse--;
}

// This is called at the end of each iteration of a looped sample,
// including the last, shortly _before_ that iteration ends.
static void _sfx_loop_callback(ISndSource *pSndSource, void *pCBD)
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
      Warning(("Trying to free a non-alloced sample %d\n",
               _sfx_get_hnd_from_ptr(fx)));
      return;
   }
   if (fx->snd)
   {
      ISndSample_Stop(fx->snd);
   }
   else
      _sfx_cleanup_fx(fx, makeCallback);
}

// actually goes and gets the ISndSample, fills in the real data
// for now, it also looks up the resource, cause why not
static bool _sfx_start_play(_sfx *fx, BOOL after_delay)
{
   char *tmpname=fx->name;
   
   // if we're delaying, then skip start
   if (fx->flags & SFXFLG_DELAY)
      return TRUE;

//   fx->name=NULL;     // so we dont mistakenly refer to it later
   if ((fx->rhnd=ResNameLoad("snd", tmpname, RN_SND, NULL)) != LRES_NO_HND)
   {
      // Do we want callbacks for looping?
      if (fx->parm.flag & SFXFLG_LOOP) {
         fx->snd = CreateMemSoundSourceLooped(sfxMixer,
                                              ResNameGetbyHandle(fx->rhnd),
                                              ResNameGetSize(fx->rhnd),
                                              _sfx_end_callback, 
                                              fx,
                                              &fx->parm.pSndSource,
                                              fx->parm.num_loops,
                                              _sfx_loop_callback, 
                                              fx);
      } else {
         fx->snd = CreateMemSoundSource(sfxMixer,
                                        ResNameGetbyHandle(fx->rhnd),
                                        ResNameGetSize(fx->rhnd),
                                        _sfx_end_callback,
                                        fx);
      }

      if (fx->snd)
      {
         NAME_SAMPLE( fx->snd, fx->name );
         if ( (fx->flags & SFXFLG_POS) && (sfx3DMethod != kSnd3DMethodNone) )
         {  // if we want 3D, make sure we arent using too many hardware channels
            eSnd3DMethod useMethod=sfx3DMethod;
            if (sfx3DMethod==kSnd3DMethodHardware)
               if (cur3dSndCnt < max_simul_hardware_3d_snds)
               {
                  cur3dSndCnt++;
                  fx->flags |= SFXFLG_IS3D;
               }                  
               else
               {               // can i do this, who knows, argh, hate hate
                  useMethod=kSnd3DMethodPanVol;  // is software==panvol
                  Warning(("Out of channels for 3d sounds"));
               }
            ISndSample_Set3DMethod( fx->snd, useMethod );
            ISndSample_Set3DMode( fx->snd, kSnd3DModeNormal );
         }
         _sfx_update(fx,TRUE,tm_get_millisec_unrecorded());
         fx->flags |= SFXFLG_HAPPY;
         // give sample back ptr to us so it can clear happy if play fails
         ISndSample_SetData( fx->snd, (long) fx );
         ISndSample_Play(fx->snd);
         // if the play fails, the stop callback will be done anyway, which will
         //   decrement our channel count...
         chanInUse++;

         if ( fx->flags&SFXFLG_HAPPY )
         {
#ifdef MPRINT_TRACK
            mprintf("Play %d: type %d rhnd %d snd %x name %s\n",
                    _sfx_get_hnd_from_ptr(fx),fx->type,fx->rhnd,fx->snd,fx->name);
#endif   
            return TRUE;
         }
         else
            Warning(("_sfx_start_play: couldnt play sound sample for %s\n",tmpname));
      } 
      else
         Warning(("_sfx_start_play: couldnt create sound sample for %s\n",tmpname));
   }
   else
      Warning(("_sfx_start_play: couldnt ResNameLoad %s\n",tmpname));
   TLOG1( "failed to alloc chan to [%d]", fx->ID );
   _sfx_free(fx,after_delay);
   return FALSE;
}

int SFX_Attenuate(int master_gain, int dist)
{

   if ( sfx3DMethod == kSnd3DMethodNone ) {
      return master_gain - (dist * attenuation_factor);
   } else {
      TLOG2( "attenuation %d at distance %d", master_gain, ISndMixer_Get3DDistanceVolume( sfxMixer, dist ) );
      return master_gain + ISndMixer_Get3DDistanceVolume( sfxMixer, dist );
   }
}

int SFX_MaxDist(int gain)
{
   int max_dist;

   if ( sfx3DMethod == kSnd3DMethodNone ) {
      max_dist = (int)((5000 + gain) / attenuation_factor);
   } else {
      max_dist = (int) ISndMixer_Get3DDistanceFromVolume( sfxMixer, -5000 );
      max_dist = (int)((5000 + gain) / attenuation_factor);
   }

   TLOG2( "Max dist %d for gain %d", max_dist, gain );
   return max_dist;
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
   // check for delayed sample
   if (fx->flags&SFXFLG_DELAY)
      if (cur_time>=fx->play_time+fx->parm.delay)
      {
         // hey, start the sample
         fx->flags = fx->flags&~SFXFLG_DELAY;
         if (!_sfx_start_play(fx,TRUE))
            return FALSE;
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
         ulong remaining_time = ISndSample_PlayTimeRemaining(fx->snd);
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
         if ( sfx3DMethod == kSnd3DMethodNone ) {
            int gain;
            float dist = fx->distance;

            if (dist < 0)
               dist = mx_dist_vec(&head_pos,&fx->loc);

            // gain check first
            gain = SFX_Attenuate(master_gain, (int)dist);
            if (gain!=fx->parm.gain)
            {
               fx->parm.gain=gain;
               change=TRUE;
            }

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
          } else {
            sndPos.x = fx->loc.x;
            sndPos.y = fx->loc.y;
            sndPos.z = fx->loc.z;
            ISndSample_Set3DPosition(fx->snd, &sndPos );
         }
      }
   }
   else
      if (fade)
      {
         fx->parm.gain = master_gain;
         change = TRUE;
      }
      

   if (change||init)
   {
      ISndSample_SetVolume(fx->snd,fx->parm.gain);
      if ( sfx3DMethod == kSnd3DMethodNone ) {
         ISndSample_SetPan(fx->snd,fx->parm.pan);
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
   BOOL init;
   int i;
   mxs_matrix orient;

#ifdef MIXER_COUNTS
   sfx_total-=sfx_cnts[sfx_ptr];
   sfx_remixes-=sfx_remix[sfx_ptr];
   sfx_cnts[sfx_ptr]=0;
#endif // MIXER_COUNTS

   if (sfxMixer==NULL) return;

   // update our global time 
   cur_time = tm_get_millisec_unrecorded();

   // @TODO: really this is an editor check only
   //  in game mode, we know we have a view position, so we should punt the ifs
   if (pos) head_pos=*pos;
   if (ang) head_ang=*ang;

   if ( sfx3DMethod != kSnd3DMethodNone ) {
      // update listener's position & orientation
      ISndMixer_Set3DPosition(sfxMixer, (sSndVector *) pos);
      mx_ang2mat( &orient, ang );
      ISndMixer_Set3DOrientation( sfxMixer, (sSndVector *) &(orient.vec[0]), (sSndVector *) &(orient.vec[2]) );
   }


   ISndMixer_Update(sfxMixer);

   for (i=0; i<SFX_MAX_COUNT; i++)
      if (fxlist[i].type!=SFX_NONE)
      {
         init = FALSE;
#ifdef MIXER_COUNTS
         if (_sfx_check_delay(&fxlist[i], &init, cur_time))
            sfx_remix[sfx_ptr]+=_sfx_update(&fxlist[i], init, cur_time)?1:0;
         sfx_cnts[sfx_ptr]++;
#else
         if (_sfx_check_delay(&fxlist[i], &init, cur_time))
            _sfx_update(&fxlist[i], init, cur_time);
#endif
      }
#ifdef MIXER_COUNTS
   sfx_total+=sfx_cnts[sfx_ptr];
   sfx_remixes+=sfx_remix[sfx_ptr];
   sfx_ptr=(sfx_ptr+1)&0xff;
   if (sfx_ptr==0)
      mprintf("sfx: Cur %d, avg %g (remix %g)\n", sfx_cnts[(sfx_ptr-1)&0xff], (float)sfx_total/256.0, (float)sfx_remixes/256.0);
#endif
}

///////////////////////////////////////

void SFX_Slam_Dist(int hnd, int distance)
{
   AssertMsg(fxlist[hnd].distance >= 0, "Attempt to distance override on absolute sample");
   fxlist[hnd].distance = distance;
}

///////////////////////////////////////
// app side creation/destruction of samples

int SFX_Play_Raw(int type, sfx_parm *parm, char *sfx_name)
{
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
#ifdef DBG_ON
   if (!ObjExists(objid))
   {
      Warning(("PlayObj: called with %d, an invalid obj (sfxtype %d)\n",objid,type));
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

   pSFX = (_sfx *) ISndSample_GetData( pSample );
   // signal that this sample couldn't be played
   pSFX->flags &= ~SFXFLG_HAPPY;

   TLOG1( "failed to alloc chan to [%d]", pSFX->ID );

   // TBD: put in a priority scheme which kills off a low priority sound
   //   if this is a high priority sound - return TRUE in that case
   return FALSE;
}


// what follows are utility functions for the editor
// basically the command interpreter, and the status dump/state check code
#ifndef SHIP
void SFX_Dump_Single(_sfx *fx)
{
   mprintf("Effect %x: type %d (flags %x): (rhnd %x) Name %s\n",
           _sfx_get_hnd_from_ptr(fx),fx->type,fx->flags,fx->rhnd,
           (fx->rhnd==LRES_NO_HND)?"NoName":(ResNameGetNameData(fx->rhnd))->name);
   mprintf("  Snd %x, attach %x, location %g %g %g\n",fx->snd,fx->attach,
           fx->loc.x,fx->loc.y,fx->loc.z);
   mprintf("  Parm pan %d gain %d (misc %d %d %d)\n",
           fx->parm.pan,fx->parm.gain,fx->parm.flag,fx->parm.pri,fx->parm.group);
}

void SFX_Dump_List(bool dump_all)
{
   int i;
   for (i=0; i<SFX_MAX_COUNT; i++)
      if (dump_all||fxlist[i].type!=SFX_NONE)
         SFX_Dump_Single(&fxlist[i]);
}

int SFX_Channel_Count(bool print)
{
   int i, cnt;
   for (i=0, cnt=0; i<SFX_MAX_COUNT; i++)
      if (fxlist[i].type!=SFX_NONE)
         cnt++;
   //mprintf("appsfx %d channels\n",cnt);
   return cnt;
}

void SFX_command(char *sfx_name)
{
   if (strnicmp(sfx_name,"show",4)==0)
      SFX_Dump_List((bool)(stricmp(sfx_name,"show_all")==0));
   else
      SFX_Play_Raw(SFX_STATIC, NULL, sfx_name);
}
#endif
