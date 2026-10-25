#pragma pack(push, 8)
#include <SDL2/SDL.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>
#pragma pack(pop)

#include <lg.h>
#include <dbfile.h>
#include <filemenu.h>
#include <winui.h>

EXTERN void load_file(char *path);
EXTERN void save_world(char *path);

namespace {
bool read_zenity_path(eFileMenuMode mode, std::string &path)
{
   const char *command = mode == kFileMenuSave
      ? "zenity --file-selection --save --confirm-overwrite --title='DromEd: Save' 2>/dev/null"
      : "zenity --file-selection --title='DromEd: Open' 2>/dev/null";
   FILE *pipe = popen(command, "r");
   if (!pipe)
      return false;

   char line[4096];
   const bool selected = fgets(line, sizeof(line), pipe) != NULL;
   const int status = pclose(pipe);
   if (!selected || status != 0)
      return false;

   path = line;
   while (!path.empty() && (path.back() == '\n' || path.back() == '\r'))
      path.pop_back();
   return !path.empty();
}

bool read_terminal_path(eFileMenuMode mode, std::string &path)
{
   if (!isatty(STDIN_FILENO))
      return false;
   fprintf(stderr, "DromEd %s path (empty to cancel): ",
           mode == kFileMenuSave ? "save" : "open");
   fflush(stderr);
   char line[4096];
   if (!fgets(line, sizeof(line), stdin))
      return false;
   path = line;
   while (!path.empty() && (path.back() == '\n' || path.back() == '\r'))
      path.pop_back();
   return !path.empty();
}
}

extern "C" char *FileDialog(eFileMenuMode mode, sFileMenuFilter *,
                             const char *defaultExtension,
                             char *buffer, int bufferLength)
{
   if (!buffer || bufferLength <= 0)
      return NULL;

   std::string path;
   if (!read_zenity_path(mode, path) && !read_terminal_path(mode, path))
      return NULL;

   if (mode == kFileMenuSave && defaultExtension && *defaultExtension)
   {
      const size_t slash = path.find_last_of('/');
      const size_t dot = path.find_last_of('.');
      if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
      {
         path += '.';
         path += defaultExtension;
      }
   }

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
   if (SDL_ShowMessageBox(&data, &selected) != 0)
      return;

   char path[4096] = {};
   if (selected == 1 && FileDialog(kFileMenuLoad, NULL, "mis", path, sizeof(path)))
      load_file(path);
   else if (selected == 2 && FileDialog(kFileMenuSave, NULL, "mis", path, sizeof(path)))
      save_world(path);
   else if (selected == 3 && winui_GetYorN("Clear the current world?"))
      dbBuildDefault();
}
