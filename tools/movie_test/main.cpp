#include <lg.h>
#include <config.h>
#include <objcoll.h>
#include <movieapi.h>
#include <appsfxi.h>
#include <runtime.h>

#include <cstdio>
#include <SDL3/SDL_main.h>

namespace
{
ISndMixer *g_testMixer;
}

extern "C" ISndMixer *SFX_GetMixer(void)
{
    if (g_testMixer)
        g_testMixer->AddRef();
    return g_testMixer;
}

int main(int argc, char *argv[])
{
    if (argc != 2 || !argv[1][0])
    {
        std::fprintf(stderr, "Usage: movie_test <movie-file>\n");
        return 1;
    }

    config_init();
    if (!DarkRenderOpen(640, 480, "Dark Engine movie test"))
    {
        config_shutdown();
        return 1;
    }

    sSndAttribs attributes{};
    attributes.dataType = kSndDataPCM;
    attributes.sampleRate = 48000;
    attributes.bitsPerSample = 16;
    attributes.nChannels = 2;
    if (!SndCreateMixer(&g_testMixer, nullptr) ||
        g_testMixer->Init(nullptr, 8, &attributes) != kSndOk)
    {
        std::fprintf(stderr, "Could not initialize the movie test audio mixer\n");
        if (g_testMixer)
            g_testMixer->Release();
        DarkRenderClose();
        config_shutdown();
        return 1;
    }

    IAggregate *aggregate = CreateGenericAggregate("Movie test", kISetKeyLGGUIDs);
    void *created = nullptr;
    const HRESULT result = _MoviePlayerCreate(IID_IMoviePlayer1, &created,
        aggregate, CLSID_ActiveMoviePlayer, 0);
    IMoviePlayer1 *player = static_cast<IMoviePlayer1 *>(created);
    bool played = SUCCEEDED(result) && player &&
        player->GetState() == kMP1NothingOpen &&
        player->Open(argv[1], 0, 0, 640, 480) && player->CanPlay() &&
        player->GetState() == kMP1Ready && player->SetVolume(0) &&
        player->Play(kMoviePlayBlock) && player->GetState() == kMP1Ready &&
        player->Play(kMoviePlayBlock) && player->GetState() == kMP1Ready;
    if (player)
    {
        player->Close();
        played = played && player->GetState() == kMP1NothingOpen && !player->CanPlay();
    }
    aggregate->ReleaseAll();
    g_testMixer->Release();
    g_testMixer = nullptr;
    DarkRenderClose();
    config_shutdown();
    std::fprintf(stderr, "%s: movie playback, replay and lifecycle\n",
        played ? "PASS" : "FAIL");
    return played ? 0 : 1;
}
