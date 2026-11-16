#include "vulkan_world_renderer.hpp"
#include "vulkan_shadow_maps.hpp"
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
   void *mapped{};size_t capacity{};
   ~Buffer(){if(mapped)device.unmapMemory(*memory);}
};
struct Image {
   vk::UniqueDeviceMemory memory;
   vk::UniqueImage image;
   vk::UniqueImageView view;
   std::unique_ptr<Buffer> staging;
   uint32_t width{},height{};bool uploaded{};uint64_t allocationBytes{};
};
struct MaterialData {float color[4],emissionRoughness[4];uint32_t texture,opacity;float metallic,cutoff;uint32_t emissionUsesBaseTexture,doubleSided,padding[2];};
struct LightData {float positionRange[4],directionInner[4],colorIntensity[4],outerType[4];};
struct CameraData {float matrix[16],positionExposure[4],ambientScaleSize[4];ShadowCameraData shadows;float fogColorDistance[4];};
struct DrawConstants {float model[16];uint32_t material,receivesFog,lightOffset,lightCount;};
static_assert(sizeof(DrawConstants)==80,"world draw shader ABI mismatch");
static_assert(sizeof(MaterialData)==64 && sizeof(LightData)==64 && sizeof(CameraData)==144,"world shader ABI mismatch");
struct Frame {
   vk::CommandBuffer command;
   vk::UniqueQueryPool timing;
   bool timingRecorded=false;
   double gpuMilliseconds=-1;std::array<double,2> depthLightingGpuMilliseconds{{-1,-1}};
   std::shared_ptr<Image> hdr,depth,viewDistance;
   vk::UniqueFramebuffer framebuffer;
   vk::UniqueDescriptorPool pool;
   vk::DescriptorSet tone,mapped;
   std::array<std::unique_ptr<Buffer>,6> buffers;
   std::vector<std::shared_ptr<Image>> textures,uploads;
   ShadowPlan shadowPlan;std::array<double,4> recordCpuMilliseconds{};
   VulkanWorldRenderer::TextureWorkload textureWorkload;
   float exposure{1};uint32_t width{},height{};
   bool nativeDepth=false;std::array<float,3> depthEncoding{};
};
std::vector<uint32_t> readShader(const std::string &path) {
   std::ifstream file(path,std::ios::binary|std::ios::ate);
   if(!file)throw std::runtime_error("missing shader "+path);
   const auto end=file.tellg();if(end<=0 || size_t(end)%4)throw std::runtime_error("invalid shader "+path);
   std::vector<uint32_t> data(size_t(end)/4);file.seekg(0);file.read(reinterpret_cast<char *>(data.data()),end);
   if(!file)throw std::runtime_error("could not read shader "+path);return data;
}
}
struct VulkanWorldRenderer::Impl {
   vk::Device device;
   vk::PhysicalDevice physical;
   vk::PhysicalDeviceProperties properties;
   vk::PhysicalDeviceMemoryProperties memory;
   vk::Format depthFormat{},outputFormat{};
   std::string shaderDirectory;
   uint32_t maxTextures{};
   uint32_t timestampBits=0;
   vk::UniqueDescriptorSetLayout toneLayout;
   vk::UniqueDescriptorSetLayout mappedLayout;
   vk::UniquePipelineLayout tonePipelineLayout;
   vk::UniquePipelineLayout mappedPipelineLayout;
   vk::UniqueRenderPass hdrPass,hdrDepthPass;
   vk::UniquePipeline tonePipeline,toneDepthPipeline;
   vk::UniquePipeline mappedOpaquePipeline,mappedBlendPipeline,depthPipeline;
   vk::UniquePipeline nativeOpaquePipeline,nativeBlendPipeline,nativeDepthPipeline;
   vk::UniqueSampler sampler;
   vk::UniqueSampler shadowSampler;
   VulkanShadowMaps shadowMaps;
   VulkanWorldGeometry rasterGeometry;
   ShadowSettings shadowSettings;
   ShadowSelection shadowSelection;
   vk::UniqueCommandPool commands;
   std::array<Frame,2> frames;
   struct CachedTexture {uint64_t revision,lastUsed;std::shared_ptr<Image> image;const WorldScene *scene;};
   std::unordered_map<uint64_t,CachedTexture> textures;
   uint64_t textureSerial=0,textureBudget=64ull*1024*1024;
   const WorldScene *materialScene=nullptr;
   uint64_t materialRevision=0;
   std::vector<MaterialData> packedMaterials;
   std::vector<uint64_t> packedTextureIds;
   std::unordered_map<uint64_t,uint32_t> packedMaterialIndices;
   const WorldScene *receiverScene=nullptr;
   uint64_t receiverRevision=0;
   std::vector<RenderSceneHandle> receiverGpuLights;
   std::vector<uint32_t> receiverIndices;
   std::unordered_map<RenderSceneHandle,std::array<uint32_t,2>> receiverRanges;
   uint32_t memoryType(uint32_t bits,vk::MemoryPropertyFlags wanted) {
      for(uint32_t i=0;i<memory.memoryTypeCount;++i)
         if((bits&(1u<<i)) && (memory.memoryTypes[i].propertyFlags & wanted)==wanted)return i;
      throw std::runtime_error("no memory type for world renderer");
   }
   std::unique_ptr<Buffer> buffer(size_t bytes,vk::BufferUsageFlags usage) {
      auto b=std::make_unique<Buffer>();b->device=device;b->capacity=std::max<size_t>(4096,bytes);
      b->buffer=device.createBufferUnique(vk::BufferCreateInfo({},b->capacity,usage,vk::SharingMode::eExclusive));
      auto r=device.getBufferMemoryRequirements(*b->buffer);
      b->memory=device.allocateMemoryUnique(vk::MemoryAllocateInfo(r.size,memoryType(r.memoryTypeBits,
         vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent)));
      device.bindBufferMemory(*b->buffer,*b->memory,0);b->mapped=device.mapMemory(*b->memory,0,VK_WHOLE_SIZE);return b;
   }
   std::shared_ptr<Image> image(uint32_t width,uint32_t height,vk::Format format,vk::ImageUsageFlags usage,vk::ImageAspectFlags aspect) {
      auto i=std::make_shared<Image>();i->width=width;i->height=height;
      vk::ImageCreateInfo info({},vk::ImageType::e2D,format,{width,height,1},1,1,vk::SampleCountFlagBits::e1,
         vk::ImageTiling::eOptimal,usage,vk::SharingMode::eExclusive);
      i->image=device.createImageUnique(info);auto r=device.getImageMemoryRequirements(*i->image);i->allocationBytes=r.size;
      i->memory=device.allocateMemoryUnique(vk::MemoryAllocateInfo(r.size,memoryType(r.memoryTypeBits,vk::MemoryPropertyFlagBits::eDeviceLocal)));
      device.bindImageMemory(*i->image,*i->memory,0);
      i->view=device.createImageViewUnique(vk::ImageViewCreateInfo({},*i->image,vk::ImageViewType::e2D,format,{},vk::ImageSubresourceRange(aspect,0,1,0,1)));
      return i;
   }
   std::shared_ptr<Image> texture(uint64_t handle,const WorldScene &scene,vk::CommandBuffer command,Frame &frame) {
      const auto *source=handle?&scene.textures.at(handle):nullptr;
      const uint64_t revision=source?source->revision:1;
      auto cached=textures.find(handle);std::shared_ptr<Image> result;
      if(cached!=textures.end() && cached->second.revision==revision && (!handle || cached->second.scene==&scene)) {result=cached->second.image;cached->second.lastUsed=textureSerial;}
      if(!result) {
         const uint32_t width=source?source->width:1,height=source?source->height:1;
         ++frame.textureWorkload.createdImages;
         result=image(width,height,vk::Format::eR8G8B8A8Srgb,vk::ImageUsageFlagBits::eTransferDst|vk::ImageUsageFlagBits::eSampled,vk::ImageAspectFlagBits::eColor);
         ++frame.textureWorkload.createdStagingBuffers;
         result->staging=buffer(size_t(width)*height*4,vk::BufferUsageFlagBits::eTransferSrc);
         const uint8_t white[]={255,255,255,255};std::memcpy(result->staging->mapped,source?source->rgba.data():white,size_t(width)*height*4);
         textures[handle]={revision,textureSerial,result,&scene};
      }
      if(!result->uploaded) {
         vk::ImageMemoryBarrier barrier({},vk::AccessFlagBits::eTransferWrite,vk::ImageLayout::eUndefined,vk::ImageLayout::eTransferDstOptimal,
            VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,*result->image,{vk::ImageAspectFlagBits::eColor,0,1,0,1});
         command.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,vk::PipelineStageFlagBits::eTransfer,{}, {}, {},barrier);
         vk::BufferImageCopy copy(0,0,0,{vk::ImageAspectFlagBits::eColor,0,0,1},{0,0,0},{result->width,result->height,1});
         command.copyBufferToImage(*result->staging->buffer,*result->image,vk::ImageLayout::eTransferDstOptimal,copy);
         barrier.srcAccessMask=vk::AccessFlagBits::eTransferWrite;barrier.dstAccessMask=vk::AccessFlagBits::eShaderRead;
         barrier.oldLayout=vk::ImageLayout::eTransferDstOptimal;barrier.newLayout=vk::ImageLayout::eShaderReadOnlyOptimal;
         command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eFragmentShader,{}, {}, {},barrier);
         result->uploaded=true;frame.uploads.push_back(result);++frame.textureWorkload.uploads;
      }return result;
   }
   void trimTextures(Frame &frame) {
      uint64_t bytes=0;for(const auto &entry:textures)bytes+=entry.second.image->allocationBytes;
      if(bytes>textureBudget) {
         std::vector<std::pair<uint64_t,uint64_t>> candidates;
         for(const auto &entry:textures)if(entry.second.image.use_count()==1)
            candidates.emplace_back(entry.second.lastUsed,entry.first);
         std::sort(candidates.begin(),candidates.end());
         for(const auto &candidate:candidates) {
            if(bytes<=textureBudget)break;
            const auto found=textures.find(candidate.second);
            bytes-=found->second.image->allocationBytes;textures.erase(found);++frame.textureWorkload.evictedImages;
         }
      }
      // Images referenced by either frame are required and can exceed the
      // retention budget. Eviction never destroys a submitted frame's resources.
      frame.textureWorkload.cachedBytes=bytes;
   }
   vk::UniquePipeline pipeline(vk::RenderPass pass,vk::PipelineLayout layout,const std::vector<uint32_t> &vertex,
      const std::vector<uint32_t> &fragment,bool world,bool blend,bool depthOnly=false,bool depthEqual=false,bool bridgeDepth=false,uint32_t colorCount=1) {
      auto vs=device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({},vertex.size()*4,vertex.data()));
      auto fs=device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({},fragment.size()*4,fragment.data()));
      std::array<vk::PipelineShaderStageCreateInfo,2> stages{{{{},vk::ShaderStageFlagBits::eVertex,*vs,"main"},{{},vk::ShaderStageFlagBits::eFragment,*fs,"main"}}};
      vk::VertexInputBindingDescription binding(0,sizeof(RenderSceneVertex),vk::VertexInputRate::eVertex);
      std::array<vk::VertexInputAttributeDescription,3> attributes{{{0,0,vk::Format::eR32G32B32Sfloat,0},{1,0,vk::Format::eR32G32B32Sfloat,12},{2,0,vk::Format::eR32G32Sfloat,24}}};
      vk::PipelineVertexInputStateCreateInfo input;
      if(world){input.vertexBindingDescriptionCount=1;input.pVertexBindingDescriptions=&binding;input.vertexAttributeDescriptionCount=3;input.pVertexAttributeDescriptions=attributes.data();}
      vk::PipelineInputAssemblyStateCreateInfo assembly({},vk::PrimitiveTopology::eTriangleList);
      vk::PipelineViewportStateCreateInfo viewport({},1,nullptr,1,nullptr);
      vk::PipelineRasterizationStateCreateInfo raster({},VK_FALSE,VK_FALSE,vk::PolygonMode::eFill,vk::CullModeFlagBits::eNone,vk::FrontFace::eCounterClockwise,VK_FALSE,0,0,0,1);
      vk::PipelineMultisampleStateCreateInfo ms({},vk::SampleCountFlagBits::e1);
      vk::PipelineDepthStencilStateCreateInfo depth({},world||bridgeDepth,bridgeDepth||(world&&!blend&&!depthEqual),bridgeDepth?vk::CompareOp::eAlways:(depthEqual?vk::CompareOp::eEqual:vk::CompareOp::eLessOrEqual));
      vk::PipelineColorBlendAttachmentState attachment;
      attachment.blendEnable=blend;attachment.srcColorBlendFactor=vk::BlendFactor::eSrcAlpha;attachment.dstColorBlendFactor=vk::BlendFactor::eOneMinusSrcAlpha;
      attachment.colorBlendOp=vk::BlendOp::eAdd;attachment.srcAlphaBlendFactor=vk::BlendFactor::eOne;attachment.dstAlphaBlendFactor=vk::BlendFactor::eOneMinusSrcAlpha;
      attachment.alphaBlendOp=vk::BlendOp::eAdd;attachment.colorWriteMask=vk::ColorComponentFlagBits::eR|vk::ColorComponentFlagBits::eG|vk::ColorComponentFlagBits::eB|vk::ColorComponentFlagBits::eA;
      if(depthOnly)attachment.colorWriteMask={};
      const std::vector<vk::PipelineColorBlendAttachmentState> blendAttachments(colorCount,attachment);
      vk::PipelineColorBlendStateCreateInfo blending({},VK_FALSE,vk::LogicOp::eCopy,colorCount,blendAttachments.data());
      std::array<vk::DynamicState,2> dynamicStates{{vk::DynamicState::eViewport,vk::DynamicState::eScissor}};
      vk::PipelineDynamicStateCreateInfo dynamic({},dynamicStates);
      vk::GraphicsPipelineCreateInfo info({},stages,&input,&assembly,nullptr,&viewport,&raster,&ms,&depth,&blending,&dynamic,layout,pass);
      return device.createGraphicsPipelineUnique({},info).value;
   }
   void targets(Frame &frame,uint32_t width,uint32_t height,bool nativeDepth) {
      if(frame.width==width && frame.height==height && frame.nativeDepth==nativeDepth)return;
      if(nativeDepth && !hdrDepthPass)throw std::runtime_error("native depth bridge requires blendable/sampleable R32_SFLOAT");
      if(nativeDepth && !nativeOpaquePipeline) {
         const auto vertex=readShader(shaderDirectory+"/world.vert.spv");
         const auto fragment=readShader(shaderDirectory+"/world_mapped_native.frag.spv");
         auto opaque=pipeline(*hdrDepthPass,*mappedPipelineLayout,vertex,fragment,true,false,false,true,false,2);
         auto blend=pipeline(*hdrDepthPass,*mappedPipelineLayout,vertex,fragment,true,true,false,false,false,2);
         auto depth=pipeline(*hdrDepthPass,*mappedPipelineLayout,vertex,readShader(shaderDirectory+"/world_depth.frag.spv"),true,false,true,false,false,2);
         nativeOpaquePipeline=std::move(opaque);nativeBlendPipeline=std::move(blend);nativeDepthPipeline=std::move(depth);
      }

      // Publish a complete target set only after every allocation succeeds.
      // A failed resize/toggle must not leave matching dimensions on half-built targets.
      auto hdr=image(width,height,vk::Format::eR16G16B16A16Sfloat,vk::ImageUsageFlagBits::eColorAttachment|vk::ImageUsageFlagBits::eSampled|vk::ImageUsageFlagBits::eTransferSrc,vk::ImageAspectFlagBits::eColor);
      auto depth=image(width,height,depthFormat,vk::ImageUsageFlagBits::eDepthStencilAttachment,vk::ImageAspectFlagBits::eDepth);
      std::shared_ptr<Image> distance;
      std::vector<vk::ImageView> views{*hdr->view,*depth->view};
      if(nativeDepth) {
         distance=image(width,height,vk::Format::eR32Sfloat,vk::ImageUsageFlagBits::eColorAttachment|vk::ImageUsageFlagBits::eSampled,vk::ImageAspectFlagBits::eColor);
         views.push_back(*distance->view);
      }
      auto framebuffer=device.createFramebufferUnique(vk::FramebufferCreateInfo({},nativeDepth?*hdrDepthPass:*hdrPass,views,width,height,1));
      frame.framebuffer=std::move(framebuffer);frame.hdr=std::move(hdr);frame.depth=std::move(depth);frame.viewDistance=std::move(distance);
      frame.nativeDepth=nativeDepth;frame.width=width;frame.height=height;
   }
   template<class T> vk::DescriptorBufferInfo upload(Frame &frame,size_t index,const T *data,size_t count,vk::BufferUsageFlags usage) {
      const size_t bytes=std::max<size_t>(sizeof(T)*count,16);
      if((usage & vk::BufferUsageFlagBits::eStorageBuffer) && bytes>properties.limits.maxStorageBufferRange)throw std::runtime_error("world buffer exceeds device storage limit");
      auto &b=frame.buffers[index];if(!b || b->capacity<bytes)b=buffer(bytes,usage);
      if(count)std::memcpy(b->mapped,data,sizeof(T)*count);return {*b->buffer,0,bytes};
   }
};
VulkanWorldRenderer::VulkanWorldRenderer():m(std::make_unique<Impl>()){}
VulkanWorldRenderer::~VulkanWorldRenderer()=default;
VulkanWorldRenderer::ShadowWorkload VulkanWorldRenderer::GetShadowWorkload(uint32_t slot) const {
   const auto stats=m->shadowMaps.FrameStats(slot);
   return {stats.renderedViews,stats.reusedViews,stats.drawCalls,stats.gpuMilliseconds,m->frames.at(slot).gpuMilliseconds,m->frames.at(slot).recordCpuMilliseconds,m->frames.at(slot).depthLightingGpuMilliseconds,stats.boundsTests,stats.drawBatches,stats.staticRendered,stats.staticReused,stats.depthCopies,stats.depthMemoryBytes};
}
VulkanWorldRenderer::TextureWorkload VulkanWorldRenderer::GetTextureWorkload(uint32_t slot) const {
   return m->frames.at(slot).textureWorkload;
}
void VulkanWorldRenderer::SetTextureCacheBudget(uint64_t bytes) {m->textureBudget=bytes;}
const ShadowSelection &VulkanWorldRenderer::GetShadowSelection() const {return m->shadowSelection;}
void VulkanWorldRenderer::SetShadowSettings(const ShadowSettings &settings) {
   m->shadowSettings=settings;
   m->shadowSettings.localLights=std::min(settings.localLights,ShadowSettings::maxLocalLights);
   m->shadowSettings.localFaces=std::min(settings.localFaces,ShadowSettings::maxLocalFaces);
   m->shadowSettings.cachedLights=std::min(settings.cachedLights,ShadowSettings::maxCachedLights);
   m->shadowSettings.cachedFaces=std::min(settings.cachedFaces,ShadowSettings::maxCachedFaces);
   m->shadowSettings.sunCascades=std::min(settings.sunCascades,4u);
   m->shadowSettings.blockerSamples=std::clamp(settings.blockerSamples,1u,64u);
   m->shadowSettings.filterSamples=std::clamp(settings.filterSamples,1u,64u);
}
void VulkanWorldRenderer::Shutdown(){m=std::make_unique<Impl>();}
bool VulkanWorldRenderer::Initialize(vk::PhysicalDevice physical,vk::Device device,uint32_t family,
   vk::RenderPass compositePass,vk::Format depthFormat,vk::Format outputFormat,const std::string &directory,std::string &error,bool indirectShadows) {
 try {
   if(m->device)throw std::runtime_error("world renderer already initialized");
   m->device=device;m->physical=physical;m->properties=physical.getProperties();m->memory=physical.getMemoryProperties();
   m->timestampBits=physical.getQueueFamilyProperties().at(family).timestampValidBits;
   m->depthFormat=depthFormat;m->outputFormat=outputFormat;m->shaderDirectory=directory;
   const auto reserve=[](uint32_t limit,uint32_t used){return limit>used?limit-used:0u;};
   m->maxTextures=std::min({1024u,reserve(m->properties.limits.maxPerStageDescriptorSamplers,3),
      reserve(m->properties.limits.maxPerStageDescriptorSampledImages,3),
      reserve(m->properties.limits.maxDescriptorSetSamplers,3),reserve(m->properties.limits.maxDescriptorSetSampledImages,3),
      reserve(m->properties.limits.maxPerStageResources,11)});
   if(m->maxTextures==0)throw std::runtime_error("no texture descriptors available");
   const auto format=physical.getFormatProperties(vk::Format::eR16G16B16A16Sfloat);
   const auto needed=vk::FormatFeatureFlagBits::eColorAttachment|vk::FormatFeatureFlagBits::eSampledImage;
   if((format.optimalTilingFeatures&needed)!=needed)throw std::runtime_error("floating-point HDR targets unavailable");
   std::array<vk::DescriptorSetLayoutBinding,8> bindings;
   bindings[0]={0,vk::DescriptorType::eCombinedImageSampler,1,vk::ShaderStageFlagBits::eFragment};
   for(uint32_t i=1;i<=6;++i)bindings[i]={i,i==4?vk::DescriptorType::eUniformBuffer:vk::DescriptorType::eStorageBuffer,1,
      (i==1 || i==4)?(vk::ShaderStageFlagBits::eVertex|vk::ShaderStageFlagBits::eFragment):vk::ShaderStageFlagBits::eFragment};
   bindings[7]={7,vk::DescriptorType::eCombinedImageSampler,m->maxTextures,vk::ShaderStageFlagBits::eFragment};
   std::array<vk::DescriptorSetLayoutBinding,11> mappedBindings;
   std::copy(bindings.begin(),bindings.end(),mappedBindings.begin());
   mappedBindings[0]={0,vk::DescriptorType::eCombinedImageSampler,1,vk::ShaderStageFlagBits::eFragment};
   mappedBindings[8]={8,vk::DescriptorType::eCombinedImageSampler,1,vk::ShaderStageFlagBits::eFragment};
   mappedBindings[9]={9,vk::DescriptorType::eCombinedImageSampler,1,vk::ShaderStageFlagBits::eFragment};
   mappedBindings[10]={10,vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eVertex};
   std::array<vk::DescriptorBindingFlags,11> mappedFlags{};mappedFlags[7]=vk::DescriptorBindingFlagBits::ePartiallyBound;
   vk::DescriptorSetLayoutBindingFlagsCreateInfo mappedFlagInfo(mappedFlags);
   vk::DescriptorSetLayoutCreateInfo mappedInfo({},mappedBindings);mappedInfo.pNext=&mappedFlagInfo;
   m->mappedLayout=device.createDescriptorSetLayoutUnique(mappedInfo);
   if(!m->shadowMaps.Initialize(physical,device,*m->mappedLayout,directory,error,family,indirectShadows))throw std::runtime_error(error);
   if(!m->rasterGeometry.Initialize(physical,device))throw std::runtime_error("raster geometry initialization failed");
   std::array<vk::DescriptorSetLayoutBinding,2> hdrBindings{{{0,vk::DescriptorType::eCombinedImageSampler,1,vk::ShaderStageFlagBits::eFragment},{1,vk::DescriptorType::eCombinedImageSampler,1,vk::ShaderStageFlagBits::eFragment}}};
   m->toneLayout=device.createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo({},hdrBindings));
   vk::PushConstantRange draw(vk::ShaderStageFlagBits::eVertex,0,sizeof(DrawConstants));
   m->mappedPipelineLayout=device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({},1,&*m->mappedLayout,1,&draw));
   vk::PushConstantRange exposure(vk::ShaderStageFlagBits::eFragment,0,24);
   m->tonePipelineLayout=device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({},1,&*m->toneLayout,1,&exposure));
   std::array<vk::AttachmentDescription,2> attachments;
   attachments[0]={{},vk::Format::eR16G16B16A16Sfloat,vk::SampleCountFlagBits::e1,vk::AttachmentLoadOp::eClear,vk::AttachmentStoreOp::eStore,
      vk::AttachmentLoadOp::eDontCare,vk::AttachmentStoreOp::eDontCare,vk::ImageLayout::eUndefined,vk::ImageLayout::eShaderReadOnlyOptimal};
   attachments[1]={{},depthFormat,vk::SampleCountFlagBits::e1,vk::AttachmentLoadOp::eClear,vk::AttachmentStoreOp::eDontCare,
      vk::AttachmentLoadOp::eDontCare,vk::AttachmentStoreOp::eDontCare,vk::ImageLayout::eUndefined,vk::ImageLayout::eDepthStencilAttachmentOptimal};
   vk::AttachmentReference color(0,vk::ImageLayout::eColorAttachmentOptimal),depth(1,vk::ImageLayout::eDepthStencilAttachmentOptimal);
   vk::SubpassDescription subpass({},vk::PipelineBindPoint::eGraphics,0,nullptr,1,&color,nullptr,&depth);
   vk::SubpassDependency dependency(0,VK_SUBPASS_EXTERNAL,vk::PipelineStageFlagBits::eColorAttachmentOutput,vk::PipelineStageFlagBits::eFragmentShader,
      vk::AccessFlagBits::eColorAttachmentWrite,vk::AccessFlagBits::eShaderRead);
   m->hdrPass=device.createRenderPassUnique(vk::RenderPassCreateInfo({},attachments,subpass,dependency));
   const auto distanceFeatures=physical.getFormatProperties(vk::Format::eR32Sfloat).optimalTilingFeatures;
   const auto distanceNeeded=vk::FormatFeatureFlagBits::eColorAttachment|vk::FormatFeatureFlagBits::eColorAttachmentBlend|vk::FormatFeatureFlagBits::eSampledImage;
   if((distanceFeatures&distanceNeeded)==distanceNeeded) {
      std::array<vk::AttachmentDescription,3> bridgeAttachments{{attachments[0],attachments[1],
         {{},vk::Format::eR32Sfloat,vk::SampleCountFlagBits::e1,vk::AttachmentLoadOp::eClear,vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,vk::AttachmentStoreOp::eDontCare,vk::ImageLayout::eUndefined,vk::ImageLayout::eShaderReadOnlyOptimal}}};
      const std::array<vk::AttachmentReference,2> colors{{color,{2,vk::ImageLayout::eColorAttachmentOptimal}}};
      vk::SubpassDescription bridge({},vk::PipelineBindPoint::eGraphics,0,nullptr,colors.size(),colors.data(),nullptr,&depth);
      m->hdrDepthPass=device.createRenderPassUnique(vk::RenderPassCreateInfo({},bridgeAttachments,bridge,dependency));
   }
   const auto vs=readShader(directory+"/world.vert.spv");
   const auto mappedFragment=readShader(directory+"/world_mapped.frag.spv");
   m->mappedOpaquePipeline=m->pipeline(*m->hdrPass,*m->mappedPipelineLayout,vs,mappedFragment,true,false,false,true);
   m->depthPipeline=m->pipeline(*m->hdrPass,*m->mappedPipelineLayout,vs,readShader(directory+"/world_depth.frag.spv"),true,false,true);
   m->mappedBlendPipeline=m->pipeline(*m->hdrPass,*m->mappedPipelineLayout,vs,mappedFragment,true,true);

   m->tonePipeline=m->pipeline(compositePass,*m->tonePipelineLayout,readShader(directory+"/world_composite.vert.spv"),readShader(directory+"/world_composite.frag.spv"),false,true);
   m->toneDepthPipeline=m->pipeline(compositePass,*m->tonePipelineLayout,readShader(directory+"/world_composite.vert.spv"),readShader(directory+"/world_composite.frag.spv"),false,true,false,false,true);
   m->sampler=device.createSamplerUnique(vk::SamplerCreateInfo({},vk::Filter::eLinear,vk::Filter::eLinear,vk::SamplerMipmapMode::eNearest,
      vk::SamplerAddressMode::eRepeat,vk::SamplerAddressMode::eRepeat,vk::SamplerAddressMode::eRepeat,0,VK_FALSE,1,VK_FALSE,vk::CompareOp::eNever,0,0));
   m->commands=device.createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,family));
   m->shadowSampler=device.createSamplerUnique(vk::SamplerCreateInfo({},vk::Filter::eNearest,vk::Filter::eNearest,vk::SamplerMipmapMode::eNearest,
      vk::SamplerAddressMode::eClampToEdge,vk::SamplerAddressMode::eClampToEdge,vk::SamplerAddressMode::eClampToEdge));
   auto commands=device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(*m->commands,vk::CommandBufferLevel::ePrimary,2));
   for(size_t i=0;i<2;++i) {
      auto &frame=m->frames[i];frame.command=commands[i];
      if(m->timestampBits)frame.timing=device.createQueryPoolUnique(vk::QueryPoolCreateInfo({},vk::QueryType::eTimestamp,4));
      std::array<vk::DescriptorPoolSize,3> sizes{{{vk::DescriptorType::eStorageBuffer,6},
         {vk::DescriptorType::eUniformBuffer,1},{vk::DescriptorType::eCombinedImageSampler,m->maxTextures+5}}};
      frame.pool=device.createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo({},2,sizes));
      std::array<vk::DescriptorSetLayout,2> layouts{{*m->toneLayout,*m->mappedLayout}};
      auto sets=device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo(*frame.pool,layouts));
      frame.tone=sets[0];frame.mapped=sets[1];
   }return true;
 }catch(const std::exception &failure){error=failure.what();Shutdown();return false;}
}
VkCommandBuffer VulkanWorldRenderer::Record(uint32_t slot,const WorldScene &scene,uint32_t width,uint32_t height,std::string &error) {
 ZoneScopedN("Record modern world lighting");
 try {
   if(!m->device || slot>=2 || !scene.hasCamera || !width || !height)return VK_NULL_HANDLE;
   auto &frame=m->frames[slot];
   const auto recordStart=std::chrono::steady_clock::now();
   if(frame.timing && frame.timingRecorded) {
      uint64_t ticks[4]{};
      if(m->device.getQueryPoolResults(*frame.timing,0,4,sizeof(ticks),ticks,sizeof(uint64_t),vk::QueryResultFlagBits::e64)==vk::Result::eSuccess) {
         const uint64_t mask=m->timestampBits==64?UINT64_MAX:((uint64_t(1)<<m->timestampBits)-1);
         const auto milliseconds=[&](unsigned first,unsigned last){return double((ticks[last]-ticks[first])&mask)*m->properties.limits.timestampPeriod/1000000;};
         frame.gpuMilliseconds=milliseconds(0,3);
         frame.depthLightingGpuMilliseconds={milliseconds(1,2),milliseconds(2,3)};
      }
   }
   const auto selection=SelectShadowLights(scene,m->shadowSettings,m->shadowSelection);
   if(!BuildShadowPlan(scene,m->shadowSettings,selection,frame.shadowPlan,&frame.shadowPlan))throw std::runtime_error("invalid shadow camera plan");
   m->shadowSelection=selection;
   const auto planEnd=std::chrono::steady_clock::now();
   for(auto &image:frame.uploads)image->staging.reset();
   frame.uploads.clear();frame.textures.clear();frame.textureWorkload={};
   for(auto it=m->textures.begin();it!=m->textures.end();)if(it->first && !scene.textures.count(it->first))it=m->textures.erase(it);else ++it;
   ++m->textureSerial;
   auto command=frame.command;command.reset();command.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
   if(frame.timing){command.resetQueryPool(*frame.timing,0,4);command.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe,*frame.timing,0);}
   auto allocatedShadows=m->shadowSettings;
   // The descriptor remains valid without reserving full sun cascades in
   // indoor/night missions. Restore the configured pool if sunlight appears.
   if(!selection.sunlight)allocatedShadows.sunCascades=0;
   if(!m->shadowMaps.Prepare(command,slot,allocatedShadows,error))throw std::runtime_error(error);
   m->targets(frame,width,height,scene.camera.nativeDepth[3]!=0);
   std::copy(scene.camera.nativeDepth,scene.camera.nativeDepth+3,frame.depthEncoding.begin());
   frame.exposure=scene.camera.exposure;
   if(m->materialScene!=&scene || m->materialRevision!=scene.materialsRevision) {
      std::vector<uint64_t> materialIds;for(const auto &entry:scene.materials)materialIds.push_back(entry.first);
      std::sort(materialIds.begin(),materialIds.end());
      std::unordered_map<uint64_t,uint32_t> materialIndices,textureIndices;
      std::vector<uint64_t> textureIds{0};textureIndices[0]=0;
      std::vector<MaterialData> materials;
      for(const auto id:materialIds) {
         const auto &source=scene.materials.at(id);MaterialData data{};
         std::copy(source.baseColor,source.baseColor+4,data.color);
         std::copy(source.emission,source.emission+3,data.emissionRoughness);data.emissionRoughness[3]=source.roughness;
         auto found=textureIndices.find(source.baseColorTexture);
         if(found==textureIndices.end()) {
            const uint32_t index=uint32_t(textureIds.size());textureIds.push_back(source.baseColorTexture);
            found=textureIndices.emplace(source.baseColorTexture,index).first;
         }
         data.emissionUsesBaseTexture=source.emissionUsesBaseTexture;data.doubleSided=source.doubleSided;
         data.texture=found->second;data.opacity=source.opacity;data.metallic=source.metallic;data.cutoff=source.alphaCutoff;
         materialIndices[id]=uint32_t(materials.size());materials.push_back(data);
      }
      if(textureIds.size()>m->maxTextures)throw std::runtime_error("world texture count exceeds device descriptor limit");
      m->packedMaterials=std::move(materials);m->packedTextureIds=std::move(textureIds);
      m->packedMaterialIndices=std::move(materialIndices);
      m->materialScene=&scene;m->materialRevision=scene.materialsRevision;
   }
   const auto &materials=m->packedMaterials;
   const auto &textureIds=m->packedTextureIds;
   const auto &materialIndices=m->packedMaterialIndices;
   for(const auto id:textureIds)frame.textures.push_back(m->texture(id,scene,command,frame));
   struct LightBounds {int x0,y0,x1,y1;};
   const uint32_t columns=(width+15)/16,rows=(height+15)/16;
   if(uint64_t(columns)*rows>UINT32_MAX)throw std::runtime_error("world target too large");
   std::vector<std::array<uint32_t,2>> tiles(size_t(columns)*rows);
   std::unordered_map<RenderSceneHandle,uint32_t> gpuLights;
   std::vector<RenderSceneHandle> gpuLightHandles;
   std::vector<LightData> lights;std::vector<LightBounds> bounds;std::vector<uint32_t> globalLights;
   auto fullScreen=[&](const LightBounds &r){return r.x0==0 && r.y0==0 && r.x1==int(columns)-1 && r.y1==int(rows)-1;};
   for(const auto &entry:scene.lights) {
      const auto &source=entry.second;if(!source.enabled || source.intensity==0)continue;
      LightBounds rect{0,0,int(columns)-1,int(rows)-1};bool global=source.type==RenderSceneDirectionalLight || source.range<=0;
      float minX=INFINITY,minY=INFINITY,maxX=-INFINITY,maxY=-INFINITY;
      if(!global)for(int corner=0;corner<8;++corner) {
         float point[4]={source.position[0]+((corner&1)?source.range:-source.range),
            source.position[1]+((corner&2)?source.range:-source.range),source.position[2]+((corner&4)?source.range:-source.range),1};
         float clip[4]{};for(int r=0;r<4;++r)for(int c=0;c<4;++c)clip[r]+=scene.camera.viewProjection[c*4+r]*point[c];
         if(clip[3]<=0){global=true;break;}
         minX=std::min(minX,clip[0]/clip[3]);maxX=std::max(maxX,clip[0]/clip[3]);
         minY=std::min(minY,clip[1]/clip[3]);maxY=std::max(maxY,clip[1]/clip[3]);
      }
      if(!global) {
         if(maxX < -1 || maxY < -1 || minX>1 || minY>1)continue;
         auto tile=[](float ndc,uint32_t pixels,int max){return std::clamp(int((std::clamp(ndc,-1.0f,1.0f)+1)*0.5f*pixels)/16,0,max);};
         rect={tile(minX,width,columns-1),tile(minY,height,rows-1),tile(maxX,width,columns-1),tile(maxY,height,rows-1)};
      }
      LightData light{};std::copy(source.position,source.position+3,light.positionRange);light.positionRange[3]=source.range;
      std::copy(source.direction,source.direction+3,light.directionInner);light.directionInner[3]=source.innerCos;
      std::copy(source.color,source.color+3,light.colorIntensity);light.colorIntensity[3]=source.intensity;
      light.outerType[0]=source.outerCos;light.outerType[1]=float(source.type);
      light.outerType[2]=-1;
      const auto shadow=frame.shadowPlan.firstView.find(entry.first);
      if(shadow!=frame.shadowPlan.firstView.end()) {
         light.outerType[2]=float(shadow->second);
         light.outerType[3]=float(frame.shadowPlan.views.at(shadow->second).metadata[3]);
      }
      gpuLights[entry.first]=uint32_t(lights.size());gpuLightHandles.push_back(entry.first);
      lights.push_back(light);bounds.push_back(rect);
      if(fullScreen(rect))globalLights.push_back(uint32_t(lights.size()-1));
      else for(int y=rect.y0;y<=rect.y1;++y)for(int x=rect.x0;x<=rect.x1;++x)++tiles[size_t(y)*columns+x][1];
   }
   uint64_t total=globalLights.size();for(auto &tile:tiles){tile[0]=uint32_t(total);total+=tile[1];if(total>UINT32_MAX)throw std::runtime_error("world light list too large");}
   if(total*4>m->properties.limits.maxStorageBufferRange)throw std::runtime_error("world light lists exceed GPU buffer limit");
   std::vector<uint32_t> lightIndices(size_t(total),0),cursor(tiles.size(),0);
   std::copy(globalLights.begin(),globalLights.end(),lightIndices.begin());
   for(uint32_t i=0;i<bounds.size();++i) {
      const auto &r=bounds[i];if(fullScreen(r))continue;for(int y=r.y0;y<=r.y1;++y)for(int x=r.x0;x<=r.x1;++x) {
         const size_t tile=size_t(y)*columns+x;lightIndices[tiles[tile][0]+cursor[tile]++]=i;
      }
   }
   // Cache receiver packing independently of poses/materials. GPU light order
   // matters because disabling/removing a light remaps the remaining indices.
   if(m->receiverScene!=&scene || m->receiverRevision!=scene.instanceLightsRevision ||
      m->receiverGpuLights!=gpuLightHandles) {
      std::vector<uint32_t> packedIndices;
      std::unordered_map<RenderSceneHandle,std::array<uint32_t,2>> ranges;
      std::map<std::vector<RenderSceneHandle>,std::array<uint32_t,2>> packedLists;
      for(const auto &entry:scene.instanceLights) {
         auto packed=packedLists.find(entry.second);
         if(packed==packedLists.end()) {
            const uint32_t first=uint32_t(packedIndices.size());
            for(const auto id:entry.second) {
               const auto found=gpuLights.find(id);
               if(found!=gpuLights.end())packedIndices.push_back(found->second);
            }
            if(packedIndices.size()>=UINT32_MAX || uint64_t(packedIndices.size())*4>m->properties.limits.maxStorageBufferRange)
               throw std::runtime_error("receiver light lists exceed GPU buffer limit");
            packed=packedLists.emplace(entry.second,std::array<uint32_t,2>{first,uint32_t(packedIndices.size())-first}).first;
         }
         ranges[entry.first]=packed->second;
      }
      m->receiverIndices=std::move(packedIndices);m->receiverRanges=std::move(ranges);
      m->receiverGpuLights=std::move(gpuLightHandles);m->receiverRevision=scene.instanceLightsRevision;m->receiverScene=&scene;
   }
   const uint64_t linkedEnd=uint64_t(lightIndices.size())+m->receiverIndices.size();
   if(linkedEnd>=UINT32_MAX || linkedEnd*4>m->properties.limits.maxStorageBufferRange)
      throw std::runtime_error("combined light lists exceed GPU buffer limit");
   const uint32_t receiverBase=uint32_t(lightIndices.size());
   lightIndices.insert(lightIndices.end(),m->receiverIndices.begin(),m->receiverIndices.end());
   CameraData camera{};std::copy(scene.camera.viewProjection,scene.camera.viewProjection+16,camera.matrix);
   camera.shadows=BuildShadowCameraData(frame.shadowPlan,m->shadowSettings);
   std::copy(scene.camera.fogColor,scene.camera.fogColor+3,camera.fogColorDistance);camera.fogColorDistance[3]=scene.camera.fogDistance;
   camera.shadows.globalLightCount=uint32_t(globalLights.size());
   std::copy(scene.camera.position,scene.camera.position+3,camera.positionExposure);camera.positionExposure[3]=frame.exposure;
   camera.ambientScaleSize[0]=scene.camera.ambient;camera.ambientScaleSize[1]=scene.camera.lightScale;
   camera.ambientScaleSize[2]=float(width);camera.ambientScaleSize[3]=float(height);
   const auto storage=vk::BufferUsageFlagBits::eStorageBuffer;
   std::array<vk::DescriptorBufferInfo,6> buffers{{m->upload(frame,0,frame.shadowPlan.views.data(),frame.shadowPlan.views.size(),storage),
      m->upload(frame,1,materials.data(),materials.size(),storage),m->upload(frame,2,lights.data(),lights.size(),storage),
      m->upload(frame,3,&camera,1,vk::BufferUsageFlagBits::eUniformBuffer),m->upload(frame,4,tiles.data(),tiles.size(),storage),
      m->upload(frame,5,lightIndices.data(),lightIndices.size(),storage)}};
   std::vector<vk::DescriptorImageInfo> images;
   for(const auto &texture:frame.textures)images.emplace_back(*m->sampler,*texture->view,vk::ImageLayout::eShaderReadOnlyOptimal);
   std::vector<vk::WriteDescriptorSet> writes;
   vk::DescriptorImageInfo hdr(*m->sampler,*frame.hdr->view,vk::ImageLayout::eShaderReadOnlyOptimal);
   writes.emplace_back(frame.tone,0,0,1,vk::DescriptorType::eCombinedImageSampler,&hdr);
   const vk::DescriptorImageInfo bridgeDepth(*m->shadowSampler,frame.nativeDepth?*frame.viewDistance->view:*frame.hdr->view,
      vk::ImageLayout::eShaderReadOnlyOptimal);
   writes.emplace_back(frame.tone,1,0,1,vk::DescriptorType::eCombinedImageSampler,&bridgeDepth);
   const auto &shadowBuffer=buffers[0];
   writes.emplace_back(frame.mapped,1,0,1,vk::DescriptorType::eStorageBuffer,nullptr,&shadowBuffer);
   for(uint32_t i=1;i<6;++i)writes.emplace_back(frame.mapped,i+1,0,1,i==3?vk::DescriptorType::eUniformBuffer:vk::DescriptorType::eStorageBuffer,nullptr,&buffers[i]);
   writes.emplace_back(frame.mapped,7,0,uint32_t(images.size()),vk::DescriptorType::eCombinedImageSampler,images.data());
   const vk::DescriptorImageInfo localShadow(*m->shadowSampler,m->shadowMaps.LocalView(slot),vk::ImageLayout::eDepthStencilReadOnlyOptimal);
   const vk::DescriptorImageInfo sunShadow(*m->shadowSampler,m->shadowMaps.SunView(slot),vk::ImageLayout::eDepthStencilReadOnlyOptimal);
   writes.emplace_back(frame.mapped,0,0,1,vk::DescriptorType::eCombinedImageSampler,&localShadow);
   writes.emplace_back(frame.mapped,8,0,1,vk::DescriptorType::eCombinedImageSampler,&sunShadow);
   const vk::DescriptorImageInfo cachedShadow(*m->shadowSampler,m->shadowMaps.CachedView(slot),vk::ImageLayout::eDepthStencilReadOnlyOptimal);
   writes.emplace_back(frame.mapped,9,0,1,vk::DescriptorType::eCombinedImageSampler,&cachedShadow);
   m->device.updateDescriptorSets(writes,{});
   if(!m->rasterGeometry.Update(slot,scene,error))throw std::runtime_error(error);
   const auto uploadEnd=std::chrono::steady_clock::now();
   if(!m->shadowMaps.Record(command,slot,frame.shadowPlan,scene,m->rasterGeometry,frame.mapped,materialIndices,error))throw std::runtime_error(error);
   const auto shadowsEnd=std::chrono::steady_clock::now();
   if(frame.timing)command.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe,*frame.timing,1);
   std::array<vk::ClearValue,3> clears{};
   clears[0].color=vk::ClearColorValue(std::array<float,4>{0,0,0,0});clears[1].depthStencil=vk::ClearDepthStencilValue(1,0);
   command.beginRenderPass(vk::RenderPassBeginInfo(frame.nativeDepth?*m->hdrDepthPass:*m->hdrPass,*frame.framebuffer,{{0,0},{width,height}},frame.nativeDepth?3u:2u,clears.data()),vk::SubpassContents::eInline);
   command.setViewport(0,vk::Viewport(0,0,float(width),float(height),0,1));command.setScissor(0,vk::Rect2D({0,0},{width,height}));
   command.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,*m->mappedPipelineLayout,0,frame.mapped,{});
   struct VisibleDraw {
      DrawConstants constants;double distance;RenderSceneHandle id;
      VkBuffer vertices,indices;uint32_t indexCount,firstIndex;int32_t vertexOffset;
   };
   std::vector<VisibleDraw> opaque,blended;
   const auto cameraFrustum=SceneExtractFrustum(scene.camera.viewProjection);
   for(const auto &entry:scene.instances) {
      const auto &instance=entry.second;if(!instance.visible)continue;
      const auto &mesh=scene.meshes.at(instance.mesh);
      const auto bounds=SceneTransformBounds(mesh.bounds,instance.transform);
      if(!SceneBoundsVisible(bounds,cameraFrustum))continue;
      double distance=0;for(unsigned axis=0;axis<3;++axis){const double delta=bounds.center[axis]-scene.camera.position[axis];distance+=delta*delta;}
      VisibleDraw item{};item.distance=distance;item.id=entry.first;
      VkDeviceSize vertexOffset{},indexOffset{};
      if(!m->rasterGeometry.MeshBuffers(slot,instance.mesh,item.vertices,item.indices,&vertexOffset,&indexOffset))
         throw std::runtime_error("missing raster mesh cache");
      if(vertexOffset/sizeof(RenderSceneVertex)>INT32_MAX || indexOffset/sizeof(uint32_t)>UINT32_MAX)
         throw std::runtime_error("geometry draw offset exceeds Vulkan draw limits");
      item.indexCount=uint32_t(mesh.indices.size());item.firstIndex=uint32_t(indexOffset/sizeof(uint32_t));
      item.vertexOffset=int32_t(vertexOffset/sizeof(RenderSceneVertex));
      std::copy(instance.transform,instance.transform+16,item.constants.model);item.constants.material=materialIndices.at(mesh.material);item.constants.receivesFog=instance.receivesFog;
      const auto receiver=m->receiverRanges.find(entry.first);
      item.constants.lightOffset=receiver==m->receiverRanges.end()?UINT32_MAX:receiverBase+receiver->second[0];
      item.constants.lightCount=receiver==m->receiverRanges.end()?0:receiver->second[1];
      (scene.materials.at(mesh.material).opacity==RenderSceneBlended?blended:opaque).push_back(item);
   }
   // Terrain instances have identity transforms; their mesh bounds provide
   // meaningful world positions for depth ordering, unlike instance origins.
   std::sort(opaque.begin(),opaque.end(),[](const auto &a,const auto &b){return a.distance!=b.distance?a.distance<b.distance:a.id<b.id;});
   std::sort(blended.begin(),blended.end(),[](const auto &a,const auto &b){return a.distance!=b.distance?a.distance>b.distance:a.id<b.id;});
   VkBuffer boundVertices{},boundIndices{};
   auto draw=[&](const auto &list,vk::Pipeline pipeline) {
      command.bindPipeline(vk::PipelineBindPoint::eGraphics,pipeline);
      for(const auto &item:list) {
         command.pushConstants(*m->mappedPipelineLayout,vk::ShaderStageFlagBits::eVertex,0,sizeof(item.constants),&item.constants);
         if(boundVertices!=item.vertices) {
            const vk::Buffer buffer=item.vertices;const vk::DeviceSize offset=0;
            command.bindVertexBuffers(0,1,&buffer,&offset);boundVertices=item.vertices;
         }
         if(boundIndices!=item.indices) {
            command.bindIndexBuffer(item.indices,0,vk::IndexType::eUint32);boundIndices=item.indices;
         }
         command.drawIndexed(item.indexCount,1,item.firstIndex,item.vertexOffset,0);
      }
   };
   draw(opaque,frame.nativeDepth?*m->nativeDepthPipeline:*m->depthPipeline);
   if(frame.timing)command.writeTimestamp(vk::PipelineStageFlagBits::eLateFragmentTests,*frame.timing,2);
   draw(opaque,frame.nativeDepth?*m->nativeOpaquePipeline:*m->mappedOpaquePipeline);draw(blended,frame.nativeDepth?*m->nativeBlendPipeline:*m->mappedBlendPipeline);
   command.endRenderPass();
   if(frame.timing){command.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe,*frame.timing,3);frame.timingRecorded=true;}
   command.end();
   m->trimTextures(frame);
   const auto lightingEnd=std::chrono::steady_clock::now();
   frame.recordCpuMilliseconds={std::chrono::duration<double,std::milli>(planEnd-recordStart).count(),
      std::chrono::duration<double,std::milli>(uploadEnd-planEnd).count(),std::chrono::duration<double,std::milli>(shadowsEnd-uploadEnd).count(),
      std::chrono::duration<double,std::milli>(lightingEnd-shadowsEnd).count()};
   return command;
 }catch(const std::exception &failure){error=failure.what();return VK_NULL_HANDLE;}
}
VkImage VulkanWorldRenderer::HdrImage(uint32_t slot) const {
   if(slot>=m->frames.size() || !m->frames[slot].hdr)return VK_NULL_HANDLE;
   return *m->frames[slot].hdr->image;
}
void VulkanWorldRenderer::Composite(vk::CommandBuffer command,uint32_t slot,int x,int y,uint32_t width,uint32_t height) {
   auto &frame=m->frames.at(slot);command.bindPipeline(vk::PipelineBindPoint::eGraphics,frame.nativeDepth?*m->toneDepthPipeline:*m->tonePipeline);
   command.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,*m->tonePipelineLayout,0,frame.tone,{});
   command.setViewport(0,vk::Viewport(float(x),float(y),float(width),float(height),0,1));command.setScissor(0,vk::Rect2D({x,y},{width,height}));
   struct {float exposure;uint32_t srgb;float a,b,c;uint32_t depth;} constants{
      frame.exposure,(m->outputFormat==vk::Format::eB8G8R8A8Srgb || m->outputFormat==vk::Format::eR8G8B8A8Srgb)?1u:0u,
      frame.depthEncoding[0],frame.depthEncoding[1],frame.depthEncoding[2],frame.nativeDepth?1u:0u};
   static_assert(sizeof(constants)==24,"composite push constant ABI");
   command.pushConstants(*m->tonePipelineLayout,vk::ShaderStageFlagBits::eFragment,0,sizeof(constants),&constants);command.draw(3,1,0,0);
}
}
