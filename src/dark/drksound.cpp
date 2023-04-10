/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drksound.cpp,v 1.22 2000/02/20 15:23:14 patmac Exp $

#include <string.h>

#include <lg.h>
#include <mprintf.h>
#include <config.h>
#include <matrix.h>

#include <str.h>

#include <ctag.h>
#include <ctagset.h>

#include <appsfxi.h>
#include <appsfx.h>

#include <esnd.h>
#include <drksound.h>
#include <drkpwups.h>

#include <physapi.h>
#include <playrobj.h>
#include <simtime.h>
#include <command.h>

#include <songutil.h>

#include <dbmem.h>

////////////////////////////////////////////////////////////
// ENVIRONMENTAL SOUND CALLBACK 
//

static cTag* find_tag_with_key(cTagSet* set,const cStr& key)
{
   if (set == NULL)
      return NULL; 

   for (int i = 0; i < set->Size(); i++)
   {
      const cTag* tag = set->GetEntry(i); 
      if (stricmp(tag->GetType(),key) == 0)
         return (cTag*)tag; 
   }
   return NULL; 
}

// finds the tags for the key and sets the second one alphabetically to 
// "key2" 
static void set_second_tag(cTagSet* s1, cTagSet* s2, const cStr& key)
{
   cTag* t1 = find_tag_with_key(s1,key); 
   cTag* t2 = find_tag_with_key(s2,key); 
   if (t1 && t2)
   {
      cStr key2 = key;
      key2 += "2"; 
      if (strcmp(t1->GetEnumValue(),t2->GetEnumValue()) < 0)
         t2->Set(key2, t2->GetEnumValue());
      else
         t1->Set(key2, t1->GetEnumValue());
   }
}

static void env_sound_CB(sESndEvent* ev)
{
   set_second_tag(ev->tagsets[kObj1MaterialTags],ev->tagsets[kObj2MaterialTags],"Material"); 
   set_second_tag(ev->tagsets[kObj1ClassTags],ev->tagsets[kObj2ClassTags],"WeaponType"); 
   FungusizeSound(ev); 
}

////////////////////////
// DARK SPECIFIC MIXER/CHANNEL VOLUME CONTROL

enum eListenModes
{
   kListenModeNormal,
   kListenModeActive
};

// from schbase, basically, we see
//  NONE, NOISE, SPEECH, AMBIENT, MUSIC, METAUI
#define DARK_SOUND_GROUP_NONE    0
#define DARK_SOUND_GROUP_NOISE   1
#define DARK_SOUND_GROUP_SPEECH  2
#define DARK_SOUND_GROUP_AMBIENT 3
#define DARK_SOUND_GROUP_MUSIC   4
#define DARK_SOUND_GROUP_METAUI  5

#define DARK_SOUND_GROUPS        6

#ifndef SHIP
static BOOL         listenTrack  = FALSE;
       BOOL         listenFade   = TRUE;
#endif

// table of actual values
#define DELTA_IGNORE (0xdeadbeef)

// else this is the volume to set
static int volume_deltas[][DARK_SOUND_GROUPS]=
{
   {DELTA_IGNORE,0,0,                          0,0,DELTA_IGNORE},     // normal
   {DELTA_IGNORE,0,500,                        -400,0,DELTA_IGNORE},  // active
   {DELTA_IGNORE,DELTA_IGNORE,DELTA_IGNORE,    -600,0,DELTA_IGNORE},  // what is this?
};

// volume_offsets are controlled by ambient/music volume sliders and commands
static int volume_offsets[DARK_SOUND_GROUPS] = { 0,0,0,    0,0,0 };

static eListenModes curListenMode=kListenModeNormal;

static int listenFadeMs[]={1000,3000};  // based on target modes

// listening mode set/check code
static void DrkSoundSetListenMode(eListenModes newMode)
{
   ISndMixer *pSndMixer=SFX_GetMixer();

   if (pSndMixer == NULL)
      return;

   curListenMode=newMode;
   int *our_deltas=volume_deltas[curListenMode];
   int volume;
   for (int i=0; i<DARK_SOUND_GROUPS; i++) {
      if (our_deltas[i]!=DELTA_IGNORE) {
         volume = our_deltas[i] + volume_offsets[i];
#if 0
         // clamp volume range
         if ( volume > 0 ) {
            volume = 0;
         }
         if ( volume < kSndMinVolume ) {
            volume = kSndMinVolume;
         }
#endif

#ifndef SHIP
         if (!listenFade)
            pSndMixer->SetGroupVolume( i, volume );
         else
#endif
            pSndMixer->FadeGroupVolume(i, volume, listenFadeMs[curListenMode],kSndFadeNone);
      }
   }
   SafeRelease(pSndMixer);
}

#if 0
static BOOL DrkSoundModeCallback(sfx_mode mode, int handle, sfx_parm *parm)
{
   ISndMixer *pSndMixer=SFX_GetMixer();
   BOOL deal=TRUE;

   if (pSndMixer == NULL)
      return deal;
   int delta=volume_deltas[curListenMode][parm->group] + volume_offsets[parm->group];
   if (delta!=DELTA_IGNORE)
   {
      if (mode==SFX_mode_game)
         delta=-delta;
#ifndef SHIP
         if (!listenFade)
            pSndMixer->SetGroupVolume(parm->group,delta);
         else
#endif
            pSndMixer->FadeGroupVolume(parm->group,delta,listenFadeMs[curListenMode],kSndFadeNone);
      deal=FALSE;
   }
   SafeRelease(pSndMixer);
   return deal;
}
#endif

////////////////////////
// The other sound types

static int          MusicVol     =  0;
static int          MetaUIVol    =  0;

void DrkSoundSetMusicVol(int vol)
{
   ISndMixer *pSndMixer=SFX_GetMixer();
   
   if (pSndMixer == NULL)
      return;
   
   MusicVol=vol;
   pSndMixer->SetGroupVolume(DARK_SOUND_GROUP_MUSIC,MusicVol);
   SafeRelease(pSndMixer);
}

int  DrkSoundGetMusicVol(void)
{
   return MusicVol;
}


void
DarkSoundSetAmbientVolume( int vol )
{
   ISndMixer *pSndMixer = SFX_GetMixer();

   volume_offsets[DARK_SOUND_GROUP_AMBIENT] = vol;
   volume_offsets[DARK_SOUND_GROUP_MUSIC] = vol;
   if ( pSndMixer ) {
      pSndMixer->SetGroupVolume( DARK_SOUND_GROUP_AMBIENT,
                                 volume_deltas[curListenMode][DARK_SOUND_GROUP_AMBIENT]
                                 + volume_offsets[DARK_SOUND_GROUP_AMBIENT] );
      pSndMixer->SetGroupVolume( DARK_SOUND_GROUP_MUSIC,
                                 volume_deltas[curListenMode][DARK_SOUND_GROUP_MUSIC]
                                 + volume_offsets[DARK_SOUND_GROUP_MUSIC] );
   }
   SafeRelease(pSndMixer);
}


int
DarkSoundGetAmbientVolume(void)
{
   return volume_offsets[DARK_SOUND_GROUP_AMBIENT];
}


void DrkSoundSetUIVol(int vol)
{
   ISndMixer *pSndMixer=SFX_GetMixer();

   if (pSndMixer == NULL)
      return;
   
   MetaUIVol=vol;
   pSndMixer->SetGroupVolume(DARK_SOUND_GROUP_METAUI,MetaUIVol);
   SafeRelease(pSndMixer);   
}

int  DrkSoundGetUIVol(void)
{
   return MetaUIVol;
}
   
// init to 0, so hey, the first frame will not 
//  be considered near by, and will be a moving frame
static mxs_vector last_pos;
static mxs_angvec last_angpos;

#define HIST_DEPTH 8
static int   hist_ptr=0;
static float hist_diffs[HIST_DEPTH][2], cur_diffs[2];

// control of state and tolerances
static int   curMsWrong     = 0;
static float listenVelTol   =  0.8;
static float listenAngTol   = 14.0;
static int   listenMsCap[2] = { 1500, 200 };

static BOOL  listenActive   = TRUE;

// head velocity evaluators to decide what listening mode you are in
void DrkSoundFrame(void)
{
   eListenModes desiredMode=kListenModeNormal;
   int ms=GetSimFrameTime();
   mxs_vector cur_pos, d_pos[2];
   mxs_angvec cur_angpos;
   int i;

   if (!listenActive)
      return;

   if ((ms==0)||(PlayerObject()==OBJ_NULL))
      return;

   // well, ok, but this is probaby frame rate dependant... damn
   PhysGetSubModLocation(PlayerObject(),PLAYER_HEAD,&cur_pos);
   PhysGetSubModRotation(PlayerObject(),PLAYER_HEAD,&cur_angpos);
   mx_sub_vec(&d_pos[0],&cur_pos,&last_pos);
   for (i=0; i<3; i++)  // need to relativize this right...
   {
      int ang_del=((int)cur_angpos.el[i]-(int)last_angpos.el[i]+0x10000)&0xffff;
      if (ang_del>0x8000) ang_del=0x10000-ang_del;
      d_pos[1].el[i]=(ang_del*180.0)/((float)0x8000);
   }
   last_pos=cur_pos;
   last_angpos=cur_angpos;
   for (i=0; i<2; i++)
   {
      cur_diffs[i]-=hist_diffs[hist_ptr][i];
      hist_diffs[hist_ptr][i]=mx_mag2_vec(&d_pos[i])*1000.0/(float)ms;
      cur_diffs[i]+=hist_diffs[hist_ptr][i];
      hist_ptr=(hist_ptr+1)%HIST_DEPTH;
   }
   if (cur_diffs[0]<HIST_DEPTH*listenVelTol)
      if (cur_diffs[1]<HIST_DEPTH*listenAngTol)
         desiredMode=kListenModeActive;

   if (desiredMode==curListenMode)
      curMsWrong=0;  // right mode, reset our capacitor
   else
   {
      curMsWrong+=GetSimFrameTime();
      if (curMsWrong>listenMsCap[curListenMode])  // have enough frames the other way way passed
      {
         DrkSoundSetListenMode(desiredMode);      // that is enough, lets go
#ifndef SHIP
         if (listenTrack)
            mprintf("Changed listen mode to %d diffs %g %g\n",curListenMode,cur_diffs[0],cur_diffs[1]);
#endif
      }
   }
}

//
// set main volume, takes string numeric argument:
//    unsigned    *pArg is absolute volume 0=silent 100=max (decibels)
//    +num        *pArg is decibels to add to volume
//    -num        *pArg is decibels to subtract from volume
//
static void
main_volume_cmd( char *pArg )
{
   int volume;
   BOOL isRelative = TRUE;

   switch( *pArg ) {
      case '+':
         pArg++;
      case '-':
         break;
      default:
         isRelative = FALSE;
   }
   // convert from decibels to millibels (*100)
   volume = atoi( pArg ) * 100;
   if ( isRelative ) {
      volume += SFX_GetMasterVolume();
   } else {
      // convert 0...10000 to -10000...0
      volume += kSndMinVolume;
   }

   // clamp volume
   if ( volume > 0 ) {
      volume = 0;
   }
   if ( volume < kSndMinVolume ) {
      volume = kSndMinVolume;
   }

   SFX_SetMasterVolume( volume );
}


//
// set ambient volume, takes string numeric argument:
//    unsigned    *pArg is absolute offset  0= -50db 50=0db 100=+50db (decibels)
//    +num        *pArg is decibels to add to volume
//    -num        *pArg is decibels to subtract from volume
//
static void
ambient_volume_cmd( char *pArg )
{
   int volume, val;
   BOOL isRelative = TRUE;

   switch( *pArg ) {
      case '+':
         pArg++;
      case '-':
         break;
      default:
         isRelative = FALSE;
   }

   val = atoi( pArg );
   if ( isRelative ) {
      // convert from decibels to millibels (*100)
      volume = (val * 100) + volume_offsets[DARK_SOUND_GROUP_AMBIENT];
   } else {
      // absolute volume - specified volume is for mode 0 (normal mode), calculate volume
      //   for other modes as relative to normal mode
#define AMBIENT_RANGE 50
      volume = (val - 50) * AMBIENT_RANGE;
   }

   DarkSoundSetAmbientVolume( volume );

}

static void
ambient_volume_up_cmd( int count ) 
{
   int i, dir;

   if ( count == 0 ) {
      count = 1;
   }

   dir = ( count < 0 ) ? -1 : 1;
   count = abs( count );

   for ( i = 0; i < count; i++ ) {
      if ( dir > 0 ) {
         ambient_volume_cmd( "+2" );
      } else {
         ambient_volume_cmd( "-2" );
      }
   }
}

static void
ambient_volume_down_cmd( int count ) 
{
   int i, dir;

   if ( count == 0 ) {
      count = 1;
   }

   dir = ( count < 0 ) ? -1 : 1;
   count = abs( count );

   for ( i = 0; i < count; i++ ) {
      if ( dir < 0 ) {
         ambient_volume_cmd( "+2" );
      } else {
         ambient_volume_cmd( "-2" );
      }
   }
}


static void
main_volume_up_cmd( int count )
{
   int i, dir;

   if ( count == 0 ) {
      count = 1;
   }

   dir = ( count < 0 ) ? -1 : 1;
   count = abs( count );

   for ( i = 0; i < count; i++ ) {
      if ( dir > 0 ) {
         main_volume_cmd( "+2" );
      } else {
         main_volume_cmd( "-2" );
      }
   }
}

static void
main_volume_down_cmd( int count )
{
   int i, dir;

   if ( count == 0 ) {
      count = 1;
   }

   dir = ( count < 0 ) ? -1 : 1;
   count = abs( count );

   for ( i = 0; i < count; i++ ) {
      if ( dir < 0 ) {
         main_volume_cmd( "+2" );
      } else {
         main_volume_cmd( "-2" );
      }
   }
}


static Command commands[] =
{
   { "main_volume", FUNC_STRING, main_volume_cmd, "main volume 0...100", HK_GAME_MODE },
   { "ambient_volume", FUNC_STRING, ambient_volume_cmd, "ambient relative volume 0...100", HK_GAME_MODE },
   { "main_volume_up", FUNC_INT, main_volume_up_cmd, "inc main volume", HK_GAME_MODE },
   { "main_volume_down", FUNC_INT, main_volume_down_cmd, "dec main volume", HK_GAME_MODE },
   { "ambient_volume_up", FUNC_INT, ambient_volume_up_cmd, "inc ambient relative volume", HK_GAME_MODE },
   { "ambient_volume_down", FUNC_INT, ambient_volume_down_cmd, "dec ambient relative volume", HK_GAME_MODE },
};

//////////////////////
// INIT/TERM OF THE DARK SOUND STUFF

void DarkSoundInit()
{
   ESndSetGameCallback(env_sound_CB);
   
   // if we could get the schemas to retrigger, maybe...
   // appsfxModeCallback=DrkSoundModeCallback;

   int tmp;
   
#ifndef SHIP
   config_get_float("listen_vel_tol",&listenVelTol);
   config_get_float("listen_ang_tol",&listenAngTol);
   config_get_int("listen_cap_ms_on",&listenMsCap[0]);
   config_get_int("listen_cap_ms_off",&listenMsCap[1]);   
   config_get_int("listen_fade_active_ms",&listenFadeMs[0]);
   config_get_int("listen_fade_normal_ms",&listenFadeMs[1]);
   if (config_get_int("listen_delta_noise",&tmp))
      volume_deltas[1][DARK_SOUND_GROUP_NOISE]=tmp;
   if (config_get_int("listen_delta_speech",&tmp))
      volume_deltas[1][DARK_SOUND_GROUP_SPEECH]=tmp;
   if (config_get_int("listen_delta_ambient",&tmp))
      volume_deltas[1][DARK_SOUND_GROUP_AMBIENT]=tmp;
   if (config_is_defined("listen_inactive"))
      listenActive=FALSE;
   if (config_is_defined("listen_track_spew"))
      listenTrack=TRUE;
   if (config_is_defined("listen_no_fades"))
      listenFade=FALSE;
#endif
   // make the metagame a tad quieter
   tmp = -400;
   config_get_int("metagame_volume", &tmp);
   volume_deltas[0][DARK_SOUND_GROUP_METAUI] = tmp;
   volume_deltas[1][DARK_SOUND_GROUP_METAUI] = tmp;
   volume_deltas[2][DARK_SOUND_GROUP_METAUI] = tmp;
   DrkSoundSetUIVol( tmp );
   // get the ambient/music volume offset
   tmp = 0;
   config_get_int( "ambient_volume", &tmp);
   volume_offsets[DARK_SOUND_GROUP_AMBIENT] = tmp;
   volume_offsets[DARK_SOUND_GROUP_MUSIC] = tmp;

   SongUtilSetSoundGroup( DARK_SOUND_GROUP_MUSIC );

   COMMANDS(commands,HK_ALL);
}

void DarkSoundTerm()
{
   config_set_int( "ambient_volume", volume_offsets[DARK_SOUND_GROUP_AMBIENT] );

   ESndSetGameCallback(NULL);    
}
