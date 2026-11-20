#pragma once
#include "vulkan_scene_renderer.h"
#include "vulkan_renderer_types.h"
#include "vulkan_descriptor_hash.h"
#include "vulkan_vertex_buffer.h"

struct cVulkanSceneRenderer::sFrameResources
{
   sVertexBuffer vertexBuffer;
   std::vector<sVertexBuffer> retiredVertexBuffers;
   std::vector<vk::raii::DescriptorPool> descriptorPools;
   size_t descriptorPoolIndex{};
   vk::DeviceSize vertexOffset{};
   struct sCachedDescriptor
   {
      vk::DescriptorSet set;
      size_t pool;
      uint64_t lastUsed;
   };
   std::unordered_map<sDescriptorKey, sCachedDescriptor, sDescriptorHash>
      descriptors;
   uint64_t descriptorEpoch{};
   vk::CommandBuffer cachedCommandBuffer{};
   vk::Pipeline boundPipeline{};
   vk::DescriptorSet boundDescriptor{};
   vk::Buffer boundVertexBuffer{};
   std::array<int, 4> viewport{};
   bool viewportValid{};
   sPushConstants constants{};
   bool constantsValid{};

   void ResetDrawState(vk::CommandBuffer command = {});
};

inline void
cVulkanSceneRenderer::sFrameResources::ResetDrawState(vk::CommandBuffer command)
{
   cachedCommandBuffer = command;
   boundPipeline = nullptr;
   boundDescriptor = nullptr;
   boundVertexBuffer = nullptr;
   viewportValid = constantsValid = false;
}
