#include "sdl_vulkan.hpp"
#include "../lgd3d/vulkan_context.hpp"
#include "sdl_window.h"

#include <SDL3/SDL_vulkan.h>

#include <cstdio>
#include <string>
#include <vector>

bool VulkanContextInitializeSDL(cVulkanContext &context,
   int physicalDeviceIndex, char *error, unsigned errorCapacity)
{
   SDL_Window *window = SDLDisplayGetWindow();
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
      static_cast<uint32_t>(drawableHeight), physicalDeviceIndex, error,
      errorCapacity);
   if (!initialized && !surfaceError.empty() && error && errorCapacity)
      std::snprintf(error, errorCapacity, "%s", surfaceError.c_str());
   return initialized;
}

bool VulkanContextRecreateSDLSurface(cVulkanContext &context, char *error,
   unsigned errorCapacity)
{
   SDL_Window *window = SDLDisplayGetWindow();
   if (!window)
   {
      if (error && errorCapacity)
         std::snprintf(error, errorCapacity, "%s", "SDL window is null");
      return false;
   }

   int drawableWidth = 0;
   int drawableHeight = 0;
   if (!SDL_GetWindowSizeInPixels(window, &drawableWidth, &drawableHeight) ||
       drawableWidth <= 0 || drawableHeight <= 0)
   {
      if (error && errorCapacity)
         std::snprintf(error, errorCapacity, "%s",
            "SDL Vulkan window has no drawable area");
      return false;
   }

   std::string surfaceError;
   const bool recreated = context.RecreateSurface(
      [&surfaceError, window](vk::Instance instance) {
         VkSurfaceKHR surface = VK_NULL_HANDLE;
         if (!SDL_Vulkan_CreateSurface(window,
               static_cast<VkInstance>(instance), nullptr, &surface))
            surfaceError = SDL_GetError();
         return vk::SurfaceKHR(surface);
      }, static_cast<uint32_t>(drawableWidth),
      static_cast<uint32_t>(drawableHeight));
   if (error && errorCapacity)
      std::snprintf(error, errorCapacity, "%s", recreated ? "" :
         (surfaceError.empty() ? "Vulkan surface recreation failed" :
            surfaceError.c_str()));
   return recreated;
}
