#include "vulkan_sdl.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <cstdio>
#include <string>
#include <vector>

bool VulkanContextInitializeSDL(cVulkanContext &context, SDL_Window *window,
   char *error, unsigned errorCapacity)
{
   if (!window)
   {
      if (error && errorCapacity)
         std::snprintf(error, errorCapacity, "%s", "SDL window is null");
      return false;
   }

   Uint32 extensionCount = 0;
   const char *const *requiredExtensions =
      SDL_Vulkan_GetInstanceExtensions(&extensionCount);
   if (!requiredExtensions)
   {
      if (error && errorCapacity)
         std::snprintf(error, errorCapacity, "%s", SDL_GetError());
      return false;
   }
   std::vector<const char *> extensions(requiredExtensions,
      requiredExtensions + extensionCount);

   int drawableWidth = 0;
   int drawableHeight = 0;
   SDL_GetWindowSizeInPixels(window, &drawableWidth, &drawableHeight);
   if (drawableWidth <= 0 || drawableHeight <= 0)
   {
      if (error && errorCapacity)
         std::snprintf(error, errorCapacity, "%s",
            "SDL Vulkan window has no drawable area");
      return false;
   }

   std::string surfaceError;
   const bool initialized = context.Initialize(extensions,
      [&surfaceError, window](vk::Instance instance) {
         VkSurfaceKHR surface = VK_NULL_HANDLE;
         if (!SDL_Vulkan_CreateSurface(window,
               static_cast<VkInstance>(instance), nullptr, &surface))
            surfaceError = SDL_GetError();
         return vk::SurfaceKHR(surface);
      }, static_cast<uint32_t>(drawableWidth),
      static_cast<uint32_t>(drawableHeight), error, errorCapacity,
      [](vk::Instance instance, vk::SurfaceKHR surface) {
         SDL_Vulkan_DestroySurface(static_cast<VkInstance>(instance),
            static_cast<VkSurfaceKHR>(surface), nullptr);
      });
   if (!initialized && !surfaceError.empty() && error && errorCapacity)
      std::snprintf(error, errorCapacity, "%s", surfaceError.c_str());
   return initialized;
}
