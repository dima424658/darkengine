#include "vita_video_settings.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>

namespace DarkVitaVideo
{
namespace
{
constexpr int kSliderMinimum = 0;
constexpr int kSliderMaximum = 100;
constexpr float kBrightnessRange = 0.12f;
constexpr float kMinimumGamma = 0.80f;
constexpr float kMaximumGamma = 1.20f;

const char kFragmentShader[] =
   "#version 100\n"
   "precision mediump float;\n"
   "varying vec4 vColor;\n"
   "uniform float uBrightness;\n"
   "uniform float uGamma;\n"
   "void main() {\n"
   "  vec3 shifted = clamp(vColor.rgb + vec3(uBrightness), "
   "vec3(0.0), vec3(1.0));\n"
   "  vec3 corrected = pow(shifted, vec3(1.0 / max(uGamma, 0.01)));\n"
   "  gl_FragColor = vec4(corrected, vColor.a);\n"
   "}\n";
const char kVertexShader[] =
   "#version 100\n"
   "attribute vec2 aPosition;\n"
   "attribute vec4 aColor;\n"
   "varying vec4 vColor;\n"
   "void main() {\n"
   "  gl_Position = vec4(aPosition, 0.0, 1.0);\n"
   "  vColor = aColor;\n"
   "}\n";

bool ParseInteger(const std::string &text, int &value)
{
   const auto result =
      std::from_chars(text.data(), text.data() + text.size(), value);
   return result.ec == std::errc{} &&
          result.ptr == text.data() + text.size();
}

std::string Trim(std::string value)
{
   const size_t first = value.find_first_not_of(" \t\r\n");
   if (first == std::string::npos)
      return {};
   const size_t last = value.find_last_not_of(" \t\r\n");
   return value.substr(first, last - first + 1);
}

bool ValidSettings(const Settings &settings)
{
   return settings.brightnessSlider >= kSliderMinimum &&
          settings.brightnessSlider <= kSliderMaximum &&
          settings.gammaSlider >= kSliderMinimum &&
          settings.gammaSlider <= kSliderMaximum;
}

void SetError(std::string *error, const std::string &message)
{
   if (error)
      *error = message;
}
}

float BrightnessOffset(const Settings &settings)
{
   const int value =
      std::clamp(settings.brightnessSlider, kSliderMinimum, kSliderMaximum);
   return (static_cast<float>(value) / kSliderMaximum * 2.0f - 1.0f) *
          kBrightnessRange;
}

float GammaValue(const Settings &settings)
{
   const int value =
      std::clamp(settings.gammaSlider, kSliderMinimum, kSliderMaximum);
   return kMinimumGamma +
          static_cast<float>(value) / kSliderMaximum *
             (kMaximumGamma - kMinimumGamma);
}

Color ApplyCorrection(Color color, const Settings &settings)
{
   const float brightness = BrightnessOffset(settings);
   const float exponent = 1.0f / GammaValue(settings);
   const auto correct = [brightness, exponent](float channel)
   {
      const float shifted = std::clamp(channel + brightness, 0.0f, 1.0f);
      return std::pow(shifted, exponent);
   };
   return {correct(color.red), correct(color.green), correct(color.blue)};
}

const char *FragmentShaderSource()
{
   return kFragmentShader;
}

const char *VertexShaderSource()
{
   return kVertexShader;
}

LoadResult Load(const std::string &path)
{
   LoadResult result;
   std::error_code fileError;
   const bool exists = std::filesystem::exists(path, fileError);
   if (fileError)
   {
      result.error = "cannot inspect video settings file: " +
                     fileError.message();
      return result;
   }
   if (!exists)
      return result;

   std::ifstream input(path);
   if (!input)
   {
      result.error = "cannot open video settings file";
      return result;
   }

   Settings loaded;
   bool foundVersion = false;
   bool foundBrightness = false;
   bool foundGamma = false;
   std::string line;
   size_t lineNumber = 0;
   while (std::getline(input, line))
   {
      ++lineNumber;
      line = Trim(std::move(line));
      if (line.empty() || line[0] == '#')
         continue;
      const size_t separator = line.find('=');
      if (separator == std::string::npos)
      {
         result.error = "invalid video settings entry at line " +
                        std::to_string(lineNumber);
         return result;
      }

      const std::string key = Trim(line.substr(0, separator));
      const std::string value = Trim(line.substr(separator + 1));
      int parsed = 0;
      if (key == "version")
      {
         if (foundVersion || !ParseInteger(value, parsed) || parsed != 1)
         {
            result.error = "unsupported or duplicate video settings version";
            return result;
         }
         foundVersion = true;
      }
      else if (key == "brightness")
      {
         if (foundBrightness || !ParseInteger(value, parsed) ||
             parsed < kSliderMinimum || parsed > kSliderMaximum)
         {
            result.error = "invalid or duplicate brightness slider value";
            return result;
         }
         loaded.brightnessSlider = parsed;
         foundBrightness = true;
      }
      else if (key == "gamma")
      {
         if (foundGamma || !ParseInteger(value, parsed) ||
             parsed < kSliderMinimum || parsed > kSliderMaximum)
         {
            result.error = "invalid or duplicate gamma slider value";
            return result;
         }
         loaded.gammaSlider = parsed;
         foundGamma = true;
      }
   }
   if (input.bad())
   {
      result.error = "failed while reading video settings file";
      return result;
   }
   if (!foundVersion || !foundBrightness || !foundGamma)
   {
      result.error = "video settings file is missing required values";
      return result;
   }

   result.settings = loaded;
   result.found = true;
   return result;
}

bool Save(const std::string &path, const Settings &settings,
          std::string *error)
{
   if (path.empty() || !ValidSettings(settings))
   {
      SetError(error, "invalid video settings path or slider value");
      return false;
   }

   const std::string temporaryPath = path + ".tmp";
   {
      std::ofstream output(temporaryPath, std::ios::trunc);
      if (!output)
      {
         SetError(error, "cannot create temporary video settings file");
         return false;
      }
      output << "version=1\n"
             << "brightness=" << settings.brightnessSlider << '\n'
             << "gamma=" << settings.gammaSlider << '\n';
      output.flush();
      if (!output)
      {
         SetError(error, "failed while writing video settings file");
         output.close();
         std::error_code removeError;
         std::filesystem::remove(temporaryPath, removeError);
         return false;
      }
   }

   std::error_code renameError;
   std::filesystem::rename(temporaryPath, path, renameError);
   if (renameError)
   {
      SetError(error, "cannot replace video settings file: " +
                      renameError.message());
      std::error_code removeError;
      std::filesystem::remove(temporaryPath, removeError);
      return false;
   }
   if (error)
      error->clear();
   return true;
}
}
