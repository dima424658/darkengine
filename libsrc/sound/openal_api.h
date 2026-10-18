#pragma once

// Small, dependency-free subset of the OpenAL ABI used by the optional
// provider.  The platform loader supplies these entry points at runtime, so
// the default Windows build has no OpenAL import or DLL requirement.

typedef char ALchar;
typedef int ALenum;
typedef int ALint;
typedef int ALsizei;
typedef unsigned int ALuint;
typedef float ALfloat;
typedef void ALvoid;
typedef char ALCchar;
typedef int ALCenum;
typedef int ALCint;
typedef int ALCsizei;
typedef unsigned char ALCboolean;
struct ALCdevice;
struct ALCcontext;

#define AL_FALSE 0
#define AL_TRUE 1
#define AL_NO_ERROR 0
#define AL_NONE 0
#define AL_SOURCE_RELATIVE 0x0202
#define AL_CONE_INNER_ANGLE 0x1001
#define AL_CONE_OUTER_ANGLE 0x1002
#define AL_PITCH 0x1003
#define AL_POSITION 0x1004
#define AL_DIRECTION 0x1005
#define AL_VELOCITY 0x1006
#define AL_LOOPING 0x1007
#define AL_BUFFER 0x1009
#define AL_GAIN 0x100A
#define AL_ORIENTATION 0x100F
#define AL_SOURCE_STATE 0x1010
#define AL_PLAYING 0x1012
#define AL_PAUSED 0x1013
#define AL_STOPPED 0x1014
#define AL_BUFFERS_QUEUED 0x1015
#define AL_BUFFERS_PROCESSED 0x1016
#define AL_REFERENCE_DISTANCE 0x1020
#define AL_ROLLOFF_FACTOR 0x1021
#define AL_CONE_OUTER_GAIN 0x1022
#define AL_MAX_DISTANCE 0x1023
#define AL_SAMPLE_OFFSET 0x1025
#define AL_FORMAT_MONO8 0x1100
#define AL_FORMAT_MONO16 0x1101
#define AL_FORMAT_STEREO8 0x1102
#define AL_FORMAT_STEREO16 0x1103
#define AL_INVERSE_DISTANCE_CLAMPED 0xD002

#define ALC_FALSE 0
#define ALC_MAX_AUXILIARY_SENDS 0x20003

#define AL_EFFECT_TYPE 0x8001
#define AL_EFFECT_REVERB 0x0001
#define AL_REVERB_DENSITY 0x0001
#define AL_REVERB_DIFFUSION 0x0002
#define AL_REVERB_GAIN 0x0003
#define AL_REVERB_GAINHF 0x0004
#define AL_REVERB_DECAY_TIME 0x0005
#define AL_REVERB_DECAY_HFRATIO 0x0006
#define AL_REVERB_REFLECTIONS_GAIN 0x0007
#define AL_REVERB_REFLECTIONS_DELAY 0x0008
#define AL_REVERB_LATE_REVERB_GAIN 0x0009
#define AL_REVERB_LATE_REVERB_DELAY 0x000A
#define AL_REVERB_AIR_ABSORPTION_GAINHF 0x000B
#define AL_REVERB_ROOM_ROLLOFF_FACTOR 0x000C
#define AL_REVERB_DECAY_HFLIMIT 0x000D
#define AL_EFFECTSLOT_EFFECT 0x0001
#define AL_FILTER_TYPE 0x8001
#define AL_FILTER_LOWPASS 0x0001
#define AL_LOWPASS_GAIN 0x0001
#define AL_LOWPASS_GAINHF 0x0002
#define AL_DIRECT_FILTER 0x20005
#define AL_AUXILIARY_SEND_FILTER 0x20006

typedef struct sOpenALApi
{
   ALCdevice *(*alcOpenDevice)(const ALCchar *);
   ALCboolean (*alcCloseDevice)(ALCdevice *);
   ALCcontext *(*alcCreateContext)(ALCdevice *, const ALCint *);
   ALCboolean (*alcMakeContextCurrent)(ALCcontext *);
   void (*alcProcessContext)(ALCcontext *);
   void (*alcSuspendContext)(ALCcontext *);
   void (*alcDestroyContext)(ALCcontext *);
   ALCboolean (*alcIsExtensionPresent)(ALCdevice *, const ALCchar *);
   void *(*alcGetProcAddress)(ALCdevice *, const ALCchar *);

   ALenum (*alGetError)(void);
   void *(*alGetProcAddress)(const ALchar *);
   void (*alGenSources)(ALsizei, ALuint *);
   void (*alDeleteSources)(ALsizei, const ALuint *);
   void (*alSourcePlay)(ALuint);
   void (*alSourcePause)(ALuint);
   void (*alSourceStop)(ALuint);
   void (*alSourcei)(ALuint, ALenum, ALint);
   void (*alSourcef)(ALuint, ALenum, ALfloat);
   void (*alSource3f)(ALuint, ALenum, ALfloat, ALfloat, ALfloat);
   void (*alSource3i)(ALuint, ALenum, ALint, ALint, ALint);
   void (*alGetSourcei)(ALuint, ALenum, ALint *);
   void (*alGetSourcef)(ALuint, ALenum, ALfloat *);
   void (*alGenBuffers)(ALsizei, ALuint *);
   void (*alDeleteBuffers)(ALsizei, const ALuint *);
   void (*alBufferData)(ALuint, ALenum, const ALvoid *, ALsizei, ALsizei);
   void (*alSourceQueueBuffers)(ALuint, ALsizei, const ALuint *);
   void (*alSourceUnqueueBuffers)(ALuint, ALsizei, ALuint *);
   void (*alListener3f)(ALenum, ALfloat, ALfloat, ALfloat);
   void (*alListenerfv)(ALenum, const ALfloat *);
   void (*alDopplerFactor)(ALfloat);
   void (*alSpeedOfSound)(ALfloat);
   void (*alDistanceModel)(ALenum);

   void (*alGenEffects)(ALsizei, ALuint *);
   void (*alDeleteEffects)(ALsizei, const ALuint *);
   void (*alEffecti)(ALuint, ALenum, ALint);
   void (*alEffectf)(ALuint, ALenum, ALfloat);
   void (*alGenAuxiliaryEffectSlots)(ALsizei, ALuint *);
   void (*alDeleteAuxiliaryEffectSlots)(ALsizei, const ALuint *);
   void (*alAuxiliaryEffectSloti)(ALuint, ALenum, ALint);
   void (*alGenFilters)(ALsizei, ALuint *);
   void (*alDeleteFilters)(ALsizei, const ALuint *);
   void (*alFilteri)(ALuint, ALenum, ALint);
   void (*alFilterf)(ALuint, ALenum, ALfloat);
} sOpenALApi;

bool OpenALLoad(sOpenALApi *api);
void OpenALUnload(void);
