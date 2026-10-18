#pragma pack(push, 8)
#include <SDL2/SDL.h>
#ifdef __ANDROID__
#include <SDL2/SDL_system.h>
#include <jni.h>
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#ifdef DARK_HAVE_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}
#endif
#pragma pack(pop)

#include <aggmemb.h>
#include <movie_backend.h>
#include <movieapi.h>
#include <platform_services.h>

#include "../render/runtime.h"

class cUnixMoviePlayer final : public IMoviePlayer1
{
public:
    explicit cUnixMoviePlayer(IUnknown *outer)
        : m_state(kMP1NothingOpen), m_volume(0)
    {
        INIT_AGGREGATION_1(outer, IID_IMoviePlayer1, this, kPriorityLibrary, nullptr);
    }

    DECLARE_SIMPLE_AGGREGATION(cUnixMoviePlayer);

    STDMETHOD_(BOOL, Open)(const char *, int, int, int, int) override
    {
        m_state = kMP1NothingOpen;
        return FALSE;
    }
    STDMETHOD_(void, Close)() override { m_state = kMP1NothingOpen; }
    STDMETHOD_(BOOL, CanPlay)() override {
#ifdef DARK_HAVE_FFMPEG
        return TRUE;
#else
        return FALSE;
#endif
    }
    STDMETHOD_(BOOL, Play)(int) override { return FALSE; }
    STDMETHOD_(BOOL, Pause)() override { return FALSE; }
    STDMETHOD_(BOOL, Stop)() override { m_state = kMP1NothingOpen; return TRUE; }
    STDMETHOD_(BOOL, GetVolume)(long *volume) override
    {
        if (volume) *volume = m_volume;
        return volume != nullptr;
    }
    STDMETHOD_(BOOL, SetVolume)(long volume) override { m_volume = volume; return TRUE; }
    STDMETHOD_(eMP1State, GetState)() override { return m_state; }
    STDMETHOD_(void, SetTermKeys)(char *) override {}

private:
    eMP1State m_state;
    long m_volume;
};

IMPLEMENT_SIMPLE_AGGREGATION_SELF_DELETE(cUnixMoviePlayer);

extern "C" tResult LGAPI _MoviePlayerCreate(REFIID, void **player,
                                              IUnknown *outer, REFCLSID,
                                              unsigned)
{
    if (!outer)
        return E_INVALIDARG;
    auto *created = new cUnixMoviePlayer(outer);
    if (player)
        *player = created;
    return created ? S_OK : E_FAIL;
}

#ifdef DARK_HAVE_FFMPEG
namespace
{
using MovieClock = std::chrono::steady_clock;

struct Decoder
{
    AVCodecContext *context{};
    int stream{-1};
};

void CloseDecoder(Decoder &decoder)
{
    avcodec_free_context(&decoder.context);
    decoder.stream = -1;
}

bool OpenDecoder(AVFormatContext *format, AVMediaType type, Decoder &decoder)
{
    const int stream = av_find_best_stream(format, type, -1, -1, nullptr, 0);
    if (stream < 0)
        return false;
    const AVCodec *codec = avcodec_find_decoder(
        format->streams[stream]->codecpar->codec_id);
    if (!codec)
        return false;
    decoder.context = avcodec_alloc_context3(codec);
    if (!decoder.context ||
        avcodec_parameters_to_context(decoder.context,
                                      format->streams[stream]->codecpar) < 0 ||
        avcodec_open2(decoder.context, codec, nullptr) < 0)
    {
        CloseDecoder(decoder);
        return false;
    }
    decoder.stream = stream;
    return true;
}

std::string NativeMoviePath(const char *legacyPath)
{
    char native[4096];
    std::string modern = legacyPath ? legacyPath : "";
    const size_t dot = modern.find_last_of('.');
    if (dot != std::string::npos)
    {
        modern.resize(dot);
        modern += ".mp4";
        if (PlatformResolvePath(modern.c_str(), native, sizeof(native)))
            return native;
    }
    if (PlatformResolvePath(legacyPath, native, sizeof(native)))
        return native;
    return legacyPath ? legacyPath : "";
}

float MovieGain(int volume)
{
    if (volume <= -10000)
        return 0.0f;
    if (volume >= 0)
        return 1.0f;
    return (float)std::pow(10.0, volume / 2000.0);
}

void ScaleAudio(std::vector<int16_t> &samples, float gain)
{
    if (gain >= 0.9999f)
        return;
    for (int16_t &sample : samples)
        sample = (int16_t)std::clamp((int)(sample * gain), -32768, 32767);
}

bool WaitForFrame(MovieClock::time_point start, double seconds)
{
    for (;;)
    {
        if (DarkRenderPumpMovieEvents())
            return false;
        const double elapsed = std::chrono::duration<double>(
            MovieClock::now() - start).count();
        if (elapsed >= seconds)
            return true;
        const double remaining = seconds - elapsed;
        std::this_thread::sleep_for(std::chrono::milliseconds(
            remaining > 0.010 ? 5 : 1));
    }
}
}
#endif

bool MovieBackendPlaySynchronous(const char *legacyPath, int volume)
{
#ifdef __ANDROID__
    JNIEnv *environment = static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
    jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
    if (!environment || !activity || !legacyPath)
        return false;

    jclass activityClass = environment->GetObjectClass(activity);
    jmethodID playMovie = activityClass ? environment->GetMethodID(
        activityClass, "playMovieSynchronously", "(Ljava/lang/String;I)Z") :
        nullptr;
    jstring path = environment->NewStringUTF(legacyPath);
    bool played = false;
    if (playMovie && path)
    {
        played = environment->CallBooleanMethod(activity, playMovie, path,
                                                 volume) == JNI_TRUE;
        if (environment->ExceptionCheck())
        {
            environment->ExceptionClear();
            played = false;
        }
    }
    if (path)
        environment->DeleteLocalRef(path);
    if (activityClass)
        environment->DeleteLocalRef(activityClass);
    environment->DeleteLocalRef(activity);
    return played;
#elif !defined(DARK_HAVE_FFMPEG)
    (void)legacyPath;
    (void)volume;
    static bool warned = false;
    if (!warned)
    {
        std::fprintf(stderr,
            "Linux movie playback requires FFmpeg development libraries at build time.\n");
        warned = true;
    }
    return false;
#else
    const std::string path = NativeMoviePath(legacyPath);
    AVFormatContext *format = nullptr;
    Decoder video;
    Decoder audio;
    SwsContext *scaler = nullptr;
    SwrContext *resampler = nullptr;
    AVFrame *frame = nullptr;
    AVPacket *packet = nullptr;
    SDL_AudioDeviceID audioDevice = 0;
    bool audioStarted = false;
    bool played = false;
    bool keepPlaying = true;
    int64_t firstVideoTimestamp = AV_NOPTS_VALUE;
    MovieClock::time_point start;

    if (path.empty() || avformat_open_input(&format, path.c_str(), nullptr,
                                            nullptr) < 0 ||
        avformat_find_stream_info(format, nullptr) < 0 ||
        !OpenDecoder(format, AVMEDIA_TYPE_VIDEO, video))
    {
        std::fprintf(stderr, "Could not decode movie: %s\n", path.c_str());
        if (format)
            avformat_close_input(&format);
        return false;
    }
    OpenDecoder(format, AVMEDIA_TYPE_AUDIO, audio);

    std::fprintf(stderr,
        "Playing movie with FFmpeg: %s (%dx%d%s)\n", path.c_str(),
        video.context->width, video.context->height,
        audio.context ? ", audio" : "");

    frame = av_frame_alloc();
    packet = av_packet_alloc();
    std::vector<uint8_t> rgba((size_t)video.context->width *
                              video.context->height * 4);
    scaler = sws_getContext(video.context->width, video.context->height,
                            video.context->pix_fmt,
                            video.context->width, video.context->height,
                            AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr,
                            nullptr);
    if (!frame || !packet || !scaler)
        keepPlaying = false;

    if (audio.context && SDL_InitSubSystem(SDL_INIT_AUDIO) == 0)
    {
        SDL_AudioSpec wanted{};
        wanted.freq = 48000;
        wanted.format = AUDIO_S16SYS;
        wanted.channels = 2;
        wanted.samples = 2048;
        audioDevice = SDL_OpenAudioDevice(nullptr, 0, &wanted, nullptr, 0);
        if (audioDevice)
        {
            AVChannelLayout outputLayout = AV_CHANNEL_LAYOUT_STEREO;
            if (swr_alloc_set_opts2(&resampler, &outputLayout,
                                    AV_SAMPLE_FMT_S16, wanted.freq,
                                    &audio.context->ch_layout,
                                    audio.context->sample_fmt,
                                    audio.context->sample_rate, 0, nullptr) < 0 ||
                swr_init(resampler) < 0)
            {
                swr_free(&resampler);
                SDL_CloseAudioDevice(audioDevice);
                audioDevice = 0;
            }
        }
    }

    const float gain = MovieGain(volume);
    while (keepPlaying && av_read_frame(format, packet) >= 0)
    {
        Decoder *decoder = packet->stream_index == video.stream ? &video :
                           packet->stream_index == audio.stream ? &audio :
                           nullptr;
        if (!decoder || avcodec_send_packet(decoder->context, packet) < 0)
        {
            av_packet_unref(packet);
            continue;
        }
        av_packet_unref(packet);

        while (keepPlaying && avcodec_receive_frame(decoder->context, frame) == 0)
        {
            if (decoder == &video)
            {
                uint8_t *planes[] = { rgba.data() };
                int strides[] = { video.context->width * 4 };
                sws_scale(scaler, frame->data, frame->linesize, 0,
                          video.context->height, planes, strides);
                const int64_t timestamp = frame->best_effort_timestamp;
                if (firstVideoTimestamp == AV_NOPTS_VALUE)
                {
                    firstVideoTimestamp = timestamp == AV_NOPTS_VALUE ? 0 :
                                          timestamp;
                    start = MovieClock::now();
                }
                const double seconds = timestamp == AV_NOPTS_VALUE ? 0.0 :
                    (timestamp - firstVideoTimestamp) *
                    av_q2d(format->streams[video.stream]->time_base);
                keepPlaying = WaitForFrame(start, std::max(seconds, 0.0));
                if (keepPlaying)
                {
                    played = DarkRenderPresentMovieFrame(
                        rgba.data(), video.context->width,
                        video.context->height, strides[0]);
                    if (audioDevice && !audioStarted)
                    {
                        SDL_PauseAudioDevice(audioDevice, 0);
                        audioStarted = true;
                    }
                }
            }
            else if (audioDevice && resampler)
            {
                const int maximum = av_rescale_rnd(
                    swr_get_delay(resampler, audio.context->sample_rate) +
                    frame->nb_samples, 48000, audio.context->sample_rate,
                    AV_ROUND_UP);
                std::vector<int16_t> samples((size_t)maximum * 2);
                uint8_t *output[] = {
                    reinterpret_cast<uint8_t *>(samples.data())
                };
                const int converted = swr_convert(
                    resampler, output, maximum,
                    (const uint8_t **)frame->extended_data,
                    frame->nb_samples);
                if (converted > 0)
                {
                    samples.resize((size_t)converted * 2);
                    ScaleAudio(samples, gain);
                    SDL_QueueAudio(audioDevice, samples.data(),
                                   (Uint32)(samples.size() * sizeof(int16_t)));
                }
            }
            av_frame_unref(frame);
        }
    }

    if (audioDevice && audioStarted && keepPlaying)
    {
        while (SDL_GetQueuedAudioSize(audioDevice) > 0 &&
               !DarkRenderPumpMovieEvents())
            SDL_Delay(5);
    }
    if (audioDevice)
    {
        SDL_ClearQueuedAudio(audioDevice);
        SDL_CloseAudioDevice(audioDevice);
    }
    swr_free(&resampler);
    sws_freeContext(scaler);
    av_packet_free(&packet);
    av_frame_free(&frame);
    CloseDecoder(audio);
    CloseDecoder(video);
    avformat_close_input(&format);
    return played;
#endif
}
