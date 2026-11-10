#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>

#include <lgsound.h>

#pragma pack(push, 8)
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

    ISndMixer *mixer{};
    ISndSample *sample{};
    int movieVolume{};
    uint32 previousMixerTimeout{~uint32{0}};
    bool mixerTimeoutChanged{};
    std::mutex mutex;
    std::deque<int16_t> queue;
    bool finished{};
    bool endOfDataSent{};
};
#pragma pack(pop)
