#include <SDL3/SDL.h>
#include <nfd.h>
#include <nfd_sdl3.h>

#include <sdl_window.h>
#include <dbfile.h>
#include <filemenu.h>
#include <lg.h>
#include <platform_services.h>
#include <winui.h>

#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

EXTERN void load_file(char *path);
EXTERN void save_world(char *path);

namespace
{
void AddDefaultExtension(std::string &path, const char *extension)
{
   if (!extension || !*extension)
      return;
   const size_t separator = path.find_last_of("/\\");
   const size_t dot = path.find_last_of('.');
   if (dot == std::string::npos || (separator != std::string::npos && dot < separator))
   {
      path += '.';
      path += extension;
   }
}
} // namespace

extern "C" char *FileDialog(eFileMenuMode mode, sFileMenuFilter *filters,
                            const char *defaultExtension, char *buffer, int bufferLength)
{
   if (PlatformBatchMode())
   {
      PlatformBatchReportError(
          "DromEd batch", "A file command omitted its path; batch mode requires explicit paths.");
      return NULL;
   }
   if (!buffer || bufferLength <= 0)
      return NULL;

   if (NFD_Init() != NFD_OKAY)
   {
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "DromEd", NFD_GetError(),
                               SDLDisplayGetWindow());
      return NULL;
   }
   struct DialogSession
   {
      ~DialogSession() { NFD_Quit(); }
   } session;
   NFD_SetDisplayPropertiesFromSDL();
   std::vector<std::string> extensions;
   std::vector<nfdu8filteritem_t> items;
   if (filters)
      for (auto *filter = filters; filter->text; ++filter)
      {
         std::string pattern = filter->pattern ? filter->pattern : "";
         if (pattern == "*.*" || pattern == "*")
            continue;
         std::string converted;
         for (size_t i = 0; i < pattern.size(); ++i)
         {
            if (pattern[i] == '*' && i + 1 < pattern.size() && pattern[i + 1] == '.')
            {
               ++i;
               continue;
            }
            converted += pattern[i] == ';' ? ',' : pattern[i];
         }
         if (!converted.empty())
         {
            extensions.push_back(converted);
            items.push_back({filter->text, nullptr});
         }
      }
   for (size_t i = 0; i < items.size(); ++i)
      items[i].spec = extensions[i].c_str();
   std::error_code directoryError;
   const std::string directory = std::filesystem::current_path(directoryError).u8string();
   nfdu8char_t *selected = nullptr;
   nfdresult_t outcome;
   if (mode == kFileMenuSave)
   {
      nfdsavedialogu8args_t args{};
      args.filterList = items.data();
      args.filterCount = items.size();
      args.defaultPath = directoryError ? nullptr : directory.c_str();
      NFD_GetNativeWindowFromSDLWindow(SDLDisplayGetWindow(), &args.parentWindow);
      outcome = NFD_SaveDialogU8_With(&selected, &args);
   }
   else
   {
      nfdopendialogu8args_t args{};
      args.filterList = items.data();
      args.filterCount = items.size();
      args.defaultPath = directoryError ? nullptr : directory.c_str();
      NFD_GetNativeWindowFromSDLWindow(SDLDisplayGetWindow(), &args.parentWindow);
      outcome = NFD_OpenDialogU8_With(&selected, &args);
   }
   std::unique_ptr<nfdu8char_t, decltype(&NFD_FreePathU8)> selection(selected, &NFD_FreePathU8);
   std::string path = outcome == NFD_OKAY && selected ? selected : "";
   if (outcome == NFD_ERROR)
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "DromEd", NFD_GetError(),
                               SDLDisplayGetWindow());
   if (path.empty())
      return NULL;

   if (mode == kFileMenuSave && defaultExtension && *defaultExtension)
   {
      const std::string selectedPath = path;
      AddDefaultExtension(path, defaultExtension);
      std::error_code error;
      if (path != selectedPath && std::filesystem::exists(std::filesystem::u8path(path), error) &&
          !winui_GetYorN("The file already exists. Replace it?"))
         return NULL;
   }

   if (path.size() >= static_cast<size_t>(bufferLength))
   {
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "DromEd", "The selected path is too long.",
                               NULL);
      return NULL;
   }
   memcpy(buffer, path.c_str(), path.size() + 1);
   return buffer;
}

extern "C" void FileMenu(void)
{
   if (PlatformBatchMode())
   {
      PlatformBatchReportError("DromEd", "File menu requested in batch mode");
      return;
   }
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
