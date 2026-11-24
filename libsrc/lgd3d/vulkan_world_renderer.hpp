#ifndef DARK_VULKAN_WORLD_RENDERER_HPP
#define DARK_VULKAN_WORLD_RENDERER_HPP
#include "world_scene.hpp"
#include "vulkan_dlss.hpp"
#include <vulkan/vulkan.hpp>
#include <memory>
#include <array>
#include <string>
namespace dark {
/* Raster visibility -> RTXDI/SHaRC -> NRD -> HDR -> native composition.
   Record after native scene capture, submit on the same ordered queue before
   the application buffer, and retire a slot before reusing its resources. */
class VulkanWorldRenderer {
 public:
   VulkanWorldRenderer();
   ~VulkanWorldRenderer();
   // Caller must enable the complete VulkanWorldFeatures RTX feature set.
   bool Initialize(vk::PhysicalDevice physical, vk::Device device, uint32_t graphicsFamily,
                   vk::RenderPass compositePass, vk::Format depthFormat, vk::Format outputFormat,
                   const std::string &shaderDirectory, std::string &error);
   VkCommandBuffer Record(uint32_t slot, const WorldScene &scene, uint32_t width, uint32_t height, std::string &error);
   // Advance temporal state only after the enclosing frame submission succeeds.
   void CompleteSubmission(bool accepted);
   void Composite(vk::CommandBuffer command, uint32_t slot, int x, int y, uint32_t width, uint32_t height);
   /* HDR image for GPU diagnostics/readback; caller owns synchronization. */
   VkImage HdrImage(uint32_t slot) const;
   VkImage NativeDepthImage(uint32_t slot) const;
   // Diagnostic readback: position/depth, NRD normal, base/metal, emission, motion, identity.
   VkImage SurfaceImage(uint32_t slot, uint32_t attachment) const;
   VkImage DirectImage(uint32_t slot, bool specular = false, bool denoised = false) const;
   std::array<uint32_t, 4>
   IndirectStatistics(uint32_t slot) const; // Retire slot before reading: updates, queries, hits, fallback vertices.
   // Settings affect subsequent recording; slot resources are resized only
   // after the caller retires that slot's frame fence.
   struct Workload {
      uint32_t blasBuilds = 0, tlasBuilds = 0, instances = 0;
      uint64_t accelerationBytes = 0;
      double gpuMilliseconds = -1;
      std::array<double, 4> recordCpuMilliseconds{};
      std::array<double, 2> lightingTransparencyGpuMilliseconds{{-1, -1}};
      std::array<double, 7> gpuPhases{{-1, -1, -1, -1, -1, -1, -1}};
      std::array<double, 3> directGpuPhases{{-1, -1, -1}};
   };
   Workload GetWorkload(uint32_t slot) const;
   bool RayTracingEnabled() const;
   struct RaySettings {
      bool gi = true;
      float giStrength = 1;
      bool sharc = true, sharcStatistics = false;
      uint32_t sharcEntries = 1u << 20, sharcDownscale = 5, sharcAccumulation = 8, sharcStaleFrames = 64;
      float sharcSceneScale = 50;
      // 0 lets NRD track elapsed time; positive values support fixed-step hosts
      // and deterministic temporal GPU fixtures. Not a gameplay quality knob.
      float denoiserFrameTimeMilliseconds = 0;
   };
   // Configure before recording; all GPU work must be retired before changing modes.
   bool ConfigureDlss(vk::Instance instance, VulkanDlss::Quality quality, std::string &error);
   bool DlssEnabled() const;
   VkImage UpscaledImage(uint32_t slot) const;
   std::array<uint32_t, 2> RenderExtent(uint32_t slot) const;
   void SetRaySettings(const RaySettings &settings);
   struct TextureWorkload {
      uint32_t createdImages = 0, createdStagingBuffers = 0, uploads = 0, evictedImages = 0;
      uint64_t cachedBytes = 0;
   };
   TextureWorkload GetTextureWorkload(uint32_t slot) const;
   void SetTextureCacheBudget(uint64_t bytes); // Evicts only images unreferenced by frame slots.
   void Shutdown();                            // All GPU work must be retired first.
 private:
   struct Impl;
   std::unique_ptr<Impl> m;
};
} // namespace dark
#endif
