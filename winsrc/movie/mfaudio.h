#ifndef __MFAUDIO_H
#define __MFAUDIO_H

#include <win32_platform.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mmsystem.h>
#include <math.h>

#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "winmm.lib")

// Media Foundation's audio renderer becomes the presentation clock for a
// session and, on some devices, advances these 22.05 kHz movie tracks at a
// fraction of real time. Decode audio to PCM and let waveOut perform the device
// conversion while the video-only media session keeps the system clock.
class cMFMovieAudio
{
public:
    cMFMovieAudio()
        : m_thread(NULL), m_stop(NULL), m_play(NULL), m_ready(NULL),
          m_result(E_FAIL), m_volume(0)
    {
        m_path[0] = L'\0';
    }

    ~cMFMovieAudio() { Stop(); }

    BOOL Prepare(const WCHAR *path, int volume)
    {
        Stop();
        if (!path || wcslen(path) >= MAX_PATH)
            return FALSE;
        wcscpy_s(m_path, MAX_PATH, path);
        m_volume = volume;
        m_stop = CreateEvent(NULL, TRUE, FALSE, NULL);
        m_play = CreateEvent(NULL, TRUE, FALSE, NULL);
        m_ready = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!m_stop || !m_play || !m_ready)
        {
            Stop();
            return FALSE;
        }
        m_thread = CreateThread(NULL, 0, ThreadEntry, this, 0, NULL);
        if (!m_thread || WaitForSingleObject(m_ready, 5000) != WAIT_OBJECT_0 ||
            FAILED(m_result))
        {
            Stop();
            return FALSE;
        }
        return TRUE;
    }

    void Play()
    {
        if (m_play)
            SetEvent(m_play);
    }

    void Stop()
    {
        if (m_stop)
            SetEvent(m_stop);
        if (m_play)
            SetEvent(m_play);
        if (m_thread)
        {
            WaitForSingleObject(m_thread, INFINITE);
            CloseHandle(m_thread);
        }
        if (m_ready)
            CloseHandle(m_ready);
        if (m_play)
            CloseHandle(m_play);
        if (m_stop)
            CloseHandle(m_stop);
        m_thread = NULL;
        m_ready = NULL;
        m_play = NULL;
        m_stop = NULL;
    }

private:
    enum { kBufferCount = 16, kPrefillCount = 8 };

    struct sAudioBuffer
    {
        WAVEHDR header;
        BYTE *data;
        BOOL prepared;
    };

    static DWORD WINAPI ThreadEntry(LPVOID context)
    {
        return ((cMFMovieAudio *)context)->ThreadMain();
    }

    static void ClearBuffer(HWAVEOUT waveOut, sAudioBuffer &buffer)
    {
        if (buffer.prepared)
            waveOutUnprepareHeader(waveOut, &buffer.header,
                                   sizeof(buffer.header));
        delete[] buffer.data;
        ZeroMemory(&buffer, sizeof(buffer));
    }

    HRESULT QueueSample(IMFSourceReader *reader, HWAVEOUT waveOut,
                        sAudioBuffer &buffer, BOOL *endOfStream)
    {
        IMFSample *sample = NULL;
        IMFMediaBuffer *mediaBuffer = NULL;
        BYTE *source = NULL;
        DWORD length = 0;
        DWORD flags = 0;
        LONGLONG timestamp = 0;
        HRESULT result = reader->ReadSample(
            (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, NULL, &flags,
            &timestamp,
            &sample);
        if (SUCCEEDED(result) && (flags & MF_SOURCE_READERF_ENDOFSTREAM))
            *endOfStream = TRUE;
        if (SUCCEEDED(result) && sample)
            result = sample->ConvertToContiguousBuffer(&mediaBuffer);
        if (SUCCEEDED(result) && mediaBuffer)
            result = mediaBuffer->Lock(&source, NULL, &length);
        if (SUCCEEDED(result) && length)
        {
            buffer.data = new BYTE[length];
            if (!buffer.data)
                result = E_OUTOFMEMORY;
            else
            {
                memcpy(buffer.data, source, length);
                buffer.header.lpData = (LPSTR)buffer.data;
                buffer.header.dwBufferLength = length;
                MMRESULT mmResult = waveOutPrepareHeader(
                    waveOut, &buffer.header, sizeof(buffer.header));
                if (mmResult == MMSYSERR_NOERROR)
                {
                    buffer.prepared = TRUE;
                    mmResult = waveOutWrite(waveOut, &buffer.header,
                                            sizeof(buffer.header));
                }
                if (mmResult != MMSYSERR_NOERROR)
                    result = E_FAIL;
            }
        }
        if (source)
            mediaBuffer->Unlock();
        if (mediaBuffer)
            mediaBuffer->Release();
        if (sample)
            sample->Release();
        return result;
    }

    DWORD ThreadMain()
    {
        IMFSourceReader *reader = NULL;
        IMFMediaType *requestedType = NULL;
        IMFMediaType *actualType = NULL;
        WAVEFORMATEX *waveFormat = NULL;
        UINT32 waveFormatSize = 0;
        HWAVEOUT waveOut = NULL;
        sAudioBuffer buffers[kBufferCount];
        ZeroMemory(buffers, sizeof(buffers));
        HRESULT result = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        BOOL uninitializeCom = SUCCEEDED(result);
        if (result == RPC_E_CHANGED_MODE)
            result = S_OK;
        if (SUCCEEDED(result))
            result = MFCreateSourceReaderFromURL(m_path, NULL, &reader);
        if (SUCCEEDED(result))
            result = reader->SetStreamSelection(
                (DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE);
        if (SUCCEEDED(result))
            result = reader->SetStreamSelection(
                (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
        if (SUCCEEDED(result))
            result = MFCreateMediaType(&requestedType);
        if (SUCCEEDED(result))
            result = requestedType->SetGUID(MF_MT_MAJOR_TYPE,
                                             MFMediaType_Audio);
        if (SUCCEEDED(result))
            result = requestedType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        if (SUCCEEDED(result))
            result = reader->SetCurrentMediaType(
                (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL,
                requestedType);
        if (SUCCEEDED(result))
            result = reader->GetCurrentMediaType(
                (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &actualType);
        if (SUCCEEDED(result))
            result = MFCreateWaveFormatExFromMFMediaType(
                actualType, &waveFormat, &waveFormatSize, 0);
        if (SUCCEEDED(result))
        {
            MMRESULT mmResult = waveOutOpen(&waveOut, WAVE_MAPPER, waveFormat,
                                             0, 0, CALLBACK_NULL);
            if (mmResult != MMSYSERR_NOERROR)
                result = E_FAIL;
        }
        if (SUCCEEDED(result))
        {
            double scalar = m_volume <= -10000
                                ? 0.0
                                : pow(10.0, (double)m_volume / 2000.0);
            if (scalar > 1.0)
                scalar = 1.0;
            WORD channelVolume = (WORD)(scalar * 65535.0);
            waveOutSetVolume(waveOut,
                             channelVolume | ((DWORD)channelVolume << 16));
            waveOutPause(waveOut);
        }

        BOOL endOfStream = FALSE;
        int queued = 0;
        while (SUCCEEDED(result) && !endOfStream && queued < kPrefillCount)
        {
            result = QueueSample(reader, waveOut, buffers[queued],
                                 &endOfStream);
            if (buffers[queued].prepared)
                ++queued;
        }
        m_result = result;
        SetEvent(m_ready);

        HANDLE startHandles[2] = { m_stop, m_play };
        if (SUCCEEDED(result) &&
            WaitForMultipleObjects(2, startHandles, FALSE, INFINITE) ==
                WAIT_OBJECT_0 + 1)
        {
            if (waveOutRestart(waveOut) != MMSYSERR_NOERROR)
                result = E_FAIL;
            int nextBuffer = queued % kBufferCount;
            while (!endOfStream &&
                   WaitForSingleObject(m_stop, 0) != WAIT_OBJECT_0)
            {
                sAudioBuffer &buffer = buffers[nextBuffer];
                if (buffer.prepared && !(buffer.header.dwFlags & WHDR_DONE))
                {
                    WaitForSingleObject(m_stop, 5);
                    continue;
                }
                ClearBuffer(waveOut, buffer);
                result = QueueSample(reader, waveOut, buffer, &endOfStream);
                if (FAILED(result))
                    break;
                nextBuffer = (nextBuffer + 1) % kBufferCount;
            }
            while (WaitForSingleObject(m_stop, 0) != WAIT_OBJECT_0)
            {
                BOOL pending = FALSE;
                for (int i = 0; i < kBufferCount; ++i)
                    if (buffers[i].prepared &&
                        !(buffers[i].header.dwFlags & WHDR_DONE))
                        pending = TRUE;
                if (!pending)
                    break;
                WaitForSingleObject(m_stop, 10);
            }
        }

        if (waveOut)
        {
            waveOutReset(waveOut);
            for (int i = 0; i < kBufferCount; ++i)
                ClearBuffer(waveOut, buffers[i]);
            waveOutClose(waveOut);
        }
        CoTaskMemFree(waveFormat);
        if (actualType)
            actualType->Release();
        if (requestedType)
            requestedType->Release();
        if (reader)
            reader->Release();
        if (uninitializeCom)
            CoUninitialize();
        return 0;
    }

    WCHAR m_path[MAX_PATH];
    HANDLE m_thread;
    HANDLE m_stop;
    HANDLE m_play;
    HANDLE m_ready;
    volatile HRESULT m_result;
    int m_volume;

    cMFMovieAudio(const cMFMovieAudio &);
    cMFMovieAudio &operator=(const cMFMovieAudio &);
};

#endif
