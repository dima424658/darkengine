#include "vita_video_settings.hpp"
#include "vita_controls.hpp"

#include <psp2/gxm.h>
#include <psp2/kernel/processmgr.h>
#include <SDL2/SDL.h>
#include <vitaGL.h>

#include <algorithm>
#include <cstdint>
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

struct sFsrPass
{
   GLuint program = 0;
   GLuint framebuffer = 0;
   GLuint texture = 0;
   GLint position = -1;
   GLint texCoord = -1;
   GLint textureUniform = -1;
   GLint texelSize = -1;
   GLint sharpness = -1;
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
constexpr int kSpatialY = 390;
constexpr int kTriggerSize = 60;

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

bool CreateFsrPass(sFsrPass &pass)
{
   const GLuint vertex =
      CompileShader(GL_VERTEX_SHADER,
                    DarkVitaVideo::FsrVertexShaderSource());
   const GLuint fragment =
      CompileShader(GL_FRAGMENT_SHADER,
                    DarkVitaVideo::FsrFragmentShaderSource());
   if (!vertex || !fragment)
      return false;
   pass.program = glCreateProgram();
   if (!pass.program)
      return false;
   glAttachShader(pass.program, vertex);
   glAttachShader(pass.program, fragment);
   glBindAttribLocation(pass.program, 0, "aPosition");
   glBindAttribLocation(pass.program, 1, "aTexCoord");
   glLinkProgram(pass.program);
   glDeleteShader(vertex);
   glDeleteShader(fragment);
   GLint linked = GL_FALSE;
   glGetProgramiv(pass.program, GL_LINK_STATUS, &linked);
   if (linked != GL_TRUE)
   {
      glDeleteProgram(pass.program);
      pass.program = 0;
      return false;
   }
   pass.position = glGetAttribLocation(pass.program, "aPosition");
   pass.texCoord = glGetAttribLocation(pass.program, "aTexCoord");
   pass.textureUniform = glGetUniformLocation(pass.program, "uTexture");
   pass.texelSize = glGetUniformLocation(pass.program, "uTexelSize");
   pass.sharpness = glGetUniformLocation(pass.program, "uSharpness");
   glGenTextures(1, &pass.texture);
   glGenFramebuffers(1, &pass.framebuffer);
   return pass.program != 0 && pass.texture != 0 &&
          pass.framebuffer != 0;
}

bool ResizeFsrPass(sFsrPass &pass, int width, int height)
{
   if (!pass.texture || !pass.framebuffer)
      return false;
   glBindTexture(GL_TEXTURE_2D, pass.texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
   glBindFramebuffer(GL_FRAMEBUFFER, pass.framebuffer);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                          GL_TEXTURE_2D, pass.texture, 0);
   GLint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glBindTexture(GL_TEXTURE_2D, 0);
   return status == GL_FRAMEBUFFER_COMPLETE;
}

void DrawFsrPass(const sFsrPass &pass, int renderWidth,
                 int renderHeight)
{
   glViewport(0, 0, kScreenWidth, kScreenHeight);
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glUseProgram(pass.program);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, pass.texture);
   glUniform1i(pass.textureUniform, 0);
   glUniform2f(pass.texelSize, 1.0f / renderWidth,
               1.0f / renderHeight);
   glUniform1f(pass.sharpness, 0.55f);
   const GLfloat vertices[] = {
      -1.0f, -1.0f, 0.0f, 0.0f,
       1.0f, -1.0f, 1.0f, 0.0f,
      -1.0f,  1.0f, 0.0f, 1.0f,
       1.0f,  1.0f, 1.0f, 1.0f
   };
   glEnableVertexAttribArray(static_cast<GLuint>(pass.position));
   glEnableVertexAttribArray(static_cast<GLuint>(pass.texCoord));
   glVertexAttribPointer(static_cast<GLuint>(pass.position), 2, GL_FLOAT,
                         GL_FALSE, 4 * sizeof(GLfloat), vertices);
   glVertexAttribPointer(static_cast<GLuint>(pass.texCoord), 2, GL_FLOAT,
                         GL_FALSE, 4 * sizeof(GLfloat), vertices + 2);
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
   glDisableVertexAttribArray(static_cast<GLuint>(pass.position));
   glDisableVertexAttribArray(static_cast<GLuint>(pass.texCoord));
   glBindTexture(GL_TEXTURE_2D, 0);
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
   for (int index = 0; index < 4; ++index)
   {
      const bool active =
         static_cast<int>(settings.spatialUpscale) == index;
      const int y = kSpatialY + index * 26;
      DrawRect(shader, settings, kSliderX, y, kSliderWidth, 18,
               active ? 0.95f : 0.12f,
               active ? 0.95f : 0.15f,
               active ? 1.0f : 0.20f);
   }
   if (selected == 2)
      DrawRect(shader, settings, kSliderX - 4, kSpatialY - 4,
               kSliderWidth + 8, 3, 0.95f, 0.95f, 1.0f);
   DrawLetter(shader, settings, 'S', kSliderX - 34, kSpatialY + 8);
}

void DrawControlLayouts(const sShader &shader,
                        const DarkVitaVideo::Settings &settings,
                        DarkVitaControls::Layout layout,
                        bool showTriggerIcons)
{
   for (int index = 0; index < 3; ++index)
   {
      const bool active =
         static_cast<int>(layout) == index;
      DrawRect(shader, settings, 400 + index * 54, 490, 44, 8,
               active ? 0.95f : 0.25f,
               active ? 0.95f : 0.28f,
               active ? 1.0f : 0.32f);
   }
   if (showTriggerIcons)
   {
      DrawRect(shader, settings, 16, kScreenHeight - kTriggerSize - 16,
               kTriggerSize, kTriggerSize, 0.55f, 0.12f, 0.12f);
      DrawRect(shader, settings, kScreenWidth - kTriggerSize - 16,
               kScreenHeight - kTriggerSize - 16, kTriggerSize, kTriggerSize,
               0.12f, 0.45f, 0.55f);
   }
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

void LogTriggerAction(const char *trigger,
                      DarkVitaControls::Action action)
{
   if (action != DarkVitaControls::Action::None)
      SDL_Log("Vita %s action: %d", trigger, static_cast<int>(action));
}

void SaveControls(const std::string &path,
                  const DarkVitaControls::Settings &settings)
{
   std::string error;
   if (!DarkVitaControls::Save(path, settings, &error))
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                   "Could not save vita_controls.cfg: %s", error.c_str());
}

void ChangeSlider(DarkVitaVideo::Settings &settings, int selected, int delta,
                  const std::string &path)
{
   if (selected == 2)
   {
      int mode = static_cast<int>(settings.spatialUpscale);
      mode = std::clamp(mode + (delta > 0 ? 1 : -1), 0, 3);
      settings.spatialUpscale =
         static_cast<DarkVitaVideo::SpatialUpscale>(mode);
      int width = 960;
      int height = 544;
      DarkVitaVideo::SpatialRenderResolution(settings.spatialUpscale,
                                             &width, &height);
      SDL_Log("Vita spatial upscaling: %s (%dx%d%s)",
              DarkVitaVideo::SpatialUpscaleName(settings.spatialUpscale),
              width, height,
              DarkVitaVideo::SpatialUpscaleNeedsFsr(settings.spatialUpscale)
                 ? ", FSR 1.0" : "");
      SaveSettings(path, settings);
      return;
   }
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
   else if (screenY >= kSpatialY - 12 &&
            screenY <= kSpatialY + 104)
   {
      selected = 2;
      const int index = std::clamp(
         (screenY - kSpatialY) / 26, 0, 3);
      settings.spatialUpscale =
         static_cast<DarkVitaVideo::SpatialUpscale>(index);
      return true;
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
   const std::string controlsPath =
      std::string(DARK_VITA_ASSET_ROOT) + "/vita_controls.cfg";
   DarkVitaVideo::LoadResult loaded = DarkVitaVideo::Load(optionsPath);
   DarkVitaVideo::Settings settings = loaded.settings;
   if (!loaded.error.empty())
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                   "Could not load vita_options.cfg: %s",
                   loaded.error.c_str());
   else if (!loaded.found)
      SDL_Log("Using neutral Vita Slim video settings");
   DarkVitaControls::Settings controls =
      DarkVitaControls::Load(controlsPath);
   SDL_Log("Vita control layout: %s",
           DarkVitaControls::LayoutName(controls.layout));

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
   sFsrPass fsrPass;
   if (!CreateFsrPass(fsrPass))
   {
      SDL_LogError(SDL_LOG_CATEGORY_RENDER,
                   "Could not create Vita Mobile FSR pass");
   }

   int fsrWidth = 960;
   int fsrHeight = 544;
   DarkVitaVideo::SpatialRenderResolution(settings.spatialUpscale,
                                          &fsrWidth, &fsrHeight);
   bool fsrTargetReady = fsrPass.framebuffer != 0 &&
      ResizeFsrPass(fsrPass, fsrWidth, fsrHeight);
   int fsrMode = static_cast<int>(settings.spatialUpscale);

   bool running = true;
   bool touchChanged = false;
   int selected = 0;
   uint32_t triggerIconsVisibleUntil = 0;
   bool hasShownTriggerIcons = false;
   DarkVitaControls::TouchTrigger leftTrigger;
   DarkVitaControls::TouchTrigger rightTrigger;
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
               selected = std::clamp(selected - 1, 0, 2);
               break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
               selected = std::clamp(selected + 1, 0, 2);
               break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
               ChangeSlider(settings, selected, -2, optionsPath);
               break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
               ChangeSlider(settings, selected, 2, optionsPath);
               break;
            case SDL_CONTROLLER_BUTTON_A:
            {
               int layout = static_cast<int>(controls.layout);
               controls.layout =
                  DarkVitaControls::ClampLayout(layout - 1);
               SaveControls(controlsPath, controls);
               SDL_Log("Vita control layout: %s",
                       DarkVitaControls::LayoutName(controls.layout));
               break;
            }
            case SDL_CONTROLLER_BUTTON_B:
            {
               int layout = static_cast<int>(controls.layout);
               controls.layout =
                  DarkVitaControls::ClampLayout(layout + 1);
               SaveControls(controlsPath, controls);
               SDL_Log("Vita control layout: %s",
                       DarkVitaControls::LayoutName(controls.layout));
               break;
            }
            default:
               break;
            }
         }
         else if (event.type == SDL_FINGERDOWN ||
                  event.type == SDL_FINGERMOTION)
         {
            const uint32_t now = SDL_GetTicks();
            touchChanged |= SetSliderFromTouch(
                     settings, selected, event.tfinger.x, event.tfinger.y);
            if (DarkVitaControls::PointInTriggerCorner(
                   event.tfinger.x, event.tfinger.y,
                   DarkVitaControls::Trigger::Left))
            {
               DarkVitaControls::UpdateTouchTrigger(
                  controls.layout, DarkVitaControls::Trigger::Left,
                  leftTrigger, event.tfinger.fingerId, true, now);
               triggerIconsVisibleUntil = now + 2200;
               hasShownTriggerIcons = true;
            }
            if (DarkVitaControls::PointInTriggerCorner(
                   event.tfinger.x, event.tfinger.y,
                   DarkVitaControls::Trigger::Right))
            {
               DarkVitaControls::UpdateTouchTrigger(
                  controls.layout, DarkVitaControls::Trigger::Right,
                  rightTrigger, event.tfinger.fingerId, true, now);
               triggerIconsVisibleUntil = now + 2200;
               hasShownTriggerIcons = true;
            }
         }
         else if (event.type == SDL_FINGERUP)
         {
            if (touchChanged)
            {
               SaveSettings(optionsPath, settings);
               touchChanged = false;
            }
            const uint32_t now = SDL_GetTicks();
            LogTriggerAction("LB", DarkVitaControls::UpdateTouchTrigger(
               controls.layout, DarkVitaControls::Trigger::Left,
               leftTrigger, event.tfinger.fingerId, true, now));
            LogTriggerAction("RB", DarkVitaControls::UpdateTouchTrigger(
               controls.layout, DarkVitaControls::Trigger::Right,
               rightTrigger, event.tfinger.fingerId, true, now));
            const DarkVitaControls::Action leftAction =
               DarkVitaControls::UpdateTouchTrigger(
                  controls.layout, DarkVitaControls::Trigger::Left,
                  leftTrigger, event.tfinger.fingerId, false, now);
            const DarkVitaControls::Action rightAction =
               DarkVitaControls::UpdateTouchTrigger(
                  controls.layout, DarkVitaControls::Trigger::Right,
                  rightTrigger, event.tfinger.fingerId, false, now);
            LogTriggerAction("LB", leftAction);
            LogTriggerAction("RB", rightAction);
         }
      }

      const uint32_t now = SDL_GetTicks();
      if (leftTrigger.down)
         LogTriggerAction("LB", DarkVitaControls::UpdateTouchTrigger(
            controls.layout, DarkVitaControls::Trigger::Left,
            leftTrigger, leftTrigger.fingerId, true, now));
      if (rightTrigger.down)
         LogTriggerAction("RB", DarkVitaControls::UpdateTouchTrigger(
            controls.layout, DarkVitaControls::Trigger::Right,
            rightTrigger, rightTrigger.fingerId, true, now));
      const bool showTriggerIcons =
         leftTrigger.down || rightTrigger.down ||
         (hasShownTriggerIcons &&
          static_cast<int32_t>(triggerIconsVisibleUntil - now) > 0);

      int renderWidth = 960;
      int renderHeight = 544;
      DarkVitaVideo::SpatialRenderResolution(settings.spatialUpscale,
                                             &renderWidth, &renderHeight);
      const bool needsFsr =
         DarkVitaVideo::SpatialUpscaleNeedsFsr(settings.spatialUpscale);
      if (needsFsr && fsrMode !=
            static_cast<int>(settings.spatialUpscale))
      {
         fsrWidth = renderWidth;
         fsrHeight = renderHeight;
         fsrTargetReady = ResizeFsrPass(fsrPass, fsrWidth, fsrHeight);
         fsrMode = static_cast<int>(settings.spatialUpscale);
      }
      if (needsFsr && fsrTargetReady)
      {
         glBindFramebuffer(GL_FRAMEBUFFER, fsrPass.framebuffer);
         glViewport(0, 0, fsrWidth, fsrHeight);
      }
      else
      {
         glBindFramebuffer(GL_FRAMEBUFFER, 0);
         glViewport(0, 0, kScreenWidth, kScreenHeight);
      }
      DrawSettings(shader, settings, selected);
      DrawControlLayouts(shader, settings, controls.layout,
                         showTriggerIcons);
      if (needsFsr && fsrTargetReady)
         DrawFsrPass(fsrPass, fsrWidth, fsrHeight);
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
