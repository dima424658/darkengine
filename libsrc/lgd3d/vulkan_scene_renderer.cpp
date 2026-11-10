#include "vulkan_scene_renderer.hpp"

#if defined(__ANDROID__)
#include <SDL3/SDL_iostream.h>
#endif

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <utility>

namespace
{
constexpr vk::DeviceSize kVertexBufferBytes = 8 * 1024 * 1024;
constexpr uint32_t kDescriptorSetsPerFrame = 4096;

std::vector<uint32_t> ReadShader(const char *path)
{
   if (!path)
      return {};
#if defined(__ANDROID__)
   size_t byteCount = 0;
   void *bytes = SDL_LoadFile(path, &byteCount);
   if (!bytes || byteCount == 0 || byteCount % sizeof(uint32_t) != 0)
   {
      SDL_free(bytes);
      return {};
   }
   std::vector<uint32_t> words(byteCount / sizeof(uint32_t));
   std::memcpy(words.data(), bytes, byteCount);
   SDL_free(bytes);
   return words;
#else
   std::ifstream file(path, std::ios::binary);
   if (!file)
      return {};
   std::vector<char> bytes((std::istreambuf_iterator<char>(file)), {});
   if (bytes.empty() || bytes.size() % sizeof(uint32_t) != 0)
      return {};
   std::vector<uint32_t> words(bytes.size() / sizeof(uint32_t));
   std::memcpy(words.data(), bytes.data(), bytes.size());
   return words;
#endif
}

struct sPushConstants
{
   float viewportSize[2];
   float gamma;
   uint32_t useTexture1;
   uint32_t fogEnabled;
   uint32_t alphaTest;
   uint32_t transparentBlack;
   uint32_t padding;
   float fogColor[4];
};
static_assert(sizeof(sPushConstants) == 48, "SPIR-V push constant layout mismatch");

bool IsValidRgbaImage(int width, int height, int rowBytes)
{
   if (width <= 0 || height <= 0 || rowBytes <= 0)
      return false;
   const size_t pixelBytes = static_cast<size_t>(width) * 4;
   const size_t sourceStride = static_cast<size_t>(rowBytes);
   if (pixelBytes / 4 != static_cast<size_t>(width) ||
       sourceStride < pixelBytes ||
       static_cast<size_t>(height) >
          std::numeric_limits<size_t>::max() / pixelBytes ||
       (static_cast<size_t>(height) > 1 &&
        static_cast<size_t>(height - 1) >
          (std::numeric_limits<size_t>::max() - pixelBytes) / sourceStride))
      return false;
   return true;
}

bool MapLogicalRect(const vk::Extent2D &extent, int viewportX, int viewportY,
   int viewportWidth, int viewportHeight, int x, int y, int width, int height,
   int logicalWidth, int logicalHeight, vk::Rect2D *rect)
{
   if (!rect || width <= 0 || height <= 0 || logicalWidth <= 0 ||
       logicalHeight <= 0 || extent.width == 0 || extent.height == 0)
      return false;

   const int64_t x1 = std::clamp<int64_t>(x, 0, logicalWidth);
   const int64_t y1 = std::clamp<int64_t>(y, 0, logicalHeight);
   const int64_t x2 = std::clamp<int64_t>(static_cast<int64_t>(x) + width,
      0, logicalWidth);
   const int64_t y2 = std::clamp<int64_t>(static_cast<int64_t>(y) + height,
      0, logicalHeight);
   if (x2 <= x1 || y2 <= y1)
      return false;

   const int64_t maxOffset = std::numeric_limits<int32_t>::max();
   const int64_t drawableWidth = std::min<int64_t>(extent.width, maxOffset);
   const int64_t drawableHeight = std::min<int64_t>(extent.height, maxOffset);
   const int64_t viewWidth = viewportWidth > 0 ? viewportWidth : drawableWidth;
   const int64_t viewHeight = viewportHeight > 0 ? viewportHeight : drawableHeight;
   const int64_t left = std::clamp<int64_t>(viewportX +
      x1 * viewWidth / logicalWidth, 0, drawableWidth);
   const int64_t top = std::clamp<int64_t>(viewportY +
      y1 * viewHeight / logicalHeight, 0, drawableHeight);
   const int64_t right = std::clamp<int64_t>(viewportX +
      x2 * viewWidth / logicalWidth, 0, drawableWidth);
   const int64_t bottom = std::clamp<int64_t>(viewportY +
      y2 * viewHeight / logicalHeight, 0, drawableHeight);
   if (right <= left || bottom <= top)
      return false;

   rect->offset = vk::Offset2D(static_cast<int32_t>(left),
      static_cast<int32_t>(top));
   rect->extent = vk::Extent2D(static_cast<uint32_t>(right - left),
      static_cast<uint32_t>(bottom - top));
   return true;
}
}

struct cVulkanSceneRenderer::sTexture
{
   sTexture() = default;
   sTexture(const sTexture &) = delete;
   sTexture &operator=(const sTexture &) = delete;
   sTexture(sTexture &&other) noexcept
      : memory(std::move(other.memory)), image(std::move(other.image)),
        view(std::move(other.view)), width(other.width), height(other.height)
   {
      other.width = other.height = 0;
   }
   sTexture &operator=(sTexture &&other) noexcept
   {
      if (this != &other)
      {
         Reset();
         memory = std::move(other.memory);
         image = std::move(other.image);
         view = std::move(other.view);
         width = other.width;
         height = other.height;
         other.width = other.height = 0;
      }
      return *this;
   }
   ~sTexture() { Reset(); }

   void Reset()
   {
      view.clear();
      image.clear();
      memory.clear();
      width = height = 0;
   }

   vk::raii::DeviceMemory memory{nullptr};
   vk::raii::Image image{nullptr};
   vk::raii::ImageView view{nullptr};
   int width{};
   int height{};
};

struct cVulkanSceneRenderer::sVertexBuffer
{
   sVertexBuffer() = default;
   sVertexBuffer(const sVertexBuffer &) = delete;
   sVertexBuffer &operator=(const sVertexBuffer &) = delete;
   sVertexBuffer(sVertexBuffer &&other) noexcept
      : device(std::exchange(other.device, nullptr)),
        memory(std::move(other.memory)), buffer(std::move(other.buffer)),
        mapped(std::exchange(other.mapped, nullptr)),
        capacity(std::exchange(other.capacity, 0)),
        hostCoherent(std::exchange(other.hostCoherent, false))
   {
   }
   ~sVertexBuffer() { Reset(); }

   sVertexBuffer &operator=(sVertexBuffer &&other) noexcept
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

   void Reset() noexcept
   {
      if (mapped)
      {
         try { device.unmapMemory(*memory); } catch (...) {}
         mapped = nullptr;
      }
      buffer.clear();
      memory.clear();
      device = nullptr;
      capacity = 0;
      hostCoherent = false;
   }

   vk::Device device{};
   vk::raii::DeviceMemory memory{nullptr};
   vk::raii::Buffer buffer{nullptr};
   void *mapped{};
   vk::DeviceSize capacity{};
   bool hostCoherent{};
};

struct cVulkanSceneRenderer::sFrameResources
{
   sVertexBuffer vertexBuffer;
   std::vector<sVertexBuffer> retiredVertexBuffers;
   std::vector<vk::raii::DescriptorPool> descriptorPools;
   size_t descriptorPoolIndex{};
   vk::DeviceSize vertexOffset{};
};

namespace
{
vk::raii::DescriptorPool CreateDescriptorPool(
   const vk::raii::Device &device)
{
   const vk::DescriptorPoolSize poolSize(
      vk::DescriptorType::eCombinedImageSampler,
      kDescriptorSetsPerFrame * 2);
   return device.createDescriptorPool(vk::DescriptorPoolCreateInfo(
      vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
      kDescriptorSetsPerFrame, 1, &poolSize));
}
}

cVulkanSceneRenderer::cVulkanSceneRenderer()
{
   m_pipelines.reserve(48);
   for (int i = 0; i < 48; ++i)
      m_pipelines.emplace_back(nullptr);
}

cVulkanSceneRenderer::~cVulkanSceneRenderer()
{
   Shutdown();
}

bool cVulkanSceneRenderer::Initialize(cVulkanContext &context,
   const char *vertexShaderPath, const char *fragmentShaderPath)
{
   Shutdown();
   const auto vertexCode = ReadShader(vertexShaderPath);
   const auto fragmentCode = ReadShader(fragmentShaderPath);
   if (vertexCode.empty() || fragmentCode.empty())
      return false;

   m_context = &context;
   m_viewportWidth = static_cast<int>(context.SwapchainExtent().width);
   m_viewportHeight = static_cast<int>(context.SwapchainExtent().height);
   const vk::Device device = context.Device();
   try
   {
      m_vertexShader = context.RaiiDevice().createShaderModule(vk::ShaderModuleCreateInfo(
         {}, vertexCode.size() * sizeof(uint32_t), vertexCode.data()));
      m_fragmentShader = context.RaiiDevice().createShaderModule(vk::ShaderModuleCreateInfo(
         {}, fragmentCode.size() * sizeof(uint32_t), fragmentCode.data()));

      const vk::DescriptorSetLayoutBinding bindings[] = {
         {0, vk::DescriptorType::eCombinedImageSampler, 1,
            vk::ShaderStageFlagBits::eFragment},
         {1, vk::DescriptorType::eCombinedImageSampler, 1,
            vk::ShaderStageFlagBits::eFragment}};
      m_descriptorLayout = context.RaiiDevice().createDescriptorSetLayout(
         vk::DescriptorSetLayoutCreateInfo({}, 2, bindings));
      vk::PushConstantRange pushRange(vk::ShaderStageFlagBits::eVertex |
         vk::ShaderStageFlagBits::eFragment, 0, sizeof(sPushConstants));
      const vk::DescriptorSetLayout descriptorLayout = *m_descriptorLayout;
      m_pipelineLayout = context.RaiiDevice().createPipelineLayout(
         vk::PipelineLayoutCreateInfo({}, 1, &descriptorLayout, 1, &pushRange));

      for (int wrap = 0; wrap < 2; ++wrap)
         for (int smooth = 0; smooth < 2; ++smooth)
         {
            vk::SamplerCreateInfo samplerInfo{};
            samplerInfo.magFilter = smooth ? vk::Filter::eLinear : vk::Filter::eNearest;
            samplerInfo.minFilter = smooth ? vk::Filter::eLinear : vk::Filter::eNearest;
            samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
            samplerInfo.addressModeU = wrap ? vk::SamplerAddressMode::eRepeat : vk::SamplerAddressMode::eClampToEdge;
            samplerInfo.addressModeV = samplerInfo.addressModeU;
            samplerInfo.addressModeW = samplerInfo.addressModeU;
            samplerInfo.maxLod = 0.0f;
            m_samplers[wrap][smooth] =
               context.RaiiDevice().createSampler(samplerInfo);
         }

      for (auto &frame : m_frames)
      {
         frame = std::make_unique<sFrameResources>();
         AllocateVertexBuffer(frame->vertexBuffer, kVertexBufferBytes);

         frame->descriptorPools.push_back(
            CreateDescriptorPool(context.RaiiDevice()));
      }

      m_whiteTexture = std::make_unique<sTexture>();
      const uint32_t whitePixel = 0xffffffffu;
      if (!CreateTextureImage(*m_whiteTexture, 1, 1, &whitePixel, 4))
         throw std::runtime_error("failed to create Vulkan fallback texture");
      m_boundTextures[0] = m_whiteTexture.get();
      m_boundTextures[1] = m_whiteTexture.get();
      return true;
   }
   catch (const std::exception &)
   {
      Shutdown();
      return false;
   }
}

void cVulkanSceneRenderer::Shutdown()
{
   if (!m_context)
      return;
   const vk::Device device = m_context->Device();
   try { m_context->RaiiDevice().waitIdle(); } catch (...) {}
   for (auto &pipeline : m_pipelines)
      pipeline.clear();
   for (auto &frame : m_frames)
      if (frame)
      {
         frame->descriptorPools.clear();
         if (frame->vertexBuffer.mapped)
            device.unmapMemory(*frame->vertexBuffer.memory);
         frame->vertexBuffer.mapped = nullptr;
         frame->retiredVertexBuffers.clear();
         frame.reset();
      }
   for (auto &entry : m_textures)
      if (entry.second)
         DestroyTextureImage(*entry.second);
   m_textures.clear();
   if (m_whiteTexture)
   {
      DestroyTextureImage(*m_whiteTexture);
      m_whiteTexture.reset();
   }
   for (auto &frameTextures : m_deferredTextures)
   {
      for (auto &texture : frameTextures)
         if (texture) DestroyTextureImage(*texture);
      frameTextures.clear();
   }
   for (auto &wrap : m_samplers)
      for (auto &sampler : wrap)
         sampler.clear();
   m_pipelineLayout.clear();
   m_descriptorLayout.clear();
   m_vertexShader.clear();
   m_fragmentShader.clear();
   m_context = nullptr;
   m_boundTextures[0] = m_boundTextures[1] = nullptr;
}

void cVulkanSceneRenderer::ResetPipelines()
{
   if (!m_context) return;
   m_context->RaiiDevice().waitIdle();
   for (auto &pipeline : m_pipelines)
      pipeline.clear();
}

void cVulkanSceneRenderer::BeginFrame(uint32_t frameIndex)
{
   if (!m_context || frameIndex >= m_frames.size())
      return;
   m_frameIndex = frameIndex;
   // The context has waited for this frame slot's fence before calling us.
   // Reclaim images referenced by its previous submission without stalling
   // unrelated frame slots or the entire device.
   auto &retiredTextures = m_deferredTextures[frameIndex];
   for (auto &texture : retiredTextures)
      if (texture) DestroyTextureImage(*texture);
   retiredTextures.clear();
   sFrameResources &frame = *m_frames[frameIndex];
   // The frame fence has signaled, so the previous submission no longer
   // references buffers retired after an in-frame capacity increase.
   frame.retiredVertexBuffers.clear();
   for (const auto &pool : frame.descriptorPools)
      m_context->Device().resetDescriptorPool(*pool);
   frame.descriptorPoolIndex = 0;
   frame.vertexOffset = 0;
}

void cVulkanSceneRenderer::AllocateVertexBuffer(sVertexBuffer &buffer,
   vk::DeviceSize capacity)
{
   if (!m_context || !capacity)
      throw std::runtime_error("invalid Vulkan vertex buffer allocation");
   sVertexBuffer allocated;
   const vk::BufferCreateInfo bufferInfo({}, capacity,
      vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive);
   allocated.buffer = m_context->RaiiDevice().createBuffer(bufferInfo);
   const auto requirements = m_context->Device().getBufferMemoryRequirements(
      *allocated.buffer);
   const uint32_t memoryType = m_context->FindHostVisibleMemoryType(
      requirements.memoryTypeBits,
      &allocated.hostCoherent);
   const vk::MemoryAllocateInfo allocation(requirements.size, memoryType);
   allocated.memory = m_context->RaiiDevice().allocateMemory(allocation);
   m_context->Device().bindBufferMemory(*allocated.buffer, *allocated.memory, 0);
   allocated.mapped = m_context->Device().mapMemory(*allocated.memory, 0,
      requirements.size);
   allocated.device = m_context->Device();
   allocated.capacity = capacity;
   buffer = std::move(allocated);
}

vk::Pipeline cVulkanSceneRenderer::GetPipeline(int primitive)
{
   primitive = std::clamp(primitive, 0, 2);
   const size_t index = static_cast<size_t>(primitive) * 16 +
      (m_depthCompare ? 1 : 0) + (m_depthWrite ? 2 : 0) +
      static_cast<size_t>(std::clamp(m_blendMode, 0, 3)) * 4;
   if (*m_pipelines[index])
      return *m_pipelines[index];

   const vk::PipelineShaderStageCreateInfo stages[] = {
      { {}, vk::ShaderStageFlagBits::eVertex, *m_vertexShader, "main" },
      { {}, vk::ShaderStageFlagBits::eFragment, *m_fragmentShader, "main" }};
   vk::VertexInputBindingDescription binding(0, sizeof(sRenderBackendVertex),
      vk::VertexInputRate::eVertex);
   const vk::VertexInputAttributeDescription attributes[] = {
      {0, 0, vk::Format::eR32G32B32A32Sfloat,
         static_cast<uint32_t>(offsetof(sRenderBackendVertex, x))},
      {1, 0, vk::Format::eR32G32B32A32Sfloat,
         static_cast<uint32_t>(offsetof(sRenderBackendVertex, r))},
      {2, 0, vk::Format::eR32Sfloat,
         static_cast<uint32_t>(offsetof(sRenderBackendVertex, fog))},
      {3, 0, vk::Format::eR32G32Sfloat,
         static_cast<uint32_t>(offsetof(sRenderBackendVertex, u0))},
      {4, 0, vk::Format::eR32G32Sfloat,
         static_cast<uint32_t>(offsetof(sRenderBackendVertex, u1))}};
   vk::PipelineVertexInputStateCreateInfo vertexInput({}, 1, &binding,
      static_cast<uint32_t>(std::size(attributes)), attributes);
   const vk::PrimitiveTopology topologies[] = {vk::PrimitiveTopology::eTriangleList,
      vk::PrimitiveTopology::eLineList, vk::PrimitiveTopology::ePointList};
   vk::PipelineInputAssemblyStateCreateInfo assembly({}, topologies[primitive], VK_FALSE);
   vk::PipelineViewportStateCreateInfo viewport({}, 1, nullptr, 1, nullptr);
   vk::PipelineRasterizationStateCreateInfo rasterization{};
   rasterization.polygonMode = vk::PolygonMode::eFill;
   rasterization.cullMode = vk::CullModeFlagBits::eNone;
   rasterization.frontFace = vk::FrontFace::eCounterClockwise;
   rasterization.lineWidth = 1.0f;
   vk::PipelineMultisampleStateCreateInfo multisample({}, vk::SampleCountFlagBits::e1);
   vk::PipelineDepthStencilStateCreateInfo depth{};
   depth.depthTestEnable = m_depthCompare || m_depthWrite;
   depth.depthWriteEnable = m_depthWrite;
   depth.depthCompareOp = m_depthCompare ? vk::CompareOp::eLessOrEqual : vk::CompareOp::eAlways;
   depth.depthBoundsTestEnable = VK_FALSE;
   depth.stencilTestEnable = VK_FALSE;
   vk::PipelineColorBlendAttachmentState blend{};
   blend.colorWriteMask = vk::ColorComponentFlagBits::eR |
      vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
      vk::ColorComponentFlagBits::eA;
   blend.blendEnable = m_blendMode != 0;
   if (m_blendMode == 2)
   {
      blend.srcColorBlendFactor = vk::BlendFactor::eDstColor;
      blend.dstColorBlendFactor = vk::BlendFactor::eZero;
      blend.srcAlphaBlendFactor = vk::BlendFactor::eOne;
      blend.dstAlphaBlendFactor = vk::BlendFactor::eZero;
   }
   else if (m_blendMode == 3)
   {
      blend.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
      blend.dstColorBlendFactor = vk::BlendFactor::eOne;
      blend.srcAlphaBlendFactor = vk::BlendFactor::eOne;
      blend.dstAlphaBlendFactor = vk::BlendFactor::eOne;
   }
   else
   {
      blend.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
      blend.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
      blend.srcAlphaBlendFactor = vk::BlendFactor::eOne;
      blend.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
   }
   blend.colorBlendOp = vk::BlendOp::eAdd;
   blend.alphaBlendOp = vk::BlendOp::eAdd;
   vk::PipelineColorBlendStateCreateInfo colorBlend({}, VK_FALSE,
      vk::LogicOp::eCopy, 1, &blend);
   const vk::DynamicState dynamicStates[] = {vk::DynamicState::eViewport,
      vk::DynamicState::eScissor};
   vk::PipelineDynamicStateCreateInfo dynamic({}, 2, dynamicStates);
   vk::GraphicsPipelineCreateInfo info{};
   info.stageCount = 2;
   info.pStages = stages;
   info.pVertexInputState = &vertexInput;
   info.pInputAssemblyState = &assembly;
   info.pViewportState = &viewport;
   info.pRasterizationState = &rasterization;
   info.pMultisampleState = &multisample;
   info.pDepthStencilState = &depth;
   info.pColorBlendState = &colorBlend;
   info.pDynamicState = &dynamic;
   info.layout = *m_pipelineLayout;
   info.renderPass = m_context->RenderPass();
   info.subpass = 0;
   m_pipelines[index] = m_context->RaiiDevice().createGraphicsPipeline(
      nullptr, info);
   return *m_pipelines[index];
}

vk::Sampler cVulkanSceneRenderer::GetSampler(bool wrap, bool smooth) const
{
   return *m_samplers[wrap ? 1 : 0][smooth ? 1 : 0];
}

bool cVulkanSceneRenderer::Draw(vk::CommandBuffer commandBuffer,
   const sRenderBackendVertex *vertices, int vertexCount, int primitive,
   bool useSecondTexture, int logicalWidth, int logicalHeight,
   bool transparentBlack)
{
   if (!m_context || !commandBuffer || !vertices || vertexCount <= 0 ||
       logicalWidth <= 0 || logicalHeight <= 0 || primitive < 0 || primitive > 2)
      return false;
   const vk::Extent2D extent = m_context->SwapchainExtent();
   const int viewportWidth = m_viewportWidth > 0 ? m_viewportWidth :
      static_cast<int>(extent.width);
   const int viewportHeight = m_viewportHeight > 0 ? m_viewportHeight :
      static_cast<int>(extent.height);
   const int64_t scissorLeft = std::max<int64_t>(m_viewportX, 0);
   const int64_t scissorTop = std::max<int64_t>(m_viewportY, 0);
   const int64_t scissorRight = std::min<int64_t>(
      static_cast<int64_t>(m_viewportX) + viewportWidth, extent.width);
   const int64_t scissorBottom = std::min<int64_t>(
      static_cast<int64_t>(m_viewportY) + viewportHeight, extent.height);
   if (scissorRight <= scissorLeft || scissorBottom <= scissorTop)
      return false;
   sFrameResources &frame = *m_frames[m_frameIndex];
   const vk::DeviceSize byteCount = static_cast<vk::DeviceSize>(vertexCount) *
      sizeof(sRenderBackendVertex);
   if (byteCount > std::numeric_limits<size_t>::max())
      return false;
   if (frame.vertexOffset > frame.vertexBuffer.capacity ||
       byteCount > frame.vertexBuffer.capacity - frame.vertexOffset)
   {
      const vk::DeviceSize doubledCapacity =
         frame.vertexBuffer.capacity <=
            std::numeric_limits<vk::DeviceSize>::max() / 2
         ? frame.vertexBuffer.capacity * 2 : byteCount;
      const vk::DeviceSize newCapacity = std::max(
         {kVertexBufferBytes, byteCount, doubledCapacity});
      frame.retiredVertexBuffers.push_back(std::move(frame.vertexBuffer));
      sVertexBuffer &retired = frame.retiredVertexBuffers.back();
      m_context->Device().unmapMemory(*retired.memory);
      retired.mapped = nullptr;
      frame.vertexOffset = 0;
      AllocateVertexBuffer(frame.vertexBuffer, newCapacity);
   }
   std::memcpy(static_cast<uint8_t *>(frame.vertexBuffer.mapped) +
         frame.vertexOffset,
      vertices, static_cast<size_t>(byteCount));
   if (!frame.vertexBuffer.hostCoherent)
   {
      const vk::DeviceSize atomSize = std::max<vk::DeviceSize>(
         m_context->Properties().limits.nonCoherentAtomSize, 1);
      const vk::DeviceSize flushOffset =
         frame.vertexOffset / atomSize * atomSize;
      m_context->Device().flushMappedMemoryRanges(
         vk::MappedMemoryRange(*frame.vertexBuffer.memory, flushOffset,
            VK_WHOLE_SIZE));
   }

   auto *first = static_cast<sTexture *>(m_boundTextures[0]);
   auto *second = useSecondTexture ? static_cast<sTexture *>(m_boundTextures[1]) :
      m_whiteTexture.get();
   if (!first) first = m_whiteTexture.get();
   if (!second) second = m_whiteTexture.get();
   vk::DescriptorSet descriptor{};
   while (!descriptor)
   {
      if (frame.descriptorPoolIndex >= frame.descriptorPools.size())
      {
      frame.descriptorPools.push_back(
         CreateDescriptorPool(m_context->RaiiDevice()));
      }
      const vk::DescriptorSetAllocateInfo setAllocation(
         *frame.descriptorPools[frame.descriptorPoolIndex],
         1, &*m_descriptorLayout);
      try
      {
         descriptor = m_context->Device().allocateDescriptorSets(
            setAllocation).front();
      }
      catch (const vk::OutOfPoolMemoryError &)
      {
         ++frame.descriptorPoolIndex;
      }
      catch (const vk::FragmentedPoolError &)
      {
         ++frame.descriptorPoolIndex;
      }
   }
   vk::DescriptorImageInfo imageInfos[] = {
      {GetSampler(m_wrap[0], m_smooth[0]), *first->view,
         vk::ImageLayout::eShaderReadOnlyOptimal},
      {GetSampler(m_wrap[1], m_smooth[1]), *second->view,
         vk::ImageLayout::eShaderReadOnlyOptimal}};
   vk::WriteDescriptorSet writes[] = {
      {descriptor, 0, 0, 1, vk::DescriptorType::eCombinedImageSampler,
         &imageInfos[0]},
      {descriptor, 1, 0, 1, vk::DescriptorType::eCombinedImageSampler,
         &imageInfos[1]}};
   m_context->Device().updateDescriptorSets(writes, {});

   commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
      GetPipeline(primitive));
   commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
      *m_pipelineLayout, 0, descriptor, {});
   commandBuffer.setViewport(0, vk::Viewport(
      static_cast<float>(m_viewportX), static_cast<float>(m_viewportY),
      static_cast<float>(viewportWidth), static_cast<float>(viewportHeight),
      0.0f, 1.0f));
   commandBuffer.setScissor(0, vk::Rect2D(
      {static_cast<int32_t>(scissorLeft), static_cast<int32_t>(scissorTop)},
      {static_cast<uint32_t>(scissorRight - scissorLeft),
       static_cast<uint32_t>(scissorBottom - scissorTop)}));
   sPushConstants constants{};
   constants.viewportSize[0] = static_cast<float>(logicalWidth);
   constants.viewportSize[1] = static_cast<float>(logicalHeight);
   constants.gamma = m_gamma;
   constants.useTexture1 = useSecondTexture ? 1u : 0u;
   constants.fogEnabled = m_fog ? 1u : 0u;
   constants.alphaTest = m_alphaTest ? 1u : 0u;
   constants.transparentBlack = transparentBlack ? 1u : 0u;
   constants.fogColor[0] = ((m_fogColor >> 16) & 255) / 255.0f;
   constants.fogColor[1] = ((m_fogColor >> 8) & 255) / 255.0f;
   constants.fogColor[2] = (m_fogColor & 255) / 255.0f;
   constants.fogColor[3] = 1.0f;
   commandBuffer.pushConstants(*m_pipelineLayout,
      vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
      0, sizeof(constants), &constants);
   commandBuffer.bindVertexBuffers(0, *frame.vertexBuffer.buffer,
      frame.vertexOffset);
   commandBuffer.draw(static_cast<uint32_t>(vertexCount), 1, 0, 0);
   frame.vertexOffset += byteCount;
   return true;
}

void cVulkanSceneRenderer::ClearDepth(vk::CommandBuffer commandBuffer)
{
   if (!m_context || !commandBuffer) return;
   const vk::Extent2D extent = m_context->SwapchainExtent();
   vk::ClearAttachment attachment{};
   attachment.aspectMask = vk::ImageAspectFlagBits::eDepth;
   attachment.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
   vk::ClearRect rect{};
   rect.rect = vk::Rect2D({0, 0}, extent);
   rect.baseArrayLayer = 0;
   rect.layerCount = 1;
   commandBuffer.clearAttachments(attachment, rect);
}

void cVulkanSceneRenderer::ClearDepthRect(vk::CommandBuffer commandBuffer,
   int x, int y, int width, int height, int logicalWidth, int logicalHeight)
{
   if (!m_context || !commandBuffer)
      return;
   const vk::Extent2D extent = m_context->SwapchainExtent();
   vk::Rect2D rect{};
   if (!MapLogicalRect(extent, m_viewportX, m_viewportY, m_viewportWidth,
       m_viewportHeight, x, y, width, height, logicalWidth, logicalHeight,
       &rect))
      return;
   vk::ClearAttachment attachment{};
   attachment.aspectMask = vk::ImageAspectFlagBits::eDepth;
   attachment.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
   vk::ClearRect clearRect{};
   clearRect.rect = rect;
   clearRect.baseArrayLayer = 0;
   clearRect.layerCount = 1;
   commandBuffer.clearAttachments(attachment, clearRect);
}

void cVulkanSceneRenderer::ClearCanvasRegion(vk::CommandBuffer commandBuffer,
   int x, int y, int width, int height, int logicalWidth, int logicalHeight)
{
   if (!m_context || !commandBuffer)
      return;
   const vk::Extent2D extent = m_context->SwapchainExtent();
   vk::Rect2D rect{};
   if (!MapLogicalRect(extent, m_viewportX, m_viewportY, m_viewportWidth,
       m_viewportHeight, x, y, width, height, logicalWidth, logicalHeight,
       &rect))
      return;
   vk::ClearAttachment attachment{};
   attachment.aspectMask = vk::ImageAspectFlagBits::eColor;
   attachment.colorAttachment = 0;
   attachment.clearValue.color = vk::ClearColorValue(
      std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
   vk::ClearRect clearRect{};
   clearRect.rect = rect;
   clearRect.baseArrayLayer = 0;
   clearRect.layerCount = 1;
   commandBuffer.clearAttachments(attachment, clearRect);
}

void cVulkanSceneRenderer::SetDepth(bool compare, bool write)
{ m_depthCompare = compare; m_depthWrite = write; }
void cVulkanSceneRenderer::SetBlend(int mode)
{ m_blendMode = std::clamp(mode, 0, 3); }
void cVulkanSceneRenderer::SetAlphaTest(bool enabled) { m_alphaTest = enabled; }
void cVulkanSceneRenderer::SetFog(bool enabled, uint32_t rgb)
{ m_fog = enabled; m_fogColor = rgb; }
void cVulkanSceneRenderer::SetGamma(float gamma)
{ if (gamma > 0.0f) m_gamma = gamma; }
void cVulkanSceneRenderer::SetViewport(int x, int y, int width, int height)
{
   if (width <= 0 || height <= 0) return;
   m_viewportX = x;
   m_viewportY = y;
   m_viewportWidth = width;
   m_viewportHeight = height;
}
void cVulkanSceneRenderer::SetSampler(int level, bool wrap, bool smooth)
{ if (level >= 0 && level < 2) { m_wrap[level] = wrap; m_smooth[level] = smooth; } }
void cVulkanSceneRenderer::BindTexture(int level, void *texture)
{ if (level >= 0 && level < 2) m_boundTextures[level] = texture ? texture : m_whiteTexture.get(); }

bool cVulkanSceneRenderer::CreateTextureImage(sTexture &texture, int width,
   int height, const void *rgba, int rowBytes)
{
   if (!m_context || !rgba || !IsValidRgbaImage(width, height, rowBytes))
      return false;
   const vk::Device device = m_context->Device();
   vk::raii::DeviceMemory stagingMemory{nullptr};
   vk::raii::Buffer staging{nullptr};
   try
   {
      const size_t pixelBytes = static_cast<size_t>(width) * 4;
      const vk::DeviceSize bytes = static_cast<vk::DeviceSize>(pixelBytes) *
         static_cast<uint32_t>(height);
      staging = m_context->RaiiDevice().createBuffer(vk::BufferCreateInfo({}, bytes,
         vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive));
      auto req = device.getBufferMemoryRequirements(*staging);
      bool stagingCoherent = false;
      const uint32_t stagingMemoryType = m_context->FindHostVisibleMemoryType(
         req.memoryTypeBits, &stagingCoherent);
      stagingMemory = m_context->RaiiDevice().allocateMemory(vk::MemoryAllocateInfo(req.size,
         stagingMemoryType));
      device.bindBufferMemory(*staging, *stagingMemory, 0);
      void *mapped = device.mapMemory(*stagingMemory, 0, req.size);
      for (int y = 0; y < height; ++y)
         std::memcpy(static_cast<uint8_t *>(mapped) +
               static_cast<size_t>(y) * pixelBytes,
            static_cast<const uint8_t *>(rgba) +
               static_cast<size_t>(y) * static_cast<size_t>(rowBytes),
            pixelBytes);
      if (!stagingCoherent)
         device.flushMappedMemoryRanges(
            vk::MappedMemoryRange(*stagingMemory, 0, VK_WHOLE_SIZE));
      device.unmapMemory(*stagingMemory);

      vk::ImageCreateInfo imageInfo{};
      imageInfo.imageType = vk::ImageType::e2D;
      imageInfo.format = vk::Format::eR8G8B8A8Unorm;
      imageInfo.extent = vk::Extent3D(width, height, 1);
      imageInfo.mipLevels = 1;
      imageInfo.arrayLayers = 1;
      imageInfo.samples = vk::SampleCountFlagBits::e1;
      imageInfo.tiling = vk::ImageTiling::eOptimal;
      imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst |
         vk::ImageUsageFlagBits::eSampled;
      imageInfo.sharingMode = vk::SharingMode::eExclusive;
      imageInfo.initialLayout = vk::ImageLayout::eUndefined;
      texture.image = m_context->RaiiDevice().createImage(imageInfo);
      req = device.getImageMemoryRequirements(*texture.image);
      texture.memory = m_context->RaiiDevice().allocateMemory(vk::MemoryAllocateInfo(req.size,
         m_context->FindMemoryType(req.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal)));
      device.bindImageMemory(*texture.image, *texture.memory, 0);
      // Finish all potentially throwing texture-object setup before queueing
      // the upload, so the failure path cannot destroy an image still used by
      // an asynchronous transfer.
      texture.view = m_context->RaiiDevice().createImageView(
         vk::ImageViewCreateInfo({}, *texture.image,
         vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Unorm,
         {}, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));
      const vk::Image textureImage = *texture.image;
      const vk::Buffer stagingBuffer = *staging;
      const bool copied = m_context->SubmitUpload([&](vk::CommandBuffer command) {
         vk::ImageMemoryBarrier toTransfer{};
         toTransfer.oldLayout = vk::ImageLayout::eUndefined;
         toTransfer.newLayout = vk::ImageLayout::eTransferDstOptimal;
         toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
         toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
         toTransfer.image = textureImage;
         toTransfer.subresourceRange = vk::ImageSubresourceRange(
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
         toTransfer.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
         command.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, toTransfer);
         vk::BufferImageCopy region{};
         region.imageSubresource = vk::ImageSubresourceLayers(
            vk::ImageAspectFlagBits::eColor, 0, 0, 1);
         region.imageExtent = vk::Extent3D(width, height, 1);
         command.copyBufferToImage(stagingBuffer, textureImage,
            vk::ImageLayout::eTransferDstOptimal, region);
         vk::ImageMemoryBarrier toShader{};
         toShader.oldLayout = vk::ImageLayout::eTransferDstOptimal;
         toShader.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
         toShader.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
         toShader.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
         toShader.image = textureImage;
         toShader.subresourceRange = vk::ImageSubresourceRange(
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
         toShader.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
         toShader.dstAccessMask = vk::AccessFlagBits::eShaderRead;
         command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, toShader);
      }, std::move(staging), std::move(stagingMemory));
      if (!copied)
         throw std::runtime_error("Vulkan texture upload failed");
      texture.width = width;
      texture.height = height;
      return true;
   }
   catch (...)
   {
      DestroyTextureImage(texture);
      return false;
   }
}

void cVulkanSceneRenderer::DestroyTextureImage(sTexture &texture)
{
   texture.Reset();
}

void *cVulkanSceneRenderer::CreateTexture(int width, int height,
   const void *rgba, int rowBytes)
{
   if (!m_context) return nullptr;
   auto texture = std::make_unique<sTexture>();
   if (!CreateTextureImage(*texture, width, height, rgba, rowBytes))
      return nullptr;
   sTexture *handle = texture.get();
   m_textures.emplace(handle, std::move(texture));
   return handle;
}

bool cVulkanSceneRenderer::UpdateTexture(void *handle, int width, int height,
   const void *rgba, int rowBytes)
{
   return UpdateTextureInternal(handle, width, height, rgba, rowBytes, false);
}

bool cVulkanSceneRenderer::UpdateFrameTexture(void *handle, int width,
   int height, const void *rgba, int rowBytes)
{
   return UpdateTextureInternal(handle, width, height, rgba, rowBytes, true);
}

bool cVulkanSceneRenderer::UpdateTextureInternal(void *handle, int width,
   int height, const void *rgba, int rowBytes, bool frameSlotIdle)
{
   auto *texture = static_cast<sTexture *>(handle);
   if (!texture || !m_context || !rgba ||
       !IsValidRgbaImage(width, height, rowBytes))
      return false;
   if (frameSlotIdle && (!m_context->FrameActive() ||
       m_frameIndex != m_context->FrameIndex()))
      return false;
   const vk::Device device = m_context->Device();
   // RenderBackendDraw records commands into the active application frame.
   // Keep the previous image alive when a texture changes mid-frame so draws
   // already recorded keep sampling the old contents, matching the legacy
   // backend's flush-before-update ordering.
   if (texture->width != width || texture->height != height ||
       (m_context->FrameActive() && !frameSlotIdle))
   {
      if (m_context->FrameActive() && !frameSlotIdle)
      {
         auto &retiredTextures = m_deferredTextures[m_frameIndex];
         retiredTextures.reserve(retiredTextures.size() + 1);
         auto oldTexture = std::make_unique<sTexture>();
         sTexture replacement;
         if (!CreateTextureImage(replacement, width, height, rgba, rowBytes))
            return false;
         *oldTexture = std::move(*texture);
         retiredTextures.push_back(std::move(oldTexture));
         *texture = std::move(replacement);
         return true;
      }

      sTexture replacement;
      if (!CreateTextureImage(replacement, width, height, rgba, rowBytes))
         return false;
      m_context->RaiiDevice().waitIdle();
      DestroyTextureImage(*texture);
      *texture = std::move(replacement);
      return true;
   }

   vk::raii::DeviceMemory stagingMemory{nullptr};
   vk::raii::Buffer staging{nullptr};
   try
   {
      const size_t pixelBytes = static_cast<size_t>(width) * 4;
      const vk::DeviceSize bytes = static_cast<vk::DeviceSize>(pixelBytes) *
         static_cast<uint32_t>(height);
      staging = m_context->RaiiDevice().createBuffer(vk::BufferCreateInfo({}, bytes,
         vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive));
      const auto requirements = device.getBufferMemoryRequirements(*staging);
      bool stagingCoherent = false;
      const uint32_t stagingMemoryType = m_context->FindHostVisibleMemoryType(
         requirements.memoryTypeBits,
         &stagingCoherent);
      stagingMemory = m_context->RaiiDevice().allocateMemory(vk::MemoryAllocateInfo(
         requirements.size, stagingMemoryType));
      device.bindBufferMemory(*staging, *stagingMemory, 0);
      void *mapped = device.mapMemory(*stagingMemory, 0, requirements.size);
      for (int y = 0; y < height; ++y)
         std::memcpy(static_cast<uint8_t *>(mapped) +
               static_cast<size_t>(y) * pixelBytes,
            static_cast<const uint8_t *>(rgba) +
               static_cast<size_t>(y) * static_cast<size_t>(rowBytes),
            pixelBytes);
      if (!stagingCoherent)
         device.flushMappedMemoryRanges(
            vk::MappedMemoryRange(*stagingMemory, 0, VK_WHOLE_SIZE));
      device.unmapMemory(*stagingMemory);
      const vk::Image image = *texture->image;
      const vk::Buffer stagingBuffer = *staging;
      const bool uploaded = m_context->SubmitUpload(
         [&](vk::CommandBuffer command) {
            vk::ImageMemoryBarrier toTransfer{};
            toTransfer.oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            toTransfer.newLayout = vk::ImageLayout::eTransferDstOptimal;
            toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toTransfer.image = image;
            toTransfer.subresourceRange = vk::ImageSubresourceRange(
               vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
            toTransfer.srcAccessMask = vk::AccessFlagBits::eMemoryRead |
               vk::AccessFlagBits::eMemoryWrite;
            toTransfer.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
            command.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
               vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, toTransfer);
            vk::BufferImageCopy region{};
            region.imageSubresource = vk::ImageSubresourceLayers(
               vk::ImageAspectFlagBits::eColor, 0, 0, 1);
            region.imageExtent = vk::Extent3D(width, height, 1);
            command.copyBufferToImage(stagingBuffer, image,
               vk::ImageLayout::eTransferDstOptimal, region);
            vk::ImageMemoryBarrier toShader{};
            toShader.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            toShader.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            toShader.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toShader.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toShader.image = image;
            toShader.subresourceRange = vk::ImageSubresourceRange(
               vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
            toShader.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            toShader.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
               vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, toShader);
         }, std::move(staging), std::move(stagingMemory));
      return uploaded;
   }
   catch (...)
   {
      return false;
   }
}

void cVulkanSceneRenderer::DestroyTexture(void *handle) noexcept
{
   try
   {
      auto *texture = static_cast<sTexture *>(handle);
      if (!texture || !m_context || texture == m_whiteTexture.get())
         return;
      const auto found = m_textures.find(texture);
      if (found == m_textures.end())
         return;
      for (void *&bound : m_boundTextures)
         if (bound == texture) bound = m_whiteTexture.get();
      if (m_context->FrameActive())
      {
         auto &retiredTextures = m_deferredTextures[m_frameIndex];
         retiredTextures.reserve(retiredTextures.size() + 1);
         retiredTextures.emplace_back(std::move(found->second));
         m_textures.erase(found);
      }
      else
      {
         m_context->RaiiDevice().waitIdle();
         DestroyTextureImage(*found->second);
         m_textures.erase(found);
      }
   }
   catch (...)
   {
      // Keep an unretired texture registered so Shutdown can release it after
      // the device has been quiesced. No Vulkan exception may escape through
      // movie, display-device, or legacy RenderBackend callers.
   }
}
