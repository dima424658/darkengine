#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>

#include <lgsound.h>

class cMovieAudioOutput
{
public:
    cMovieAudioOutput() = default;
    ~cMovieAudioOutput();

    cMovieAudioOutput(const cMovieAudioOutput &) = delete;
    cMovieAudioOutput &operator=(const cMovieAudioOutput &) = delete;

    bool Open(int volume);
    bool Submit(const int16_t *samples, size_t count);
    void Finish();
    bool IsDrained();

private:
    bool EnsureStarted();
    void Close();
    static void Fill(ISndSample *sample, void *data, uint32 available);

    std::mutex mutex;
    std::deque<int16_t> queue;
    ISndMixer *mixer{};
    ISndSample *sample{};
    int movieVolume{};
    uint32 previousMixerTimeout{~uint32{0}};
    bool mixerTimeoutChanged{};
    bool finished{};
    bool endOfDataSent{};
};
