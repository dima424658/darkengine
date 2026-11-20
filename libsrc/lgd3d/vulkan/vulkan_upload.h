#pragma once
#include "vulkan_context.h"

struct cVulkanContext::sPendingUpload
{
   ~sPendingUpload();
   vk::Device device{};
   void * mapped{};
   vk::DeviceSize capacity{};
   vk::DeviceSize allocationBytes{};
   bool coherent{};
   vk::raii::DeviceMemory stagingMemory{nullptr};
   vk::raii::Buffer stagingBuffer{nullptr};
   vk::raii::Fence fence{nullptr};
   std::vector<vk::raii::CommandBuffer> commandBuffers;
};

inline cVulkanContext::sPendingUpload::~sPendingUpload()
{
   if (mapped)
      try
      {
         device.unmapMemory(*stagingMemory);
      }
      catch (...)
      {
      }
}
