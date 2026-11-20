#include <windows.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <stdint.h>
#include <stdio.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "ole32.lib")

template <class T> static void Release(T *&value) {
  if (value) {
    value->Release();
    value = nullptr;
  }
}

static uint64_t HashSample(IMFSample *sample) {
  IMFMediaBuffer *buffer = nullptr;
  BYTE *bytes = nullptr;
  DWORD length = 0;
  uint64_t hash = 1469598103934665603ULL;
  if (FAILED(sample->ConvertToContiguousBuffer(&buffer)) ||
      FAILED(buffer->Lock(&bytes, nullptr, &length))) {
    Release(buffer);
    return 0;
  }
  // Sampling the buffer is sufficient to reject a decoder returning one
  // frozen/black frame without spending most of the probe hashing pixels.
  DWORD stride = length > 4096 ? length / 4096 : 1;
  for (DWORD offset = 0; offset < length; offset += stride) {
    hash ^= bytes[offset];
    hash *= 1099511628211ULL;
  }
  buffer->Unlock();
  Release(buffer);
  return hash;
}

int wmain(int argc, wchar_t **argv) {
  if (argc != 2) {
    fwprintf(stderr, L"usage: media_decode_probe <movie>\n");
    return 2;
  }

  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  const bool uninitializeCom = SUCCEEDED(hr);
  if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
    fwprintf(stderr, L"CoInitializeEx failed: 0x%08lx\n", hr);
    return 1;
  }
  hr = MFStartup(MF_VERSION);
  if (FAILED(hr)) {
    fwprintf(stderr, L"MFStartup failed: 0x%08lx\n", hr);
    if (uninitializeCom)
      CoUninitialize();
    return 1;
  }

  IMFAttributes *attributes = nullptr;
  IMFSourceReader *reader = nullptr;
  IMFMediaType *outputType = nullptr;
  hr = MFCreateAttributes(&attributes, 1);
  if (SUCCEEDED(hr))
    hr = attributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
  if (SUCCEEDED(hr))
    hr = MFCreateSourceReaderFromURL(argv[1], attributes, &reader);
  if (SUCCEEDED(hr))
    hr = reader->SetStreamSelection(
        static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE);
  if (SUCCEEDED(hr))
    hr = reader->SetStreamSelection(
        static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), TRUE);
  if (SUCCEEDED(hr))
    hr = MFCreateMediaType(&outputType);
  if (SUCCEEDED(hr))
    hr = outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  if (SUCCEEDED(hr))
    hr = outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
  if (SUCCEEDED(hr))
    hr = reader->SetCurrentMediaType(
        static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), nullptr,
        outputType);

  const LONGLONG targetTimestamp = 5LL * 1000 * 1000 * 10;
  LONGLONG firstTimestamp = -1;
  LONGLONG lastTimestamp = -1;
  uint64_t previousHash = 0;
  unsigned frames = 0;
  unsigned changedFrames = 0;
  LARGE_INTEGER frequency = {}, started = {}, finished = {};
  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&started);

  while (SUCCEEDED(hr) && lastTimestamp < targetTimestamp) {
    DWORD flags = 0;
    LONGLONG timestamp = 0;
    IMFSample *sample = nullptr;
    hr = reader->ReadSample(
        static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), 0, nullptr,
        &flags, &timestamp, &sample);
    if (SUCCEEDED(hr) && sample) {
      const uint64_t hash = HashSample(sample);
      if (firstTimestamp < 0)
        firstTimestamp = timestamp;
      lastTimestamp = timestamp;
      ++frames;
      if (frames == 1 || hash != previousHash)
        ++changedFrames;
      previousHash = hash;
    }
    Release(sample);
    if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
      break;
  }
  QueryPerformanceCounter(&finished);

  const double elapsed =
      double(finished.QuadPart - started.QuadPart) / frequency.QuadPart;
  const double mediaSeconds =
      lastTimestamp > firstTimestamp ? (lastTimestamp - firstTimestamp) / 1e7
                                     : 0.0;
  const double decodeFps = elapsed > 0.0 ? frames / elapsed : 0.0;
  wprintf(L"movie: %ls\n", argv[1]);
  wprintf(L"decoded: %u frames, %.3f media seconds in %.3f wall seconds\n",
          frames, mediaSeconds, elapsed);
  wprintf(L"throughput: %.1f frames/s; changed frames: %u\n", decodeFps,
          changedFrames);

  const bool passed = SUCCEEDED(hr) && frames >= 45 && mediaSeconds >= 4.0 &&
                      changedFrames >= 30 && elapsed < 10.0;
  if (!passed)
    fwprintf(stderr,
             L"FAIL: expected >=45 frames, >=4s timestamps, >=30 changed "
             L"frames, and <10s decode time (hr=0x%08lx)\n",
             hr);
  else
    wprintf(L"PASS\n");

  Release(outputType);
  Release(reader);
  Release(attributes);
  MFShutdown();
  if (uninitializeCom)
    CoUninitialize();
  return passed ? 0 : 1;
}
