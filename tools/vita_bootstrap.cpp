#include "vita_video_settings.hpp"

#include <psp2/gxm.h>
#include <psp2/kernel/processmgr.h>
#include <SDL2/SDL.h>
#include <vitaGL.h>

#include <algorithm>
#include <cstdio>
#include <string>

#ifndef DARK_VITA_TITLE
#define DARK_VITA_TITLE "Thief Trilogy Vita Test"
#endif

#ifndef DARK_VITA_TITLE_ID
#define DARK_VITA_TITLE_ID "THFG00000"
#endif

#ifndef DARK_VITA_ASSET_ROOT
#define DARK_VITA_ASSET_ROOT "ux0:data/thiefgold"
#endif

namespace
{
struct sShader
{
   GLuint program = 0;
   GLint position = -1;
   GLint color = -1;
   GLint brightness = -1;
   GLint gamma = -1;
};

struct sVertex
{
   GLfloat x;
   GLfloat y;
   GLfloat r;
   GLfloat g;
   GLfloat b;
   GLfloat a;
};

constexpr int kScreenWidth = 960;
constexpr int kScreenHeight = 544;
constexpr int kSliderX = 230;
constexpr int kSliderWidth = 500;
constexpr int kSliderHeight = 42;
constexpr int kBrightnessY = 190;
constexpr int kGammaY = 310;

GLuint CompileShader(GLenum type, const char *source)
{
   const GLuint shader = glCreateShader(type);
   if (!shader)
      return 0;

   glShaderSource(shader, 1, &source, nullptr);
   glCompileShader(shader);
   GLint compiled = GL_FALSE;
   glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
   if (compiled != GL_TRUE)
   {
      char log[512] = {};
      GLsizei length = 0;
      glGetShaderInfoLog(shader, sizeof(log), &length, log);
      SDL_LogError(SDL_LOG_CATEGORY_RENDER,
                   "Vita correction shader compile failed: %.*s",
                   static_cast<int>(length), log);
      glDeleteShader(shader);
      return 0;
   }
   return shader;
}

bool CreateShader(sShader &shader)
{
   const GLuint vertex =
      CompileShader(GL_VERTEX_SHADER, DarkVitaVideo::VertexShaderSource());
   const GLuint fragment =
      CompileShader(GL_FRAGMENT_SHADER, DarkVitaVideo::FragmentShaderSource());
   if (!vertex || !fragment)
   {
      if (vertex)
         glDeleteShader(vertex);
      if (fragment)
         glDeleteShader(fragment);
      return false;
   }

   shader.program = glCreateProgram();
   if (!shader.program)
   {
      glDeleteShader(vertex);
      glDeleteShader(fragment);
      return false;
   }
   glAttachShader(shader.program, vertex);
   glAttachShader(shader.program, fragment);
   glBindAttribLocation(shader.program, 0, "aPosition");
   glBindAttribLocation(shader.program, 1, "aColor");
   glLinkProgram(shader.program);
   glDeleteShader(vertex);
   glDeleteShader(fragment);

   GLint linked = GL_FALSE;
   glGetProgramiv(shader.program, GL_LINK_STATUS, &linked);
   if (linked != GL_TRUE)
   {
      char log[512] = {};
      GLsizei length = 0;
      glGetProgramInfoLog(shader.program, sizeof(log), &length, log);
      SDL_LogError(SDL_LOG_CATEGORY_RENDER,
                   "Vita correction shader link failed: %.*s",
                   static_cast<int>(length), log);
      glDeleteProgram(shader.program);
      shader.program = 0;
      return false;
   }

   shader.position = glGetAttribLocation(shader.program, "aPosition");
   shader.color = glGetAttribLocation(shader.program, "aColor");
   shader.brightness = glGetUniformLocation(shader.program, "uBrightness");
   shader.gamma = glGetUniformLocation(shader.program, "uGamma");
   if (shader.position < 0 || shader.color < 0 ||
       shader.brightness < 0 || shader.gamma < 0)
   {
      glDeleteProgram(shader.program);
      shader.program = 0;
      return false;
   }
   return true;
}

void DrawRect(const sShader &shader, const DarkVitaVideo::Settings &settings,
              float x, float y, float width, float height,
              float red, float green, float blue)
{
   const GLfloat left = x / kScreenWidth * 2.0f - 1.0f;
   const GLfloat right = (x + width) / kScreenWidth * 2.0f - 1.0f;
   const GLfloat top = 1.0f - y / kScreenHeight * 2.0f;
   const GLfloat bottom = 1.0f - (y + height) / kScreenHeight * 2.0f;
   const sVertex vertices[] = {
      {left, top, red, green, blue, 1.0f},
      {right, top, red, green, blue, 1.0f},
      {left, bottom, red, green, blue, 1.0f},
      {right, bottom, red, green, blue, 1.0f}
   };

   glUseProgram(shader.program);
   glUniform1f(shader.brightness,
               DarkVitaVideo::BrightnessOffset(settings));
   glUniform1f(shader.gamma, DarkVitaVideo::GammaValue(settings));
   glEnableVertexAttribArray(static_cast<GLuint>(shader.position));
   glEnableVertexAttribArray(static_cast<GLuint>(shader.color));
   glVertexAttribPointer(static_cast<GLuint>(shader.position), 2, GL_FLOAT,
                         GL_FALSE, sizeof(sVertex), &vertices[0].x);
   glVertexAttribPointer(static_cast<GLuint>(shader.color), 4, GL_FLOAT,
                         GL_FALSE, sizeof(sVertex), &vertices[0].r);
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
   glDisableVertexAttribArray(static_cast<GLuint>(shader.position));
   glDisableVertexAttribArray(static_cast<GLuint>(shader.color));
}

void DrawLetter(const sShader &shader,
                const DarkVitaVideo::Settings &settings,
                char letter, int x, int y)
{
   static const unsigned char brightnessGlyph[] = {6, 5, 6, 5, 6};
   static const unsigned char gammaGlyph[] = {7, 4, 5, 5, 7};
   const unsigned char *rows = letter == 'B' ? brightnessGlyph : gammaGlyph;
   for (int row = 0; row < 5; ++row)
      for (int column = 0; column < 3; ++column)
         if (rows[row] & (1u << (2 - column)))
            DrawRect(shader, settings, x + column * 5, y + row * 5,
                     4, 4, 0.9f, 0.94f, 1.0f);
}

void DrawSlider(const sShader &shader,
                const DarkVitaVideo::Settings &settings,
                int y, int value, bool selected, float red, float green,
                float blue, char label)
{
   DrawRect(shader, settings, kSliderX, y, kSliderWidth, kSliderHeight,
            0.12f, 0.15f, 0.2f);
   const float fillWidth = kSliderWidth * value / 100.0f;
   if (fillWidth > 0.0f)
      DrawRect(shader, settings, kSliderX, y, fillWidth, kSliderHeight,
               red, green, blue);
   const float handleX = kSliderX + fillWidth - 5.0f;
   DrawRect(shader, settings, std::clamp(handleX, float(kSliderX),
            float(kSliderX + kSliderWidth - 10)), y - 5, 10,
            kSliderHeight + 10, 0.96f, 0.97f, 1.0f);
   if (selected)
   {
      DrawRect(shader, settings, kSliderX - 4, y - 4, kSliderWidth + 8, 3,
               0.95f, 0.95f, 1.0f);
      DrawRect(shader, settings, kSliderX - 4, y + kSliderHeight + 1,
               kSliderWidth + 8, 3, 0.95f, 0.95f, 1.0f);
   }
   DrawLetter(shader, settings, label, kSliderX - 34, y + 8);
}

void DrawSettings(const sShader &shader,
                  const DarkVitaVideo::Settings &settings, int selected)
{
   glClearColor(0.025f, 0.04f, 0.065f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT);
   DrawRect(shader, settings, 120, 110, 720, 340, 0.07f, 0.09f, 0.13f);
   DrawSlider(shader, settings, kBrightnessY, settings.brightnessSlider,
              selected == 0, 0.22f, 0.62f, 0.92f, 'B');
   DrawSlider(shader, settings, kGammaY, settings.gammaSlider,
              selected == 1, 0.85f, 0.52f, 0.2f, 'G');
}

bool SaveSettings(const std::string &path,
                  const DarkVitaVideo::Settings &settings)
{
   std::string error;
   if (!DarkVitaVideo::Save(path, settings, &error))
   {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                   "Could not save vita_options.cfg: %s", error.c_str());
      return false;
   }
   return true;
}

void ChangeSlider(DarkVitaVideo::Settings &settings, int selected, int delta,
                  const std::string &path)
{
   int &value = selected == 0 ? settings.brightnessSlider
                              : settings.gammaSlider;
   const int changed = std::clamp(value + delta, 0, 100);
   if (changed == value)
      return;
   value = changed;
   SDL_Log("Vita video sliders: brightness=%d gamma=%d",
           settings.brightnessSlider, settings.gammaSlider);
   SaveSettings(path, settings);
}

bool SetSliderFromTouch(DarkVitaVideo::Settings &settings, int &selected,
                        float x, float y)
{
   const int screenX = static_cast<int>(x * kScreenWidth);
   const int screenY = static_cast<int>(y * kScreenHeight);
   int *value = nullptr;
   if (screenY >= kBrightnessY - 12 &&
       screenY <= kBrightnessY + kSliderHeight + 12)
   {
      selected = 0;
      value = &settings.brightnessSlider;
   }
   else if (screenY >= kGammaY - 12 &&
            screenY <= kGammaY + kSliderHeight + 12)
   {
      selected = 1;
      value = &settings.gammaSlider;
   }
   if (!value)
      return false;

   const int changed = std::clamp(
      (screenX - kSliderX) * 100 / kSliderWidth, 0, 100);
   if (changed == *value)
      return true;
   *value = changed;
   return true;
}
}

int main()
{
   if (!vglInitExtended(0, kScreenWidth, kScreenHeight, 0x800000,
                       SCE_GXM_MULTISAMPLE_NONE))
      sceKernelExitProcess(1);

   if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0)
   {
      std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
      sceKernelExitProcess(2);
   }

   const std::string optionsPath =
      std::string(DARK_VITA_ASSET_ROOT) + "/vita_options.cfg";
   DarkVitaVideo::LoadResult loaded = DarkVitaVideo::Load(optionsPath);
   DarkVitaVideo::Settings settings = loaded.settings;
   if (!loaded.error.empty())
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                   "Could not load vita_options.cfg: %s",
                   loaded.error.c_str());
   else if (!loaded.found)
      SDL_Log("Using neutral Vita Slim video settings");

   SDL_Log("Vita bootstrap: %s (%s), assets: %s",
           DARK_VITA_TITLE, DARK_VITA_TITLE_ID, DARK_VITA_ASSET_ROOT);
   SDL_Log("D-pad up/down selects brightness or gamma; left/right adjusts. "
           "Touch either slider to set its value. START exits.");
   SDL_GameController *controller = nullptr;
   if (SDL_NumJoysticks() > 0 && SDL_IsGameController(0))
      controller = SDL_GameControllerOpen(0);

   glViewport(0, 0, kScreenWidth, kScreenHeight);
   sShader shader;
   if (!CreateShader(shader))
   {
      SDL_Quit();
      sceKernelExitProcess(3);
   }

   bool running = true;
   bool touchChanged = false;
   int selected = 0;
   while (running)
   {
      SDL_Event event;
      while (SDL_PollEvent(&event))
      {
         if (event.type == SDL_QUIT)
            running = false;
         else if (event.type == SDL_CONTROLLERBUTTONDOWN)
         {
            switch (event.cbutton.button)
            {
            case SDL_CONTROLLER_BUTTON_START:
               running = false;
               break;
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
               selected = 0;
               break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
               selected = 1;
               break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
               ChangeSlider(settings, selected, -2, optionsPath);
               break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
               ChangeSlider(settings, selected, 2, optionsPath);
               break;
            default:
               break;
            }
         }
         else if (event.type == SDL_FINGERDOWN ||
                  event.type == SDL_FINGERMOTION)
            touchChanged |= SetSliderFromTouch(
                     settings, selected, event.tfinger.x, event.tfinger.y);
         else if (event.type == SDL_FINGERUP && touchChanged)
         {
            SaveSettings(optionsPath, settings);
            touchChanged = false;
         }
      }

      DrawSettings(shader, settings, selected);
      vglSwapBuffers(GL_FALSE);
      SDL_Delay(16);
   }

   glDeleteProgram(shader.program);
   if (controller)
      SDL_GameControllerClose(controller);
   SDL_Quit();
   sceKernelExitProcess(0);
   return 0;
}
