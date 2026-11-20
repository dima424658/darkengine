#ifndef DARK_ACTMOVIE_MOVIE_H
#define DARK_ACTMOVIE_MOVIE_H

#include <string>

#include <aggmemb.h>
#include <movieapi.h>

struct AVFormatContext;
struct AVCodecContext;

class cSDLMoviePlayer final : public IMoviePlayer1
{
    DECLARE_SIMPLE_AGGREGATION(cSDLMoviePlayer);

public:
    explicit cSDLMoviePlayer(IUnknown *outer);
    ~cSDLMoviePlayer();
    STDMETHOD_(BOOL, Open)(const char *name, int, int, int, int) override;
    STDMETHOD_(void, Close)() override;
    STDMETHOD_(BOOL, CanPlay)() override;
    STDMETHOD_(BOOL, Play)(int flags) override;
    STDMETHOD_(BOOL, Pause)() override;
    STDMETHOD_(BOOL, Stop)() override;
    STDMETHOD_(BOOL, GetVolume)(long *volume) override;
    STDMETHOD_(BOOL, SetVolume)(long volume) override;
    STDMETHOD_(eMP1State, GetState)() override;
    STDMETHOD_(void, SetTermKeys)(char *keys) override;

private:
    struct Decoder
    {
        AVCodecContext *context{};
        int stream{-1};
    };

    static void CloseDecoder(Decoder &decoder);
    static bool OpenDecoder(AVFormatContext *format, int type, Decoder &decoder);

    AVFormatContext *m_format{};
    Decoder m_video;
    Decoder m_audio;
    eMP1State m_state{kMP1NothingOpen};
    long m_volume{};
    bool m_hasPlayed{};
    bool m_stopRequested{};
    bool m_useTermKeys{};
    std::string m_termKeys;
};

#endif
