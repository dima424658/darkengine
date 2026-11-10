#include "movie_audio_output.h"

#include <algorithm>
#include <vector>

#include <appsfxi.h>

cMovieAudioOutput::~cMovieAudioOutput()
{
    Close();
}

bool cMovieAudioOutput::Open(int volume)
{
    mixer = SFX_GetMixer();
    movieVolume = volume;
    if (mixer)
    {
        previousMixerTimeout = mixer->SetTimeout(10);
        mixerTimeoutChanged = true;
    }
    return mixer != nullptr;
}

bool cMovieAudioOutput::Submit(const int16_t *samples, size_t count)
{
    if (!mixer || !samples || count == 0)
        return false;
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (finished)
            return false;
        queue.insert(queue.end(), samples, samples + count);
    }
    return EnsureStarted();
}

void cMovieAudioOutput::Finish()
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        finished = true;
    }
    EnsureStarted();
}

bool cMovieAudioOutput::IsDrained()
{
    if (!sample)
    {
        std::lock_guard<std::mutex> lock(mutex);
        return finished && queue.empty();
    }
    eSndState state{};
    eSndFlagSet flags{};
    sample->GetStatus(&state, &flags);
    return state == kSndStateStopped || state == kSndStateDestroyed;
}

bool cMovieAudioOutput::EnsureStarted()
{
    if (sample)
        return true;
    if (!mixer)
        return false;

    {
        std::lock_guard<std::mutex> lock(mutex);
        constexpr size_t prebufferSamples = 4800; // 50 ms, stereo interleaved
        if (finished && queue.empty())
            return true;
        if (!finished && queue.size() < prebufferSamples)
            return true;
    }

    sSndAttribs attribs{};
    attribs.dataType = kSndDataPCM;
    attribs.sampleRate = 48000;
    attribs.bitsPerSample = 16;
    attribs.nChannels = 2;
    attribs.bytesPerBlock = 4;
    attribs.samplesPerBlock = 1;
    attribs.numSamples = 0;

    constexpr uint32 bufferLength = 48000 * 4; // one second of stereo PCM
    sample = mixer->CreateRawSample(kSndSampleStream, nullptr, bufferLength,
                                    0, &attribs);
    if (!sample)
        return false;

    sample->SetVolume(movieVolume);
    sample->SetName("Movie audio");
    sample->RegisterFillCallback(Fill, this);
    sample->Play();
    return true;
}

void cMovieAudioOutput::Fill(ISndSample *sample, void *data, uint32 available)
{
    auto *self = static_cast<cMovieAudioOutput *>(data);
    std::vector<int16_t> buffer;
    bool endOfData = false;
    {
        std::lock_guard<std::mutex> lock(self->mutex);
        const size_t count = std::min<size_t>(available / sizeof(int16_t),
                                              self->queue.size()) & ~size_t(1);
        buffer.reserve(count);
        for (size_t i = 0; i < count; ++i)
        {
            buffer.push_back(self->queue.front());
            self->queue.pop_front();
        }
        endOfData = count == 0 && self->finished && self->queue.empty() &&
                    !self->endOfDataSent;
        if (endOfData)
            self->endOfDataSent = true;
    }

    if (!buffer.empty())
        sample->LoadBuffer(reinterpret_cast<uint8 *>(buffer.data()),
                           static_cast<uint32>(buffer.size() * sizeof(int16_t)));
    else if (endOfData)
        sample->LoadBuffer(nullptr, 0);
}

void cMovieAudioOutput::Close()
{
    if (sample)
    {
        sample->RegisterFillCallback(nullptr, nullptr);
        sample->Stop();
        sample->Release();
        sample = nullptr;
    }
    if (mixer)
    {
        if (mixerTimeoutChanged)
        {
            mixer->SetTimeout(previousMixerTimeout);
            mixerTimeoutChanged = false;
        }
        mixer->Release();
        mixer = nullptr;
    }
    std::lock_guard<std::mutex> lock(mutex);
    queue.clear();
    finished = false;
    endOfDataSent = false;
}
