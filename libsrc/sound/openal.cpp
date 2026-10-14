#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#include <algsndi.h>
#include <comtools.h>
#include <lg.h>
#include <mixerlck.h>
#include <mprintf.h>

#define IS_STREAM  (mStateFlags & kSndFlagStream)
#define IS_LOOP    (mStateFlags & kSndFlagLooped)
#define IS_AUDIBLE (mStateFlags & kSndFlagAudible)
#define MIXER_MUTEX cMixerAutoLock mixerLock(mpMixer->MutexRef())

namespace
{
float MillibelsToGain(int32 millibels)
{
   if (millibels <= kSndMinVolume)
      return 0.0f;
   return std::pow(10.0f, static_cast<float>(millibels) / 2000.0f);
}

template <class T>
bool LoadExtension(T &target, sOpenALApi &al, const char *name)
{
   target = reinterpret_cast<T>(al.alGetProcAddress(name));
   return target != NULL;
}
}

BOOL SndCreateOpenALMixer(ISndMixer **mixer, IUnknown *outer)
{
   if (mixer == NULL || outer != NULL)
      return FALSE;
   *mixer = new cALSndMixer;
   return *mixer != NULL;
}

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cALSndMixer, ISndMixer);
IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cALSndSample, ISndSample);

cALSndMixer::cALSndMixer()
   : mDevice(NULL), mContext(NULL), mEffect(0), mEffectSlot(0),
     mEffectsAvailable(FALSE), mReverbEnabled(FALSE)
{
   std::memset(&mAL, 0, sizeof(mAL));
   std::memset(&mReverbSettings, 0, sizeof(mReverbSettings));
   mReverbSettings.flags = kREVERB_FlagType;
   mReverbSettings.type = kREVERB_Generic;
   mReverbSettings.level = 1.0f;
   mReverbSettings.decay = 1.49f;
   mReverbSettings.damping = 0.83f;
}

cALSndMixer::~cALSndMixer()
{
   MakeCurrent();
   Destroy();
   if (mEffectsAvailable)
   {
      mAL.alDeleteAuxiliaryEffectSlots(1, &mEffectSlot);
      mAL.alDeleteEffects(1, &mEffect);
   }
   if (mContext != NULL)
   {
      mAL.alcMakeContextCurrent(NULL);
      mAL.alcDestroyContext(mContext);
   }
   if (mDevice != NULL)
      mAL.alcCloseDevice(mDevice);
   OpenALUnload();
}

bool cALSndMixer::MakeCurrent(void)
{
   return mContext != NULL && mAL.alcMakeContextCurrent(mContext) != ALC_FALSE;
}

STDMETHODIMP_(eSndError) cALSndMixer::Init(sSndSetup *, uint32 channels,
                                           sSndAttribs *attribs)
{
   if (mDevice != NULL)
      return kSndDeviceAlready;
   if (!OpenALLoad(&mAL))
   {
      mprintf("OpenAL: unable to load the OpenAL runtime; audio is disabled\n");
      return kSndCantCreateDevice;
   }

   mDevice = mAL.alcOpenDevice(NULL);
   if (mDevice == NULL)
   {
      mprintf("OpenAL: no default playback device (check the PulseAudio, "
              "PipeWire, or ALSA service); audio is disabled\n");
      return kSndCantCreateDevice;
   }

   const ALCint effectAttributes[] = { ALC_MAX_AUXILIARY_SENDS, 1, 0 };
   mContext = mAL.alcCreateContext(mDevice, effectAttributes);
   if (mContext == NULL)
      mContext = mAL.alcCreateContext(mDevice, NULL);
   if (mContext == NULL || !MakeCurrent())
   {
      mprintf("OpenAL: unable to create a playback context; audio is disabled\n");
      return kSndCantSetupDevice;
   }

   mNumMixerChans = static_cast<int32>(channels);
   mNumFreeMixerChans = static_cast<int32>(channels);
   mPrimaryAttribs = attribs != NULL ? *attribs : sSndAttribs{};
   if (attribs == NULL)
   {
      mPrimaryAttribs.sampleRate = 22050;
      mPrimaryAttribs.bitsPerSample = 16;
      mPrimaryAttribs.nChannels = 2;
   }
   mPrimaryAttribs.dataType = kSndDataPCM;
   mPrimaryAttribs.bytesPerBlock =
      (mPrimaryAttribs.bitsPerSample * mPrimaryAttribs.nChannels) / 8;
   mPrimaryAttribs.samplesPerBlock = 1;
   mPrimaryAttribs.numSamples = ~0u;

   for (int group = 0; group <= kSndNumGroups; ++group)
      mGroupVolumes[group] = 0;
   mGroupFadesActive = FALSE;
   mMasterVolume = 0;
   mTimerState = eSndTimerUnknown;
   InitPlatformState();

   mAL.alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
   Set3DPosition(&m3DPosition);
   Set3DOrientation(&m3DFrontVector, &m3DTopVector);
   Set3DVelocity(&m3DVelocity);
   Set3DEnvironment(&m3DEnvironment);
   InitEffects();
   return kSndOk;
}

bool cALSndMixer::InitEffects(void)
{
   if (mAL.alcIsExtensionPresent(mDevice, "ALC_EXT_EFX") == ALC_FALSE)
      return false;

#define LOAD_EFX(name) LoadExtension(mAL.name, mAL, #name)
   const bool loaded =
      LOAD_EFX(alGenEffects) && LOAD_EFX(alDeleteEffects) &&
      LOAD_EFX(alEffecti) && LOAD_EFX(alEffectf) &&
      LOAD_EFX(alGenAuxiliaryEffectSlots) &&
      LOAD_EFX(alDeleteAuxiliaryEffectSlots) &&
      LOAD_EFX(alAuxiliaryEffectSloti) && LOAD_EFX(alGenFilters) &&
      LOAD_EFX(alDeleteFilters) && LOAD_EFX(alFilteri) && LOAD_EFX(alFilterf);
#undef LOAD_EFX
   if (!loaded)
      return false;

   while (mAL.alGetError() != AL_NO_ERROR) {}
   mAL.alGenEffects(1, &mEffect);
   mAL.alEffecti(mEffect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
   mAL.alGenAuxiliaryEffectSlots(1, &mEffectSlot);
   if (mAL.alGetError() != AL_NO_ERROR)
   {
      if (mEffectSlot != 0)
         mAL.alDeleteAuxiliaryEffectSlots(1, &mEffectSlot);
      if (mEffect != 0)
         mAL.alDeleteEffects(1, &mEffect);
      mEffect = mEffectSlot = 0;
      return false;
   }
   mEffectsAvailable = TRUE;
   UpdateEffect();
   return true;
}

STDMETHODIMP_(ISndSample *) cALSndMixer::CreateRawSample(
   eSndSampleCreateFlagSet flags, void *data, uint32 len, uint32 samples,
   sSndAttribs *attribs)
{
   cALSndSample *sample = new cALSndSample(this, flags);
   if (sample == NULL || !sample->Init(attribs, data, len, samples))
   {
      SafeRelease(sample);
      return NULL;
   }

   if (flags == kSndSampleStream)
   {
      if (mpThread != NULL)
         mpThread->CallWorker(kThreadNewStream);
   }
   else if (flags != kSndSampleInternal && data != NULL)
   {
      if (sample->MakeAudible())
         sample->LoadBuffer(static_cast<uint8 *>(data), len);
      else
         SafeRelease(sample);
   }
   if (sample != NULL)
      sample->SetGroupVolume(mMasterVolume + mGroupVolumes[0]);
   return sample;
}

STDMETHODIMP_(void) cALSndMixer::Pause(void)
{
   if (MakeCurrent())
      mAL.alcSuspendContext(mContext);
}

STDMETHODIMP_(void) cALSndMixer::Resume(void)
{
   if (MakeCurrent())
      mAL.alcProcessContext(mContext);
}

STDMETHODIMP_(void) cALSndMixer::Set3DPosition(sSndVector *position)
{
   m3DPosition = *position;
   if (MakeCurrent())
      mAL.alListener3f(AL_POSITION, position->x, position->y, position->z);
}

STDMETHODIMP_(void) cALSndMixer::Set3DOrientation(sSndVector *front, sSndVector *top)
{
   m3DFrontVector = *front;
   m3DTopVector = *top;
   const ALfloat orientation[] = { front->x, front->y, front->z,
                                   top->x, top->y, top->z };
   if (MakeCurrent())
      mAL.alListenerfv(AL_ORIENTATION, orientation);
}

STDMETHODIMP_(void) cALSndMixer::Set3DVelocity(sSndVector *velocity)
{
   m3DVelocity = *velocity;
   if (MakeCurrent())
      mAL.alListener3f(AL_VELOCITY, velocity->x, velocity->y, velocity->z);
}

STDMETHODIMP_(void) cALSndMixer::Set3DEnvironment(sSndEnvironment *environment)
{
   m3DEnvironment = *environment;
   if (MakeCurrent())
   {
      mAL.alDopplerFactor(environment->dopplerFactor);
      if (environment->distanceFactor > 0.0f)
         mAL.alSpeedOfSound(343.3f / environment->distanceFactor);
   }
   for (cSndSample *sample = mpAudibleListHead; sample != NULL; sample = sample->Next())
      static_cast<cALSndSample *>(sample)->RefreshSpatialState();
}

STDMETHODIMP_(void) cALSndMixer::Set3DMethod(eSnd3DMethod method)
{
   m3DMixMethod = method;
}

STDMETHODIMP_(void) cALSndMixer::Get3DMethodCapabilities(uint32 *methods)
{
   if (methods != NULL)
      *methods = kSnd3DMethodNone | kSnd3DMethodPanVol | kSnd3DMethodSoftware;
}

STDMETHODIMP_(void) cALSndMixer::Set3DDeferMode(BOOL defer)
{
   m3DDeferFlag = defer != FALSE;
}

STDMETHODIMP_(void) cALSndMixer::FreeHWChannelCount(int32 *channels, int32 *channels3D)
{
   if (channels != NULL) *channels = 0;
   if (channels3D != NULL) *channels3D = 0;
}

STDMETHODIMP_(int32) cALSndMixer::Init3DReverb(void)
{
   mReverbEnabled = mEffectsAvailable;
   RefreshEffects();
   return mReverbEnabled ? kREVERB_InitOK_SW : kREVERB_InitFail;
}

STDMETHODIMP_(void) cALSndMixer::Shutdown3DReverb(void)
{
   mReverbEnabled = FALSE;
   RefreshEffects();
}

STDMETHODIMP_(BOOL) cALSndMixer::Have3DReverb(void) { return ReverbEnabled(); }
STDMETHODIMP_(BOOL) cALSndMixer::CanDo3DReverb(void) { return mEffectsAvailable; }

STDMETHODIMP_(BOOL) cALSndMixer::Set3DReverbSettings(ReverbSettings *settings)
{
   if (settings == NULL || !mEffectsAvailable)
      return FALSE;
   if (settings->flags & kREVERB_FlagType) mReverbSettings.type = settings->type;
   if (settings->flags & kREVERB_FlagLevel) mReverbSettings.level = settings->level;
   if (settings->flags & kREVERB_FlagDecay) mReverbSettings.decay = settings->decay;
   if (settings->flags & kREVERB_FlagDamping) mReverbSettings.damping = settings->damping;
   mReverbSettings.flags |= settings->flags;
   UpdateEffect();
   return TRUE;
}

STDMETHODIMP_(BOOL) cALSndMixer::Get3DReverbSettings(ReverbSettings *settings)
{
   if (settings == NULL)
      return FALSE;
   *settings = mReverbSettings;
   return TRUE;
}

STDMETHODIMP_(BOOL) cALSndMixer::Have3DOcclusion(void) { return mEffectsAvailable; }
STDMETHODIMP_(const char *) cALSndMixer::GetBackendName(void) { return "openal"; }

STDMETHODIMP_(uint32) cALSndMixer::GetCapabilities(void)
{
   return kSndCapPositional |
      (mEffectsAvailable ? kSndCapReverb | kSndCapOcclusion : 0);
}

STDMETHODIMP_(int32) cALSndMixer::Kludge(int, void *, int32) { return 0; }

void cALSndMixer::UpdateEffect(void)
{
   if (!mEffectsAvailable || !MakeCurrent())
      return;

   struct sPreset { float density, diffusion, gain, gainHF, decay, decayHF; };
   static const sPreset presets[kREVERB_COUNT] = {
      {1.f,1.f,.32f,.89f,1.49f,.83f}, {1.f,1.f,.25f,.10f,.17f,.10f}, {1.f,1.f,.42f,.59f,.40f,.83f},
      {1.f,1.f,.32f,.25f,1.49f,.54f}, {1.f,1.f,.32f,.00f,.50f,.10f}, {1.f,1.f,.50f,.71f,2.31f,.64f},
      {1.f,1.f,.40f,.58f,4.32f,.59f}, {1.f,1.f,.50f,.56f,3.92f,.70f}, {1.f,1.f,.50f,1.00f,2.91f,1.30f},
      {1.f,1.f,.36f,.45f,7.24f,.33f}, {1.f,1.f,.50f,.32f,10.05f,.23f}, {1.f,1.f,.25f,.01f,.30f,.10f},
      {1.f,1.f,.36f,.71f,1.49f,.59f}, {1.f,1.f,.44f,.64f,2.70f,.79f}, {1.f,.30f,.32f,.73f,1.49f,.86f},
      {1.f,.30f,.15f,.54f,1.49f,.54f}, {1.f,.50f,.32f,.67f,1.49f,.67f}, {1.f,.27f,.27f,.21f,1.49f,.21f},
      {1.f,1.f,1.00f,.32f,1.49f,.83f}, {1.f,.21f,.21f,.50f,1.49f,.50f}, {1.f,1.f,1.00f,1.00f,1.65f,1.50f},
      {1.f,.80f,.32f,.32f,2.81f,.14f}, {1.f,1.f,.36f,.01f,1.49f,.10f}, {1.f,.50f,.43f,1.00f,8.39f,1.39f},
      {1.f,.60f,.36f,.63f,17.23f,.56f}, {1.f,.50f,.49f,.84f,7.56f,.91f}
   };
   const int type = std::max(0, std::min(mReverbSettings.type, kREVERB_COUNT - 1));
   const sPreset &preset = presets[type];
   float level = preset.gain;
   if (mReverbSettings.flags & kREVERB_FlagLevel)
      level *= std::max(0.0f, std::min(mReverbSettings.level, 1.0f));
   const float decay = (mReverbSettings.flags & kREVERB_FlagDecay)
      ? std::max(0.1f, std::min(mReverbSettings.decay, 20.0f)) : preset.decay;
   const float damping = (mReverbSettings.flags & kREVERB_FlagDamping)
      ? std::max(0.1f, std::min(mReverbSettings.damping, 2.0f)) : preset.decayHF;

   mAL.alEffectf(mEffect, AL_REVERB_DENSITY, preset.density);
   mAL.alEffectf(mEffect, AL_REVERB_DIFFUSION, preset.diffusion);
   mAL.alEffectf(mEffect, AL_REVERB_GAIN, level);
   mAL.alEffectf(mEffect, AL_REVERB_GAINHF, preset.gainHF);
   mAL.alEffectf(mEffect, AL_REVERB_DECAY_TIME, decay);
   mAL.alEffectf(mEffect, AL_REVERB_DECAY_HFRATIO, damping);
   mAL.alEffectf(mEffect, AL_REVERB_REFLECTIONS_GAIN, 0.05f);
   mAL.alEffectf(mEffect, AL_REVERB_REFLECTIONS_DELAY, 0.007f);
   mAL.alEffectf(mEffect, AL_REVERB_LATE_REVERB_GAIN, 1.26f);
   mAL.alEffectf(mEffect, AL_REVERB_LATE_REVERB_DELAY, 0.011f);
   mAL.alEffectf(mEffect, AL_REVERB_AIR_ABSORPTION_GAINHF, 0.994f);
   mAL.alEffectf(mEffect, AL_REVERB_ROOM_ROLLOFF_FACTOR, 0.0f);
   mAL.alEffecti(mEffect, AL_REVERB_DECAY_HFLIMIT, AL_TRUE);
   mAL.alAuxiliaryEffectSloti(mEffectSlot, AL_EFFECTSLOT_EFFECT,
                              static_cast<ALint>(mEffect));
}

void cALSndMixer::RefreshEffects(void)
{
   for (cSndSample *sample = mpAudibleListHead; sample != NULL; sample = sample->Next())
      static_cast<cALSndSample *>(sample)->UpdateEffectRouting();
}

cALSndSample::cALSndSample(cALSndMixer *mixer, eSndSampleCreateFlagSet flags)
   : cSndSample(mixer, flags), mALMixer(mixer), mSource(0), mStaticBuffer(0),
     mFilter(0), mSendFilter(0), mQueuedBytes(0), mPlayedBytes(0),
     mReverbMix(0.0f), mOcclusion(0)
{
}

cALSndSample::~cALSndSample()
{
   MIXER_MUTEX;
   if (mSource != 0)
   {
      mpMixer->DoTrace(reinterpret_cast<void *>(static_cast<uintptr_t>(mBufferLen)),
                       kSndBufferFree);
      LLRelease();
      mpMixer->FreeChannel();
   }
}

ALenum cALSndSample::Format(void) const
{
   if (mAttribs.nChannels == 1)
      return mAttribs.bitsPerSample == 8 ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
   return mAttribs.bitsPerSample == 8 ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
}

BOOL cALSndSample::MakeAudible(void)
{
   MIXER_MUTEX;
   if (IS_AUDIBLE)
      return TRUE;
   if (!mpMixer->AllocChannel(this))
      return FALSE;
   if (!mALMixer->MakeCurrent())
   {
      mpMixer->FreeChannel();
      return FALSE;
   }

   sOpenALApi &al = mALMixer->AL();
   while (al.alGetError() != AL_NO_ERROR) {}
   al.alGenSources(1, &mSource);
   if (!IS_STREAM)
      al.alGenBuffers(1, &mStaticBuffer);
   if (al.alGetError() != AL_NO_ERROR)
   {
      LLRelease();
      mpMixer->FreeChannel();
      return FALSE;
   }

   MoveToList(mpMixer->AudibleHead());
   SetFlags(kSndFlagAudible);
   mpMixer->DoTrace(reinterpret_cast<void *>(static_cast<uintptr_t>(mBufferLen)),
                    kSndBufferAllocate);
   mGroupVolume = mpMixer->GetGroupVolume(mGroup) + mpMixer->GetMasterVolume();
   ApplySpatialState();
   SetVolume(mVolume);
   SetFrequency(static_cast<uint32>(mFrequency));
   UpdateEffectRouting();
   return TRUE;
}

void cALSndSample::LLStop(void)
{
   if (mSource != 0 && mALMixer->MakeCurrent())
      mALMixer->AL().alSourceStop(mSource);
}

void cALSndSample::LLRelease(void)
{
   if (!mALMixer->MakeCurrent())
      return;
   sOpenALApi &al = mALMixer->AL();
   if (mSource != 0)
   {
      al.alSourceStop(mSource);
      al.alDeleteSources(1, &mSource);
      mSource = 0;
   }
   for (std::deque<sQueuedBuffer>::const_iterator it = mQueuedBuffers.begin();
        it != mQueuedBuffers.end(); ++it)
      al.alDeleteBuffers(1, &it->id);
   mQueuedBuffers.clear();
   mQueuedBytes = 0;
   if (mStaticBuffer != 0)
   {
      al.alDeleteBuffers(1, &mStaticBuffer);
      mStaticBuffer = 0;
   }
   if (mFilter != 0)
   {
      al.alDeleteFilters(1, &mFilter);
      mFilter = 0;
   }
   if (mSendFilter != 0)
   {
      al.alDeleteFilters(1, &mSendFilter);
      mSendFilter = 0;
   }
}

void cALSndSample::LLInit(void) {}

eSndError cALSndSample::LLStart(void)
{
   if (!mALMixer->MakeCurrent())
      return kSndUnknownError;
   if (fnFillCB != NULL && AvailToWrite() != 0)
      fnFillCB(this, mpFillCBData, AvailToWrite());
   if (mInitBuffPos != -1)
   {
      SetPosition(static_cast<uint32>(mInitBuffPos));
      mInitBuffPos = -1;
   }
   mALMixer->AL().alSourcei(mSource, AL_LOOPING,
      (!IS_STREAM && IS_LOOP) ? AL_TRUE : AL_FALSE);
   mALMixer->AL().alSourcePlay(mSource);
   return mALMixer->AL().alGetError() == AL_NO_ERROR ? kSndOk : kSndUnknownError;
}

void cALSndSample::LLPause(void)
{
   if (mALMixer->MakeCurrent()) mALMixer->AL().alSourcePause(mSource);
}

void cALSndSample::LLResume(void)
{
   if (mALMixer->MakeCurrent()) mALMixer->AL().alSourcePlay(mSource);
}

void cALSndSample::LLUnMute(void)
{
   if (MakeAudible() && fnFillCB != NULL)
      fnFillCB(this, mpFillCBData, AvailToWrite());
}

void cALSndSample::LLSetPosition(uint32 bytePosition)
{
   if (mSource != 0 && mALMixer->MakeCurrent())
      mALMixer->AL().alSourcei(mSource, AL_SAMPLE_OFFSET,
         static_cast<ALint>(bytePosition / (mBytesPerSample != 0 ? mBytesPerSample : 1)));
}

uint32 cALSndSample::LLGetPosition(void)
{
   if (mSource == 0 || !mALMixer->MakeCurrent())
      return mBasePos;
   ALint offset = 0;
   mALMixer->AL().alGetSourcei(mSource, AL_SAMPLE_OFFSET, &offset);
   const uint32 bytes = IS_STREAM ? mPlayedBytes : 0;
   return (bytes / (mBytesPerSample != 0 ? mBytesPerSample : 1)) +
      static_cast<uint32>(offset > 0 ? offset : 0);
}

void cALSndSample::LLSetVolume(int32 volume)
{
   if (mSource != 0 && mALMixer->MakeCurrent())
      mALMixer->AL().alSourcef(mSource, AL_GAIN, MillibelsToGain(volume));
}

STDMETHODIMP_(void) cALSndSample::SetPan(int32 pan)
{
   mPan = std::max(static_cast<int32>(kSndPanLeft),
                   std::min(pan, static_cast<int32>(kSndPanRight)));
   if (mSource != 0 && m3DMethod == kSnd3DMethodNone && mALMixer->MakeCurrent())
      mALMixer->AL().alSource3f(mSource, AL_POSITION,
         static_cast<float>(mPan) / kSndPanRight, 0.0f, -1.0f);
}

STDMETHODIMP_(void) cALSndSample::SetFrequency(uint32 frequency)
{
   frequency = std::max(static_cast<uint32>(kSndMinFrequency),
                        std::min(frequency, static_cast<uint32>(kSndMaxFrequency)));
   mFrequency = static_cast<int32>(frequency);
   if (mSource != 0 && mAttribs.sampleRate > 0 && mALMixer->MakeCurrent())
      mALMixer->AL().alSourcef(mSource, AL_PITCH,
         static_cast<float>(frequency) / mAttribs.sampleRate);
}

void cALSndSample::ReclaimProcessed(void)
{
   if (!IS_STREAM || mSource == 0 || !mALMixer->MakeCurrent())
      return;
   ALint processed = 0;
   sOpenALApi &al = mALMixer->AL();
   al.alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);
   while (processed-- > 0 && !mQueuedBuffers.empty())
   {
      ALuint buffer = 0;
      al.alSourceUnqueueBuffers(mSource, 1, &buffer);
      const uint32 bytes = mQueuedBuffers.front().bytes;
      mQueuedBuffers.pop_front();
      mQueuedBytes -= bytes;
      mPlayedBytes += bytes;
      al.alDeleteBuffers(1, &buffer);
   }
}

STDMETHODIMP_(uint32) cALSndSample::AvailToWrite(void)
{
   if (!IS_AUDIBLE)
      return 0;
   ReclaimProcessed();
   if (!IS_STREAM)
      return mStaticBuffer != 0 && mLastWrite == 0 ? mBufferLen : 0;
   const uint32 freeBytes = mBufferLen > mQueuedBytes ? mBufferLen - mQueuedBytes : 0;
   return freeBytes > 4 ? freeBytes - 4 : 0;
}

STDMETHODIMP_(BOOL) cALSndSample::BufferReady(uint32 len)
{
   return len == 0 || (mSource != 0 && len <= AvailToWrite());
}

void cALSndSample::QueueData(const void *data, uint32 len)
{
   sOpenALApi &al = mALMixer->AL();
   if (IS_STREAM)
   {
      const uint32 blockSize = std::max(static_cast<uint32>(mAttribs.bytesPerBlock),
                                        static_cast<uint32>(1));
      uint32 chunkSize = std::max(
         static_cast<uint32>(mAttribs.sampleRate) * mBytesPerSample / 10, blockSize);
      chunkSize -= chunkSize % blockSize;
      const uint8 *bytes = static_cast<const uint8 *>(data);
      for (uint32 offset = 0; offset < len; offset += chunkSize)
      {
         const uint32 bytesThisBuffer = std::min(chunkSize, len - offset);
         ALuint buffer = 0;
         al.alGenBuffers(1, &buffer);
         al.alBufferData(buffer, Format(), bytes + offset,
                         static_cast<ALsizei>(bytesThisBuffer), mAttribs.sampleRate);
         al.alSourceQueueBuffers(mSource, 1, &buffer);
         mQueuedBuffers.push_back(sQueuedBuffer{ buffer, bytesThisBuffer });
         mQueuedBytes += bytesThisBuffer;
      }
   }
   else
   {
      al.alBufferData(mStaticBuffer, Format(), data, static_cast<ALsizei>(len),
                      mAttribs.sampleRate);
      al.alSourcei(mSource, AL_BUFFER, static_cast<ALint>(mStaticBuffer));
      mLastWrite = len;
   }
}

STDMETHODIMP_(eSndError) cALSndSample::LoadBufferIndirect(
   SndLoadFunction loader, void *loaderData, uint32 len)
{
   if (IS_STREAM && len == 0)
   {
      SetFlags(kSndFlagEndOfData);
      return kSndOk;
   }
   if (loader == NULL || len > AvailToWrite() || !mALMixer->MakeCurrent())
      return kSndUnknownError;

   std::vector<uint8> bytes(len);
   void *source = loader(loaderData, bytes.data(), len);
   if (source != bytes.data())
      std::memcpy(bytes.data(), source, len);
   QueueData(bytes.data(), len);
   return mALMixer->AL().alGetError() == AL_NO_ERROR ? kSndOk : kSndUnknownError;
}

STDMETHODIMP_(void) cALSndSample::SilenceFill(uint32 bytes)
{
   if (bytes == 0 || bytes > AvailToWrite() || !mALMixer->MakeCurrent())
      return;
   const uint8 silence = mAttribs.bitsPerSample == 8 ? 0x80 : 0;
   std::vector<uint8> data(bytes, silence);
   QueueData(data.data(), bytes);
}

STDMETHODIMP_(void) cALSndSample::CheckStream(void)
{
   if (!IS_STREAM || mSource == 0)
      return;
   ReclaimProcessed();
   if ((mStateFlags & kSndFlagEndOfData) && mQueuedBuffers.empty())
   {
      DeferredStop();
      return;
   }
   const uint32 available = AvailToWrite();
   if (!(mStateFlags & kSndFlagEndOfData) && fnFillCB != NULL && available != 0)
      fnFillCB(this, mpFillCBData, available);

   ALint state = AL_STOPPED;
   mALMixer->AL().alGetSourcei(mSource, AL_SOURCE_STATE, &state);
   if (mState == kSndStatePlaying && state != AL_PLAYING && !mQueuedBuffers.empty())
      mALMixer->AL().alSourcePlay(mSource);
}

BOOL cALSndSample::IsPlaying(void)
{
   if (mSource == 0 || !mALMixer->MakeCurrent())
      return FALSE;
   ALint state = AL_STOPPED;
   mALMixer->AL().alGetSourcei(mSource, AL_SOURCE_STATE, &state);
   return state == AL_PLAYING || state == AL_PAUSED;
}

void cALSndSample::ApplySpatialState(void)
{
   if (mSource == 0 || !mALMixer->MakeCurrent())
      return;
   sOpenALApi &al = mALMixer->AL();
   const bool positional = m3DMethod == kSnd3DMethodSoftware ||
                           m3DMethod == kSnd3DMethodHardware;
   al.alSourcei(mSource, AL_SOURCE_RELATIVE,
      m3DMode == kSnd3DModeHeadRelative || !positional ? AL_TRUE : AL_FALSE);
   al.alSourcef(mSource, AL_ROLLOFF_FACTOR,
      positional ? mALMixer->RolloffFactor() : 0.0f);
   if (positional)
   {
      al.alSource3f(mSource, AL_POSITION, m3DPosition.x, m3DPosition.y, m3DPosition.z);
      al.alSource3f(mSource, AL_VELOCITY, m3DVelocity.x, m3DVelocity.y, m3DVelocity.z);
      al.alSource3f(mSource, AL_DIRECTION, m3DConeOrientation.x,
                    m3DConeOrientation.y, m3DConeOrientation.z);
      al.alSourcef(mSource, AL_REFERENCE_DISTANCE, m3DMinDistance);
      al.alSourcef(mSource, AL_MAX_DISTANCE, m3DMaxDistance);
      al.alSourcef(mSource, AL_CONE_INNER_ANGLE, static_cast<float>(m3DConeInnerAngle));
      al.alSourcef(mSource, AL_CONE_OUTER_ANGLE, static_cast<float>(m3DConeOuterAngle));
      al.alSourcef(mSource, AL_CONE_OUTER_GAIN, MillibelsToGain(mAmbientVolume));
   }
   else
      SetPan(mPan);
}

STDMETHODIMP_(void) cALSndSample::Set3DPosition(sSndVector *position)
{
   m3DPosition = *position;
   ApplySpatialState();
}

STDMETHODIMP_(void) cALSndSample::Set3DVelocity(sSndVector *velocity)
{
   m3DVelocity = *velocity;
   ApplySpatialState();
}

STDMETHODIMP_(void) cALSndSample::Set3DConeAngles(uint32 inside, uint32 outside)
{
   m3DConeInnerAngle = inside;
   m3DConeOuterAngle = outside;
   ApplySpatialState();
}

STDMETHODIMP_(void) cALSndSample::Set3DConeOrientation(sSndVector *orientation)
{
   m3DConeOrientation = *orientation;
   ApplySpatialState();
}

STDMETHODIMP_(void) cALSndSample::Set3DDistanceRange(float minimum, float maximum)
{
   m3DMinDistance = minimum;
   m3DMaxDistance = maximum;
   ApplySpatialState();
}

STDMETHODIMP_(void) cALSndSample::Set3DMode(eSnd3DMode mode)
{
   m3DMode = mode;
   ApplySpatialState();
}

STDMETHODIMP_(void) cALSndSample::SetAmbientVolume(int32 volume)
{
   mAmbientVolume = volume;
   ApplySpatialState();
}

STDMETHODIMP_(void) cALSndSample::Set3DMethod(eSnd3DMethod method)
{
   m3DMethod = method;
   ApplySpatialState();
}

STDMETHODIMP_(eSnd3DMethod) cALSndSample::Get3DMethod(void) { return m3DMethod; }

STDMETHODIMP_(void) cALSndSample::Set3DReverbMix(float mix)
{
   mReverbMix = std::max(0.0f, std::min(mix, 1.0f));
   UpdateEffectRouting();
}

STDMETHODIMP_(void) cALSndSample::Set3DOcclusion(int32 millibels)
{
   mOcclusion = std::max(static_cast<int32>(kSndMinVolume),
                         std::min(millibels, static_cast<int32>(0)));
   UpdateEffectRouting();
}

STDMETHODIMP_(int32) cALSndSample::Kludge(int, void *, int32) { return 0; }

void cALSndSample::UpdateEffectRouting(void)
{
   if (mSource == 0 || !mALMixer->EffectsAvailable() || !mALMixer->MakeCurrent())
      return;
   sOpenALApi &al = mALMixer->AL();
   if (mFilter == 0)
   {
      al.alGenFilters(1, &mFilter);
      al.alFilteri(mFilter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
   }
   if (mSendFilter == 0)
   {
      al.alGenFilters(1, &mSendFilter);
      al.alFilteri(mSendFilter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
   }
   al.alFilterf(mFilter, AL_LOWPASS_GAIN, 1.0f);
   al.alFilterf(mFilter, AL_LOWPASS_GAINHF, MillibelsToGain(mOcclusion));
   al.alSourcei(mSource, AL_DIRECT_FILTER, static_cast<ALint>(mFilter));

   const float wet = mALMixer->ReverbEnabled() ? mReverbMix : 0.0f;
   al.alFilterf(mSendFilter, AL_LOWPASS_GAIN, wet);
   al.alFilterf(mSendFilter, AL_LOWPASS_GAINHF, 1.0f);
   al.alSource3i(mSource, AL_AUXILIARY_SEND_FILTER,
      static_cast<ALint>(mALMixer->EffectSlot()), 0, static_cast<ALint>(mSendFilter));
}
