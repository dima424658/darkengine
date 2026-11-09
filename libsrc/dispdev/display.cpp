#pragma pack(push, 8)
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>
#pragma pack(pop)

#include <appagg.h>
#include <appapi.h>
#include <dispapi.h>
#include <fix.h>
#include <grd.h>
#include <mode.h>
#include <render_backend.h>

#include "dispbase.h"
#include "display_window.h"
#include "../lgd3d/runtime.h"
#include "../lgd3d/event_bridge.h"

#pragma pack(push, 8)
class cSDLDisplayDevice : public cDisplayDevice
{
public:
   cSDLDisplayDevice(IUnknown *outer, eDisplayDeviceKind kind, int flags)
      : cDisplayDevice(outer, kind, flags), m_kind(kind), m_options(flags)
   {
      for (int index = 0; index < 256; ++index)
      {
         const unsigned char value = static_cast<unsigned char>(index);
         m_palette[index * 3] = value;
         m_palette[index * 3 + 1] = value;
         m_palette[index * 3 + 2] = value;
      }
   }

   ~cSDLDisplayDevice() override
   {
      // Display-device instances are transient across logical mode changes.
      // The SDL window and Vulkan context are owned by the SDL display layer.
      Close();
   }

   eDisplayDeviceKind GetKind() override { return m_kind; }

   void SetKind(eDisplayDeviceKind kind, int flags) override
   {
      m_kind = kind;
      m_options = flags;
   }

   void SetKind2(eDisplayDeviceKind kind, int flags, GUID *) override
   {
      SetKind(kind, flags);
   }

   void GetKind2(eDisplayDeviceKind *kind, int *flags, GUID **guid) override
   {
      if (kind) *kind = m_kind;
      if (flags) *flags = m_options;
      if (guid) *guid = nullptr;
   }

   void GetInfo(sGrDeviceInfo *info, sGrModeInfo *modes) override
   {
      if (!info)
         return;

      info->id_maj = 0;
      info->id_min = 0;
      info->memory = 0;
      int supportedModes = 0;
      for (int index = 0; index < GRD_MODES; ++index)
      {
         sGrModeInfo mode = grd_mode_info[index];
         if (mode.bitDepth != 8 && mode.bitDepth != 15 && mode.bitDepth != 16)
            continue;

         mode.flags |= GRM_IS_SUPPORTED | GRM_IS_LINEAR | GRM_IS_NATIVE |
            GRM_CAN_WINDOW | GRM_CAN_FULLSCREEN;
         grd_mode_info[index].flags = mode.flags;
         info->modes[supportedModes++] = static_cast<short>(index);
         if (modes)
            modes[index] = mode;
      }
      info->modes[supportedModes] = -1;
   }

   HRESULT Open(sGrModeCap *cap, int) override
   {
      m_cap = cap;
      return S_OK;
   }

   HRESULT Close() override
   {
      // Dark closes the logical display device during mode changes. Keep the
      // native window and presentation context alive for menus, movies, and
      // gameplay.
      m_pixels.clear();
      m_mode = -1;
      if (m_cap)
         std::memset(m_cap, 0, sizeof(*m_cap));
      return S_OK;
   }

   HRESULT SetMode(eGrDispMode mode, int flags) override
   {
      if (!m_cap || mode < 0 || mode >= GRD_MODES)
         return E_FAIL;

      const sGrModeInfo &modeInfo = grd_mode_info[mode];
      if (modeInfo.bitDepth != 8 && modeInfo.bitDepth != 15 &&
          modeInfo.bitDepth != 16)
      {
         std::fprintf(stderr,
            "SDL SetMode rejected depth %d for mode %d (%dx%d).\n",
            modeInfo.bitDepth, mode, modeInfo.w, modeInfo.h);
         return E_FAIL;
      }

      const int rowBytes = modeInfo.w * (modeInfo.bitDepth == 8 ? 1 : 2);
      m_pixels.assign(static_cast<size_t>(rowBytes) * modeInfo.h, 0);
      m_cap->aspect = FIX_UNIT;
      m_cap->w = modeInfo.w;
      m_cap->h = modeInfo.h;
      m_cap->vbase = m_pixels.data();
      m_cap->vbase2 = nullptr;
      m_cap->rowBytes = rowBytes;

      IApplication *application = AppGetObj(IApplication);
      const char *title = application
         ? application->GetCaption() : "Dark Engine";
      const bool opened = DarkRenderOpen(modeInfo.w, modeInfo.h, title);
      SafeRelease(application);
      if (!opened || !SDLDisplaySetFullscreen(m_kind == kDispFullScreen))
      {
         std::fprintf(stderr,
            "SDL SetMode could not open the render surface (opened=%d).\n",
            opened);
         m_pixels.clear();
         std::memset(m_cap, 0, sizeof(*m_cap));
         return E_FAIL;
      }
      DarkRenderWindowResized();

      if (flags & MODE_CLEAR_BIT)
         std::fill(m_pixels.begin(), m_pixels.end(), 0);
      m_mode = mode;
      return S_OK;
   }

   eGrDispMode GetMode(sGrModeInfoEx *out) override
   {
      if (!out)
         return m_mode;

      std::memset(out, 0, sizeof(*out));
      if (m_mode < 0)
      {
         out->flags = kGrModeNotSet;
         return m_mode;
      }

      const sGrModeInfo &modeInfo = grd_mode_info[m_mode];
      out->mode = m_mode;
      out->width = modeInfo.w;
      out->height = modeInfo.h;
      out->rowBytes = m_cap->rowBytes;
      out->flags = kGrModeSupported | kGrModeLinear |
         kGrModeCanWindow | kGrModeCanFullscreen;
      if (m_kind == kDispWindowed)
         out->flags |= kGrModeIsWindowed;
      out->depth = modeInfo.bitDepth;
      out->redMask = modeInfo.bitDepth == 16 ? 0xf800 : 0x7c00;
      out->greenMask = modeInfo.bitDepth == 16 ? 0x07e0 : 0x03e0;
      out->blueMask = 0x001f;
      return m_mode;
   }

   BOOL SetPalette(unsigned start, unsigned count,
      const unsigned char *rgb) override
   {
      if (!rgb || start > 256 || count > 256 - start)
         return FALSE;
      std::memcpy(m_palette + start * 3, rgb, count * 3);
      return TRUE;
   }

   BOOL GetPalette(unsigned start, unsigned count,
      unsigned char *rgb) override
   {
      if (!rgb || start > 256 || count > 256 - start)
         return FALSE;
      std::memcpy(rgb, m_palette + start * 3, count * 3);
      return TRUE;
   }

   HRESULT PageFlip() override
   {
      Flush();
      return S_OK;
   }

   int Lock() override { return ++m_locks; }

   int Unlock() override
   {
      if (m_locks > 0)
         --m_locks;
      return m_locks;
   }

   int BreakLock() override
   {
      const int locks = m_locks;
      m_locks = 0;
      return locks;
   }

   void RestoreLock(int count) override { m_locks = count; }
   BOOL IsLocked() override { return m_locks != 0; }

   void Flush() override
   {
      if (m_mode < 0 || IsFlushingPaused())
         return;
      const sGrModeInfo &modeInfo = grd_mode_info[m_mode];
      DarkRenderPresent(m_pixels.data(), modeInfo.w, modeInfo.h,
         m_cap->rowBytes, modeInfo.bitDepth, m_palette);
   }

   void FlushRect(int, int, int, int) override { Flush(); }
   BOOL SetGamma(double gamma) override
   {
      RenderBackendSetGamma(static_cast<float>(gamma));
      return TRUE;
   }
   BOOL SetFlip(BOOL) override { return FALSE; }

private:
   eDisplayDeviceKind m_kind;
   int m_options{};
   int m_mode{-1};
   int m_locks{};
   sGrModeCap *m_cap{};
   std::vector<unsigned char> m_pixels;
   unsigned char m_palette[768]{};
};
static_assert(alignof(cSDLDisplayDevice) >= alignof(std::vector<unsigned char>),
   "SDL display storage must retain native vector alignment");
#pragma pack(pop)

extern "C" tResult LGAPI _DisplayDeviceCreate(REFIID, IDisplayDevice **,
   IUnknown *outer, eDisplayDeviceKind kind, int flags)
{
   return new cSDLDisplayDevice(outer, kind, flags) ? S_OK : E_FAIL;
}
