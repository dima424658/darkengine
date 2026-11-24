#include "vulkan_nrd.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <cstdio>
#include <stdexcept>
#include <vector>
struct Image {vk::UniqueDeviceMemory memory;vk::UniqueImage image;vk::UniqueImageView view;};
int main(){try {
   vk::raii::Context context;vk::ApplicationInfo application("Dark NRD validation",1,"Dark",1,VK_API_VERSION_1_2);
   auto instance=context.createInstance(vk::InstanceCreateInfo({},&application));
   for(const auto physical:(*instance).enumeratePhysicalDevices()) {
      const auto queues=physical.getQueueFamilyProperties();uint32_t family=0;
      while(family<queues.size() && !(queues[family].queueFlags&vk::QueueFlagBits::eCompute))++family;
      if(family==queues.size())continue;
      const float priority=1;vk::DeviceQueueCreateInfo queueInfo({},family,1,&priority);
      auto features=physical.getFeatures();auto device=physical.createDeviceUnique(vk::DeviceCreateInfo({},1,&queueInfo,0,nullptr,0,nullptr,&features));
      const auto memory=physical.getMemoryProperties();
      auto memoryType=[&](uint32_t bits,vk::MemoryPropertyFlags wanted){
         for(uint32_t i=0;i<memory.memoryTypeCount;++i)if((bits&(1u<<i)) && (memory.memoryTypes[i].propertyFlags&wanted)==wanted)return i;
         throw std::runtime_error("test memory type unavailable");
      };
      dark::VulkanNrd denoiser;std::string error;
      if(!denoiser.Initialize(physical,*device,64,64,error))throw std::runtime_error(error);
      dark::VulkanNrd::Resources resources{};std::vector<Image> images;
      auto pool=device->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,family));
      const auto command=device->allocateCommandBuffers(vk::CommandBufferAllocateInfo(*pool,vk::CommandBufferLevel::ePrimary,1))[0];
      const auto queue=device->getQueue(family,0);
      auto submit=[&]{auto fence=device->createFenceUnique({});queue.submit(vk::SubmitInfo({}, {},command),*fence);(void)device->waitForFences(*fence,VK_TRUE,UINT64_MAX);};
      command.begin(vk::CommandBufferBeginInfo{});
      auto make=[&](nrd::ResourceType type,vk::Format format,std::array<float,4> clear){
         Image image;image.image=device->createImageUnique(vk::ImageCreateInfo({},vk::ImageType::e2D,format,{64,64,1},1,1,vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,vk::ImageUsageFlagBits::eSampled|vk::ImageUsageFlagBits::eStorage|vk::ImageUsageFlagBits::eTransferDst|vk::ImageUsageFlagBits::eTransferSrc));
         auto requirements=device->getImageMemoryRequirements(*image.image);
         image.memory=device->allocateMemoryUnique(vk::MemoryAllocateInfo(requirements.size,memoryType(requirements.memoryTypeBits,vk::MemoryPropertyFlagBits::eDeviceLocal)));
         device->bindImageMemory(*image.image,*image.memory,0);
         const vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor,0,1,0,1);
         image.view=device->createImageViewUnique(vk::ImageViewCreateInfo({},*image.image,vk::ImageViewType::e2D,format,{},range));
         vk::ImageMemoryBarrier barrier({},vk::AccessFlagBits::eTransferWrite,vk::ImageLayout::eUndefined,vk::ImageLayout::eGeneral,VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,*image.image,range);
         command.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,vk::PipelineStageFlagBits::eTransfer,{}, {},{},barrier);
         command.clearColorImage(*image.image,vk::ImageLayout::eGeneral,vk::ClearColorValue(clear),range);
         resources[size_t(type)]={*image.image,*image.view};images.push_back(std::move(image));
      };
      make(nrd::ResourceType::IN_MV,vk::Format::eR16G16B16A16Sfloat,{0,0,0,0});
      make(nrd::ResourceType::IN_NORMAL_ROUGHNESS,vk::Format::eA2B10G10R10UnormPack32,{.5f,.5f,.75f,0});
      make(nrd::ResourceType::IN_VIEWZ,vk::Format::eR32Sfloat,{1,0,0,0});
      for(const auto type:{nrd::ResourceType::IN_DIFF_RADIANCE_HITDIST,nrd::ResourceType::IN_SPEC_RADIANCE_HITDIST,
         nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST,nrd::ResourceType::OUT_SPEC_RADIANCE_HITDIST})make(type,vk::Format::eR16G16B16A16Sfloat,{0,0,0,0});
      command.end();submit();
      for(unsigned frame=0;frame<6;++frame) {
         command.reset();command.begin(vk::CommandBufferBeginInfo{});nrd::CommonSettings common;
         for(unsigned i=0;i<4;++i)common.viewToClipMatrix[i*5]=common.viewToClipMatrixPrev[i*5]=common.worldToViewMatrix[i*5]=common.worldToViewMatrixPrev[i*5]=1;
         common.resourceSize[0]=common.resourceSize[1]=common.resourceSizePrev[0]=common.resourceSizePrev[1]=64;
         common.rectSize[0]=common.rectSize[1]=common.rectSizePrev[0]=common.rectSizePrev[1]=64;common.frameIndex=frame;
         if(frame==3)denoiser.ResetHistory();
         if(frame==0 || frame==4) {
            // Exercise a fully recorded command buffer rejected before submission,
            // including the first initialization of internal pool images.
            if(!denoiser.Record(command,frame%2,resources,common,error))throw std::runtime_error(error);
            command.end();denoiser.CompleteSubmission(false);command.reset();command.begin(vk::CommandBufferBeginInfo{});
         }
         if(!denoiser.Record(command,frame%2,resources,common,error))throw std::runtime_error(error);
         command.end();submit();denoiser.CompleteSubmission(true);
      }
      device->waitIdle();denoiser.Shutdown();std::puts("PASS native Vulkan NRD RELAX dispatches, frame slots and reset");return 0;
   }
   std::puts("SKIP no Vulkan compute device");return 77;
 }catch(const std::exception &failure){std::fprintf(stderr,"NRD validation: %s\n",failure.what());return 1;}}
