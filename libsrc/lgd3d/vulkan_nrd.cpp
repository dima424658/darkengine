// Vulkan integration reference: nvpro-samples/vk_denoise_nrd, revision
// 67f28c7c6a7bd2495bd6382dac8cffbf85673858 (Apache-2.0 sample).
#include "vulkan_nrd.hpp"
#include <algorithm>
#include <vector>
#include <stdexcept>
namespace dark {
namespace {
void require(nrd::Result value,const char *operation){if(value!=nrd::Result::SUCCESS)throw std::runtime_error(operation);}
vk::Format imageFormat(nrd::Format value) {switch(value) {
   case nrd::Format::R8_UNORM:return vk::Format(VK_FORMAT_R8_UNORM);
   case nrd::Format::R8_SNORM:return vk::Format(VK_FORMAT_R8_SNORM);
   case nrd::Format::R8_UINT:return vk::Format(VK_FORMAT_R8_UINT);
   case nrd::Format::R8_SINT:return vk::Format(VK_FORMAT_R8_SINT);
   case nrd::Format::RG8_UNORM:return vk::Format(VK_FORMAT_R8G8_UNORM);
   case nrd::Format::RG8_SNORM:return vk::Format(VK_FORMAT_R8G8_SNORM);
   case nrd::Format::RG8_UINT:return vk::Format(VK_FORMAT_R8G8_UINT);
   case nrd::Format::RG8_SINT:return vk::Format(VK_FORMAT_R8G8_SINT);
   case nrd::Format::RGBA8_UNORM:return vk::Format(VK_FORMAT_R8G8B8A8_UNORM);
   case nrd::Format::RGBA8_SNORM:return vk::Format(VK_FORMAT_R8G8B8A8_SNORM);
   case nrd::Format::RGBA8_UINT:return vk::Format(VK_FORMAT_R8G8B8A8_UINT);
   case nrd::Format::RGBA8_SINT:return vk::Format(VK_FORMAT_R8G8B8A8_SINT);
   case nrd::Format::RGBA8_SRGB:return vk::Format(VK_FORMAT_R8G8B8A8_SRGB);
   case nrd::Format::R16_UNORM:return vk::Format(VK_FORMAT_R16_UNORM);
   case nrd::Format::R16_SNORM:return vk::Format(VK_FORMAT_R16_SNORM);
   case nrd::Format::R16_UINT:return vk::Format(VK_FORMAT_R16_UINT);
   case nrd::Format::R16_SINT:return vk::Format(VK_FORMAT_R16_SINT);
   case nrd::Format::R16_SFLOAT:return vk::Format(VK_FORMAT_R16_SFLOAT);
   case nrd::Format::RG16_UNORM:return vk::Format(VK_FORMAT_R16G16_UNORM);
   case nrd::Format::RG16_SNORM:return vk::Format(VK_FORMAT_R16G16_SNORM);
   case nrd::Format::RG16_UINT:return vk::Format(VK_FORMAT_R16G16_UINT);
   case nrd::Format::RG16_SINT:return vk::Format(VK_FORMAT_R16G16_SINT);
   case nrd::Format::RG16_SFLOAT:return vk::Format(VK_FORMAT_R16G16_SFLOAT);
   case nrd::Format::RGBA16_UNORM:return vk::Format(VK_FORMAT_R16G16B16A16_UNORM);
   case nrd::Format::RGBA16_SNORM:return vk::Format(VK_FORMAT_R16G16B16A16_SNORM);
   case nrd::Format::RGBA16_UINT:return vk::Format(VK_FORMAT_R16G16B16A16_UINT);
   case nrd::Format::RGBA16_SINT:return vk::Format(VK_FORMAT_R16G16B16A16_SINT);
   case nrd::Format::RGBA16_SFLOAT:return vk::Format(VK_FORMAT_R16G16B16A16_SFLOAT);
   case nrd::Format::R32_UINT:return vk::Format(VK_FORMAT_R32_UINT);
   case nrd::Format::R32_SINT:return vk::Format(VK_FORMAT_R32_SINT);
   case nrd::Format::R32_SFLOAT:return vk::Format(VK_FORMAT_R32_SFLOAT);
   case nrd::Format::RG32_UINT:return vk::Format(VK_FORMAT_R32G32_UINT);
   case nrd::Format::RG32_SINT:return vk::Format(VK_FORMAT_R32G32_SINT);
   case nrd::Format::RG32_SFLOAT:return vk::Format(VK_FORMAT_R32G32_SFLOAT);
   case nrd::Format::RGB32_UINT:return vk::Format(VK_FORMAT_R32G32B32_UINT);
   case nrd::Format::RGB32_SINT:return vk::Format(VK_FORMAT_R32G32B32_SINT);
   case nrd::Format::RGB32_SFLOAT:return vk::Format(VK_FORMAT_R32G32B32_SFLOAT);
   case nrd::Format::RGBA32_UINT:return vk::Format(VK_FORMAT_R32G32B32A32_UINT);
   case nrd::Format::RGBA32_SINT:return vk::Format(VK_FORMAT_R32G32B32A32_SINT);
   case nrd::Format::RGBA32_SFLOAT:return vk::Format(VK_FORMAT_R32G32B32A32_SFLOAT);
   case nrd::Format::R10_G10_B10_A2_UNORM:return vk::Format(VK_FORMAT_A2B10G10R10_UNORM_PACK32);
   case nrd::Format::R10_G10_B10_A2_UINT:return vk::Format(VK_FORMAT_A2B10G10R10_UINT_PACK32);
   case nrd::Format::R11_G11_B10_UFLOAT:return vk::Format(VK_FORMAT_B10G11R11_UFLOAT_PACK32);
   case nrd::Format::R9_G9_B9_E5_UFLOAT:return vk::Format(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32);
   default:throw std::runtime_error("unsupported NRD image format");
}}
struct Image {vk::UniqueDeviceMemory memory;vk::UniqueImage image;vk::UniqueImageView view;};
struct Pipeline {vk::UniqueDescriptorSetLayout resources;vk::UniquePipelineLayout layout;vk::UniquePipeline pipeline;};
}
struct VulkanNrd::Impl {
   vk::Device device;
   vk::PhysicalDeviceMemoryProperties memory;
   nrd::Instance *instance=nullptr;
   nrd::Identifier identifier=0;
   bool initializedImages=false,reset=true,pending=false;
   uint32_t width=0,height=0;
   vk::UniqueDeviceMemory constantMemory;
   vk::UniqueBuffer constants;
   std::vector<vk::UniqueSampler> samplers;
   vk::UniqueDescriptorSetLayout commonLayout;
   std::array<vk::UniqueDescriptorPool,2> pools;
   std::vector<Image> permanent,transient;
   std::vector<Pipeline> pipelines;
   ~Impl(){if(instance)nrd::DestroyInstance(*instance);}
   uint32_t memoryType(uint32_t bits,vk::MemoryPropertyFlags flags) {
      for(uint32_t i=0;i<memory.memoryTypeCount;++i)if((bits&(1u<<i)) && (memory.memoryTypes[i].propertyFlags&flags)==flags)return i;
      throw std::runtime_error("no NRD Vulkan memory type");
   }
   Image image(const nrd::TextureDesc &description) {
      if(!description.downsampleFactor)throw std::runtime_error("invalid NRD downsample factor");
      const uint32_t w=(width+description.downsampleFactor-1)/description.downsampleFactor;
      const uint32_t h=(height+description.downsampleFactor-1)/description.downsampleFactor;
      const auto format=imageFormat(description.format);Image result;
      result.image=device.createImageUnique(vk::ImageCreateInfo({},vk::ImageType::e2D,format,{w,h,1},1,1,vk::SampleCountFlagBits::e1,
         vk::ImageTiling::eOptimal,vk::ImageUsageFlagBits::eSampled|vk::ImageUsageFlagBits::eStorage|vk::ImageUsageFlagBits::eTransferDst));
      const auto requirements=device.getImageMemoryRequirements(*result.image);
      result.memory=device.allocateMemoryUnique(vk::MemoryAllocateInfo(requirements.size,memoryType(requirements.memoryTypeBits,vk::MemoryPropertyFlagBits::eDeviceLocal)));
      device.bindImageMemory(*result.image,*result.memory,0);
      result.view=device.createImageViewUnique(vk::ImageViewCreateInfo({},*result.image,vk::ImageViewType::e2D,format,{}, {vk::ImageAspectFlagBits::eColor,0,1,0,1}));
      return result;
   }
};
VulkanNrd::VulkanNrd():m(std::make_unique<Impl>()){}
VulkanNrd::~VulkanNrd()=default;
void VulkanNrd::Shutdown(){m=std::make_unique<Impl>();}
void VulkanNrd::ResetHistory(){m->reset=true;}
void VulkanNrd::CompleteSubmission(bool accepted){
   if(accepted && m->pending){m->initializedImages=true;m->reset=false;}
   else m->reset=true;
   m->pending=false;
}
bool VulkanNrd::Initialize(vk::PhysicalDevice physical,vk::Device device,uint32_t width,uint32_t height,std::string &error) {
 try {
   Shutdown();
   if(!width || !height || width>65535 || height>65535)throw std::runtime_error("invalid NRD dimensions");
   m->device=device;m->memory=physical.getMemoryProperties();m->width=width;m->height=height;
   const nrd::DenoiserDesc denoiser{m->identifier,nrd::Denoiser::RELAX_DIFFUSE_SPECULAR};
   const nrd::InstanceCreationDesc creation{{},&denoiser,1};require(nrd::CreateInstance(creation,m->instance),"create NRD RELAX instance");
   const auto &desc=*nrd::GetInstanceDesc(*m->instance);const auto &offset=nrd::GetLibraryDesc()->spirvBindingOffsets;
   if(desc.constantBufferAndSamplersSpaceIndex==desc.resourcesSpaceIndex || std::max(desc.constantBufferAndSamplersSpaceIndex,desc.resourcesSpaceIndex)>1)
      throw std::runtime_error("unsupported NRD descriptor spaces");
   m->constants=device.createBufferUnique(vk::BufferCreateInfo({},desc.constantBufferMaxDataSize,vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst));
   const auto requirements=device.getBufferMemoryRequirements(*m->constants);
   m->constantMemory=device.allocateMemoryUnique(vk::MemoryAllocateInfo(requirements.size,m->memoryType(requirements.memoryTypeBits,vk::MemoryPropertyFlagBits::eDeviceLocal)));
   device.bindBufferMemory(*m->constants,*m->constantMemory,0);
   for(uint32_t i=0;i<desc.permanentPoolSize;++i)m->permanent.push_back(m->image(desc.permanentPool[i]));
   for(uint32_t i=0;i<desc.transientPoolSize;++i)m->transient.push_back(m->image(desc.transientPool[i]));
   std::vector<vk::DescriptorSetLayoutBinding> common{{desc.constantBufferRegisterIndex+offset.constantBufferOffset,vk::DescriptorType::eUniformBuffer,1,vk::ShaderStageFlagBits::eCompute}};
   for(uint32_t i=0;i<desc.samplersNum;++i) {
      const auto filter=desc.samplers[i]==nrd::Sampler::NEAREST_CLAMP?vk::Filter::eNearest:vk::Filter::eLinear;
      m->samplers.push_back(device.createSamplerUnique(vk::SamplerCreateInfo({},filter,filter,vk::SamplerMipmapMode::eNearest,
         vk::SamplerAddressMode::eClampToEdge,vk::SamplerAddressMode::eClampToEdge,vk::SamplerAddressMode::eClampToEdge)));
      common.emplace_back(desc.samplersBaseRegisterIndex+offset.samplerOffset+i,vk::DescriptorType::eSampler,1,vk::ShaderStageFlagBits::eCompute);
   }
   m->commonLayout=device.createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo({},common));
   for(uint32_t i=0;i<desc.pipelinesNum;++i) {
      const auto &source=desc.pipelines[i];Pipeline pipeline;std::vector<vk::DescriptorSetLayoutBinding> bindings;
      for(uint32_t r=0;r<source.resourceRangesNum;++r) {
         const auto &range=source.resourceRanges[r];const bool storage=range.descriptorType==nrd::DescriptorType::STORAGE_TEXTURE;
         for(uint32_t j=0;j<range.descriptorsNum;++j)bindings.emplace_back(desc.resourcesBaseRegisterIndex+j+(storage?offset.storageTextureAndBufferOffset:offset.textureOffset),
            storage?vk::DescriptorType::eStorageImage:vk::DescriptorType::eSampledImage,1,vk::ShaderStageFlagBits::eCompute);
      }
      pipeline.resources=device.createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo({},bindings));
      std::array<vk::DescriptorSetLayout,2> layouts;layouts[desc.constantBufferAndSamplersSpaceIndex]=*m->commonLayout;layouts[desc.resourcesSpaceIndex]=*pipeline.resources;
      pipeline.layout=device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({},layouts));
      if(!source.computeShaderSPIRV.bytecode || !source.computeShaderSPIRV.size)throw std::runtime_error("NRD SPIR-V bytecode unavailable");
      auto module=device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({},size_t(source.computeShaderSPIRV.size),static_cast<const uint32_t*>(source.computeShaderSPIRV.bytecode)));
      pipeline.pipeline=device.createComputePipelineUnique({},vk::ComputePipelineCreateInfo({},{{},vk::ShaderStageFlagBits::eCompute,*module,desc.shaderEntryPoint},*pipeline.layout)).value;
      m->pipelines.push_back(std::move(pipeline));
   }
   for(auto &pool:m->pools) {
      const auto &limits=desc.descriptorPoolDesc;
      const std::array<vk::DescriptorPoolSize,4> sizes{{{vk::DescriptorType::eSampledImage,std::max(1u,limits.totalTexturesNum)},
         {vk::DescriptorType::eStorageImage,std::max(1u,limits.totalStorageTexturesNum)},
         {vk::DescriptorType::eUniformBuffer,1},{vk::DescriptorType::eSampler,desc.samplersNum}}};
      pool=device.createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo({},limits.setsMaxNum+1,sizes));
   }
   nrd::RelaxSettings settings{};
   // ReSTIR has already resampled direct lighting. The SDK defaults use a
   // 30/50-pixel preblur, which washes narrow shadows into fully occluded
   // receivers. Keep a small prefilter for indirect noise and make hit-distance
   // rejection sensitive to shadow boundaries (SDK guidance for RTXDI signals).
   settings.diffusePrepassBlurRadius=2;settings.specularPrepassBlurRadius=4;
   settings.minHitDistanceWeight=.01f;
   settings.diffuseMaxAccumulatedFrameNum=settings.specularMaxAccumulatedFrameNum=16;
   settings.diffuseMaxFastAccumulatedFrameNum=settings.specularMaxFastAccumulatedFrameNum=3;
   settings.historyFixBasePixelStride=settings.historyFixAlternatePixelStride=4;
   settings.atrousIterationNum=4;
   require(nrd::SetDenoiserSettings(*m->instance,m->identifier,&settings),"configure NRD RELAX");
   return true;
 }catch(const std::exception &failure){error=failure.what();Shutdown();return false;}
}
bool VulkanNrd::Record(vk::CommandBuffer command,uint32_t slot,const Resources &resources,
   const nrd::CommonSettings &settings,std::string &error) {
 try {
   if(!m->instance || slot>=m->pools.size())throw std::runtime_error("invalid NRD instance or frame slot");
   const auto &desc=*nrd::GetInstanceDesc(*m->instance);const auto &offset=nrd::GetLibraryDesc()->spirvBindingOffsets;
   auto common=settings;
   if(m->reset)common.accumulationMode=nrd::AccumulationMode::CLEAR_AND_RESTART;
   require(nrd::SetCommonSettings(*m->instance,common),"set NRD frame settings");
   const nrd::DispatchDesc *dispatches=nullptr;uint32_t count=0;
   require(nrd::GetComputeDispatches(*m->instance,&m->identifier,1,dispatches,count),"get NRD dispatches");
   if(!m->initializedImages) {
      const vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor,0,1,0,1);
      for(auto *pool:{&m->permanent,&m->transient})for(const auto &image:*pool) {
         vk::ImageMemoryBarrier transition({},vk::AccessFlagBits::eTransferWrite,vk::ImageLayout::eUndefined,vk::ImageLayout::eTransferDstOptimal,
            VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,*image.image,range);
         command.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,vk::PipelineStageFlagBits::eTransfer,{}, {},{},transition);
         command.clearColorImage(*image.image,vk::ImageLayout::eTransferDstOptimal,vk::ClearColorValue(std::array<uint32_t,4>{}),range);
         transition.srcAccessMask=vk::AccessFlagBits::eTransferWrite;transition.dstAccessMask=vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eShaderWrite;
         transition.oldLayout=vk::ImageLayout::eTransferDstOptimal;transition.newLayout=vk::ImageLayout::eGeneral;
         command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eComputeShader,{}, {},{},transition);
      }
   }
   m->device.resetDescriptorPool(*m->pools[slot]);
   const auto commonSet=m->device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo(*m->pools[slot],1,&*m->commonLayout))[0];
   const vk::DescriptorBufferInfo constants(*m->constants,0,desc.constantBufferMaxDataSize);
   std::vector<vk::DescriptorImageInfo> samplerInfos;samplerInfos.reserve(m->samplers.size());
   for(const auto &sampler:m->samplers)samplerInfos.emplace_back(*sampler,vk::ImageView{},vk::ImageLayout::eUndefined);
   std::vector<vk::WriteDescriptorSet> commonWrites;
   commonWrites.emplace_back(commonSet,desc.constantBufferRegisterIndex+offset.constantBufferOffset,0,1,vk::DescriptorType::eUniformBuffer,nullptr,&constants);
   for(uint32_t i=0;i<samplerInfos.size();++i)commonWrites.emplace_back(commonSet,desc.samplersBaseRegisterIndex+offset.samplerOffset+i,0,1,vk::DescriptorType::eSampler,&samplerInfos[i]);
   m->device.updateDescriptorSets(commonWrites,{});
   for(uint32_t d=0;d<count;++d) {
      const auto &dispatch=dispatches[d];const auto &source=desc.pipelines[dispatch.pipelineIndex];auto &pipeline=m->pipelines.at(dispatch.pipelineIndex);
      const auto set=m->device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo(*m->pools[slot],1,&*pipeline.resources))[0];
      std::vector<vk::DescriptorImageInfo> images;images.reserve(dispatch.resourcesNum);
      std::vector<vk::WriteDescriptorSet> writes;writes.reserve(dispatch.resourcesNum);
      uint32_t index=0;
      for(uint32_t r=0;r<source.resourceRangesNum;++r) {
         const auto &range=source.resourceRanges[r];const bool storage=range.descriptorType==nrd::DescriptorType::STORAGE_TEXTURE;
         for(uint32_t j=0;j<range.descriptorsNum;++j) {
            if(index>=dispatch.resourcesNum)throw std::runtime_error("invalid NRD resource count");
            const auto &binding=dispatch.resources[index++];Texture texture;
            if(binding.type==nrd::ResourceType::PERMANENT_POOL) {
               const auto &image=m->permanent.at(binding.indexInPool);texture={*image.image,*image.view};
            } else if(binding.type==nrd::ResourceType::TRANSIENT_POOL) {
               const auto &image=m->transient.at(binding.indexInPool);texture={*image.image,*image.view};
            } else texture=resources.at(size_t(binding.type));
            if(!texture.image || !texture.view)throw std::runtime_error(std::string("missing NRD input/output: ")+nrd::GetResourceTypeString(binding.type));
            images.emplace_back(vk::Sampler{},texture.view,vk::ImageLayout::eGeneral);
            writes.emplace_back(set,desc.resourcesBaseRegisterIndex+j+(storage?offset.storageTextureAndBufferOffset:offset.textureOffset),0,1,
               storage?vk::DescriptorType::eStorageImage:vk::DescriptorType::eSampledImage,&images.back());
         }
      }
      // Covers both RAW and WAR dependencies, including prior submissions on
      // the same queue. Pool images remain in GENERAL throughout all passes.
      const vk::MemoryBarrier dependency(vk::AccessFlagBits::eMemoryWrite|vk::AccessFlagBits::eMemoryRead,vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eShaderWrite);
      command.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,vk::PipelineStageFlagBits::eComputeShader,{},dependency,{},{});
      m->device.updateDescriptorSets(writes,{});
      if(source.hasConstantData && !dispatch.constantBufferDataMatchesPreviousDispatch) {
         if(!dispatch.constantBufferData || !dispatch.constantBufferDataSize || dispatch.constantBufferDataSize%4 || dispatch.constantBufferDataSize>desc.constantBufferMaxDataSize)
            throw std::runtime_error("invalid NRD constants");
         vk::BufferMemoryBarrier barrier(vk::AccessFlagBits::eUniformRead,vk::AccessFlagBits::eTransferWrite,VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,*m->constants,0,VK_WHOLE_SIZE);
         command.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eTransfer,{}, {},barrier,{});
         command.updateBuffer(*m->constants,0,dispatch.constantBufferDataSize,dispatch.constantBufferData);
         barrier.srcAccessMask=vk::AccessFlagBits::eTransferWrite;barrier.dstAccessMask=vk::AccessFlagBits::eUniformRead;
         command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eComputeShader,{}, {},barrier,{});
      }
      std::array<vk::DescriptorSet,2> sets;sets[desc.constantBufferAndSamplersSpaceIndex]=commonSet;sets[desc.resourcesSpaceIndex]=set;
      command.bindPipeline(vk::PipelineBindPoint::eCompute,*pipeline.pipeline);
      command.bindDescriptorSets(vk::PipelineBindPoint::eCompute,*pipeline.layout,0,sets,{});
      command.dispatch(dispatch.gridWidth,dispatch.gridHeight,1);
   }
   const vk::MemoryBarrier finished(vk::AccessFlagBits::eShaderWrite,vk::AccessFlagBits::eShaderRead);
   command.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eFragmentShader,{},finished,{},{});
   m->pending=true;return true;
 }catch(const std::exception &failure){error=failure.what();m->reset=true;m->pending=false;return false;}
}
}
