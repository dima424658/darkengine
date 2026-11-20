#pragma once
#include <utility>
#include "vulkan_scene_renderer.h"
#include "vulkan_renderer_types.h"

struct cVulkanSceneRenderer::sTexture
{
   sTexture() = default;
   sTexture(const sTexture &) = delete;
   sTexture & operator=(const sTexture &) = delete;
   sTexture(sTexture && other) noexcept;
   sTexture & operator=(sTexture && other) noexcept;
   ~sTexture();

   void Reset();

   vk::raii::DeviceMemory memory{nullptr};
   vk::raii::Image image{nullptr};
   vk::raii::ImageView view{nullptr};
   int width{};
   int height{};
   uint64_t identity{};
};

inline cVulkanSceneRenderer::sTexture::sTexture(sTexture && other) noexcept
    : memory(std::move(other.memory)), image(std::move(other.image)),
      view(std::move(other.view)), width(other.width), height(other.height),
      identity(std::exchange(other.identity, 0))
{
   other.width = other.height = 0;
}

inline cVulkanSceneRenderer::sTexture::~sTexture()
{
   Reset();
}

inline void cVulkanSceneRenderer::sTexture::Reset()
{
   view.clear();
   image.clear();
   memory.clear();
   width = height = 0;
   identity = 0;
}

inline cVulkanSceneRenderer::sTexture &
cVulkanSceneRenderer::sTexture::operator=(sTexture && other) noexcept
{
   if (this != &other)
   {
      Reset();
      memory = std::move(other.memory);
      image = std::move(other.image);
      view = std::move(other.view);
      width = other.width;
      height = other.height;
      identity = std::exchange(other.identity, 0);
      other.width = other.height = 0;
   }
   return *this;
}
