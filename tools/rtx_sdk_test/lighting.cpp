#include "../../libsrc/lgd3d/vulkan_world_features.hpp"
#include "../../libsrc/lgd3d/vulkan_world_renderer.hpp"
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <vulkan/vulkan_raii.hpp>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <cstring>
static void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
int main(int argc,char **argv) {
 try {
   check(argc==2,"pass generated Vulkan shader directory");
   vk::raii::Context context;
   vk::ApplicationInfo app("Dark RTX SDK lighting",1,"Dark",1,VK_API_VERSION_1_1);
   auto instance=context.createInstance(vk::InstanceCreateInfo({},&app));
   for(const auto physical:(*instance).enumeratePhysicalDevices()) {
      dark::VulkanWorldFeatures features;if(!features.Probe(physical,true) || !features.rayTracing)continue;
      auto queues=physical.getQueueFamilyProperties();uint32_t family=0;
      while(family<queues.size() && !(queues[family].queueFlags & vk::QueueFlagBits::eGraphics))++family;
      check(family<queues.size(),"no graphics queue");const float priority=1;
      vk::DeviceQueueCreateInfo qi({},family,1,&priority);
      vk::DeviceCreateInfo di({},1,&qi,0,nullptr,uint32_t(features.extensions.size()),features.extensions.data());di.pNext=&features.features;
      auto device=physical.createDeviceUnique(di);const auto queue=device->getQueue(family,0);
      auto pool=device->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,family));
      auto commands=device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(*pool,vk::CommandBufferLevel::ePrimary,1));

      const std::array<vk::AttachmentDescription,2> attachments{{
         {{},vk::Format::eR8G8B8A8Unorm,vk::SampleCountFlagBits::e1,vk::AttachmentLoadOp::eClear,vk::AttachmentStoreOp::eStore,
          vk::AttachmentLoadOp::eDontCare,vk::AttachmentStoreOp::eDontCare,vk::ImageLayout::eUndefined,vk::ImageLayout::eColorAttachmentOptimal},
         {{},vk::Format::eD32Sfloat,vk::SampleCountFlagBits::e1,vk::AttachmentLoadOp::eClear,vk::AttachmentStoreOp::eStore,
          vk::AttachmentLoadOp::eDontCare,vk::AttachmentStoreOp::eDontCare,vk::ImageLayout::eUndefined,vk::ImageLayout::eDepthStencilAttachmentOptimal}}};
      vk::AttachmentReference color(0,vk::ImageLayout::eColorAttachmentOptimal),depth(1,vk::ImageLayout::eDepthStencilAttachmentOptimal);
      vk::SubpassDescription subpass({},vk::PipelineBindPoint::eGraphics,0,nullptr,1,&color,nullptr,&depth);
      auto pass=device->createRenderPassUnique(vk::RenderPassCreateInfo({},attachments,subpass));
      dark::VulkanWorldRenderer renderer;std::string error;
      check(renderer.Initialize(physical,*device,family,*pass,vk::Format::eD32Sfloat,vk::Format::eR8G8B8A8Unorm,argv[1],error),error.c_str());
      dark::VulkanWorldRenderer::RaySettings rays;rays.gi=false;rays.sharcStatistics=true;
      rays.denoiserFrameTimeMilliseconds=1000.f/60;
      renderer.SetRaySettings(rays);
      check(renderer.RayTracingEnabled(),"ray tracing pipeline selected");
      auto output=device->createBufferUnique(vk::BufferCreateInfo({},32,vk::BufferUsageFlagBits::eTransferDst,vk::SharingMode::eExclusive));
      const auto requirements=device->getBufferMemoryRequirements(*output);const auto memory=physical.getMemoryProperties();uint32_t type=0;
      const auto wanted=vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent;
      while(type<memory.memoryTypeCount && (!(requirements.memoryTypeBits&(1u<<type)) || (memory.memoryTypes[type].propertyFlags&wanted)!=wanted))++type;
      check(type<memory.memoryTypeCount,"host readback memory");
      auto allocation=device->allocateMemoryUnique(vk::MemoryAllocateInfo(requirements.size,type));device->bindBufferMemory(*output,*allocation,0);
      const auto *mapped=static_cast<const uint16_t *>(device->mapMemory(*allocation,0,32));
      auto submit=[&](vk::CommandBuffer command){
         auto fence=device->createFenceUnique({});vk::SubmitInfo info;info.commandBufferCount=1;info.pCommandBuffers=&command;
         queue.submit(info,*fence);check(device->waitForFences(*fence,VK_TRUE,UINT64_MAX)==vk::Result::eSuccess,"GPU test wait");
      };
      RenderSceneClear();RenderSceneMaterial material{};material.doubleSided=1;
      material.baseColor[0]=material.baseColor[1]=material.baseColor[2]=0.5f;material.baseColor[3]=1;material.roughness=0.8f;material.alphaCutoff=0.5f;
      const auto receiverMaterial=RenderSceneCreateMaterial(&material);
      RenderSceneVertex receiverVertices[3]={{{-4,-4,0},{0,0,-1},{0,0}},{{4,-4,0},{0,0,-1},{1,0}},{{0,4,0},{0,0,-1},{0.5,1}}};
      uint32_t indices[]={0,1,2};RenderSceneMesh receiverMesh{receiverVertices,3,indices,3,receiverMaterial,1};
      const auto receiver=RenderSceneCreateMesh(&receiverMesh);
      RenderSceneInstance instanceData{};instanceData.mesh=receiver;instanceData.visible=instanceData.castsShadow=1;
      for(int i=0;i<4;++i)instanceData.transform[i*5]=instanceData.previousTransform[i*5]=1;
      const auto receiverInstance=RenderSceneCreateInstance(&instanceData);check(receiverInstance!=0,"receiver instance");
      const auto occluderMaterial=RenderSceneCreateMaterial(&material);
      RenderSceneVertex occluderVertices[3]={{{0.5f,-0.5f,-1},{0,0,-1},{0,0}},{{1.5f,-0.5f,-1},{0,0,-1},{1,0}},{{1,0.5f,-1},{0,0,-1},{0.5,1}}};
      RenderSceneMesh occluderMesh{occluderVertices,3,indices,3,occluderMaterial,1};const auto occluder=RenderSceneCreateMesh(&occluderMesh);
      instanceData.mesh=occluder;instanceData.visible=1;const auto caster=RenderSceneCreateInstance(&instanceData);check(caster!=0,"occluder instance");
      RenderSceneCamera camera{};camera.exposure=1;camera.ambient=0.01f;camera.lightScale=1;
      const float eye[]={0,0,-3},forward[]={0,0,1},right[]={.1f,0,1},bottom[]={0,.1f,1};
      check(RenderSceneCameraFromRays(&camera,eye,forward,right,bottom,0.1f,100),"camera projection");check(RenderSceneSetCamera(&camera),"set camera");
      check(!dark::SceneBoundsVisible(dark::GetWorldScene().meshes.at(occluder).bounds,instanceData.transform,camera.viewProjection),"caster outside raster frustum");
      RenderSceneLight light{};light.position[0]=2;light.position[2]=-2;light.intensity=100;light.enabled=1;light.castsShadow=1;
      light.color[0]=light.color[1]=light.color[2]=1;const auto lamp=RenderSceneCreateLight(&light);check(lamp!=0,"light");
      auto readSample=[&](uint32_t slot,uint32_t width=64,uint32_t height=64,int sampleX=-1,bool direct=false,bool denoised=false,bool specular=false){
         auto cmd=*commands[0];cmd.reset();cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
         const vk::Image hdr=direct?renderer.DirectImage(slot,specular,denoised):renderer.HdrImage(slot);
         check(bool(hdr),"lighting image exists");
         const auto layout=direct?vk::ImageLayout::eGeneral:vk::ImageLayout::eShaderReadOnlyOptimal;
         vk::ImageMemoryBarrier barrier(vk::AccessFlagBits::eColorAttachmentWrite,vk::AccessFlagBits::eTransferRead,
            layout,vk::ImageLayout::eTransferSrcOptimal,VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,hdr,
            {vk::ImageAspectFlagBits::eColor,0,1,0,1});
         barrier.srcAccessMask=vk::AccessFlagBits::eMemoryWrite;
         cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,vk::PipelineStageFlagBits::eTransfer,{}, {}, {},barrier);
         vk::BufferImageCopy copy(0,0,0,{vk::ImageAspectFlagBits::eColor,0,0,1},{sampleX<0?int32_t(width/2):sampleX,int32_t(height/2),0},{1,1,1});
         cmd.copyImageToBuffer(hdr,vk::ImageLayout::eTransferSrcOptimal,*output,copy);
         barrier.srcAccessMask=vk::AccessFlagBits::eTransferRead;barrier.dstAccessMask=vk::AccessFlagBits::eShaderRead;
         barrier.oldLayout=vk::ImageLayout::eTransferSrcOptimal;barrier.newLayout=layout;
         cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eAllCommands,{}, {}, {},barrier);
         vk::MemoryBarrier read(vk::AccessFlagBits::eTransferWrite,vk::AccessFlagBits::eHostRead);
         cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},read,{},{});cmd.end();submit(cmd);
         const unsigned exponent=(mapped[0]>>10)&31,mantissa=mapped[0]&1023;
         const float result=exponent?std::ldexp(1+mantissa/1024.0f,int(exponent)-15):std::ldexp(float(mantissa),-24);
         check(!(mapped[0]&0x8000) && exponent!=31,"finite nonnegative radiance");return result;
      };
      const dark::WorldScene *renderScene=&dark::GetWorldScene();
      auto sample=[&](uint32_t slot,uint32_t width=64,uint32_t height=64,int sampleX=-1){
         const auto recorded=renderer.Record(slot,*renderScene,width,height,error);check(recorded!=VK_NULL_HANDLE,error.c_str());submit(recorded);renderer.CompleteSubmission(true);
         return readSample(slot,width,height,sampleX);
      };
      // Geometry/motion assertions use one frame. Lighting assertions explicitly
      // allow the temporal estimators to converge after discontinuous edits.
      auto stableSample=[&](uint32_t slot) {
         for(unsigned i=0;i<48;++i)sample(i%2);
         return sample(slot);
      };
      auto surfaceSample=[&](uint32_t slot,uint32_t attachment) {
         const vk::Image image=attachment==6?renderer.NativeDepthImage(slot):renderer.SurfaceImage(slot,attachment);check(bool(image),"surface/depth image exists");
         const auto layout=attachment==6?vk::ImageLayout::eShaderReadOnlyOptimal:vk::ImageLayout::eGeneral;
         auto cmd=*commands[0];cmd.reset();cmd.begin(vk::CommandBufferBeginInfo{});
         vk::ImageMemoryBarrier barrier(vk::AccessFlagBits::eMemoryRead|vk::AccessFlagBits::eMemoryWrite,vk::AccessFlagBits::eTransferRead,
            layout,vk::ImageLayout::eTransferSrcOptimal,VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,image,{vk::ImageAspectFlagBits::eColor,0,1,0,1});
         cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,vk::PipelineStageFlagBits::eTransfer,{}, {},{},barrier);
         cmd.copyImageToBuffer(image,vk::ImageLayout::eTransferSrcOptimal,*output,vk::BufferImageCopy(0,0,0,{vk::ImageAspectFlagBits::eColor,0,0,1},{32,32,0},{1,1,1}));
         barrier.oldLayout=vk::ImageLayout::eTransferSrcOptimal;barrier.newLayout=layout;
         barrier.srcAccessMask=vk::AccessFlagBits::eTransferRead;barrier.dstAccessMask=vk::AccessFlagBits::eShaderRead;
         cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eFragmentShader,{}, {},{},barrier);
         const vk::MemoryBarrier host(vk::AccessFlagBits::eTransferWrite,vk::AccessFlagBits::eHostRead);
         cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},host,{},{});cmd.end();submit(cmd);
         std::array<float,4> value;std::memcpy(value.data(),mapped,sizeof(value));return value;
      };
      sample(0);check(surfaceSample(0,4)[3]==0,"first surface frame rejects motion history");
      sample(1);auto velocity=surfaceSample(1,4);
      check(velocity[3]==1 && std::abs(velocity[0])<.0001f && std::abs(velocity[1])<.0001f,"static surface has valid zero motion");
      const auto normal=surfaceSample(1,1);uint32_t packedNormal=0;std::memcpy(&packedNormal,normal.data(),4);
      const float packedR=float(packedNormal&1023)/1023,packedG=float((packedNormal>>10)&1023)/1023,packedB=float((packedNormal>>20)&1023)/1023;
      check(std::abs(packedR-.5f)<.002f && std::abs(packedG-.5f)<.002f && packedB<.5f && std::abs((1-2*packedB)-material.roughness)<.003f,"surface normal and roughness match NRD encoding");
      const auto ids=surfaceSample(1,5);std::array<uint32_t,4> identity;std::memcpy(identity.data(),ids.data(),sizeof(identity));
      check((uint64_t(identity[0])|(uint64_t(identity[1])<<32))==receiverInstance && (identity[3]&1)==1,"surface identity is stable across submitted frames");
      check((identity[3]>>1)==0x3fffffffu,"surface metadata retains negative-Z geometric normal");
      const auto position=surfaceSample(1,0);check(std::abs(position[2])<.001f && std::abs(position[3]-3)<.001f,"surface exports world position and linear view depth");
      // Authored maps must retain linear scalar values and reach the actual
      // raster guides/composition, rather than just material metadata.
      const uint8_t normalPixel[]={192,128,255,255},roughPixel[]={64,64,64,255},metalPixel[]={192,192,192,255};
      const uint8_t aoPixel[]={64,64,64,255},emissionPixel[]={128,64,32,255},ormPixel[]={0,255,0,255};
      auto mappedMaterial=material;
      mappedMaterial.normalTexture=RenderSceneCreateDataTexture(1,1,normalPixel,4);
      mappedMaterial.roughnessTexture=RenderSceneCreateDataTexture(1,1,roughPixel,4);
      mappedMaterial.metallicTexture=RenderSceneCreateDataTexture(1,1,metalPixel,4);
      mappedMaterial.occlusionTexture=RenderSceneCreateDataTexture(1,1,aoPixel,4);
      mappedMaterial.emissionTexture=RenderSceneCreateTexture(1,1,emissionPixel,4);
      mappedMaterial.ormTexture=RenderSceneCreateDataTexture(1,1,ormPixel,4);
      check(RenderSceneUpdateMaterial(receiverMaterial,&mappedMaterial),"assign PBR texture set");
      const float mappedHdr=sample(0);
      const auto mappedPacked=surfaceSample(0,1);uint32_t pbrBits=0;std::memcpy(&pbrBits,mappedPacked.data(),4);
      const float pr=float(pbrBits&1023)/1023,pg=float((pbrBits>>10)&1023)/1023,pb=float((pbrBits>>20)&1023)/1023;
      const float nx=pr-pg,ny=pr+pg-1,nz=-(1-std::abs(nx)-std::abs(ny));
      check(nx/std::sqrt(nx*nx+ny*ny+nz*nz)>.49f && std::abs(std::abs(pb*2-1)-64.f/255)<.003f,
         "normal map affects world shading normal and roughness remains linear");
      const auto mappedIdentity=surfaceSample(0,5);uint32_t geometricMetadata=0;std::memcpy(&geometricMetadata,mappedIdentity.data()+3,4);
      check((geometricMetadata>>1)==(identity[3]>>1),"mapped shading normal leaves geometric normal unchanged");
      auto halfPixel=[&](uint32_t attachment) {
         surfaceSample(0,attachment);std::array<float,4> values;
         for(unsigned c=0;c<4;++c) {
            const unsigned exponent=(mapped[c]>>10)&31,mantissa=mapped[c]&1023;
            values[c]=exponent?std::ldexp(1+mantissa/1024.f,int(exponent)-15):std::ldexp(float(mantissa),-24);
         }
         return values;
      };
      check(std::abs(halfPixel(2)[3]-192.f/255)<.002f,"separate metallic map overrides packed ORM");
      const auto emissionGuide=halfPixel(3);
      check(std::abs(emissionGuide[0]-.21586f)<.003f && std::abs(emissionGuide[3]-(1-64.f/255)*.25f)<.002f,
         "emissive texture samples sRGB while AO retains linear values");
      check(mappedHdr>.21f && mappedHdr<.24f,"emissive map contributes to displayed HDR");
      mappedMaterial.roughnessTexture=mappedMaterial.metallicTexture=mappedMaterial.occlusionTexture=0;
      check(RenderSceneUpdateMaterial(receiverMaterial,&mappedMaterial),"use packed ORM without separate overrides");sample(0);
      const auto ormNormal=surfaceSample(0,1);std::memcpy(&pbrBits,ormNormal.data(),4);
      check(std::abs(float((pbrBits>>20)&1023)/1023*2-1)>.99f && halfPixel(2)[3]<.001f && halfPixel(3)[3]>.249f,
         "packed ORM maps RGB to occlusion roughness metallic");
      check(!RenderSceneDestroy(mappedMaterial.normalTexture),"referenced normal map cannot be destroyed");
      check(RenderSceneUpdateMaterial(receiverMaterial,&material),"restore native material after PBR map checks");sample(0);sample(1);
      const auto defaultsPacked=surfaceSample(1,1);std::memcpy(&pbrBits,defaultsPacked.data(),4);
      check(std::abs(std::abs(float((pbrBits>>20)&1023)/1023*2-1)-.8f)<.003f,
         "material without roughness map retains native roughness default");
      sample(0);
      check(halfPixel(2)[3]==0 && halfPixel(3)[0]==0 && halfPixel(3)[3]==0,
         "removing PBR maps restores nonmetallic unoccluded nonemissive defaults");
      auto slopedReceiver=dark::GetWorldScene().instances.at(receiverInstance);
      const float slope=std::sqrt(.75f);
      slopedReceiver.transform[0]=slopedReceiver.transform[10]=slope;
      slopedReceiver.transform[2]=-.5f;slopedReceiver.transform[8]=.5f;
      check(RenderSceneUpdateInstance(receiverInstance,&slopedReceiver),"rotate geometric-normal fixture");sample(0);
      const auto slopedIdentity=surfaceSample(0,5);std::memcpy(&geometricMetadata,slopedIdentity.data()+3,4);
      const uint32_t oct=geometricMetadata>>1;
      float gx=float(oct&32767)/32767*2-1,gy=float((oct>>15)&32767)/32767*2-1;
      const float gz=1-std::abs(gx)-std::abs(gy);
      if(gz<0){const float x=gx;gx=(1-std::abs(gy))*(gx<0?-1:1);gy=(1-std::abs(x))*(gy<0?-1:1);}
      const float length=std::sqrt(gx*gx+gy*gy+gz*gz);
      check(std::abs(gx/length+.5f)<.001f && std::abs(gy/length)<.001f && std::abs(gz/length+slope)<.001f,
         "packed geometric normal follows rotated world geometry");
      slopedReceiver.transform[0]=slopedReceiver.transform[10]=1;slopedReceiver.transform[2]=slopedReceiver.transform[8]=0;
      check(RenderSceneUpdateInstance(receiverInstance,&slopedReceiver),"restore receiver after geometric-normal fixture");sample(0);sample(1);
      std::puts("PASS PBR normal/scalar/ORM/emissive maps, color spaces and displayed composition");
      auto movedReceiver=dark::GetWorldScene().instances.at(receiverInstance);movedReceiver.transform[12]=.05f;
      check(RenderSceneUpdateInstance(receiverInstance,&movedReceiver),"move primary surface");sample(0);velocity=surfaceSample(0,4);
      check(velocity[3]==1 && velocity[0]<-.05f,"surface motion points from current pixel to previous pixel");
      RenderSceneVertex movedVertices[3];std::copy(receiverVertices,receiverVertices+3,movedVertices);
      for(auto &vertex:movedVertices)vertex.position[0]+=.05f;
      check(RenderSceneUpdateVertices(receiver,movedVertices,3),"deform primary surface");sample(1);velocity=surfaceSample(1,4);
      check(velocity[3]==1 && velocity[0]<-.05f,"surface motion includes vertex deformation");
      movedReceiver.transform[12]=0;check(RenderSceneUpdateInstance(receiverInstance,&movedReceiver),"restore primary pose");
      check(RenderSceneUpdateVertices(receiver,receiverVertices,3),"restore primary geometry");
      std::puts("PASS raster surface guides, submitted-frame object motion and deformation motion");
      const float shadow=stableSample(0);check(shadow>.004f && shadow<.006f,"opaque offscreen RT caster leaves ambient");
      check(std::abs(stableSample(0)-shadow)<.001f,"cached BLAS retains shadow");
      check(readSample(0,64,64,-1,true)<.0001f,"RTXDI raw diffuse rejects occluded light");
      check(renderer.GetWorkload(0).blasBuilds==0,"unchanged mesh does not rebuild BLAS");
      instanceData.transform[12]=5;check(RenderSceneUpdateInstance(caster,&instanceData),"move caster");
      float revealedFiltered=0,revealedRawFirst=0;
      for(unsigned frame=0;frame<4;++frame) {
         sample(frame%2);
         revealedFiltered=readSample(frame%2,64,64,-1,true,true);
         if(!frame)revealedRawFirst=readSample(0,64,64,-1,true);
         std::printf("Caster reveal frame=%u raw=%.6f filtered=%.6f\n",frame+1,
            readSample(frame%2,64,64,-1,true),revealedFiltered);
      }
      const float lit=stableSample(0);check(lit>.1f && lit>shadow*10,"moving caster updates TLAS");
      const float directLit=readSample(0,64,64,-1,true);
      std::printf("RTXDI raw diffuse lit=%.6f\n",directLit);
      check(directLit>.01f,"actual RTXDI selected light contributes radiance");

      float denoisedLit=0;
      for(unsigned i=0;i<12;++i){sample(i%2);denoisedLit=readSample(i%2,64,64,-1,true,true);}
      const float convergedRaw=readSample(1,64,64,-1,true);
      std::printf("RTXDI RELAX diffuse lit=%.6f raw=%.6f\n",denoisedLit,convergedRaw);
      check(std::abs(denoisedLit-convergedRaw)<convergedRaw*.25f,"RELAX preserves converged radiance magnitude");
      check(denoisedLit>.005f,"RELAX preserves RTXDI direct radiance");
      check(revealedRawFirst>convergedRaw*.5f && revealedRawFirst<convergedRaw*1.5f,
         "temporal bias correction traces the previous caster pose on the first revealed frame");
      check(revealedFiltered>denoisedLit*.5f && revealedFiltered<denoisedLit*1.5f,
         "newly exposed light reaches the settled radiance range within four frames");
      instanceData.transform[12]=0;check(RenderSceneUpdateInstance(caster,&instanceData),"restore caster");
      float occludedFiltered=0;
      for(unsigned frame=0;frame<4;++frame) {
         sample(frame%2);
         const float raw=readSample(frame%2,64,64,-1,true);
         occludedFiltered=readSample(frame%2,64,64,-1,true,true);
         std::printf("Caster occlusion frame=%u raw=%.6f filtered=%.6f\n",frame+1,
            raw,occludedFiltered);
         check(raw<.0001f,"moving occluder rejects direct light on its first submitted frame");
      }
      check(occludedFiltered<denoisedLit*.05f,"RELAX removes moving-shadow residual within four frames");
      check(std::abs(stableSample(1)-shadow)<.001f,"restored pose restores shadow");
      check(readSample(1,64,64,-1,true)<.0001f,"RTXDI rejects newly occluded temporal lighting");
      // A foreground surface replaces the receiver at this pixel, then leaves.
      // Unlike the offscreen shadow test, this exercises primary-depth and
      // surface-identity disocclusion through actual RELAX history.
      instanceData.transform[12]=-1;instanceData.transform[14]=-1.5f;
      check(RenderSceneUpdateInstance(caster,&instanceData),"cover receiver with foreground geometry");
      for(unsigned frame=0;frame<8;++frame)sample(frame%2);
      auto foregroundIds=surfaceSample(1,5);std::array<uint32_t,4> foregroundIdentity;
      std::memcpy(foregroundIdentity.data(),foregroundIds.data(),sizeof(foregroundIdentity));
      check((uint64_t(foregroundIdentity[0])|(uint64_t(foregroundIdentity[1])<<32))==caster,
         "disocclusion fixture rasterizes the foreground instance");
      check(readSample(1,64,64,-1,true,true)<denoisedLit*.05f,
         "foreground facing away from the source rejects old background illumination");
      instanceData.transform[12]=5;instanceData.transform[14]=0;
      check(RenderSceneUpdateInstance(caster,&instanceData),"reveal receiver behind foreground geometry");
      float disoccluded=0;
      for(unsigned frame=0;frame<4;++frame) {
         sample(frame%2);
         const float raw=readSample(frame%2,64,64,-1,true);
         disoccluded=readSample(frame%2,64,64,-1,true,true);
         std::printf("Primary disocclusion frame=%u raw=%.6f filtered=%.6f\n",frame+1,raw,disoccluded);
         check(raw>convergedRaw*.5f && raw<convergedRaw*1.5f,
            "primary disocclusion restores direct lighting immediately");
      }
      check(disoccluded>denoisedLit*.5f && disoccluded<denoisedLit*1.5f,
         "RELAX restores a disoccluded receiver within four frames");
      instanceData.transform[12]=0;
      check(RenderSceneUpdateInstance(caster,&instanceData),"restore caster after primary disocclusion");
      stableSample(0);
      auto directMoments=[&](unsigned count) {
         double mean=0,squared=0;
         for(unsigned i=0;i<count;++i) {
            sample(i%2);const float value=readSample(i%2,64,64,-1,true);
            mean+=value/count;squared+=value*value/count;
         }
         return std::array<double,2>{mean,std::sqrt(std::max(0.0,squared-mean*mean))};
      };
      auto pointMean=[&](float casterX) {
         instanceData.transform[12]=casterX;
         check(RenderSceneUpdateInstance(caster,&instanceData),"move hard-shadow boundary");
         stableSample(0);
         return directMoments(64);
      };
      const auto clearPoint=pointMean(5),edgePoint=pointMean(.3f);
      std::printf("Point-source energy clear=%.6f near_shadow=%.6f ratio=%.6f stddev=%.6f/%.6f\n",clearPoint[0],edgePoint[0],edgePoint[0]/clearPoint[0],clearPoint[1],edgePoint[1]);
      check(std::abs(edgePoint[0]-clearPoint[0])<clearPoint[0]*.05 && edgePoint[1]<clearPoint[0]*.05,
         "unoccluded point-source energy and variance remain stable beside a hard shadow");
      instanceData.transform[12]=0;check(RenderSceneUpdateInstance(caster,&instanceData),"restore hard-shadow caster");
      stableSample(0);
      auto weakLight=light;weakLight.position[0]=-2;weakLight.intensity=1;
      const auto weakLamp=RenderSceneCreateLight(&weakLight);check(weakLamp!=0,"unblocked weak source");
      auto disabledBright=light;disabledBright.enabled=0;
      check(RenderSceneUpdateLight(lamp,&disabledBright),"measure weak source alone");
      stableSample(0);const auto weakOnly=directMoments(128);
      check(RenderSceneUpdateLight(lamp,&light),"enable occluded source one hundred times brighter");
      stableSample(0);const auto mixed=directMoments(128);
      std::printf("Occluded bright source: weak=%.6f mixed=%.6f stddev=%.6f\n",weakOnly[0],mixed[0],mixed[1]);
      check(weakOnly[0]>.001 && std::abs(mixed[0]-weakOnly[0])<weakOnly[0]*.5 && mixed[1]<weakOnly[0],
         "bright occluded source preserves converged weak-source energy without extreme variance");
      check(RenderSceneDestroy(weakLamp),"remove weak-source fixture");
      stableSample(0);
      // Mutate vertex data independently of its instance pose.
      sample(0); // Reuse this same retired slot after changing its BLAS below.
      RenderSceneVertex deformed[3];std::copy(occluderVertices,occluderVertices+3,deformed);
      for(auto &v:deformed)v.position[0]+=5;
      check(RenderSceneUpdateVertices(occluder,deformed,3),"deform mesh");
      sample(0);
      check(readSample(0,64,64,-1,true)>convergedRaw*.5f,
         "same-slot deformation retains the accepted previous BLAS for temporal visibility");
      check(stableSample(0)>.1f,"deformation rebuilds BLAS visibility");
      check(RenderSceneUpdateVertices(occluder,occluderVertices,3),"restore mesh deformation");
      check(std::abs(stableSample(0)-shadow)<.001f,"restored deformation occludes");
      for(unsigned frame=0;frame<12;++frame) {
         const bool reveal=frame%2==0;
         check(RenderSceneUpdateVertices(occluder,reveal?deformed:occluderVertices,3),"animate caster deformation");
         sample(frame%2);
         const float raw=readSample(frame%2,64,64,-1,true);
         std::printf("Deforming caster frame=%u reveal=%d raw=%.6f\n",frame,int(reveal),raw);
         check(reveal?raw>.0001f:raw<.0001f,
            "recycled ray-scene storage preserves alternating current and previous geometry");
      }
      stableSample(0);
      uint8_t alphaPixel[4]={255,255,255,0};const auto alpha=RenderSceneCreateTexture(1,1,alphaPixel,4);
      auto cutout=material;cutout.opacity=RenderSceneMasked;cutout.baseColorTexture=alpha;
      check(RenderSceneUpdateMaterial(occluderMaterial,&cutout),"alpha cutout caster");
      sample(0);
      check(readSample(0,64,64,-1,true)>convergedRaw*.5f,
         "temporal visibility retains the previous opaque caster material");
      check(stableSample(0)>.1f,"ray queries pass through alpha holes");
      alphaPixel[3]=255;check(RenderSceneUpdateTexture(alpha,alphaPixel,4),"fill cutout");
      check(std::abs(stableSample(1)-shadow)<.001f,"alpha revision changes ray visibility");
      alphaPixel[3]=0;check(RenderSceneUpdateTexture(alpha,alphaPixel,4),"reopen alpha cutout");
      sample(0);
      check(readSample(0,64,64,-1,true)>convergedRaw*.5f,
         "temporal visibility retains the previous alpha texture revision");
      alphaPixel[3]=255;check(RenderSceneUpdateTexture(alpha,alphaPixel,4),"restore filled cutout");
      stableSample(1);
      cutout.emitterSurface=1;check(RenderSceneUpdateMaterial(occluderMaterial,&cutout),"emitter lens");
      instanceData.objectId=42;check(RenderSceneUpdateInstance(caster,&instanceData),"emitter object identity");
      check(RenderSceneSetLightEmitter(lamp,42),"associate lens with light");
      check(stableSample(0)>.1f,"emitter lens transmits its own light");
      auto opaqueLens=cutout;opaqueLens.opacity=RenderSceneOpaque;
      check(RenderSceneUpdateMaterial(occluderMaterial,&opaqueLens),"opaque emitter lens");
      check(stableSample(0)>.1f,"opaque emitter lens remains shader-tested for its own source");
      check(RenderSceneSetLightEmitter(lamp,99),"different light emitter");
      check(std::abs(stableSample(1)-shadow)<.001f,"lens still occludes other emitters");
      check(RenderSceneSetLightEmitter(lamp,0),"restore emitter");
      check(RenderSceneUpdateMaterial(occluderMaterial,&material),"restore opaque material");
      auto facingMaterial=material;facingMaterial.doubleSided=0;
      check(RenderSceneUpdateMaterial(occluderMaterial,&facingMaterial),"single-sided opaque caster");
      check(std::abs(stableSample(0)-shadow)<.001f,"hardware opaque traversal accepts a front face");
      uint32_t reversedIndices[]={0,2,1};
      auto reversedMesh=occluderMesh;reversedMesh.indices=reversedIndices;
      const auto reversed=RenderSceneCreateMesh(&reversedMesh);check(reversed!=0,"reversed caster mesh");
      instanceData.mesh=reversed;check(RenderSceneUpdateInstance(caster,&instanceData),"reverse caster winding");
      check(stableSample(1)>.1f,"hardware traversal culls a single-sided opaque back face");
      facingMaterial.doubleSided=1;check(RenderSceneUpdateMaterial(occluderMaterial,&facingMaterial),"double-sided opaque back face");
      check(std::abs(stableSample(0)-shadow)<.001f,"double-sided opaque instance overrides hardware back-face culling");
      facingMaterial.opacity=RenderSceneMasked;facingMaterial.baseColorTexture=alpha;facingMaterial.doubleSided=0;
      check(RenderSceneUpdateMaterial(occluderMaterial,&facingMaterial),"single-sided masked back face");
      check(stableSample(1)>.1f,"masked back-face culling matches opaque traversal");
      facingMaterial.doubleSided=1;check(RenderSceneUpdateMaterial(occluderMaterial,&facingMaterial),"double-sided masked back face");
      check(std::abs(stableSample(0)-shadow)<.001f,"double-sided cutout still evaluates alpha coverage");
      instanceData.mesh=occluder;check(RenderSceneUpdateInstance(caster,&instanceData),"restore caster winding");
      check(RenderSceneDestroy(reversed),"remove winding fixture mesh");
      check(RenderSceneUpdateMaterial(occluderMaterial,&material),"restore caster material after facing tests");
      for(unsigned type:{uint32_t(RenderScenePointLight),uint32_t(RenderSceneSpotLight),uint32_t(RenderSceneDirectionalLight)}) {
         light.type=type;light.direction[0]=-1;light.direction[2]=1;light.innerCos=.95f;light.outerCos=.6f;
         light.sourceRadius=0;light.angularRadius=0;
         check(RenderSceneUpdateLight(lamp,&light),"point spot sun RT visibility");
         const float typeShadow=stableSample(0);
         std::printf("RTXDI type=%u occluded HDR=%.6f raw=%.6f filtered=%.6f\n",type,typeShadow,readSample(0,64,64,-1,true),readSample(0,64,64,-1,true,true));
         check(std::abs(typeShadow-shadow)<.001f,"all light types occluded");
         instanceData.castsShadow=0;check(RenderSceneUpdateInstance(caster,&instanceData),"disable direct caster");
         check(stableSample(1)>.1f,"all light types illuminate without caster");
         check(readSample(1,64,64,-1,true)>.001f,"RTXDI point spot and sun produce direct diffuse");
         instanceData.castsShadow=1;check(RenderSceneUpdateInstance(caster,&instanceData),"restore direct caster");
      }
      light.type=RenderScenePointLight;check(RenderSceneUpdateLight(lamp,&light),"restore point source");
      std::vector<RenderSceneHandle> extraLights;
      for(unsigned i=0;i<24;++i)extraLights.push_back(RenderSceneCreateLight(&light));
      check(std::abs(stableSample(0)-shadow)<.001f,"RTXDI includes all enabled lights");
      for(const auto id:extraLights)check(RenderSceneDestroy(id),"remove extra light");
      // Two native-linked lights have GPU indices whose order differs from
      // handle order. A much brighter excluded light must not leak through
      // temporal/spatial reuse when the packed GPU light order changes.
      instanceData.castsShadow=0;check(RenderSceneUpdateInstance(caster,&instanceData),"receiver-link fixture disables caster");
      auto faint=light;faint.intensity=.1f;const auto faintId=RenderSceneCreateLight(&faint);
      auto excluded=light;excluded.intensity=1000;const auto excludedId=RenderSceneCreateLight(&excluded);
      const RenderSceneHandle nativeLinks[]={faintId,lamp};
      check(RenderSceneSetInstanceLights(receiverInstance,nativeLinks,2),"set native receiver light links");
      const float linkedRadiance=stableSample(0);
      check(linkedRadiance>.1f && std::abs(linkedRadiance-lit)<lit*.2f,"sorted native light links preserve eligible lights and reject brighter excluded source");
      std::vector<RenderSceneHandle> maskLights;
      for(unsigned i=0;i<70;++i)maskLights.push_back(RenderSceneCreateLight(&excluded));
      std::vector<RenderSceneHandle> gpuOrder;
      for(const auto &entry:dark::GetWorldScene().lights)if(entry.second.enabled && entry.second.intensity>0)gpuOrder.push_back(entry.first);
      check(gpuOrder.size()>64,"large receiver light fixture");
      const RenderSceneHandle boundaryLinks[]={gpuOrder[31],gpuOrder[32],gpuOrder[64]};
      auto boundaryLight=light;boundaryLight.intensity=100.f/3;
      for(const auto id:boundaryLinks)check(RenderSceneUpdateLight(id,&boundaryLight),"enable boundary light");
      check(RenderSceneSetInstanceLights(receiverInstance,boundaryLinks,3),"set links at high GPU indices");
      check(std::abs(stableSample(1)-lit)<lit*.2f,"receiver links preserve indices 31, 32 and 64 and exclude other bright sources");
      check(RenderSceneClearInstanceLights(receiverInstance),"restore unrestricted receiver");
      for(const auto id:maskLights)check(RenderSceneDestroy(id),"remove membership boundary fixture");
      check(RenderSceneUpdateLight(lamp,&light),"restore original light intensity");
      check(RenderSceneDestroy(faintId) && RenderSceneDestroy(excludedId),"remove receiver-link fixture lights");
      instanceData.castsShadow=1;check(RenderSceneUpdateInstance(caster,&instanceData),"restore receiver-link fixture caster");
      auto behind=light;behind.position[0]=0;behind.position[2]=2;
      check(RenderSceneUpdateLight(lamp,&behind),"source behind primary surface");
      check(std::abs(stableSample(0)-shadow)<.001f,"back-facing source contributes no radiance or NaNs");
      check(readSample(0,64,64,-1,true)<.0001f && readSample(0,64,64,-1,true,false,true)<.0001f,"diffuse and specular reject back-facing source");
      light.sourceRadius=1.5f;check(RenderSceneUpdateLight(lamp,&light),"finite area emitter");
      const float soft=stableSample(1);std::printf("RT area shadow ambient=%.6f soft=%.6f lit=%.6f\n",shadow,soft,lit);
      check(soft>shadow+.01f && soft<lit*.95f,"finite emitter produces partial ray-traced shadow coverage");
      light.sourceRadius=0;check(RenderSceneUpdateLight(lamp,&light),"restore sharp source");
      // A straight offscreen half-plane isolates one penumbra edge. Normalize
      // each column by its own unblocked value to remove BRDF/distance falloff.
      instanceData.castsShadow=0;check(RenderSceneUpdateInstance(caster,&instanceData),"disable triangular penumbra caster");
      RenderSceneVertex edgeVertices[3]={{{1,-20,-1},{0,0,1},{0,0}},{{20,-20,-1},{0,0,1},{1,0}},{{1,20,-1},{0,0,1},{0,1}}};
      RenderSceneMesh edgeMesh{edgeVertices,3,indices,3,occluderMaterial,0};
      const auto edgeGeometry=RenderSceneCreateMesh(&edgeMesh);check(edgeGeometry!=0,"penumbra half-plane mesh");
      auto edgeInstance=instanceData;edgeInstance.mesh=edgeGeometry;edgeInstance.visible=0;edgeInstance.castsShadow=0;
      const auto edgeCaster=RenderSceneCreateInstance(&edgeInstance);check(edgeCaster!=0,"offscreen penumbra half-plane");
      light.sourceRadius=.3f;check(RenderSceneUpdateLight(lamp,&light),"penumbra disk source");
      constexpr std::array<int,9> columns{{4,8,16,24,32,40,48,56,60}};
      constexpr unsigned profileSamples=512;
      struct Profile {std::array<double,columns.size()> raw{},filtered{};};
      auto profile=[&]() {
         stableSample(0);Profile result;
         for(unsigned i=0;i<profileSamples;++i) {
            sample(i%2);
            for(unsigned j=0;j<columns.size();++j) {
               result.raw[j]+=readSample(i%2,64,64,columns[j],true)/profileSamples;
               result.filtered[j]+=readSample(i%2,64,64,columns[j],true,true)/profileSamples;
            }
         }
         return result;
      };
      const auto clearProfile=profile();
      edgeInstance.castsShadow=1;check(RenderSceneUpdateInstance(edgeCaster,&edgeInstance),"enable straight shadow edge");
      const auto shadowProfile=profile();
      unsigned partial=0;double previousVisibility=1;
      std::ofstream profileCsv;
      if(const char *path=std::getenv("DARK_LIGHTING_PROFILE_CSV")) {
         profileCsv.open(path);check(bool(profileCsv),"open requested penumbra profile CSV");
         profileCsv<<"pixel,reference_raw,shadow_raw,visibility_raw,visibility_filtered\n";
      }
      for(unsigned j=0;j<columns.size();++j) {
         check(clearProfile.raw[j]>.001 && clearProfile.filtered[j]>.001,"penumbra reference is illuminated");
         const double visibility=shadowProfile.filtered[j]/clearProfile.filtered[j];
         const double rawVisibility=shadowProfile.raw[j]/clearProfile.raw[j];
         std::printf("Penumbra column=%d raw=%.6f filtered=%.6f\n",columns[j],rawVisibility,visibility);
         if(profileCsv.is_open())profileCsv<<columns[j]<<','<<clearProfile.raw[j]<<','<<shadowProfile.raw[j]<<','<<rawVisibility<<','<<visibility<<'\n';
         check(visibility>=0 && visibility<1.08 && visibility<=previousVisibility+.06,"denoised straight-edge penumbra is bounded and monotonic");
         if(visibility>.08 && visibility<.92)++partial;
         previousVisibility=visibility;
      }
      if(profileCsv.is_open()){profileCsv.flush();check(bool(profileCsv),"write requested penumbra profile CSV");}
      check(shadowProfile.filtered.front()/clearProfile.filtered.front()>.8 &&
         shadowProfile.filtered.back()/clearProfile.filtered.back()<.2 && partial>=3,
         "finite source produces multiple intermediate levels from lit surface to shadow");
      check(RenderSceneDestroy(edgeCaster) && RenderSceneDestroy(edgeGeometry),"remove penumbra fixture");
      instanceData.castsShadow=1;check(RenderSceneUpdateInstance(caster,&instanceData),"restore original shadow caster");
      light.sourceRadius=0;check(RenderSceneUpdateLight(lamp,&light),"restore point emitter after penumbra profile");
      std::puts("PASS RT point/spot/sun, offscreen casters, poses/deformation, alpha, emitters, queued slots and unlimited light coverage");
      // Place an emissive panel facing the receiver. It participates in indirect
      // rays but remains outside the native raster draw list for this fixture.
      light.enabled=0;check(RenderSceneUpdateLight(lamp,&light),"GI fixture disables direct source");
      instanceData.castsShadow=0;instanceData.visible=0;check(RenderSceneUpdateInstance(caster,&instanceData),"remove old occluder from GI fixture");
      RenderSceneMaterial emission=material;emission.baseColor[0]=emission.baseColor[1]=emission.baseColor[2]=0;
      emission.emission[0]=2;const auto panelMaterial=RenderSceneCreateMaterial(&emission);
      RenderSceneVertex panelVertices[3]={{{-6,-6,-1},{0,0,1},{0,0}},{{6,-6,-1},{0,0,1},{1,0}},{{0,6,-1},{0,0,1},{.5f,1}}};
      RenderSceneMesh panelMesh{panelVertices,3,indices,3,panelMaterial,0};
      instanceData.mesh=RenderSceneCreateMesh(&panelMesh);instanceData.castsShadow=1;instanceData.visible=0;
      const auto panel=RenderSceneCreateInstance(&instanceData);check(panel!=0,"GI emissive panel");
      const float unlit=stableSample(0);
      auto rootEmission=material;rootEmission.emission[0]=1;
      check(RenderSceneUpdateMaterial(receiverMaterial,&rootEmission),"primary emissive material");
      check(std::abs(sample(0)-(1+unlit))<.01f,"primary emission bypasses demodulation and denoising");
      auto foggedReceiver=movedReceiver;foggedReceiver.receivesFog=1;
      auto fogCamera=camera;fogCamera.fogColor[0]=.2f;fogCamera.fogDistance=6;
      check(RenderSceneUpdateInstance(receiverInstance,&foggedReceiver),"enable receiver fog");
      check(RenderSceneSetCamera(&fogCamera),"enable camera fog");
      check(std::abs(sample(1)-((1+unlit)*.5f+.1f))<.01f,"native fog composes after lighting and emission");
      check(RenderSceneUpdateInstance(receiverInstance,&movedReceiver),"restore receiver fog");
      check(RenderSceneSetCamera(&camera),"restore unfogged camera");
      check(RenderSceneUpdateMaterial(receiverMaterial,&material),"restore primary material");
      // The native-depth target shares primary depth with the HDR pass.
      // Transparent foreground blends over it; geometry behind it must fail Z.
      auto nativeCamera=camera;nativeCamera.nativeDepth[2]=.5f;nativeCamera.nativeDepth[3]=1;
      check(RenderSceneSetCamera(&nativeCamera),"enable native depth export");
      auto overlayMaterial=material;overlayMaterial.opacity=RenderSceneBlended;
      overlayMaterial.baseColor[0]=overlayMaterial.baseColor[1]=overlayMaterial.baseColor[2]=0;
      overlayMaterial.baseColor[3]=.5f;overlayMaterial.emission[0]=.4f;
      const auto overlayMaterialId=RenderSceneCreateMaterial(&overlayMaterial);
      RenderSceneVertex overlayVertices[3]={{{-4,-4,-.5f},{0,0,-1},{0,0}},{{4,-4,-.5f},{0,0,-1},{1,0}},{{0,4,-.5f},{0,0,-1},{.5f,1}}};
      RenderSceneMesh overlayMesh{overlayVertices,3,indices,3,overlayMaterialId,0};
      auto overlayInstance=movedReceiver;overlayInstance.mesh=RenderSceneCreateMesh(&overlayMesh);overlayInstance.castsShadow=0;
      const auto overlayId=RenderSceneCreateInstance(&overlayInstance);
      check(std::abs(sample(0)-(.2f+unlit*.5f))<.005f,"transparent foreground blends over denoised HDR");
      check(std::abs(surfaceSample(0,6)[0]-3)<.001f,"transparent foreground preserves exported opaque view distance");
      overlayInstance.transform[14]=1;check(RenderSceneUpdateInstance(overlayId,&overlayInstance),"move transparent overlay behind receiver");
      check(std::abs(sample(1)-unlit)<.005f,"shared primary depth rejects transparent geometry behind receiver");
      const uint8_t transparentNormalPixel[]={255,128,128,255};
      const auto transparentNormal=RenderSceneCreateDataTexture(1,1,transparentNormalPixel,4);
      auto reflectiveOverlay=material;reflectiveOverlay.opacity=RenderSceneBlended;
      reflectiveOverlay.normalTexture=transparentNormal;reflectiveOverlay.baseColor[3]=0;
      overlayInstance.transform[14]=0;
      check(RenderSceneUpdateInstance(overlayId,&overlayInstance),"restore transparent foreground for backface test");
      check(RenderSceneUpdateMaterial(overlayMaterialId,&reflectiveOverlay),"make foreground temporarily invisible");
      auto transparentLight=light;transparentLight.enabled=1;transparentLight.type=RenderScenePointLight;
      transparentLight.position[0]=2;transparentLight.position[1]=0;transparentLight.position[2]=-.25f;
      check(RenderSceneUpdateLight(lamp,&transparentLight),"place direct source behind transparent foreground");
      const float litBackground=stableSample(0);
      reflectiveOverlay.baseColor[3]=.5f;
      check(RenderSceneUpdateMaterial(overlayMaterialId,&reflectiveOverlay),"show normal-mapped transparent foreground");
      const float transparentBackface=sample(1);
      const float expectedTransparent=litBackground*.5f+material.baseColor[0]*camera.ambient*.5f;
      std::printf("Transparent normal-map backface HDR=%.6f expected=%.6f\n",transparentBackface,expectedTransparent);
      check(std::abs(transparentBackface-expectedTransparent)<.01f,
         "transparent mapped normal cannot reflect direct light through its geometric backface");
      check(RenderSceneUpdateLight(lamp,&light),"disable transparent fixture source");
      check(RenderSceneUpdateMaterial(overlayMaterialId,&overlayMaterial),"restore original overlay material");
      check(RenderSceneDestroy(transparentNormal),"retire transparent normal fixture");
      check(RenderSceneDestroy(overlayId),"remove transparent fixture");
      check(RenderSceneSetCamera(&camera),"restore regular HDR target");
      rays.gi=true;renderer.SetRaySettings(rays);
      float bounced=0,sharcEmission=0;
      for(unsigned i=0;i<40;++i) {
         bounced=sample(i%2);
         sharcEmission+=readSample(i%2,64,64,-1,true)/40;
      }
      std::printf("GI emission bounce baseline=%.6f lit=%.6f\n",unlit,bounced);
      check(bounced>unlit+.2f,"displayed HDR uses SHaRC indirect emission bounce");
      const auto sharcStats=renderer.IndirectStatistics(1);
      std::printf("SHaRC emission mean=%.6f updates=%u queries=%u hits=%u fallback=%u\n",sharcEmission,sharcStats[0],sharcStats[1],sharcStats[2],sharcStats[3]);
      check(sharcEmission>.01f,"SHaRC query produces indirect emission radiance");
      const float sharcSpecular=readSample(1,64,64,-1,true,true,true);
      std::printf("SHaRC RELAX rough specular=%.6f\n",sharcSpecular);
      check(sharcSpecular>.001f,"rough-specular indirect radiance survives RELAX");
      check(sharcStats[0]>0 && sharcStats[1]>0 && sharcStats[2]>0,"actual SHaRC updates and successful secondary cache queries execute");
      float cacheMean=0,referenceMean=0;
      for(unsigned i=0;i<24;++i){sample(i%2);cacheMean+=readSample(i%2,64,64,-1,true)/24;}
      rays.sharc=false;renderer.SetRaySettings(rays);
      for(unsigned i=0;i<24;++i){sample(i%2);referenceMean+=readSample(i%2,64,64,-1,true)/24;}
      check(renderer.IndirectStatistics(1)[2]==0,"reference path does not query SHaRC cache");
      std::printf("SHaRC cached mean=%.6f reference=%.6f\n",cacheMean,referenceMean);
      check(referenceMean>.1f && std::abs(cacheMean-referenceMean)<referenceMean*.4f,"SHaRC preserves reference indirect emission energy");
      const uint8_t redEmissionPixel[]={255,0,0,255};
      const auto giEmissionMap=RenderSceneCreateTexture(1,1,redEmissionPixel,4);
      auto pbrPanel=emission;pbrPanel.emission[0]=1;pbrPanel.emissionTexture=giEmissionMap;
      check(RenderSceneUpdateMaterial(panelMaterial,&pbrPanel),"switch GI from scalar emission to equivalent scalar plus PBR emission");
      for(unsigned i=0;i<8;++i)sample(i%2);
      float pbrReferenceMean=0;
      for(unsigned i=0;i<24;++i){sample(i%2);pbrReferenceMean+=readSample(i%2,64,64,-1,true)/24;}
      std::printf("GI material variants native=%.6f PBR=%.6f\n",referenceMean,pbrReferenceMean);
      check(std::abs(pbrReferenceMean-referenceMean)<referenceMean*.25f,
         "PBR GI pipeline samples secondary emission maps with matching energy");
      check(RenderSceneUpdateMaterial(panelMaterial,&emission),"return GI to native material");sample(0);
      check(RenderSceneDestroy(giEmissionMap),"retire optional GI emission map");
      const uint8_t grazingNormalPixel[]={255,128,128,255};
      const auto grazingMap=RenderSceneCreateDataTexture(1,1,grazingNormalPixel,4);
      auto grazingReceiver=material;grazingReceiver.normalTexture=grazingMap;
      const auto frontPanel=dark::GetWorldScene().instances.at(panel);
      auto behindPanel=frontPanel;behindPanel.transform[14]+=2;
      check(RenderSceneUpdateInstance(panel,&behindPanel),"place indirect emitter behind opaque receiver");
      check(RenderSceneUpdateMaterial(receiverMaterial,&grazingReceiver),"tilt receiver shading normal toward tangent");
      auto behindLight=light;behindLight.enabled=1;behindLight.type=RenderScenePointLight;
      // Keep this source between the receiver and the emissive panel, so the
      // panel cannot accidentally hide a receiver-backface visibility bug.
      behindLight.position[0]=2;behindLight.position[1]=0;behindLight.position[2]=.25f;
      rays.gi=false;renderer.SetRaySettings(rays);
      check(RenderSceneUpdateLight(lamp,&behindLight),"place direct source behind mapped receiver");
      sample(0);const float behindDirect=readSample(0,64,64,-1,true);
      std::printf("Normal-map backface direct leakage=%.6f\n",behindDirect);
      check(behindDirect<.0001f,"mapped shading normal cannot admit direct light through receiver backface");
      check(RenderSceneUpdateLight(lamp,&light),"disable behind-surface direct source");
      rays.gi=true;renderer.SetRaySettings(rays);
      for(unsigned i=0;i<16;++i)sample(i%2);
      float behindReceiverMean=0,behindSpecularMean=0;
      for(unsigned i=0;i<32;++i) {
         sample(i%2);behindReceiverMean+=readSample(i%2,64,64,-1,true)/32;
         behindSpecularMean+=readSample(i%2,64,64,-1,true,false,true)/32;
      }
      std::printf("GI normal-map backface leakage diffuse=%.6f specular=%.6f\n",behindReceiverMean,behindSpecularMean);
      check(behindReceiverMean<.0001f && behindSpecularMean<.0001f,
         "mapped shading normal cannot send diffuse/specular GI through receiver backface");
      check(RenderSceneUpdateInstance(panel,&frontPanel),"restore indirect emitter in front of receiver");
      check(RenderSceneUpdateMaterial(receiverMaterial,&material),"restore receiver geometric shading");sample(0);
      check(RenderSceneDestroy(grazingMap),"retire grazing normal fixture");
      rays.sharc=true;renderer.SetRaySettings(rays);
      auto moveCamera=[&](float x) {
         auto movedCamera=camera;const float movedEye[]={x,0,-3},look[]={-x,0,3};
         const float rightLook[]={look[0]+.1f*look[2],0,look[2]-.1f*look[0]};
         const float downLook[]={look[0],.1f*std::sqrt(x*x+9),look[2]};
         check(RenderSceneCameraFromRays(&movedCamera,movedEye,look,rightLook,downLook,.1f,200),"moving GI camera");
         check(RenderSceneSetCamera(&movedCamera),"set moving GI camera");
      };
      stableSample(0);
      const float initialMotionSpecular=readSample(0,64,64,-1,true,true,true);
      float minMotionSpecular=initialMotionSpecular,maxMotionSpecular=initialMotionSpecular;
      for(unsigned step=1;step<=16;++step) {
         moveCamera(float(step)/8);sample(step%2);
         const float value=readSample(step%2,64,64,-1,true,true,true);
         minMotionSpecular=std::min(minMotionSpecular,value);maxMotionSpecular=std::max(maxMotionSpecular,value);
      }
      const float movingSpecular=readSample(0,64,64,-1,true,true,true);
      const float rolled=stableSample(1);
      const float settledMotionSpecular=readSample(1,64,64,-1,true,true,true);
      std::printf("GI specular camera motion initial=%.6f range=%.6f/%.6f moving=%.6f settled=%.6f\n",initialMotionSpecular,minMotionSpecular,maxMotionSpecular,movingSpecular,settledMotionSpecular);
      check(initialMotionSpecular>.001f && minMotionSpecular>initialMotionSpecular*.1f && maxMotionSpecular<initialMotionSpecular*4 &&
         movingSpecular>settledMotionSpecular*.5f && movingSpecular<settledMotionSpecular*1.5f,
         "rough-specular GI remains finite and tracks moving-camera energy without a convergence wait");
      check(rolled>unlit+.02f && std::abs(rolled-bounced)<bounced*.5f,"camera motion preserves indirect illumination");
      moveCamera(16);const float outside=stableSample(0);std::printf("GI camera outside=%.6f baseline=%.6f rolled=%.6f\n",outside,unlit,rolled);check(outside>unlit+.02f,"camera outside former bounded grid retains world-space indirect lighting");
      check(RenderSceneSetCamera(&camera),"return GI camera");
      for(unsigned i=0;i<32;++i)bounced=sample(i%2);
      // A wide black separator blocks the emissive panel from the receiver.
      RenderSceneMaterial black=material;black.baseColor[0]=black.baseColor[1]=black.baseColor[2]=0;
      const auto blackMaterial=RenderSceneCreateMaterial(&black);
      RenderSceneVertex wallVertices[3]={{{-20,-20,-.5f},{0,0,1},{0,0}},{{20,-20,-.5f},{0,0,1},{1,0}},{{0,20,-.5f},{0,0,1},{.5f,1}}};
      RenderSceneMesh wallMesh{wallVertices,3,indices,3,blackMaterial,0};
      instanceData.mesh=RenderSceneCreateMesh(&wallMesh);const auto separator=RenderSceneCreateInstance(&instanceData);
      float blocked=0;for(unsigned i=0;i<80;++i)blocked=sample(i%2);
      std::printf("GI wall separation unobstructed=%.6f blocked=%.6f\n",bounced,blocked);
      check(blocked<unlit+(bounced-unlit)*.25f,"traced indirect paths stop at a separating wall");
      check(readSample(1,64,64,-1,true)<.001f,"SHaRC continuation respects separating wall");
      check(RenderSceneDestroy(separator),"remove GI separator");
      emission.emission[0]=0;check(RenderSceneUpdateMaterial(panelMaterial,&emission),"turn off GI emission");
      float decayed=0;for(unsigned i=0;i<100;++i)decayed=sample(i%2);
      check(decayed<bounced*.25f,"indirect history follows dynamic emission");
      check(readSample(1,64,64,-1,true)<.001f,"SHaRC material edit clears stale emissive radiance");
      rays.gi=false;renderer.SetRaySettings(rays);check(std::abs(sample(0)-unlit)<.001f,"GI switch restores direct-only result");
      // Isolate diffuse light bounce: the receiver has no directly linked
      // emitters, while the panel can reflect the point source into indirect paths.
      for(unsigned axis=0;axis<3;++axis)emission.baseColor[axis]=.5f;
      check(RenderSceneUpdateMaterial(panelMaterial,&emission),"diffuse bounce panel");
      check(RenderSceneSetInstanceLights(receiverInstance,nullptr,0),"receiver excludes direct source");
      light.enabled=1;light.position[0]=0;light.position[2]=-.25f;light.intensity=80;
      check(RenderSceneUpdateLight(lamp,&light),"indirect point light");
      rays.gi=true;renderer.SetRaySettings(rays);
      float diffuseBounce=0,sharcDiffuseMean=0;
      for(unsigned i=0;i<40;++i) {
         diffuseBounce=sample(i%2);
         sharcDiffuseMean+=readSample(i%2,64,64,-1,true)/40;
      }
      std::printf("GI diffuse point bounce baseline=%.6f lit=%.6f\n",unlit,diffuseBounce);
      check(diffuseBounce>unlit+.003f,"point light reaches directly unlit receiver through diffuse GI");
      std::printf("SHaRC reflected point light mean=%.6f\n",sharcDiffuseMean);
      check(sharcDiffuseMean>.001f,"SHaRC reflects direct light onto a receiver with no direct links");
      stableSample(0);
      float brightCached=0,brightSpecular=0;
      for(unsigned i=0;i<32;++i) {
         brightCached+=sample(i%2)/32;
         brightSpecular+=readSample(i%2,64,64,-1,true,true,true)/32;
      }
      light.intensity=10;check(RenderSceneUpdateLight(lamp,&light),"dim enabled indirect source without changing its identity");
      float dimCached=0,earlyDim=0,earlyRaw=0,lateRaw=0;
      float earlySpecular=0,dimSpecular=0;
      for(unsigned i=0;i<64;++i) {
         const float value=sample(i%2);
         const float raw=readSample(i%2,64,64,-1,true);
         const float specular=readSample(i%2,64,64,-1,true,true,true);
         if(i>=8 && i<16)earlyDim+=value/8;
         if(i>=48)dimCached+=value/16;
         if(i>=8 && i<16)earlyRaw+=raw/8;
         if(i>=48)lateRaw+=raw/16;
         if(i>=8 && i<16)earlySpecular+=specular/8;
         if(i>=48)dimSpecular+=specular/16;
         if(i==0 || i==3 || i==7 || i==15 || i==31 || i==63)
            std::printf("GI intensity frame=%u HDR=%.6f specular=%.6f\n",i+1,value,specular);
      }
      rays.sharc=false;renderer.SetRaySettings(rays);stableSample(0);
      float dimReference=0,specularReference=0;
      for(unsigned i=0;i<32;++i) {
         dimReference+=sample(i%2)/32;
         specularReference+=readSample(i%2,64,64,-1,true,true,true)/32;
      }
      std::printf("GI enabled-source intensity bright=%.6f early=%.6f dim=%.6f reference=%.6f raw=%.6f/%.6f\n",brightCached,earlyDim,dimCached,dimReference,earlyRaw,lateRaw);
      check(earlyDim<unlit+(brightCached-unlit)*.5f,"indirect lighting responds to an enabled-source intensity change within sixteen frames");
      check(dimReference>unlit && std::abs(dimCached-dimReference)<(dimReference-unlit)*.4f,
         "settled cached intensity change agrees with the uncached indirect reference");
      std::printf("GI specular intensity bright=%.6f early=%.6f dim=%.6f reference=%.6f\n",brightSpecular,earlySpecular,dimSpecular,specularReference);
      check(brightSpecular>.001f && earlySpecular<brightSpecular*.5f &&
         specularReference>.001f && std::abs(dimSpecular-specularReference)<specularReference*.4f,
         "denoised rough-specular GI responds to intensity changes and preserves reference energy");
      light.intensity=80;check(RenderSceneUpdateLight(lamp,&light),"restore indirect source intensity");
      rays.sharc=true;renderer.SetRaySettings(rays);stableSample(0);
      light.position[2]=-3;check(RenderSceneUpdateLight(lamp,&light),"move enabled source behind the bounce panel");
      float movedEarly=0,movedLate=0;
      for(unsigned i=0;i<80;++i) {
         const float value=sample(i%2);
         if(i>=8 && i<16)movedEarly+=value/8;
         if(i>=64)movedLate+=value/16;
      }
      std::printf("GI source movement early=%.6f late=%.6f baseline=%.6f\n",movedEarly,movedLate,unlit);
      check(movedEarly<unlit+(brightCached-unlit)*.5f && movedLate<unlit+.003f,
         "enabled source movement removes stale indirect lighting without deleting the source");
      light.position[2]=-.25f;check(RenderSceneUpdateLight(lamp,&light),"return indirect source to the panel front");
      stableSample(0);
      // Enclose an offset source in an offscreen black box. Primary bounce
      // rays still see the panel, but its direct illumination becomes blocked.
      // The source itself is unchanged while the occluder enters/leaves.
      light.position[0]=2;check(RenderSceneUpdateLight(lamp,&light),"offset source for moving-occluder GI test");
      stableSample(0);float geometryBright=0;
      for(unsigned i=0;i<32;++i)geometryBright+=sample(i%2)/32;
      constexpr unsigned faces[6][4]={{1,5,7,3},{0,2,6,4},{2,3,7,6},{0,4,5,1},{4,6,7,5},{0,1,3,2}};
      constexpr float faceNormals[6][3]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
      std::array<RenderSceneVertex,24> boxVertices{};std::array<uint32_t,36> boxIndices{};
      for(unsigned f=0;f<6;++f) {
         for(unsigned v=0;v<4;++v) {
            auto &vertex=boxVertices[f*4+v];
            for(unsigned axis=0;axis<3;++axis) {
               vertex.position[axis]=light.position[axis]+((faces[f][v]&(1u<<axis))?.15f:-.15f);
               vertex.normal[axis]=faceNormals[f][axis];
            }
         }
         const uint32_t quad[]={f*4,f*4+1,f*4+2,f*4,f*4+2,f*4+3};
         std::copy(quad,quad+6,boxIndices.begin()+f*6);
      }
      RenderSceneMesh boxMesh{boxVertices.data(),uint32_t(boxVertices.size()),boxIndices.data(),uint32_t(boxIndices.size()),blackMaterial,0};
      const auto boxGeometry=RenderSceneCreateMesh(&boxMesh);check(boxGeometry!=0,"GI occlusion box mesh");
      auto boxInstance=movedReceiver;boxInstance.mesh=boxGeometry;boxInstance.visible=0;boxInstance.castsShadow=1;
      const auto box=RenderSceneCreateInstance(&boxInstance);check(box!=0,"offscreen GI occlusion box");
      float geometryEarly=0,geometryDark=0;
      for(unsigned i=0;i<80;++i) {
         const float value=sample(i%2);
         if(i>=8 && i<16)geometryEarly+=value/8;
         if(i>=64)geometryDark+=value/16;
      }
      std::printf("GI moving occluder bright=%.6f early=%.6f blocked=%.6f\n",geometryBright,geometryEarly,geometryDark);
      check(geometryBright>unlit+.01f && geometryEarly<unlit+(geometryBright-unlit)*.5f && geometryDark<unlit+.003f,
         "geometry changes remove cached indirect illumination while the source stays fixed");
      uint8_t boxCoverage[]={255,255,255,255};
      const auto coverageTexture=RenderSceneCreateTexture(1,1,boxCoverage,4);check(coverageTexture!=0,"GI alpha occluder texture");
      auto maskedBlack=black;maskedBlack.opacity=RenderSceneMasked;maskedBlack.baseColorTexture=coverageTexture;
      check(RenderSceneUpdateMaterial(blackMaterial,&maskedBlack),"mask GI occlusion box");
      stableSample(0);
      boxCoverage[3]=0;check(RenderSceneUpdateTexture(coverageTexture,boxCoverage,4),"animate only the occluder's alpha coverage");
      float coverageEarly=0,coverageLate=0;
      for(unsigned i=0;i<64;++i) {
         const float value=sample(i%2);
         if(i>=8 && i<16)coverageEarly+=value/8;
         if(i>=48)coverageLate+=value/16;
      }
      std::printf("GI alpha-only visibility early=%.6f late=%.6f reference=%.6f\n",coverageEarly,coverageLate,geometryBright);
      check(coverageEarly>unlit+(geometryBright-unlit)*.5f && std::abs(coverageLate-geometryBright)<(geometryBright-unlit)*.4f,
         "texture-only alpha animation updates indirect visibility without a light or transform change");
      check(RenderSceneUpdateMaterial(blackMaterial,&black),"restore opaque black material");
      check(RenderSceneDestroy(coverageTexture),"remove GI alpha fixture texture");
      check(RenderSceneDestroy(box) && RenderSceneDestroy(boxGeometry),"remove GI occlusion box");
      light.position[0]=0;check(RenderSceneUpdateLight(lamp,&light),"restore indirect source position");
      stableSample(0);
      auto scaledLighting=camera;scaledLighting.lightScale=.125f;
      check(RenderSceneSetCamera(&scaledLighting),"change only global light scale");
      float scaleEarly=0,scaleLate=0;
      for(unsigned i=0;i<64;++i) {
         const float value=sample(i%2);
         if(i>=8 && i<16)scaleEarly+=value/8;
         if(i>=48)scaleLate+=value/16;
      }
      std::printf("GI global light scale early=%.6f late=%.6f reference=%.6f\n",scaleEarly,scaleLate,dimReference);
      check(scaleEarly<unlit+(brightCached-unlit)*.5f && std::abs(scaleLate-dimReference)<(dimReference-unlit)*.4f,
         "global light-scale edits refresh indirect lighting without modifying individual lights");
      check(RenderSceneSetCamera(&camera),"restore global light scale");stableSample(0);
      light.enabled=0;check(RenderSceneUpdateLight(lamp,&light),"turn off indirect point light");
      float darkSharc=0;
      for(unsigned i=0;i<100;++i) {
         decayed=sample(i%2);
         if(i>=84)darkSharc=std::max(darkSharc,readSample(i%2,64,64,-1,true));
      }
      check(decayed<unlit+(diffuseBounce-unlit)*.25f,"indirect cache responds to dynamic direct light changes");
      std::printf("SHaRC disabled light maximum residual=%.6f\n",darkSharc);
      check(darkSharc<.001f,"SHaRC indirect light decays when its direct source is disabled");
      // A world reset with the same owner pointer must discard indirect history.
      rays.gi=true;renderer.SetRaySettings(rays);
      auto other=*renderScene;other.resetRevision++;other.materials.at(panelMaterial).emission[0]=0;
      renderScene=&other;check(sample(1)<unlit+.002f,"scene reset discards stale irradiance");
      device->waitIdle();device->unmapMemory(*allocation);renderer.Shutdown();RenderSceneClear();
      std::puts("PASS displayed RTXDI + SHaRC + RELAX, emission, fog, indirect visibility, dynamic lighting and reset");
      return 0;
   }
   std::puts("SKIP no ray-query capable graphics device");return 77;
 }catch(const std::exception &failure){std::fprintf(stderr,"RTX SDK lighting regression failed: %s\n",failure.what());return 1;}
}
