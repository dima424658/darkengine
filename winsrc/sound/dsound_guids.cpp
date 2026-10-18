#include <win32_platform.h>
#include <dsound.h>
#include <initguid.h>

// These are queried by the dynamically loaded DirectSound provider and
// therefore cannot rely on a DirectSound import library to instantiate them.
DEFINE_GUID(IID_IDirectSound3DListener, 0x279AFA84, 0x4981, 0x11CE,
            0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60);
DEFINE_GUID(IID_IDirectSound3DBuffer, 0x279AFA86, 0x4981, 0x11CE,
            0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60);
