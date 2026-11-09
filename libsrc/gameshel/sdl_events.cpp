#pragma pack(push, 8)
#include <SDL3/SDL.h>
#pragma pack(pop)

#include <algorithm>
#include <cmath>

#include <kb.h>
#include <mouse.h>

#include "sdl_events.h"
#include "../dispdev/sdl_window.h"
#include "../lgd3d/event_bridge.h"
#include <render_backend.h>

extern "C" void DarkInputKey(uchar scan, int down);
extern "C" void DarkInputMouse(short x, short y, uchar type, uchar buttons,
   char wheel);
extern "C" void DarkInputMouseRelative(int x, int y, uchar buttons);

#pragma pack(push, 8)
namespace
{
class cSDLEventPump
{
public:
   ~cSDLEventPump()
   {
      if (m_lifecycleWatchRegistered)
         SDL_RemoveEventWatch(LifecycleEvent, nullptr);
   }

   bool Pump()
   {
      ResetForWindow();
      if (!DarkRenderBeginEventPump())
         return false;
      EnsureLifecycleWatch();

      SDL_Event event;
      while (SDL_PollEvent(&event))
         HandleEvent(event);

      FlushMotion();
      if (SDLDisplayRelativeMouseActive())
      {
         const int relativeX = static_cast<int>(m_relativeX);
         const int relativeY = static_cast<int>(m_relativeY);
         m_relativeX -= relativeX;
         m_relativeY -= relativeY;
         if (relativeX || relativeY)
            DarkInputMouseRelative(relativeX, relativeY, m_buttons);
      }
      else
         m_relativeX = m_relativeY = 0;
      DarkRenderEndEventPump();
      return m_quit;
   }

   bool PumpMovie()
   {
      ResetForWindow();
      if (!SDLDisplayGetWindow())
         return true;
      EnsureLifecycleWatch();

      SDL_Event event;
      bool stop = false;
      while (SDL_PollEvent(&event))
      {
         if (event.type == SDL_EVENT_QUIT ||
             event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
         {
            m_quit = true;
            stop = true;
         }
         else if (event.type == SDL_EVENT_KEY_DOWN ||
                  event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                  event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
            stop = true;
         else
            HandleEvent(event);
      }
      return stop;
   }

private:
   static bool SDLCALL LifecycleEvent(void *, SDL_Event *event)
   {
      if (!event)
         return true;
      switch (event->type)
      {
      case SDL_EVENT_WILL_ENTER_BACKGROUND:
      case SDL_EVENT_DID_ENTER_BACKGROUND:
         DarkRenderApplicationSuspended();
         break;
      case SDL_EVENT_DID_ENTER_FOREGROUND:
         DarkRenderApplicationResumed();
         break;
      default:
         break;
      }
      return true;
   }

   void EnsureLifecycleWatch()
   {
      if (m_lifecycleWatchRegistered)
         return;
      m_lifecycleWatchRegistered =
         SDL_AddEventWatch(LifecycleEvent, nullptr);
      if (!m_lifecycleWatchRegistered)
         SDL_Log("SDL lifecycle event watcher registration failed: %s",
            SDL_GetError());
   }

   void ResetForWindow()
   {
      const SDL_WindowID id = SDLDisplayGetWindowId();
      if (id == m_windowId)
         return;

      m_windowId = id;
      m_quit = false;
      m_relativeX = m_relativeY = 0;
      m_pendingMotion = false;
      m_buttons = 0;
   }

   static uchar KeyScan(SDL_Scancode key)
   {
      if (key >= SDL_SCANCODE_F1 && key <= SDL_SCANCODE_F10)
         return static_cast<uchar>(0x3b + key - SDL_SCANCODE_F1);
      switch (key)
      {
      case SDL_SCANCODE_ESCAPE: return 0x01;
      case SDL_SCANCODE_1: return 0x02; case SDL_SCANCODE_2: return 0x03;
      case SDL_SCANCODE_3: return 0x04; case SDL_SCANCODE_4: return 0x05;
      case SDL_SCANCODE_5: return 0x06; case SDL_SCANCODE_6: return 0x07;
      case SDL_SCANCODE_7: return 0x08; case SDL_SCANCODE_8: return 0x09;
      case SDL_SCANCODE_9: return 0x0a; case SDL_SCANCODE_0: return 0x0b;
      case SDL_SCANCODE_MINUS: return 0x0c; case SDL_SCANCODE_EQUALS: return 0x0d;
      case SDL_SCANCODE_BACKSPACE: return 0x0e; case SDL_SCANCODE_TAB: return 0x0f;
      case SDL_SCANCODE_Q: return 0x10; case SDL_SCANCODE_W: return 0x11;
      case SDL_SCANCODE_E: return 0x12; case SDL_SCANCODE_R: return 0x13;
      case SDL_SCANCODE_T: return 0x14; case SDL_SCANCODE_Y: return 0x15;
      case SDL_SCANCODE_U: return 0x16; case SDL_SCANCODE_I: return 0x17;
      case SDL_SCANCODE_O: return 0x18; case SDL_SCANCODE_P: return 0x19;
      case SDL_SCANCODE_LEFTBRACKET: return 0x1a;
      case SDL_SCANCODE_RIGHTBRACKET: return 0x1b;
      case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: return 0x1c;
      case SDL_SCANCODE_LCTRL: return KBC_LCTRL;
      case SDL_SCANCODE_RCTRL: return KBC_RCTRL;
      case SDL_SCANCODE_A: return 0x1e; case SDL_SCANCODE_S: return 0x1f;
      case SDL_SCANCODE_D: return 0x20; case SDL_SCANCODE_F: return 0x21;
      case SDL_SCANCODE_G: return 0x22; case SDL_SCANCODE_H: return 0x23;
      case SDL_SCANCODE_J: return 0x24; case SDL_SCANCODE_K: return 0x25;
      case SDL_SCANCODE_L: return 0x26; case SDL_SCANCODE_SEMICOLON: return 0x27;
      case SDL_SCANCODE_APOSTROPHE: return 0x28; case SDL_SCANCODE_GRAVE: return 0x29;
      case SDL_SCANCODE_LSHIFT: return KBC_LSHIFT;
      case SDL_SCANCODE_BACKSLASH: return 0x2b; case SDL_SCANCODE_Z: return 0x2c;
      case SDL_SCANCODE_X: return 0x2d; case SDL_SCANCODE_C: return 0x2e;
      case SDL_SCANCODE_V: return 0x2f; case SDL_SCANCODE_B: return 0x30;
      case SDL_SCANCODE_N: return 0x31; case SDL_SCANCODE_M: return 0x32;
      case SDL_SCANCODE_COMMA: return 0x33; case SDL_SCANCODE_PERIOD: return 0x34;
      case SDL_SCANCODE_SLASH: return 0x35;
      case SDL_SCANCODE_RSHIFT: return KBC_RSHIFT;
      case SDL_SCANCODE_LALT: return KBC_LALT;
      case SDL_SCANCODE_RALT: return KBC_RALT;
      case SDL_SCANCODE_SPACE: return 0x39;
      case SDL_SCANCODE_CAPSLOCK: return KBC_CAPS;
      case SDL_SCANCODE_NUMLOCKCLEAR: return KBC_NUM;
      case SDL_SCANCODE_SCROLLLOCK: return KBC_SCROLL;
      case SDL_SCANCODE_F11: return 0x57; case SDL_SCANCODE_F12: return 0x58;
      case SDL_SCANCODE_HOME: return 0xc7; case SDL_SCANCODE_UP: return 0xc8;
      case SDL_SCANCODE_PAGEUP: return 0xc9; case SDL_SCANCODE_LEFT: return 0xcb;
      case SDL_SCANCODE_RIGHT: return 0xcd; case SDL_SCANCODE_END: return 0xcf;
      case SDL_SCANCODE_DOWN: return 0xd0; case SDL_SCANCODE_PAGEDOWN: return 0xd1;
      case SDL_SCANCODE_INSERT: return 0xd2; case SDL_SCANCODE_DELETE: return 0xd3;
      default: return KBC_NONE;
      }
   }

   void FlushMotion()
   {
      if (!m_pendingMotion)
         return;
      m_pendingMotion = false;
      DarkInputMouse(m_mouseX, m_mouseY, MOUSE_MOTION, m_buttons, 0);
   }

   void HandleEvent(const SDL_Event &event)
   {
      const SDL_WindowID eventWindowId = EventWindowId(event);
      if (eventWindowId && eventWindowId != m_windowId)
         return;

      if (event.type != SDL_EVENT_MOUSE_MOTION)
         FlushMotion();

      if (event.type == SDL_EVENT_QUIT ||
          event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
         m_quit = true;
      if (event.type >= SDL_EVENT_WINDOW_FIRST &&
          event.type <= SDL_EVENT_WINDOW_LAST)
      {
         HandleWindowEvent(event);
         return;
      }
      if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
      {
         const uchar scan = KeyScan(event.key.scancode);
         const bool down = event.type == SDL_EVENT_KEY_DOWN;
         if (scan != KBC_NONE && !event.key.repeat)
            DarkInputKey(scan, down);
      }
      else if (event.type == SDL_EVENT_MOUSE_MOTION)
         HandleMouseMotion(event.motion);
      else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
               event.type == SDL_EVENT_MOUSE_BUTTON_UP)
         HandleMouseButton(event.button, event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
      else if (event.type == SDL_EVENT_MOUSE_WHEEL)
         HandleMouseWheel(event.wheel);
   }

   void HandleWindowEvent(const SDL_Event &event)
   {
      switch (event.type)
      {
      case SDL_EVENT_WINDOW_RESIZED:
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
         DarkRenderWindowResized();
         break;
      case SDL_EVENT_WINDOW_MINIMIZED:
         DarkRenderWindowMinimized();
         break;
      case SDL_EVENT_WINDOW_RESTORED:
         DarkRenderWindowRestored();
         break;
      case SDL_EVENT_WINDOW_FOCUS_GAINED:
         SDLDisplaySetFocused(true);
         DarkRenderWindowRestored();
         break;
      case SDL_EVENT_WINDOW_FOCUS_LOST:
         SDLDisplaySetFocused(false);
         break;
      default:
         break;
      }
   }

   static SDL_WindowID EventWindowId(const SDL_Event &event)
   {
      switch (event.type)
      {
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      case SDL_EVENT_WINDOW_FOCUS_GAINED: case SDL_EVENT_WINDOW_FOCUS_LOST:
      case SDL_EVENT_WINDOW_MINIMIZED: case SDL_EVENT_WINDOW_RESTORED:
      case SDL_EVENT_WINDOW_RESIZED: case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
         return event.window.windowID;
      case SDL_EVENT_KEY_DOWN: case SDL_EVENT_KEY_UP:
         return event.key.windowID;
      case SDL_EVENT_MOUSE_MOTION: return event.motion.windowID;
      case SDL_EVENT_MOUSE_BUTTON_DOWN: case SDL_EVENT_MOUSE_BUTTON_UP:
         return event.button.windowID;
      case SDL_EVENT_MOUSE_WHEEL: return event.wheel.windowID;
      default: return 0;
      }
   }

   void HandleMouseMotion(const SDL_MouseMotionEvent &event)
   {
      if (SDLDisplayRelativeMouseActive())
      {
         m_relativeX += event.xrel;
         m_relativeY += event.yrel;
         return;
      }
      int x = static_cast<int>(event.x);
      int y = static_cast<int>(event.y);
      (void)RenderBackendClientToLogicalPoint(&x, &y);
      m_mouseX = static_cast<short>(x);
      m_mouseY = static_cast<short>(y);
      m_pendingMotion = true;
   }

   void HandleMouseButton(const SDL_MouseButtonEvent &event, bool down)
   {
      const int button = event.button == SDL_BUTTON_LEFT ? 0 :
         event.button == SDL_BUTTON_RIGHT ? 1 :
         event.button == SDL_BUTTON_MIDDLE ? 2 : -1;
      if (button < 0)
         return;

      const uchar bit = static_cast<uchar>(1 << button);
      if (down)
         m_buttons |= bit;
      else
         m_buttons &= static_cast<uchar>(~bit);
      const uchar type = static_cast<uchar>(1 << (1 + 2 * button + !down));
      int x = static_cast<int>(event.x);
      int y = static_cast<int>(event.y);
      if (SDLDisplayRelativeMouseActive())
      {
         int width = 0;
         int height = 0;
         if (!RenderBackendGetLogicalSize(&width, &height))
         {
            width = static_cast<int>(event.x);
            height = static_cast<int>(event.y);
         }
         x = width / 2;
         y = height / 2;
      }
      else
         (void)RenderBackendClientToLogicalPoint(&x, &y);
      DarkInputMouse(static_cast<short>(x), static_cast<short>(y), type,
         m_buttons, 0);
   }

   void HandleMouseWheel(const SDL_MouseWheelEvent &event)
   {
      int width = 0;
      int height = 0;
      (void)RenderBackendGetLogicalSize(&width, &height);
      int x = width / 2;
      int y = height / 2;
      if (!SDLDisplayRelativeMouseActive())
      {
         x = static_cast<int>(event.mouse_x);
         y = static_cast<int>(event.mouse_y);
         (void)RenderBackendClientToLogicalPoint(&x, &y);
      }
      const int direction = event.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1;
      const int wheel = std::clamp(static_cast<int>(std::lround(event.y)) *
         direction, -127, 127);
      if (wheel)
         DarkInputMouse(static_cast<short>(x), static_cast<short>(y),
            MOUSE_WHEEL, m_buttons, static_cast<char>(wheel));
   }


   SDL_WindowID m_windowId{};
   bool m_quit{};
   float m_relativeX{};
   float m_relativeY{};
   bool m_pendingMotion{};
   short m_mouseX{};
   short m_mouseY{};
   uchar m_buttons{};
   bool m_lifecycleWatchRegistered{};
};

cSDLEventPump &EventPump()
{
   static cSDLEventPump pump;
   return pump;
}
}

bool DarkPumpSDLEvents()
{
   return EventPump().Pump();
}

bool DarkPumpMovieEvents()
{
   return EventPump().PumpMovie();
}
#pragma pack(pop)
