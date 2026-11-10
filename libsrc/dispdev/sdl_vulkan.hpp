#ifndef DARK_SDL_VULKAN_H
#define DARK_SDL_VULKAN_H

#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <vector>

bool SDLVulkanGetInstanceExtensions(std::vector<const char *> &extensions,
   uint32_t &drawableWidth, uint32_t &drawableHeight, char *error,
   unsigned errorCapacity);
bool SDLVulkanGetDrawableSize(uint32_t &drawableWidth,
   uint32_t &drawableHeight, char *error, unsigned errorCapacity);
bool SDLVulkanCreateSurface(vk::Instance instance, vk::SurfaceKHR &surface,
   char *error, unsigned errorCapacity);

#endif
