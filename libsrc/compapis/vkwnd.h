#ifndef __VKWND_H
#define __VKWND_H

#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <vector>

#include <comtools.h>
#include <dispguid.h>

#undef INTERFACE
#define INTERFACE IVulkanWindow
DECLARE_INTERFACE_(IVulkanWindow,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   STDMETHOD_(BOOL, GetInstanceExtensions)(THIS_ std::vector<const char *> &extensions,
      uint32_t &drawableWidth, uint32_t &drawableHeight, char *error,
      unsigned errorCapacity) PURE;
   STDMETHOD_(BOOL, GetDrawableSize)(THIS_ uint32_t &drawableWidth,
      uint32_t &drawableHeight, char *error, unsigned errorCapacity) PURE;
   STDMETHOD_(BOOL, CreateSurface)(THIS_ vk::Instance instance, vk::SurfaceKHR &surface,
      char *error, unsigned errorCapacity) PURE;
};
#undef INTERFACE

#endif
