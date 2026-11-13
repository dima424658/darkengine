#include <SDL3/SDL.h>

#include <lg.h>
#include <dbfile.h>
#include <filemenu.h>
#include <winui.h>
#include "../../libsrc/dispdev/sdl_window.h"

#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

EXTERN void load_file(char *path);
EXTERN void save_world(char *path);

namespace {
struct sDialogResult
{
   std::mutex mutex;
   std::condition_variable completedCondition;
   bool completed{};
   std::string path;
};

void SDLCALL FileDialogCompleted(void *userdata,
   const char * const *filelist, int)
{
   auto *result = static_cast<sDialogResult *>(userdata);
   {
      std::lock_guard<std::mutex> lock(result->mutex);
      if (filelist && filelist[0])
         result->path = filelist[0];
      result->completed = true;
   }
   result->completedCondition.notify_one();
}

std::vector<SDL_DialogFileFilter> MakeFilters(const sFileMenuFilter *filters,
   std::vector<std::string> &patterns)
{
   std::vector<SDL_DialogFileFilter> result;
   if (!filters)
      return result;
   for (; filters->text; ++filters)
   {
      if (!filters->pattern)
         continue;
      std::string pattern = filters->pattern;
      if (pattern == "*.*")
         pattern = "*";
      else if (pattern.size() > 1 && pattern[0] == '*' && pattern[1] == '.')
         pattern.erase(0, 2);
      patterns.push_back(std::move(pattern));
      result.push_back({filters->text, nullptr});
   }
   for (size_t i = 0; i < result.size(); ++i)
      result[i].pattern = patterns[i].c_str();
   return result;
}

void AddDefaultExtension(std::string &path, const char *extension)
{
   if (!extension || !*extension)
      return;
   const size_t separator = path.find_last_of("/\\");
   const size_t dot = path.find_last_of('.');
   if (dot == std::string::npos ||
       (separator != std::string::npos && dot < separator))
   {
      path += '.';
      path += extension;
   }
}
}

extern "C" char *FileDialog(eFileMenuMode mode, sFileMenuFilter *filters,
                             const char *defaultExtension,
                             char *buffer, int bufferLength)
{
   if (!buffer || bufferLength <= 0)
      return NULL;

   std::vector<std::string> patterns;
   std::vector<SDL_DialogFileFilter> dialogFilters =
      MakeFilters(filters, patterns);
   sDialogResult result;
   SDL_Window *window = SDLDisplayGetWindow();
   if (mode == kFileMenuSave)
      SDL_ShowSaveFileDialog(FileDialogCompleted, &result, window,
         dialogFilters.empty() ? nullptr : dialogFilters.data(),
         static_cast<int>(dialogFilters.size()), nullptr);
   else
      SDL_ShowOpenFileDialog(FileDialogCompleted, &result, window,
         dialogFilters.empty() ? nullptr : dialogFilters.data(),
         static_cast<int>(dialogFilters.size()), nullptr, false);

   std::unique_lock<std::mutex> lock(result.mutex);
   while (!result.completed)
   {
      result.completedCondition.wait_for(lock, std::chrono::milliseconds(10));
      if (!result.completed)
      {
         lock.unlock();
         SDL_PumpEvents();
         lock.lock();
      }
   }
   std::string path = std::move(result.path);
   lock.unlock();
   if (path.empty())
      return NULL;

   if (mode == kFileMenuSave && defaultExtension && *defaultExtension)
      AddDefaultExtension(path, defaultExtension);

   if (path.size() >= static_cast<size_t>(bufferLength))
   {
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "DromEd",
                               "The selected path is too long.", NULL);
      return NULL;
   }
   memcpy(buffer, path.c_str(), path.size() + 1);
   return buffer;
}

extern "C" void FileMenu(void)
{
   const SDL_MessageBoxButtonData buttons[] = {
      {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Cancel"},
      {0, 1, "Load world"},
      {0, 2, "Save world"},
      {0, 3, "Clear world"},
   };
   const SDL_MessageBoxData data = {
      SDL_MESSAGEBOX_INFORMATION,
      NULL,
      "DromEd: File",
      "Choose a file operation",
      4,
      buttons,
      NULL,
   };
   int selected = 0;
   if (!SDL_ShowMessageBox(&data, &selected))
      return;

   char path[4096] = {};
   if (selected == 1 && FileDialog(kFileMenuLoad, NULL, "mis", path, sizeof(path)))
      load_file(path);
   else if (selected == 2 && FileDialog(kFileMenuSave, NULL, "mis", path, sizeof(path)))
      save_world(path);
   else if (selected == 3 && winui_GetYorN("Clear the current world?"))
      dbBuildDefault();
}
