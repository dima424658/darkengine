#ifndef DARK_VULKAN_SDL_HPP
#define DARK_VULKAN_SDL_HPP

#include "vulkan_context.hpp"

struct SDL_Window;

bool VulkanContextInitializeSDL(cVulkanContext &context, SDL_Window *window,
   char *error, unsigned errorCapacity);

#endif
