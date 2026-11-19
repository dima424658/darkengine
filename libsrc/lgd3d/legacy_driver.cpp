#include <render_backend.h>
#include <dev2d.h>
#include <lgd3d.h>
#include <string.h>
#include <string>
#include "legacy_texture_driver.h"
#include <texture.h>
#include <tmgr.h>
#include <vector>
#include <material.h>

extern "C" {
texture_manager *g_tmgr = NULL;
}

// Dark's portal and model UVs routinely extend beyond the base texture and
// rely on the primary texture stage repeating.  Starting stage zero clamped
// turns every out-of-range surface into a stretched edge row/column until a
// later effect happens to enable wrapping.  Lightmaps occupy an atlas, so the
// secondary stage must remain clamped.
static BOOL g_wrap[2] = {TRUE, FALSE};
static grs_bitmap *g_defaultBitmap = NULL;
static std::vector<lgd3ds_device_info> g_deviceInfos;
static std::vector<std::string> g_deviceNames;
static short g_supportedModes[GRD_MODES + 1];

extern "C" {
extern int RenderBackendVulkanEnumeratePhysicalDevices(void);
extern const char *RenderBackendVulkanPhysicalDeviceName(int index);
extern void SetTextureId(int n);
extern void UnsetTextureId(int n);
extern void lgd3d_render_init(lgd3ds_device_info *info);
extern void lgd3d_render_shutdown(void);
extern void lgd3d_render_start_frame(void);
extern void lgd3d_render_end_frame(void);
extern void lgd3d_render_flush(void);
extern void lgd3d_reset_texture_bindings(void);
extern void lgd3d_set_znearfar(double znear, double zfar);
extern void ScrnClearHardwareOverlay(void);
}

static void InitTextureManager(lgd3ds_device_info *info) {
  g_tmgr = lgd3d_get_texture_manager();
  g_tmgr->init(g_defaultBitmap, LGD3D_MAX_TEXTURES, NULL, 0,
               (info->flags & LGD3DF_SPEW) ? TMGRF_SPEW : 0);
}

extern "C" int lgd3d_enumerate_devices(void) {
  // Enumeration happens before the platform renderer creates its output.
  int i;
  int supportedModeCount = gr_get_registered_mode_count(16);
  for (i = 0; i < supportedModeCount && i < GRD_MODES; ++i)
    g_supportedModes[i] = (short)gr_get_registered_mode(i, 16);
  g_supportedModes[i] = -1;
  const int deviceCount = RenderBackendVulkanEnumeratePhysicalDevices();
  if (deviceCount <= 0)
    return 0;
  try {
    g_deviceInfos.resize((size_t)deviceCount);
    g_deviceNames.resize((size_t)deviceCount);
    for (int device = 0; device < deviceCount; ++device) {
      const char *name = RenderBackendVulkanPhysicalDeviceName(device);
      g_deviceNames[(size_t)device] = name ? name : "Vulkan device";
    }
    for (int device = 0; device < deviceCount; ++device) {
      lgd3ds_device_info &info = g_deviceInfos[(size_t)device];
      memset(&info, 0, sizeof(info));
      info.supported_modes = g_supportedModes;
      info.name = g_deviceNames[(size_t)device].c_str();
      info.flags = LGD3DF_CAN_DO_ZBUFFER | LGD3DF_CAN_DO_VERTEX_FOG |
                   LGD3DF_CAN_DO_SINGLE_PASS_MT | LGD3DF_CAN_DO_WINDOWED |
                   LGD3DF_CAN_DO_ITERATE_ALPHA;
    }
    return deviceCount;
  } catch (...) {
    g_deviceInfos.clear();
    g_deviceNames.clear();
    return 0;
  }
}
extern "C" lgd3ds_device_info *lgd3d_get_device_info(int device) {
  return device >= 0 && (size_t)device < g_deviceInfos.size()
      ? &g_deviceInfos[(size_t)device] : NULL;
}

extern "C" BOOL lgd3d_init(lgd3ds_device_info *info) {
  if (!RenderBackendAvailable())
    return FALSE;
  ResMaterialReleaseRenderData();
  LegacyTexturesInit();
  g_defaultBitmap = gr_alloc_bitmap(BMT_FLAT8, 0, 2, 2);
  if (!g_defaultBitmap)
    return FALSE;
  memset(g_defaultBitmap->bits, 255, 4);
  lgd3d_set_texture_level(0);
  lgd3d_reset_texture_bindings();
  RenderBackendSetSampler(0, g_wrap[0], TRUE);
  RenderBackendSetSampler(1, g_wrap[1], TRUE);
  RenderBackendSetDepth(FALSE, FALSE);
  RenderBackendSetBlend(kRenderBackendBlendOpaque);
  RenderBackendSetAlphaTest(FALSE);
  info->flags |=
      LGD3DF_ZBUFFER | LGD3DF_MULTI_TEXTURING | LGD3DF_MULTITEXTURE_COLOR;
  InitTextureManager(info);
  lgd3d_render_init(info);
  // Initializing compatibility state does not begin or render a scene.
  RenderBackendDeactivateScene();
  return TRUE;
}
extern "C" void lgd3d_shutdown(void) {
  int i;
  RenderBackendDeactivateScene();
  ResMaterialReleaseRenderData();
  lgd3d_render_shutdown();
  if (g_tmgr) {
    g_tmgr->shutdown();
    g_tmgr = NULL;
  }
  for (i = 0; i < LGD3D_MAX_TEXTURES; ++i)
    LegacyTextureRelease(i);
  if (g_defaultBitmap)
    gr_free(g_defaultBitmap);
  g_defaultBitmap = NULL;
}
extern "C" void lgd3d_start_frame(int frame) {
  lgd3d_set_texture_level(0);
  ScrnClearHardwareOverlay();
  RenderBackendBeginScene();
  lgd3d_render_start_frame();
  if (g_tmgr)
    g_tmgr->start_frame(frame);
}
extern "C" void lgd3d_end_frame(void) {
  lgd3d_render_end_frame();
}
extern "C" void lgd3d_scene_suspend(void) { RenderBackendDeactivateScene(); }
extern "C" void lgd3d_clean_render_surface(BOOL depth) {
  if (depth)
    RenderBackendClearDepth();
}
extern "C" BOOL lgd3d_overlays_master_switch(BOOL on) {
  const BOOL previous = RenderBackendCanvasOverlayEnabled();
  RenderBackendSetCanvasOverlayEnabled(on);
  return previous;
}

extern "C" BOOL lgd3d_get_texture_wrapping(uint level) {
  return level < 2 ? g_wrap[level] : FALSE;
}
extern "C" BOOL lgd3d_set_texture_wrapping(uint level, BOOL wrap) {
  BOOL old;
  if (level >= 2)
    return FALSE;
  lgd3d_render_flush();
  old = g_wrap[level];
  g_wrap[level] = wrap;
  RenderBackendSetSampler(level, wrap, TRUE);
  return old;
}
