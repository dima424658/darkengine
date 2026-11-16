#include "../../libsrc/lgd3d/vulkan_shadow_maps.hpp"
#include "../../libsrc/lgd3d/vulkan_world_features.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
static void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
struct HostBuffer {vk::Device device;vk::UniqueDeviceMemory memory;vk::UniqueBuffer buffer;void *mapped{};~HostBuffer(){if(mapped)device.unmapMemory(*memory);}};
int main(int argc,char **argv) {
 try {
   check(argc==2,"pass generated shader directory");vk::raii::Context context;
   vk::ApplicationInfo app("Raster shadow-map regression",1,"Dark",1,VK_API_VERSION_1_1);
   auto instance=context.createInstance(vk::InstanceCreateInfo({},&app));
   for(const auto physical:(*instance).enumeratePhysicalDevices()) {
      dark::VulkanWorldFeatures features;if(!features.Probe(physical))continue;
      if(std::getenv("DARK_TEST_SCALAR_SHADOWS"))features.features.features.multiDrawIndirect=features.features.features.drawIndirectFirstInstance=VK_FALSE;
      const auto families=physical.getQueueFamilyProperties();uint32_t family=0;
      while(family<families.size() && !(families[family].queueFlags&vk::QueueFlagBits::eGraphics))++family;
      if(family==families.size())continue;
      const float priority=1;vk::DeviceQueueCreateInfo qi({},family,1,&priority);
      vk::DeviceCreateInfo di({},1,&qi,0,nullptr,uint32_t(features.extensions.size()),features.extensions.data());di.pNext=&features.features;
      auto device=physical.createDeviceUnique(di);const auto queue=device->getQueue(family,0);const auto memory=physical.getMemoryProperties();
      auto host=[&](size_t bytes,vk::BufferUsageFlags usage) {
         auto result=std::make_unique<HostBuffer>();result->device=*device;
         result->buffer=device->createBufferUnique(vk::BufferCreateInfo({},bytes,usage,vk::SharingMode::eExclusive));
         const auto req=device->getBufferMemoryRequirements(*result->buffer);uint32_t type=0;
         const auto wanted=vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent;
         while(type<memory.memoryTypeCount && (!(req.memoryTypeBits&(1u<<type)) || (memory.memoryTypes[type].propertyFlags&wanted)!=wanted))++type;
         check(type<memory.memoryTypeCount,"host memory");result->memory=device->allocateMemoryUnique(vk::MemoryAllocateInfo(req.size,type));
         device->bindBufferMemory(*result->buffer,*result->memory,0);result->mapped=device->mapMemory(*result->memory,0,bytes);return result;
      };
      std::array<vk::DescriptorSetLayoutBinding,4> bindings{{
         {1,vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eVertex|vk::ShaderStageFlagBits::eFragment},
         {2,vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eFragment},
         {7,vk::DescriptorType::eCombinedImageSampler,1,vk::ShaderStageFlagBits::eFragment},
         {10,vk::DescriptorType::eStorageBuffer,1,vk::ShaderStageFlagBits::eVertex}}};
      auto layout=device->createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo({},bindings));
      dark::VulkanShadowMaps shadows;std::string error;check(shadows.Initialize(physical,*device,*layout,argv[1],error,family,features.IndirectShadows()),error.c_str());
      dark::VulkanWorldGeometry geometry;check(geometry.Initialize(physical,*device),"geometry initialization");
      auto views=host(sizeof(dark::ShadowView)*10,vk::BufferUsageFlagBits::eStorageBuffer),material=host(64,vk::BufferUsageFlagBits::eStorageBuffer);
      auto readback=host(4,vk::BufferUsageFlagBits::eTransferDst),staging=host(4,vk::BufferUsageFlagBits::eTransferSrc);
      vk::UniqueDeviceMemory textureMemory;
      auto texture=device->createImageUnique(vk::ImageCreateInfo({},vk::ImageType::e2D,vk::Format::eR8G8B8A8Unorm,{1,1,1},1,1,vk::SampleCountFlagBits::e1,vk::ImageTiling::eOptimal,vk::ImageUsageFlagBits::eTransferDst|vk::ImageUsageFlagBits::eSampled,vk::SharingMode::eExclusive));
      const auto req=device->getImageMemoryRequirements(*texture);uint32_t type=0;
      while(type<memory.memoryTypeCount && (!(req.memoryTypeBits&(1u<<type)) || !(memory.memoryTypes[type].propertyFlags&vk::MemoryPropertyFlagBits::eDeviceLocal)))++type;
      check(type<memory.memoryTypeCount,"texture memory");textureMemory=device->allocateMemoryUnique(vk::MemoryAllocateInfo(req.size,type));device->bindImageMemory(*texture,*textureMemory,0);
      auto textureView=device->createImageViewUnique(vk::ImageViewCreateInfo({},*texture,vk::ImageViewType::e2D,vk::Format::eR8G8B8A8Unorm,{},vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor,0,1,0,1)));
      auto sampler=device->createSamplerUnique(vk::SamplerCreateInfo({},vk::Filter::eNearest,vk::Filter::eNearest,vk::SamplerMipmapMode::eNearest,
         vk::SamplerAddressMode::eRepeat,vk::SamplerAddressMode::eRepeat,vk::SamplerAddressMode::eRepeat));
      const std::array<vk::DescriptorPoolSize,2> poolSizes{{{vk::DescriptorType::eStorageBuffer,3},{vk::DescriptorType::eCombinedImageSampler,1}}};
      auto descriptors=device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo({},1,poolSizes));
      auto sets=device->allocateDescriptorSets(vk::DescriptorSetAllocateInfo(*descriptors,1,&*layout));
      vk::DescriptorBufferInfo viewInfo(*views->buffer,0,VK_WHOLE_SIZE),materialInfo(*material->buffer,0,64);
      vk::DescriptorImageInfo textureInfo(*sampler,*textureView,vk::ImageLayout::eShaderReadOnlyOptimal);
      const std::array<vk::WriteDescriptorSet,3> writes{{{sets[0],1,0,1,vk::DescriptorType::eStorageBuffer,nullptr,&viewInfo},
         {sets[0],2,0,1,vk::DescriptorType::eStorageBuffer,nullptr,&materialInfo},{sets[0],7,0,1,vk::DescriptorType::eCombinedImageSampler,&textureInfo}}};device->updateDescriptorSets(writes,{});
      auto commandsPool=device->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,family));
      auto commands=device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(*commandsPool,vk::CommandBufferLevel::ePrimary,1));
      RenderSceneClear();RenderSceneMaterial mat{};mat.doubleSided=1;for(float &value:mat.baseColor)value=1;mat.roughness=.8f;mat.alphaCutoff=.5f;
      const uint8_t initialPixel[]={255,255,255,255};
      mat.baseColorTexture=RenderSceneCreateTexture(1,1,initialPixel,4);
      const auto materialId=RenderSceneCreateMaterial(&mat);
      RenderSceneVertex vertices[3]={{{-1,-1,0},{0,0,-1},{0,0}},{{1,-1,0},{0,0,-1},{1,0}},{{0,1,0},{0,0,-1},{.5f,1}}};
      const uint32_t indices[]={0,1,2};RenderSceneMesh mesh{vertices,3,indices,3,materialId,1};const auto meshId=RenderSceneCreateMesh(&mesh);
      RenderSceneInstance object{};object.mesh=meshId;object.castsShadow=1; // Explicitly offscreen/invisible raster caster.
      for(unsigned i=0;i<4;++i)object.transform[i*5]=object.previousTransform[i*5]=1;
      const auto objectId=RenderSceneCreateInstance(&object);check(objectId!=0,"caster instance");
      RenderSceneCamera camera{};camera.exposure=camera.lightScale=1;
      const float eye[]={0,0,-3},forward[]={0,0,1},right[]={1,0,1},bottom[]={0,1,1};
      check(RenderSceneCameraFromRays(&camera,eye,forward,right,bottom,.1f,1000) && RenderSceneSetCamera(&camera),"camera");
      RenderSceneLight light{};light.type=RenderScenePointLight;light.position[2]=-2;light.range=10;light.enabled=light.castsShadow=1;
      light.intensity=1;light.color[0]=light.color[1]=light.color[2]=1;const auto lightId=RenderSceneCreateLight(&light);
      dark::ShadowSettings settings;settings.localFaces=6;settings.localResolution=64;settings.sunResolution=128;settings.shadowDistance=16;
      settings.staticDepthCache=std::getenv("DARK_TEST_STATIC_SHADOWS")!=nullptr;
      dark::ShadowSelection selection;selection.localLights={lightId};selection.localFaces=6;
      dark::ShadowPlan plan;bool uploaded=false;uint8_t previousAlpha=255;
      auto submit=[&](vk::CommandBuffer command) {
         auto fence=device->createFenceUnique({});vk::SubmitInfo info;info.commandBufferCount=1;info.pCommandBuffers=&command;
         queue.submit(info,*fence);check(device->waitForFences(*fence,VK_TRUE,UINT64_MAX)==vk::Result::eSuccess,"depth readback wait");
      };
      auto depthPixels=[&](uint32_t slot=0,uint32_t viewIndex=0) {
         const bool sun=plan.views[viewIndex].metadata[0]==2;
         const uint32_t size=sun?settings.sunResolution:settings.localResolution,layer=plan.views[viewIndex].metadata[1];
         auto pixels=host(vk::DeviceSize(size)*size*4,vk::BufferUsageFlagBits::eTransferDst);
         auto cmd=*commands[0];cmd.reset();cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
         const vk::Image image=sun?shadows.SunImage(slot):shadows.LocalImage(slot);
         vk::ImageMemoryBarrier barrier(vk::AccessFlagBits::eMemoryRead|vk::AccessFlagBits::eMemoryWrite,vk::AccessFlagBits::eTransferRead,
            vk::ImageLayout::eDepthStencilReadOnlyOptimal,vk::ImageLayout::eTransferSrcOptimal,VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,image,{vk::ImageAspectFlagBits::eDepth,0,1,layer,1});
         cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,vk::PipelineStageFlagBits::eTransfer,{}, {}, {},barrier);
         cmd.copyImageToBuffer(image,vk::ImageLayout::eTransferSrcOptimal,*pixels->buffer,vk::BufferImageCopy(0,0,0,{vk::ImageAspectFlagBits::eDepth,0,layer,1},{0,0,0},{size,size,1}));
         barrier.srcAccessMask=vk::AccessFlagBits::eTransferRead;barrier.dstAccessMask=vk::AccessFlagBits::eShaderRead;
         barrier.oldLayout=vk::ImageLayout::eTransferSrcOptimal;barrier.newLayout=vk::ImageLayout::eDepthStencilReadOnlyOptimal;
         cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eFragmentShader,{}, {}, {},barrier);
         vk::MemoryBarrier hostRead(vk::AccessFlagBits::eTransferWrite,vk::AccessFlagBits::eHostRead);
         cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},hostRead,{},{});
         cmd.end();submit(cmd);
         const auto *bytes=static_cast<const unsigned char *>(pixels->mapped);
         return std::vector<unsigned char>(bytes,bytes+size_t(size)*size*4);
      };
      auto sample=[&](uint32_t slot,uint32_t face,uint8_t alpha=255) {
         check(dark::BuildShadowPlan(dark::GetWorldScene(),settings,selection,plan,&plan),"build shadow cameras");
         check(geometry.Update(slot,dark::GetWorldScene(),error),error.c_str());check(plan.views.size()<=10,"test view buffer capacity");
         std::memcpy(views->mapped,plan.views.data(),plan.views.size()*sizeof(dark::ShadowView));
         struct Material {float color[4],emission[4];uint32_t texture,opacity;float metallic,cutoff;uint32_t emissionUsesBaseTexture,doubleSided,padding[2];} packed{};static_assert(sizeof(Material)==64,"world material ABI");packed.doubleSided=mat.doubleSided;
         std::copy(mat.baseColor,mat.baseColor+4,packed.color);packed.opacity=mat.opacity;packed.cutoff=mat.alphaCutoff;std::memcpy(material->mapped,&packed,sizeof(packed));
         const uint8_t pixel[]={255,255,255,alpha};std::memcpy(staging->mapped,pixel,4);
         if(alpha!=previousAlpha){check(RenderSceneUpdateTexture(mat.baseColorTexture,pixel,4),"opacity source revision");previousAlpha=alpha;}
         auto command=*commands[0];command.reset();command.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
         vk::ImageMemoryBarrier upload(uploaded?vk::AccessFlagBits::eShaderRead:vk::AccessFlags{},vk::AccessFlagBits::eTransferWrite,
            uploaded?vk::ImageLayout::eShaderReadOnlyOptimal:vk::ImageLayout::eUndefined,vk::ImageLayout::eTransferDstOptimal,
            VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,*texture,{vk::ImageAspectFlagBits::eColor,0,1,0,1});
         command.pipelineBarrier(uploaded?vk::PipelineStageFlagBits::eFragmentShader:vk::PipelineStageFlagBits::eTopOfPipe,vk::PipelineStageFlagBits::eTransfer,{}, {}, {},upload);
         command.copyBufferToImage(*staging->buffer,*texture,vk::ImageLayout::eTransferDstOptimal,vk::BufferImageCopy(0,0,0,{vk::ImageAspectFlagBits::eColor,0,0,1},{0,0,0},{1,1,1}));
         upload.srcAccessMask=vk::AccessFlagBits::eTransferWrite;upload.dstAccessMask=vk::AccessFlagBits::eShaderRead;upload.oldLayout=vk::ImageLayout::eTransferDstOptimal;upload.newLayout=vk::ImageLayout::eShaderReadOnlyOptimal;
         command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eFragmentShader,{}, {}, {},upload);uploaded=true;
         check(shadows.Prepare(command,slot,settings,error),error.c_str());
         check(shadows.Record(command,slot,plan,dark::GetWorldScene(),geometry,sets[0],{{materialId,0}},error),error.c_str());
         const auto &view=plan.views.at(face);const bool sun=view.metadata[0]==2;const vk::Image image=sun?shadows.SunImage(slot):shadows.LocalImage(slot);
         const uint32_t layer=view.metadata[1],size=sun?settings.sunResolution:settings.localResolution;
         vk::ImageMemoryBarrier barrier(vk::AccessFlagBits::eDepthStencilAttachmentWrite,vk::AccessFlagBits::eTransferRead,
            vk::ImageLayout::eDepthStencilReadOnlyOptimal,vk::ImageLayout::eTransferSrcOptimal,VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,image,{vk::ImageAspectFlagBits::eDepth,0,1,layer,1});
         command.pipelineBarrier(vk::PipelineStageFlagBits::eLateFragmentTests,vk::PipelineStageFlagBits::eTransfer,{}, {}, {},barrier);
         command.copyImageToBuffer(image,vk::ImageLayout::eTransferSrcOptimal,*readback->buffer,vk::BufferImageCopy(0,0,0,{vk::ImageAspectFlagBits::eDepth,0,layer,1},{int32_t(size/2),int32_t(size/2),0},{1,1,1}));
         barrier.srcAccessMask=vk::AccessFlagBits::eTransferRead;barrier.dstAccessMask=vk::AccessFlagBits::eShaderRead;barrier.oldLayout=vk::ImageLayout::eTransferSrcOptimal;barrier.newLayout=vk::ImageLayout::eDepthStencilReadOnlyOptimal;
         command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eFragmentShader,{}, {}, {},barrier);
         vk::MemoryBarrier read(vk::AccessFlagBits::eTransferWrite,vk::AccessFlagBits::eHostRead);command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},read,{},{});command.end();
         submit(command);return *static_cast<const float *>(readback->mapped);
      };
      check(std::abs(sample(0,4)-.2f)<.002f,"point radial depth");
      check(std::abs(sample(0,4)-.2f)<.002f,"cached depth contents");
      check(shadows.FrameStats(0).reusedViews==6 && shadows.FrameStats(0).renderedViews==0,"unchanged point faces reused");
      check(shadows.FrameStats(0).boundsTests==0,"unchanged caster/frustum membership reused");
      {
         const bool previousStatic=settings.staticDepthCache;settings.staticDepthCache=true;
         auto moving=object;moving.transform[14]=.5f;
         const auto movingId=RenderSceneCreateInstance(&moving);check(movingId!=0,"topology cache moving caster");
         for(unsigned warm=0;warm<6;++warm) {
            moving.transform[12]=.02f*warm;
            check(RenderSceneUpdateInstance(movingId,&moving),"warm stable base with moving caster");
            for(uint32_t slot=0;slot<2;++slot)sample(slot,4);
         }
         const auto added=RenderSceneCreateInstance(&object);check(added!=0,"unrelated caster topology addition");
         moving.transform[12]+=.02f;check(RenderSceneUpdateInstance(movingId,&moving),"dirty depth after topology addition");
         for(uint32_t slot=0;slot<2;++slot) {
            check(std::abs(sample(slot,4)-.2f)<.002f,"topology addition preserves blocker depth");
            const auto work=shadows.FrameStats(slot);
            check(work.staticRendered==0 && work.staticReused>0,"topology addition preserves unchanged static base");
         }
         check(RenderSceneDestroy(added),"remove newly added caster");
         moving.transform[12]+=.02f;check(RenderSceneUpdateInstance(movingId,&moving),"dirty depth after topology removal");
         std::array<std::vector<unsigned char>,2> composed;
         for(uint32_t slot=0;slot<2;++slot) {
            check(std::abs(sample(slot,4)-.2f)<.002f,"topology removal preserves blocker depth");
            const auto work=shadows.FrameStats(slot);
            check(work.staticRendered==0 && work.staticReused>0,"topology removal preserves unchanged static base");
            composed[slot]=depthPixels(slot,4);
         }
         auto controller=object;controller.transform[14]=.8f;
         const auto controllerId=RenderSceneCreateInstance(&controller);check(controllerId!=0,"moving controller for paused caster");
         // The first moving caster now pauses while another one keeps maps
         // dirty. It must not be repeatedly baked into and out of the base.
         for(unsigned pause=0;pause<6;++pause) {
            controller.transform[12]=.02f*pause;
            check(RenderSceneUpdateInstance(controllerId,&controller),"dirty maps while prior caster pauses");
            for(uint32_t slot=0;slot<2;++slot)sample(slot,4);
         }
         moving.transform[12]+=.02f;check(RenderSceneUpdateInstance(movingId,&moving),"resume paused caster");
         for(uint32_t slot=0;slot<2;++slot) {
            sample(slot,4);const auto work=shadows.FrameStats(slot);
            check(work.staticRendered==0 && work.staticReused>0,"resuming known moving caster preserves static base");
            composed[slot]=depthPixels(slot,4);
         }
         settings.staticDepthCache=false;
         for(uint32_t slot=0;slot<2;++slot) {
            sample(slot,4);check(depthPixels(slot,4)==composed[slot],"topology composition matches complete depth image redraw");
         }
         check(RenderSceneDestroy(controllerId),"retire paused-caster controller");
         check(RenderSceneDestroy(movingId),"retire topology fixture moving caster");
         settings.staticDepthCache=previousStatic;
         for(uint32_t slot=0;slot<2;++slot)sample(slot,4);
      }
      check(sample(0,0)==1,"warm excluded point-face membership");
      const auto originalPlacement=object;
      object.transform[0]=object.transform[10]=.70710678f;
      object.transform[2]=-.70710678f;object.transform[8]=.70710678f;
      object.transform[12]=5;object.transform[14]=-2;
      check(RenderSceneUpdateInstance(objectId,&object),"move caster into previously excluded face");
      const float enteredDepth=sample(0,0);std::printf("Cached face entry depth %.6f\n",enteredDepth);
      // Readback is at the center texel's sample, half a texel off the cube
      // axis. Intersect that ray with the tilted plane, then use radial depth.
      const float halfTexel=1.f/settings.localResolution;
      const float expectedEntry=.5f*std::sqrt(1+2*halfTexel*halfTexel)/(1-halfTexel);
      check(std::abs(enteredDepth-expectedEntry)<.002f,"newly intersecting caster enters cached face");
      check(shadows.FrameStats(0).boundsTests>0,"changed bounds retest cached membership");
      object=originalPlacement;check(RenderSceneUpdateInstance(objectId,&object),"restore cached membership fixture");
      check(std::abs(sample(0,4)-.2f)<.002f,"restored caster reenters original face");
      mat.baseColor[0]=.3f;mat.emission[0]=2;mat.roughness=.2f;mat.metallic=.8f;
      check(RenderSceneUpdateMaterial(materialId,&mat),"change opaque appearance");
      check(std::abs(sample(0,4)-.2f)<.002f,"opaque appearance preserves shadow depth");
      check(shadows.FrameStats(0).reusedViews==6,"opaque appearance reuses cached depth");
      const uint8_t recolored[]={20,80,160,255};
      check(RenderSceneUpdateTexture(mat.baseColorTexture,recolored,4),"opaque RGB texture change");
      check(std::abs(sample(0,4)-.2f)<.002f && shadows.FrameStats(0).reusedViews==6,"opaque texture change reuses cached depth");
      mat.opacity=RenderSceneMasked;check(RenderSceneUpdateMaterial(materialId,&mat),"masked RGB cache fixture");
      check(std::abs(sample(0,4)-.2f)<.002f,"masked RGB fixture casts");
      const uint8_t recoloredAgain[]={160,80,20,255};
      check(RenderSceneUpdateTexture(mat.baseColorTexture,recoloredAgain,4),"masked RGB-only change");
      check(std::abs(sample(0,4)-.2f)<.002f && shadows.FrameStats(0).reusedViews==6,"masked RGB change preserves cached depth");
      mat.baseColor[3]=0;check(RenderSceneUpdateMaterial(materialId,&mat),"masked material alpha change");
      check(sample(0,4)==1 && shadows.FrameStats(0).renderedViews>0,"material alpha invalidates cached depth");
      mat.baseColor[3]=1;mat.opacity=RenderSceneOpaque;check(RenderSceneUpdateMaterial(materialId,&mat),"restore opaque cache fixture");
      check(std::abs(sample(0,4)-.2f)<.002f,"restore opaque depth");
      if(families[family].timestampValidBits)
         check(shadows.FrameStats(0).gpuMilliseconds>=0,"retired slot exposes completed GPU timestamps");
      check(sample(1,5)==1,"opposite point face clear");
      mat.doubleSided=0;check(RenderSceneUpdateMaterial(materialId,&mat),"single-sided caster");
      const float firstSide=sample(0,4);
      object.transform[0]=-1;check(RenderSceneUpdateInstance(objectId,&object),"mirror caster winding");
      const float reverseSide=sample(1,4);
      check(std::max(firstSide,reverseSide)==1 && std::abs(std::min(firstSide,reverseSide)-.2f)<.002f,"only front winding casts shadow");
      mat.doubleSided=1;check(RenderSceneUpdateMaterial(materialId,&mat),"two-sided caster");
      check(std::abs(sample(0,4)-.2f)<.002f,"two-sided mirrored caster shadows");
      object.transform[0]=1;check(RenderSceneUpdateInstance(objectId,&object),"restore caster winding");
      mat.opacity=RenderSceneMasked;check(RenderSceneUpdateMaterial(materialId,&mat),"mask material");check(sample(0,4,0)==1,"cutout hole does not cast");
      check(std::abs(sample(1,4)-.2f)<.002f,"opaque cutout casts");
      mat.opacity=RenderSceneBlended;check(RenderSceneUpdateMaterial(materialId,&mat),"blend material");check(sample(0,4)==1,"blended caster excluded");
      mat.opacity=RenderSceneOpaque;check(RenderSceneUpdateMaterial(materialId,&mat),"opaque material");
      object.transform[12]=5;check(RenderSceneUpdateInstance(objectId,&object),"move caster");check(sample(0,4)==1,"moved instance no longer covers ray");
      check(shadows.FrameStats(0).boundsTests>0,"moving caster invalidates cached frustum membership");
      object.transform[12]=0;check(RenderSceneUpdateInstance(objectId,&object),"restore caster");
      for(auto &vertex:vertices)vertex.position[0]+=5;check(RenderSceneUpdateVertices(meshId,vertices,3),"pose mesh");check(sample(1,4)==1,"deformed caster leaves shadow sample");
      for(auto &vertex:vertices)vertex.position[0]-=5;check(RenderSceneUpdateVertices(meshId,vertices,3),"restore pose");
      // Retiring an object must clear its cached silhouette independently in
      // both slots; recreating the same placement with a new handle restores it.
      check(RenderSceneDestroy(objectId),"remove cached caster");
      for(uint32_t slot=0;slot<2;++slot)check(sample(slot,4)==1,"removed caster clears cached depth");
      const auto replacementObject=RenderSceneCreateInstance(&object);check(replacementObject!=0,"replacement caster");
      for(uint32_t slot=0;slot<2;++slot)check(std::abs(sample(slot,4)-.2f)<.002f,"replacement caster restores cached depth");
      light.type=RenderSceneSpotLight;light.direction[2]=1;light.innerCos=.9f;light.outerCos=.8f;check(RenderSceneUpdateLight(lightId,&light),"spot light");
      check(std::abs(sample(0,0)-.2f)<.0001f,"spot linear depth");
      light.type=RenderSceneDirectionalLight;check(RenderSceneUpdateLight(lightId,&light),"sun light");selection.localLights.clear();selection.sunlight=lightId;
      const float depth=sample(1,0);const auto &view=plan.views[0];const float expected=view.depthPlane[3]/view.parameters[0];
      check(std::abs(depth-expected)<.0001f && depth<1,"directional cascade linear depth");
      const bool previousStaticDepth=settings.staticDepthCache;
      const auto previousSunResolution=settings.sunResolution;
      auto staticSunCaster=object;staticSunCaster.transform[12]=.25f;staticSunCaster.transform[14]=.5f;
      const auto staticSunCasterId=RenderSceneCreateInstance(&staticSunCaster);check(staticSunCasterId!=0,"static sun composition caster");
      settings.staticDepthCache=true;settings.sunResolution*=2;
      for(uint32_t slot=0;slot<2;++slot) {
         const float measured=sample(slot,0);const auto &current=plan.views[0];
         check(std::abs(measured-current.depthPlane[3]/current.parameters[0])<.0001f,"direct sun depth matches on a fresh projection");
         const auto work=shadows.FrameStats(slot);
         check(work.depthCopies==0 && work.staticRendered==0 && work.staticReused==0,"fresh sun projection bypasses stable composition");
      }
      for(unsigned warm=0;warm<5;++warm)for(uint32_t slot=0;slot<2;++slot)sample(slot,0);
      const auto previousCaster=dark::GetWorldScene().instances.at(replacementObject);
      auto movedCaster=previousCaster;movedCaster.transform[12]+=.01f;
      check(RenderSceneUpdateInstance(replacementObject,&movedCaster),"move caster under stable sun projection");
      std::array<std::vector<unsigned char>,2> composedSun;
      for(uint32_t slot=0;slot<2;++slot) {
         const float measured=sample(slot,0);const auto &current=plan.views[0];
         check(std::abs(measured-current.depthPlane[3]/current.parameters[0])<.0001f,"stable sun composition retains exact depth");
         const auto work=shadows.FrameStats(slot);
         check(work.depthCopies==settings.sunCascades && work.staticRendered+work.staticReused==settings.sunCascades,"stable sun projection uses composition for changing casters");
         composedSun[slot]=depthPixels(slot);
      }
      settings.staticDepthCache=false;
      for(uint32_t slot=0;slot<2;++slot) {
         sample(slot,0);
         check(depthPixels(slot)==composedSun[slot],"composed sun depth matches direct full-image redraw exactly");
      }
      check(RenderSceneDestroy(staticSunCasterId),"retire static sun composition caster");
      check(RenderSceneUpdateInstance(replacementObject,&previousCaster),"restore sun caster");
      settings.staticDepthCache=previousStaticDepth;settings.sunResolution=previousSunResolution;
      // Reallocate each retired sun array, including shrinking its layer
      // count. Newly created images must never inherit old layer signatures.
      for(const auto resolution:{64u,256u,128u}) {
         settings.sunResolution=resolution;settings.sunCascades=resolution==64?1:4;
         for(uint32_t slot=0;slot<2;++slot) {
            const float measured=sample(slot,0);const auto &sunView=plan.views[0];
            check(std::abs(measured-sunView.depthPlane[3]/sunView.parameters[0])<.0001f,"sun depth survives array resize");
            check(shadows.FrameStats(slot).renderedViews==settings.sunCascades && shadows.FrameStats(slot).reusedViews==0,"resized sun layers redraw");
            check(std::abs(sample(slot,0)-measured)<.0001f,"resized sun depth remains cached");
            check(shadows.FrameStats(slot).reusedViews==settings.sunCascades && shadows.FrameStats(slot).renderedViews==0,"resized sun layers cache after submission");
         }
      }
      light.type=RenderScenePointLight;check(RenderSceneUpdateLight(lightId,&light),"restore point after sun");
      selection.sunlight=0;selection.localLights={lightId};settings.localResolution=128;
      check(std::abs(sample(1,4)-.2f)<.002f,"retired slot can replace shadow resolution safely");
      auto spot=light;spot.type=RenderSceneSpotLight;spot.outerCos=.8f;
      const auto spotId=RenderSceneCreateLight(&spot);check(spotId!=0,"retained spot light");
      settings.localFaces=7;selection.localLights={lightId,spotId};
      for(uint32_t slot=0;slot<2;++slot) {
         selection.localLights={lightId,spotId};
         check(std::abs(sample(slot,6)-.2f)<.002f,"spot depth before view-order change");
         const auto layer=plan.views[6].metadata[1];
         selection.localLights={spotId};
         check(std::abs(sample(slot,0)-.2f)<.002f,"retained spot depth after view-order change");
         check(plan.views[0].metadata[1]==layer,"retained spot keeps its array layer");
         check(shadows.FrameStats(slot).reusedViews==1 && shadows.FrameStats(slot).renderedViews==0,
            "view-buffer reorder preserves cached shadow depth");
      }
      {
         auto extra=object;extra.transform[0]=.7f;extra.transform[12]=.75f;extra.transform[13]=.25f;
         const auto extraId=RenderSceneCreateInstance(&extra);check(extraId!=0,"second transformed caster for image equivalence");
         settings.localResolution=64;settings.indirect=true;sample(0,0);
         const auto batched=depthPixels();
         if(features.IndirectShadows())check(shadows.FrameStats(0).drawBatches<shadows.FrameStats(0).drawCalls,"depth image uses multi-draw batch");
         settings.localResolution=32;sample(0,0); // Retire and recreate storage to force the scalar redraw.
         settings.localResolution=64;settings.indirect=false;sample(0,0);
         check(depthPixels()==batched,"scalar and indirect depth images match exactly for transformed casters");
         check(shadows.FrameStats(0).drawBatches==shadows.FrameStats(0).drawCalls,"scalar comparison actually uses direct draws");
         settings.indirect=features.IndirectShadows();settings.staticDepthCache=true;
         for(unsigned warm=0;warm<5;++warm)for(uint32_t slot=0;slot<2;++slot)sample(slot,0);
         for(unsigned step=0;step<2;++step) {
            extra.transform[12]=1.0f+.25f*step;check(RenderSceneUpdateInstance(extraId,&extra),"move dynamic caster over stable base");
            for(uint32_t slot=0;slot<2;++slot) {
               sample(slot,0);const auto work=shadows.FrameStats(slot);
               check(work.depthCopies>0,"stable depth composes into current map");
               check(step?work.staticReused>0:work.staticRendered>0,"stable depth rebuilds then reuses under moving caster");
            }
         }
         std::array<std::vector<unsigned char>,2> composed;
         for(uint32_t slot=0;slot<2;++slot){sample(slot,0);composed[slot]=depthPixels(slot);}
         settings.staticDepthCache=false;
         for(uint32_t slot=0;slot<2;++slot) {
            settings.localResolution=32;sample(slot,0);settings.localResolution=64;sample(slot,0);
            check(depthPixels(slot)==composed[slot],"composed static/dynamic depth matches full redraw exactly");
         }
         // Move the original caster so the extra caster enters the stable base,
         // then retire the latter and compare against a fresh complete render.
         settings.staticDepthCache=true;for(unsigned warm=0;warm<5;++warm)for(uint32_t slot=0;slot<2;++slot)sample(slot,0);
         object.transform[13]=.2f;check(RenderSceneUpdateInstance(replacementObject,&object),"dirty map while extra caster stays stable");
         for(uint32_t slot=0;slot<2;++slot)sample(slot,0);
         check(RenderSceneDestroy(extraId),"retire caster held in stable depth");
         for(uint32_t slot=0;slot<2;++slot){sample(slot,0);composed[slot]=depthPixels(slot);}
         settings.staticDepthCache=false;
         for(uint32_t slot=0;slot<2;++slot) {
            settings.localResolution=32;sample(slot,0);settings.localResolution=64;sample(slot,0);
            check(depthPixels(slot)==composed[slot],"retiring stable caster leaves no stale silhouette");
         }
      }
      shadows.Shutdown();geometry.Shutdown();RenderSceneClear();
      std::printf("PASS on %s: point/spot/sun depth maps, invisible casters, alpha holes, blend exclusion, movement, deformation, caster retirement and sun array recreation in two frame slots\n",physical.getProperties().deviceName.data());return 0;
   }
   std::puts("SKIP: texture indexing unavailable");return 77;
 }catch(const std::exception &error){std::fprintf(stderr,"Shadow-map regression failed: %s\n",error.what());return 1;}
}
