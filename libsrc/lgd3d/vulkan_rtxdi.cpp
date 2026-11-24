#include "vulkan_rtxdi.hpp"
#include "vulkan_nrd.hpp"
#include <cmath>
#include <Rtxdi/RtxdiUtils.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <unordered_map>
namespace dark {
namespace {
struct Buffer {
   vk::Device device;vk::UniqueDeviceMemory memory;vk::UniqueBuffer buffer;void *mapped=nullptr;vk::DeviceSize capacity=0;
   ~Buffer(){if(mapped)device.unmapMemory(*memory);}
};
struct Image {vk::UniqueDeviceMemory memory;vk::UniqueImage image;vk::UniqueImageView view;vk::Format format;};
struct alignas(16) Constants {
   uint32_t dimensionsFrame[4],counts[4];float previousEye[4];
   RTXDI_ReservoirBufferParameters addressing;RTXDI_RuntimeParameters runtime;float jitterDelta[4];
};
static_assert(sizeof(Constants)==96 && sizeof(RTXDI_PackedDIReservoir)==24,"RTXDI shader ABI");
struct alignas(16) SharcConstants {uint32_t addresses[3][4];float tuning[4];uint32_t counts[4];};
static_assert(sizeof(SharcConstants)==80,"SHaRC shader ABI");
struct Frame {
   vk::UniqueDescriptorPool pool;
   std::unique_ptr<Buffer> config,remap,sharcConfig,statistics;
   Image diffuse,specular,denoisedDiffuse,denoisedSpecular;bool initialized=false;
   std::vector<std::unique_ptr<Buffer>> retired;
};
// NRD uses conventional camera coordinates (right/up/back). The raster
// projection maps downward screen Y into Vulkan's positive-height viewport.
// Undo that Y flip for NRD's internal UV reconstruction.
void nrdCamera(const RenderSceneCamera &camera,float *view,float *projection) {
   float basis[3][3]{};
   for(unsigned row=0;row<3;++row) {
      const unsigned source=row==2?3:row;float length=0;
      for(unsigned col=0;col<3;++col){basis[row][col]=camera.viewProjection[col*4+source];length+=basis[row][col]*basis[row][col];}
      if(length<1e-12f)throw std::runtime_error("invalid NRD camera basis");
      const float scale=(row==0?1.0f:-1.0f)/std::sqrt(length);
      for(auto &value:basis[row])value*=scale;
   }
   std::fill(view,view+16,0);view[15]=1;
   for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col){
      view[col*4+row]=basis[row][col];view[12+row]-=basis[row][col]*camera.position[col];
   }
   for(unsigned row=0;row<4;++row) {
      const float sign=row==1?-1.0f:1.0f;
      for(unsigned col=0;col<3;++col){projection[col*4+row]=0;
         for(unsigned k=0;k<3;++k)projection[col*4+row]+=sign*camera.viewProjection[k*4+row]*basis[col][k];}
      projection[12+row]=sign*camera.viewProjection[12+row];
      for(unsigned k=0;k<3;++k)projection[12+row]+=sign*camera.viewProjection[k*4+row]*camera.position[k];
   }
}
std::vector<uint32_t> shader(const std::string &path) {
   std::ifstream file(path,std::ios::binary|std::ios::ate);if(!file)throw std::runtime_error("missing RTXDI shader "+path);
   const auto size=file.tellg();if(size<=0 || size%4)throw std::runtime_error("invalid RTXDI shader size");
   std::vector<uint32_t> result(size_t(size)/4);file.seekg(0);file.read(reinterpret_cast<char*>(result.data()),size);
   if(!file)throw std::runtime_error("cannot read RTXDI shader");return result;
}
}
struct VulkanRtxdi::Impl {
   vk::Device device;vk::PhysicalDeviceMemoryProperties memory;vk::PhysicalDeviceProperties properties;
   uint32_t width=0,height=0,frameIndex=0;
   bool initialized=false,history=false,pending=false;
   std::array<Frame,2> frames;
   std::array<Image,4> previousSurfaces;
   std::unique_ptr<Buffer> reservoirs,neighbors,oldLights,oldDraws,oldLinks;
   vk::UniqueDescriptorSetLayout layout;vk::UniquePipelineLayout pipelineLayout;std::array<vk::UniquePipeline,6> pipelines;
   vk::UniqueSampler sampler;
   std::vector<RenderSceneHandle> previousIds,pendingIds;
   std::array<float,3> previousEye{},pendingEye{};
   RTXDI_ReservoirBufferParameters addressing{};
   VulkanNrd denoiser;RenderSceneCamera previousCamera{},pendingCamera{};
   uint32_t cacheEntries=1u<<20;
   std::array<std::unique_ptr<Buffer>,3> cache;
   PFN_vkGetBufferDeviceAddressKHR bufferAddress=nullptr;
   bool previousGi=false,pendingGi=false,previousCache=false,pendingCache=false;float previousScale=0,pendingScale=0;uint64_t materialRevision=0,pendingMaterialRevision=0;
   uint64_t lightsRevision=0,pendingLightsRevision=0;
   uint64_t texturesRevision=0,pendingTexturesRevision=0;
   uint32_t fastGiFrames=0,pendingFastGiFrames=0;
   uint32_t memoryType(uint32_t bits,vk::MemoryPropertyFlags wanted) {
      for(uint32_t i=0;i<memory.memoryTypeCount;++i)if((bits&(1u<<i)) && (memory.memoryTypes[i].propertyFlags&wanted)==wanted)return i;
      throw std::runtime_error("no RTXDI Vulkan memory type");
   }
   std::unique_ptr<Buffer> buffer(vk::DeviceSize bytes,vk::BufferUsageFlags usage,bool host=false) {
      auto result=std::make_unique<Buffer>();result->device=device;result->capacity=std::max<vk::DeviceSize>(bytes,256);
      if((usage&vk::BufferUsageFlagBits::eStorageBuffer) && result->capacity>properties.limits.maxStorageBufferRange)throw std::runtime_error("RTXDI buffer exceeds descriptor range limit");
      result->buffer=device.createBufferUnique(vk::BufferCreateInfo({},result->capacity,usage));
      const auto requirements=device.getBufferMemoryRequirements(*result->buffer);
      const auto flags=host?(vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent):vk::MemoryPropertyFlagBits::eDeviceLocal;
      vk::MemoryAllocateFlagsInfo allocationFlags(vk::MemoryAllocateFlagBits::eDeviceAddress);
      vk::MemoryAllocateInfo allocation(requirements.size,memoryType(requirements.memoryTypeBits,flags));
      if(usage&vk::BufferUsageFlagBits::eShaderDeviceAddress)allocation.pNext=&allocationFlags;
      result->memory=device.allocateMemoryUnique(allocation);
      device.bindBufferMemory(*result->buffer,*result->memory,0);if(host)result->mapped=device.mapMemory(*result->memory,0,VK_WHOLE_SIZE);return result;
   }
   Image image(vk::Format format) {
      Image result;result.format=format;
      result.image=device.createImageUnique(vk::ImageCreateInfo({},vk::ImageType::e2D,format,{width,height,1},1,1,vk::SampleCountFlagBits::e1,
         vk::ImageTiling::eOptimal,vk::ImageUsageFlagBits::eSampled|vk::ImageUsageFlagBits::eStorage|vk::ImageUsageFlagBits::eTransferSrc|vk::ImageUsageFlagBits::eTransferDst));
      const auto requirements=device.getImageMemoryRequirements(*result.image);
      result.memory=device.allocateMemoryUnique(vk::MemoryAllocateInfo(requirements.size,memoryType(requirements.memoryTypeBits,vk::MemoryPropertyFlagBits::eDeviceLocal)));
      device.bindImageMemory(*result.image,*result.memory,0);
      result.view=device.createImageViewUnique(vk::ImageViewCreateInfo({},*result.image,vk::ImageViewType::e2D,format,{}, {vk::ImageAspectFlagBits::eColor,0,1,0,1}));return result;
   }
   void barrier(vk::CommandBuffer command,vk::PipelineStageFlags destination,vk::AccessFlags access) {
      const vk::MemoryBarrier dependency(vk::AccessFlagBits::eMemoryRead|vk::AccessFlagBits::eMemoryWrite,access);
      command.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,destination,{},dependency,{},{});
   }
};
VulkanRtxdi::VulkanRtxdi():m(std::make_unique<Impl>()){}
VulkanRtxdi::~VulkanRtxdi()=default;
void VulkanRtxdi::Shutdown(){m=std::make_unique<Impl>();}
bool VulkanRtxdi::Initialize(vk::PhysicalDevice physical,vk::Device device,vk::DescriptorSetLayout sceneLayout,
   uint32_t width,uint32_t height,const std::string &directory,std::string &error,uint32_t cacheEntries) {
 try {
   Shutdown();
   if(cacheEntries<(1u<<16) || cacheEntries>(1u<<22) || (cacheEntries&(cacheEntries-1)))throw std::runtime_error("SHaRC capacity must be a power of two between 65536 and 4194304");
   m->cacheEntries=cacheEntries;
   if(!width || !height)throw std::runtime_error("invalid RTXDI dimensions");
   m->device=device;m->memory=physical.getMemoryProperties();m->properties=physical.getProperties();m->width=width;m->height=height;
   if(!m->denoiser.Initialize(physical,device,width,height,error))throw std::runtime_error(error);
   m->bufferAddress=reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(device.getProcAddr("vkGetBufferDeviceAddressKHR"));
   if(!m->bufferAddress)throw std::runtime_error("SHaRC requires buffer device addresses");
   for(unsigned i=0;i<3;++i)m->cache[i]=m->buffer(uint64_t(m->cacheEntries)*(i==0?8:16),
      vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eShaderDeviceAddress|vk::BufferUsageFlagBits::eTransferDst);
   m->addressing=rtxdi::CalculateReservoirBufferParameters(width,height,rtxdi::CheckerboardMode::Off);
   m->reservoirs=m->buffer(uint64_t(m->addressing.reservoirArrayPitch)*sizeof(RTXDI_PackedDIReservoir)*4,
      vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst);
   std::vector<uint8_t> packed(256*2);rtxdi::FillNeighborOffsetBuffer(packed.data(),256);
   std::vector<std::array<float,2>> offsets;
   for(unsigned i=0;i<256;++i)offsets.push_back({float(static_cast<int8_t>(packed[i*2]))/127,float(static_cast<int8_t>(packed[i*2+1]))/127});
   m->neighbors=m->buffer(offsets.size()*sizeof(offsets[0]),vk::BufferUsageFlagBits::eStorageBuffer,true);
   std::memcpy(m->neighbors->mapped,offsets.data(),offsets.size()*sizeof(offsets[0]));
   std::array<vk::Format,4> formats{{vk::Format::eR32G32B32A32Sfloat,vk::Format::eA2B10G10R10UnormPack32,
      vk::Format::eR16G16B16A16Sfloat,vk::Format::eR32G32B32A32Uint}};
   for(unsigned i=0;i<4;++i)m->previousSurfaces[i]=m->image(formats[i]);
   std::vector<vk::DescriptorSetLayoutBinding> bindings;
   for(uint32_t i=0;i<9;++i)bindings.emplace_back(i,vk::DescriptorType::eCombinedImageSampler,1,vk::ShaderStageFlagBits::eCompute);
   for(uint32_t i=9;i<15;++i)bindings.emplace_back(i,vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute);
   bindings.emplace_back(15,vk::DescriptorType::eUniformBuffer,1,vk::ShaderStageFlagBits::eCompute);
   for(uint32_t i=16;i<18;++i)bindings.emplace_back(i,vk::DescriptorType::eStorageImage,1,vk::ShaderStageFlagBits::eCompute);
   bindings.emplace_back(18,vk::DescriptorType::eUniformBuffer,1,vk::ShaderStageFlagBits::eCompute);
   bindings.emplace_back(19,vk::DescriptorType::eCombinedImageSampler,1,vk::ShaderStageFlagBits::eCompute);
   bindings.emplace_back(20,vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eCompute);
   m->layout=device.createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo({},bindings));
   const std::array<vk::DescriptorSetLayout,3> layouts{{sceneLayout,*m->layout,sceneLayout}};
   m->pipelineLayout=device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({},layouts));
   for(unsigned i=0;i<6;++i) {
      const std::string file=i<3?"rtxdi_"+std::to_string(i):i==3?"sharc_update":i==4?"sharc_resolve":"sharc_query";
      const auto code=shader(directory+"/"+file+".comp.spv");auto module=device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({},code));
      m->pipelines[i]=device.createComputePipelineUnique({},vk::ComputePipelineCreateInfo({},{{},vk::ShaderStageFlagBits::eCompute,*module,"main"},*m->pipelineLayout)).value;
   }
   m->sampler=device.createSamplerUnique(vk::SamplerCreateInfo({},vk::Filter::eNearest,vk::Filter::eNearest,vk::SamplerMipmapMode::eNearest,
      vk::SamplerAddressMode::eClampToEdge,vk::SamplerAddressMode::eClampToEdge,vk::SamplerAddressMode::eClampToEdge));
   for(auto &frame:m->frames) {
      const std::array<vk::DescriptorPoolSize,4> sizes{{{vk::DescriptorType::eCombinedImageSampler,10},{vk::DescriptorType::eStorageBuffer,7},
         {vk::DescriptorType::eUniformBuffer,2},{vk::DescriptorType::eStorageImage,2}}};
      frame.pool=device.createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo({},1,sizes));
      frame.config=m->buffer(sizeof(Constants),vk::BufferUsageFlagBits::eUniformBuffer,true);
      frame.sharcConfig=m->buffer(sizeof(SharcConstants),vk::BufferUsageFlagBits::eUniformBuffer,true);
      frame.statistics=m->buffer(16,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,true);
      frame.diffuse=m->image(vk::Format::eR16G16B16A16Sfloat);frame.specular=m->image(vk::Format::eR16G16B16A16Sfloat);
      frame.denoisedDiffuse=m->image(vk::Format::eR16G16B16A16Sfloat);frame.denoisedSpecular=m->image(vk::Format::eR16G16B16A16Sfloat);
   }
   return true;
 }catch(const std::exception &failure){error=failure.what();Shutdown();return false;}
}
bool VulkanRtxdi::Record(vk::CommandBuffer command,uint32_t slot,vk::DescriptorSet sceneSet,const Input &input,std::string &error) {
 try {
   if(!m->device || slot>=m->frames.size() || !input.lightIds)throw std::runtime_error("invalid RTXDI frame input");
   auto timestamp=[&](uint32_t index){if(input.timing)command.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe,input.timing,input.timestampBase+index);};
   timestamp(0);
   auto &frame=m->frames[slot];frame.retired.clear();
   const vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor,0,1,0,1);
   auto initializeImage=[&](const Image &image) {
      const vk::ImageMemoryBarrier transition({},vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eTransferWrite,
         vk::ImageLayout::eUndefined,vk::ImageLayout::eGeneral,VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,*image.image,range);
      command.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eTransfer,{}, {},{},transition);
   };
   if(!m->initialized) {
      for(const auto &image:m->previousSurfaces)initializeImage(image);
      command.fillBuffer(*m->reservoirs->buffer,0,VK_WHOLE_SIZE,0);
   }
   if(!frame.initialized){initializeImage(frame.diffuse);initializeImage(frame.specular);initializeImage(frame.denoisedDiffuse);initializeImage(frame.denoisedSpecular);}
   auto previousBuffer=[&](std::unique_ptr<Buffer> &buffer,vk::DeviceSize bytes) {
      if(bytes==VK_WHOLE_SIZE || !bytes)throw std::runtime_error("RTXDI inputs require explicit descriptor ranges");
      if(!buffer || buffer->capacity<bytes) {
         auto replacement=m->buffer(bytes,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst);
         if(buffer) {
            m->barrier(command,vk::PipelineStageFlagBits::eTransfer,vk::AccessFlagBits::eTransferRead|vk::AccessFlagBits::eTransferWrite);
            command.copyBuffer(*buffer->buffer,*replacement->buffer,vk::BufferCopy(0,0,buffer->capacity));
            frame.retired.push_back(std::move(buffer));
         }
         buffer=std::move(replacement);
      }
   };
   previousBuffer(m->oldLights,input.lights.range);previousBuffer(m->oldDraws,input.draws.range);previousBuffer(m->oldLinks,input.links.range);
   std::unordered_map<RenderSceneHandle,int32_t> currentMap,oldMap;
   for(uint32_t i=0;i<input.lightIds->size();++i)currentMap.emplace(input.lightIds->at(i),int32_t(i));
   for(uint32_t i=0;i<m->previousIds.size();++i)oldMap.emplace(m->previousIds[i],int32_t(i));
   std::vector<int32_t> remap;remap.reserve(input.lightIds->size()+m->previousIds.size());
   for(const auto id:*input.lightIds){const auto found=oldMap.find(id);remap.push_back(found==oldMap.end()?-1:found->second);}
   for(const auto id:m->previousIds){const auto found=currentMap.find(id);remap.push_back(found==currentMap.end()?-1:found->second);}
   const size_t remapBytes=std::max<size_t>(remap.size()*4,16);
   if(!frame.remap || frame.remap->capacity<remapBytes)frame.remap=m->buffer(remapBytes,vk::BufferUsageFlagBits::eStorageBuffer,true);
   if(!remap.empty())std::memcpy(frame.remap->mapped,remap.data(),remap.size()*4);
   Constants constants{};constants.dimensionsFrame[0]=m->width;constants.dimensionsFrame[1]=m->height;constants.dimensionsFrame[2]=m->frameIndex;
   for(unsigned axis=0;axis<2;++axis)constants.jitterDelta[axis]=input.jitter[axis]-input.previousJitter[axis];
   constants.dimensionsFrame[3]=m->history && input.historyValid?1:0;
   const bool changed=!constants.dimensionsFrame[3] || !m->previousGi || input.geometryChanged ||
      input.lightsRevision!=m->lightsRevision || input.materialRevision!=m->materialRevision || input.texturesRevision!=m->texturesRevision ||
      input.camera.lightScale!=m->previousCamera.lightScale;
   m->pendingFastGiFrames=changed?16u:(m->fastGiFrames?m->fastGiFrames-1:0u);
   m->pendingLightsRevision=input.lightsRevision;
   m->pendingTexturesRevision=input.texturesRevision;
   constants.counts[0]=uint32_t(input.lightIds->size());constants.counts[1]=uint32_t(m->previousIds.size());constants.counts[2]=16;
   // Spatial output and submitted history exchange slots; no full-screen
   // reservoir copy is needed. Rejected submissions never advance frameIndex.
   constants.counts[3]=2+(m->frameIndex&1);
   std::copy(m->previousEye.begin(),m->previousEye.end(),constants.previousEye);constants.addressing=m->addressing;
   constants.runtime.neighborOffsetMask=255;constants.runtime.frameIndex=m->frameIndex;
   std::memcpy(frame.config->mapped,&constants,sizeof(constants));
   SharcConstants sharc{};
   for(unsigned i=0;i<3;++i) {
      const VkBufferDeviceAddressInfo info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,nullptr,*m->cache[i]->buffer};
      const uint64_t address=m->bufferAddress(m->device,&info);
      if(!address)throw std::runtime_error("null SHaRC buffer device address");
      sharc.addresses[i][0]=uint32_t(address);sharc.addresses[i][1]=uint32_t(address>>32);
   }
   sharc.addresses[0][2]=input.cacheEnabled?1u:0u;sharc.addresses[0][3]=input.collectStatistics?1u:0u;
   sharc.tuning[0]=input.sharcSceneScale;sharc.tuning[1]=.4f;sharc.tuning[2]=1000;sharc.tuning[3]=input.giStrength;
   sharc.counts[0]=m->cacheEntries;sharc.counts[1]=std::clamp(input.sharcDownscale,1u,10u);
   sharc.counts[2]=std::clamp(input.sharcAccumulation,1u,100u);sharc.counts[3]=std::clamp(input.sharcStaleFrames,8u,100u);
   if(m->pendingFastGiFrames) {
      // Bound old signal after edits without clearing the spatial cache. A
      // stationary scene resumes the configured accumulation/staleness limits.
      sharc.counts[2]=1;sharc.counts[3]=std::min(sharc.counts[3],8u);
   }
   std::memcpy(frame.sharcConfig->mapped,&sharc,sizeof(sharc));
   m->device.resetDescriptorPool(*frame.pool);
   const auto set=m->device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo(*frame.pool,1,&*m->layout))[0];
   const uint32_t currentAttachments[5]={0,1,2,4,5};
   std::vector<vk::DescriptorImageInfo> images;images.reserve(12);
   for(unsigned i=0;i<5;++i)images.emplace_back(*m->sampler,input.surface[currentAttachments[i]].view,vk::ImageLayout::eGeneral);
   for(unsigned i=0;i<4;++i)images.emplace_back(*m->sampler,*m->previousSurfaces[i].view,vk::ImageLayout::eGeneral);
   images.emplace_back(vk::Sampler{},*frame.diffuse.view,vk::ImageLayout::eGeneral);
   images.emplace_back(vk::Sampler{},*frame.specular.view,vk::ImageLayout::eGeneral);
   images.emplace_back(*m->sampler,input.surface[3].view,vk::ImageLayout::eGeneral);
   const vk::DescriptorBufferInfo statisticsBuffer(*frame.statistics->buffer,0,16);
   const vk::DescriptorBufferInfo sharcBuffer(*frame.sharcConfig->buffer,0,sizeof(SharcConstants));
   const std::array<vk::DescriptorBufferInfo,7> buffers{{
      {*m->reservoirs->buffer,0,m->reservoirs->capacity},{*m->neighbors->buffer,0,m->neighbors->capacity},
      {*m->oldLights->buffer,0,m->oldLights->capacity},{*m->oldDraws->buffer,0,m->oldDraws->capacity},
      {*m->oldLinks->buffer,0,m->oldLinks->capacity},{*frame.remap->buffer,0,frame.remap->capacity},{*frame.config->buffer,0,sizeof(Constants)}}};
   std::vector<vk::WriteDescriptorSet> writes;
   for(uint32_t i=0;i<9;++i)writes.emplace_back(set,i,0,1,vk::DescriptorType::eCombinedImageSampler,&images[i]);
   for(uint32_t i=0;i<7;++i)writes.emplace_back(set,9+i,0,1,i==6?vk::DescriptorType::eUniformBuffer:vk::DescriptorType::eStorageBuffer,nullptr,&buffers[i]);
   for(uint32_t i=0;i<2;++i)writes.emplace_back(set,16+i,0,1,vk::DescriptorType::eStorageImage,&images[9+i]);
   writes.emplace_back(set,20,0,1,vk::DescriptorType::eStorageBuffer,nullptr,&statisticsBuffer);
   writes.emplace_back(set,18,0,1,vk::DescriptorType::eUniformBuffer,nullptr,&sharcBuffer);
   writes.emplace_back(set,19,0,1,vk::DescriptorType::eCombinedImageSampler,&images[11]);
   m->device.updateDescriptorSets(writes,{});
   const std::array<vk::DescriptorSet,3> sets{{sceneSet,set,input.previousSceneSet}};
   command.bindDescriptorSets(vk::PipelineBindPoint::eCompute,*m->pipelineLayout,0,sets,{});
   for(unsigned pass=0;pass<3;++pass) {
      m->barrier(command,vk::PipelineStageFlagBits::eComputeShader,vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eShaderWrite);
      command.bindPipeline(vk::PipelineBindPoint::eCompute,*m->pipelines[pass]);command.dispatch((m->width+7)/8,(m->height+7)/8,1);
      if(input.timing && pass<2 && input.directTimestampBase!=UINT32_MAX)
         command.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe,input.timing,input.directTimestampBase+pass);
   }
   timestamp(1);
   m->barrier(command,vk::PipelineStageFlagBits::eTransfer,vk::AccessFlagBits::eTransferWrite);
   command.fillBuffer(*frame.statistics->buffer,0,VK_WHOLE_SIZE,0);
   if(input.gi) {
      // Reuse only cache contents from accepted submissions in the same scene.
      if(!constants.dimensionsFrame[3] || !m->previousGi || !m->previousCache || m->previousScale!=input.sharcSceneScale || m->materialRevision!=input.materialRevision || m->previousIds!=*input.lightIds) {
         m->barrier(command,vk::PipelineStageFlagBits::eTransfer,vk::AccessFlagBits::eTransferWrite);
         // HASH_GRID_INVALID_HASH_KEY is zero; all SDK accumulation/resolved
         // entries are also zero initialized (8/16/16-byte strides).
         for(const auto &buffer:m->cache)command.fillBuffer(*buffer->buffer,0,VK_WHOLE_SIZE,0);
      }
      for(unsigned pass=input.cacheEnabled?3:5;pass<6;++pass) {
         // Update writes must reach Resolve; resolved radiance must reach Query.
         m->barrier(command,vk::PipelineStageFlagBits::eComputeShader,vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eShaderWrite);
         command.bindPipeline(vk::PipelineBindPoint::eCompute,*m->pipelines[pass]);
         if(pass==4)command.dispatch((m->cacheEntries+255)/256,1,1);
         else {const uint32_t scale=pass==3?sharc.counts[1]:1;
            command.dispatch(((m->width+scale-1)/scale+7)/8,((m->height+scale-1)/scale+7)/8,1);}
      }
   }
   timestamp(2);
   m->pendingCache=input.cacheEnabled;m->pendingScale=input.sharcSceneScale;m->pendingGi=input.gi;m->pendingMaterialRevision=input.materialRevision;
   VulkanNrd::Resources resources{};
   resources[size_t(nrd::ResourceType::IN_MV)]={input.surface[4].image,input.surface[4].view};
   resources[size_t(nrd::ResourceType::IN_NORMAL_ROUGHNESS)]={input.surface[1].image,input.surface[1].view};
   resources[size_t(nrd::ResourceType::IN_VIEWZ)]={input.surface[0].image,input.linearDepth};
   resources[size_t(nrd::ResourceType::IN_DIFF_RADIANCE_HITDIST)]={*frame.diffuse.image,*frame.diffuse.view};
   resources[size_t(nrd::ResourceType::IN_SPEC_RADIANCE_HITDIST)]={*frame.specular.image,*frame.specular.view};
   resources[size_t(nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST)]={*frame.denoisedDiffuse.image,*frame.denoisedDiffuse.view};
   resources[size_t(nrd::ResourceType::OUT_SPEC_RADIANCE_HITDIST)]={*frame.denoisedSpecular.image,*frame.denoisedSpecular.view};
   nrd::CommonSettings common;
   nrdCamera(input.camera,common.worldToViewMatrix,common.viewToClipMatrix);
   nrdCamera(constants.dimensionsFrame[3]?m->previousCamera:input.camera,common.worldToViewMatrixPrev,common.viewToClipMatrixPrev);
   common.resourceSize[0]=common.resourceSizePrev[0]=common.rectSize[0]=common.rectSizePrev[0]=uint16_t(m->width);
   common.resourceSize[1]=common.resourceSizePrev[1]=common.rectSize[1]=common.rectSizePrev[1]=uint16_t(m->height);
   std::copy(input.jitter.begin(),input.jitter.end(),common.cameraJitter);
   std::copy(input.previousJitter.begin(),input.previousJitter.end(),common.cameraJitterPrev);
   common.frameIndex=m->frameIndex;
   common.timeDeltaBetweenFrames=input.denoiserFrameTimeMilliseconds;
   if(!constants.dimensionsFrame[3] || m->previousGi!=input.gi)m->denoiser.ResetHistory();
   if(!m->denoiser.Record(command,slot,resources,common,error))throw std::runtime_error(error);
   timestamp(3);
   m->pendingCamera=input.camera;
   // GPU-only history copies avoid racing CPU writes to a preceding frame
   // slot's mapped data while the next frame is still reading it.
   m->barrier(command,vk::PipelineStageFlagBits::eTransfer,vk::AccessFlagBits::eTransferRead|vk::AccessFlagBits::eTransferWrite);
   const uint32_t historyAttachments[4]={0,1,2,5}; // Motion is only consumed in the current frame.
   for(unsigned i=0;i<4;++i) {
      const auto &source=input.surface[historyAttachments[i]];
      const vk::ImageCopy copy({vk::ImageAspectFlagBits::eColor,0,0,1},{0,0,0},{vk::ImageAspectFlagBits::eColor,0,0,1},{0,0,0},{m->width,m->height,1});
      command.copyImage(source.image,vk::ImageLayout::eGeneral,*m->previousSurfaces[i].image,vk::ImageLayout::eGeneral,copy);
   }
   for(const auto &entry:{std::pair<vk::DescriptorBufferInfo,Buffer*>{input.lights,m->oldLights.get()},
      {input.draws,m->oldDraws.get()},{input.links,m->oldLinks.get()}})
      command.copyBuffer(entry.first.buffer,*entry.second->buffer,vk::BufferCopy(entry.first.offset,0,entry.first.range));
   m->barrier(command,vk::PipelineStageFlagBits::eComputeShader,vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eShaderWrite);
   const vk::MemoryBarrier statisticsReady(vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eTransferWrite,vk::AccessFlagBits::eHostRead);
   command.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},statisticsReady,{},{});
   timestamp(4);
   m->pendingIds=*input.lightIds;std::copy(input.eye,input.eye+3,m->pendingEye.begin());m->pending=true;
   frame.initialized=true;return true;
 }catch(const std::exception &failure){error=failure.what();m->denoiser.CompleteSubmission(false);m->history=false;m->initialized=false;m->pending=false;return false;}
}
void VulkanRtxdi::CompleteSubmission(bool accepted) {
   m->denoiser.CompleteSubmission(accepted && m->pending);
   if(accepted && m->pending) {
      m->previousIds=std::move(m->pendingIds);
      m->previousEye=m->pendingEye;
      m->previousCamera=m->pendingCamera;
      m->previousCache=m->pendingCache;
      m->previousScale=m->pendingScale;
      m->previousGi=m->pendingGi;
      m->materialRevision=m->pendingMaterialRevision;
      m->lightsRevision=m->pendingLightsRevision;
      m->texturesRevision=m->pendingTexturesRevision;
      m->fastGiFrames=m->pendingFastGiFrames;
      ++m->frameIndex;
      m->history=true;
      m->initialized=true;
   } else {
      m->denoiser.ResetHistory();
      m->history=false;
      m->initialized=false;
      for(auto &frame:m->frames)frame.initialized=false;
   }
   m->pending=false;
}
VulkanRtxdi::Texture VulkanRtxdi::Diffuse(uint32_t slot) const {const auto &i=m->frames.at(slot).diffuse;return {*i.image,*i.view,i.format};}
VulkanRtxdi::Texture VulkanRtxdi::Specular(uint32_t slot) const {const auto &i=m->frames.at(slot).specular;return {*i.image,*i.view,i.format};}
VulkanRtxdi::Texture VulkanRtxdi::DenoisedDiffuse(uint32_t slot) const {const auto &i=m->frames.at(slot).denoisedDiffuse;return {*i.image,*i.view,i.format};}
VulkanRtxdi::Texture VulkanRtxdi::DenoisedSpecular(uint32_t slot) const {const auto &i=m->frames.at(slot).denoisedSpecular;return {*i.image,*i.view,i.format};}

std::array<uint32_t,4> VulkanRtxdi::Statistics(uint32_t slot) const {
   std::array<uint32_t,4> result{};if(slot<2 && m->frames[slot].statistics)std::memcpy(result.data(),m->frames[slot].statistics->mapped,sizeof(result));return result;
}

}
