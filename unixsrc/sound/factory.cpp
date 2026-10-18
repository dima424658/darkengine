#include <lgsound.h>

#pragma pack(push, 8)
#include <strings.h>
#pragma pack(pop)

extern BOOL SndCreateOpenALMixer(ISndMixer **mixer, IUnknown *outer);

extern "C" BOOL SndCreateMixer(ISndMixer **mixer, IUnknown *outer)
{
    return SndCreateOpenALMixer(mixer, outer);
}

extern "C" BOOL SndCreateMixerNamed(ISndMixer **mixer, IUnknown *outer,
                                      const char *backend)
{
    if (!backend || !*backend || strcasecmp(backend, "native") == 0 ||
        strcasecmp(backend, "openal") == 0)
        return SndCreateOpenALMixer(mixer, outer);
    // The common auto path retries the portable backend after a native
    // provider fails. Linux has no separate native provider yet, so defer
    // directly to that single OpenAL attempt instead of opening it twice.
    if (strcasecmp(backend, "auto") == 0)
    {
        *mixer = nullptr;
        return FALSE;
    }
    *mixer = nullptr;
    return FALSE;
}
