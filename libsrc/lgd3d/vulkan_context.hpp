#ifndef DARK_VULKAN_CONTEXT_HPP
#define DARK_VULKAN_CONTEXT_HPP

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <functional>
#include <memory>
#include <vector>
#include <string>

class cVulkanSceneRenderer;

inline constexpr uint32_t kVulkanFramesInFlight = 2;

// Vulkan instance, platform surface, and a graphics/presentation capable
// device. Rendering resources are layered on this shared context.
class cVulkanContext
{
public:
   using tSurfaceFactory = std::function<vk::SurfaceKHR(vk::Instance)>;

   cVulkanContext() = default;
   ~cVulkanContext();

   bool Initialize(const std::vector<const char *> &surfaceExtensions,
      const tSurfaceFactory &createSurface, uint32_t drawableWidth,
      uint32_t drawableHeight, int physicalDeviceIndex, char *error,
      unsigned errorCapacity, bool enableWorldRendering = false);
   void Shutdown();
   // Applied when creating or recreating the swapchain.
   void SetVSync(bool enabled) { m_vsync = enabled; }
   bool PresentClearFrame(float red, float green, float blue, float alpha);
   // BeginFrame acquires a swapchain image and starts its render pass. The
   // returned command buffer is valid until EndFrame submits and presents it.
   bool BeginFrame(const std::array<float, 4> &clearColor,
      vk::CommandBuffer *commandBuffer,
      const std::function<void(vk::CommandBuffer, uint32_t)> &prepare = {});
   bool EndFrame(vk::CommandBuffer preparation = {});
   // Upload storage is reused only after its fence signals.
   bool SubmitUpload(const void *pixels, int width, int height, int rowBytes,
      const std::function<void(vk::CommandBuffer, vk::Buffer)> &record);
   bool Resize(uint32_t drawableWidth, uint32_t drawableHeight);
   bool RecreateSurface(const tSurfaceFactory &createSurface,
      uint32_t drawableWidth, uint32_t drawableHeight);
   bool WorldRenderingEnabled() const { return m_worldRenderingEnabled; }
   bool WorldRayTracing() const {return m_worldRenderingEnabled && m_worldRayTracing;}
   bool WorldRenderingSupported() const { return m_worldRenderingSupported; }
   const std::string &WorldRenderingUnavailableReason() const { return m_worldRenderingUnavailable; }
   bool NeedsSwapchainRecreation() const { return m_recreateSwapchain; }
   bool NeedsSurfaceRecreation() const { return m_surfaceLost; }

   vk::Instance Instance() const { return m_instance; }
   vk::SurfaceKHR Surface() const { return *m_surface; }
   vk::PhysicalDevice PhysicalDevice() const { return m_physicalDevice; }
   vk::Device Device() const { return m_device; }
   const vk::raii::Device &RaiiDevice() const { return m_device; }
   vk::Queue GraphicsQueue() const { return m_graphicsQueue; }
   vk::Queue PresentQueue() const { return m_presentQueue; }
   uint32_t GraphicsQueueFamily() const { return m_graphicsQueueFamily; }
   uint32_t PresentQueueFamily() const { return m_presentQueueFamily; }
   const vk::PhysicalDeviceProperties &Properties() const
      { return m_properties; }
   vk::Extent2D SwapchainExtent() const { return m_swapchainExtent; }
   uint32_t SwapchainImageCount() const
      { return static_cast<uint32_t>(m_swapchainImages.size()); }
   vk::RenderPass RenderPass() const { return *m_renderPass; }
   vk::Format SwapchainFormat() const { return m_swapchainFormat; }
   vk::Format DepthFormat() const { return m_depthFormat; }
   uint32_t FrameIndex() const { return m_frameIndex; }
   bool FrameActive() const { return m_frameActive; }

private:
   struct sPendingUpload
   {
      ~sPendingUpload()
      {
         if (mapped)
            try { device.unmapMemory(*stagingMemory); } catch (...) {}
      }
      vk::Device device{};
      void *mapped{};
      vk::DeviceSize capacity{};
      vk::DeviceSize allocationBytes{};
      bool coherent{};
      vk::raii::DeviceMemory stagingMemory{nullptr};
      vk::raii::Buffer stagingBuffer{nullptr};
      vk::raii::Fence fence{nullptr};
      std::vector<vk::raii::CommandBuffer> commandBuffers;
   };

   friend class cVulkanSceneRenderer;
   uint32_t FindMemoryType(uint32_t typeBits,
      vk::MemoryPropertyFlags properties) const;
   uint32_t FindHostVisibleMemoryType(uint32_t typeBits,
      bool *hostCoherent) const;
   bool CreateSwapchain();
   void DestroySwapchain();
   void CollectCompletedUploads();
   std::unique_ptr<sPendingUpload> AcquireUpload(vk::DeviceSize bytes);

   bool m_worldRenderingSupported{}, m_worldRenderingEnabled{};
   bool m_worldRayTracing{};
   std::string m_worldRenderingUnavailable;
   vk::raii::Context m_raiiContext;
   std::vector<std::unique_ptr<sPendingUpload>> m_pendingUploads;
   size_t m_pendingUploadHead{};
   std::vector<std::unique_ptr<sPendingUpload>> m_uploadPool;
   vk::DeviceSize m_cachedUploadBytes{};
   vk::raii::Instance m_instance{nullptr};
   vk::raii::SurfaceKHR m_surface{nullptr};
   vk::raii::PhysicalDevice m_physicalDevice{nullptr};
   vk::raii::Device m_device{nullptr};
   vk::Queue m_graphicsQueue{};
   vk::Queue m_presentQueue{};
   vk::raii::SwapchainKHR m_swapchain{nullptr};
   vk::Format m_swapchainFormat{};
   vk::Format m_depthFormat{};
   vk::Extent2D m_swapchainExtent{};
   vk::raii::RenderPass m_renderPass{nullptr};
   vk::raii::CommandPool m_commandPool{nullptr};
   std::vector<vk::Image> m_swapchainImages;
   std::vector<vk::raii::ImageView> m_imageViews;
   std::vector<vk::raii::Image> m_depthImages;
   std::vector<vk::raii::DeviceMemory> m_depthMemory;
   std::vector<vk::raii::ImageView> m_depthViews;
   std::vector<vk::raii::Framebuffer> m_framebuffers;
   std::vector<vk::raii::CommandBuffer> m_commandBuffers;
   std::vector<vk::raii::Semaphore> m_imageAvailable;
   std::vector<vk::raii::Semaphore> m_renderFinished;
   std::vector<vk::raii::Fence> m_inFlightFences;
   uint32_t m_frameIndex{};
   uint32_t m_acquiredImage{};
   bool m_frameActive{};
   bool m_recreateSwapchain{};
   bool m_surfaceLost{};
   bool m_vsync{true};
   uint32_t m_drawableWidth{}, m_drawableHeight{};
   vk::PhysicalDeviceProperties m_properties{};
   uint32_t m_graphicsQueueFamily{};
   uint32_t m_presentQueueFamily{};
};

#endif
