#include "vulkan_world_renderer.hpp"
#include "vulkan_world_geometry.hpp"
#include "vulkan_ray_scene.hpp"
#include "world_history.hpp"
#include "vulkan_rtxdi.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <cstring>
#include <fstream>
#include <limits>
#include <map>
#include <unordered_map>
#include <tracy/Tracy.hpp>
namespace dark {
namespace {
struct Buffer {
   vk::Device device;
   vk::UniqueDeviceMemory memory;
   vk::UniqueBuffer buffer;
   void *mapped{};
   size_t capacity{};
   ~Buffer() {
      if (mapped) device.unmapMemory(*memory);
   }
};
struct Image {
   vk::UniqueDeviceMemory memory;
   vk::UniqueImage image;
   vk::UniqueImageView view;
   std::unique_ptr<Buffer> staging;
   uint32_t width{}, height{};
   bool uploaded{};
   uint64_t allocationBytes{};
};
struct MaterialData {
   float color[4], emissionRoughness[4];
   uint32_t texture, opacity;
   float metallic, cutoff;
   uint32_t emissionUsesBaseTexture, doubleSided, pbrFlags, padding;
   uint32_t normalTexture,roughnessTexture,metallicTexture,occlusionTexture;
   uint32_t emissionTexture,ormTexture,padding2[2];
};
struct LightData {
   float positionRange[4], directionInner[4], colorIntensity[4], outerType[4], rayParameters[4];
};
struct CameraData {
   float matrix[16], positionExposure[4], ambientScaleSize[4];
   float depthPlane[4];
   uint32_t lightingCounts[4];
   float fogColorDistance[4];
};
struct DrawConstants {
   float model[16];
   uint32_t material, receivesFog, lightOffset, lightCount;
};
static_assert(sizeof(DrawConstants) == 80, "world draw shader ABI mismatch");
static_assert(sizeof(MaterialData) == 96 && sizeof(LightData) == 80 && sizeof(CameraData) == 144,
              "world shader ABI mismatch");
struct SurfaceDrawData {
   float model[16], previousModel[16];
   uint32_t metadata[4], identity[4];
};
struct SurfaceCameraData {
   float current[16], previous[16], eye[4], forward[4], previousEye[4], previousForward[4];
};
static_assert(sizeof(SurfaceDrawData) == 160 && sizeof(SurfaceCameraData) == 192, "surface shader ABI");
struct Frame {
   vk::CommandBuffer command;
   vk::UniqueQueryPool timing;
   bool timingRecorded = false;
   double gpuMilliseconds = -1;
   std::array<double, 2> lightingTransparencyGpuMilliseconds{{-1, -1}};
   std::array<double, 7> gpuPhases{{-1, -1, -1, -1, -1, -1, -1}};
   std::array<double, 3> directGpuPhases{{-1, -1, -1}};
   std::shared_ptr<Image> hdr, depth, viewDistance;
   vk::UniqueFramebuffer framebuffer;
   std::array<std::shared_ptr<Image>, 6> surface;
   std::shared_ptr<Image> surfaceDepth;
   vk::UniqueImageView linearDepthView;
   vk::UniqueFramebuffer surfaceFramebuffer;
   vk::UniqueDescriptorPool pool;
   vk::DescriptorSet tone, sceneSet, previousSceneSet, lighting;
   std::unique_ptr<Buffer> previousMaterials;
   std::array<std::unique_ptr<Buffer>, 8> buffers;
   std::vector<std::shared_ptr<Image>> textures, previousTextures, uploads;
   std::array<double, 4> recordCpuMilliseconds{};
   VulkanWorldRenderer::TextureWorkload textureWorkload;
   float exposure{1};
   uint32_t width{}, height{};
   bool nativeDepth = false;
   std::array<float, 3> depthEncoding{};
};
std::vector<uint32_t> readShader(const std::string &path) {
   std::ifstream file(path, std::ios::binary | std::ios::ate);
   if (!file) throw std::runtime_error("missing shader " + path);
   const auto end = file.tellg();
   if (end <= 0 || size_t(end) % 4) throw std::runtime_error("invalid shader " + path);
   std::vector<uint32_t> data(size_t(end) / 4);
   file.seekg(0);
   file.read(reinterpret_cast<char *>(data.data()), end);
   if (!file) throw std::runtime_error("could not read shader " + path);
   return data;
}
} // namespace
struct VulkanWorldRenderer::Impl {
   vk::Device device;
   vk::PhysicalDevice physical;
   vk::PhysicalDeviceProperties properties;
   vk::PhysicalDeviceMemoryProperties memory;
   vk::Format depthFormat{}, outputFormat{};
   std::string shaderDirectory;
   uint32_t maxTextures{};
   uint32_t timestampBits = 0;
   vk::UniqueDescriptorSetLayout toneLayout;
   vk::UniqueDescriptorSetLayout sceneLayout;
   vk::UniquePipelineLayout tonePipelineLayout;
   vk::UniquePipelineLayout transparentPipelineLayout;
   vk::UniquePipeline tonePipeline, toneDepthPipeline;
   vk::UniquePipeline transparentPipeline;
   vk::UniquePipeline transparentNativePipeline;
   vk::UniqueSampler sampler;
   vk::UniqueSampler pointSampler;
   VulkanRayScene rayScene;
   RaySettings raySettings;
   WorldHistory history;
   vk::UniqueDescriptorSetLayout lightingLayout;
   vk::UniquePipelineLayout lightingPipelineLayout;
   vk::UniqueRenderPass lightingPass, lightingNativePass;
   vk::UniquePipeline lightingPipeline, lightingNativePipeline;
   VulkanRtxdi directLighting;
   uint32_t directWidth = 0, directHeight = 0, directCacheEntries = 0;
   vk::UniqueRenderPass surfacePass;
   vk::UniquePipelineLayout surfaceLayout;
   vk::UniquePipeline surfacePipeline;
   VulkanWorldGeometry rasterGeometry;
   vk::UniqueCommandPool commands;
   std::array<Frame, 2> frames;
   struct CachedTexture {
      uint64_t revision, lastUsed;
      std::shared_ptr<Image> image;
      const WorldScene *scene;
   };
   std::unordered_map<uint64_t, CachedTexture> textures;
   uint64_t textureSerial = 0, textureBudget = 64ull * 1024 * 1024;
   const WorldScene *materialScene = nullptr;
   uint64_t materialRevision = 0;
   std::vector<MaterialData> packedMaterials;
   std::vector<MaterialData> acceptedMaterials;
   std::vector<std::shared_ptr<Image>> acceptedTextures;
   int pendingSlot = -1;
   std::vector<uint64_t> packedTextureIds;
   std::unordered_map<uint64_t, uint32_t> packedMaterialIndices;
   const WorldScene *receiverScene = nullptr;
   uint64_t receiverRevision = 0;
   std::vector<RenderSceneHandle> receiverGpuLights;
   std::vector<uint32_t> receiverIndices;
   std::unordered_map<RenderSceneHandle, std::array<uint32_t, 2>> receiverRanges;
   uint32_t memoryType(uint32_t bits, vk::MemoryPropertyFlags wanted) {
      for (uint32_t i = 0; i < memory.memoryTypeCount; ++i)
         if ((bits & (1u << i)) && (memory.memoryTypes[i].propertyFlags & wanted) == wanted) return i;
      throw std::runtime_error("no memory type for world renderer");
   }
   std::unique_ptr<Buffer> buffer(size_t bytes, vk::BufferUsageFlags usage, bool host = true) {
      auto b = std::make_unique<Buffer>();
      b->device = device;
      b->capacity = std::max<size_t>(4096, bytes);
      usage |= vk::BufferUsageFlagBits::eTransferSrc;
      b->buffer = device.createBufferUnique(vk::BufferCreateInfo({}, b->capacity, usage, vk::SharingMode::eExclusive));
      auto r = device.getBufferMemoryRequirements(*b->buffer);
      const auto wanted = host ? (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
                               : vk::MemoryPropertyFlagBits::eDeviceLocal;
      b->memory = device.allocateMemoryUnique(vk::MemoryAllocateInfo(r.size, memoryType(r.memoryTypeBits, wanted)));
      device.bindBufferMemory(*b->buffer, *b->memory, 0);
      if (host) b->mapped = device.mapMemory(*b->memory, 0, VK_WHOLE_SIZE);
      return b;
   }
   std::shared_ptr<Image> image(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage,
                                vk::ImageAspectFlags aspect) {
      auto i = std::make_shared<Image>();
      i->width = width;
      i->height = height;
      vk::ImageCreateInfo info({}, vk::ImageType::e2D, format, {width, height, 1}, 1, 1, vk::SampleCountFlagBits::e1,
                               vk::ImageTiling::eOptimal, usage, vk::SharingMode::eExclusive);
      i->image = device.createImageUnique(info);
      auto r = device.getImageMemoryRequirements(*i->image);
      i->allocationBytes = r.size;
      i->memory = device.allocateMemoryUnique(
          vk::MemoryAllocateInfo(r.size, memoryType(r.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)));
      device.bindImageMemory(*i->image, *i->memory, 0);
      i->view = device.createImageViewUnique(vk::ImageViewCreateInfo({}, *i->image, vk::ImageViewType::e2D, format, {},
                                                                     vk::ImageSubresourceRange(aspect, 0, 1, 0, 1)));
      return i;
   }
   std::shared_ptr<Image> texture(uint64_t handle, const WorldScene &scene, vk::CommandBuffer command, Frame &frame) {
      const auto *source = handle ? &scene.textures.at(handle) : nullptr;
      const uint64_t revision = source ? source->revision : 1;
      auto cached = textures.find(handle);
      std::shared_ptr<Image> result;
      if (cached != textures.end() && cached->second.revision == revision &&
          (!handle || cached->second.scene == &scene)) {
         result = cached->second.image;
         cached->second.lastUsed = textureSerial;
      }
      if (!result) {
         const uint32_t width = source ? source->width : 1, height = source ? source->height : 1;
         ++frame.textureWorkload.createdImages;
         result = image(width, height, source && source->linearData?vk::Format::eR8G8B8A8Unorm:vk::Format::eR8G8B8A8Srgb,
                        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                        vk::ImageAspectFlagBits::eColor);
         ++frame.textureWorkload.createdStagingBuffers;
         result->staging = buffer(size_t(width) * height * 4, vk::BufferUsageFlagBits::eTransferSrc);
         const uint8_t white[] = {255, 255, 255, 255};
         std::memcpy(result->staging->mapped, source ? source->rgba.data() : white, size_t(width) * height * 4);
         textures[handle] = {revision, textureSerial, result, &scene};
      }
      if (!result->uploaded) {
         vk::ImageMemoryBarrier barrier({}, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eUndefined,
                                        vk::ImageLayout::eTransferDstOptimal, VK_QUEUE_FAMILY_IGNORED,
                                        VK_QUEUE_FAMILY_IGNORED, *result->image,
                                        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
         command.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {},
                                 {}, barrier);
         vk::BufferImageCopy copy(0, 0, 0, {vk::ImageAspectFlagBits::eColor, 0, 0, 1}, {0, 0, 0},
                                  {result->width, result->height, 1});
         command.copyBufferToImage(*result->staging->buffer, *result->image, vk::ImageLayout::eTransferDstOptimal,
                                   copy);
         barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
         barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
         barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
         barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
         command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {},
                                 {}, {}, barrier);
         result->uploaded = true;
         frame.uploads.push_back(result);
         ++frame.textureWorkload.uploads;
      }
      return result;
   }
   void trimTextures(Frame &frame) {
      uint64_t bytes = 0;
      for (const auto &entry : textures)
         bytes += entry.second.image->allocationBytes;
      if (bytes > textureBudget) {
         std::vector<std::pair<uint64_t, uint64_t>> candidates;
         for (const auto &entry : textures)
            if (entry.second.image.use_count() == 1) candidates.emplace_back(entry.second.lastUsed, entry.first);
         std::sort(candidates.begin(), candidates.end());
         for (const auto &candidate : candidates) {
            if (bytes <= textureBudget) break;
            const auto found = textures.find(candidate.second);
            bytes -= found->second.image->allocationBytes;
            textures.erase(found);
            ++frame.textureWorkload.evictedImages;
         }
      }
      // Images referenced by either frame are required and can exceed the
      // retention budget. Eviction never destroys a submitted frame's resources.
      frame.textureWorkload.cachedBytes = bytes;
   }
   vk::UniquePipeline pipeline(vk::RenderPass pass, vk::PipelineLayout layout, const std::vector<uint32_t> &vertex,
                               const std::vector<uint32_t> &fragment, bool world, bool blend, bool bridgeDepth = false,
                               uint32_t colorCount = 1) {
      auto vs = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, vertex.size() * 4, vertex.data()));
      auto fs = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, fragment.size() * 4, fragment.data()));
      std::array<vk::PipelineShaderStageCreateInfo, 2> stages{
          {{{}, vk::ShaderStageFlagBits::eVertex, *vs, "main"}, {{}, vk::ShaderStageFlagBits::eFragment, *fs, "main"}}};
      vk::VertexInputBindingDescription binding(0, sizeof(RenderSceneVertex), vk::VertexInputRate::eVertex);
      std::array<vk::VertexInputAttributeDescription, 3> attributes{{{0, 0, vk::Format::eR32G32B32Sfloat, 0},
                                                                     {1, 0, vk::Format::eR32G32B32Sfloat, 12},
                                                                     {2, 0, vk::Format::eR32G32Sfloat, 24}}};
      vk::PipelineVertexInputStateCreateInfo input;
      if (world) {
         input.vertexBindingDescriptionCount = 1;
         input.pVertexBindingDescriptions = &binding;
         input.vertexAttributeDescriptionCount = 3;
         input.pVertexAttributeDescriptions = attributes.data();
      }
      vk::PipelineInputAssemblyStateCreateInfo assembly({}, vk::PrimitiveTopology::eTriangleList);
      vk::PipelineViewportStateCreateInfo viewport({}, 1, nullptr, 1, nullptr);
      vk::PipelineRasterizationStateCreateInfo raster({}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill,
                                                      vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise,
                                                      VK_FALSE, 0, 0, 0, 1);
      vk::PipelineMultisampleStateCreateInfo ms({}, vk::SampleCountFlagBits::e1);
      vk::PipelineDepthStencilStateCreateInfo depth({}, world || bridgeDepth, bridgeDepth || (world && !blend),
                                                    bridgeDepth ? vk::CompareOp::eAlways : vk::CompareOp::eLessOrEqual);
      vk::PipelineColorBlendAttachmentState attachment;
      attachment.blendEnable = blend;
      attachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
      attachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
      attachment.colorBlendOp = vk::BlendOp::eAdd;
      attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
      attachment.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
      attachment.alphaBlendOp = vk::BlendOp::eAdd;
      attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                  vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
      const std::vector<vk::PipelineColorBlendAttachmentState> blendAttachments(colorCount, attachment);
      vk::PipelineColorBlendStateCreateInfo blending({}, VK_FALSE, vk::LogicOp::eCopy, colorCount,
                                                     blendAttachments.data());
      std::array<vk::DynamicState, 2> dynamicStates{{vk::DynamicState::eViewport, vk::DynamicState::eScissor}};
      vk::PipelineDynamicStateCreateInfo dynamic({}, dynamicStates);
      vk::GraphicsPipelineCreateInfo info({}, stages, &input, &assembly, nullptr, &viewport, &raster, &ms, &depth,
                                          &blending, &dynamic, layout, pass);
      return device.createGraphicsPipelineUnique({}, info).value;
   }
   void targets(Frame &frame, uint32_t width, uint32_t height, bool nativeDepth) {
      if (frame.width == width && frame.height == height && frame.nativeDepth == nativeDepth) return;
      if (nativeDepth && !lightingNativePass)
         throw std::runtime_error("native depth bridge requires blendable/sampleable R32_SFLOAT");
      if (nativeDepth && !transparentNativePipeline) {
         transparentNativePipeline =
             pipeline(*lightingNativePass, *transparentPipelineLayout, readShader(shaderDirectory + "/world.vert.spv"),
                      readShader(shaderDirectory + "/world_transparent_native.frag.spv"), true, true, false, 2);
      }

      // Build the complete target set before publishing any ownership changes.
      auto hdr = image(width, height, vk::Format::eR16G16B16A16Sfloat,
                       vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
                           vk::ImageUsageFlagBits::eTransferSrc,
                       vk::ImageAspectFlagBits::eColor);
      std::shared_ptr<Image> depth, distance, surfaceDepth;
      std::array<std::shared_ptr<Image>, 6> surfaces;
      vk::UniqueFramebuffer surfaceFramebuffer;
      vk::UniqueImageView linearDepthView;

      const std::array<vk::Format, 6> formats{{vk::Format::eR32G32B32A32Sfloat, vk::Format::eA2B10G10R10UnormPack32,
                                               vk::Format::eR16G16B16A16Sfloat, vk::Format::eR16G16B16A16Sfloat,
                                               vk::Format::eR32G32B32A32Sfloat, vk::Format::eR32G32B32A32Uint}};
      std::vector<vk::ImageView> surfaceViews;
      for (unsigned i = 0; i < formats.size(); ++i) {
         surfaces[i] = image(width, height, formats[i],
                             vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
                                 vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc,
                             vk::ImageAspectFlagBits::eColor);
         surfaceViews.push_back(*surfaces[i]->view);
      }
      surfaceDepth = image(width, height, depthFormat, vk::ImageUsageFlagBits::eDepthStencilAttachment,
                           vk::ImageAspectFlagBits::eDepth);
      surfaceViews.push_back(*surfaceDepth->view);
      surfaceFramebuffer =
          device.createFramebufferUnique(vk::FramebufferCreateInfo({}, *surfacePass, surfaceViews, width, height, 1));
      vk::ComponentMapping mapping(vk::ComponentSwizzle::eA, vk::ComponentSwizzle::eZero, vk::ComponentSwizzle::eZero,
                                   vk::ComponentSwizzle::eOne);
      linearDepthView = device.createImageViewUnique(
          vk::ImageViewCreateInfo({}, *surfaces[0]->image, vk::ImageViewType::e2D, formats[0], mapping,
                                  {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}));

      depth = surfaceDepth;
      std::vector<vk::ImageView> views{*hdr->view, *depth->view};
      if (nativeDepth) {
         distance = image(width, height, vk::Format::eR32Sfloat,
                          vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
                              vk::ImageUsageFlagBits::eTransferSrc,
                          vk::ImageAspectFlagBits::eColor);
         views.push_back(*distance->view);
      }
      vk::RenderPass pass = nativeDepth ? *lightingNativePass : *lightingPass;
      auto framebuffer = device.createFramebufferUnique(vk::FramebufferCreateInfo({}, pass, views, width, height, 1));
      frame.framebuffer = std::move(framebuffer);
      frame.hdr = std::move(hdr);
      frame.depth = std::move(depth);
      frame.viewDistance = std::move(distance);
      frame.surfaceFramebuffer = std::move(surfaceFramebuffer);
      frame.linearDepthView = std::move(linearDepthView);
      frame.surface = std::move(surfaces);
      frame.surfaceDepth = std::move(surfaceDepth);
      frame.nativeDepth = nativeDepth;
      frame.width = width;
      frame.height = height;
   }
   template <class T>
   vk::DescriptorBufferInfo upload(Frame &frame, size_t index, const T *data, size_t count,
                                   vk::BufferUsageFlags usage) {
      const size_t bytes = std::max<size_t>(sizeof(T) * count, 16);
      if ((usage & vk::BufferUsageFlagBits::eStorageBuffer) && bytes > properties.limits.maxStorageBufferRange)
         throw std::runtime_error("world buffer exceeds device storage limit");
      auto &b = frame.buffers[index];
      if (!b || b->capacity < bytes) b = buffer(bytes, usage);
      if (count) std::memcpy(b->mapped, data, sizeof(T) * count);
      return {*b->buffer, 0, bytes};
   }
};
VulkanWorldRenderer::VulkanWorldRenderer() : m(std::make_unique<Impl>()) {
}
VulkanWorldRenderer::~VulkanWorldRenderer() = default;
VulkanWorldRenderer::Workload VulkanWorldRenderer::GetWorkload(uint32_t slot) const {
   const auto rt = m->rayScene.Stats(slot);
   const auto &frame = m->frames.at(slot);
   return {rt.blasBuilds,
           rt.tlasBuilds,
           rt.instances,
           rt.bytes,
           frame.gpuMilliseconds,
           frame.recordCpuMilliseconds,
           frame.lightingTransparencyGpuMilliseconds,
           frame.gpuPhases,
           frame.directGpuPhases};
}
VulkanWorldRenderer::TextureWorkload VulkanWorldRenderer::GetTextureWorkload(uint32_t slot) const {
   return m->frames.at(slot).textureWorkload;
}
bool VulkanWorldRenderer::RayTracingEnabled() const {
   return bool(m->device);
}
void VulkanWorldRenderer::SetRaySettings(const RaySettings &settings) {
   m->raySettings = settings;
   m->raySettings.sharcEntries = std::clamp(settings.sharcEntries, 1u << 16, 1u << 22);
   uint32_t capacity = 1u << 16;
   while (capacity * 2 <= m->raySettings.sharcEntries)
      capacity *= 2;
   m->raySettings.sharcEntries = capacity;
   m->raySettings.sharcDownscale = std::clamp(settings.sharcDownscale, 1u, 10u);
   m->raySettings.sharcAccumulation = std::clamp(settings.sharcAccumulation, 1u, 100u);
   m->raySettings.sharcStaleFrames = std::clamp(settings.sharcStaleFrames, 8u, 100u);
   m->raySettings.denoiserFrameTimeMilliseconds = std::isfinite(settings.denoiserFrameTimeMilliseconds)
      ? std::clamp(settings.denoiserFrameTimeMilliseconds, 0.f, 1000.f) : 0.f;
   m->raySettings.sharcSceneScale =
       std::isfinite(settings.sharcSceneScale) ? std::clamp(settings.sharcSceneScale, 1.f, 100.f) : 50.f;
   m->raySettings.giStrength = std::isfinite(settings.giStrength) ? std::clamp(settings.giStrength, 0.0f, 4.0f) : 1.0f;
}
void VulkanWorldRenderer::SetTextureCacheBudget(uint64_t bytes) {
   m->textureBudget = bytes;
}
void VulkanWorldRenderer::CompleteSubmission(bool accepted) {
   m->history.CompleteSubmission(accepted);
   m->directLighting.CompleteSubmission(accepted);
   m->rayScene.CompleteSubmission(accepted);
   if (accepted && m->pendingSlot >= 0) {
      m->acceptedMaterials = m->packedMaterials;
      m->acceptedTextures = m->frames.at(m->pendingSlot).textures;
   } else if (!accepted) {
      m->acceptedMaterials.clear();
      m->acceptedTextures.clear();
   }
   m->pendingSlot = -1;
}
void VulkanWorldRenderer::Shutdown() {
   m = std::make_unique<Impl>();
}
bool VulkanWorldRenderer::Initialize(vk::PhysicalDevice physical, vk::Device device, uint32_t family,
                                     vk::RenderPass compositePass, vk::Format depthFormat, vk::Format outputFormat,
                                     const std::string &directory, std::string &error) {
   try {
      if (m->device) throw std::runtime_error("world renderer already initialized");
      m->device = device;
      m->physical = physical;
      m->properties = physical.getProperties();
      m->memory = physical.getMemoryProperties();
      m->timestampBits = physical.getQueueFamilyProperties().at(family).timestampValidBits;
      m->depthFormat = depthFormat;
      m->outputFormat = outputFormat;
      m->shaderDirectory = directory;
      const auto reserve = [](uint32_t limit, uint32_t used) { return limit > used ? limit - used : 0u; };
      // DI binds current and previous scene textures simultaneously.
      m->maxTextures = std::min({1024u, reserve(m->properties.limits.maxPerStageDescriptorSamplers, 12u) / 2,
                                 reserve(m->properties.limits.maxPerStageDescriptorSampledImages, 12u) / 2,
                                 reserve(m->properties.limits.maxDescriptorSetSamplers, 12u) / 2,
                                 reserve(m->properties.limits.maxDescriptorSetSampledImages, 12u) / 2,
                                 reserve(m->properties.limits.maxPerStageResources, 64u) / 2});
      if (m->maxTextures == 0) throw std::runtime_error("no texture descriptors available");
      const auto format = physical.getFormatProperties(vk::Format::eR16G16B16A16Sfloat);
      const auto needed = vk::FormatFeatureFlagBits::eColorAttachment | vk::FormatFeatureFlagBits::eSampledImage;
      if ((format.optimalTilingFeatures & needed) != needed)
         throw std::runtime_error("floating-point HDR targets unavailable");
      const auto graphicsCompute = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute;
      const auto allStages = graphicsCompute | vk::ShaderStageFlagBits::eVertex;
      const std::vector<vk::DescriptorSetLayoutBinding> mappedBindings{
          {2, vk::DescriptorType::eStorageBuffer, 1, graphicsCompute},
          {3, vk::DescriptorType::eStorageBuffer, 1, graphicsCompute},
          {4, vk::DescriptorType::eUniformBuffer, 1, allStages},
          {5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment},
          {6, vk::DescriptorType::eStorageBuffer, 1, graphicsCompute},
          {7, vk::DescriptorType::eCombinedImageSampler, m->maxTextures, graphicsCompute},
          {11, vk::DescriptorType::eAccelerationStructureKHR, 1, graphicsCompute},
          {12, vk::DescriptorType::eStorageBuffer, 1, graphicsCompute},
          {13, vk::DescriptorType::eStorageBuffer, 1, graphicsCompute},
          {17, vk::DescriptorType::eStorageBuffer, 1, allStages},
          {18, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex},
          {19, vk::DescriptorType::eUniformBuffer, 1,
           vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}};
      std::vector<vk::DescriptorBindingFlags> mappedFlags(mappedBindings.size());
      mappedFlags[5] = vk::DescriptorBindingFlagBits::ePartiallyBound;
      vk::DescriptorSetLayoutBindingFlagsCreateInfo mappedFlagInfo(mappedFlags);
      vk::DescriptorSetLayoutCreateInfo mappedInfo({}, mappedBindings);
      mappedInfo.pNext = &mappedFlagInfo;
      m->sceneLayout = device.createDescriptorSetLayoutUnique(mappedInfo);

      if (!m->rayScene.Initialize(physical, device, error)) throw std::runtime_error(error);

      if (!m->rasterGeometry.Initialize(physical, device))
         throw std::runtime_error("raster geometry initialization failed");
      std::array<vk::DescriptorSetLayoutBinding, 2> hdrBindings{
          {{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
           {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};
      m->toneLayout = device.createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo({}, hdrBindings));
      vk::PushConstantRange draw(vk::ShaderStageFlagBits::eVertex, 0, sizeof(DrawConstants));
      m->transparentPipelineLayout =
          device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, 1, &*m->sceneLayout, 1, &draw));
      vk::PushConstantRange exposure(vk::ShaderStageFlagBits::eFragment, 0, 24);
      m->tonePipelineLayout =
          device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, 1, &*m->toneLayout, 1, &exposure));
      std::array<vk::AttachmentDescription, 2> attachments;
      attachments[0] = {{},
                        vk::Format::eR16G16B16A16Sfloat,
                        vk::SampleCountFlagBits::e1,
                        vk::AttachmentLoadOp::eClear,
                        vk::AttachmentStoreOp::eStore,
                        vk::AttachmentLoadOp::eDontCare,
                        vk::AttachmentStoreOp::eDontCare,
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eShaderReadOnlyOptimal};
      attachments[1] = {{},
                        depthFormat,
                        vk::SampleCountFlagBits::e1,
                        vk::AttachmentLoadOp::eClear,
                        vk::AttachmentStoreOp::eDontCare,
                        vk::AttachmentLoadOp::eDontCare,
                        vk::AttachmentStoreOp::eDontCare,
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eDepthStencilAttachmentOptimal};
      vk::AttachmentReference color(0, vk::ImageLayout::eColorAttachmentOptimal),
          depth(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
      vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &color, nullptr, &depth);
      vk::SubpassDependency dependency(0, VK_SUBPASS_EXTERNAL, vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                       vk::PipelineStageFlagBits::eFragmentShader,
                                       vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead);
      const auto distanceFeatures = physical.getFormatProperties(vk::Format::eR32Sfloat).optimalTilingFeatures;
      const auto distanceNeeded = vk::FormatFeatureFlagBits::eColorAttachment |
                                  vk::FormatFeatureFlagBits::eColorAttachmentBlend |
                                  vk::FormatFeatureFlagBits::eSampledImage;

      // Primary visibility already wrote this depth image. Preserve it for
      // transparent forward draws instead of rasterizing opaque geometry again.
      auto loadAttachments = attachments;
      loadAttachments[1].loadOp = vk::AttachmentLoadOp::eLoad;
      loadAttachments[1].initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
      const std::array<vk::SubpassDependency, 2> dependencies{
          {{VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eAllCommands,
            vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eEarlyFragmentTests |
                vk::PipelineStageFlagBits::eLateFragmentTests | vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::AccessFlagBits::eMemoryWrite,
            vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eDepthStencilAttachmentRead |
                vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eColorAttachmentWrite},
           dependency}};
      m->lightingPass =
          device.createRenderPassUnique(vk::RenderPassCreateInfo({}, loadAttachments, subpass, dependencies));
      if ((distanceFeatures & distanceNeeded) == distanceNeeded) {
         const std::array<vk::AttachmentDescription, 3> descriptions{{loadAttachments[0],
                                                                      loadAttachments[1],
                                                                      {{},
                                                                       vk::Format::eR32Sfloat,
                                                                       vk::SampleCountFlagBits::e1,
                                                                       vk::AttachmentLoadOp::eClear,
                                                                       vk::AttachmentStoreOp::eStore,
                                                                       vk::AttachmentLoadOp::eDontCare,
                                                                       vk::AttachmentStoreOp::eDontCare,
                                                                       vk::ImageLayout::eUndefined,
                                                                       vk::ImageLayout::eShaderReadOnlyOptimal}}};
         const std::array<vk::AttachmentReference, 2> colors{{color, {2, vk::ImageLayout::eColorAttachmentOptimal}}};
         vk::SubpassDescription bridge({}, vk::PipelineBindPoint::eGraphics, 0, nullptr, colors.size(), colors.data(),
                                       nullptr, &depth);
         m->lightingNativePass =
             device.createRenderPassUnique(vk::RenderPassCreateInfo({}, descriptions, bridge, dependencies));
      }
      std::array<vk::DescriptorSetLayoutBinding, 6> bindings;
      for (uint32_t i = 0; i < 6; ++i)
         bindings[i] = {i, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment};
      m->lightingLayout = device.createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo({}, bindings));
      const std::array<vk::DescriptorSetLayout, 2> layouts{{*m->sceneLayout, *m->lightingLayout}};
      m->lightingPipelineLayout = device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, layouts));
      const auto vertex = readShader(directory + "/world_composite.vert.spv");
      m->lightingPipeline = m->pipeline(*m->lightingPass, *m->lightingPipelineLayout, vertex,
                                        readShader(directory + "/world_lighting.frag.spv"), false, false);
      if (m->lightingNativePass)
         m->lightingNativePipeline =
             m->pipeline(*m->lightingNativePass, *m->lightingPipelineLayout, vertex,
                         readShader(directory + "/world_lighting_native.frag.spv"), false, false, false, 2);

      {
         if (m->properties.limits.maxColorAttachments < 6)
            throw std::runtime_error("RTX surface pass requires six color attachments");
         const std::array<vk::Format, 6> formats{{vk::Format::eR32G32B32A32Sfloat, vk::Format::eA2B10G10R10UnormPack32,
                                                  vk::Format::eR16G16B16A16Sfloat, vk::Format::eR16G16B16A16Sfloat,
                                                  vk::Format::eR32G32B32A32Sfloat, vk::Format::eR32G32B32A32Uint}};
         std::vector<vk::AttachmentDescription> descriptions;
         std::vector<vk::AttachmentReference> references;
         for (uint32_t i = 0; i < formats.size(); ++i) {
            const auto supported = physical.getFormatProperties(formats[i]).optimalTilingFeatures;
            const auto required = vk::FormatFeatureFlagBits::eColorAttachment |
                                  vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eStorageImage;
            if ((supported & required) != required) throw std::runtime_error("RTX surface image format unsupported");
            descriptions.push_back({{},
                                    formats[i],
                                    vk::SampleCountFlagBits::e1,
                                    vk::AttachmentLoadOp::eClear,
                                    vk::AttachmentStoreOp::eStore,
                                    vk::AttachmentLoadOp::eDontCare,
                                    vk::AttachmentStoreOp::eDontCare,
                                    vk::ImageLayout::eUndefined,
                                    vk::ImageLayout::eGeneral});
            references.emplace_back(i, vk::ImageLayout::eColorAttachmentOptimal);
         }
         descriptions.push_back({{},
                                 depthFormat,
                                 vk::SampleCountFlagBits::e1,
                                 vk::AttachmentLoadOp::eClear,
                                 vk::AttachmentStoreOp::eStore,
                                 vk::AttachmentLoadOp::eDontCare,
                                 vk::AttachmentStoreOp::eDontCare,
                                 vk::ImageLayout::eUndefined,
                                 vk::ImageLayout::eDepthStencilAttachmentOptimal});
         const vk::AttachmentReference surfaceDepth(6, vk::ImageLayout::eDepthStencilAttachmentOptimal);
         vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, 0, nullptr, references.size(),
                                        references.data(), nullptr, &surfaceDepth);
         const std::array<vk::SubpassDependency, 2> dependencies{
             {{VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eAllCommands,
               vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
               vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite,
               vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite},
              {0, VK_SUBPASS_EXTERNAL, vk::PipelineStageFlagBits::eColorAttachmentOutput,
               vk::PipelineStageFlagBits::eComputeShader, vk::AccessFlagBits::eColorAttachmentWrite,
               vk::AccessFlagBits::eShaderRead}}};
         m->surfacePass =
             device.createRenderPassUnique(vk::RenderPassCreateInfo({}, descriptions, subpass, dependencies));
         m->surfaceLayout = device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, 1, &*m->sceneLayout));
         m->surfacePipeline =
             m->pipeline(*m->surfacePass, *m->surfaceLayout, readShader(directory + "/world_surface.vert.spv"),
                         readShader(directory + "/world_surface.frag.spv"), true, false, false, 6);
      }
      m->transparentPipeline =
          m->pipeline(*m->lightingPass, *m->transparentPipelineLayout, readShader(directory + "/world.vert.spv"),
                      readShader(directory + "/world_transparent.frag.spv"), true, true);

      m->tonePipeline =
          m->pipeline(compositePass, *m->tonePipelineLayout, readShader(directory + "/world_composite.vert.spv"),
                      readShader(directory + "/world_composite.frag.spv"), false, true);
      m->toneDepthPipeline =
          m->pipeline(compositePass, *m->tonePipelineLayout, readShader(directory + "/world_composite.vert.spv"),
                      readShader(directory + "/world_composite.frag.spv"), false, true, true);
      m->sampler = device.createSamplerUnique(vk::SamplerCreateInfo(
          {}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest,
          vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, 0,
          VK_FALSE, 1, VK_FALSE, vk::CompareOp::eNever, 0, 0));
      m->commands = device.createCommandPoolUnique(
          vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, family));
      m->pointSampler = device.createSamplerUnique(
          vk::SamplerCreateInfo({}, vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest,
                                vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge,
                                vk::SamplerAddressMode::eClampToEdge));
      auto commands = device.allocateCommandBuffers(
          vk::CommandBufferAllocateInfo(*m->commands, vk::CommandBufferLevel::ePrimary, 2));
      for (size_t i = 0; i < 2; ++i) {
         auto &frame = m->frames[i];
         frame.command = commands[i];
         if (m->timestampBits)
            frame.timing = device.createQueryPoolUnique(vk::QueryPoolCreateInfo({}, vk::QueryType::eTimestamp, 11));
         std::vector<vk::DescriptorPoolSize> sizes{{vk::DescriptorType::eStorageBuffer, 16u},
                                                   {vk::DescriptorType::eUniformBuffer, 4u},
                                                   {vk::DescriptorType::eCombinedImageSampler, 2 * m->maxTextures + 8}};
         sizes.push_back({vk::DescriptorType::eAccelerationStructureKHR, 2});
         frame.pool = device.createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo({}, 4, sizes));
         std::array<vk::DescriptorSetLayout, 3> layouts{{*m->toneLayout, *m->sceneLayout, *m->sceneLayout}};
         auto sets = device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo(*frame.pool, layouts));
         frame.tone = sets[0];
         frame.sceneSet = sets[1];
         frame.previousSceneSet = sets[2];
         frame.lighting =
             device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo(*frame.pool, 1, &*m->lightingLayout))[0];
      }
      return true;
   } catch (const std::exception &failure) {
      error = failure.what();
      Shutdown();
      return false;
   }
}
VkCommandBuffer VulkanWorldRenderer::Record(uint32_t slot, const WorldScene &scene, uint32_t width, uint32_t height,
                                            std::string &error) {
   ZoneScopedN("Record modern world lighting");
   try {
      if (!m->device || slot >= 2 || !scene.hasCamera || !width || !height) return VK_NULL_HANDLE;
      auto &frame = m->frames[slot];
      m->history.Prepare(scene, width, height);
      const auto recordStart = std::chrono::steady_clock::now();
      if (frame.timing && frame.timingRecorded) {
         uint64_t ticks[11]{};
         if (m->device.getQueryPoolResults(*frame.timing, 0, 11, sizeof(ticks), ticks, sizeof(uint64_t),
                                           vk::QueryResultFlagBits::e64) == vk::Result::eSuccess) {
            const uint64_t mask = m->timestampBits == 64 ? UINT64_MAX : ((uint64_t(1) << m->timestampBits) - 1);
            const auto milliseconds = [&](unsigned first, unsigned last) {
               return double((ticks[last] - ticks[first]) & mask) * m->properties.limits.timestampPeriod / 1000000;
            };
            frame.gpuMilliseconds = milliseconds(0, 3);
            frame.lightingTransparencyGpuMilliseconds = {milliseconds(1, 2), milliseconds(2, 3)};
            frame.gpuPhases = {milliseconds(1, 4), milliseconds(4, 5), milliseconds(5, 6), milliseconds(6, 7),
                               milliseconds(7, 8), milliseconds(8, 2), milliseconds(2, 3)};
            frame.directGpuPhases = {milliseconds(4, 9), milliseconds(9, 10), milliseconds(10, 5)};
         }
      }
      const auto planEnd = std::chrono::steady_clock::now();
      for (auto &image : frame.uploads)
         image->staging.reset();
      frame.uploads.clear();
      frame.textures.clear();
      frame.textureWorkload = {};
      for (auto it = m->textures.begin(); it != m->textures.end();)
         if (it->first && !scene.textures.count(it->first))
            it = m->textures.erase(it);
         else
            ++it;
      ++m->textureSerial;
      auto command = frame.command;
      command.reset();
      command.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
      if (frame.timing) {
         command.resetQueryPool(*frame.timing, 0, 11);
         command.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, *frame.timing, 0);
      }
      m->targets(frame, width, height, scene.camera.nativeDepth[3] != 0);
      std::copy(scene.camera.nativeDepth, scene.camera.nativeDepth + 3, frame.depthEncoding.begin());
      frame.exposure = scene.camera.exposure;
      if (m->materialScene != &scene || m->materialRevision != scene.materialsRevision) {
         std::vector<uint64_t> materialIds;
         for (const auto &entry : scene.materials)
            materialIds.push_back(entry.first);
         std::sort(materialIds.begin(), materialIds.end());
         std::unordered_map<uint64_t, uint32_t> materialIndices, textureIndices;
         std::vector<uint64_t> textureIds{0};
         textureIndices[0] = 0;
         std::vector<MaterialData> materials;
         for (const auto id : materialIds) {
            const auto &source = scene.materials.at(id);
            MaterialData data{};
            std::copy(source.baseColor, source.baseColor + 4, data.color);
            std::copy(source.emission, source.emission + 3, data.emissionRoughness);
            data.emissionRoughness[3] = source.roughness;
            auto found = textureIndices.find(source.baseColorTexture);
            if (found == textureIndices.end()) {
               const uint32_t index = uint32_t(textureIds.size());
               textureIds.push_back(source.baseColorTexture);
               found = textureIndices.emplace(source.baseColorTexture, index).first;
            }
            data.emissionUsesBaseTexture = source.emissionUsesBaseTexture;
            data.doubleSided = source.doubleSided;
            data.texture = found->second;
            data.opacity = source.opacity;
            data.metallic = source.metallic;
            data.cutoff = source.alphaCutoff;
            auto mapIndex=[&](uint64_t texture) {
               auto found=textureIndices.find(texture);
               if(found==textureIndices.end()) {
                  const auto index=uint32_t(textureIds.size());textureIds.push_back(texture);
                  found=textureIndices.emplace(texture,index).first;
               }
               return found->second;
            };
            data.normalTexture=mapIndex(source.normalTexture);
            data.roughnessTexture=mapIndex(source.roughnessTexture);
            data.metallicTexture=mapIndex(source.metallicTexture);
            data.occlusionTexture=mapIndex(source.occlusionTexture);
            data.emissionTexture=mapIndex(source.emissionTexture);
            data.ormTexture=mapIndex(source.ormTexture);
            data.pbrFlags=(source.normalTexture || source.roughnessTexture || source.metallicTexture ||
               source.occlusionTexture || source.emissionTexture || source.ormTexture)?1u:0u;
            materialIndices[id] = uint32_t(materials.size());
            materials.push_back(data);
         }
         if (textureIds.size() > m->maxTextures)
            throw std::runtime_error("world texture count exceeds device descriptor limit");
         m->packedMaterials = std::move(materials);
         m->packedTextureIds = std::move(textureIds);
         m->packedMaterialIndices = std::move(materialIndices);
         m->materialScene = &scene;
         m->materialRevision = scene.materialsRevision;
      }
      const auto &materials = m->packedMaterials;
      const auto &textureIds = m->packedTextureIds;
      const auto &materialIndices = m->packedMaterialIndices;
      for (const auto id : textureIds)
         frame.textures.push_back(m->texture(id, scene, command, frame));
      struct LightBounds {
         int x0, y0, x1, y1;
      };
      const uint32_t columns = (width + 15) / 16, rows = (height + 15) / 16;
      if (uint64_t(columns) * rows > UINT32_MAX) throw std::runtime_error("world target too large");
      std::vector<std::array<uint32_t, 2>> tiles(size_t(columns) * rows);
      std::unordered_map<RenderSceneHandle, uint32_t> gpuLights;
      std::vector<RenderSceneHandle> gpuLightHandles;
      std::vector<LightData> lights;
      std::vector<LightBounds> bounds;
      std::vector<uint32_t> globalLights;
      auto fullScreen = [&](const LightBounds &r) {
         return r.x0 == 0 && r.y0 == 0 && r.x1 == int(columns) - 1 && r.y1 == int(rows) - 1;
      };
      for (const auto &entry : scene.lights) {
         const auto &source = entry.second;
         if (!source.enabled || source.intensity == 0) continue;
         LightBounds rect{0, 0, int(columns) - 1, int(rows) - 1};
         bool global = source.type == RenderSceneDirectionalLight || source.range <= 0;
         float minX = INFINITY, minY = INFINITY, maxX = -INFINITY, maxY = -INFINITY;
         if (!global)
            for (int corner = 0; corner < 8; ++corner) {
               float point[4] = {source.position[0] + ((corner & 1) ? source.range : -source.range),
                                 source.position[1] + ((corner & 2) ? source.range : -source.range),
                                 source.position[2] + ((corner & 4) ? source.range : -source.range), 1};
               float clip[4]{};
               for (int r = 0; r < 4; ++r)
                  for (int c = 0; c < 4; ++c)
                     clip[r] += scene.camera.viewProjection[c * 4 + r] * point[c];
               if (clip[3] <= 0) {
                  global = true;
                  break;
               }
               minX = std::min(minX, clip[0] / clip[3]);
               maxX = std::max(maxX, clip[0] / clip[3]);
               minY = std::min(minY, clip[1] / clip[3]);
               maxY = std::max(maxY, clip[1] / clip[3]);
            }
         if (!global) {
            const bool outside = maxX < -1 || maxY < -1 || minX > 1 || minY > 1;
            auto tile = [](float ndc, uint32_t pixels, int max) {
               return std::clamp(int((std::clamp(ndc, -1.0f, 1.0f) + 1) * 0.5f * pixels) / 16, 0, max);
            };
            rect = outside ? LightBounds{0, 0, -1, -1}
                           : LightBounds{tile(minX, width, columns - 1), tile(minY, height, rows - 1),
                                         tile(maxX, width, columns - 1), tile(maxY, height, rows - 1)};
         }
         LightData light{};
         std::copy(source.position, source.position + 3, light.positionRange);
         light.positionRange[3] = source.range;
         std::copy(source.direction, source.direction + 3, light.directionInner);
         light.directionInner[3] = source.innerCos;
         std::copy(source.color, source.color + 3, light.colorIntensity);
         light.colorIntensity[3] = source.intensity;
         light.outerType[0] = source.outerCos;
         light.outerType[1] = float(source.type);
         light.outerType[2] = -1;

         light.outerType[2] = source.castsShadow ? 0.0f : -1.0f;
         light.outerType[3] = source.type == RenderSceneDirectionalLight ? source.angularRadius : source.sourceRadius;
         std::memcpy(light.rayParameters, &source.emitterObjectId, sizeof(source.emitterObjectId));

         gpuLights[entry.first] = uint32_t(lights.size());
         gpuLightHandles.push_back(entry.first);
         lights.push_back(light);
         bounds.push_back(rect);
         if (fullScreen(rect))
            globalLights.push_back(uint32_t(lights.size() - 1));
         else
            for (int y = rect.y0; y <= rect.y1; ++y)
               for (int x = rect.x0; x <= rect.x1; ++x)
                  ++tiles[size_t(y) * columns + x][1];
      }
      uint64_t total = globalLights.size();
      for (auto &tile : tiles) {
         tile[0] = uint32_t(total);
         total += tile[1];
         if (total > UINT32_MAX) throw std::runtime_error("world light list too large");
      }
      if (total * 4 > m->properties.limits.maxStorageBufferRange)
         throw std::runtime_error("world light lists exceed GPU buffer limit");
      std::vector<uint32_t> lightIndices(size_t(total), 0), cursor(tiles.size(), 0);
      std::copy(globalLights.begin(), globalLights.end(), lightIndices.begin());
      for (uint32_t i = 0; i < bounds.size(); ++i) {
         const auto &r = bounds[i];
         if (fullScreen(r)) continue;
         for (int y = r.y0; y <= r.y1; ++y)
            for (int x = r.x0; x <= r.x1; ++x) {
               const size_t tile = size_t(y) * columns + x;
               lightIndices[tiles[tile][0] + cursor[tile]++] = i;
            }
      }
      // Cache receiver packing independently of poses/materials. GPU light order
      // matters because disabling/removing a light remaps the remaining indices.
      if (m->receiverScene != &scene || m->receiverRevision != scene.instanceLightsRevision ||
          m->receiverGpuLights != gpuLightHandles) {
         std::vector<uint32_t> packedIndices;
         std::unordered_map<RenderSceneHandle, std::array<uint32_t, 2>> ranges;
         std::map<std::vector<RenderSceneHandle>, std::array<uint32_t, 2>> packedLists;
         for (const auto &entry : scene.instanceLights) {
            auto packed = packedLists.find(entry.second);
            if (packed == packedLists.end()) {
               const uint32_t first = uint32_t(packedIndices.size());
               for (const auto id : entry.second) {
                  const auto found = gpuLights.find(id);
                  if (found != gpuLights.end()) packedIndices.push_back(found->second);
               }
               // Reused RTXDI samples need membership checks at each receiver.
               // Sorted GPU indices make those checks logarithmic.
               std::sort(packedIndices.begin()+first,packedIndices.end());
               if (packedIndices.size() >= UINT32_MAX ||
                   uint64_t(packedIndices.size()) * 4 > m->properties.limits.maxStorageBufferRange)
                  throw std::runtime_error("receiver light lists exceed GPU buffer limit");
               packed =
                   packedLists
                       .emplace(entry.second, std::array<uint32_t, 2>{first, uint32_t(packedIndices.size()) - first})
                       .first;
            }
            ranges[entry.first] = packed->second;
         }
         m->receiverIndices = std::move(packedIndices);
         m->receiverRanges = std::move(ranges);
         m->receiverGpuLights = std::move(gpuLightHandles);
         m->receiverRevision = scene.instanceLightsRevision;
         m->receiverScene = &scene;
      }
      const uint64_t linkedEnd = uint64_t(lightIndices.size()) + m->receiverIndices.size();
      if (linkedEnd >= UINT32_MAX || linkedEnd * 4 > m->properties.limits.maxStorageBufferRange)
         throw std::runtime_error("combined light lists exceed GPU buffer limit");
      const uint32_t receiverBase = uint32_t(lightIndices.size());
      lightIndices.insert(lightIndices.end(), m->receiverIndices.begin(), m->receiverIndices.end());
      CameraData camera{};
      std::copy(scene.camera.viewProjection, scene.camera.viewProjection + 16, camera.matrix);
      std::copy(scene.camera.fogColor, scene.camera.fogColor + 3, camera.fogColorDistance);
      camera.fogColorDistance[3] = scene.camera.fogDistance;

      // Fog and native depth are camera properties, independent of shadow maps.
      double length = 0;
      for (unsigned i = 0; i < 3; ++i)
         length += double(scene.camera.viewProjection[i * 4 + 3]) * scene.camera.viewProjection[i * 4 + 3];
      if (length <= 0) throw std::runtime_error("invalid lighting camera depth axis");
      for (unsigned i = 0; i < 3; ++i) {
         camera.depthPlane[i] = scene.camera.viewProjection[i * 4 + 3] / float(std::sqrt(length));
         camera.depthPlane[3] -= camera.depthPlane[i] * scene.camera.position[i];
      }

      camera.lightingCounts[3] = uint32_t(globalLights.size());
      std::copy(scene.camera.position, scene.camera.position + 3, camera.positionExposure);
      camera.positionExposure[3] = frame.exposure;
      camera.ambientScaleSize[0] = scene.camera.ambient;
      camera.ambientScaleSize[1] = scene.camera.lightScale;
      camera.ambientScaleSize[2] = float(width);
      camera.ambientScaleSize[3] = float(height);
      const auto storage = vk::BufferUsageFlagBits::eStorageBuffer;
      std::array<vk::DescriptorBufferInfo, 5> buffers{
          {m->upload(frame, 0, materials.data(), materials.size(), storage),
           m->upload(frame, 1, lights.data(), lights.size(), storage),
           m->upload(frame, 2, &camera, 1, vk::BufferUsageFlagBits::eUniformBuffer),
           m->upload(frame, 3, tiles.data(), tiles.size(), storage),
           m->upload(frame, 4, lightIndices.data(), lightIndices.size(), storage)}};
      std::vector<vk::DescriptorImageInfo> images;
      for (const auto &texture : frame.textures)
         images.emplace_back(*m->sampler, *texture->view, vk::ImageLayout::eShaderReadOnlyOptimal);
      std::vector<vk::WriteDescriptorSet> writes;
      vk::DescriptorImageInfo hdr(*m->sampler, *frame.hdr->view, vk::ImageLayout::eShaderReadOnlyOptimal);
      writes.emplace_back(frame.tone, 0, 0, 1, vk::DescriptorType::eCombinedImageSampler, &hdr);
      const vk::DescriptorImageInfo bridgeDepth(*m->pointSampler,
                                                frame.nativeDepth ? *frame.viewDistance->view : *frame.hdr->view,
                                                vk::ImageLayout::eShaderReadOnlyOptimal);
      writes.emplace_back(frame.tone, 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &bridgeDepth);
      for (uint32_t i = 0; i < 5; ++i)
         writes.emplace_back(frame.sceneSet, i + 2, 0, 1,
                             i == 2 ? vk::DescriptorType::eUniformBuffer : vk::DescriptorType::eStorageBuffer, nullptr,
                             &buffers[i]);
      writes.emplace_back(frame.sceneSet, 7, 0, uint32_t(images.size()), vk::DescriptorType::eCombinedImageSampler,
                          images.data());
      vk::WriteDescriptorSetAccelerationStructureKHR acceleration;
      vk::AccelerationStructureKHR topLevel;
      vk::DescriptorBufferInfo rayTriangles, rayInstances;

      const auto rayRecordStart = std::chrono::steady_clock::now();
      if (!m->rayScene.Record(command, slot, scene, materialIndices, error)) throw std::runtime_error(error);
      const double rayRecordCpu =
          std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - rayRecordStart).count();
      topLevel = m->rayScene.TopLevel(slot);
      acceleration.accelerationStructureCount = 1;
      acceleration.pAccelerationStructures = &topLevel;
      vk::WriteDescriptorSet write(frame.sceneSet, 11, 0, 1, vk::DescriptorType::eAccelerationStructureKHR);
      write.pNext = &acceleration;
      writes.push_back(write);
      rayTriangles = m->rayScene.Triangles(slot);
      rayInstances = m->rayScene.Instances(slot);
      writes.emplace_back(frame.sceneSet, 12, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &rayTriangles);
      writes.emplace_back(frame.sceneSet, 13, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &rayInstances);

      const auto &previousMaterials = m->acceptedMaterials.empty() ? materials : m->acceptedMaterials;
      const auto previousMaterialBytes = previousMaterials.size() * sizeof(MaterialData);
      if (!frame.previousMaterials || frame.previousMaterials->capacity < previousMaterialBytes)
         frame.previousMaterials = m->buffer(previousMaterialBytes, storage);
      std::memcpy(frame.previousMaterials->mapped, previousMaterials.data(), previousMaterialBytes);
      const vk::DescriptorBufferInfo oldMaterials(*frame.previousMaterials->buffer, 0, previousMaterialBytes);
      frame.previousTextures = m->acceptedTextures.empty() ? frame.textures : m->acceptedTextures;
      std::vector<vk::DescriptorImageInfo> oldImages;
      for (const auto &texture : frame.previousTextures)
         oldImages.emplace_back(*m->sampler, *texture->view, vk::ImageLayout::eShaderReadOnlyOptimal);
      const vk::AccelerationStructureKHR oldTopLevel = m->rayScene.PreviousTopLevel(slot);
      const vk::WriteDescriptorSetAccelerationStructureKHR oldAcceleration(1, &oldTopLevel);
      vk::WriteDescriptorSet oldAs(frame.previousSceneSet, 11, 0, 1, vk::DescriptorType::eAccelerationStructureKHR);
      oldAs.pNext = &oldAcceleration;
      const auto oldTriangles = m->rayScene.PreviousTriangles(slot);
      const auto oldInstances = m->rayScene.PreviousInstances(slot);
      writes.push_back(oldAs);
      writes.emplace_back(frame.previousSceneSet, 2, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &oldMaterials);
      writes.emplace_back(frame.previousSceneSet, 7, 0, uint32_t(oldImages.size()), vk::DescriptorType::eCombinedImageSampler, oldImages.data());
      writes.emplace_back(frame.previousSceneSet, 12, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &oldTriangles);
      writes.emplace_back(frame.previousSceneSet, 13, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &oldInstances);

      if (!m->rasterGeometry.Update(slot, scene, error)) throw std::runtime_error(error);
      struct VisibleDraw {
         DrawConstants constants;
         double distance;
         RenderSceneHandle id;
         VkBuffer vertices, indices;
         uint32_t indexCount, firstIndex;
         int32_t vertexOffset;
      };
      std::vector<VisibleDraw> opaque, blended;
      const auto cameraFrustum = SceneExtractFrustum(scene.camera.viewProjection);
      for (const auto &entry : scene.instances) {
         const auto &instance = entry.second;
         if (!instance.visible) continue;
         const auto &mesh = scene.meshes.at(instance.mesh);
         const auto bounds = SceneTransformBounds(mesh.bounds, instance.transform);
         if (!SceneBoundsVisible(bounds, cameraFrustum)) continue;
         double distance = 0;
         for (unsigned axis = 0; axis < 3; ++axis) {
            const double delta = bounds.center[axis] - scene.camera.position[axis];
            distance += delta * delta;
         }
         VisibleDraw item{};
         item.distance = distance;
         item.id = entry.first;
         VkDeviceSize vertexOffset{}, indexOffset{};
         if (!m->rasterGeometry.MeshBuffers(slot, instance.mesh, item.vertices, item.indices, &vertexOffset,
                                            &indexOffset))
            throw std::runtime_error("missing raster mesh cache");
         if (vertexOffset / sizeof(RenderSceneVertex) > INT32_MAX || indexOffset / sizeof(uint32_t) > UINT32_MAX)
            throw std::runtime_error("geometry draw offset exceeds Vulkan draw limits");
         item.indexCount = uint32_t(mesh.indices.size());
         item.firstIndex = uint32_t(indexOffset / sizeof(uint32_t));
         item.vertexOffset = int32_t(vertexOffset / sizeof(RenderSceneVertex));
         std::copy(instance.transform, instance.transform + 16, item.constants.model);
         item.constants.material = materialIndices.at(mesh.material);
         item.constants.receivesFog = instance.receivesFog;
         const auto receiver = m->receiverRanges.find(entry.first);
         item.constants.lightOffset =
             receiver == m->receiverRanges.end() ? UINT32_MAX : receiverBase + receiver->second[0];
         item.constants.lightCount = receiver == m->receiverRanges.end() ? 0 : receiver->second[1];
         (scene.materials.at(mesh.material).opacity == RenderSceneBlended ? blended : opaque).push_back(item);
      }
      // Terrain instances have identity transforms; their mesh bounds provide
      // meaningful world positions for depth ordering, unlike instance origins.
      std::sort(opaque.begin(), opaque.end(), [](const auto &a, const auto &b) {
         return a.distance != b.distance ? a.distance < b.distance : a.id < b.id;
      });
      std::sort(blended.begin(), blended.end(), [](const auto &a, const auto &b) {
         return a.distance != b.distance ? a.distance > b.distance : a.id < b.id;
      });
      std::array<vk::DescriptorBufferInfo, 3> surfaceBuffers;

      std::vector<SurfaceDrawData> surfaceDraws;
      std::vector<std::array<float, 4>> previousPositions;
      std::unordered_map<RenderSceneHandle, uint32_t> positionOffsets;
      const auto *previous = m->history.Previous();
      for (const auto &item : opaque) {
         const auto &instance = scene.instances.at(item.id);
         const auto &mesh = scene.meshes.at(instance.mesh);
         bool valid = previous != nullptr;
         const RenderSceneInstance *oldInstance = nullptr;
         const std::vector<RenderSceneVertex> *oldVertices = &mesh.vertices;
         if (previous) {
            const auto old = previous->instances.find(item.id);
            const auto geometry = previous->meshes.find(instance.mesh);
            const bool previousGeometry =
                geometry != previous->meshes.end() && geometry->second.vertices->size() == mesh.vertices.size();
            // Shared meshes must use one previous vertex stream regardless of
            // whether the first visible instance is new or already existed.
            if (previousGeometry) oldVertices = geometry->second.vertices.get();
            valid = old != previous->instances.end() && old->second.visible && old->second.mesh == instance.mesh &&
                    previousGeometry;
            if (valid) oldInstance = &old->second;
            valid = valid && geometry->second.material == mesh.material &&
                    m->history.MaterialMatches(mesh.material, scene.materials.at(mesh.material));
         }
         auto offset = positionOffsets.find(instance.mesh);
         if (offset == positionOffsets.end()) {
            if (previousPositions.size() + mesh.vertices.size() > UINT32_MAX)
               throw std::runtime_error("previous surface vertices exceed indexing limit");
            offset = positionOffsets.emplace(instance.mesh, uint32_t(previousPositions.size())).first;
            for (const auto &vertex : *oldVertices)
               previousPositions.push_back({vertex.position[0], vertex.position[1], vertex.position[2], 1});
         }
         SurfaceDrawData data{};
         std::copy(instance.transform, instance.transform + 16, data.model);
         const auto *oldModel = oldInstance ? oldInstance->transform : instance.transform;
         std::copy(oldModel, oldModel + 16, data.previousModel);
         data.metadata[0] = item.constants.material;
         data.metadata[1] = offset->second;
         data.metadata[2] = uint32_t(item.vertexOffset);
         data.metadata[3] = (valid ? 1u : 0u) | (instance.receivesFog ? 2u : 0u);
         data.identity[0] = uint32_t(item.id);
         data.identity[1] = uint32_t(item.id >> 32);
         data.identity[2] = item.constants.lightOffset;
         data.identity[3] = item.constants.lightCount;
         surfaceDraws.push_back(data);
      }
      SurfaceCameraData surfaceCamera{};
      const auto &oldCamera = previous ? previous->camera : scene.camera;
      std::copy(scene.camera.viewProjection, scene.camera.viewProjection + 16, surfaceCamera.current);
      std::copy(oldCamera.viewProjection, oldCamera.viewProjection + 16, surfaceCamera.previous);
      std::copy(scene.camera.position, scene.camera.position + 3, surfaceCamera.eye);
      std::copy(oldCamera.position, oldCamera.position + 3, surfaceCamera.previousEye);
      auto forward = [](const auto &camera, float *output) {
         double length = 0;
         for (unsigned axis = 0; axis < 3; ++axis) {
            output[axis] = camera.viewProjection[axis * 4 + 3];
            length += double(output[axis]) * output[axis];
         }
         if (length <= 0) throw std::runtime_error("invalid surface camera forward");
         for (unsigned axis = 0; axis < 3; ++axis)
            output[axis] /= float(std::sqrt(length));
      };
      forward(scene.camera, surfaceCamera.forward);
      forward(oldCamera, surfaceCamera.previousForward);
      surfaceBuffers = {m->upload(frame, 5, surfaceDraws.data(), surfaceDraws.size(), storage),
                        m->upload(frame, 6, previousPositions.data(), previousPositions.size(), storage),
                        m->upload(frame, 7, &surfaceCamera, 1, vk::BufferUsageFlagBits::eUniformBuffer)};
      for (unsigned i = 0; i < 3; ++i)
         writes.emplace_back(frame.sceneSet, 17 + i, 0, 1,
                             i == 2 ? vk::DescriptorType::eUniformBuffer : vk::DescriptorType::eStorageBuffer, nullptr,
                             &surfaceBuffers[i]);

      m->device.updateDescriptorSets(writes, {});
      const auto uploadEnd = std::chrono::steady_clock::now();
      if (frame.timing) command.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, *frame.timing, 1);

      std::array<vk::ClearValue, 7> surfaceClears{};
      surfaceClears[0].color = vk::ClearColorValue(std::array<float, 4>{0, 0, 0, 1000000});
      surfaceClears[1].color = vk::ClearColorValue(std::array<float, 4>{.5f, .5f, 1, 0});
      surfaceClears[5].color = vk::ClearColorValue(std::array<uint32_t, 4>{});
      surfaceClears[6].depthStencil = vk::ClearDepthStencilValue(1, 0);
      command.beginRenderPass(
          vk::RenderPassBeginInfo(*m->surfacePass, *frame.surfaceFramebuffer, {{0, 0}, {width, height}}, surfaceClears),
          vk::SubpassContents::eInline);
      command.setViewport(0, vk::Viewport(0, 0, float(width), float(height), 0, 1));
      command.setScissor(0, vk::Rect2D({0, 0}, {width, height}));
      command.bindPipeline(vk::PipelineBindPoint::eGraphics, *m->surfacePipeline);
      command.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m->surfaceLayout, 0, frame.sceneSet, {});
      uint32_t surfaceIndex = 0;
      for (const auto &item : opaque) {
         const vk::Buffer buffer = item.vertices;
         const vk::DeviceSize offset = 0;
         command.bindVertexBuffers(0, 1, &buffer, &offset);
         command.bindIndexBuffer(item.indices, 0, vk::IndexType::eUint32);
         command.drawIndexed(item.indexCount, 1, item.firstIndex, item.vertexOffset, surfaceIndex++);
      }
      command.endRenderPass();

      {
         if (m->directWidth != width || m->directHeight != height ||
             m->directCacheEntries != m->raySettings.sharcEntries) {
            // Target-size changes are rare; retire all users before replacing
            // persistent temporal resources shared across frame slots.
            m->device.waitIdle();
            if (!m->directLighting.Initialize(m->physical, m->device, *m->sceneLayout, width, height,
                                              m->shaderDirectory, error, m->raySettings.sharcEntries))
               throw std::runtime_error(error);
            m->directWidth = width;
            m->directHeight = height;
            m->directCacheEntries = m->raySettings.sharcEntries;
         }
         VulkanRtxdi::Input input;
         input.previousSceneSet = frame.previousSceneSet;
         if (frame.timing) {
            input.timing = *frame.timing;
            input.timestampBase = 4;
            input.directTimestampBase = 9;
         }
         for (unsigned i = 0; i < 6; ++i)
            input.surface[i] = {*frame.surface[i]->image, *frame.surface[i]->view,
                                i == 0 || i == 4 ? vk::Format::eR32G32B32A32Sfloat
                                : i == 1         ? vk::Format::eA2B10G10R10UnormPack32
                                : i == 5         ? vk::Format::eR32G32B32A32Uint
                                                 : vk::Format::eR16G16B16A16Sfloat};
         input.lights = buffers[1];
         input.draws = surfaceBuffers[0];
         input.links = buffers[4];
         input.lightIds = &m->receiverGpuLights;
         input.collectStatistics = m->raySettings.sharcStatistics;
         input.cacheEnabled = m->raySettings.sharc;
         input.sharcSceneScale = m->raySettings.sharcSceneScale;
         input.sharcDownscale = m->raySettings.sharcDownscale;
         input.sharcAccumulation = m->raySettings.sharcAccumulation;
         input.sharcStaleFrames = m->raySettings.sharcStaleFrames;
         input.gi = m->raySettings.gi;
         input.giStrength = m->raySettings.giStrength;
         input.denoiserFrameTimeMilliseconds = m->raySettings.denoiserFrameTimeMilliseconds;
         input.materialRevision = scene.materialsRevision;
         input.lightsRevision = scene.lightsRevision;
         input.texturesRevision = scene.texturesRevision;
         const auto rayWork = m->rayScene.Stats(slot);
         input.geometryChanged = rayWork.blasBuilds != 0 || rayWork.tlasBuilds != 0;
         input.camera = scene.camera;
         input.linearDepth = *frame.linearDepthView;
         input.historyValid = m->history.Previous() != nullptr;
         std::copy(scene.camera.position, scene.camera.position + 3, input.eye);
         if (!m->directLighting.Record(command, slot, frame.sceneSet, input, error)) throw std::runtime_error(error);
         std::array<vk::DescriptorImageInfo, 6> images;
         for (unsigned i = 0; i < 4; ++i)
            images[i] = {*m->pointSampler, *frame.surface[i]->view, vk::ImageLayout::eGeneral};
         images[4] = {*m->pointSampler, m->directLighting.DenoisedDiffuse(slot).view, vk::ImageLayout::eGeneral};
         images[5] = {*m->pointSampler, m->directLighting.DenoisedSpecular(slot).view, vk::ImageLayout::eGeneral};
         std::array<vk::WriteDescriptorSet, 6> writes;
         for (unsigned i = 0; i < 6; ++i)
            writes[i] = {frame.lighting, i, 0, 1, vk::DescriptorType::eCombinedImageSampler, &images[i]};
         m->device.updateDescriptorSets(writes, {});
      }
      std::array<vk::ClearValue, 3> clears{};
      clears[0].color = vk::ClearColorValue(std::array<float, 4>{0, 0, 0, 0});
      clears[1].depthStencil = vk::ClearDepthStencilValue(1, 0);
      vk::RenderPass lightingPass = frame.nativeDepth ? *m->lightingNativePass : *m->lightingPass;
      command.beginRenderPass(vk::RenderPassBeginInfo(lightingPass, *frame.framebuffer, {{0, 0}, {width, height}},
                                                      frame.nativeDepth ? 3u : 2u, clears.data()),
                              vk::SubpassContents::eInline);
      command.setViewport(0, vk::Viewport(0, 0, float(width), float(height), 0, 1));
      command.setScissor(0, vk::Rect2D({0, 0}, {width, height}));
      command.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m->transparentPipelineLayout, 0, frame.sceneSet,
                                 {});
      VkBuffer boundVertices{}, boundIndices{};
      auto draw = [&](const auto &list, vk::Pipeline pipeline) {
         command.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
         for (const auto &item : list) {
            command.pushConstants(*m->transparentPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0,
                                  sizeof(item.constants), &item.constants);
            if (boundVertices != item.vertices) {
               const vk::Buffer buffer = item.vertices;
               const vk::DeviceSize offset = 0;
               command.bindVertexBuffers(0, 1, &buffer, &offset);
               boundVertices = item.vertices;
            }
            if (boundIndices != item.indices) {
               command.bindIndexBuffer(item.indices, 0, vk::IndexType::eUint32);
               boundIndices = item.indices;
            }
            command.drawIndexed(item.indexCount, 1, item.firstIndex, item.vertexOffset, 0);
         }
      };

      const std::array<vk::DescriptorSet, 2> sets{{frame.sceneSet, frame.lighting}};
      command.bindPipeline(vk::PipelineBindPoint::eGraphics,
                           frame.nativeDepth ? *m->lightingNativePipeline : *m->lightingPipeline);
      command.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m->lightingPipelineLayout, 0, sets, {});
      command.draw(3, 1, 0, 0);
      command.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m->transparentPipelineLayout, 0, frame.sceneSet,
                                 {});

      if (frame.timing) command.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, *frame.timing, 2);
      draw(blended, frame.nativeDepth ? *m->transparentNativePipeline : *m->transparentPipeline);
      command.endRenderPass();
      if (frame.timing) {
         command.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, *frame.timing, 3);
         frame.timingRecorded = true;
      }
      command.end();
      m->trimTextures(frame);
      const auto lightingEnd = std::chrono::steady_clock::now();
      frame.recordCpuMilliseconds = {
          std::chrono::duration<double, std::milli>(planEnd - recordStart).count(),
          std::chrono::duration<double, std::milli>(uploadEnd - planEnd).count() - rayRecordCpu, rayRecordCpu,
          std::chrono::duration<double, std::milli>(lightingEnd - uploadEnd).count()};
      m->pendingSlot = int(slot);
      return command;
   } catch (const std::exception &failure) {
      error = failure.what();
      m->history.CompleteSubmission(false);
      m->directLighting.CompleteSubmission(false);
      m->rayScene.CompleteSubmission(false);
      m->acceptedMaterials.clear();
      m->acceptedTextures.clear();
      m->pendingSlot = -1;
      // Recording may have populated caches whose GPU builds never submitted.
      // Discard those slot-local resources before allowing a retry.
      if (slot < m->frames.size()) {
         auto &frame = m->frames[slot];
         frame.command.reset();
         frame.timingRecorded = false;
         m->rayScene.Invalidate(slot);
         m->textures.clear();
      }
      return VK_NULL_HANDLE;
   }
}
VkImage VulkanWorldRenderer::NativeDepthImage(uint32_t slot) const {
   if (slot >= m->frames.size() || !m->frames[slot].viewDistance) return VK_NULL_HANDLE;
   return *m->frames[slot].viewDistance->image;
}
VkImage VulkanWorldRenderer::DirectImage(uint32_t slot, bool specular, bool denoised) const {
   if (m->directWidth && slot < 2) {
      if (denoised)
         return specular ? VkImage(m->directLighting.DenoisedSpecular(slot).image)
                         : VkImage(m->directLighting.DenoisedDiffuse(slot).image);
      return specular ? VkImage(m->directLighting.Specular(slot).image)
                      : VkImage(m->directLighting.Diffuse(slot).image);
   }
   return VK_NULL_HANDLE;
}
std::array<uint32_t, 4> VulkanWorldRenderer::IndirectStatistics(uint32_t slot) const {
   if (m->directWidth && slot < 2) return m->directLighting.Statistics(slot);
   return {};
}
VkImage VulkanWorldRenderer::SurfaceImage(uint32_t slot, uint32_t attachment) const {
   if (slot >= m->frames.size() || attachment >= 6 || !m->frames[slot].surface[attachment]) return VK_NULL_HANDLE;
   return *m->frames[slot].surface[attachment]->image;
}
VkImage VulkanWorldRenderer::HdrImage(uint32_t slot) const {
   if (slot >= m->frames.size() || !m->frames[slot].hdr) return VK_NULL_HANDLE;
   return *m->frames[slot].hdr->image;
}
void VulkanWorldRenderer::Composite(vk::CommandBuffer command, uint32_t slot, int x, int y, uint32_t width,
                                    uint32_t height) {
   auto &frame = m->frames.at(slot);
   command.bindPipeline(vk::PipelineBindPoint::eGraphics, frame.nativeDepth ? *m->toneDepthPipeline : *m->tonePipeline);
   command.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m->tonePipelineLayout, 0, frame.tone, {});
   command.setViewport(0, vk::Viewport(float(x), float(y), float(width), float(height), 0, 1));
   command.setScissor(0, vk::Rect2D({x, y}, {width, height}));
   struct {
      float exposure;
      uint32_t srgb;
      float a, b, c;
      uint32_t depth;
   } constants{frame.exposure,
               (m->outputFormat == vk::Format::eB8G8R8A8Srgb || m->outputFormat == vk::Format::eR8G8B8A8Srgb) ? 1u : 0u,
               frame.depthEncoding[0],
               frame.depthEncoding[1],
               frame.depthEncoding[2],
               frame.nativeDepth ? 1u : 0u};
   static_assert(sizeof(constants) == 24, "composite push constant ABI");
   command.pushConstants(*m->tonePipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(constants), &constants);
   command.draw(3, 1, 0, 0);
}
} // namespace dark
