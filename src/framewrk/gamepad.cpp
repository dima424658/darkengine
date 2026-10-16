#include <math.h>
#include <string.h>
#include <ctype.h>

#include <atomic>

#include <config.h>
#include <gamepad.h>
#include <gamepad_backend.h>

typedef struct sGamepadSettings
{
   float left_deadzone;
   float right_deadzone;
   float left_response;
   float right_response;
   float trigger_press;
   float trigger_release;
   int preferred_index;
   int auto_select;
   int rumble;
} sGamepadSettings;

static sGamepadState g_states[GAMEPAD_MAX_CONTROLLERS];
static const sGamepadBackendOps *g_backend;
static sGamepadSettings g_settings;
static unsigned g_active_index;
static std::atomic<unsigned> g_last_input_index(GAMEPAD_MAX_CONTROLLERS);
static int g_initialized;
static int g_active;
static int g_enabled;

static int StrEqualNoCase(const char *left, const char *right)
{
   while (*left && *right)
   {
      if (tolower((unsigned char)*left) != tolower((unsigned char)*right))
         return 0;
      ++left;
      ++right;
   }
   return *left == *right;
}

static float Clamp(float value, float minimum, float maximum)
{
   if (value < minimum)
      return minimum;
   if (value > maximum)
      return maximum;
   return value;
}

static float NormalizeAxis(int value)
{
   if (value < 0)
      return Clamp((float)value / 32768.0f, -1.0f, 0.0f);
   return Clamp((float)value / 32767.0f, 0.0f, 1.0f);
}

static void FilterStick(int raw_x, int raw_y, float deadzone, float response,
                        float *out_x, float *out_y)
{
   const float x = NormalizeAxis(raw_x);
   const float y = NormalizeAxis(raw_y);
   const float magnitude = (float)sqrt((double)(x * x + y * y));
   if (magnitude <= deadzone || magnitude == 0.0f)
   {
      *out_x = 0.0f;
      *out_y = 0.0f;
      return;
   }

   float scaled = Clamp((magnitude - deadzone) / (1.0f - deadzone), 0.0f, 1.0f);
   if (response != 1.0f)
      scaled = (float)pow((double)scaled, (double)response);

   const float direction_scale = scaled / magnitude;
   *out_x = Clamp(x * direction_scale, -1.0f, 1.0f);
   *out_y = Clamp(y * direction_scale, -1.0f, 1.0f);
}

static void ClearState(unsigned index, int report_releases)
{
   const uint32_t old_buttons = g_states[index].buttons;
   memset(&g_states[index], 0, sizeof(g_states[index]));
   g_states[index].controller_index = index;
   if (report_releases)
      g_states[index].buttons_released = old_buttons;
}

static void LoadSettings(void)
{
   g_settings.left_deadzone = 0.18f;
   g_settings.right_deadzone = 0.14f;
   g_settings.left_response = 1.0f;
   g_settings.right_response = 1.35f;
   g_settings.trigger_press = 0.15f;
   g_settings.trigger_release = 0.10f;
   g_settings.preferred_index = -1;
   g_settings.auto_select = 1;
   g_settings.rumble = 1;

   config_get_float("gamepad_left_deadzone", &g_settings.left_deadzone);
   config_get_float("gamepad_right_deadzone", &g_settings.right_deadzone);
   config_get_float("gamepad_left_response", &g_settings.left_response);
   config_get_float("gamepad_right_response", &g_settings.right_response);
   config_get_float("gamepad_trigger_press", &g_settings.trigger_press);
   config_get_float("gamepad_trigger_release", &g_settings.trigger_release);
   if (config_get_int("gamepad_index", &g_settings.preferred_index))
      g_settings.auto_select = 0;
   config_get_int("gamepad_rumble", &g_settings.rumble);

   g_settings.left_deadzone = Clamp(g_settings.left_deadzone, 0.0f, 0.95f);
   g_settings.right_deadzone = Clamp(g_settings.right_deadzone, 0.0f, 0.95f);
   g_settings.left_response = Clamp(g_settings.left_response, 0.1f, 4.0f);
   g_settings.right_response = Clamp(g_settings.right_response, 0.1f, 4.0f);
   g_settings.trigger_press = Clamp(g_settings.trigger_press, 0.0f, 1.0f);
   g_settings.trigger_release = Clamp(g_settings.trigger_release, 0.0f, 1.0f);
   if (g_settings.trigger_release > g_settings.trigger_press)
      g_settings.trigger_release = g_settings.trigger_press;
   if (g_settings.preferred_index < 0 ||
       g_settings.preferred_index >= GAMEPAD_MAX_CONTROLLERS)
   {
      g_settings.preferred_index = 0;
      g_settings.auto_select = 1;
   }
}

static int HasActivity(const sGamepadState *state)
{
   return state->buttons_pressed ||
      state->left_x != 0.0f || state->left_y != 0.0f ||
      state->right_x != 0.0f || state->right_y != 0.0f ||
      state->left_trigger >= g_settings.trigger_press ||
      state->right_trigger >= g_settings.trigger_press;
}

static int HasHeldInput(const sGamepadState *state)
{
   return state->buttons ||
      state->left_x != 0.0f || state->left_y != 0.0f ||
      state->right_x != 0.0f || state->right_y != 0.0f ||
      state->left_trigger >= g_settings.trigger_press ||
      state->right_trigger >= g_settings.trigger_press;
}

static void SelectActiveController(unsigned selected)
{
   if (selected == g_active_index)
      return;

   const uint32_t old_buttons = g_states[g_active_index].buttons;
   sGamepadState *next = &g_states[selected];
   next->buttons_pressed |= next->buttons & ~old_buttons;
   next->buttons_released |= old_buttons & ~next->buttons;
   g_active_index = selected;
}

int GamepadInit(void)
{
   if (g_initialized)
      return g_backend != 0;

   g_initialized = 1;
   g_active = 1;
   g_active_index = 0;
   for (unsigned i = 0; i < GAMEPAD_MAX_CONTROLLERS; ++i)
      ClearState(i, 0);
   g_last_input_index.store(GAMEPAD_MAX_CONTROLLERS);

#ifdef THIEF2_GAME
   g_enabled = 1;
   config_get_int("gamepad_enable", &g_enabled);
#else
   // The editor owns its own input lifecycle.  Gamepad support will be
   // enabled there only when play-preview gets an explicit mode hook.
   g_enabled = 0;
#endif
   LoadSettings();

   char backend_name[32] = "auto";
   config_get_raw("gamepad_backend", backend_name, sizeof(backend_name));
   backend_name[sizeof(backend_name) - 1] = '\0';
   if (!g_enabled || StrEqualNoCase(backend_name, "off") || StrEqualNoCase(backend_name, "legacy"))
      return 0;
#ifdef _WIN32
   if (!StrEqualNoCase(backend_name, "auto") && !StrEqualNoCase(backend_name, "xinput"))
      return 0;
   g_backend = GamepadXInputBackend();
#else
   if (!StrEqualNoCase(backend_name, "auto") && !StrEqualNoCase(backend_name, "sdl"))
      return 0;
   g_backend = GamepadSDLBackend();
#endif
   if (!g_backend || !g_backend->init())
   {
      g_backend = 0;
      return 0;
   }

   g_active_index = (unsigned)g_settings.preferred_index;
   return 1;
}

void GamepadTerm(void)
{
   if (!g_initialized)
      return;
   GamepadStopAllRumble();
   if (g_backend)
      g_backend->term();
   g_backend = 0;
   g_initialized = 0;
   g_enabled = 0;
   g_last_input_index.store(GAMEPAD_MAX_CONTROLLERS);
   for (unsigned i = 0; i < GAMEPAD_MAX_CONTROLLERS; ++i)
      ClearState(i, 0);
}

void GamepadUpdate(void)
{
   if (!g_initialized)
      GamepadInit();
   if (!g_backend || !g_active)
      return;

   const int active_was_connected = g_states[g_active_index].connected;
   int input_activity[GAMEPAD_MAX_CONTROLLERS] = {};
   for (unsigned i = 0; i < GAMEPAD_MAX_CONTROLLERS; ++i)
   {
      sGamepadRawState raw = {};
      const int was_connected = g_states[i].connected;
      const uint32_t old_buttons = g_states[i].buttons;
      if (!g_backend->poll(i, &raw) || !raw.connected)
      {
         ClearState(i, old_buttons != 0);
         continue;
      }
      if (was_connected && raw.packet_number == g_states[i].packet_number)
      {
         g_states[i].buttons_pressed = 0;
         g_states[i].buttons_released = 0;
         continue;
      }

      sGamepadState next = {};
      next.connected = 1;
      next.controller_index = i;
      next.packet_number = raw.packet_number;
      next.buttons = raw.buttons;
      FilterStick(raw.left_x, raw.left_y, g_settings.left_deadzone,
                  g_settings.left_response, &next.left_x, &next.left_y);
      FilterStick(raw.right_x, raw.right_y, g_settings.right_deadzone,
                  g_settings.right_response, &next.right_x, &next.right_y);
      next.left_trigger = Clamp((float)raw.left_trigger / GAMEPAD_RAW_TRIGGER_MAX, 0.0f, 1.0f);
      next.right_trigger = Clamp((float)raw.right_trigger / GAMEPAD_RAW_TRIGGER_MAX, 0.0f, 1.0f);

      const uint32_t left_trigger_bit = 1u << kGamepadButtonLeftTrigger;
      const uint32_t right_trigger_bit = 1u << kGamepadButtonRightTrigger;
      if (next.left_trigger >= g_settings.trigger_press ||
          ((old_buttons & left_trigger_bit) && next.left_trigger > g_settings.trigger_release))
         next.buttons |= left_trigger_bit;
      if (next.right_trigger >= g_settings.trigger_press ||
          ((old_buttons & right_trigger_bit) && next.right_trigger > g_settings.trigger_release))
         next.buttons |= right_trigger_bit;

      // Establish a baseline on initial connection/reconnection.  Buttons
      // already held while a device appears must not fire stale press edges.
      next.buttons_pressed = was_connected ? (next.buttons & ~old_buttons) : 0;
      next.buttons_released = old_buttons & ~next.buttons;
      g_states[i] = next;
      input_activity[i] = HasActivity(&next);
   }

   if (g_settings.auto_select)
   {
      unsigned selected = g_active_index;
      for (unsigned i = 0; i < GAMEPAD_MAX_CONTROLLERS; ++i)
         if (input_activity[i])
            g_last_input_index.store(i);

      // Android handhelds can expose one set of built-in controls through
      // two controller interfaces.  Both slots then report the same physical
      // stick in one update, and selecting the last slot makes ownership (and
      // sometimes the axis mapping) alternate in the middle of a turn.  Keep
      // a controller while it has held input; another controller may claim
      // ownership once the active one returns to neutral.
      if (g_states[selected].connected && !HasHeldInput(&g_states[selected]))
      {
         for (unsigned i = 0; i < GAMEPAD_MAX_CONTROLLERS; ++i)
         {
            if (i != selected && input_activity[i])
            {
               selected = i;
               break;
            }
         }
      }

      // Keep a disconnected active slot selected for one update so its
      // releases and zero axes reach the compatibility bridge first.
      if (!g_states[selected].connected && !active_was_connected)
      {
         for (unsigned i = 0; i < GAMEPAD_MAX_CONTROLLERS; ++i)
         {
            if (g_states[i].connected)
            {
               selected = i;
               break;
            }
         }
      }
      SelectActiveController(selected);
   }
   else if (!g_states[g_active_index].connected && !active_was_connected)
   {
      const unsigned preferred = (unsigned)g_settings.preferred_index;
      if (g_states[preferred].connected)
         SelectActiveController(preferred);
   }

   if (!g_settings.auto_select)
   {
      for (unsigned i = 0; i < GAMEPAD_MAX_CONTROLLERS; ++i)
         if (input_activity[i])
            g_last_input_index.store(i);
   }
}

const sGamepadState *GamepadGetState(unsigned index)
{
   if (index >= GAMEPAD_MAX_CONTROLLERS)
      return 0;
   return &g_states[index];
}

unsigned GamepadGetActiveIndex(void)
{
   return g_active_index;
}

unsigned GamepadGetLastInputIndex(void)
{
   return g_last_input_index.load();
}

void GamepadNotifyInput(unsigned index)
{
   if (index < GAMEPAD_MAX_CONTROLLERS)
      g_last_input_index.store(index);
}

int GamepadOwnsInput(void)
{
   return g_backend && g_active && g_states[g_active_index].connected;
}

const char *GamepadGetBackendName(void)
{
   return g_backend ? g_backend->name : "none";
}

void GamepadSetActive(int active)
{
   active = !!active;
   if (g_active == active)
      return;
   g_active = active;
   if (!active)
   {
      GamepadStopAllRumble();
      for (unsigned i = 0; i < GAMEPAD_MAX_CONTROLLERS; ++i)
         ClearState(i, g_states[i].buttons != 0);
   }
}

void GamepadSetRumble(unsigned index, float low, float high)
{
   if (!g_backend || !g_active || !g_settings.rumble || index >= GAMEPAD_MAX_CONTROLLERS)
      return;
   g_backend->rumble(index, Clamp(low, 0.0f, 1.0f), Clamp(high, 0.0f, 1.0f));
}

void GamepadStopAllRumble(void)
{
   if (!g_backend)
      return;
   for (unsigned i = 0; i < GAMEPAD_MAX_CONTROLLERS; ++i)
      g_backend->rumble(i, 0.0f, 0.0f);
}
