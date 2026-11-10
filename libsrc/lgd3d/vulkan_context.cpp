#include "vulkan_context.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr vk::Format kDepthFormats[] = {vk::Format::eD32Sfloat,
   vk::Format::eD24UnormS8Uint, vk::Format::eD16Unorm};
constexpr char kPortabilitySubsetExtension[] =
   "VK_KHR_portability_subset";

void SetError(char *buffer, unsigned capacity, const char *message)
{
   if (buffer && capacity)
      std::snprintf(buffer, capacity, "%s", message ? message : "unknown error");
}

bool HasInstanceExtension(const std::vector<vk::ExtensionProperties> &extensions,
   const char *name)
{
   return std::any_of(extensions.begin(), extensions.end(),
      [name](const vk::ExtensionProperties &extension) {
         return std::strcmp(extension.extensionName, name) == 0;
      });
}

bool HasDeviceExtension(vk::PhysicalDevice device, const char *name)
{
   const auto extensions = device.enumerateDeviceExtensionProperties();
   return std::any_of(extensions.begin(), extensions.end(),
      [name](const vk::ExtensionProperties &extension) {
         return std::strcmp(extension.extensionName, name) == 0;
      });
}

bool SupportsColorAttachmentFormat(vk::PhysicalDevice device,
   vk::Format format)
{
   if (format == vk::Format::eUndefined)
      return false;
   const auto properties = device.getFormatProperties(format);
   return (properties.optimalTilingFeatures &
      vk::FormatFeatureFlagBits::eColorAttachment) !=
      vk::FormatFeatureFlags{};
}

bool SelectSurfaceFormat(vk::PhysicalDevice device,
   const std::vector<vk::SurfaceFormatKHR> &formats,
   vk::SurfaceFormatKHR *selected)
{
   if (formats.empty() || !selected)
      return false;
   if (formats.size() == 1 &&
       formats.front().format == vk::Format::eUndefined)
   {
      const vk::Format preferred[] = {vk::Format::eB8G8R8A8Unorm,
         vk::Format::eR8G8B8A8Unorm};
      for (vk::Format format : preferred)
         if (SupportsColorAttachmentFormat(device, format))
         {
            *selected = vk::SurfaceFormatKHR(format,
               formats.front().colorSpace);
            return true;
         }
      return false;
   }

   for (const auto &format : formats)
      if ((format.format == vk::Format::eB8G8R8A8Unorm ||
           format.format == vk::Format::eR8G8B8A8Unorm) &&
          format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear &&
          SupportsColorAttachmentFormat(device, format.format))
      {
         *selected = format;
         return true;
      }
   for (const auto &format : formats)
      if (SupportsColorAttachmentFormat(device, format.format))
      {
         *selected = format;
         return true;
      }
   return false;
}

void WriteLE16(std::ostream &stream, uint16_t value)
{
   const char bytes[] = {static_cast<char>(value),
      static_cast<char>(value >> 8)};
   stream.write(bytes, sizeof(bytes));
}

void WriteLE32(std::ostream &stream, uint32_t value)
{
   const char bytes[] = {static_cast<char>(value),
      static_cast<char>(value >> 8), static_cast<char>(value >> 16),
      static_cast<char>(value >> 24)};
   stream.write(bytes, sizeof(bytes));
}

bool SaveFrameBmp(const std::string &path, const uint8_t *pixels,
   uint32_t width, uint32_t height, vk::Format format)
{
   if (path.empty() || !pixels || !width || !height ||
       (format != vk::Format::eB8G8R8A8Unorm &&
        format != vk::Format::eB8G8R8A8Srgb &&
        format != vk::Format::eR8G8B8A8Unorm &&
        format != vk::Format::eR8G8B8A8Srgb))
      return false;
   const uint64_t dataSize = static_cast<uint64_t>(width) * height * 4;
   if (dataSize > std::numeric_limits<uint32_t>::max() - 54u)
      return false;

   std::ofstream file(path, std::ios::binary);
   if (!file)
      return false;
   file.put('B');
   file.put('M');
   WriteLE32(file, static_cast<uint32_t>(54 + dataSize));
   WriteLE16(file, 0);
   WriteLE16(file, 0);
   WriteLE32(file, 54);
   WriteLE32(file, 40);
   WriteLE32(file, width);
   WriteLE32(file, 0u - height); // top-down rows
   WriteLE16(file, 1);
   WriteLE16(file, 32);
   WriteLE32(file, 0); // BI_RGB
   WriteLE32(file, static_cast<uint32_t>(dataSize));
   WriteLE32(file, 2835);
   WriteLE32(file, 2835);
   WriteLE32(file, 0);
   WriteLE32(file, 0);

   const bool isBgra = format == vk::Format::eB8G8R8A8Unorm ||
      format == vk::Format::eB8G8R8A8Srgb;
   std::vector<uint8_t> row(static_cast<size_t>(width) * 4);
   for (uint32_t y = 0; y < height; ++y)
   {
      const uint8_t *source = pixels + static_cast<size_t>(y) * width * 4;
      if (isBgra)
         file.write(reinterpret_cast<const char *>(source), row.size());
      else
      {
         for (uint32_t x = 0; x < width; ++x)
         {
            row[x * 4] = source[x * 4 + 2];
            row[x * 4 + 1] = source[x * 4 + 1];
            row[x * 4 + 2] = source[x * 4];
            row[x * 4 + 3] = source[x * 4 + 3];
         }
         file.write(reinterpret_cast<const char *>(row.data()), row.size());
      }
   }
   return file.good();
}

std::vector<std::string> &EnumeratedPhysicalDeviceNames()
{
   static std::vector<std::string> names;
   return names;
}
}

extern "C" int RenderBackendVulkanEnumeratePhysicalDevices(void)
{
   std::vector<std::string> &names = EnumeratedPhysicalDeviceNames();
   names.clear();
   try
   {
      vk::raii::Context context;
      std::vector<const char *> extensions;
      const auto availableExtensions = vk::enumerateInstanceExtensionProperties();
      vk::InstanceCreateFlags instanceFlags{};
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
      if (HasInstanceExtension(availableExtensions,
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
      {
         extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
         instanceFlags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
      }
#endif
      vk::ApplicationInfo applicationInfo("Dark Engine", 1,
         "Dark Engine Vulkan Device Enumeration", 1, VK_API_VERSION_1_1);
      vk::InstanceCreateInfo instanceInfo(instanceFlags, &applicationInfo,
         0, nullptr, static_cast<uint32_t>(extensions.size()),
         extensions.data());
      vk::raii::Instance instance = context.createInstance(instanceInfo);
      for (vk::PhysicalDevice device : (*instance).enumeratePhysicalDevices())
      {
         const vk::PhysicalDeviceProperties properties = device.getProperties();
         names.emplace_back(properties.deviceName.data());
      }
      if (names.empty())
         std::fprintf(stderr,
            "Vulkan device enumeration found no physical devices\n");
      return static_cast<int>(names.size());
   }
   catch (const std::exception &exception)
   {
      names.clear();
      std::fprintf(stderr, "Vulkan device enumeration failed: %s\n",
         exception.what());
      return 0;
   }
   catch (...)
   {
      names.clear();
      std::fprintf(stderr,
         "Vulkan device enumeration failed with an unknown error\n");
      return 0;
   }
}

extern "C" const char *RenderBackendVulkanPhysicalDeviceName(int index)
{
   const std::vector<std::string> &names = EnumeratedPhysicalDeviceNames();
   return index >= 0 && static_cast<size_t>(index) < names.size()
      ? names[static_cast<size_t>(index)].c_str() : nullptr;
}

cVulkanContext::~cVulkanContext()
{
   Shutdown();
}

uint32_t cVulkanContext::FindMemoryType(uint32_t typeBits,
   vk::MemoryPropertyFlags properties) const
{
   const auto memory = m_physicalDevice.getMemoryProperties();
   for (uint32_t index = 0; index < memory.memoryTypeCount; ++index)
      if ((typeBits & (1u << index)) &&
          (memory.memoryTypes[index].propertyFlags & properties) == properties)
         return index;
   throw std::runtime_error("no compatible Vulkan memory type");
}

uint32_t cVulkanContext::FindHostVisibleMemoryType(uint32_t typeBits,
   bool *hostCoherent) const
{
   const auto memory = m_physicalDevice.getMemoryProperties();
   const vk::MemoryPropertyFlags hostVisible =
      vk::MemoryPropertyFlagBits::eHostVisible;
   const vk::MemoryPropertyFlags coherent =
      vk::MemoryPropertyFlagBits::eHostCoherent;
   for (bool requireCoherent : {true, false})
      for (uint32_t index = 0; index < memory.memoryTypeCount; ++index)
      {
         const auto flags = memory.memoryTypes[index].propertyFlags;
         if ((typeBits & (1u << index)) && (flags & hostVisible) &&
             (!requireCoherent || (flags & coherent)))
         {
            if (hostCoherent)
               *hostCoherent = (flags & coherent) != vk::MemoryPropertyFlags{};
            return index;
         }
      }
   throw std::runtime_error("no host-visible Vulkan memory type");
}

bool cVulkanContext::Initialize(
   const std::vector<const char *> &surfaceExtensions,
   const tSurfaceFactory &createSurface, uint32_t drawableWidth,
   uint32_t drawableHeight, int physicalDeviceIndex, char *error,
   unsigned errorCapacity)
{
   Shutdown();
   if (!createSurface || !drawableWidth || !drawableHeight)
   {
      SetError(error, errorCapacity, "invalid Vulkan surface provider or extent");
      return false;
   }

   try
   {
      std::vector<const char *> extensions = surfaceExtensions;

      const auto availableExtensions = vk::enumerateInstanceExtensionProperties();
      vk::InstanceCreateFlags instanceFlags{};
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
      if (HasInstanceExtension(availableExtensions,
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
      {
         extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
         instanceFlags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
      }
#endif
      vk::ApplicationInfo applicationInfo("Dark Engine", 1,
         "Dark Engine Vulkan Renderer", 1, VK_API_VERSION_1_1);
      vk::InstanceCreateInfo instanceInfo(instanceFlags, &applicationInfo,
         0, nullptr, static_cast<uint32_t>(extensions.size()),
         extensions.data());
      m_instance = m_raiiContext.createInstance(instanceInfo);

      const vk::SurfaceKHR surface = createSurface(*m_instance);
      if (!surface)
      {
         SetError(error, errorCapacity, "platform Vulkan surface creation failed");
         Shutdown();
         return false;
      }
      m_surface = vk::raii::SurfaceKHR(m_instance,
         static_cast<VkSurfaceKHR>(surface));

      struct sCandidate
      {
         vk::PhysicalDevice device;
         uint32_t graphicsFamily;
         uint32_t presentFamily;
         vk::PhysicalDeviceProperties properties;
         int score;
         uint32_t enumerationIndex;
      };
      std::vector<sCandidate> candidates;
      uint32_t enumerationIndex = 0;
      for (vk::PhysicalDevice device : (*m_instance).enumeratePhysicalDevices())
      {
         const uint32_t deviceIndex = enumerationIndex++;
         if (!HasDeviceExtension(device, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
            continue;
         const auto queueFamilies = device.getQueueFamilyProperties();
         uint32_t graphics = UINT32_MAX;
         uint32_t present = UINT32_MAX;
         for (uint32_t i = 0; i < queueFamilies.size(); ++i)
         {
            if (queueFamilies[i].queueCount == 0)
               continue;
            const bool supportsGraphics = (queueFamilies[i].queueFlags &
               vk::QueueFlagBits::eGraphics) != vk::QueueFlags{};
            const bool supportsPresent =
               device.getSurfaceSupportKHR(i, *m_surface);
            if (supportsGraphics && supportsPresent)
            {
               graphics = present = i;
               break;
            }
            if (supportsGraphics && graphics == UINT32_MAX)
               graphics = i;
            if (supportsPresent && present == UINT32_MAX)
               present = i;
         }
         if (graphics == UINT32_MAX || present == UINT32_MAX)
            continue;
         const auto surfaceCapabilities =
            device.getSurfaceCapabilitiesKHR(*m_surface);
         const auto surfaceFormats = device.getSurfaceFormatsKHR(*m_surface);
         vk::SurfaceFormatKHR surfaceFormat{};
         if (!(surfaceCapabilities.supportedUsageFlags &
               vk::ImageUsageFlagBits::eColorAttachment) ||
             !SelectSurfaceFormat(device, surfaceFormats, &surfaceFormat))
            continue;
         bool supportsDepthAttachment = false;
         for (vk::Format format : kDepthFormats)
         {
            const auto formatProperties = device.getFormatProperties(format);
            if (formatProperties.optimalTilingFeatures &
                vk::FormatFeatureFlagBits::eDepthStencilAttachment)
            {
               supportsDepthAttachment = true;
               break;
            }
         }
         if (!supportsDepthAttachment)
            continue;
         vk::PhysicalDeviceProperties properties = device.getProperties();
         int score = properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu
            ? 100 : properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu
            ? 50 : 0;
         candidates.push_back({device, graphics, present, properties, score,
            deviceIndex});
      }
      if (candidates.empty())
      {
         SetError(error, errorCapacity,
            "no Vulkan device supports graphics, presentation, and VK_KHR_swapchain");
         Shutdown();
         return false;
      }
      const sCandidate *chosen = nullptr;
      if (physicalDeviceIndex >= 0)
      {
         const auto selected = std::find_if(candidates.begin(), candidates.end(),
            [physicalDeviceIndex](const sCandidate &candidate) {
               return candidate.enumerationIndex ==
                  static_cast<uint32_t>(physicalDeviceIndex);
            });
         if (selected != candidates.end())
            chosen = &*selected;
      }
      if (!chosen)
         chosen = &*std::max_element(candidates.begin(),
         candidates.end(), [](const sCandidate &a, const sCandidate &b) {
            return a.score < b.score;
         });
      m_physicalDevice = vk::raii::PhysicalDevice(m_instance, static_cast<VkPhysicalDevice>(chosen->device));
      m_graphicsQueueFamily = chosen->graphicsFamily;
      m_presentQueueFamily = chosen->presentFamily;
      m_properties = chosen->properties;

      const float priority = 1.0f;
      std::set<uint32_t> uniqueFamilies{m_graphicsQueueFamily,
         m_presentQueueFamily};
      std::vector<vk::DeviceQueueCreateInfo> queueInfos;
      for (uint32_t family : uniqueFamilies)
         queueInfos.emplace_back(vk::DeviceQueueCreateFlags{}, family, 1,
            &priority);
      std::vector<const char *> deviceExtensions{
         VK_KHR_SWAPCHAIN_EXTENSION_NAME};
      if (HasDeviceExtension(m_physicalDevice,
            kPortabilitySubsetExtension))
         deviceExtensions.push_back(kPortabilitySubsetExtension);
      vk::PhysicalDeviceFeatures features{};
      vk::DeviceCreateInfo deviceInfo({},
         static_cast<uint32_t>(queueInfos.size()), queueInfos.data(),
         0, nullptr, static_cast<uint32_t>(deviceExtensions.size()),
         deviceExtensions.data(), &features);
      m_device = m_physicalDevice.createDevice(deviceInfo);
      m_graphicsQueue = (*m_device).getQueue(m_graphicsQueueFamily, 0);
      m_presentQueue = (*m_device).getQueue(m_presentQueueFamily, 0);
      m_drawableWidth = drawableWidth;
      m_drawableHeight = drawableHeight;
      if (!CreateSwapchain())
      {
         SetError(error, errorCapacity, "failed to create Vulkan swapchain");
         Shutdown();
         return false;
      }
      if (error && errorCapacity)
         error[0] = '\0';
      return true;
   }
   catch (const std::exception &exception)
   {
      SetError(error, errorCapacity, exception.what());
      Shutdown();
      return false;
   }
}

void cVulkanContext::Shutdown()
{
   m_frameActive = false;
   m_recreateSwapchain = false;
   m_surfaceLost = false;
   if (*m_device)
   {
      try { (void)(*m_device).waitIdle(); } catch (...) {}
      m_pendingUploads.clear();
      DestroySwapchain();
      m_inFlightFences.clear();
      m_imageAvailable.clear();
      m_renderFinished.clear();
      if (*m_commandPool)
      {
         m_commandPool.clear();
      }
      m_device.clear();
   }
   m_graphicsQueue = nullptr;
   m_presentQueue = nullptr;
   m_physicalDevice.clear();
   m_surface.clear();
   if (*m_instance)
   {
      m_instance.clear();
   }
   m_properties = vk::PhysicalDeviceProperties{};
   m_drawableWidth = m_drawableHeight = 0;
   m_frameIndex = 0;
   m_graphicsQueueFamily = 0;
   m_presentQueueFamily = 0;
   m_captureSupported = false;
}

bool cVulkanContext::CreateSwapchain()
{
   const auto capabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(*m_surface);
   const auto formats = m_physicalDevice.getSurfaceFormatsKHR(*m_surface);
   vk::SurfaceFormatKHR surfaceFormat{};
   if (!SelectSurfaceFormat(*m_physicalDevice, formats, &surfaceFormat))
      return false;

   if (!m_drawableWidth || !m_drawableHeight)
      return false;
   if (capabilities.currentExtent.width != UINT32_MAX)
      m_swapchainExtent = capabilities.currentExtent;
   else
   {
      m_swapchainExtent.width = std::clamp(
         m_drawableWidth,
         capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
      m_swapchainExtent.height = std::clamp(
         m_drawableHeight,
         capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
   }

   uint32_t imageCount = capabilities.minImageCount + 1;
   if (capabilities.maxImageCount && imageCount > capabilities.maxImageCount)
      imageCount = capabilities.maxImageCount;
   const uint32_t queueFamilies[] = {m_graphicsQueueFamily,
      m_presentQueueFamily};
   vk::SwapchainCreateInfoKHR swapchainInfo{};
   swapchainInfo.surface = *m_surface;
   swapchainInfo.minImageCount = imageCount;
   swapchainInfo.imageFormat = surfaceFormat.format;
   swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
   swapchainInfo.imageExtent = m_swapchainExtent;
   swapchainInfo.imageArrayLayers = 1;
   swapchainInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
   const bool transferSourceSupported = (capabilities.supportedUsageFlags &
      vk::ImageUsageFlagBits::eTransferSrc) != vk::ImageUsageFlags{};
   if (transferSourceSupported)
      swapchainInfo.imageUsage |= vk::ImageUsageFlagBits::eTransferSrc;
   if (m_graphicsQueueFamily != m_presentQueueFamily)
   {
      swapchainInfo.imageSharingMode = vk::SharingMode::eConcurrent;
      swapchainInfo.queueFamilyIndexCount = 2;
      swapchainInfo.pQueueFamilyIndices = queueFamilies;
   }
   else
      swapchainInfo.imageSharingMode = vk::SharingMode::eExclusive;
   swapchainInfo.preTransform = capabilities.currentTransform;
   swapchainInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
   if (!(capabilities.supportedCompositeAlpha & swapchainInfo.compositeAlpha))
   {
      const vk::CompositeAlphaFlagBitsKHR modes[] = {
         vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
         vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
         vk::CompositeAlphaFlagBitsKHR::eInherit};
      bool found = false;
      for (auto mode : modes)
         if (capabilities.supportedCompositeAlpha & mode)
         {
            swapchainInfo.compositeAlpha = mode;
            found = true;
            break;
         }
      if (!found)
         return false;
   }
   swapchainInfo.presentMode = vk::PresentModeKHR::eFifo;
   swapchainInfo.clipped = VK_TRUE;
   m_swapchainFormat = surfaceFormat.format;
   const bool supportedCaptureFormat =
      m_swapchainFormat == vk::Format::eB8G8R8A8Unorm ||
      m_swapchainFormat == vk::Format::eB8G8R8A8Srgb ||
      m_swapchainFormat == vk::Format::eR8G8B8A8Unorm ||
      m_swapchainFormat == vk::Format::eR8G8B8A8Srgb;
   m_captureSupported = transferSourceSupported && supportedCaptureFormat;
   m_swapchain = m_device.createSwapchainKHR(swapchainInfo);
   m_swapchainImages = (*m_device).getSwapchainImagesKHR(*m_swapchain);
   if (m_swapchainImages.empty())
      return false;

   for (vk::Image image : m_swapchainImages)
   {
      vk::ImageViewCreateInfo viewInfo{};
      viewInfo.image = image;
      viewInfo.viewType = vk::ImageViewType::e2D;
      viewInfo.format = m_swapchainFormat;
      viewInfo.components = vk::ComponentMapping{};
      viewInfo.subresourceRange = vk::ImageSubresourceRange(
         vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
      m_imageViews.push_back(m_device.createImageView(viewInfo));
   }

   m_depthFormat = vk::Format::eUndefined;
   for (vk::Format format : kDepthFormats)
   {
      const auto properties = m_physicalDevice.getFormatProperties(format);
      if (properties.optimalTilingFeatures &
          vk::FormatFeatureFlagBits::eDepthStencilAttachment)
      {
         m_depthFormat = format;
         break;
      }
   }
   if (m_depthFormat == vk::Format::eUndefined)
      return false;
   for (size_t i = 0; i < m_swapchainImages.size(); ++i)
   {
      vk::ImageCreateInfo imageInfo{};
      imageInfo.imageType = vk::ImageType::e2D;
      imageInfo.format = m_depthFormat;
      imageInfo.extent = vk::Extent3D(m_swapchainExtent.width,
         m_swapchainExtent.height, 1);
      imageInfo.mipLevels = 1;
      imageInfo.arrayLayers = 1;
      imageInfo.samples = vk::SampleCountFlagBits::e1;
      imageInfo.tiling = vk::ImageTiling::eOptimal;
      imageInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
      imageInfo.sharingMode = vk::SharingMode::eExclusive;
      imageInfo.initialLayout = vk::ImageLayout::eUndefined;
      vk::raii::DeviceMemory memory{nullptr};
      vk::raii::Image image = m_device.createImage(imageInfo);
      const auto requirements = (*m_device).getImageMemoryRequirements(*image);
      vk::MemoryAllocateInfo allocation(requirements.size,
         FindMemoryType(requirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal));
      memory = m_device.allocateMemory(allocation);
      (*m_device).bindImageMemory(*image, *memory, 0);
      vk::ImageViewCreateInfo viewInfo{};
      viewInfo.image = *image;
      viewInfo.viewType = vk::ImageViewType::e2D;
      viewInfo.format = m_depthFormat;
      vk::ImageAspectFlags aspects = vk::ImageAspectFlagBits::eDepth;
      if (m_depthFormat == vk::Format::eD24UnormS8Uint)
         aspects |= vk::ImageAspectFlagBits::eStencil;
      viewInfo.subresourceRange = vk::ImageSubresourceRange(aspects, 0, 1, 0, 1);
      m_depthImages.push_back(std::move(image));
      m_depthMemory.push_back(std::move(memory));
      m_depthViews.push_back(m_device.createImageView(viewInfo));
   }

   vk::AttachmentDescription colorAttachment{};
   colorAttachment.format = m_swapchainFormat;
   colorAttachment.samples = vk::SampleCountFlagBits::e1;
   colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
   colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
   colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
   colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
   colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
   colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
   vk::AttachmentReference colorReference(0,
      vk::ImageLayout::eColorAttachmentOptimal);
   vk::AttachmentDescription depthAttachment{};
   depthAttachment.format = m_depthFormat;
   depthAttachment.samples = vk::SampleCountFlagBits::e1;
   depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
   depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
   depthAttachment.stencilLoadOp =
      m_depthFormat == vk::Format::eD24UnormS8Uint
         ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare;
   depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
   depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
   depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
   vk::AttachmentReference depthReference(1,
      vk::ImageLayout::eDepthStencilAttachmentOptimal);
   vk::SubpassDescription subpass{};
   subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
   subpass.colorAttachmentCount = 1;
   subpass.pColorAttachments = &colorReference;
   subpass.pDepthStencilAttachment = &depthReference;
   vk::SubpassDependency dependency{};
   dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
   dependency.dstSubpass = 0;
   dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
   dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
   dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
   dependency.dstStageMask |= vk::PipelineStageFlagBits::eEarlyFragmentTests;
   dependency.dstAccessMask |= vk::AccessFlagBits::eDepthStencilAttachmentWrite;
   vk::RenderPassCreateInfo renderPassInfo{};
   const vk::AttachmentDescription attachments[] = {colorAttachment,
      depthAttachment};
   renderPassInfo.attachmentCount = 2;
   renderPassInfo.pAttachments = attachments;
   renderPassInfo.subpassCount = 1;
   renderPassInfo.pSubpasses = &subpass;
   renderPassInfo.dependencyCount = 1;
   renderPassInfo.pDependencies = &dependency;
   m_renderPass = m_device.createRenderPass(renderPassInfo);

   for (size_t i = 0; i < m_imageViews.size(); ++i)
   {
      const vk::ImageView attachments[] = {*m_imageViews[i], *m_depthViews[i]};
      vk::FramebufferCreateInfo framebufferInfo{};
      framebufferInfo.renderPass = *m_renderPass;
      framebufferInfo.attachmentCount = 2;
      framebufferInfo.pAttachments = attachments;
      framebufferInfo.width = m_swapchainExtent.width;
      framebufferInfo.height = m_swapchainExtent.height;
      framebufferInfo.layers = 1;
      m_framebuffers.push_back(m_device.createFramebuffer(framebufferInfo));
   }

   if (!*m_commandPool)
   {
      vk::CommandPoolCreateInfo poolInfo(
         vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
         m_graphicsQueueFamily);
      m_commandPool = m_device.createCommandPool(poolInfo);
   }
   vk::CommandBufferAllocateInfo commandInfo(*m_commandPool,
      vk::CommandBufferLevel::ePrimary, kVulkanFramesInFlight);
   m_commandBuffers.clear();
   auto commandBuffers = m_device.allocateCommandBuffers(commandInfo);
   for (auto &commandBuffer : commandBuffers)
      m_commandBuffers.push_back(std::move(commandBuffer));
   if (m_imageAvailable.empty())
   {
      for (uint32_t i = 0; i < kVulkanFramesInFlight; ++i)
      {
         m_imageAvailable.push_back(m_device.createSemaphore({}));
         m_inFlightFences.push_back(m_device.createFence(
            vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)));
      }
   }
   // A graphics fence only signals completion of the rendering submission;
   // it does not guarantee that the presentation queue has consumed its wait
   // semaphore. Associate each semaphore with a swapchain image so acquiring
   // that image again guarantees the previous presentation wait has finished.
   m_renderFinished.clear();
   for (size_t i = 0; i < m_swapchainImages.size(); ++i)
      m_renderFinished.push_back(m_device.createSemaphore({}));
   m_imageFences.assign(m_swapchainImages.size(), vk::Fence{});
   m_frameIndex = 0;
   return true;
}

void cVulkanContext::DestroySwapchain()
{
   if (!*m_device)
      return;
   m_commandBuffers.clear();
   m_framebuffers.clear();
   m_renderPass.clear();
   m_depthViews.clear();
   m_depthImages.clear();
   m_depthMemory.clear();
   m_imageViews.clear();
   m_swapchain.clear();
   m_swapchainImages.clear();
   m_imageFences.clear();
   m_swapchainExtent = vk::Extent2D{};
}

bool cVulkanContext::Resize(uint32_t drawableWidth, uint32_t drawableHeight)
{
   if (!*m_device || !drawableWidth || !drawableHeight)
      return false;
   try
   {
      m_frameActive = false;
      (void)(*m_device).waitIdle();
      m_pendingUploads.clear();
      // A failed frame can leave an acquire semaphore signaled or a fence
      // unsignaled. Recreate synchronization objects together with the
      // swapchain so the next frame starts from a known state.
      m_inFlightFences.clear();
      m_imageAvailable.clear();
      m_renderFinished.clear();
      DestroySwapchain();
      m_drawableWidth = drawableWidth;
      m_drawableHeight = drawableHeight;
      const bool created = CreateSwapchain();
      m_recreateSwapchain = !created;
      return created;
   }
   catch (const vk::SurfaceLostKHRError &)
   {
      m_surfaceLost = true;
      m_recreateSwapchain = true;
      return false;
   }
   catch (const std::exception &)
   {
      m_recreateSwapchain = true;
      return false;
   }
}

void cVulkanContext::ConfigureFrameCapture(const char *path,
   uint32_t targetFrame)
{
   m_capturePath = path ? path : "";
   m_captureTargetFrame = std::max(targetFrame, 1u);
   m_captureFrameCount = 0;
   m_captureComplete = false;
   if (!m_capturePath.empty() && !m_captureSupported)
      std::fprintf(stderr,
         "Vulkan frame capture is unavailable: swapchain transfer-source usage is unsupported\n");
}

bool cVulkanContext::BeginFrame(const std::array<float, 4> &clearColor,
   vk::CommandBuffer *commandBuffer)
{
   if (!*m_device || m_frameActive || !commandBuffer)
      return false;
   CollectCompletedUploads();
   if (m_commandBuffers.empty())
      return false;
   try
   {
      const vk::Fence frameFence = *m_inFlightFences[m_frameIndex];
      (void)(*m_device).waitForFences(frameFence, VK_TRUE, UINT64_MAX);
      const auto acquired = (*m_device).acquireNextImageKHR(*m_swapchain,
         UINT64_MAX, *m_imageAvailable[m_frameIndex], nullptr);
      if (acquired.result == vk::Result::eSuboptimalKHR)
         m_recreateSwapchain = true;
      m_acquiredImage = acquired.value;
      if (m_imageFences[m_acquiredImage])
         (void)(*m_device).waitForFences(m_imageFences[m_acquiredImage], VK_TRUE,
            UINT64_MAX);
      m_imageFences[m_acquiredImage] = frameFence;

      vk::CommandBuffer active = *m_commandBuffers[m_frameIndex];
      (void)active.reset();
      active.begin(vk::CommandBufferBeginInfo{});
      std::array<vk::ClearValue, 2> clearValues{};
      clearValues[0].color = vk::ClearColorValue(clearColor);
      clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
      vk::RenderPassBeginInfo passInfo{};
      passInfo.renderPass = *m_renderPass;
      passInfo.framebuffer = *m_framebuffers[m_acquiredImage];
      passInfo.renderArea = vk::Rect2D({0, 0}, m_swapchainExtent);
      passInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
      passInfo.pClearValues = clearValues.data();
      active.beginRenderPass(passInfo, vk::SubpassContents::eInline);
      m_frameActive = true;
      *commandBuffer = active;
      return true;
   }
   catch (const vk::OutOfDateKHRError &)
   {
      m_recreateSwapchain = true;
      return false;
   }
   catch (const vk::SurfaceLostKHRError &)
   {
      m_surfaceLost = true;
      m_recreateSwapchain = true;
      return false;
   }
   catch (const std::exception &)
   {
      m_recreateSwapchain = true;
      return false;
   }
}

bool cVulkanContext::RecreateSurface(const tSurfaceFactory &createSurface,
   uint32_t drawableWidth, uint32_t drawableHeight)
{
   if (!*m_instance || !*m_device || !createSurface ||
       !drawableWidth || !drawableHeight)
      return false;
   try
   {
      const vk::SurfaceKHR surface = createSurface(*m_instance);
      if (!surface)
         return false;
      vk::raii::SurfaceKHR replacement(m_instance,
         static_cast<VkSurfaceKHR>(surface));
      if (!m_physicalDevice.getSurfaceSupportKHR(m_presentQueueFamily,
            *replacement))
         return false;

      m_frameActive = false;
      (void)(*m_device).waitIdle();
      m_pendingUploads.clear();
      m_inFlightFences.clear();
      m_imageAvailable.clear();
      m_renderFinished.clear();
      DestroySwapchain();
      m_surface = std::move(replacement);
      m_drawableWidth = drawableWidth;
      m_drawableHeight = drawableHeight;
      const bool created = CreateSwapchain();
      m_surfaceLost = false;
      m_recreateSwapchain = !created;
      return created;
   }
   catch (const vk::SurfaceLostKHRError &)
   {
      m_surfaceLost = true;
      m_recreateSwapchain = true;
      return false;
   }
   catch (const std::exception &)
   {
      m_recreateSwapchain = true;
      return false;
   }
}

bool cVulkanContext::EndFrame()
{
   if (!*m_device || !m_frameActive || m_commandBuffers.empty())
      return false;
   bool captureRecorded = false;
   bool frameSubmitted = false;
   bool captureMemoryCoherent = false;
   vk::DeviceSize captureMemorySize = 0;
   vk::raii::DeviceMemory captureMemory{nullptr};
   vk::raii::Buffer captureBuffer{nullptr};
   try
   {
      vk::CommandBuffer commandBuffer = *m_commandBuffers[m_frameIndex];
      commandBuffer.endRenderPass();

      if (!m_capturePath.empty() && !m_captureComplete &&
          ++m_captureFrameCount >= m_captureTargetFrame &&
          m_captureSupported)
      {
         try
         {
            const vk::DeviceSize byteCount =
               static_cast<vk::DeviceSize>(m_swapchainExtent.width) *
               m_swapchainExtent.height * 4;
            captureBuffer = m_device.createBuffer(vk::BufferCreateInfo({},
               byteCount, vk::BufferUsageFlagBits::eTransferDst,
               vk::SharingMode::eExclusive));
            const auto requirements =
               (*m_device).getBufferMemoryRequirements(*captureBuffer);
            const uint32_t memoryType = FindHostVisibleMemoryType(
               requirements.memoryTypeBits,
               &captureMemoryCoherent);
            captureMemorySize = requirements.size;
            captureMemory = m_device.allocateMemory(vk::MemoryAllocateInfo(
               requirements.size, memoryType));
            (*m_device).bindBufferMemory(*captureBuffer, *captureMemory, 0);

            vk::ImageMemoryBarrier toTransfer{};
            toTransfer.oldLayout = vk::ImageLayout::ePresentSrcKHR;
            toTransfer.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toTransfer.image = m_swapchainImages[m_acquiredImage];
            toTransfer.subresourceRange = vk::ImageSubresourceRange(
               vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
            toTransfer.srcAccessMask =
               vk::AccessFlagBits::eColorAttachmentWrite;
            toTransfer.dstAccessMask = vk::AccessFlagBits::eTransferRead;
            commandBuffer.pipelineBarrier(
               vk::PipelineStageFlagBits::eColorAttachmentOutput,
               vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, toTransfer);
            vk::BufferImageCopy region{};
            region.imageSubresource = vk::ImageSubresourceLayers(
               vk::ImageAspectFlagBits::eColor, 0, 0, 1);
            region.imageExtent = vk::Extent3D(m_swapchainExtent.width,
               m_swapchainExtent.height, 1);
            commandBuffer.copyImageToBuffer(
               m_swapchainImages[m_acquiredImage],
               vk::ImageLayout::eTransferSrcOptimal, *captureBuffer, region);
            vk::ImageMemoryBarrier toPresent{};
            toPresent.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            toPresent.newLayout = vk::ImageLayout::ePresentSrcKHR;
            toPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toPresent.image = m_swapchainImages[m_acquiredImage];
            toPresent.subresourceRange = toTransfer.subresourceRange;
            toPresent.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
               vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {}, toPresent);
            captureRecorded = true;
         }
         catch (const std::exception &)
         {
            captureBuffer.clear();
            captureMemory.clear();
         }
      }
      commandBuffer.end();

      const vk::Fence frameFence = *m_inFlightFences[m_frameIndex];
      (void)(*m_device).resetFences(frameFence);
      const vk::Semaphore waitSemaphore = *m_imageAvailable[m_frameIndex];
      const vk::Semaphore signalSemaphore = *m_renderFinished[m_acquiredImage];
      const vk::PipelineStageFlags waitStage =
         vk::PipelineStageFlagBits::eColorAttachmentOutput;
      const vk::CommandBuffer submitCommand = commandBuffer;
      vk::SubmitInfo submitInfo{};
      submitInfo.waitSemaphoreCount = 1;
      submitInfo.pWaitSemaphores = &waitSemaphore;
      submitInfo.pWaitDstStageMask = &waitStage;
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &submitCommand;
      submitInfo.signalSemaphoreCount = 1;
      submitInfo.pSignalSemaphores = &signalSemaphore;
      m_graphicsQueue.submit(submitInfo, frameFence);
      frameSubmitted = true;

      if (captureRecorded)
      {
         (void)(*m_device).waitForFences(frameFence, VK_TRUE, UINT64_MAX);
         const vk::DeviceSize byteCount =
            static_cast<vk::DeviceSize>(m_swapchainExtent.width) *
            m_swapchainExtent.height * 4;
         void *mapped = (*m_device).mapMemory(*captureMemory, 0,
            captureMemorySize);
         if (!captureMemoryCoherent)
            (*m_device).invalidateMappedMemoryRanges(
               vk::MappedMemoryRange(*captureMemory, 0, VK_WHOLE_SIZE));
         m_captureComplete = SaveFrameBmp(m_capturePath,
            static_cast<const uint8_t *>(mapped), m_swapchainExtent.width,
            m_swapchainExtent.height, m_swapchainFormat);
         (*m_device).unmapMemory(*captureMemory);
      }
      captureBuffer.clear();
      captureMemory.clear();

      vk::PresentInfoKHR presentInfo{};
      presentInfo.waitSemaphoreCount = 1;
      presentInfo.pWaitSemaphores = &signalSemaphore;
      presentInfo.swapchainCount = 1;
      const vk::SwapchainKHR swapchain = *m_swapchain;
      presentInfo.pSwapchains = &swapchain;
      presentInfo.pImageIndices = &m_acquiredImage;
      const vk::Result result = m_presentQueue.presentKHR(presentInfo);
      if (result == vk::Result::eSuboptimalKHR ||
          result == vk::Result::eErrorOutOfDateKHR)
         m_recreateSwapchain = true;
      if (result == vk::Result::eErrorSurfaceLostKHR)
      {
         m_surfaceLost = true;
         m_recreateSwapchain = true;
      }
      m_frameIndex = (m_frameIndex + 1) %
         static_cast<uint32_t>(m_commandBuffers.size());
      m_frameActive = false;
      return result == vk::Result::eSuccess ||
         result == vk::Result::eSuboptimalKHR;
   }
   catch (const vk::OutOfDateKHRError &)
   {
      if (frameSubmitted)
         try { (void)(*m_device).waitIdle(); } catch (...) {}
      m_frameActive = false;
      m_recreateSwapchain = true;
      return false;
   }
   catch (const vk::SurfaceLostKHRError &)
   {
      if (frameSubmitted)
         try { (void)(*m_device).waitIdle(); } catch (...) {}
      m_frameActive = false;
      m_surfaceLost = true;
      m_recreateSwapchain = true;
      return false;
   }
   catch (const std::exception &)
   {
      if (frameSubmitted)
         try { (void)(*m_device).waitIdle(); } catch (...) {}
      m_frameActive = false;
      m_recreateSwapchain = true;
      return false;
   }
}

bool cVulkanContext::SubmitUpload(
   const std::function<void(vk::CommandBuffer)> &record,
   vk::raii::Buffer &&stagingBuffer,
   vk::raii::DeviceMemory &&stagingMemory)
{
   if (!*m_device || !*m_commandPool || !record)
      return false;
   try
   {
      m_pendingUploads.reserve(m_pendingUploads.size() + 1);
      auto upload = std::make_unique<sPendingUpload>();
      vk::CommandBufferAllocateInfo allocation(*m_commandPool,
         vk::CommandBufferLevel::ePrimary, 1);
      upload->commandBuffers = m_device.allocateCommandBuffers(allocation);
      vk::CommandBuffer commandBuffer = *upload->commandBuffers.front();
      commandBuffer.begin(vk::CommandBufferBeginInfo(
         vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
      record(commandBuffer);
      commandBuffer.end();
      upload->fence = m_device.createFence({});
      upload->stagingBuffer = std::move(stagingBuffer);
      upload->stagingMemory = std::move(stagingMemory);

      // Uploads and frame draws use the same queue. Later frame submissions
      // are ordered after this transfer, so texture images are ready before
      // their first draw without stalling the host on queue.waitIdle().
      const vk::CommandBuffer submittedCommandBuffer = commandBuffer;
      vk::SubmitInfo submit{};
      submit.commandBufferCount = 1;
      submit.pCommandBuffers = &submittedCommandBuffer;
      m_graphicsQueue.submit(submit, *upload->fence);
      m_pendingUploads.push_back(std::move(upload));
      return true;
   }
   catch (const std::exception &)
   {
      return false;
   }
}

void cVulkanContext::CollectCompletedUploads()
{
   for (auto upload = m_pendingUploads.begin();
        upload != m_pendingUploads.end();)
   {
      try
      {
         if ((*m_device).getFenceStatus(*(*upload)->fence) == vk::Result::eSuccess)
         {
            upload = m_pendingUploads.erase(upload);
            continue;
         }
      }
      catch (const std::exception &)
      {
      }
      ++upload;
   }
}

bool cVulkanContext::PresentClearFrame(float red, float green, float blue,
   float alpha)
{
   vk::CommandBuffer commandBuffer{};
   return BeginFrame({red, green, blue, alpha}, &commandBuffer) && EndFrame();
}
