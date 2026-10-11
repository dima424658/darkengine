#include <aggmemb.h>
#include <movie_backend.h>
#include <movieapi.h>

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
    STDMETHOD_(BOOL, CanPlay)() override { return FALSE; }
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

bool MovieBackendPlaySynchronous(const char *, int)
{
    return false;
}
