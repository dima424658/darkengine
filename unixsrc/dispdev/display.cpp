#pragma pack(push, 8)
#include <algorithm>
#include <cstring>
#include <vector>
#pragma pack(pop)

#include <appagg.h>
#include <appapi.h>
#include <dispapi.h>
#include "../../libsrc/dispdev/dispbase.h"
#include <fix.h>
#include <grd.h>
#include <mode.h>

#include "../render/runtime.h"

class cUnixDisplayDevice : public cDisplayDevice
{
public:
   cUnixDisplayDevice(IUnknown *outer, eDisplayDeviceKind kind, int flags)
      : cDisplayDevice(outer, kind, flags), m_kind(kind), m_options(flags)
   {
      for (int i = 0; i < 256; ++i) m_palette[i*3] = m_palette[i*3+1] = m_palette[i*3+2] = static_cast<unsigned char>(i);
   }
   ~cUnixDisplayDevice() override
   {
      // Display-device objects are transient across Dark's logical canvas
      // changes.  The SDL window and GL context are process-owned and must
      // survive those changes; the OS releases them at process shutdown.
      Close();
   }

   eDisplayDeviceKind GetKind() override { return m_kind; }
   void SetKind(eDisplayDeviceKind kind, int flags) override { m_kind = kind; m_options = flags; }
   void SetKind2(eDisplayDeviceKind kind, int flags, GUID *) override { SetKind(kind, flags); }
   void GetKind2(eDisplayDeviceKind *kind, int *flags, GUID **guid) override
   { if(kind)*kind=m_kind;if(flags)*flags=m_options;if(guid)*guid=nullptr; }

   void GetInfo(sGrDeviceInfo *info, sGrModeInfo *modes) override
   {
      if (!info) return; info->id_maj = info->id_min = 0; info->memory = 0; int n = 0;
      for (int i = 0; i < GRD_MODES; ++i) {
         sGrModeInfo mode = grd_mode_info[i];
         if (mode.bitDepth != 8 && mode.bitDepth != 15 && mode.bitDepth != 16) continue;
         mode.flags |= GRM_IS_SUPPORTED | GRM_IS_LINEAR | GRM_IS_NATIVE | GRM_CAN_WINDOW | GRM_CAN_FULLSCREEN;
         grd_mode_info[i].flags = mode.flags; info->modes[n] = static_cast<short>(i); if (modes) modes[i] = mode; ++n;
      }
      info->modes[n] = -1;
   }
   HRESULT Open(sGrModeCap *cap, int) override { m_cap = cap; return S_OK; }
   HRESULT Close() override
   {
      // Dark treats this as a logical display-device close during ordinary
      // mode changes.  Keep the native window and GL context alive so menus,
      // movies, and gameplay all use one presentation surface.
      m_pixels.clear(); m_mode = -1;
      if (m_cap) std::memset(m_cap, 0, sizeof(*m_cap)); return S_OK;
   }
   HRESULT SetMode(eGrDispMode mode, int flags) override
   {
      if (!m_cap || mode < 0 || mode >= GRD_MODES) return E_FAIL;
      const sGrModeInfo &info = grd_mode_info[mode];
      if (info.bitDepth != 8 && info.bitDepth != 15 && info.bitDepth != 16) return E_FAIL;
      const int rowBytes = info.w * (info.bitDepth == 8 ? 1 : 2);
      m_pixels.assign(static_cast<size_t>(rowBytes) * info.h, 0);
      m_cap->aspect = FIX_UNIT; m_cap->w = info.w; m_cap->h = info.h;
      m_cap->vbase = m_pixels.data(); m_cap->vbase2 = nullptr; m_cap->rowBytes = rowBytes;
      IApplication *app = AppGetObj(IApplication); const char *title = app ? app->GetCaption() : "Dark Engine";
      bool opened = DarkRenderOpen(info.w, info.h, title); SafeRelease(app);
      if (!opened || !DarkRenderSetFullscreen(m_kind == kDispFullScreen)) {
         m_pixels.clear(); std::memset(m_cap, 0, sizeof(*m_cap)); return E_FAIL;
      }
      if (flags & MODE_CLEAR_BIT) std::fill(m_pixels.begin(), m_pixels.end(), 0);
      m_mode = mode; return S_OK;
   }
   eGrDispMode GetMode(sGrModeInfoEx *out) override
   {
      if (out) {
         std::memset(out, 0, sizeof(*out));
         if (m_mode < 0) out->flags = kGrModeNotSet;
         else { const auto &info=grd_mode_info[m_mode];out->mode=m_mode;out->width=info.w;out->height=info.h;
            out->rowBytes=m_cap->rowBytes;out->flags=kGrModeSupported|kGrModeLinear|kGrModeCanWindow|kGrModeCanFullscreen;
            if (m_kind == kDispWindowed) out->flags |= kGrModeIsWindowed;
            out->depth=info.bitDepth;out->redMask=info.bitDepth==16?0xf800:0x7c00;out->greenMask=info.bitDepth==16?0x07e0:0x03e0;out->blueMask=0x001f; }
      }
      return m_mode;
   }
   BOOL SetPalette(unsigned start, unsigned count, const unsigned char *rgb) override
   { if(!rgb||start+count>256)return FALSE;std::memcpy(m_palette+start*3,rgb,count*3);return TRUE; }
   BOOL GetPalette(unsigned start, unsigned count, unsigned char *rgb) override
   { if(!rgb||start+count>256)return FALSE;std::memcpy(rgb,m_palette+start*3,count*3);return TRUE; }
   HRESULT PageFlip() override { Flush(); return S_OK; }
   int Lock() override { return ++m_locks; }
   int Unlock() override { if(m_locks>0)--m_locks; return m_locks; }
   int BreakLock() override { int result=m_locks;m_locks=0;return result; }
   void RestoreLock(int count) override { m_locks=count; }
   BOOL IsLocked() override { return m_locks != 0; }
   void Flush() override
   {
      if(m_mode<0||IsFlushingPaused())return;const auto &info=grd_mode_info[m_mode];
      DarkRenderPresent(m_pixels.data(),info.w,info.h,m_cap->rowBytes,info.bitDepth,m_palette);
   }
   void FlushRect(int,int,int,int) override { Flush(); }
   BOOL SetGamma(double) override { return TRUE; }
   BOOL SetFlip(BOOL) override { return FALSE; }
private:
   eDisplayDeviceKind m_kind; int m_options{}; int m_mode{-1}; int m_locks{};
   sGrModeCap *m_cap{}; std::vector<unsigned char> m_pixels; unsigned char m_palette[768]{};
};

extern "C" tResult LGAPI _DisplayDeviceCreate(REFIID, IDisplayDevice **,
   IUnknown *outer, eDisplayDeviceKind kind, int flags)
{
   return new cUnixDisplayDevice(outer, kind, flags) ? S_OK : E_FAIL;
}
