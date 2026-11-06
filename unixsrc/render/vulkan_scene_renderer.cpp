#include "vulkan_scene_renderer.hpp"

#if defined(__ANDROID__)
#include <SDL3/SDL_iostream.h>
#endif

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>

namespace
{
constexpr vk::DeviceSize kVertexBufferBytes = 8 * 1024 * 1024;
constexpr uint32_t kDescriptorSetsPerFrame = 4096;

uint32_t FindMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeBits,
   vk::MemoryPropertyFlags properties)
{
   const auto memory = physicalDevice.getMemoryProperties();
   for (uint32_t index = 0; index < memory.memoryTypeCount; ++index)
      if ((typeBits & (1u << index)) &&
          (memory.memoryTypes[index].propertyFlags & properties) == properties)
         return index;
   throw std::runtime_error("no compatible Vulkan memory type");
}

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
}

struct cVulkanSceneRenderer::sTexture
{
   vk::Image image{};
   vk::DeviceMemory memory{};
   vk::ImageView view{};
   int width{};
   int height{};
};

struct cVulkanSceneRenderer::sFrameResources
{
   vk::Buffer vertexBuffer{};
   vk::DeviceMemory vertexMemory{};
   void *mapped{};
   std::vector<vk::DescriptorPool> descriptorPools;
   size_t descriptorPoolIndex{};
   vk::DeviceSize vertexOffset{};
};

namespace
{
vk::DescriptorPool CreateDescriptorPool(vk::Device device)
{
   const vk::DescriptorPoolSize poolSize(
      vk::DescriptorType::eCombinedImageSampler,
      kDescriptorSetsPerFrame * 2);
   return device.createDescriptorPool(vk::DescriptorPoolCreateInfo(
      vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
      kDescriptorSetsPerFrame, 1, &poolSize));
}
}

cVulkanSceneRenderer::cVulkanSceneRenderer() = default;

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
      m_vertexShader = device.createShaderModule(vk::ShaderModuleCreateInfo(
         {}, vertexCode.size() * sizeof(uint32_t), vertexCode.data()));
      m_fragmentShader = device.createShaderModule(vk::ShaderModuleCreateInfo(
         {}, fragmentCode.size() * sizeof(uint32_t), fragmentCode.data()));

      const vk::DescriptorSetLayoutBinding bindings[] = {
         {0, vk::DescriptorType::eCombinedImageSampler, 1,
            vk::ShaderStageFlagBits::eFragment},
         {1, vk::DescriptorType::eCombinedImageSampler, 1,
            vk::ShaderStageFlagBits::eFragment}};
      m_descriptorLayout = device.createDescriptorSetLayout(
         vk::DescriptorSetLayoutCreateInfo({}, 2, bindings));
      vk::PushConstantRange pushRange(vk::ShaderStageFlagBits::eVertex |
         vk::ShaderStageFlagBits::eFragment, 0, sizeof(sPushConstants));
      m_pipelineLayout = device.createPipelineLayout(
         vk::PipelineLayoutCreateInfo({}, 1, &m_descriptorLayout, 1, &pushRange));

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
            m_samplers[wrap][smooth] = device.createSampler(samplerInfo);
         }

      for (auto &frame : m_frames)
      {
         frame = std::make_unique<sFrameResources>();
         vk::BufferCreateInfo bufferInfo({}, kVertexBufferBytes,
            vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive);
         frame->vertexBuffer = device.createBuffer(bufferInfo);
         const auto requirements = device.getBufferMemoryRequirements(frame->vertexBuffer);
         vk::MemoryAllocateInfo allocation(requirements.size,
            FindMemoryType(context.PhysicalDevice(), requirements.memoryTypeBits,
               vk::MemoryPropertyFlagBits::eHostVisible |
               vk::MemoryPropertyFlagBits::eHostCoherent));
         frame->vertexMemory = device.allocateMemory(allocation);
         device.bindBufferMemory(frame->vertexBuffer, frame->vertexMemory, 0);
         frame->mapped = device.mapMemory(frame->vertexMemory, 0,
            kVertexBufferBytes);

         frame->descriptorPools.push_back(CreateDescriptorPool(device));
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
   try { (void)device.waitIdle(); } catch (...) {}
   for (vk::Pipeline &pipeline : m_pipelines)
      if (pipeline) device.destroyPipeline(pipeline);
   for (auto &frame : m_frames)
      if (frame)
      {
         for (vk::DescriptorPool pool : frame->descriptorPools)
            if (pool) device.destroyDescriptorPool(pool);
         frame->descriptorPools.clear();
         if (frame->mapped) device.unmapMemory(frame->vertexMemory);
         if (frame->vertexBuffer) device.destroyBuffer(frame->vertexBuffer);
         if (frame->vertexMemory) device.freeMemory(frame->vertexMemory);
         frame.reset();
      }
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
      for (vk::Sampler &sampler : wrap)
         if (sampler) device.destroySampler(sampler);
   if (m_pipelineLayout) device.destroyPipelineLayout(m_pipelineLayout);
   if (m_descriptorLayout) device.destroyDescriptorSetLayout(m_descriptorLayout);
   if (m_vertexShader) device.destroyShaderModule(m_vertexShader);
   if (m_fragmentShader) device.destroyShaderModule(m_fragmentShader);
   m_pipelineLayout = nullptr;
   m_descriptorLayout = nullptr;
   m_vertexShader = nullptr;
   m_fragmentShader = nullptr;
   m_context = nullptr;
   m_boundTextures[0] = m_boundTextures[1] = nullptr;
}

void cVulkanSceneRenderer::ResetPipelines()
{
   if (!m_context) return;
   (void)m_context->Device().waitIdle();
   for (vk::Pipeline &pipeline : m_pipelines)
   {
      if (pipeline) m_context->Device().destroyPipeline(pipeline);
      pipeline = nullptr;
   }
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
   for (vk::DescriptorPool pool : frame.descriptorPools)
      m_context->Device().resetDescriptorPool(pool);
   frame.descriptorPoolIndex = 0;
   frame.vertexOffset = 0;
}

vk::Pipeline cVulkanSceneRenderer::GetPipeline(int primitive)
{
   primitive = std::clamp(primitive, 0, 2);
   const size_t index = static_cast<size_t>(primitive) * 16 +
      (m_depthCompare ? 1 : 0) + (m_depthWrite ? 2 : 0) +
      static_cast<size_t>(std::clamp(m_blendMode, 0, 3)) * 4;
   if (m_pipelines[index])
      return m_pipelines[index];

   const vk::Device device = m_context->Device();
   const vk::PipelineShaderStageCreateInfo stages[] = {
      { {}, vk::ShaderStageFlagBits::eVertex, m_vertexShader, "main" },
      { {}, vk::ShaderStageFlagBits::eFragment, m_fragmentShader, "main" }};
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
   info.layout = m_pipelineLayout;
   info.renderPass = m_context->RenderPass();
   info.subpass = 0;
   m_pipelines[index] = device.createGraphicsPipeline(nullptr, info).value;
   return m_pipelines[index];
}

vk::Sampler cVulkanSceneRenderer::GetSampler(bool wrap, bool smooth) const
{
   return m_samplers[wrap ? 1 : 0][smooth ? 1 : 0];
}

bool cVulkanSceneRenderer::Draw(vk::CommandBuffer commandBuffer,
   const sRenderBackendVertex *vertices, int vertexCount, int primitive,
   bool useSecondTexture, int logicalWidth, int logicalHeight,
   bool transparentBlack)
{
   if (!m_context || !commandBuffer || !vertices || vertexCount <= 0 ||
       logicalWidth <= 0 || logicalHeight <= 0 || primitive < 0 || primitive > 2)
      return false;
   sFrameResources &frame = *m_frames[m_frameIndex];
   const vk::DeviceSize byteCount = static_cast<vk::DeviceSize>(vertexCount) *
      sizeof(sRenderBackendVertex);
   if (byteCount > kVertexBufferBytes - frame.vertexOffset)
      return false;
   std::memcpy(static_cast<uint8_t *>(frame.mapped) + frame.vertexOffset,
      vertices, static_cast<size_t>(byteCount));

   auto *first = static_cast<sTexture *>(m_boundTextures[0]);
   auto *second = useSecondTexture ? static_cast<sTexture *>(m_boundTextures[1]) :
      m_whiteTexture.get();
   if (!first) first = m_whiteTexture.get();
   if (!second) second = m_whiteTexture.get();
   vk::DescriptorSet descriptor{};
   while (!descriptor)
   {
      if (frame.descriptorPoolIndex >= frame.descriptorPools.size())
         frame.descriptorPools.push_back(
            CreateDescriptorPool(m_context->Device()));
      const vk::DescriptorSetAllocateInfo setAllocation(
         frame.descriptorPools[frame.descriptorPoolIndex],
         1, &m_descriptorLayout);
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
      {GetSampler(m_wrap[0], m_smooth[0]), first->view,
         vk::ImageLayout::eShaderReadOnlyOptimal},
      {GetSampler(m_wrap[1], m_smooth[1]), second->view,
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
      m_pipelineLayout, 0, descriptor, {});
   const vk::Extent2D extent = m_context->SwapchainExtent();
   const int viewportWidth = m_viewportWidth > 0 ? m_viewportWidth :
      static_cast<int>(extent.width);
   const int viewportHeight = m_viewportHeight > 0 ? m_viewportHeight :
      static_cast<int>(extent.height);
   commandBuffer.setViewport(0, vk::Viewport(
      static_cast<float>(m_viewportX), static_cast<float>(m_viewportY),
      static_cast<float>(viewportWidth), static_cast<float>(viewportHeight),
      0.0f, 1.0f));
   commandBuffer.setScissor(0, vk::Rect2D(
      {std::max(m_viewportX, 0), std::max(m_viewportY, 0)},
      {static_cast<uint32_t>(viewportWidth),
       static_cast<uint32_t>(viewportHeight)}));
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
   commandBuffer.pushConstants(m_pipelineLayout,
      vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
      0, sizeof(constants), &constants);
   commandBuffer.bindVertexBuffers(0, frame.vertexBuffer, frame.vertexOffset);
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

void cVulkanSceneRenderer::ClearCanvasRegion(vk::CommandBuffer commandBuffer,
   int x, int y, int width, int height, int logicalWidth, int logicalHeight)
{
   if (!m_context || !commandBuffer || width <= 0 || height <= 0 ||
       logicalWidth <= 0 || logicalHeight <= 0)
      return;
   const int x1 = std::clamp(x, 0, logicalWidth);
   const int y1 = std::clamp(y, 0, logicalHeight);
   const int x2 = std::clamp(x + width, 0, logicalWidth);
   const int y2 = std::clamp(y + height, 0, logicalHeight);
   if (x2 <= x1 || y2 <= y1) return;
   const vk::Extent2D extent = m_context->SwapchainExtent();
   const int viewWidth = m_viewportWidth > 0 ? m_viewportWidth :
      static_cast<int>(extent.width);
   const int viewHeight = m_viewportHeight > 0 ? m_viewportHeight :
      static_cast<int>(extent.height);
   const int left = m_viewportX + x1 * viewWidth / logicalWidth;
   const int top = m_viewportY + y1 * viewHeight / logicalHeight;
   const int right = m_viewportX + x2 * viewWidth / logicalWidth;
   const int bottom = m_viewportY + y2 * viewHeight / logicalHeight;
   vk::ClearAttachment attachment{};
   attachment.aspectMask = vk::ImageAspectFlagBits::eColor;
   attachment.colorAttachment = 0;
   attachment.clearValue.color = vk::ClearColorValue(
      std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
   vk::ClearRect rect{};
   rect.rect = vk::Rect2D({std::max(left, 0), std::max(top, 0)},
      {static_cast<uint32_t>(std::max(right - left, 0)),
       static_cast<uint32_t>(std::max(bottom - top, 0))});
   rect.baseArrayLayer = 0;
   rect.layerCount = 1;
   if (rect.rect.extent.width && rect.rect.extent.height)
      commandBuffer.clearAttachments(attachment, rect);
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
   if (!m_context || width <= 0 || height <= 0 || !rgba || rowBytes < width * 4)
      return false;
   const vk::Device device = m_context->Device();
   std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);
   for (int y = 0; y < height; ++y)
      std::memcpy(pixels.data() + static_cast<size_t>(y) * width * 4,
         static_cast<const uint8_t *>(rgba) + static_cast<size_t>(y) * rowBytes,
         static_cast<size_t>(width) * 4);
   vk::Buffer staging{};
   vk::DeviceMemory stagingMemory{};
   try
   {
      const vk::DeviceSize bytes = pixels.size();
      staging = device.createBuffer(vk::BufferCreateInfo({}, bytes,
         vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive));
      auto req = device.getBufferMemoryRequirements(staging);
      stagingMemory = device.allocateMemory(vk::MemoryAllocateInfo(req.size,
         FindMemoryType(m_context->PhysicalDevice(), req.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent)));
      device.bindBufferMemory(staging, stagingMemory, 0);
      void *mapped = device.mapMemory(stagingMemory, 0, bytes);
      std::memcpy(mapped, pixels.data(), pixels.size());
      device.unmapMemory(stagingMemory);

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
      texture.image = device.createImage(imageInfo);
      req = device.getImageMemoryRequirements(texture.image);
      texture.memory = device.allocateMemory(vk::MemoryAllocateInfo(req.size,
         FindMemoryType(m_context->PhysicalDevice(), req.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal)));
      device.bindImageMemory(texture.image, texture.memory, 0);
      const bool copied = m_context->SubmitImmediate([&](vk::CommandBuffer command) {
         vk::ImageMemoryBarrier toTransfer{};
         toTransfer.oldLayout = vk::ImageLayout::eUndefined;
         toTransfer.newLayout = vk::ImageLayout::eTransferDstOptimal;
         toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
         toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
         toTransfer.image = texture.image;
         toTransfer.subresourceRange = vk::ImageSubresourceRange(
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
         toTransfer.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
         command.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, toTransfer);
         vk::BufferImageCopy region{};
         region.imageSubresource = vk::ImageSubresourceLayers(
            vk::ImageAspectFlagBits::eColor, 0, 0, 1);
         region.imageExtent = vk::Extent3D(width, height, 1);
         command.copyBufferToImage(staging, texture.image,
            vk::ImageLayout::eTransferDstOptimal, region);
         vk::ImageMemoryBarrier toShader{};
         toShader.oldLayout = vk::ImageLayout::eTransferDstOptimal;
         toShader.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
         toShader.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
         toShader.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
         toShader.image = texture.image;
         toShader.subresourceRange = vk::ImageSubresourceRange(
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
         toShader.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
         toShader.dstAccessMask = vk::AccessFlagBits::eShaderRead;
         command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, toShader);
      });
      device.destroyBuffer(staging);
      device.freeMemory(stagingMemory);
      staging = nullptr;
      stagingMemory = nullptr;
      if (!copied)
         throw std::runtime_error("Vulkan texture upload failed");
      texture.view = device.createImageView(vk::ImageViewCreateInfo({},
         texture.image, vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Unorm,
         {}, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));
      texture.width = width;
      texture.height = height;
      return true;
   }
   catch (...)
   {
      if (staging) device.destroyBuffer(staging);
      if (stagingMemory) device.freeMemory(stagingMemory);
      DestroyTextureImage(texture);
      return false;
   }
}

void cVulkanSceneRenderer::DestroyTextureImage(sTexture &texture)
{
   if (!m_context) return;
   const vk::Device device = m_context->Device();
   if (texture.view) device.destroyImageView(texture.view);
   if (texture.image) device.destroyImage(texture.image);
   if (texture.memory) device.freeMemory(texture.memory);
   texture = {};
}

void *cVulkanSceneRenderer::CreateTexture(int width, int height,
   const void *rgba, int rowBytes)
{
   if (!m_context) return nullptr;
   auto texture = std::make_unique<sTexture>();
   if (!CreateTextureImage(*texture, width, height, rgba, rowBytes))
      return nullptr;
   return texture.release();
}

bool cVulkanSceneRenderer::UpdateTexture(void *handle, int width, int height,
   const void *rgba, int rowBytes)
{
   auto *texture = static_cast<sTexture *>(handle);
   if (!texture || !m_context || width <= 0 || height <= 0 || !rgba ||
       rowBytes < width * 4)
      return false;
   const vk::Device device = m_context->Device();
   // RenderBackendDraw records commands into the active application frame.
   // Keep the previous image alive when a texture changes mid-frame so draws
   // already recorded keep sampling the old contents, matching the legacy
   // backend's flush-before-update ordering.
   if (texture->width != width || texture->height != height ||
       m_context->FrameActive())
   {
      sTexture replacement;
      if (!CreateTextureImage(replacement, width, height, rgba, rowBytes))
         return false;
      if (m_context->FrameActive())
      {
         auto oldTexture = std::make_unique<sTexture>(*texture);
         m_deferredTextures[m_frameIndex].push_back(std::move(oldTexture));
      }
      else
      {
         (void)device.waitIdle();
         DestroyTextureImage(*texture);
      }
      *texture = replacement;
      return true;
   }

   std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);
   for (int y = 0; y < height; ++y)
      std::memcpy(pixels.data() + static_cast<size_t>(y) * width * 4,
         static_cast<const uint8_t *>(rgba) + static_cast<size_t>(y) * rowBytes,
         static_cast<size_t>(width) * 4);
   vk::Buffer staging{};
   vk::DeviceMemory stagingMemory{};
   try
   {
      const vk::DeviceSize bytes = pixels.size();
      staging = device.createBuffer(vk::BufferCreateInfo({}, bytes,
         vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive));
      const auto requirements = device.getBufferMemoryRequirements(staging);
      stagingMemory = device.allocateMemory(vk::MemoryAllocateInfo(
         requirements.size, FindMemoryType(m_context->PhysicalDevice(),
            requirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent)));
      device.bindBufferMemory(staging, stagingMemory, 0);
      void *mapped = device.mapMemory(stagingMemory, 0, bytes);
      std::memcpy(mapped, pixels.data(), pixels.size());
      device.unmapMemory(stagingMemory);
      const vk::Image image = texture->image;
      const bool uploaded = m_context->SubmitImmediate(
         [&](vk::CommandBuffer command) {
            vk::ImageMemoryBarrier toTransfer{};
            toTransfer.oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            toTransfer.newLayout = vk::ImageLayout::eTransferDstOptimal;
            toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toTransfer.image = image;
            toTransfer.subresourceRange = vk::ImageSubresourceRange(
               vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
            toTransfer.srcAccessMask = vk::AccessFlagBits::eShaderRead;
            toTransfer.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
            command.pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader,
               vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, toTransfer);
            vk::BufferImageCopy region{};
            region.imageSubresource = vk::ImageSubresourceLayers(
               vk::ImageAspectFlagBits::eColor, 0, 0, 1);
            region.imageExtent = vk::Extent3D(width, height, 1);
            command.copyBufferToImage(staging, image,
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
         });
      device.destroyBuffer(staging);
      device.freeMemory(stagingMemory);
      return uploaded;
   }
   catch (...)
   {
      if (staging) device.destroyBuffer(staging);
      if (stagingMemory) device.freeMemory(stagingMemory);
      return false;
   }
}

void cVulkanSceneRenderer::DestroyTexture(void *handle)
{
   auto *texture = static_cast<sTexture *>(handle);
   if (!texture || !m_context || texture == m_whiteTexture.get()) return;
   for (void *&bound : m_boundTextures)
      if (bound == texture) bound = m_whiteTexture.get();
   if (m_context->FrameActive())
      m_deferredTextures[m_frameIndex].emplace_back(texture);
   else
   {
      (void)m_context->Device().waitIdle();
      DestroyTextureImage(*texture);
      delete texture;
   }
}
