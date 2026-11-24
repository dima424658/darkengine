#ifndef DARK_VULKAN_RAY_SCENE_HPP
#define DARK_VULKAN_RAY_SCENE_HPP
#include "world_scene.hpp"
#include <vulkan/vulkan.hpp>
#include <memory>
#include <string>
namespace dark {
// Frame-slot owned BLAS/TLAS. Call Record only after retiring that slot's fence.
// Camera movement does not rebuild geometry. Poses affect TLAS; deformation
// rebuilds only the affected mesh BLAS. Offscreen casters remain in the TLAS.
class VulkanRayScene {
public:
   VulkanRayScene();
   ~VulkanRayScene();
   bool Initialize(vk::PhysicalDevice physical,vk::Device device,std::string &error);
   bool Record(vk::CommandBuffer command,uint32_t slot,const WorldScene &scene,
      const std::unordered_map<uint64_t,uint32_t> &materials,std::string &error);
   VkAccelerationStructureKHR TopLevel(uint32_t slot) const;
   vk::DescriptorBufferInfo Triangles(uint32_t slot) const;
   vk::DescriptorBufferInfo Instances(uint32_t slot) const;
   // Last accepted scene, retained by the recording slot until its fence retires.
   // First frames bind the current scene instead; the lighting history is invalid.
   VkAccelerationStructureKHR PreviousTopLevel(uint32_t slot) const;
   vk::DescriptorBufferInfo PreviousTriangles(uint32_t slot) const;
   vk::DescriptorBufferInfo PreviousInstances(uint32_t slot) const;
   void CompleteSubmission(bool accepted);
   struct Workload {uint32_t blasBuilds=0,tlasBuilds=0,instances=0;uint64_t bytes=0;};
   Workload Stats(uint32_t slot) const;
   void Invalidate(uint32_t slot);
private:
   struct Impl;
   std::unique_ptr<Impl> m;
};
}
#endif
