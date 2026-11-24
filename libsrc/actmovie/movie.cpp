#define SDL_ENABLE_OLD_NAMES
#include <SDL3/SDL.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#ifdef __ANDROID__
#include <jni.h>
#endif

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include "movie.h"
#include <platform_services.h>
#include "movie_audio_output.h"

#include <appagg.h>
#include <vkrend.h>
#include <sdl_events.h>

namespace
{
std::atomic_bool g_movie_playing{false};

class cMoviePlaybackScope
{
public:
    cMoviePlaybackScope() { g_movie_playing.store(true); }
    ~cMoviePlaybackScope() { g_movie_playing.store(false); }
};
}

extern "C" bool DarkMoviePlaybackActive(void)
{
    return g_movie_playing.load();
}

#ifdef __ANDROID__
extern "C" JNIEXPORT jboolean JNICALL
Java_io_github_inkmakerdroid_darkengine_ThiefActivity_nativeMoviePlaybackActive(
    JNIEnv *, jclass)
{
    return DarkMoviePlaybackActive() ? JNI_TRUE : JNI_FALSE;
}
#endif

namespace
{
using MovieClock = std::chrono::steady_clock;

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

bool WaitForFrame(MovieClock::time_point start, double seconds,
    bool allowSkip, const char *termKeys)
{
    for (;;)
    {
        if (DarkPumpMovieEvents(allowSkip, termKeys))
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

void cSDLMoviePlayer::CloseDecoder(Decoder &decoder)
{
    avcodec_free_context(&decoder.context);
    decoder.stream = -1;
}

bool cSDLMoviePlayer::OpenDecoder(AVFormatContext *format, int type, Decoder &decoder)
{
    const int stream = av_find_best_stream(format,
        static_cast<AVMediaType>(type), -1, -1, nullptr, 0);
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

cSDLMoviePlayer::cSDLMoviePlayer(IUnknown *outer)
{
    INIT_AGGREGATION_1(outer, IID_IMoviePlayer1, this, kPriorityLibrary, nullptr);
}

cSDLMoviePlayer::~cSDLMoviePlayer()
{
    Close();
}

BOOL STDMETHODCALLTYPE cSDLMoviePlayer::Open(const char *name, int, int, int, int)
{
    Close();
    if (!name || !*name)
        return FALSE;

    const std::string path = NativeMoviePath(name);
    if (avformat_open_input(&m_format, path.c_str(), nullptr, nullptr) < 0 ||
        avformat_find_stream_info(m_format, nullptr) < 0 ||
        !OpenDecoder(m_format, AVMEDIA_TYPE_VIDEO, m_video))
    {
        std::fprintf(stderr, "Could not decode movie: %s\n", path.c_str());
        Close();
        return FALSE;
    }
    OpenDecoder(m_format, AVMEDIA_TYPE_AUDIO, m_audio);
    m_state = kMP1Ready;
    return TRUE;
}

void STDMETHODCALLTYPE cSDLMoviePlayer::Close()
{
    CloseDecoder(m_audio);
    CloseDecoder(m_video);
    avformat_close_input(&m_format);
    m_state = kMP1NothingOpen;
    m_hasPlayed = false;
}

BOOL STDMETHODCALLTYPE cSDLMoviePlayer::CanPlay()
{
    return m_state == kMP1Ready;
}

BOOL STDMETHODCALLTYPE cSDLMoviePlayer::Pause()
{
    return FALSE;
}

BOOL STDMETHODCALLTYPE cSDLMoviePlayer::Stop()
{
    m_stopRequested = true;
    return TRUE;
}

BOOL STDMETHODCALLTYPE cSDLMoviePlayer::GetVolume(long *volume)
{
    if (!volume)
        return FALSE;
    *volume = m_volume;
    return TRUE;
}

BOOL STDMETHODCALLTYPE cSDLMoviePlayer::SetVolume(long volume)
{
    m_volume = std::clamp(volume, -10000L, 0L);
    return TRUE;
}

eMP1State STDMETHODCALLTYPE cSDLMoviePlayer::GetState()
{
    return m_state;
}

void STDMETHODCALLTYPE cSDLMoviePlayer::SetTermKeys(char *keys)
{
    m_termKeys = keys ? keys : "";
    m_useTermKeys = keys != nullptr;
}

IMPLEMENT_SIMPLE_AGGREGATION_SELF_DELETE(cSDLMoviePlayer);

extern "C" tResult LGAPI _MoviePlayerCreate(REFIID, void **player,
    IUnknown *outer, REFCLSID, unsigned)
{
    if (!outer)
        return E_INVALIDARG;
    auto *created = new cSDLMoviePlayer(outer);
    if (player)
        *player = created;
    return created ? S_OK : E_FAIL;
}

BOOL STDMETHODCALLTYPE cSDLMoviePlayer::Play(int flags)
{
    if (m_state != kMP1Ready || !(flags & kMoviePlayBlock))
        return FALSE;
    AutoAppIPtr_(VulkanRenderer,renderer);
    if (!renderer)
        return FALSE;
    if (m_hasPlayed)
    {
        if (av_seek_frame(m_format, -1, 0, AVSEEK_FLAG_BACKWARD) < 0)
            return FALSE;
        avcodec_flush_buffers(m_video.context);
        if (m_audio.context)
            avcodec_flush_buffers(m_audio.context);
    }
    m_hasPlayed = true;
    m_stopRequested = false;
    m_state = kMP1Playing;
    cMoviePlaybackScope playbackScope;
    AVFormatContext *format = m_format;
    Decoder &video = m_video;
    Decoder &audio = m_audio;
    const bool allowSkip = !(flags & kMoviePlayNoStop);
    const char *termKeys = m_useTermKeys ? m_termKeys.c_str() : nullptr;
    SwsContext *scaler = nullptr;
    SwrContext *resampler = nullptr;
    AVFrame *frame = nullptr;
    AVPacket *packet = nullptr;
    cMovieAudioOutput audioOutput;
    bool audioAvailable = false;
    bool played = false;
    bool keepPlaying = true;
    int64_t firstVideoTimestamp = AV_NOPTS_VALUE;
    MovieClock::time_point start;

    std::fprintf(stderr, "Playing movie with FFmpeg: %s (%dx%d%s)\n",
        format->url, video.context->width, video.context->height,
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

    if (audio.context && audioOutput.Open(m_volume))
    {
        AVChannelLayout outputLayout = AV_CHANNEL_LAYOUT_STEREO;
        AVChannelLayout inputLayout = audio.context->ch_layout;
        if (inputLayout.nb_channels > 0 &&
            inputLayout.order == AV_CHANNEL_ORDER_UNSPEC)
            av_channel_layout_default(&inputLayout, inputLayout.nb_channels);
        const int resampleResult = swr_alloc_set_opts2(&resampler,
            &outputLayout, AV_SAMPLE_FMT_S16, 48000, &inputLayout,
            audio.context->sample_fmt, audio.context->sample_rate, 0, nullptr);
        if (resampleResult < 0 || !resampler || swr_init(resampler) < 0)
        {
            swr_free(&resampler);
            audioAvailable = false;
        }
        else
            audioAvailable = true;
        if (inputLayout.order != audio.context->ch_layout.order)
            av_channel_layout_uninit(&inputLayout);
    }

    while (keepPlaying && !m_stopRequested && av_read_frame(format, packet) >= 0)
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

        while (keepPlaying && !m_stopRequested && avcodec_receive_frame(decoder->context, frame) == 0)
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
                keepPlaying = WaitForFrame(start, std::max(seconds, 0.0),
                    allowSkip, termKeys);
                if (keepPlaying)
                {
                    played = renderer->PresentMovieFrame(
                        rgba.data(), video.context->width,
                        video.context->height, strides[0]);
                }
            }
            else if (audioAvailable && resampler)
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
                    if (!audioOutput.Submit(samples.data(), samples.size()))
                        audioAvailable = false;
                }
            }
            av_frame_unref(frame);
        }
    }

    if (audioAvailable && keepPlaying)
    {
        audioOutput.Finish();
        while (!audioOutput.IsDrained() && !DarkPumpMovieEvents(allowSkip, termKeys))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    swr_free(&resampler);
    sws_freeContext(scaler);
    av_packet_free(&packet);
    av_frame_free(&frame);
    m_state = kMP1Ready;
    return played;
}
