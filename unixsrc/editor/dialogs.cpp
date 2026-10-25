#pragma pack(push, 8)
#include <SDL2/SDL.h>
#pragma pack(pop)

#include <lg.h>
#include <ged_win.h>
#include <winui.h>

static BOOL ask_yes_no(const char *message)
{
   const SDL_MessageBoxButtonData buttons[] = {
      {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "No"},
      {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Yes"},
   };
   const SDL_MessageBoxData data = {
      SDL_MESSAGEBOX_INFORMATION,
      NULL,
      "DromEd",
      message ? message : "",
      2,
      buttons,
      NULL,
   };
   int selected = 0;
   if (SDL_ShowMessageBox(&data, &selected) != 0)
      return FALSE;
   return selected == 1;
}

extern "C" BOOL ged_winui_GetYorN(char *message)
{
   return ask_yes_no(message);
}

extern "C" void ged_winui_Text(char *message)
{
   SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "DromEd",
                            message ? message : "", NULL);
}

extern "C" BOOL winui_GetYorN(const char *message)
{
   return ask_yes_no(message);
}

extern "C" void winui_Text(const char *message)
{
   SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "DromEd",
                            message ? message : "", NULL);
}
