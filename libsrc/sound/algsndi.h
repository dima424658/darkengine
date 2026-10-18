#pragma once

#include <deque>

#include <lgsndi.h>
#include <openal_api.h>

class cALSndSample;

class cALSndMixer : public cSndMixer
{
   DECLARE_UNAGGREGATABLE();

public:
   cALSndMixer();
   virtual ~cALSndMixer();

   STDMETHOD_(eSndError, Init)(sSndSetup *setup, uint32 channels, sSndAttribs *attribs);
   STDMETHOD_(ISndSample *, CreateRawSample)(eSndSampleCreateFlagSet flags, void *data,
                                              uint32 len, uint32 samples, sSndAttribs *attribs);
   STDMETHOD_(void, Pause)(void);
   STDMETHOD_(void, Resume)(void);
   STDMETHOD_(void, Set3DPosition)(sSndVector *position);
   STDMETHOD_(void, Set3DOrientation)(sSndVector *front, sSndVector *top);
   STDMETHOD_(void, Set3DVelocity)(sSndVector *velocity);
   STDMETHOD_(void, Set3DEnvironment)(sSndEnvironment *environment);
   STDMETHOD_(void, Set3DMethod)(eSnd3DMethod method);
   STDMETHOD_(void, Get3DMethodCapabilities)(uint32 *methods);
   STDMETHOD_(void, Set3DDeferMode)(BOOL defer);
   STDMETHOD_(void, FreeHWChannelCount)(int32 *channels, int32 *channels3D);
   STDMETHOD_(int32, Init3DReverb)(void);
   STDMETHOD_(void, Shutdown3DReverb)(void);
   STDMETHOD_(BOOL, Have3DReverb)(void);
   STDMETHOD_(BOOL, CanDo3DReverb)(void);
   STDMETHOD_(BOOL, Set3DReverbSettings)(ReverbSettings *settings);
   STDMETHOD_(BOOL, Get3DReverbSettings)(ReverbSettings *settings);
   STDMETHOD_(BOOL, Have3DOcclusion)(void);
   STDMETHOD_(const char *, GetBackendName)(void);
   STDMETHOD_(uint32, GetCapabilities)(void);
   STDMETHOD_(int32, Kludge)(int selector, void *data, int32 size);

   bool MakeCurrent(void);
   bool EffectsAvailable(void) const { return mEffectsAvailable; }
   bool ReverbEnabled(void) const { return mReverbEnabled && mEffectsAvailable; }
   ALuint EffectSlot(void) const { return mEffectSlot; }
   float RolloffFactor(void) const { return m3DEnvironment.rolloffFactor; }
   sOpenALApi &AL(void) { return mAL; }
   void UpdateEffect(void);

private:
   sOpenALApi mAL;
   ALCdevice *mDevice;
   ALCcontext *mContext;
   ALuint mEffect;
   ALuint mEffectSlot;
   BOOL mEffectsAvailable;
   BOOL mReverbEnabled;
   ReverbSettings mReverbSettings;

   bool InitEffects(void);
   void RefreshEffects(void);
};

class cALSndSample : public cSndSample
{
   DECLARE_UNAGGREGATABLE();

public:
   cALSndSample(cALSndMixer *mixer, eSndSampleCreateFlagSet flags);
   virtual ~cALSndSample();

   STDMETHOD_(void, SetPan)(int32 pan);
   STDMETHOD_(void, SetFrequency)(uint32 frequency);
   STDMETHOD_(uint32, AvailToWrite)(void);
   STDMETHOD_(void, CheckStream)(void);
   STDMETHOD_(BOOL, BufferReady)(uint32 len);
   STDMETHOD_(eSndError, LoadBufferIndirect)(SndLoadFunction loader, void *data, uint32 len);
   STDMETHOD_(void, SilenceFill)(uint32 bytes);
   STDMETHOD_(void, Set3DPosition)(sSndVector *position);
   STDMETHOD_(void, Set3DVelocity)(sSndVector *velocity);
   STDMETHOD_(void, Set3DConeAngles)(uint32 inside, uint32 outside);
   STDMETHOD_(void, Set3DConeOrientation)(sSndVector *orientation);
   STDMETHOD_(void, Set3DDistanceRange)(float minimum, float maximum);
   STDMETHOD_(void, Set3DMode)(eSnd3DMode mode);
   STDMETHOD_(void, SetAmbientVolume)(int32 volume);
   STDMETHOD_(void, Set3DMethod)(eSnd3DMethod method);
   STDMETHOD_(eSnd3DMethod, Get3DMethod)(void);
   STDMETHOD_(void, Set3DReverbMix)(float mix);
   STDMETHOD_(void, Set3DOcclusion)(int32 millibels);
   STDMETHOD_(int32, Kludge)(int selector, void *data, int32 size);

   virtual BOOL IsPlaying(void);
   virtual BOOL MakeAudible(void);
   void RefreshSpatialState(void) { ApplySpatialState(); }
   void UpdateEffectRouting(void);

protected:
   virtual void LLStop(void);
   virtual void LLRelease(void);
   virtual void LLInit(void);
   virtual eSndError LLStart(void);
   virtual void LLPause(void);
   virtual void LLResume(void);
   virtual void LLUnMute(void);
   virtual void LLSetPosition(uint32 pos);
   virtual uint32 LLGetPosition(void);
   virtual void LLSetVolume(int32 volume);

private:
   struct sQueuedBuffer { ALuint id; uint32 bytes; };

   cALSndMixer *mALMixer;
   ALuint mSource;
   ALuint mStaticBuffer;
   ALuint mFilter;
   ALuint mSendFilter;
   std::deque<sQueuedBuffer> mQueuedBuffers;
   uint32 mQueuedBytes;
   uint32 mPlayedBytes;
   float mReverbMix;
   int32 mOcclusion;

   ALenum Format(void) const;
   void ReclaimProcessed(void);
   void ApplySpatialState(void);
   void QueueData(const void *data, uint32 len);
};

BOOL SndCreateOpenALMixer(ISndMixer **mixer, IUnknown *outer);
