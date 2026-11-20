#ifndef DARK_VULKAN_CONTEXT_HPP
#define DARK_VULKAN_CONTEXT_HPP

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <functional>
#include <memory>
#include <vector>

class cVulkanSceneRenderer;

inline constexpr uint32_t kVulkanFramesInFlight = 2;

// Vulkan instance, platform surface, and a graphics/presentation capable
// device. Rendering resources are layered on this shared context.
class cVulkanContext
{
public:
   using tSurfaceFactory = std::function<vk::SurfaceKHR(vk::Instance)>;

   cVulkanContext();
   ~cVulkanContext();

   bool Initialize(const std::vector<const char *> & surfaceExtensions,
                   const tSurfaceFactory & createSurface,
                   uint32_t drawableWidth, uint32_t drawableHeight,
                   int physicalDeviceIndex, char * error,
                   unsigned errorCapacity);
   void Shutdown();
   // Applied when creating or recreating the swapchain.
   void SetVSync(bool enabled);
   bool PresentClearFrame(float red, float green, float blue, float alpha);
   // BeginFrame acquires a swapchain image and starts its render pass. The
   // returned command buffer is valid until EndFrame submits and presents it.
   bool BeginFrame(const std::array<float, 4> & clearColor,
                   vk::CommandBuffer * commandBuffer);
   bool EndFrame();
   // Copies RGBA8 storage (including packed legacy canvases) into a reusable
   // mapped staging allocation. A slot is reused only after its fence signals.
   bool SubmitUpload(
      const void * pixels, int width, int height, int rowBytes,
      const std::function<void(vk::CommandBuffer, vk::Buffer)> & record);
   bool Resize(uint32_t drawableWidth, uint32_t drawableHeight);
   bool RecreateSurface(const tSurfaceFactory & createSurface,
                        uint32_t drawableWidth, uint32_t drawableHeight);
   bool NeedsSwapchainRecreation() const;
   bool NeedsSurfaceRecreation() const;

   vk::Instance Instance() const;
   vk::SurfaceKHR Surface() const;
   vk::PhysicalDevice PhysicalDevice() const;
   vk::Device Device() const;
   const vk::raii::Device & RaiiDevice() const;
   vk::Queue GraphicsQueue() const;
   vk::Queue PresentQueue() const;
   uint32_t GraphicsQueueFamily() const;
   uint32_t PresentQueueFamily() const;
   const vk::PhysicalDeviceProperties & Properties() const;
   vk::Extent2D SwapchainExtent() const;
   uint32_t SwapchainImageCount() const;
   vk::RenderPass RenderPass() const;
   vk::Format SwapchainFormat() const;
   vk::Format DepthFormat() const;
   uint32_t FrameIndex() const;
   bool FrameActive() const;

private:
   struct sPendingUpload;

   friend class cVulkanSceneRenderer;
   uint32_t FindMemoryType(uint32_t typeBits,
                           vk::MemoryPropertyFlags properties) const;
   uint32_t FindHostVisibleMemoryType(uint32_t typeBits,
                                      bool * hostCoherent) const;
   bool CreateSwapchain();
   void DestroySwapchain();
   void CollectCompletedUploads();
   std::unique_ptr<sPendingUpload> AcquireUpload(vk::DeviceSize bytes);

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
   std::vector<vk::Fence> m_imageFences;
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
