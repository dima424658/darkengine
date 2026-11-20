#pragma once
#include <utility>
#include "vulkan_scene_renderer.h"
#include "vulkan_renderer_types.h"

struct cVulkanSceneRenderer::sVertexBuffer
{
   sVertexBuffer() = default;
   sVertexBuffer(const sVertexBuffer &) = delete;
   sVertexBuffer & operator=(const sVertexBuffer &) = delete;
   sVertexBuffer(sVertexBuffer && other) noexcept;
   ~sVertexBuffer();

   sVertexBuffer & operator=(sVertexBuffer && other) noexcept;

   void Reset() noexcept;

   vk::Device device{};
   vk::raii::DeviceMemory memory{nullptr};
   vk::raii::Buffer buffer{nullptr};
   void * mapped{};
   vk::DeviceSize capacity{};
   bool hostCoherent{};
};

inline cVulkanSceneRenderer::sVertexBuffer::sVertexBuffer(
   sVertexBuffer && other) noexcept
    : device(std::exchange(other.device, nullptr)),
      memory(std::move(other.memory)), buffer(std::move(other.buffer)),
      mapped(std::exchange(other.mapped, nullptr)),
      capacity(std::exchange(other.capacity, 0)),
      hostCoherent(std::exchange(other.hostCoherent, false))
{
}

inline cVulkanSceneRenderer::sVertexBuffer::~sVertexBuffer()
{
   Reset();
}

inline void cVulkanSceneRenderer::sVertexBuffer::Reset() noexcept
{
   if (mapped)
   {
      try
      {
         device.unmapMemory(*memory);
      }
      catch (...)
      {
      }
      mapped = nullptr;
   }
   buffer.clear();
   memory.clear();
   device = nullptr;
   capacity = 0;
   hostCoherent = false;
}

inline cVulkanSceneRenderer::sVertexBuffer &
cVulkanSceneRenderer::sVertexBuffer::operator=(sVertexBuffer && other) noexcept
{
   if (this != &other)
   {
      Reset();
      device = std::exchange(other.device, nullptr);
      memory = std::move(other.memory);
      buffer = std::move(other.buffer);
      mapped = std::exchange(other.mapped, nullptr);
      capacity = std::exchange(other.capacity, 0);
      hostCoherent = std::exchange(other.hostCoherent, false);
   }
   return *this;
}
