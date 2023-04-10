/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/appsfx.h,v 1.33 2000/01/29 13:41:42 adurant Exp $
#pragma once

#ifndef __APPSFX_H
#define __APPSFX_H

#include <playlist.h>
#include <matrixs.h>

// says "this parm isnt being set in this struct"
#define SFX_DEF_PARM (-1)

// prototype parms for sfx
typedef struct {
   int    pan;       // using standard sndlib pan (-10k to 10k)
   uint   flag;      // flag overrides, not all are app setable
   ushort dummy;     // a couple of bytes for good alignment
   uchar  pri;       // priority for the sound
   uchar  group;     // what group the sound effect goes into
   int    gain;      // currently in snd.lib decibel format, maybe not a good idea
   int    delay;     // initial delay
   ulong  fade;      // fade in and out time
   int    radius;    // forced sound radius
   int    distance;  // distance to sound origination point (-1 == straight line)
   void (*end_callback)(int hSound, void *user_data);  // when sample ends
   void (*loop_callback)(int hSound, void *user_data); // when each loop ends
   void  *user_data; // for callbacks
   ISndSource *pSndSource; // for identifying our sound in the loop callback
   int    num_loops; // 
   mxs_real blocking_factor;  // sound attenuation due to doors (0.0 is no attenuation, 1.0 is full)
} sfx_parm;          //    since we really want local vol overrides? who knows

#define SFX_LOOP_INFINITE 0

#define SFX_MAX_MODE_COUNT 16
#define SFX_MAX_COUNT (SFX_MAX_MODE_COUNT * 2)
#define SFX_CHANNELS  8

#define SFX_NO_HND   (-1)

// system mode, for transitions between sim & metagame
typedef enum sfx_mode {
   SFX_mode_metagame = 65536,
   SFX_mode_game,
   SFX_mode_all,
} sfx_mode;

// types of sfx
#define SFX_NONE      0
#define SFX_STATIC    1
#define SFX_3D        2

// some flags can be set by the app, others are internal only
// app sfx flags
#define SFXFLG_LOOP        (1<<0)  // does the sample loop
#define SFXFLG_STREAM      (1<<5)  // this sample is a stream, not all in memory
#define SFXFLG_QSOUND      (1<<7)  // really a qsounded thing, deal with it
#define SFXFLG_PHYS        (1<<8)  // consider speed of object in sound calc
#define SFXFLG_NOCACHE     (1<<11) // dont cache, drop right after use
#define SFXFLG_SHARP       (1<<12) // sharp attenuation curve
#define SFXFLG_ASYNCH      (1<<13) // does one try and asynch load me
#define SFXFLG_NET_AMB     (1<<15) // network this ambient sound
#define SFXFLG_NO_NET      (1<<16) // don't network this spatial sound
#define SFXFLG_APP_MASK \
   (SFXFLG_LOOP|SFXFLG_STREAM|SFXFLG_QSOUND|SFXFLG_PHYS|SFXFLG_NOCACHE|SFXFLG_SHARP|SFXFLG_ASYNCH|SFXFLG_NET_AMB|SFXFLG_NO_NET)

// internal only flags
#define SFXFLG_STATIC      (1<<1)  // if static, never update pan/gain
#define SFXFLG_POS         (1<<2)  // do pain/gan positional update
#define SFXFLG_OBJ         (1<<3)  // reload location out of object
#define SFXFLG_DELAY       (1<<4)  // delay start of sample

#define SFXFLG_HAPPY       (1<<9)  // happy or not, ie. did i play
#define SFXFLG_IS3D        (1<<10) // did i ask for 3d hardware for this or not?
#define SFXFLG_ASYNCHWAIT  (1<<14) // am i waiting for the asynch load to finish?
#define SFXFLG_NOCALLBACK  (1<<17) // dont make the callback

//   valid type/so ons are
// _Raw, STATIC - just play the darn effect, right here, right now
// _Obj, STATIC - just play it, but if obj goes away, kill it
// _Obj, 3D - play it, compute pan and gain continually, based on object pos
// _Vec, STATIC - compute pan/gain once for vec, the play it there??? if useful
// _Vec, 3D - play it, compute pan and gain continually, based on src vec

//   the parm->flag field can be used to make a given sample loop, or tell it
// that it is really a data stream, and so on.  this is somewhat inconvienient
// as you have to get parm struct, default out all the other fields, and then
// set flags.  maybe we should just have an int flags you pass in to all the
// SFX_Play calls?

//////////////////////////////
// ProtoTypes

// setup functions
EXTERN void SFXClose(void);
EXTERN BOOL SFXInit(void);
EXTERN BOOL SFXReset(void);

#define SFXDEVICE_NoSound      (0)
#define SFXDEVICE_Software     (1)
#define SFXDEVICE_A3D          (2)
#define SFXDEVICE_QMIXER       (3)

EXTERN BOOL SFX_SetSoundDevice(int device, BOOL reset);
EXTERN int  SFX_GetSoundDevice();

// pass in an sfx_mode
EXTERN void SFX_SetMode(sfx_mode mode);

// if you want to set a mode callback
// return TRUE to mean "go ahead and deal with it (ie. auto pause-resume)
// else FALSE to mean "let me deal with it"/"i dealt with it"/etc
EXTERN BOOL (*appsfxModeCallback)(sfx_mode mode, int handle, sfx_parm *parm);

// stop all sounds w/out making callbacks
EXTERN void SFX_KillAll(sfx_mode mode);

// TRUE iff we are between SFX init and close 
EXTERN BOOL SFXActive(void);

// Test for free physical channel, or get count
EXTERN int  SFX_Channel_Count(void);
EXTERN BOOL SFX_IsChannelAvail(void);

// the play functions
EXTERN BOOL SFX_PreLoad(const char *pName);

//  *parm can always be NULL to default, or can be filled in by APP
EXTERN int SFX_Play_Raw(int type, sfx_parm *parm, char *sfx_name);
EXTERN int SFX_Play_Obj(int type, sfx_parm *parm, char *sfx_name, int objid);
EXTERN int SFX_Play_Vec(int type, sfx_parm *parm, char *sfx_name, mxs_vector *vector);

// Set stereo reversal
EXTERN void SFX_SetReverseStereo(BOOL state);
EXTERN BOOL SFX_StereoReversed();

// Master volume
EXTERN void SFX_SetMasterVolume(int vol);
EXTERN int SFX_GetMasterVolume();
EXTERN int SFX_GetNumChannels ();

// Volumes.
EXTERN void SFX_Set2dVolume (int vol);
EXTERN int  SFX_Get2dVolume ();
EXTERN void SFX_Set3dVolume (int vol);
EXTERN int  SFX_Get3dVolume ();
EXTERN void SFX_SetMusicVolume (int vol);
EXTERN int  SFX_GetMusicVolume ();

// Distance override
EXTERN void SFX_Slam_Dist(int hnd, int distance, BOOL force_update);
// Distance override with blocking factor
EXTERN void SFX_Slam_Dist_BF(int hnd, int distance, mxs_real blocking_factor, BOOL force_update);


// Attenuation stuff
EXTERN int SFX_MaxDist(int gain);
EXTERN int SFX_Attenuate(int master_gain, int dist);

// removing particular effects
EXTERN void SFX_Kill_Hnd(int hnd);
EXTERN void SFX_Kill_Obj(int objid);

// frame update call me often sort of thing
EXTERN void SFX_Frame(mxs_vector *pos, mxs_angvec *ang);

// calculate attenuate volume at a distance
EXTERN int SFX_Attenuate(int master_gain, int dist);

// name of resource for sound
EXTERN const char *SFX_Name(int hnd);
// parm struct for sound
EXTERN const sfx_parm *SFX_Parm(int hnd);

#ifndef SHIP
EXTERN void SFX_command(char *sfx_name);
EXTERN void SFX_Dump_List(BOOL dump_all);
EXTERN void SFX_Quick_List(BOOL dump_all);
#endif

///////////////////////
// EAX control

EXTERN void SFX_Enable_EAX (void);
EXTERN void SFX_Disable_EAX (void);
EXTERN BOOL SFX_Is_EAX_Enabled (void);
EXTERN BOOL SFX_Is_EAX_Available (void);
// TRUE IFF we are using EAX 2+ occlusion for doors and such
EXTERN BOOL SFX_Use_Occlusion( int sfxHandle );

#endif //  __APPSFX_H
