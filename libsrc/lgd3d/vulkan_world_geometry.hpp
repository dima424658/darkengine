#ifndef DARK_VULKAN_WORLD_GEOMETRY_HPP
#define DARK_VULKAN_WORLD_GEOMETRY_HPP
#include "world_scene.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <string>
namespace dark {
// Shared raster geometry for lighting and shadow-map passes. No device addresses
// or RT features. The caller retires a frame slot before updating its buffers.
class VulkanWorldGeometry {
public:
   VulkanWorldGeometry();
   ~VulkanWorldGeometry();
   VulkanWorldGeometry(const VulkanWorldGeometry &)=delete;
   VulkanWorldGeometry &operator=(const VulkanWorldGeometry &)=delete;
   bool Initialize(VkPhysicalDevice physical,VkDevice device);
   bool Update(uint32_t slot,const WorldScene &scene,std::string &error);
   // Meshes share arena buffers. Request byte offsets for drawing/readback;
   // buffer handles alone identify storage, not an individual mesh allocation.
   bool MeshBuffers(uint32_t slot,RenderSceneHandle mesh,VkBuffer &vertices,VkBuffer &indices,VkDeviceSize *vertexOffset=nullptr,VkDeviceSize *indexOffset=nullptr) const;
   size_t MeshCount(uint32_t slot) const;
   void Shutdown(); // Retire all GPU work first.
private:
   struct Impl;
   std::unique_ptr<Impl> m;
};
}
#endif
