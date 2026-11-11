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

    // Keep synchronization and STL members first. The engine builds with
    // -fpack-struct=1, so placing a mutex after scalar fields misaligns its
    // pthread storage even inside the pack(8) section.
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
#pragma pack(pop)
