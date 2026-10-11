#pragma pack(push, 8)
#include <algorithm>
#include <chrono>
#include <deque>
#include <mutex>
#include <vector>
#pragma pack(pop)

#include <aggmemb.h>
#include <appagg.h>
#include <inpapi.h>
#include <inpbase.h>
#include <joystick.h>
#include <kb.h>
#include <inputimp.h>
#include <kbcook.h>
#include <mouse.h>
#include <mousevel.h>

#include "../render/runtime.h"

namespace {
std::mutex g_mutex;
std::deque<kbs_event> g_keys;
std::deque<lgMouseEvent> g_mouse;
uchar g_keyState[256]{};
short g_x, g_y, g_xmax = 1023, g_ymax = 1023;
int g_vx, g_vy;
bool g_relativeMode;
int g_relativeX, g_relativeY;
ulong *g_ticks;
struct Callback { int id; mouse_callfunc fn; void *data; };
std::vector<Callback> g_callbacks;
int g_nextCallback = 1;
constexpr size_t kMaxInputEvents = 32;

ulong now_ms()
{
   if (g_ticks) return *g_ticks;
   using namespace std::chrono;
   return static_cast<ulong>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

void clamp_mouse()
{
   g_x = std::max<short>(0, std::min(g_x, g_xmax));
   g_y = std::max<short>(0, std::min(g_y, g_ymax));
}
}

extern "C" {
kbs_event kb_null_event = {KBC_NONE, KBS_UP};
char *kbd_array = reinterpret_cast<char *>(g_keyState);
int kbd_modifier_state;
ubyte mouseMask = 0xff;
short mouseInstantButts;

int kb_startup(void *) { kb_flush(); return 0; }
int kb_shutdown(void) { kb_flush(); return 0; }
uchar kb_state(uchar code) { return g_keyState[code]; }
uchar _kb_state(uchar code) { return kb_state(code); }
uchar kb_get_state(uchar code) { return kb_state(code); }
uchar _kb_get_state(uchar code) { return kb_state(code); }
void kb_set_state(uchar code, uchar bits) { g_keyState[code] |= bits; }
void kb_clear_state(uchar code, uchar bits) { g_keyState[code] &= ~bits; }
void kb_generate(kbs_event event)
{
   std::lock_guard<std::mutex> lock(g_mutex);
   g_keyState[event.code] = event.state == KBS_DOWN;
   if (g_keys.size() == kMaxInputEvents) g_keys.pop_front();
   g_keys.push_back(event);
}
kbs_event kb_next(void)
{
   std::lock_guard<std::mutex> lock(g_mutex);
   if (g_keys.empty()) return kb_null_event;
   kbs_event event = g_keys.front(); g_keys.pop_front(); return event;
}
void _kb_next(kbs_event &event) { event = kb_next(); }
kbs_event kb_look_next(void)
{
   std::lock_guard<std::mutex> lock(g_mutex);
   return g_keys.empty() ? kb_null_event : g_keys.front();
}
void _kb_look_next(kbs_event &event) { event = kb_look_next(); }
void kb_flush(void) { std::lock_guard<std::mutex> lock(g_mutex); g_keys.clear(); }
void kb_flush_bios(void) {}
void kb_set_signal(uchar, uchar) {}
int kb_get_flags(void) { return 0; }
int _kb_get_flags(void) { return 0; }
void kb_set_flags(int) {}
HANDLE kb_get_queue_available_signal(void) { return nullptr; }
int kb_set_leds(int) { return 0; }

errtype kb_cook_real(kbs_event ev, ushort *cooked, bool *result, bool allSpecial)
{
   int oldMods = kbd_modifier_state;
   ushort flags = KB_CNV(ev.code, 0), value = 0;
   bool shifted = ((kbd_modifier_state >> KBM_SHIFT_SHF) |
                   (kbd_modifier_state >> (KBM_SHIFT_SHF + 1))) & 1;
   bool caps = ((flags >> CNV_CAPS_SHF) &
                (kbd_modifier_state >> KBM_CAPS_SHF)) & 1;
   bool rightAlt = false;
   if (allSpecial) value = ev.code | KB_FLAG_SPECIAL | KB_FLAG_SHIFT | KB_FLAG_ALT | KB_FLAG_CTRL;
   else {
      if ((kbd_modifier_state >> (KBM_ALT_SHF + 1)) & 1) {
         value = KB_CNV(ev.code, 2); rightAlt = value != 0;
      }
      if (!rightAlt) value = KB_CNV(ev.code, shifted ^ caps);
   }
   *cooked = value & (CNV_SPECIAL | CNV_2ND | 0xff);
   *result = false;
   if (ev.state == KBS_UP) kbd_modifier_state = ~kbd_modifier_state;
   switch (ev.code) {
   case KBC_LSHIFT: kbd_modifier_state |= KBM_LSHIFT; break;
   case KBC_RSHIFT: kbd_modifier_state |= KBM_RSHIFT; break;
   case KBC_LCTRL:  kbd_modifier_state |= KBM_LCTRL; break;
   case KBC_RCTRL:  kbd_modifier_state |= KBM_RCTRL; break;
   case KBC_LALT:   kbd_modifier_state |= KBM_LALT; break;
   case KBC_RALT:   kbd_modifier_state |= KBM_RALT; break;
   case KBC_CAPS: if (ev.state == KBS_DOWN) kbd_modifier_state ^= KBM_CAPS; break;
   case KBC_NUM: if (ev.state == KBS_DOWN) kbd_modifier_state ^= KBM_NUM; break;
   case KBC_SCROLL: if (ev.state == KBS_DOWN) kbd_modifier_state ^= KBM_SCROLL; break;
   default: *result = true; break;
   }
   if (ev.state == KBS_UP) kbd_modifier_state = ~kbd_modifier_state;
   if ((kbd_modifier_state & KBM_LED_MASK) != (oldMods & KBM_LED_MASK)) kb_set_leds(kbd_modifier_state);
   if (!*result) return OK;
   if ((value & CNV_NUM) && !(kbd_modifier_state & KBM_NUM)) *cooked = ev.code | KB_FLAG_SPECIAL;
   *cooked |= static_cast<ushort>(ev.state) << KB_DOWN_SHF;
   if (!rightAlt) {
      *cooked |= (((kbd_modifier_state << (KB_CTRL_SHF - KBM_CTRL_SHF)) |
                   (kbd_modifier_state << (KB_CTRL_SHF - KBM_CTRL_SHF - 1))) & KB_FLAG_CTRL) & value;
      *cooked |= (((kbd_modifier_state << (KB_ALT_SHF - KBM_ALT_SHF)) |
                   (kbd_modifier_state << (KB_ALT_SHF - KBM_ALT_SHF - 1))) & KB_FLAG_ALT) & value;
   }
   *cooked |= (shifted << KB_SHIFT_SHF) & value;
   return OK;
}
bool kb_get_cooked(ushort *key)
{
   kbs_event event = kb_next(); bool result = false;
   if (event.code != KBC_NONE) kb_cook_real(event, key, &result, false);
   return result;
}
bool _kb_get_cooked(ushort *key) { return kb_get_cooked(key); }

errtype mouse_init(short x, short y) { return mouse_set_screensize(x, y); }
void mouse_shutdown(void) { mouse_flush(); }
errtype mouse_set_timestamp_register(ulong *ticks) { g_ticks = ticks; return OK; }
ulong mouse_get_time(void) { return now_ms(); }
errtype _mouse_get_xy(short *x, short *y)
{
   g_x += static_cast<short>(g_vx >> 16); g_y += static_cast<short>(g_vy >> 16); clamp_mouse();
   *x = g_x; *y = g_y; return OK;
}
errtype mouse_get_xy(short *x, short *y) { return _mouse_get_xy(x, y); }
errtype mouse_put_xy(short x, short y)
{
   g_x = x; g_y = y; clamp_mouse();
   if (!g_relativeMode) DarkRenderSetMousePosition(g_x, g_y);
   // Match the legacy input contract: mouseMask suppresses motion generated
   // by programmatic pointer warps, not motion received from the platform.
   // Relative input already supplies a delta around the logical center; its
   // programmatic recenter must not enqueue another motion event.
   if (!g_relativeMode && (mouseMask & MOUSE_MOTION)) {
      lgMouseEvent event{};
      event.x = g_x; event.y = g_y; event.type = MOUSE_MOTION;
      event.buttons = static_cast<uchar>(mouseInstantButts);
      event.timestamp = now_ms();
      mouse_generate(event);
   }
   return OK;
}
errtype mouse_set_relative_mode(bool enabled)
{
   {
      std::lock_guard<std::mutex> lock(g_mutex);
      g_relativeMode = enabled;
      g_relativeX = g_relativeY = 0;
      g_mouse.erase(std::remove_if(g_mouse.begin(), g_mouse.end(),
         [](const lgMouseEvent &event) { return event.type == MOUSE_MOTION; }),
         g_mouse.end());
      if (enabled) {
         g_x = static_cast<short>(g_xmax / 2);
         g_y = static_cast<short>(g_ymax / 2);
      }
   }
   DarkRenderSetRelativeMouse(enabled);
   return OK;
}
bool mouse_consume_relative_motion(int *x, int *y)
{
   std::lock_guard<std::mutex> lock(g_mutex);
   if (!g_relativeMode)
      return false;
   *x = g_relativeX;
   *y = g_relativeY;
   g_relativeX = g_relativeY = 0;
   return true;
}
errtype _mouse_check_btn(int button, bool *result) { *result = (mouseInstantButts >> button) & 1; return OK; }
errtype mouse_check_btn(int button, bool *result) { return _mouse_check_btn(button, result); }
errtype mouse_look_next(lgMouseEvent *event)
{
   std::lock_guard<std::mutex> lock(g_mutex); if (g_mouse.empty()) return ERR_DUNDERFLOW;
   *event = g_mouse.front(); return OK;
}
errtype _mouse_look_next(lgMouseEvent *event) { return mouse_look_next(event); }
errtype mouse_next(lgMouseEvent *event)
{
   std::lock_guard<std::mutex> lock(g_mutex); if (g_mouse.empty()) return ERR_DUNDERFLOW;
   *event = g_mouse.front(); g_mouse.pop_front(); return OK;
}
errtype _mouse_next(lgMouseEvent *event) { return mouse_next(event); }
errtype mouse_flush(void)
{
   std::lock_guard<std::mutex> lock(g_mutex);
   g_mouse.clear();
   g_relativeX = g_relativeY = 0;
   return OK;
}
errtype mouse_generate(lgMouseEvent event)
{
   std::vector<Callback> callbacks;
   {
      std::lock_guard<std::mutex> lock(g_mutex);
      g_x = event.x; g_y = event.y; mouseInstantButts = event.buttons;
      if (g_mouse.size() == kMaxInputEvents) g_mouse.pop_front();
      g_mouse.push_back(event);
      callbacks.reserve(g_callbacks.size());
      for (const auto &callback : g_callbacks)
         callbacks.push_back(callback);
   }
   for (const auto &cb : callbacks) cb.fn(&event, cb.data);
   return OK;
}
errtype mouse_set_callback(mouse_callfunc fn, void *data, int *id)
{
   std::lock_guard<std::mutex> lock(g_mutex);
   *id = g_nextCallback++; g_callbacks.push_back({*id, fn, data}); return OK;
}
errtype mouse_unset_callback(int id)
{
   std::lock_guard<std::mutex> lock(g_mutex);
   g_callbacks.erase(std::remove_if(g_callbacks.begin(), g_callbacks.end(),
      [id](const Callback &cb) { return cb.id == id; }), g_callbacks.end()); return OK;
}
errtype mouse_constrain_xy(short xl, short yl, short xh, short yh)
{ (void)xl; (void)yl; g_xmax = xh; g_ymax = yh; clamp_mouse(); return OK; }
errtype mouse_set_rate(short, short, short) { return OK; }
errtype mouse_get_rate(short *x, short *y, short *threshold) { *x = *y = 1; *threshold = 0; return OK; }
errtype _mouse_get_rate(short *x, short *y, short *threshold) { return mouse_get_rate(x, y, threshold); }
errtype mouse_set_screensize(short x, short y) { g_xmax = x - 1; g_ymax = y - 1; clamp_mouse(); return OK; }
errtype mouse_extremes(short *xl, short *yl, short *xh, short *yh)
{ *xl = *yl = 0; *xh = g_xmax; *yh = g_ymax; return OK; }
errtype _mouse_extremes(short *xl, short *yl, short *xh, short *yh) { return mouse_extremes(xl, yl, xh, yh); }
errtype mouse_set_lefty(bool) { return OK; }
void mouse_wait_for_queue_mutex(void) { g_mutex.lock(); }
void mouse_release_queue_mutex(void) { g_mutex.unlock(); }
HANDLE mouse_get_queue_available_signal(void) { return nullptr; }
errtype mouse_set_velocity_range(int, int, int, int) { return OK; }
errtype mouse_set_velocity(int x, int y) { g_vx = x; g_vy = y; return OK; }
errtype mouse_add_velocity(int x, int y) { g_vx += x; g_vy += y; return OK; }
errtype mouse_get_velocity(int *x, int *y) { *x = g_vx; *y = g_vy; return OK; }

uchar joy_read_buttons(void) { return 0; }
void joy_read_pots(char *pots) { if (pots) std::fill(pots, pots + 4, 0); }
void joy_read_pots_raw(short *pots) { if (pots) std::fill(pots, pots + 4, 0); }

void DarkInputKey(uchar scan, int down) { kb_generate({scan, static_cast<uchar>(down != 0)}); }
void DarkInputMouse(short x, short y, uchar type, uchar buttons, char wheel)
{
   lgMouseEvent event{}; event.x = x; event.y = y; event.type = type;
   event.buttons = buttons; event.wheel = wheel; event.timestamp = now_ms(); mouse_generate(event);
}
void DarkInputMouseRelative(int x, int y, uchar buttons)
{
   if (x == 0 && y == 0)
      return;

   lgMouseEvent event{};
   std::vector<Callback> callbacks;
   {
      std::lock_guard<std::mutex> lock(g_mutex);
      if (!g_relativeMode)
         return;

      g_relativeX += x;
      g_relativeY += y;
      mouseInstantButts = buttons;

      // The queue only wakes the legacy dispatcher. Raw motion itself lives
      // in the accumulator, so an event backlog cannot add input latency.
      const auto pending = std::find_if(g_mouse.begin(), g_mouse.end(),
         [](const lgMouseEvent &queued) { return queued.type == MOUSE_MOTION; });
      if (pending != g_mouse.end())
         return;

      event.x = g_x;
      event.y = g_y;
      event.type = MOUSE_MOTION;
      event.buttons = buttons;
      event.timestamp = now_ms();
      if (g_mouse.size() == kMaxInputEvents)
         g_mouse.pop_front();
      g_mouse.push_back(event);
      callbacks.reserve(g_callbacks.size());
      for (const auto &callback : g_callbacks)
         callbacks.push_back(callback);
   }
   for (const auto &callback : callbacks)
      callback.fn(&event, callback.data);
}
}

class cUnixInputManager : public IInputManager
{
   DECLARE_AGGREGATION(cUnixInputManager);
public:
   explicit cUnixInputManager(IUnknown *outer)
   { INIT_AGGREGATION_1(outer, IID_IInputManager, this, kPriorityLibrary, nullptr); }
   void IterStart(sInputDeviceIter *it, GUID) override { it->m_pDev = nullptr; }
   void IterNext(sInputDeviceIter *it) override { it->m_pDev = nullptr; }
   IInputDevice *IterGet(sInputDeviceIter *) override { return nullptr; }
   BOOL IterFinished(sInputDeviceIter *) override { return TRUE; }
   HRESULT Init() { return S_OK; }
   HRESULT End() { return S_OK; }
};
IMPLEMENT_AGGREGATION_SELF_DELETE(cUnixInputManager);

extern "C" tResult LGAPI _InputManagerCreate(IUnknown *outer)
{ return new cUnixInputManager(outer) ? S_OK : E_FAIL; }
