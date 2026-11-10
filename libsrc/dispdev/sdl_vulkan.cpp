#include "sdl_vulkan.hpp"
#include "sdl_window.h"

#include <SDL3/SDL_vulkan.h>

#include <cstdio>

namespace
{
void SetError(char *error, unsigned errorCapacity, const char *message)
{
   if (error && errorCapacity)
      std::snprintf(error, errorCapacity, "%s", message ? message : "");
}
}

bool SDLVulkanGetDrawableSize(uint32_t &drawableWidth,
   uint32_t &drawableHeight, char *error, unsigned errorCapacity)
{
   SDL_Window *window = SDLDisplayGetWindow();
   if (!window)
   {
      SetError(error, errorCapacity, "SDL window is null");
      return false;
   }

   int width = 0;
   int height = 0;
   if (!SDL_GetWindowSizeInPixels(window, &width, &height) ||
       width <= 0 || height <= 0)
   {
      SetError(error, errorCapacity, "SDL Vulkan window has no drawable area");
      return false;
   }

   drawableWidth = static_cast<uint32_t>(width);
   drawableHeight = static_cast<uint32_t>(height);
   SetError(error, errorCapacity, "");
   return true;
}

bool SDLVulkanGetInstanceExtensions(std::vector<const char *> &extensions,
   uint32_t &drawableWidth, uint32_t &drawableHeight, char *error,
   unsigned errorCapacity)
{
   SDL_Window *window = SDLDisplayGetWindow();
   if (!window)
   {
      SetError(error, errorCapacity, "SDL window is null");
      return false;
   }

   Uint32 extensionCount = 0;
   const char *const *requiredExtensions =
      SDL_Vulkan_GetInstanceExtensions(&extensionCount);
   if (!requiredExtensions)
   {
      SetError(error, errorCapacity, SDL_GetError());
      return false;
   }
   extensions.assign(requiredExtensions,
      requiredExtensions + extensionCount);
   if (!SDLVulkanGetDrawableSize(drawableWidth, drawableHeight, error,
         errorCapacity))
      return false;
   SetError(error, errorCapacity, "");
   return true;
}

bool SDLVulkanCreateSurface(vk::Instance instance, vk::SurfaceKHR &surface,
   char *error, unsigned errorCapacity)
{
   SDL_Window *window = SDLDisplayGetWindow();
   if (!window)
   {
      SetError(error, errorCapacity, "SDL window is null");
      return false;
   }

   VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
   if (!SDL_Vulkan_CreateSurface(window, static_cast<VkInstance>(instance),
         nullptr, &rawSurface))
   {
      SetError(error, errorCapacity, SDL_GetError());
      return false;
   }

   surface = vk::SurfaceKHR(rawSurface);
   SetError(error, errorCapacity, "");
   return true;
}
