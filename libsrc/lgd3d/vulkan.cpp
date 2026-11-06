#pragma pack(push, 8)
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#pragma pack(pop)

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <utility>
#include <vector>


#include <kb.h>
#include <mouse.h>
#include <render_backend.h>

#include "runtime.h"
#include "event_bridge.h"
#include "vulkan_context.hpp"
#include "vulkan_render_backend.hpp"
#include "vulkan_sdl.hpp"
#include "vulkan_scene_renderer.hpp"

extern "C" void DarkInputKey(uchar scan, int down);
extern "C" void DarkInputMouse(short x, short y, uchar type, uchar buttons,
   char wheel);
extern "C" void DarkInputMouseRelative(int x, int y, uchar buttons);

namespace
{
SDL_Window *g_window;
cVulkanContext g_context;
cVulkanSceneRenderer g_renderer;
vk::CommandBuffer g_activeCommandBuffer;
SDL_Cursor *g_invisibleCursor;
int g_width, g_height;
int g_clientWidth, g_clientHeight;
int g_windowedHintWidth, g_windowedHintHeight;
int g_drawableWidth, g_drawableHeight;
int g_canvasWidth, g_canvasHeight;
int g_canvasTextureWidth, g_canvasTextureHeight;
bool g_quit;
bool g_relativeRequested;
bool g_relativeActive;
bool g_focused;
float g_pumpRelativeX;
float g_pumpRelativeY;
std::atomic<bool> g_windowMinimized;
std::atomic<bool> g_appInBackground;
std::atomic<bool> g_lifecycleResizePending;
bool g_appEventWatchRegistered;
bool g_showNativeCursor;
bool g_frameActive;
bool g_canvasChanged;
bool g_resizePending;
bool g_exitHookRegistered;
uchar g_buttons;
void *g_canvasTexture;
std::vector<uint8_t> g_canvasPixels;

uchar key_scan(SDL_Scancode key)
{
   if (key >= SDL_SCANCODE_F1 && key <= SDL_SCANCODE_F10)
      return static_cast<uchar>(0x3b + key - SDL_SCANCODE_F1);
   switch (key) {
   case SDL_SCANCODE_ESCAPE: return 0x01; case SDL_SCANCODE_1: return 0x02; case SDL_SCANCODE_2: return 0x03;
   case SDL_SCANCODE_3: return 0x04; case SDL_SCANCODE_4: return 0x05; case SDL_SCANCODE_5: return 0x06;
   case SDL_SCANCODE_6: return 0x07; case SDL_SCANCODE_7: return 0x08; case SDL_SCANCODE_8: return 0x09;
   case SDL_SCANCODE_9: return 0x0a; case SDL_SCANCODE_0: return 0x0b; case SDL_SCANCODE_MINUS: return 0x0c;
   case SDL_SCANCODE_EQUALS: return 0x0d; case SDL_SCANCODE_BACKSPACE: return 0x0e; case SDL_SCANCODE_TAB: return 0x0f;
   case SDL_SCANCODE_Q: return 0x10; case SDL_SCANCODE_W: return 0x11; case SDL_SCANCODE_E: return 0x12;
   case SDL_SCANCODE_R: return 0x13; case SDL_SCANCODE_T: return 0x14; case SDL_SCANCODE_Y: return 0x15;
   case SDL_SCANCODE_U: return 0x16; case SDL_SCANCODE_I: return 0x17; case SDL_SCANCODE_O: return 0x18;
   case SDL_SCANCODE_P: return 0x19; case SDL_SCANCODE_LEFTBRACKET: return 0x1a; case SDL_SCANCODE_RIGHTBRACKET: return 0x1b;
   case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: return 0x1c;
   case SDL_SCANCODE_LCTRL: return KBC_LCTRL; case SDL_SCANCODE_RCTRL: return KBC_RCTRL;
   case SDL_SCANCODE_A: return 0x1e; case SDL_SCANCODE_S: return 0x1f; case SDL_SCANCODE_D: return 0x20;
   case SDL_SCANCODE_F: return 0x21; case SDL_SCANCODE_G: return 0x22; case SDL_SCANCODE_H: return 0x23;
   case SDL_SCANCODE_J: return 0x24; case SDL_SCANCODE_K: return 0x25; case SDL_SCANCODE_L: return 0x26;
   case SDL_SCANCODE_SEMICOLON: return 0x27; case SDL_SCANCODE_APOSTROPHE: return 0x28; case SDL_SCANCODE_GRAVE: return 0x29;
   case SDL_SCANCODE_LSHIFT: return KBC_LSHIFT; case SDL_SCANCODE_BACKSLASH: return 0x2b; case SDL_SCANCODE_Z: return 0x2c;
   case SDL_SCANCODE_X: return 0x2d; case SDL_SCANCODE_C: return 0x2e; case SDL_SCANCODE_V: return 0x2f;
   case SDL_SCANCODE_B: return 0x30; case SDL_SCANCODE_N: return 0x31; case SDL_SCANCODE_M: return 0x32;
   case SDL_SCANCODE_COMMA: return 0x33; case SDL_SCANCODE_PERIOD: return 0x34; case SDL_SCANCODE_SLASH: return 0x35;
   case SDL_SCANCODE_RSHIFT: return KBC_RSHIFT; case SDL_SCANCODE_LALT: return KBC_LALT; case SDL_SCANCODE_RALT: return KBC_RALT;
   case SDL_SCANCODE_SPACE: return 0x39; case SDL_SCANCODE_CAPSLOCK: return KBC_CAPS;
   case SDL_SCANCODE_NUMLOCKCLEAR: return KBC_NUM; case SDL_SCANCODE_SCROLLLOCK: return KBC_SCROLL;
   case SDL_SCANCODE_F11: return 0x57; case SDL_SCANCODE_F12: return 0x58;
   case SDL_SCANCODE_HOME: return 0xc7; case SDL_SCANCODE_UP: return 0xc8; case SDL_SCANCODE_PAGEUP: return 0xc9;
   case SDL_SCANCODE_LEFT: return 0xcb; case SDL_SCANCODE_RIGHT: return 0xcd; case SDL_SCANCODE_END: return 0xcf;
   case SDL_SCANCODE_DOWN: return 0xd0; case SDL_SCANCODE_PAGEDOWN: return 0xd1; case SDL_SCANCODE_INSERT: return 0xd2;
   case SDL_SCANCODE_DELETE: return 0xd3; default: return KBC_NONE;
   }
}


void update_window_dimensions()
{
   if (!g_window) return;
   (void)SDL_GetWindowSize(g_window, &g_clientWidth, &g_clientHeight);
   (void)SDL_GetWindowSizeInPixels(g_window, &g_drawableWidth,
      &g_drawableHeight);
   RenderBackendVulkanSetWindowMetrics(g_clientWidth, g_clientHeight,
      g_drawableWidth, g_drawableHeight);
   if (g_context.Device() && g_drawableWidth > 0 && g_drawableHeight > 0)
   {
      if (g_frameActive)
         g_resizePending = true;
      else
      {
         g_renderer.ResetPipelines();
         g_resizePending = !g_context.Resize(
            static_cast<uint32_t>(g_drawableWidth),
            static_cast<uint32_t>(g_drawableHeight));
      }
   }
}

bool SDLCALL app_lifecycle_event(void *, SDL_Event *event)
{
   if (!event)
      return true;
   switch (event->type)
   {
   case SDL_EVENT_WILL_ENTER_BACKGROUND:
   case SDL_EVENT_DID_ENTER_BACKGROUND:
      g_appInBackground.store(true, std::memory_order_relaxed);
      g_windowMinimized.store(true, std::memory_order_relaxed);
      break;
   case SDL_EVENT_DID_ENTER_FOREGROUND:
      g_appInBackground.store(false, std::memory_order_relaxed);
      g_lifecycleResizePending.store(true, std::memory_order_release);
      break;
   default:
      break;
   }
   return true;
}

void process_pending_lifecycle_resize()
{
   if (!g_lifecycleResizePending.exchange(false, std::memory_order_acq_rel))
      return;
   update_window_dimensions();
   if (!g_appInBackground.load(std::memory_order_relaxed))
      g_windowMinimized.store(false, std::memory_order_release);
}

bool apply_pending_resize()
{
   if (!g_resizePending || g_frameActive) return true;
   g_renderer.ResetPipelines();
   if (!g_context.Resize(static_cast<uint32_t>(g_drawableWidth),
         static_cast<uint32_t>(g_drawableHeight)))
      return false;
   g_resizePending = false;
   return true;
}

void logical_point(int &x, int &y)
{
   const int drawableX = g_clientWidth > 0
      ? x * g_drawableWidth / g_clientWidth : x;
   const int drawableY = g_clientHeight > 0
      ? y * g_drawableHeight / g_clientHeight : y;
   int viewportX, viewportY, viewportWidth, viewportHeight;
   RenderBackendVulkanGetViewport(g_width, g_height, g_drawableWidth,
      g_drawableHeight, 1, &viewportX, &viewportY, &viewportWidth,
      &viewportHeight);
   x = viewportWidth > 0
      ? (drawableX - viewportX) * g_width / viewportWidth : 0;
   y = viewportHeight > 0
      ? (drawableY - viewportY) * g_height / viewportHeight : 0;
   x = std::clamp(x, 0, std::max(g_width - 1, 0));
   y = std::clamp(y, 0, std::max(g_height - 1, 0));
}

void update_relative_mouse()
{
   if (!g_window) return;
   const bool shouldBeActive = g_relativeRequested && g_focused;
   if (SDL_GetWindowRelativeMouseMode(g_window) != shouldBeActive)
      (void)SDL_SetWindowRelativeMouseMode(g_window, shouldBeActive);
   g_relativeActive = SDL_GetWindowRelativeMouseMode(g_window);
   if (g_relativeActive || !g_showNativeCursor)
   {
      if (g_invisibleCursor) (void)SDL_SetCursor(g_invisibleCursor);
      (void)SDL_HideCursor();
   }
   else
   {
      if (g_invisibleCursor && SDL_GetCursor() == g_invisibleCursor)
         (void)SDL_SetCursor(SDL_GetDefaultCursor());
      (void)SDL_ShowCursor();
   }
}

void convert_canvas(const void *pixels, int width, int height, int rowBytes,
   int depth, const uint8_t *palette)
{
   if (!pixels || width <= 0 || height <= 0 || rowBytes <= 0) return;
   g_canvasWidth = width;
   g_canvasHeight = height;
   g_canvasPixels.resize(static_cast<size_t>(width) * height * 4);
   for (int y = 0; y < height; ++y)
   {
      const uint8_t *row = static_cast<const uint8_t *>(pixels) +
         static_cast<size_t>(y) * rowBytes;
      for (int x = 0; x < width; ++x)
      {
         uint32_t raw;
         uint8_t r, g, b;
         if (depth == 8)
         {
            raw = row[x];
            if (!palette) continue;
            r = palette[raw * 3]; g = palette[raw * 3 + 1];
            b = palette[raw * 3 + 2];
         }
         else
         {
            const uint16_t value = reinterpret_cast<const uint16_t *>(row)[x];
            raw = value;
            if (depth == 16)
            {
               r = ((value >> 11) & 31) * 255 / 31;
               g = ((value >> 5) & 63) * 255 / 63;
               b = (value & 31) * 255 / 31;
            }
            else
            {
               r = ((value >> 10) & 31) * 255 / 31;
               g = ((value >> 5) & 31) * 255 / 31;
               b = (value & 31) * 255 / 31;
            }
         }
         const size_t offset = (static_cast<size_t>(y) * width + x) * 4;
         g_canvasPixels[offset] = r;
         g_canvasPixels[offset + 1] = g;
         g_canvasPixels[offset + 2] = b;
         g_canvasPixels[offset + 3] =
            RenderBackendVulkanSceneActive() && raw == 0 ? 0 : 255;
      }
   }
   g_canvasChanged = true;
}

bool prepare_canvas_texture()
{
   if (g_canvasPixels.empty() || !g_canvasWidth || !g_canvasHeight)
      return true;
   const int rowBytes = g_canvasWidth * 4;
   if (!g_canvasTexture || g_canvasTextureWidth != g_canvasWidth ||
       g_canvasTextureHeight != g_canvasHeight)
   {
      if (g_canvasTexture)
         RenderBackendDestroyTexture(g_canvasTexture);
      g_canvasTexture = RenderBackendCreateTexture(g_canvasWidth,
         g_canvasHeight, g_canvasPixels.data(), rowBytes);
      if (!g_canvasTexture) return false;
      g_canvasTextureWidth = g_canvasWidth;
      g_canvasTextureHeight = g_canvasHeight;
      g_canvasChanged = false;
      return true;
   }
   if (g_canvasChanged)
   {
      if (!RenderBackendUpdateTexture(g_canvasTexture, g_canvasWidth,
            g_canvasHeight, g_canvasPixels.data(), rowBytes))
         return false;
      g_canvasChanged = false;
   }
   return true;
}

void draw_canvas(vk::CommandBuffer commandBuffer, bool transparent)
{
   if (!g_canvasTexture || !g_canvasWidth || !g_canvasHeight) return;
   g_renderer.SetDepth(false, false);
   g_renderer.SetBlend(kRenderBackendBlendAlpha);
   g_renderer.SetAlphaTest(false);
   g_renderer.SetFog(false, 0);
   g_renderer.SetSampler(0, false,
      RenderBackendVulkanUseLinearCanvasSampling() != 0);
   g_renderer.BindTexture(0, g_canvasTexture);
   int viewportX, viewportY, viewportWidth, viewportHeight;
   RenderBackendVulkanGetViewport(g_canvasWidth, g_canvasHeight,
      static_cast<int>(g_context.SwapchainExtent().width),
      static_cast<int>(g_context.SwapchainExtent().height), 1, &viewportX,
      &viewportY, &viewportWidth, &viewportHeight);
   g_renderer.SetViewport(viewportX, viewportY, viewportWidth, viewportHeight);
   const float w = static_cast<float>(g_canvasWidth);
   const float h = static_cast<float>(g_canvasHeight);
   const sRenderBackendVertex quad[] = {
      {0,0,0,1, 1,1,1,1, 1, 0,0, 0,0},
      {w,0,0,1, 1,1,1,1, 1, 1,0, 1,0},
      {w,h,0,1, 1,1,1,1, 1, 1,1, 1,1},
      {0,0,0,1, 1,1,1,1, 1, 0,0, 0,0},
      {w,h,0,1, 1,1,1,1, 1, 1,1, 1,1},
      {0,h,0,1, 1,1,1,1, 1, 0,1, 0,1}};
   (void)g_renderer.Draw(commandBuffer, quad, 6, kRenderBackendTriangles,
      false, g_canvasWidth, g_canvasHeight, transparent);
}
}

bool DarkRenderOpen(int width, int height, const char *title)
{
   if (g_window)
   {
      g_width = width; g_height = height;
      SDL_SetWindowTitle(g_window, title ? title : "Dark Engine");
      update_window_dimensions();
      return true;
   }
   if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
   {
      std::fprintf(stderr, "SDL3 initialization failed: %s\n", SDL_GetError());
      return false;
   }
   g_appInBackground.store(false, std::memory_order_relaxed);
   g_windowMinimized.store(false, std::memory_order_relaxed);
   g_lifecycleResizePending.store(false, std::memory_order_relaxed);
   if (!g_exitHookRegistered)
   {
      std::atexit(DarkRenderClose);
      g_exitHookRegistered = true;
   }
   g_width = width; g_height = height;
   g_window = SDL_CreateWindow(title ? title : "Dark Engine", width, height,
      SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
   if (!g_window)
   {
      std::fprintf(stderr, "SDL3 Vulkan window creation failed: %s\n", SDL_GetError());
      SDL_Quit();
      return false;
   }
   g_appEventWatchRegistered = SDL_AddEventWatch(app_lifecycle_event, nullptr);
   if (!g_appEventWatchRegistered)
      std::fprintf(stderr, "SDL lifecycle event watcher registration failed: %s\n",
         SDL_GetError());
   update_window_dimensions();
   char error[512]{};
   if (!VulkanContextInitializeSDL(g_context, g_window, error, sizeof(error)))
   {
      std::fprintf(stderr, "Vulkan initialization failed: %s\n", error);
      DarkRenderClose();
      return false;
   }
   const char *capturePath = std::getenv("DARK_RENDER_CAPTURE");
   const char *captureFrameEnv = std::getenv("DARK_RENDER_CAPTURE_FRAME");
   const unsigned long parsedCaptureFrame = captureFrameEnv
      ? std::strtoul(captureFrameEnv, nullptr, 10) : 1ul;
   const uint32_t captureFrame = parsedCaptureFrame > 0 &&
      parsedCaptureFrame <= UINT32_MAX
      ? static_cast<uint32_t>(parsedCaptureFrame) : 1u;
   g_context.ConfigureFrameCapture(capturePath, captureFrame);
#ifndef DARK_VULKAN_SHADER_DIR
#define DARK_VULKAN_SHADER_DIR "generated/vulkan"
#endif
   std::string shaderDir = DARK_VULKAN_SHADER_DIR;
#if !defined(__ANDROID__)
   const char *executablePath = SDL_GetBasePath();
   if (executablePath)
   {
      std::string executableShaderDir(executablePath);
      if (!executableShaderDir.empty() &&
          executableShaderDir.back() != '/' &&
          executableShaderDir.back() != '\\')
         executableShaderDir.push_back('/');
      executableShaderDir += DARK_VULKAN_SHADER_DIR;
      std::ifstream vertexShader(executableShaderDir + "/scene.vert.spv",
         std::ios::binary);
      if (vertexShader.good())
         shaderDir = std::move(executableShaderDir);
   }
#endif
   if (!g_renderer.Initialize(g_context,
         (shaderDir + "/scene.vert.spv").c_str(),
         (shaderDir + "/scene.frag.spv").c_str()))
   {
      std::fprintf(stderr, "Vulkan scene renderer initialization failed\n");
      DarkRenderClose();
      return false;
   }
   RenderBackendVulkanSetRenderer(&g_renderer);
   const Uint8 cursorBits[8]{};
   g_invisibleCursor = SDL_CreateCursor(cursorBits, cursorBits, 8, 8, 0, 0);
   g_focused = (SDL_GetWindowFlags(g_window) & SDL_WINDOW_INPUT_FOCUS) != 0;
   g_windowMinimized =
      (SDL_GetWindowFlags(g_window) & SDL_WINDOW_MINIMIZED) != 0;
   g_showNativeCursor = false;
   g_quit = false;
   update_relative_mouse();
   std::fprintf(stderr, "SDL3 video backend: %s (%dx%d points, %dx%d pixels)\n",
      SDL_GetCurrentVideoDriver(), g_clientWidth, g_clientHeight,
      g_drawableWidth, g_drawableHeight);
   return true;
}

void DarkRenderClose(void)
{
   if (g_appEventWatchRegistered)
   {
      SDL_RemoveEventWatch(app_lifecycle_event, nullptr);
      g_appEventWatchRegistered = false;
   }
   if (g_frameActive)
   {
      RenderBackendVulkanDetach();
      (void)g_context.EndFrame();
      g_frameActive = false;
   }
   RenderBackendVulkanSetRenderer(nullptr);
   if (g_canvasTexture)
   {
      g_renderer.DestroyTexture(g_canvasTexture);
      g_canvasTexture = nullptr;
   }
   g_renderer.Shutdown();
   g_context.Shutdown();
   if (g_invisibleCursor)
   {
      (void)SDL_SetCursor(SDL_GetDefaultCursor());
      SDL_DestroyCursor(g_invisibleCursor);
      g_invisibleCursor = nullptr;
   }
   if (g_window)
   {
      SDL_DestroyWindow(g_window);
      g_window = nullptr;
   }
   g_width = g_height = g_clientWidth = g_clientHeight = 0;
   g_drawableWidth = g_drawableHeight = 0;
   g_canvasWidth = g_canvasHeight = 0;
   g_canvasPixels.clear();
   g_canvasChanged = false; g_frameActive = false;
   g_canvasTextureWidth = g_canvasTextureHeight = 0;
   g_activeCommandBuffer = nullptr; g_resizePending = false;
   SDL_Quit();
}

bool DarkRenderSetFullscreen(bool enabled)
{
   if (!g_window) return false;
   if (!SDL_SetWindowFullscreen(g_window, enabled))
   {
      std::fprintf(stderr, "SDL fullscreen change failed: %s\n", SDL_GetError());
      return false;
   }
   update_window_dimensions();
   return true;
}

bool DarkRenderBeginEventPump(void)
{
   if (!g_window) return false;
   process_pending_lifecycle_resize();
   g_pumpRelativeX = g_pumpRelativeY = 0.0f;
   return true;
}

void DarkRenderHandleEvent(const SDL_Event *event)
{
   if (!event || !g_window) return;

   SDL_WindowID eventWindowId = 0;
   switch (event->type)
   {
   case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
   case SDL_EVENT_WINDOW_FOCUS_GAINED:
   case SDL_EVENT_WINDOW_FOCUS_LOST:
   case SDL_EVENT_WINDOW_MINIMIZED:
   case SDL_EVENT_WINDOW_RESTORED:
   case SDL_EVENT_WINDOW_RESIZED:
   case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
      eventWindowId = event->window.windowID; break;
   case SDL_EVENT_KEY_DOWN: case SDL_EVENT_KEY_UP:
      eventWindowId = event->key.windowID; break;
   case SDL_EVENT_MOUSE_MOTION:
      eventWindowId = event->motion.windowID; break;
   case SDL_EVENT_MOUSE_BUTTON_DOWN: case SDL_EVENT_MOUSE_BUTTON_UP:
      eventWindowId = event->button.windowID; break;
   case SDL_EVENT_MOUSE_WHEEL:
      eventWindowId = event->wheel.windowID; break;
   default: break;
   }
   if (eventWindowId && eventWindowId != SDL_GetWindowID(g_window))
      return;
   if (event->type == SDL_EVENT_QUIT ||
       event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
      g_quit = true;
   else if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED ||
            event->type == SDL_EVENT_WINDOW_RESIZED)
      update_window_dimensions();
   else if (event->type == SDL_EVENT_WINDOW_MINIMIZED)
   {
      g_windowMinimized = true;
      update_window_dimensions();
   }
   else if (event->type == SDL_EVENT_WINDOW_RESTORED)
   {
      g_windowMinimized = false;
      update_window_dimensions();
   }
   else if (event->type == SDL_EVENT_WINDOW_FOCUS_GAINED)
   {
      g_focused = true;
      g_windowMinimized = false;
      update_window_dimensions();
      update_relative_mouse();
   }
   else if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST)
   { g_focused = false; update_relative_mouse(); }
   else if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP)
   {
      const uchar scan = key_scan(event->key.scancode);
      if (scan != KBC_NONE && !event->key.repeat)
         DarkInputKey(scan, event->type == SDL_EVENT_KEY_DOWN);
   }
   else if (event->type == SDL_EVENT_MOUSE_MOTION)
   {
      if (g_relativeActive)
      { g_pumpRelativeX += event->motion.xrel; g_pumpRelativeY += event->motion.yrel; }
      else
      {
         int x = static_cast<int>(event->motion.x);
         int y = static_cast<int>(event->motion.y);
         logical_point(x, y);
         DarkInputMouse(static_cast<short>(x), static_cast<short>(y),
            MOUSE_MOTION, g_buttons, 0);
      }
   }
   else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
            event->type == SDL_EVENT_MOUSE_BUTTON_UP)
   {
      const int logical = event->button.button == SDL_BUTTON_LEFT ? 0 :
         event->button.button == SDL_BUTTON_RIGHT ? 1 :
         event->button.button == SDL_BUTTON_MIDDLE ? 2 : -1;
      if (logical >= 0)
      {
         const bool down = event->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
         const uchar bit = static_cast<uchar>(1 << logical);
         if (down) g_buttons |= bit; else g_buttons &= ~bit;
         const uchar type = static_cast<uchar>(1 << (1 + 2 * logical + !down));
         int x = static_cast<int>(event->button.x);
         int y = static_cast<int>(event->button.y);
         if (g_relativeActive) { x = g_width / 2; y = g_height / 2; }
         else logical_point(x, y);
         DarkInputMouse(static_cast<short>(x), static_cast<short>(y),
            type, g_buttons, 0);
      }
   }
   else if (event->type == SDL_EVENT_MOUSE_WHEEL)
   {
      int x = g_width / 2, y = g_height / 2;
      if (!g_relativeActive)
      {
         x = static_cast<int>(event->wheel.mouse_x);
         y = static_cast<int>(event->wheel.mouse_y);
         logical_point(x, y);
      }
      const int direction = event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1;
      const int wheel = std::clamp(static_cast<int>(std::lround(event->wheel.y)) * direction,
         -127, 127);
      if (wheel) DarkInputMouse(static_cast<short>(x), static_cast<short>(y),
         MOUSE_WHEEL, g_buttons, static_cast<char>(wheel));
   }
}

bool DarkRenderEndEventPump(void)
{
   process_pending_lifecycle_resize();
   if (g_relativeActive && (g_pumpRelativeX != 0 || g_pumpRelativeY != 0))
      DarkInputMouseRelative(static_cast<int>(g_pumpRelativeX),
         static_cast<int>(g_pumpRelativeY), g_buttons);
   update_relative_mouse();
   return g_quit;
}

bool DarkRenderPumpMovieEvents(void)
{
   if (!g_window) return true;
   process_pending_lifecycle_resize();
   SDL_Event event;
   bool stop = false;
   while (SDL_PollEvent(&event))
   {
      if (event.type == SDL_EVENT_QUIT ||
          event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
      { g_quit = true; stop = true; }
      else if (event.type == SDL_EVENT_KEY_DOWN ||
               event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
               event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
         stop = true;
      else if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
         update_window_dimensions();
      else if (event.type == SDL_EVENT_WINDOW_MINIMIZED)
      {
         g_windowMinimized = true;
         update_window_dimensions();
      }
      else if (event.type == SDL_EVENT_WINDOW_RESTORED ||
               event.type == SDL_EVENT_WINDOW_FOCUS_GAINED)
      {
         g_windowMinimized = false;
         update_window_dimensions();
      }
      else if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
      {
         g_focused = false;
         update_relative_mouse();
      }
   }
   process_pending_lifecycle_resize();
   return stop;
}

extern "C" int RenderBackendCaptureComplete(void)
{ return g_context.FrameCaptureComplete() ? 1 : 0; }

void DarkRenderBeginApplicationFrame(void)
{
   process_pending_lifecycle_resize();
   if (!g_window || g_frameActive ||
       g_windowMinimized.load(std::memory_order_acquire) ||
       g_drawableWidth <= 0 || g_drawableHeight <= 0)
      return;
   if (!apply_pending_resize()) return;
   if (!prepare_canvas_texture())
      std::fprintf(stderr, "Vulkan canvas texture update failed\n");
   vk::CommandBuffer commandBuffer{};
   if (!g_context.BeginFrame({0.0f, 0.0f, 0.0f, 1.0f}, &commandBuffer))
   {
      if (g_context.NeedsSwapchainRecreation())
         update_window_dimensions();
      return;
   }
   g_activeCommandBuffer = commandBuffer;
   g_renderer.BeginFrame(g_context.FrameIndex());
   RenderBackendVulkanAttach(commandBuffer, g_width, g_height, g_clientWidth,
      g_clientHeight, g_drawableWidth, g_drawableHeight);
   g_frameActive = true;
}

void DarkRenderEndApplicationFrame(void)
{
   if (!g_frameActive) return;
   draw_canvas(g_activeCommandBuffer, RenderBackendVulkanSceneActive() != 0);
   // Scene pixels are composited into this swapchain image only. The next
   // frame starts from a freshly acquired image, so don't let later canvas
   // updates treat palette index zero as transparent over a nonexistent
   // retained scene.
   RenderBackendDeactivateScene();
   RenderBackendVulkanDetach();
   (void)g_context.EndFrame();
   g_activeCommandBuffer = nullptr;
   g_frameActive = false;
   if (g_context.NeedsSwapchainRecreation())
      update_window_dimensions();
}

void DarkRenderSetTitle(const char *title)
{ if (g_window) SDL_SetWindowTitle(g_window, title ? title : "Dark Engine"); }
void DarkRenderShowCursor(bool show)
{ g_showNativeCursor = show; update_relative_mouse(); }
void DarkRenderSetMousePosition(int x, int y)
{
   if (g_window && !g_relativeActive && g_width > 0 && g_height > 0)
      (void)SDL_WarpMouseInWindow(g_window,
         static_cast<float>(x) * g_clientWidth / g_width,
         static_cast<float>(y) * g_clientHeight / g_height);
}
void DarkRenderSetRelativeMouse(bool enabled)
{ g_relativeRequested = enabled; update_relative_mouse(); }

extern "C" void RenderBackendSetWindowedClientSizeHint(int width, int height)
{
   g_windowedHintWidth = std::max(width, 0);
   g_windowedHintHeight = std::max(height, 0);
   if (!g_window || g_windowedHintWidth <= 0 || g_windowedHintHeight <= 0)
      return;
   if (SDL_GetWindowFlags(g_window) & SDL_WINDOW_FULLSCREEN)
      return;
   SDL_SetWindowSize(g_window, g_windowedHintWidth, g_windowedHintHeight);
   update_window_dimensions();
}

void DarkRenderPresent(const void *pixels, int width, int height, int rowBytes,
   int bitDepth, const uint8_t *palette)
{ convert_canvas(pixels, width, height, rowBytes, bitDepth, palette); }

bool DarkRenderPresentMovieFrame(const void *rgba, int width, int height,
   int rowBytes)
{
   if (!g_window || !rgba || width <= 0 || height <= 0 || rowBytes < width * 4)
      return false;
   std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);
   for (int y = 0; y < height; ++y)
      std::memcpy(pixels.data() + static_cast<size_t>(y) * width * 4,
         static_cast<const uint8_t *>(rgba) + static_cast<size_t>(y) * rowBytes,
         static_cast<size_t>(width) * 4);
   void *texture = g_renderer.CreateTexture(width, height, pixels.data(), width * 4);
   if (!texture) return false;
   vk::CommandBuffer commandBuffer{};
   const bool began = g_context.BeginFrame({0.0f,0.0f,0.0f,1.0f}, &commandBuffer);
   if (began)
   {
      g_renderer.BeginFrame(g_context.FrameIndex());
      g_renderer.SetDepth(false, false);
      g_renderer.SetBlend(kRenderBackendBlendOpaque);
      g_renderer.SetSampler(0, false, true);
      g_renderer.BindTexture(0, texture);
      const float w = static_cast<float>(width), h = static_cast<float>(height);
      const sRenderBackendVertex quad[] = {
         {0,0,0,1,1,1,1,1,1,0,0,0,0}, {w,0,0,1,1,1,1,1,1,1,0,0,0},
         {w,h,0,1,1,1,1,1,1,1,1,0,0}, {0,0,0,1,1,1,1,1,1,0,0,0,0},
         {w,h,0,1,1,1,1,1,1,1,1,0,0}, {0,h,0,1,1,1,1,1,1,0,1,0,0}};
      g_renderer.SetViewport(0, 0, static_cast<int>(g_context.SwapchainExtent().width),
         static_cast<int>(g_context.SwapchainExtent().height));
      (void)g_renderer.Draw(commandBuffer, quad, 6, kRenderBackendTriangles,
         false, width, height);
      (void)g_context.EndFrame();
   }
   g_renderer.DestroyTexture(texture);
   return began;
}
