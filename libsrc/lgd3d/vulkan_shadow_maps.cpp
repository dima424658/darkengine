#include "vulkan_shadow_maps.hpp"
#include <array>
#include <fstream>
#include <cstring>
#include <tracy/Tracy.hpp>
namespace dark {
namespace {
struct Culling {
   float projection[16]{};
   std::vector<uint64_t> boundsRevisions;
   std::vector<unsigned char> visible;
   uint64_t staticCohort=0,staticHeader=0,staticSignature=0,previousHeader=0;
   std::vector<uint32_t> staticIndices;
};
struct HostBuffer {
   vk::Device device;vk::UniqueDeviceMemory memory;vk::UniqueBuffer buffer;void *mapped=nullptr;vk::DeviceSize capacity=0;
   ~HostBuffer(){if(mapped)device.unmapMemory(*memory);}
};
struct InstanceData {float model[16];uint32_t material[4];};
static_assert(sizeof(InstanceData)==80,"shadow instance SSBO ABI");
struct Array {
   vk::UniqueDeviceMemory memory;
   vk::UniqueImage image;
   vk::UniqueImageView view;
   std::vector<vk::UniqueImageView> layers;
   std::vector<vk::UniqueFramebuffer> targets;
   uint32_t size=0,count=0;
   vk::DeviceSize memoryBytes=0;
   bool copyDestination=false;
   std::vector<uint64_t> signatures;
   std::vector<Culling> culling;
   void swap(Array &other) {
      using std::swap;
      swap(memory,other.memory);swap(image,other.image);swap(view,other.view);
      swap(layers,other.layers);swap(targets,other.targets);swap(size,other.size);swap(count,other.count);
      swap(signatures,other.signatures);
      swap(culling,other.culling);
      swap(copyDestination,other.copyDestination);
      swap(memoryBytes,other.memoryBytes);
   }
};
struct Frame {
   const WorldScene *scene=nullptr;
   Array local,sun,cached,staticLocal,staticSun,staticCached;VulkanShadowMaps::Stats stats;vk::UniqueQueryPool timing;bool timingSubmitted=false;
   std::vector<RenderSceneHandle> casterIds;
   std::vector<SceneOrientedBounds> casterBounds;
   std::vector<uint64_t> boundsRevisions;
   uint64_t boundsSerial=0;
   std::unique_ptr<HostBuffer> instances,commands;
   bool indirect=false;
   std::vector<VkDrawIndexedIndirectCommand> packets;
   std::vector<uint64_t> previousKeys;
   std::vector<unsigned char> keyAges;
   bool staticDepthCache=false;
};
struct Constants {float model[16];uint32_t material,view;};
static_assert(sizeof(Constants)==72,"shadow push constant layout");
std::vector<uint32_t> shader(const std::string &path) {
   std::ifstream file(path,std::ios::binary|std::ios::ate);
   if(!file)throw std::runtime_error("missing shader "+path);
   const auto size=file.tellg();if(size<=0 || size_t(size)%4)throw std::runtime_error("invalid shader "+path);
   std::vector<uint32_t> bytes(size_t(size)/4);file.seekg(0);file.read(reinterpret_cast<char *>(bytes.data()),size);
   if(!file)throw std::runtime_error("read shader "+path);return bytes;
}
}
struct VulkanShadowMaps::Impl {
   vk::Device device;
   vk::PhysicalDeviceProperties properties;
   vk::PhysicalDeviceMemoryProperties memory;
   vk::UniqueRenderPass pass;
   vk::UniqueRenderPass loadPass;
   vk::UniquePipelineLayout layout;
   vk::UniquePipeline pipeline;
   std::array<Frame,2> frames;
   uint32_t timestampBits=0;
   bool indirectEnabled=false;
   void buffer(std::unique_ptr<HostBuffer> &target,vk::DeviceSize bytes,vk::BufferUsageFlags usage) {
      bytes=std::max<vk::DeviceSize>(bytes,80);
      if(target && target->capacity>=bytes)return;
      auto replacement=std::make_unique<HostBuffer>();replacement->device=device;replacement->capacity=bytes;
      replacement->buffer=device.createBufferUnique(vk::BufferCreateInfo({},bytes,usage));
      const auto requirements=device.getBufferMemoryRequirements(*replacement->buffer);
      const auto wanted=vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent;uint32_t type=0;
      while(type<memory.memoryTypeCount && (!(requirements.memoryTypeBits&(1u<<type)) || (memory.memoryTypes[type].propertyFlags&wanted)!=wanted))++type;
      if(type==memory.memoryTypeCount)throw std::runtime_error("shadow command host memory unavailable");
      replacement->memory=device.allocateMemoryUnique(vk::MemoryAllocateInfo(requirements.size,type));
      device.bindBufferMemory(*replacement->buffer,*replacement->memory,0);
      replacement->mapped=device.mapMemory(*replacement->memory,0,bytes);target=std::move(replacement);
   }
   Array array(uint32_t size,uint32_t count,bool copyDestination) {
      if(!size || size>properties.limits.maxImageDimension2D || !count || count>properties.limits.maxImageArrayLayers)throw std::runtime_error("shadow array exceeds device limits");
      Array result;result.size=size;result.count=count;result.copyDestination=copyDestination;result.signatures.resize(count);result.culling.resize(count);
      auto usage=vk::ImageUsageFlagBits::eDepthStencilAttachment|vk::ImageUsageFlagBits::eSampled|vk::ImageUsageFlagBits::eTransferSrc;
      if(copyDestination)usage|=vk::ImageUsageFlagBits::eTransferDst;
      result.image=device.createImageUnique(vk::ImageCreateInfo({},vk::ImageType::e2D,vk::Format::eD32Sfloat,{size,size,1},1,count,
         vk::SampleCountFlagBits::e1,vk::ImageTiling::eOptimal,usage,vk::SharingMode::eExclusive));
      const auto requirements=device.getImageMemoryRequirements(*result.image);uint32_t type=0;
      while(type<memory.memoryTypeCount && (!(requirements.memoryTypeBits&(1u<<type)) || !(memory.memoryTypes[type].propertyFlags&vk::MemoryPropertyFlagBits::eDeviceLocal)))++type;
      if(type==memory.memoryTypeCount)throw std::runtime_error("shadow image memory unavailable");
      result.memory=device.allocateMemoryUnique(vk::MemoryAllocateInfo(requirements.size,type));device.bindImageMemory(*result.image,*result.memory,0);
      result.memoryBytes=requirements.size;
      result.view=device.createImageViewUnique(vk::ImageViewCreateInfo({},*result.image,vk::ImageViewType::e2DArray,vk::Format::eD32Sfloat,{},vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth,0,1,0,count)));
      for(uint32_t layer=0;layer<count;++layer) {
         result.layers.push_back(device.createImageViewUnique(vk::ImageViewCreateInfo({},*result.image,vk::ImageViewType::e2D,vk::Format::eD32Sfloat,{},vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth,0,1,layer,1))));
         const vk::ImageView view=*result.layers.back();result.targets.push_back(device.createFramebufferUnique(vk::FramebufferCreateInfo({},*pass,1,&view,size,size,1)));
      }return result;
   }
   void prepare(vk::CommandBuffer command,Array &target,uint32_t size,uint32_t count,bool copyDestination=false) {
      // Even an empty budget needs a valid, unsampled descriptor backing image.
      if(!count)size=1;
      count=std::max(1u,count);
      if(target.size==size && target.count==count && target.copyDestination==copyDestination)return;
      auto replacement=array(size,count,copyDestination);target.swap(replacement);
      vk::ImageMemoryBarrier barrier({},vk::AccessFlagBits::eShaderRead,vk::ImageLayout::eUndefined,vk::ImageLayout::eDepthStencilReadOnlyOptimal,
         VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,*target.image,{vk::ImageAspectFlagBits::eDepth,0,1,0,count});
      command.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,vk::PipelineStageFlagBits::eFragmentShader,{}, {}, {},barrier);
   }
};
VulkanShadowMaps::VulkanShadowMaps():m(std::make_unique<Impl>()){}
VulkanShadowMaps::~VulkanShadowMaps()=default;
void VulkanShadowMaps::Shutdown(){m=std::make_unique<Impl>();}
bool VulkanShadowMaps::Initialize(vk::PhysicalDevice physical,vk::Device device,vk::DescriptorSetLayout materials,const std::string &directory,std::string &error,uint32_t graphicsFamily,bool indirectEnabled) {
 try {
   m->device=device;m->memory=physical.getMemoryProperties();m->properties=physical.getProperties();
   m->indirectEnabled=indirectEnabled;
   const auto queues=physical.getQueueFamilyProperties();
   if(graphicsFamily==UINT32_MAX) {
      for(uint32_t i=0;i<queues.size();++i)if(queues[i].queueFlags&vk::QueueFlagBits::eGraphics){graphicsFamily=i;break;}
   }
   if(graphicsFamily>=queues.size() || !(queues[graphicsFamily].queueFlags&vk::QueueFlagBits::eGraphics))
      throw std::runtime_error("invalid shadow graphics queue family");
   m->timestampBits=queues[graphicsFamily].timestampValidBits;
   if(m->timestampBits)for(auto &frame:m->frames)
      frame.timing=device.createQueryPoolUnique(vk::QueryPoolCreateInfo({},vk::QueryType::eTimestamp,2));
   const auto flags=physical.getFormatProperties(vk::Format::eD32Sfloat).optimalTilingFeatures;
   const auto needed=vk::FormatFeatureFlagBits::eDepthStencilAttachment|vk::FormatFeatureFlagBits::eSampledImage;
   if((flags&needed)!=needed)throw std::runtime_error("sampled D32 shadow targets unavailable");
   vk::AttachmentDescription attachment({},vk::Format::eD32Sfloat,vk::SampleCountFlagBits::e1,vk::AttachmentLoadOp::eClear,vk::AttachmentStoreOp::eStore,
      vk::AttachmentLoadOp::eDontCare,vk::AttachmentStoreOp::eDontCare,vk::ImageLayout::eDepthStencilReadOnlyOptimal,vk::ImageLayout::eDepthStencilReadOnlyOptimal);
   vk::AttachmentReference depth(0,vk::ImageLayout::eDepthStencilAttachmentOptimal);
   vk::SubpassDescription subpass({},vk::PipelineBindPoint::eGraphics,0,nullptr,0,nullptr,nullptr,&depth);
   const auto tests=vk::PipelineStageFlagBits::eEarlyFragmentTests|vk::PipelineStageFlagBits::eLateFragmentTests;
   std::array<vk::SubpassDependency,2> dependencies{{
      {VK_SUBPASS_EXTERNAL,0,vk::PipelineStageFlagBits::eFragmentShader,tests,vk::AccessFlagBits::eShaderRead,vk::AccessFlagBits::eDepthStencilAttachmentWrite},
      {0,VK_SUBPASS_EXTERNAL,tests,vk::PipelineStageFlagBits::eFragmentShader,vk::AccessFlagBits::eDepthStencilAttachmentWrite,vk::AccessFlagBits::eShaderRead}}};
   m->pass=device.createRenderPassUnique(vk::RenderPassCreateInfo({},1,&attachment,1,&subpass,uint32_t(dependencies.size()),dependencies.data()));
   attachment.loadOp=vk::AttachmentLoadOp::eLoad;attachment.initialLayout=vk::ImageLayout::eDepthStencilAttachmentOptimal;
   m->loadPass=device.createRenderPassUnique(vk::RenderPassCreateInfo({},1,&attachment,1,&subpass,uint32_t(dependencies.size()),dependencies.data()));
   vk::PushConstantRange push(vk::ShaderStageFlagBits::eVertex|vk::ShaderStageFlagBits::eFragment,0,sizeof(Constants));
   m->layout=device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({},1,&materials,1,&push));
   const auto vertex=shader(directory+"/shadow_map.vert.spv"),fragment=shader(directory+"/shadow_map.frag.spv");
   auto vs=device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({},vertex.size()*4,vertex.data()));
   auto fs=device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({},fragment.size()*4,fragment.data()));
   std::array<vk::PipelineShaderStageCreateInfo,2> stages{{{{},vk::ShaderStageFlagBits::eVertex,*vs,"main"},{{},vk::ShaderStageFlagBits::eFragment,*fs,"main"}}};
   vk::VertexInputBindingDescription binding(0,sizeof(RenderSceneVertex),vk::VertexInputRate::eVertex);
   std::array<vk::VertexInputAttributeDescription,2> attributes{{{0,0,vk::Format::eR32G32B32Sfloat,0},{2,0,vk::Format::eR32G32Sfloat,24}}};
   vk::PipelineVertexInputStateCreateInfo inputs({},binding,attributes);
   vk::PipelineInputAssemblyStateCreateInfo assembly({},vk::PrimitiveTopology::eTriangleList);
   vk::PipelineViewportStateCreateInfo viewport({},1,nullptr,1,nullptr);
   vk::PipelineRasterizationStateCreateInfo raster({},VK_FALSE,VK_FALSE,vk::PolygonMode::eFill,vk::CullModeFlagBits::eNone,vk::FrontFace::eCounterClockwise,VK_FALSE,0,0,0,1);
   vk::PipelineMultisampleStateCreateInfo samples({},vk::SampleCountFlagBits::e1);
   vk::PipelineDepthStencilStateCreateInfo depthState({},VK_TRUE,VK_TRUE,vk::CompareOp::eLessOrEqual);
   vk::PipelineColorBlendStateCreateInfo blend;
   const std::array<vk::DynamicState,2> dynamic{{vk::DynamicState::eViewport,vk::DynamicState::eScissor}};
   vk::PipelineDynamicStateCreateInfo dynamicState({},dynamic);
   m->pipeline=device.createGraphicsPipelineUnique({},vk::GraphicsPipelineCreateInfo({},stages,&inputs,&assembly,nullptr,&viewport,&raster,&samples,&depthState,&blend,&dynamicState,*m->layout,*m->pass)).value;
   return true;
 }catch(const std::exception &failure){error=failure.what();Shutdown();return false;}
}
bool VulkanShadowMaps::Prepare(vk::CommandBuffer command,uint32_t slot,const ShadowSettings &settings,std::string &error) {
 try {
   if(!m->device || slot>=m->frames.size())throw std::runtime_error("invalid shadow frame slot");
   auto &frame=m->frames[slot];m->prepare(command,frame.local,settings.localResolution,settings.localFaces,settings.staticDepthCache);
   frame.indirect=m->indirectEnabled && settings.indirect;
   m->prepare(command,frame.sun,settings.sunResolution,settings.sunCascades,settings.staticDepthCache);
   m->prepare(command,frame.cached,settings.cachedResolution,settings.cachedFaces,settings.staticDepthCache);
   frame.staticDepthCache=settings.staticDepthCache;
   if(frame.staticDepthCache) {
      m->prepare(command,frame.staticLocal,settings.localResolution,settings.localFaces);
      m->prepare(command,frame.staticSun,settings.sunResolution,settings.sunCascades);
      m->prepare(command,frame.staticCached,settings.cachedResolution,settings.cachedFaces);
   }else {
      frame.previousKeys.clear();
      frame.keyAges.clear();
      for(auto *target:{&frame.staticLocal,&frame.staticSun,&frame.staticCached}) {
         Array empty;target->swap(empty);
      }
   }
   return true;
 }catch(const std::exception &failure){error=failure.what();return false;}
}
bool VulkanShadowMaps::Record(vk::CommandBuffer command,uint32_t slot,const ShadowPlan &plan,const WorldScene &scene,
   const VulkanWorldGeometry &geometry,vk::DescriptorSet materials,const std::unordered_map<RenderSceneHandle,uint32_t> &indices,std::string &error) {
 ZoneScopedN("Render shadow depth maps");
 try {
   if(!m->device || slot>=m->frames.size())throw std::runtime_error("invalid shadow frame slot");
   auto &frame=m->frames[slot];
   if(frame.scene!=&scene) {
      // A different scene may reuse handles and revision values. Retain the
      // retired slot's images, but never its depth/membership history.
      for(auto *array:{&frame.local,&frame.sun,&frame.cached,&frame.staticLocal,&frame.staticSun,&frame.staticCached}) {
         std::fill(array->signatures.begin(),array->signatures.end(),0);
         for(auto &culling:array->culling)culling=Culling{};
      }
      frame.casterIds.clear();frame.casterBounds.clear();frame.boundsRevisions.clear();
      frame.previousKeys.clear();frame.keyAges.clear();
   }
   auto hashBytes=[](uint64_t &hash,const void *pointer,size_t size) {
      const auto *bytes=static_cast<const unsigned char *>(pointer);
      for(size_t i=0;i<size;++i){hash^=bytes[i];hash*=1099511628211ull;}
   };
   double previousTime=-1;
   if(frame.timing && frame.timingSubmitted) {
      uint64_t ticks[2]{};
      const auto result=m->device.getQueryPoolResults(*frame.timing,0,2,sizeof(ticks),ticks,sizeof(uint64_t),vk::QueryResultFlagBits::e64);
      if(result==vk::Result::eSuccess) {
         const uint64_t mask=m->timestampBits==64?UINT64_MAX:((uint64_t(1)<<m->timestampBits)-1);
         previousTime=double((ticks[1]-ticks[0])&mask)*m->properties.limits.timestampPeriod/1000000;
      }
   }
   frame.stats={};frame.stats.gpuMilliseconds=previousTime;
   for(const auto *array:{&frame.local,&frame.sun,&frame.cached,&frame.staticLocal,&frame.staticSun,&frame.staticCached})
      frame.stats.depthMemoryBytes+=array->memoryBytes;
   if(frame.timing) {
      command.resetQueryPool(*frame.timing,0,2);
      command.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe,*frame.timing,0);
   }
   // Stable order is necessary because the scene containers are unordered.
   std::vector<RenderSceneHandle> casters;
   for(const auto &entry:scene.instances)if(entry.second.castsShadow)casters.push_back(entry.first);
   std::sort(casters.begin(),casters.end());
   // Pose/material revisions are view-independent. Hash them once per frame,
   // rather than once for every shadow face that intersects the caster.
   struct Caster {
      const RenderSceneInstance *instance;uint64_t signature;VkBuffer vertices,elements;
      SceneOrientedBounds bounds;RenderSceneHandle id;
      VkDrawIndexedIndirectCommand packet;
      uint32_t material;int32_t objectId;bool emitterSurface;
   };
   std::vector<Caster> prepared;prepared.reserve(casters.size());
   for(const auto id:casters) {
      const auto &instance=scene.instances.at(id);const auto &mesh=scene.meshes.at(instance.mesh);
      const auto &material=scene.materials.at(mesh.material);if(material.opacity==RenderSceneBlended)continue;
      uint64_t signature=1469598103934665603ull;
      hashBytes(signature,&id,sizeof(id));hashBytes(signature,&instance.mesh,sizeof(instance.mesh));
      hashBytes(signature,&instance.objectId,sizeof(instance.objectId));
      hashBytes(signature,instance.transform,sizeof(instance.transform));hashBytes(signature,&mesh.revision,sizeof(mesh.revision));
      // Only depth-affecting state belongs in this cache. Opaque color,
      // emission and BRDF changes must not redraw an identical shadow map.
      hashBytes(signature,&material.opacity,sizeof(material.opacity));
      hashBytes(signature,&material.doubleSided,sizeof(material.doubleSided));
      hashBytes(signature,&material.emitterSurface,sizeof(material.emitterSurface));
      if(material.opacity==RenderSceneMasked) {
         hashBytes(signature,&material.alphaCutoff,sizeof(material.alphaCutoff));
         hashBytes(signature,&material.baseColor[3],sizeof(material.baseColor[3]));
         hashBytes(signature,&material.baseColorTexture,sizeof(material.baseColorTexture));
         if(material.baseColorTexture) {
            const auto revision=scene.textures.at(material.baseColorTexture).alphaRevision;
            hashBytes(signature,&revision,sizeof(revision));
         }
      }
      VkBuffer vertices{},elements{};VkDeviceSize vertexOffset{},indexOffset{};
      if(!geometry.MeshBuffers(slot,instance.mesh,vertices,elements,&vertexOffset,&indexOffset))throw std::runtime_error("missing shadow caster geometry");
      if(vertexOffset/sizeof(RenderSceneVertex)>INT32_MAX || indexOffset/sizeof(uint32_t)>UINT32_MAX || prepared.size()>UINT32_MAX)
         throw std::runtime_error("shadow geometry draw offset exceeds Vulkan limits");
      const VkDrawIndexedIndirectCommand packet{uint32_t(mesh.indices.size()),1,uint32_t(indexOffset/sizeof(uint32_t)),int32_t(vertexOffset/sizeof(RenderSceneVertex)),uint32_t(prepared.size())};
      prepared.push_back({&instance,signature,vertices,elements,SceneTransformBounds(mesh.bounds,instance.transform),id,packet,indices.at(mesh.material),instance.objectId,material.emitterSurface!=0});
   }
   // Geometric membership is independent of pose/material depth revisions.
   // Compare bounds once per caster, then reuse exact per-layer frustum tests.
   bool topologyChanged=frame.casterIds.size()!=prepared.size();
   if(!topologyChanged)for(size_t i=0;i<prepared.size();++i)
      if(frame.casterIds[i]!=prepared[i].id){topologyChanged=true;break;}
   // A new/removed caster invalidates index-based memberships, not the
   // stability of unrelated handles. Merge the sorted identities before
   // replacing the previous index table.
   std::vector<unsigned char> topologyAges;
   if(frame.staticDepthCache && topologyChanged) {
      topologyAges.resize(prepared.size());size_t previous=0;
      for(size_t i=0;i<prepared.size();++i) {
         while(previous<frame.casterIds.size() && frame.casterIds[previous]<prepared[i].id)++previous;
         if(previous<frame.casterIds.size() && frame.casterIds[previous]==prepared[i].id &&
            previous<frame.previousKeys.size() && previous<frame.keyAges.size() &&
            frame.previousKeys[previous]==prepared[i].signature)
            topologyAges[i]=std::min(255,int(frame.keyAges[previous])+1);
      }
   }
   if(topologyChanged) {
      frame.casterIds.resize(prepared.size());frame.casterBounds.resize(prepared.size());frame.boundsRevisions.resize(prepared.size());
      for(auto *array:{&frame.local,&frame.sun,&frame.cached})for(auto &cache:array->culling) {
         cache.boundsRevisions.clear();cache.staticCohort=0;cache.staticIndices.clear();
      }
   }
   for(size_t i=0;i<prepared.size();++i) {
      if(topologyChanged || std::memcmp(&frame.casterBounds[i],&prepared[i].bounds,sizeof(SceneOrientedBounds))) {
         frame.casterIds[i]=prepared[i].id;frame.casterBounds[i]=prepared[i].bounds;frame.boundsRevisions[i]=++frame.boundsSerial;
      }
   }
   std::vector<unsigned char> stable;
   std::vector<uint32_t> stableIndices,movingIndices;
   uint64_t staticCohort=1469598103934665603ull;
   std::vector<const Caster *> staticCasters,dynamicCasters;
   if(frame.staticDepthCache) {
      stable.resize(prepared.size());
      const bool history=!topologyChanged && frame.previousKeys.size()==prepared.size();
      if(topologyChanged)frame.keyAges=std::move(topologyAges);else frame.keyAges.resize(prepared.size());
      for(size_t i=0;i<prepared.size();++i) {
         auto &age=frame.keyAges[i];
         if(!topologyChanged)age=history && frame.previousKeys[i]==prepared[i].signature?std::min(255,int(age)+1):0;
         stable[i]=age>=4;
         if(stable[i]){stableIndices.push_back(uint32_t(i));staticCohort=(staticCohort^prepared[i].signature)*1099511628211ull;}
         else movingIndices.push_back(uint32_t(i));
      }
      frame.previousKeys.resize(prepared.size());
      for(size_t i=0;i<prepared.size();++i)frame.previousKeys[i]=prepared[i].signature;
      staticCasters.reserve(prepared.size());dynamicCasters.reserve(prepared.size());
   }
   std::vector<const Caster *> visibleCasters;visibleCasters.reserve(prepared.size());
   const auto instanceBytes=std::max<vk::DeviceSize>(80,vk::DeviceSize(prepared.size())*sizeof(InstanceData));
   if(instanceBytes>m->properties.limits.maxStorageBufferRange)throw std::runtime_error("shadow instance storage exceeds device limit");
   m->buffer(frame.instances,instanceBytes,vk::BufferUsageFlagBits::eStorageBuffer);
   auto *instanceData=static_cast<InstanceData *>(frame.instances->mapped);
   for(size_t i=0;i<prepared.size();++i) {
      std::copy(prepared[i].instance->transform,prepared[i].instance->transform+16,instanceData[i].model);
      instanceData[i].material[0]=prepared[i].material;
      instanceData[i].material[1]=instanceData[i].material[2]=instanceData[i].material[3]=0;
   }
   const vk::DescriptorBufferInfo instanceInfo(*frame.instances->buffer,0,instanceBytes);
   m->device.updateDescriptorSets(vk::WriteDescriptorSet(materials,10,0,1,vk::DescriptorType::eStorageBuffer,nullptr,&instanceInfo),{});
   if(frame.indirect) {
      const uint64_t commands=uint64_t(prepared.size())*plan.views.size();
      if(commands>SIZE_MAX/sizeof(VkDrawIndexedIndirectCommand))throw std::runtime_error("shadow command count overflow");
      m->buffer(frame.commands,commands*sizeof(VkDrawIndexedIndirectCommand),vk::BufferUsageFlagBits::eIndirectBuffer);
   }
   size_t commandCursor=0;
   frame.packets.clear();
   for(uint32_t viewIndex=0;viewIndex<plan.views.size();++viewIndex) {
      const auto &view=plan.views[viewIndex];auto &target=view.metadata[0]==2?frame.sun:
         ((view.metadata[1]&ShadowCachedLayer)?frame.cached:frame.local);
      const uint32_t layer=view.metadata[1]&~ShadowCachedLayer;if(layer>=target.count)throw std::runtime_error("shadow layer out of bounds");
      uint64_t signature=1469598103934665603ull;
      // Sampling metadata/view-buffer order does not alter stored depth. Keep
      // retained layers reusable when another light enters/leaves the plan.
      hashBytes(signature,view.viewProjection,sizeof(view.viewProjection));
      hashBytes(signature,view.originNear,sizeof(view.originNear));
      hashBytes(signature,view.depthPlane,sizeof(view.depthPlane));
      hashBytes(signature,&view.parameters[0],sizeof(float));
      hashBytes(signature,&view.metadata[0],sizeof(uint32_t));
      const int32_t emitter=viewIndex<plan.emitterObjects.size()?plan.emitterObjects[viewIndex]:0;
      hashBytes(signature,&emitter,sizeof(emitter));
      uint64_t staticSignature=signature;
      visibleCasters.clear();staticCasters.clear();dynamicCasters.clear();
      const auto frustum=SceneExtractFrustum(view.viewProjection);
      auto &culling=target.culling[layer];
      // A changed sun projection cannot reuse its static base. Render it once
      // directly; retain composition for stationary/stabilized cascades.
      const bool useStaticDepth=frame.staticDepthCache &&
         (view.metadata[0]!=2 || (signature && culling.previousHeader==signature));
      culling.previousHeader=signature;
      if(culling.boundsRevisions.size()!=prepared.size() || std::memcmp(culling.projection,view.viewProjection,sizeof(culling.projection))) {
         std::copy(view.viewProjection,view.viewProjection+16,culling.projection);
         culling.boundsRevisions.assign(prepared.size(),0);culling.visible.resize(prepared.size());
         culling.staticCohort=0;culling.staticIndices.clear();
      }
      auto intersects=[&](size_t i) {
         const auto &caster=prepared[i];
         if(emitter && caster.objectId==emitter && caster.emitterSurface)return false;
         if(culling.boundsRevisions[i]!=frame.boundsRevisions[i]) {
            culling.visible[i]=SceneBoundsVisible(caster.bounds,frustum);culling.boundsRevisions[i]=frame.boundsRevisions[i];
            ++frame.stats.boundsTests;
         }
         return culling.visible[i]!=0;
      };
      if(useStaticDepth) {
         if(staticCohort && culling.staticCohort==staticCohort && culling.staticHeader==staticSignature) {
            staticSignature=culling.staticSignature;
         }else {
            culling.staticIndices.clear();
            for(const auto i:stableIndices)if(intersects(i)) {
               culling.staticIndices.push_back(i);staticSignature=(staticSignature^prepared[i].signature)*1099511628211ull;
            }
            culling.staticCohort=staticCohort;culling.staticHeader=signature;culling.staticSignature=staticSignature;
         }
         signature=staticSignature;
         for(const auto i:movingIndices)if(intersects(i)) {
            dynamicCasters.push_back(&prepared[i]);signature=(signature^prepared[i].signature)*1099511628211ull;
         }
      }else for(size_t i=0;i<prepared.size();++i)if(intersects(i)) {
         visibleCasters.push_back(&prepared[i]);signature=(signature^prepared[i].signature)*1099511628211ull;
      }
      if(signature && target.signatures[layer]==signature){++frame.stats.reusedViews;continue;}
      auto render=[&](Array &output,vk::RenderPass usePass,const std::vector<const Caster *> &drawCasters) {
      vk::ClearValue clear;clear.depthStencil=vk::ClearDepthStencilValue(1,0);
      command.beginRenderPass(vk::RenderPassBeginInfo(usePass,*output.targets[layer],{{0,0},{output.size,output.size}},1,&clear),vk::SubpassContents::eInline);
      command.bindPipeline(vk::PipelineBindPoint::eGraphics,*m->pipeline);
      command.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,*m->layout,0,materials,{});
      command.setViewport(0,vk::Viewport(0,0,float(output.size),float(output.size),0,1));command.setScissor(0,vk::Rect2D({0,0},{output.size,output.size}));
      VkBuffer boundVertices{},boundIndices{};
      if(frame.indirect) {
         Constants constants{};constants.material=UINT32_MAX;constants.view=viewIndex;
         command.pushConstants(*m->layout,vk::ShaderStageFlagBits::eVertex|vk::ShaderStageFlagBits::eFragment,0,sizeof(constants),&constants);
         size_t cursor=0;
         while(cursor<drawCasters.size()) {
            const auto *first=drawCasters[cursor];const size_t start=commandCursor;
            const vk::Buffer vertices=first->vertices;const vk::DeviceSize offset=0;
            command.bindVertexBuffers(0,1,&vertices,&offset);command.bindIndexBuffer(first->elements,0,vk::IndexType::eUint32);
            do {
               const auto *caster=drawCasters[cursor];
               frame.packets.push_back(caster->packet);++commandCursor;
               ++cursor;++frame.stats.drawCalls;
            }while(cursor<drawCasters.size() && drawCasters[cursor]->vertices==first->vertices && drawCasters[cursor]->elements==first->elements && commandCursor-start<m->properties.limits.maxDrawIndirectCount);
            command.drawIndexedIndirect(*frame.commands->buffer,start*sizeof(VkDrawIndexedIndirectCommand),uint32_t(commandCursor-start),sizeof(VkDrawIndexedIndirectCommand));
            ++frame.stats.drawBatches;
         }
      }else
      // Cache validation already computed this exact view's caster set.
      for(const auto *caster:drawCasters) {
         Constants constants{};std::copy(caster->instance->transform,caster->instance->transform+16,constants.model);
         constants.material=caster->material;constants.view=viewIndex;
         command.pushConstants(*m->layout,vk::ShaderStageFlagBits::eVertex|vk::ShaderStageFlagBits::eFragment,0,sizeof(constants),&constants);
         if(boundVertices!=caster->vertices) {
            const vk::Buffer buffer=caster->vertices;const vk::DeviceSize offset=0;command.bindVertexBuffers(0,1,&buffer,&offset);boundVertices=caster->vertices;
         }
         if(boundIndices!=caster->elements){command.bindIndexBuffer(caster->elements,0,vk::IndexType::eUint32);boundIndices=caster->elements;}
         command.drawIndexed(caster->packet.indexCount,1,caster->packet.firstIndex,caster->packet.vertexOffset,0);
         ++frame.stats.drawCalls;
         ++frame.stats.drawBatches;
      }
      command.endRenderPass();
      };
      if(useStaticDepth) {
         auto &base=view.metadata[0]==2?frame.staticSun:
            ((view.metadata[1]&ShadowCachedLayer)?frame.staticCached:frame.staticLocal);
         if(!staticSignature || base.signatures[layer]!=staticSignature) {
            for(const auto i:culling.staticIndices)staticCasters.push_back(&prepared[i]);
            render(base,*m->pass,staticCasters);base.signatures[layer]=staticSignature;++frame.stats.staticRendered;
         }else ++frame.stats.staticReused;
         const vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eDepth,0,1,layer,1);
         const auto readsWrites=vk::AccessFlagBits::eMemoryRead|vk::AccessFlagBits::eMemoryWrite;
         std::array<vk::ImageMemoryBarrier,2> barriers{{
            {readsWrites,vk::AccessFlagBits::eTransferRead,vk::ImageLayout::eDepthStencilReadOnlyOptimal,vk::ImageLayout::eTransferSrcOptimal,VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,*base.image,range},
            {readsWrites,vk::AccessFlagBits::eTransferWrite,vk::ImageLayout::eDepthStencilReadOnlyOptimal,vk::ImageLayout::eTransferDstOptimal,VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,*target.image,range}}};
         command.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,vk::PipelineStageFlagBits::eTransfer,{}, {}, {},barriers);
         const vk::ImageSubresourceLayers subresource(vk::ImageAspectFlagBits::eDepth,0,layer,1);
         command.copyImage(*base.image,vk::ImageLayout::eTransferSrcOptimal,*target.image,vk::ImageLayout::eTransferDstOptimal,
            vk::ImageCopy(subresource,{0,0,0},subresource,{0,0,0},{target.size,target.size,1}));
         barriers[0].srcAccessMask=vk::AccessFlagBits::eTransferRead;barriers[0].dstAccessMask=vk::AccessFlagBits::eShaderRead;
         barriers[0].oldLayout=vk::ImageLayout::eTransferSrcOptimal;barriers[0].newLayout=vk::ImageLayout::eDepthStencilReadOnlyOptimal;
         barriers[1].srcAccessMask=vk::AccessFlagBits::eTransferWrite;
         barriers[1].dstAccessMask=vk::AccessFlagBits::eDepthStencilAttachmentRead|vk::AccessFlagBits::eDepthStencilAttachmentWrite;
         barriers[1].oldLayout=vk::ImageLayout::eTransferDstOptimal;barriers[1].newLayout=vk::ImageLayout::eDepthStencilAttachmentOptimal;
         command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eFragmentShader|vk::PipelineStageFlagBits::eEarlyFragmentTests|vk::PipelineStageFlagBits::eLateFragmentTests,{}, {}, {},barriers);
         ++frame.stats.depthCopies;render(target,*m->loadPass,dynamicCasters);
      }else render(target,*m->pass,visibleCasters);
      target.signatures[layer]=signature;
      ++frame.stats.renderedViews;
   }
   // Coherent host writes are complete before queue submission. Bulk-copy the
   // packed commands instead of many small stores into mapped device memory.
   if(frame.indirect && !frame.packets.empty())std::memcpy(frame.commands->mapped,frame.packets.data(),frame.packets.size()*sizeof(VkDrawIndexedIndirectCommand));
   if(frame.timing){command.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe,*frame.timing,1);frame.timingSubmitted=true;}
   frame.scene=&scene;
   return true;
 }catch(const std::exception &failure){error=failure.what();return false;}
}
vk::ImageView VulkanShadowMaps::LocalView(uint32_t slot) const{return *m->frames.at(slot).local.view;}
vk::ImageView VulkanShadowMaps::SunView(uint32_t slot) const{return *m->frames.at(slot).sun.view;}
vk::ImageView VulkanShadowMaps::CachedView(uint32_t slot) const{return *m->frames.at(slot).cached.view;}
VkImage VulkanShadowMaps::LocalImage(uint32_t slot) const{return *m->frames.at(slot).local.image;}
VkImage VulkanShadowMaps::SunImage(uint32_t slot) const{return *m->frames.at(slot).sun.image;}
VulkanShadowMaps::Stats VulkanShadowMaps::FrameStats(uint32_t slot) const{return m->frames.at(slot).stats;}
}
