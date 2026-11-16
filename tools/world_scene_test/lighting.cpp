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
   vk::ApplicationInfo app("Dark world shadow maps",1,"Dark",1,VK_API_VERSION_1_1);
   auto instance=context.createInstance(vk::InstanceCreateInfo({},&app));
   for(const auto physical:(*instance).enumeratePhysicalDevices()) {
      dark::VulkanWorldFeatures features;if(!features.Probe(physical))continue;
      if(std::getenv("DARK_TEST_SCALAR_SHADOWS"))features.features.features.multiDrawIndirect=features.features.features.drawIndirectFirstInstance=VK_FALSE;
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
      check(renderer.Initialize(physical,*device,family,*pass,vk::Format::eD32Sfloat,vk::Format::eR8G8B8A8Unorm,argv[1],error,features.IndirectShadows()),error.c_str());
      auto output=device->createBufferUnique(vk::BufferCreateInfo({},8,vk::BufferUsageFlagBits::eTransferDst,vk::SharingMode::eExclusive));
      const auto requirements=device->getBufferMemoryRequirements(*output);const auto memory=physical.getMemoryProperties();uint32_t type=0;
      const auto wanted=vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent;
      while(type<memory.memoryTypeCount && (!(requirements.memoryTypeBits&(1u<<type)) || (memory.memoryTypes[type].propertyFlags&wanted)!=wanted))++type;
      check(type<memory.memoryTypeCount,"host readback memory");
      auto allocation=device->allocateMemoryUnique(vk::MemoryAllocateInfo(requirements.size,type));device->bindBufferMemory(*output,*allocation,0);
      const auto *mapped=static_cast<const uint16_t *>(device->mapMemory(*allocation,0,8));
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
      auto readSample=[&](uint32_t slot,uint32_t width=64,uint32_t height=64,int sampleX=-1){
         auto cmd=*commands[0];cmd.reset();cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
         const vk::Image hdr=renderer.HdrImage(slot);
         vk::ImageMemoryBarrier barrier(vk::AccessFlagBits::eColorAttachmentWrite,vk::AccessFlagBits::eTransferRead,
            vk::ImageLayout::eShaderReadOnlyOptimal,vk::ImageLayout::eTransferSrcOptimal,VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,hdr,
            {vk::ImageAspectFlagBits::eColor,0,1,0,1});
         cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,vk::PipelineStageFlagBits::eTransfer,{}, {}, {},barrier);
         vk::BufferImageCopy copy(0,0,0,{vk::ImageAspectFlagBits::eColor,0,0,1},{sampleX<0?int32_t(width/2):sampleX,int32_t(height/2),0},{1,1,1});
         cmd.copyImageToBuffer(hdr,vk::ImageLayout::eTransferSrcOptimal,*output,copy);
         barrier.srcAccessMask=vk::AccessFlagBits::eTransferRead;barrier.dstAccessMask=vk::AccessFlagBits::eShaderRead;
         barrier.oldLayout=vk::ImageLayout::eTransferSrcOptimal;barrier.newLayout=vk::ImageLayout::eShaderReadOnlyOptimal;
         cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eFragmentShader,{}, {}, {},barrier);
         vk::MemoryBarrier read(vk::AccessFlagBits::eTransferWrite,vk::AccessFlagBits::eHostRead);
         cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},read,{},{});cmd.end();submit(cmd);
         const unsigned exponent=(mapped[0]>>10)&31,mantissa=mapped[0]&1023;
         const float result=exponent?std::ldexp(1+mantissa/1024.0f,int(exponent)-15):std::ldexp(float(mantissa),-24);
         check(!(mapped[0]&0x8000) && exponent!=31,"finite nonnegative radiance");return result;
      };
      const dark::WorldScene *renderScene=&dark::GetWorldScene();
      auto sample=[&](uint32_t slot,uint32_t width=64,uint32_t height=64,int sampleX=-1){
         const auto recorded=renderer.Record(slot,*renderScene,width,height,error);check(recorded!=VK_NULL_HANDLE,error.c_str());submit(recorded);
         return readSample(slot,width,height,sampleX);
      };
      const float shadow=sample(0);check(shadow>0.004f && shadow<0.006f,"opaque shadow leaves ambient");
      const auto initialWork=renderer.GetShadowWorkload(0);
      if(features.IndirectShadows())check(initialWork.drawBatches<initialWork.drawCalls,"compatible casters use multi-command batches");
      else check(initialWork.drawBatches==initialWork.drawCalls,"feature-disabled device retains scalar submission");
      check(RenderSceneUpdateVertices(occluder,occluderVertices,3),"unchanged shadow pose update");
      check(std::abs(sample(0)-shadow)<.001f,"unchanged pose preserves shadow radiance");
      const auto unchangedPoseWork=renderer.GetShadowWorkload(0);
      check(unchangedPoseWork.renderedViews==0 && unchangedPoseWork.reusedViews==6,"unchanged vertex data retains shadow maps");
      instanceData.objectId=42;check(RenderSceneUpdateInstance(caster,&instanceData),"label emitter geometry");
      auto emitterMaterial=dark::GetWorldScene().materials.at(occluderMaterial);emitterMaterial.emitterSurface=1;
      check(RenderSceneUpdateMaterial(occluderMaterial,&emitterMaterial),"tag emitter surface");
      check(RenderSceneSetLightEmitter(lamp,42),"associate emitter geometry");
      check(sample(1)>.1f,"emitter surface does not extinguish its own light");
      const auto emitterLight=dark::GetWorldScene().lights.at(lamp);
      instanceData.visible=0;check(RenderSceneUpdateInstance(caster,&instanceData),"hide lens from camera while retaining shadow geometry");
      for(unsigned type:{uint32_t(RenderScenePointLight),uint32_t(RenderSceneSpotLight)}) {
         auto closeLight=emitterLight;closeLight.type=type;closeLight.range=5;
         closeLight.direction[0]=-1;closeLight.direction[2]=1;closeLight.innerCos=.9f;closeLight.outerCos=.6f;
         check(RenderSceneUpdateLight(lamp,&closeLight),"bounded close-camera emitter");
         for(float separation:{1.0f,.1f,.02f}) {
            const float nearEye[]={2,0,-2-separation},look[]={-2,0,2+separation};
            const float length=std::sqrt(look[0]*look[0]+look[2]*look[2]);
            const float rightRay[]={look[0]+.1f*look[2],0,look[2]-.1f*look[0]};
            const float downRay[]={look[0],.1f*length,look[2]};
            auto closeCamera=camera;check(RenderSceneCameraFromRays(&closeCamera,nearEye,look,rightRay,downRay,.1f,100),"camera near emitter");
            check(RenderSceneSetCamera(&closeCamera),"set close-light camera");
            check(sample(type%2)>.1f,"approaching a shadowed emitter preserves surface illumination");
         }
      }
      check(RenderSceneUpdateLight(lamp,&emitterLight),"restore emitter after close-camera tests");
      check(RenderSceneSetCamera(&camera),"restore camera after close-light tests");
      instanceData.visible=1;check(RenderSceneUpdateInstance(caster,&instanceData),"restore lens camera visibility");
      check(RenderSceneSetLightEmitter(lamp,99),"switch emitter association");
      check(std::abs(sample(0)-shadow)<.001f,"emitter surface still occludes another light");
      check(RenderSceneSetLightEmitter(lamp,42),"restore emitter association");
      emitterMaterial.emitterSurface=0;check(RenderSceneUpdateMaterial(occluderMaterial,&emitterMaterial),"restore metal housing");
      check(std::abs(sample(1)-shadow)<.001f,"non-emitting housing still casts its own light shadow");
      check(RenderSceneSetLightEmitter(lamp,0),"clear emitter association");
      instanceData.objectId=0;check(RenderSceneUpdateInstance(caster,&instanceData),"restore caster label");
      instanceData.castsShadow=0;check(RenderSceneUpdateInstance(caster,&instanceData),"disable caster");
      const float lit=sample(1);check(lit>0.1f && lit>shadow*10,"unoccluded direct light");
      check(RenderSceneSetInstanceLights(receiverInstance,nullptr,0),"empty receiver light list");
      check(std::abs(sample(0)-shadow)<.001f,"empty receiver list retains only ambient");
      check(RenderSceneSetInstanceLights(receiverInstance,&lamp,1),"link direct emitter");
      check(std::abs(sample(1)-lit)<lit*.01f,"receiver list restores unbounded direct emitter");
      light.enabled=0;check(RenderSceneUpdateLight(lamp,&light),"disable linked emitter");
      check(std::abs(sample(0)-shadow)<.001f,"enabled-light remap invalidates receiver cache");
      light.enabled=1;check(RenderSceneUpdateLight(lamp,&light),"restore linked emitter");
      check(std::abs(sample(1)-lit)<lit*.01f,"restored enabled emitter remaps receiver cache");
      auto partialLight=light;partialLight.position[0]=.2f;partialLight.position[1]=.2f;partialLight.position[2]=-.4f;
      partialLight.range=.05f;partialLight.castsShadow=0;
      const auto partialLamp=RenderSceneCreateLight(&partialLight);check(partialLamp!=0,"partial-tile emitter");
      sample(0);
      check(std::abs(sample(1,96,96)-lit)<lit*.02f,"cached receiver range survives changed tile-index prefix");
      check(RenderSceneDestroy(partialLamp),"remove partial-tile emitter");
      const auto queuedLinked=renderer.Record(1,dark::GetWorldScene(),64,64,error);check(queuedLinked!=VK_NULL_HANDLE,error.c_str());
      check(RenderSceneSetInstanceLights(receiverInstance,nullptr,0),"change receiver while another slot is queued");
      check(std::abs(sample(0)-shadow)<.001f,"new slot uses new receiver list");
      submit(queuedLinked);check(std::abs(readSample(1)-lit)<lit*.01f,"queued slot retains its prior receiver list");
      check(RenderSceneSetInstanceLights(receiverInstance,&lamp,1),"restore linked emitter after queued-list test");
      auto excludedLight=light;excludedLight.intensity*=10;
      const auto excluded=RenderSceneCreateLight(&excludedLight);check(excluded!=0,"excluded bright emitter");
      check(std::abs(sample(0)-lit)<lit*.01f,"unlisted light cannot reach receiver");
      check(RenderSceneSetInstanceLights(receiverInstance,&excluded,1),"switch receiver emitter");
      check(sample(1)>lit*5,"receiver list switches to another GPU light index");
      check(RenderSceneDestroy(excluded),"destroy linked emitter");
      check(std::abs(sample(0)-shadow)<.001f,"destroyed linked light leaves an empty receiver list");
      check(RenderSceneClearInstanceLights(receiverInstance),"restore unrestricted receiver");
      check(std::abs(sample(1)-lit)<lit*.01f,"unrestricted receiver uses tiled lighting again");
      // Fog is opt-in per instance and uses camera-forward depth, matching
      // the native linear distance ramp rather than radial eye distance.
      auto fogPlacement=dark::GetWorldScene().instances.at(receiverInstance);
      camera.fogColor[0]=.2f;camera.fogColor[1]=.3f;camera.fogColor[2]=.4f;camera.fogDistance=6;
      check(RenderSceneSetCamera(&camera),"fog camera");
      check(std::abs(sample(0)-lit)<lit*.01f,"unflagged instance stays clear");
      fogPlacement.receivesFog=1;check(RenderSceneUpdateInstance(receiverInstance,&fogPlacement),"fog-enabled receiver");
      check(std::abs(sample(1)-(lit+.2f)*.5f)<.002f,"half-distance linear fog");
      instanceData.visible=0;check(RenderSceneUpdateInstance(caster,&instanceData),"hide foreground caster in wide fog view");
      auto wideFogCamera=camera;const float fogRight[]={1,0,1},fogBottom[]={0,1,1};
      check(RenderSceneCameraFromRays(&wideFogCamera,eye,forward,fogRight,fogBottom,.1f,100),"wide fog camera");
      check(RenderSceneSetCamera(&wideFogCamera),"off-axis fog camera");
      fogPlacement.receivesFog=0;check(RenderSceneUpdateInstance(receiverInstance,&fogPlacement),"clear off-axis reference");
      const float offAxisClear=sample(0,64,64,48);
      fogPlacement.receivesFog=1;check(RenderSceneUpdateInstance(receiverInstance,&fogPlacement),"fogged off-axis receiver");
      check(offAxisClear>.1f && std::abs(sample(1,64,64,48)-(offAxisClear+.2f)*.5f)<.002f,"off-axis fog uses forward depth instead of eye distance");
      instanceData.visible=1;check(RenderSceneUpdateInstance(caster,&instanceData),"restore foreground caster visibility");
      camera.fogDistance=3;check(RenderSceneSetCamera(&camera),"full fog distance");
      check(std::abs(sample(0)-.2f)<.001f,"full-distance fog reaches linear color");
      camera.fogDistance=0;check(RenderSceneSetCamera(&camera),"disable fog distance");
      check(std::abs(sample(1)-lit)<lit*.01f,"zero fog distance restores direct lighting");
      fogPlacement.receivesFog=0;check(RenderSceneUpdateInstance(receiverInstance,&fogPlacement),"restore clear receiver");
      // A second mesh has different vertex data and nonzero arena offsets.
      // Moving its instance back must reproduce the first mesh's lit pixel;
      // binding the arena at zero without baseVertex/firstIndex would miss it.
      auto offsetVertices=std::array<RenderSceneVertex,4>{receiverVertices[0],receiverVertices[0],receiverVertices[1],receiverVertices[2]};
      for(auto &vertex:offsetVertices)vertex.position[0]+=10;
      const uint32_t offsetIndices[]={1,2,3};
      RenderSceneMesh offsetMesh{offsetVertices.data(),4,offsetIndices,3,receiverMaterial,0};
      const auto offsetMeshId=RenderSceneCreateMesh(&offsetMesh);check(offsetMeshId!=0,"offset raster mesh");
      auto originalReceiver=dark::GetWorldScene().instances.at(receiverInstance);
      originalReceiver.visible=0;check(RenderSceneUpdateInstance(receiverInstance,&originalReceiver),"hide original receiver");
      auto offsetPlacement=originalReceiver;offsetPlacement.mesh=offsetMeshId;offsetPlacement.visible=1;offsetPlacement.castsShadow=0;
      offsetPlacement.transform[12]=-10;
      const auto offsetInstance=RenderSceneCreateInstance(&offsetPlacement);check(offsetInstance!=0,"offset raster instance");
      check(std::abs(sample(0)-lit)<lit*.01f && std::abs(sample(1)-lit)<lit*.01f,"nonzero vertex/index arena offsets draw in both slots");
      check(RenderSceneDestroy(offsetInstance) && RenderSceneDestroy(offsetMeshId),"retire offset raster fixture");
      originalReceiver.visible=1;check(RenderSceneUpdateInstance(receiverInstance,&originalReceiver),"restore original receiver");
      // Mirroring reverses winding. A one-sided surface must write neither
      // depth nor radiance on its back face, while two-sided restores both.
      auto sided=material;sided.doubleSided=0;
      check(RenderSceneUpdateMaterial(receiverMaterial,&sided),"single-sided receiver");
      const float firstSide=sample(0);
      auto receiverPlacement=dark::GetWorldScene().instances.at(receiverInstance);
      receiverPlacement.transform[0]=-1;
      check(RenderSceneUpdateInstance(receiverInstance,&receiverPlacement),"mirror receiver winding");
      const float reverseSide=sample(1);
      check(std::min(firstSide,reverseSide)==0 && std::max(firstSide,reverseSide)>.1f,"only front winding contributes radiance");
      check(RenderSceneUpdateMaterial(receiverMaterial,&material),"two-sided receiver");
      check(sample(0)>.1f,"two-sided mirrored receiver visible");
      receiverPlacement.transform[0]=1;
      check(RenderSceneUpdateInstance(receiverInstance,&receiverPlacement),"restore receiver winding");
      auto localLight=light;localLight.range=.05f;localLight.position[0]=.03f;localLight.position[2]=-1;localLight.castsShadow=0;
      const auto localLamp=RenderSceneCreateLight(&localLight);check(localLamp!=0,"local tile fixture");
      const float mixed=sample(0);
      check(std::abs(mixed-lit)<lit*.01f,"mixed global/local tile lists neither duplicate nor drop direct light");
      check(RenderSceneDestroy(localLamp),"remove local tile fixture");

      light.sourceRadius=.1f;check(RenderSceneUpdateLight(lamp,&light),"finite emitter size");
      for(const auto filter:{dark::ShadowFilter::Hard,dark::ShadowFilter::PCF,dark::ShadowFilter::PCSS}) {
         dark::ShadowSettings settings;settings.filter=filter;renderer.SetShadowSettings(settings);
         instanceData.castsShadow=1;check(RenderSceneUpdateInstance(caster,&instanceData),"restore finite-emitter caster");
         const float occluded=sample(0);
         check(occluded>=shadow*.9f && occluded<lit*.5f,"selected filter retains occluded receiver");
         instanceData.castsShadow=0;check(RenderSceneUpdateInstance(caster,&instanceData),"remove finite-emitter caster");
         check(sample(1)>lit*.9f,"selected filter leaves clear receiver illuminated");
      }
      const auto pointLight=light;
      // Rotate one complete scene through all six cube directions. This
      // exercises UV selection and sampled-ray reconstruction together.
      for(bool cached:{false,true})for(unsigned orientation=0;orientation<6;++orientation) {
         auto rotate=[&](const float *v,float *out) {
            const float x=v[0],y=v[1],z=v[2];
            switch(orientation) {
            case 0:out[0]=x;out[1]=y;out[2]=z;break;
            case 1:out[0]=-x;out[1]=y;out[2]=-z;break;
            case 2:out[0]=z;out[1]=y;out[2]=-x;break;
            case 3:out[0]=-z;out[1]=y;out[2]=x;break;
            case 4:out[0]=x;out[1]=-z;out[2]=y;break;
            default:out[0]=x;out[1]=z;out[2]=-y;break;
            }
         };
         auto rotatedReceiver=receiverPlacement,rotatedCaster=instanceData;
         rotatedCaster.visible=0;
         for(unsigned column=0;column<3;++column) {
            rotate(receiverPlacement.transform+column*4,rotatedReceiver.transform+column*4);
            rotate(instanceData.transform+column*4,rotatedCaster.transform+column*4);
         }
         const float translation[]={-1,0,0};rotate(translation,rotatedCaster.transform+12);
         check(RenderSceneUpdateInstance(receiverInstance,&rotatedReceiver),"rotate cube receiver");
         float re[3],rf[3],rr[3],rb[3];rotate(eye,re);rotate(forward,rf);rotate(right,rr);rotate(bottom,rb);
         auto rotatedCamera=camera;
         check(RenderSceneCameraFromRays(&rotatedCamera,re,rf,rr,rb,.1f,100),"rotate cube camera");
         check(RenderSceneSetCamera(&rotatedCamera),"set rotated cube camera");
         auto rotatedLight=pointLight;const float lampPosition[]={0,0,-2};rotate(lampPosition,rotatedLight.position);
         check(RenderSceneUpdateLight(lamp,&rotatedLight),"rotate cube emitter");
         dark::ShadowSettings settings;
         if(cached) {settings.localLights=settings.localFaces=0;settings.cachedLights=1;settings.cachedFaces=6;settings.cachedResolution=128;}
         renderer.SetShadowSettings(settings);
         for(uint32_t slot=0;slot<2;++slot) {
            rotatedCaster.castsShadow=0;check(RenderSceneUpdateInstance(caster,&rotatedCaster),"cube face clear reference");
            const float reference=sample(slot);
            rotatedCaster.castsShadow=1;check(RenderSceneUpdateInstance(caster,&rotatedCaster),"cube face blocker");
            const float occluded=sample(slot);
            std::printf("Cube orientation=%u cached=%d slot=%u clear=%.5f occluded=%.5f\n",orientation,int(cached),slot,reference,occluded);
            check(reference>.1f && occluded<reference*.1f,"all six cube faces preserve shadow occlusion");
         }
      }
      check(RenderSceneUpdateInstance(receiverInstance,&receiverPlacement),"restore rotated receiver");
      check(RenderSceneUpdateInstance(caster,&instanceData),"restore rotated caster");
      check(RenderSceneSetCamera(&camera),"restore rotated camera");
      check(RenderSceneUpdateLight(lamp,&pointLight),"restore rotated light");
      renderer.SetShadowSettings(dark::ShadowSettings{});
      // A smoothed shading normal must not tilt the actual receiver plane.
      for(auto &vertex:receiverVertices){vertex.normal[1]=.6f;vertex.normal[2]=-.8f;}
      check(RenderSceneUpdateVertices(receiver,receiverVertices,3),"smoothed receiver normals");
      instanceData.castsShadow=0;check(RenderSceneUpdateInstance(caster,&instanceData),"remove blocker for local self-shadow check");
      for(bool cached:{false,true}) {
         dark::ShadowSettings clearSettings;
         if(cached) {
            clearSettings.localLights=clearSettings.localFaces=0;
            clearSettings.cachedLights=1;clearSettings.cachedFaces=6;clearSettings.cachedResolution=128;
         }
         renderer.SetShadowSettings(clearSettings);
         for(const auto type:{RenderScenePointLight,RenderSceneSpotLight})for(float x:{.5f,2.f,6.f,20.f,60.f}) {
            light=pointLight;light.type=type;light.position[0]=x;light.sourceRadius=.4f;
            const float length=std::sqrt(x*x+4);
            // Keep grazing-angle light strong enough that ambient cannot hide self-shadowing.
            if(x>6)light.intensity*=((x*x+4)/8)*std::max(1.f,length/6);
            light.direction[0]=-x/length;light.direction[2]=2/length;
            light.innerCos=.9f;light.outerCos=.5f;light.castsShadow=0;
            check(RenderSceneUpdateLight(lamp,&light),"unshadowed local-light reference");const float reference=sample(0);
            light.castsShadow=1;check(RenderSceneUpdateLight(lamp,&light),"local receiver self-shadow test");const float clear=sample(1);
            std::printf("Local clear surface cached=%d type %u angle x=%.1f: reference=%.4f shadow-enabled=%.4f\n",int(cached),unsigned(type),x,reference,clear);
            check(clear>reference*.99f,"point/spot emitter preserves a clear receiving plane");
         }
      }
      renderer.SetShadowSettings(dark::ShadowSettings{});
      light=pointLight;
      for(auto &vertex:receiverVertices){vertex.normal[1]=0;vertex.normal[2]=-1;}
      check(RenderSceneUpdateVertices(receiver,receiverVertices,3),"restore receiver shading normals");
      for(const auto type:{RenderSceneSpotLight,RenderSceneDirectionalLight}) {
         light.type=type;light.direction[0]=-float(std::sqrt(.5));light.direction[2]=float(std::sqrt(.5));
         light.innerCos=.9f;light.outerCos=.5f;light.angularRadius=.0047f;
         check(RenderSceneUpdateLight(lamp,&light),"spot/sun lighting fixture");
         instanceData.castsShadow=0;check(RenderSceneUpdateInstance(caster,&instanceData),"spot/sun clear receiver");
         const float reference=sample(0);check(reference>.1f,"spot/sun contributes direct lighting");
         instanceData.castsShadow=1;check(RenderSceneUpdateInstance(caster,&instanceData),"spot/sun occluder");
         for(const auto filter:{dark::ShadowFilter::Hard,dark::ShadowFilter::PCF,dark::ShadowFilter::PCSS}) {
            dark::ShadowSettings settings;settings.filter=filter;renderer.SetShadowSettings(settings);
            const float occluded=sample(1);
            std::printf("Light type %u filter %u radiance: clear=%.4f shadow=%.4f\n",unsigned(type),unsigned(filter),reference,occluded);
            check(occluded<reference*.1f && occluded<.006f,"spot/sun offscreen caster shadows receiver");
         }
      }
      // Sweep a fixed shadowed receiver across the sun's split/blend ranges.
      for(const float distance:{3.f,7.5f,8.2f,16.8f,17.6f,34.3f,35.7f,75.f}) {
         const float cascadeEye[]={0,0,-distance};
         check(RenderSceneCameraFromRays(&camera,cascadeEye,forward,right,bottom,.1f,100),"cascade sweep camera");
         check(RenderSceneSetCamera(&camera),"set cascade sweep camera");
         instanceData.castsShadow=0;check(RenderSceneUpdateInstance(caster,&instanceData),"cascade unoccluded reference");
         const float reference=sample(0);
         instanceData.castsShadow=1;check(RenderSceneUpdateInstance(caster,&instanceData),"cascade blocker");
         const float occluded=sample(1);
         std::printf("Sun cascade receiver distance %.1f: clear=%.4f shadow=%.4f\n",distance,reference,occluded);
         check(reference>11.1f && occluded<reference*.1f,"sun cascade sweep keeps clear receivers lit and retains occlusion through split/blend ranges");
      }
      check(RenderSceneCameraFromRays(&camera,eye,forward,right,bottom,.1f,100),"restore camera after cascade sweep");
      check(RenderSceneSetCamera(&camera),"set restored cascade camera");
      light=pointLight;check(RenderSceneUpdateLight(lamp,&light),"restore point light");
      // Scan across the projected occluder edge with a wider camera. Normalize
      // each pixel against its unoccluded radiance to isolate shadow filtering.
      const float wideRight[]={1,0,1},wideBottom[]={0,1,1};
      check(RenderSceneCameraFromRays(&camera,eye,forward,wideRight,wideBottom,.1f,100),"penumbra camera");
      check(RenderSceneSetCamera(&camera),"set penumbra camera");
      light.sourceRadius=.4f;check(RenderSceneUpdateLight(lamp,&light),"penumbra emitter");
      // Hold projection and geometry fixed while varying only the eye uniform.
      // Normalize by each eye's unoccluded BRDF to isolate shadow lookup bias
      // from the expected view-dependent specular response.
      for(unsigned type:{uint32_t(RenderScenePointLight),uint32_t(RenderSceneSpotLight)}) {
         light.type=type;light.direction[0]=-float(std::sqrt(.5));light.direction[2]=float(std::sqrt(.5));
         light.innerCos=.9f;light.outerCos=.5f;
         check(RenderSceneUpdateLight(lamp,&light),"camera-independent shadow emitter");
         float maximumDifference=0;
         for(int x=32;x<48;++x) {
            float visibility[2]{};
            for(unsigned farEye=0;farEye<2;++farEye) {
               auto eyeCamera=camera;eyeCamera.position[2]=farEye?-30000.f:camera.position[2];
               check(RenderSceneSetCamera(&eyeCamera),"shadow eye uniform isolation");
               instanceData.castsShadow=0;check(RenderSceneUpdateInstance(caster,&instanceData),"eye shadow reference");
               const float reference=sample(0,64,64,x);
               instanceData.castsShadow=1;check(RenderSceneUpdateInstance(caster,&instanceData),"eye shadow blocker");
               visibility[farEye]=(sample(1,64,64,x)-shadow)/(reference-shadow);
            }
            maximumDifference=std::max(maximumDifference,std::abs(visibility[0]-visibility[1]));
         }
         std::printf("Eye-uniform shadow invariance type=%u maximum_difference=%.6f\n",type,maximumDifference);
         check(maximumDifference<.01f,"camera distance does not change local shadow visibility");
      }
      check(RenderSceneSetCamera(&camera),"restore camera after eye isolation");
      light=pointLight;light.sourceRadius=.4f;check(RenderSceneUpdateLight(lamp,&light),"restore penumbra emitter");
      unsigned hardPartial=0,softPartial=0,smallPartial=0;
      for(int x=32;x<48;++x) {
         instanceData.castsShadow=0;check(RenderSceneUpdateInstance(caster,&instanceData),"penumbra reference");
         const float reference=sample(0,64,64,x);
         instanceData.castsShadow=1;check(RenderSceneUpdateInstance(caster,&instanceData),"penumbra caster");
         for(unsigned mode=0;mode<3;++mode) {
            const auto filter=mode==0?dark::ShadowFilter::Hard:dark::ShadowFilter::PCSS;
            light.sourceRadius=mode==1?.1f:.4f;check(RenderSceneUpdateLight(lamp,&light),"scan emitter radius");
            dark::ShadowSettings settings;settings.filter=filter;renderer.SetShadowSettings(settings);
            const float visibility=(sample(1,64,64,x)-shadow)/(reference-shadow);
            if(visibility>.1f && visibility<.9f) {
               if(mode==0)++hardPartial;else if(mode==1)++smallPartial;else ++softPartial;
            }
         }
      }
      std::printf("Penumbra intermediate pixels: hard=%u small PCSS=%u large PCSS=%u\n",hardPartial,smallPartial,softPartial);
      check(softPartial>smallPartial,"larger emitter widens the PCSS penumbra");
      check(softPartial>hardPartial,"finite emitter PCSS produces a wider penumbra than hard shadows");
      // Move the blocker from halfway to near the receiver while preserving
      // its projected silhouette from the lamp, isolating contact hardening.
      instanceData.transform[0]=instanceData.transform[5]=instanceData.transform[10]=1.75f;
      instanceData.transform[12]=-1.5f;instanceData.transform[14]=1.5f;
      unsigned contactPartial=0;
      for(int x=32;x<48;++x) {
         instanceData.castsShadow=0;check(RenderSceneUpdateInstance(caster,&instanceData),"contact reference");
         const float reference=sample(0,64,64,x);
         instanceData.castsShadow=1;check(RenderSceneUpdateInstance(caster,&instanceData),"near-contact caster");
         const float visibility=(sample(1,64,64,x)-shadow)/(reference-shadow);
         if(visibility>.1f && visibility<.9f)++contactPartial;
      }
      std::printf("Contact-hardening intermediate pixels: distant=%u near=%u\n",softPartial,contactPartial);
      check(contactPartial<softPartial,"PCSS penumbra narrows as blocker approaches receiver");
      instanceData.transform[0]=instanceData.transform[5]=instanceData.transform[10]=1;
      instanceData.transform[12]=instanceData.transform[14]=0;
      check(RenderSceneUpdateInstance(caster,&instanceData),"restore blocker placement");
      for(const auto type:{RenderSceneSpotLight,RenderSceneDirectionalLight}) {
         light.type=type;light.direction[0]=-float(std::sqrt(.5));light.direction[2]=float(std::sqrt(.5));
         light.innerCos=.9f;light.outerCos=.5f;
         unsigned partial[2]{};
         for(unsigned size=0;size<2;++size) {
            light.sourceRadius=size?.4f:.1f;light.angularRadius=size?.15f:.02f;
            check(RenderSceneUpdateLight(lamp,&light),"spot/sun penumbra emitter");
            for(int x=32;x<48;++x) {
               instanceData.castsShadow=0;check(RenderSceneUpdateInstance(caster,&instanceData),"spot/sun penumbra reference");
               const float reference=sample(0,64,64,x);
               instanceData.castsShadow=1;check(RenderSceneUpdateInstance(caster,&instanceData),"spot/sun penumbra blocker");
               const float visibility=(sample(1,64,64,x)-shadow)/(reference-shadow);
               if(visibility>.1f && visibility<.9f)++partial[size];
            }
         }
         std::printf("Light type %u penumbra pixels: small=%u large=%u\n",unsigned(type),partial[0],partial[1]);
         check(partial[1]>partial[0],"spot/sun penumbra widens with emitter size");
      }
      // The distant pool must preserve physical emitter-size response, not
      // just a binary shadow at the fully occluded center of the receiver.
      for(unsigned type:{uint32_t(RenderScenePointLight),uint32_t(RenderSceneSpotLight)}) {
         for(bool cached:{false,true}) {
            dark::ShadowSettings settings;
            if(cached) {
               settings.localLights=settings.localFaces=0;
               settings.cachedLights=1;settings.cachedFaces=6;settings.cachedResolution=128;
            }
            renderer.SetShadowSettings(settings);
            light=pointLight;light.type=type;
            light.direction[0]=-float(std::sqrt(.5));light.direction[2]=float(std::sqrt(.5));
            light.innerCos=.9f;light.outerCos=.5f;
            unsigned partial[2]{};
            for(unsigned size=0;size<2;++size) {
               light.sourceRadius=size?.4f:.1f;
               check(RenderSceneUpdateLight(lamp,&light),"pool penumbra emitter size");
               for(int x=32;x<48;++x) {
                  instanceData.castsShadow=0;check(RenderSceneUpdateInstance(caster,&instanceData),"pool penumbra reference");
                  const float reference=sample(0,64,64,x);
                  instanceData.castsShadow=1;check(RenderSceneUpdateInstance(caster,&instanceData),"pool penumbra blocker");
                  const float visibility=(sample(1,64,64,x)-shadow)/(reference-shadow);
                  if(visibility>.1f && visibility<.9f)++partial[size];
               }
            }
            std::printf("Pool penumbra type=%u cached=%d small=%u large=%u\n",type,int(cached),partial[0],partial[1]);
            check(partial[1]>partial[0],"both shadow pools preserve PCSS emitter-size response");
            const auto &selection=renderer.GetShadowSelection();
            check((cached?selection.cachedLights:selection.localLights).size()==1,"penumbra samples use requested shadow pool");
            instanceData.transform[0]=instanceData.transform[5]=instanceData.transform[10]=1.75f;
            instanceData.transform[12]=-1.5f;instanceData.transform[14]=1.5f;
            unsigned contact=0;
            for(int x=32;x<48;++x) {
               instanceData.castsShadow=0;check(RenderSceneUpdateInstance(caster,&instanceData),"pool contact reference");
               const float reference=sample(0,64,64,x);
               instanceData.castsShadow=1;check(RenderSceneUpdateInstance(caster,&instanceData),"pool contact blocker");
               const float visibility=(sample(1,64,64,x)-shadow)/(reference-shadow);
               if(visibility>.1f && visibility<.9f)++contact;
            }
            std::printf("Pool contact type=%u cached=%d distant=%u near=%u\n",type,int(cached),partial[1],contact);
            check(contact<partial[1],"both shadow pools preserve PCSS contact hardening");
            instanceData.transform[0]=instanceData.transform[5]=instanceData.transform[10]=1;
            instanceData.transform[12]=instanceData.transform[14]=0;
            check(RenderSceneUpdateInstance(caster,&instanceData),"restore pool contact blocker");
         }
      }
      renderer.SetShadowSettings(dark::ShadowSettings{});
      light=pointLight;check(RenderSceneUpdateLight(lamp,&light),"restore point emitter after penumbra tests");
      check(RenderSceneCameraFromRays(&camera,eye,forward,right,bottom,.1f,100),"restore narrow camera");
      check(RenderSceneSetCamera(&camera),"restore camera");
      light.sourceRadius=.1f;check(RenderSceneUpdateLight(lamp,&light),"restore emitter");
      // Recreate HDR/depth targets independently in both retired frame slots.
      // The offscreen caster must keep shadowing after growing and shrinking.
      instanceData.castsShadow=1;check(RenderSceneUpdateInstance(caster,&instanceData),"resize caster");
      for(const auto size:{128u,32u,64u})for(uint32_t slot=0;slot<2;++slot)
         check(sample(slot,size,size)<lit*.5f,"shadow survives HDR target resize in both slots");
      // No directional light must cost the same as explicitly disabling sun
      // cascades. Adding/removing one must grow/retire both frame-slot pools,
      // including the optional stable-depth bases, without losing local shadows.
      for(const bool stableDepth:{false,true}) {
         dark::ShadowSettings allocationSettings;allocationSettings.staticDepthCache=stableDepth;
         allocationSettings.sunCascades=0;renderer.SetShadowSettings(allocationSettings);
         std::array<uint64_t,2> withoutSun{};
         for(uint32_t slot=0;slot<2;++slot) {
            check(sample(slot)<lit*.5f,"local shadow with disabled sun pool");
            withoutSun[slot]=renderer.GetShadowWorkload(slot).depthMemoryBytes;
         }
         allocationSettings.sunCascades=4;renderer.SetShadowSettings(allocationSettings);
         for(uint32_t slot=0;slot<2;++slot) {
            check(sample(slot)<lit*.5f,"local shadow with unused sun budget");
            check(renderer.GetShadowWorkload(slot).depthMemoryBytes==withoutSun[slot],"unused sun budget reserves no cascade images");
         }
         auto sun=light;sun.type=RenderSceneDirectionalLight;sun.direction[0]=0;sun.direction[1]=0;sun.direction[2]=1;
         const auto sunHandle=RenderSceneCreateLight(&sun);check(sunHandle!=0,"temporary allocation sun");
         for(uint32_t slot=0;slot<2;++slot) {
            sample(slot);
            check(renderer.GetShadowSelection().sunlight==sunHandle,"new sun selected");
            check(renderer.GetShadowWorkload(slot).depthMemoryBytes>withoutSun[slot],"sun allocates cascades on demand");
            std::printf("Shadow image memory stable=%u slot=%u without_sun=%llu with_sun=%llu\n",
               unsigned(stableDepth),slot,static_cast<unsigned long long>(withoutSun[slot]),
               static_cast<unsigned long long>(renderer.GetShadowWorkload(slot).depthMemoryBytes));
         }
         check(RenderSceneDestroy(sunHandle),"retire temporary allocation sun");
         for(uint32_t slot=0;slot<2;++slot) {
            check(sample(slot)<lit*.5f,"local shadow survives sun removal");
            check(renderer.GetShadowWorkload(slot).depthMemoryBytes==withoutSun[slot],"sun removal retires unused cascade images");
         }
      }
      dark::ShadowSettings noShadows;noShadows.localLights=0;noShadows.localFaces=0;noShadows.sunCascades=0;
      renderer.SetShadowSettings(noShadows);
      for(uint32_t slot=0;slot<2;++slot)
         check(sample(slot)>lit*.9f,"zero shadow budget retains direct illumination");
      renderer.SetShadowSettings(dark::ShadowSettings{});
      for(uint32_t slot=0;slot<2;++slot)
         check(sample(slot)<lit*.5f,"restored shadow budget rebuilds both slots");
      // Explicit higher budgets must survive the production renderer settings
      // path, not just the CPU selector. Retire each slot before resizing it.
      std::vector<RenderSceneHandle> extraLights;
      auto extra=light;extra.type=RenderSceneSpotLight;extra.direction[2]=1;
      extra.outerCos=.5f;extra.innerCos=.8f;extra.intensity=.001f;
      for(unsigned i=0;i<17;++i) {
         const auto handle=RenderSceneCreateLight(&extra);check(handle!=0,"extra budget light");
         extraLights.push_back(handle);
      }
      dark::ShadowSettings expanded;expanded.localLights=18;expanded.localFaces=24;
      expanded.localResolution=32;renderer.SetShadowSettings(expanded);
      for(uint32_t slot=0;slot<2;++slot) {
         sample(slot);
         const auto workload=renderer.GetShadowWorkload(slot);
         check(workload.renderedViews+workload.reusedViews==23,"expanded budget includes all 18 emitters");
      }
      for(const auto handle:extraLights)check(RenderSceneDestroy(handle),"remove extra budget light");
      renderer.SetShadowSettings(dark::ShadowSettings{});
      for(uint32_t slot=0;slot<2;++slot)
         check(sample(slot)<lit*.5f,"default shadow budget survives shrinking expanded targets");
      dark::ShadowSettings distant;distant.localLights=distant.localFaces=0;
      distant.cachedLights=1;distant.cachedFaces=6;distant.cachedResolution=64;
      renderer.SetShadowSettings(distant);
      for(uint32_t slot=0;slot<2;++slot) {
         check(sample(slot)<lit*.5f,"distant point cache preserves occlusion outside active budget");
         check(renderer.GetShadowSelection().localLights.empty() && renderer.GetShadowSelection().cachedLights.size()==1,"distant pool selected independently");
         instanceData.castsShadow=0;check(RenderSceneUpdateInstance(caster,&instanceData),"remove distant blocker");
         check(sample(slot)>lit*.9f,"distant cache responds to caster removal");
         instanceData.castsShadow=1;check(RenderSceneUpdateInstance(caster,&instanceData),"restore distant blocker");
         check(sample(slot)<lit*.5f,"distant cache restores caster shadow");
      }
      const auto cachedPoint=light;
      light.type=RenderSceneSpotLight;light.direction[0]=-.70710678f;light.direction[2]=.70710678f;
      light.outerCos=.5f;light.innerCos=.8f;check(RenderSceneUpdateLight(lamp,&light),"cached spot emitter");
      for(uint32_t slot=0;slot<2;++slot)check(sample(slot)<lit*.5f,"distant spot cache preserves occlusion");
      light=cachedPoint;check(RenderSceneUpdateLight(lamp,&light),"restore cached point emitter");
      // Move the camera across a real priority crossover. The competing light
      // is behind the receiver, so its priority changes without adding direct
      // illumination to the sampled front surface.
      auto competitor=light;competitor.position[0]=-2;competitor.position[2]=2;
      const auto competitorId=RenderSceneCreateLight(&competitor);check(competitorId!=0,"camera crossover competitor");
      const RenderSceneHandle crossoverLights[]={lamp,competitorId};
      check(RenderSceneSetInstanceLights(receiverInstance,crossoverLights,2),"camera crossover receiver lights");
      distant.localLights=1;distant.localFaces=6;renderer.SetShadowSettings(distant);
      instanceData.visible=0;check(RenderSceneUpdateInstance(caster,&instanceData),"hide crossover caster from camera");
      for(unsigned type:{uint32_t(RenderScenePointLight),uint32_t(RenderSceneSpotLight)}) {
         light=cachedPoint;light.type=type;
         if(type==RenderSceneSpotLight) {
            light.direction[0]=-.70710678f;light.direction[2]=.70710678f;
            light.outerCos=.5f;light.innerCos=.8f;
         }
         check(RenderSceneUpdateLight(lamp,&light),"crossover light type");
         for(auto filter:{dark::ShadowFilter::Hard,dark::ShadowFilter::PCF,dark::ShadowFilter::PCSS}) {
            distant.filter=filter;renderer.SetShadowSettings(distant);
            bool sawActive=false,sawCached=false;
            for(float x:{-8.f,-4.f,0.f,4.f,8.f,4.f,0.f,-4.f,-8.f}) {
               const float movingEye[]={x,0,-3},look[]={-x,0,3};
               const float length=std::sqrt(x*x+9);
               const float movingRight[]={look[0]+.05f*look[2],0,look[2]-.05f*look[0]};
               const float movingDown[]={look[0],.05f*length,look[2]};
               auto movingCamera=camera;
               check(RenderSceneCameraFromRays(&movingCamera,movingEye,look,movingRight,movingDown,.1f,100),"moving crossover camera");
               check(RenderSceneSetCamera(&movingCamera),"set crossover camera");
               for(uint32_t slot=0;slot<2;++slot) {
                  instanceData.castsShadow=0;check(RenderSceneUpdateInstance(caster,&instanceData),"crossover unoccluded reference");
                  const float reference=sample(slot);
                  instanceData.castsShadow=1;check(RenderSceneUpdateInstance(caster,&instanceData),"crossover blocker");
                  const float occluded=sample(slot);
                  check(reference>shadow*10 && occluded<reference*.2f,"camera priority crossover preserves receiver occlusion");
                  const auto &selection=renderer.GetShadowSelection();
                  const bool active=std::find(selection.localLights.begin(),selection.localLights.end(),lamp)!=selection.localLights.end();
                  const bool cached=std::find(selection.cachedLights.begin(),selection.cachedLights.end(),lamp)!=selection.cachedLights.end();
                  check(active!=cached,"crossover emitter has exactly one shadow pool");
                  sawActive|=active;sawCached|=cached;
                  std::printf("Camera crossover type=%u filter=%u x=%.1f slot=%u active=%d cached=%d radiance=%.5f reference=%.5f\n",type,uint32_t(filter),x,slot,int(active),int(cached),occluded,reference);
               }
            }
            check(sawActive && sawCached,"moving camera exercises both shadow pools");
         }
      }
      light=cachedPoint;check(RenderSceneUpdateLight(lamp,&light),"restore emitter after filter crossovers");
      check(RenderSceneDestroy(competitorId),"remove crossover competitor");
      check(RenderSceneSetInstanceLights(receiverInstance,&lamp,1),"restore receiver after crossover");
      check(RenderSceneSetCamera(&camera),"restore camera after crossover");
      instanceData.visible=1;check(RenderSceneUpdateInstance(caster,&instanceData),"restore caster after crossover");
      renderer.SetShadowSettings(dark::ShadowSettings{});
      for(uint32_t slot=0;slot<2;++slot)check(sample(slot)<lit*.5f,"active shadow restored after distant pool disable");
      light.enabled=0;check(RenderSceneUpdateLight(lamp,&light),"disable light");check(sample(0)<0.006f,"disabled light");
      light.enabled=1;check(RenderSceneUpdateLight(lamp,&light),"enable light");
      uint8_t pixel[]={255,255,255,0};const auto texture=RenderSceneCreateTexture(1,1,pixel,4);
      material.baseColorTexture=texture;material.opacity=RenderSceneMasked;
      check(RenderSceneUpdateMaterial(occluderMaterial,&material),"masked material");instanceData.castsShadow=1;
      check(RenderSceneUpdateInstance(caster,&instanceData),"enable caster");check(sample(0)>0.1f,"cutout hole passes shadow map");
      pixel[3]=255;check(RenderSceneUpdateTexture(texture,pixel,4),"update opacity texture");check(sample(1)<0.006f,"opaque cutout blocks shadow map");
      const uint8_t resizedCutout[]={255,255,255,0,255,255,255,0,255,255,255,0,255,255,255,0};
      check(RenderSceneReplaceTexture(texture,2,2,resizedCutout,8),"resize masked texture without replacing material handle");
      check(sample(0)>.1f,"resized alpha image rebuilds GPU texture and opens shadow");
      check(RenderSceneReplaceTexture(texture,1,1,pixel,4),"restore masked texture dimensions");
      check(sample(1)<.006f,"restored image rebuilds GPU texture and shadow");
      material.opacity=RenderSceneBlended;check(RenderSceneUpdateMaterial(occluderMaterial,&material),"blended material");check(sample(0)>0.1f,"blended surface does not cast opaque shadow");
      std::printf("PASS on %s: HDR direct light %.4f vs offscreen-caster shadow %.4f, light toggles, alpha-tested shadow maps and texture updates\n",physical.getProperties().deviceName.data(),lit,shadow);
      // Read the production tone-map/composition shader over a known native
      // background: uncovered sky, opaque geometry, and quarter coverage.
      struct Target {vk::UniqueDeviceMemory memory;vk::UniqueImage image;vk::UniqueImageView view;};
      auto target=[&](vk::Format format,vk::ImageUsageFlags usage,vk::ImageAspectFlags aspect) {
         Target result;
         result.image=device->createImageUnique(vk::ImageCreateInfo({},vk::ImageType::e2D,format,{64,64,1},1,1,
            vk::SampleCountFlagBits::e1,vk::ImageTiling::eOptimal,usage,vk::SharingMode::eExclusive));
         const auto requirements=device->getImageMemoryRequirements(*result.image);uint32_t index=0;
         while(index<memory.memoryTypeCount && (!(requirements.memoryTypeBits&(1u<<index)) || !(memory.memoryTypes[index].propertyFlags&vk::MemoryPropertyFlagBits::eDeviceLocal)))++index;
         check(index<memory.memoryTypeCount,"composition image memory");
         result.memory=device->allocateMemoryUnique(vk::MemoryAllocateInfo(requirements.size,index));device->bindImageMemory(*result.image,*result.memory,0);
         result.view=device->createImageViewUnique(vk::ImageViewCreateInfo({},*result.image,vk::ImageViewType::e2D,format,{},vk::ImageSubresourceRange(aspect,0,1,0,1)));
         return result;
      };
      auto colorTarget=target(vk::Format::eR8G8B8A8Unorm,vk::ImageUsageFlagBits::eColorAttachment|vk::ImageUsageFlagBits::eTransferSrc,vk::ImageAspectFlagBits::eColor);
      auto depthTarget=target(vk::Format::eD32Sfloat,vk::ImageUsageFlagBits::eDepthStencilAttachment|vk::ImageUsageFlagBits::eTransferSrc,vk::ImageAspectFlagBits::eDepth);
      const std::array<vk::ImageView,2> views{{*colorTarget.view,*depthTarget.view}};
      auto framebuffer=device->createFramebufferUnique(vk::FramebufferCreateInfo({},*pass,views,64,64,1));
      float compositeDepth=0;
      auto compositePixel=[&](uint32_t slot) {
         const auto recorded=renderer.Record(slot,dark::GetWorldScene(),64,64,error);check(recorded!=VK_NULL_HANDLE,error.c_str());submit(recorded);
         auto cmd=*commands[0];cmd.reset();cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
         std::array<vk::ClearValue,2> clear;clear[0].color=vk::ClearColorValue(std::array<float,4>{{0,.25f,1,1}});clear[1].depthStencil=vk::ClearDepthStencilValue(1,0);
         cmd.beginRenderPass(vk::RenderPassBeginInfo(*pass,*framebuffer,{{0,0},{64,64}},clear),vk::SubpassContents::eInline);
         renderer.Composite(cmd,slot,0,0,64,64);cmd.endRenderPass();
         vk::ImageMemoryBarrier barrier(vk::AccessFlagBits::eColorAttachmentWrite,vk::AccessFlagBits::eTransferRead,
            vk::ImageLayout::eColorAttachmentOptimal,vk::ImageLayout::eTransferSrcOptimal,VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,
            *colorTarget.image,{vk::ImageAspectFlagBits::eColor,0,1,0,1});
         cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,vk::PipelineStageFlagBits::eTransfer,{}, {}, {},barrier);
         cmd.copyImageToBuffer(*colorTarget.image,vk::ImageLayout::eTransferSrcOptimal,*output,
            vk::BufferImageCopy(0,0,0,{vk::ImageAspectFlagBits::eColor,0,0,1},{32,32,0},{1,1,1}));
         vk::ImageMemoryBarrier depthBarrier(vk::AccessFlagBits::eDepthStencilAttachmentWrite,vk::AccessFlagBits::eTransferRead,
            vk::ImageLayout::eDepthStencilAttachmentOptimal,vk::ImageLayout::eTransferSrcOptimal,VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,
            *depthTarget.image,{vk::ImageAspectFlagBits::eDepth,0,1,0,1});
         cmd.pipelineBarrier(vk::PipelineStageFlagBits::eEarlyFragmentTests|vk::PipelineStageFlagBits::eLateFragmentTests,vk::PipelineStageFlagBits::eTransfer,{}, {}, {},depthBarrier);
         cmd.copyImageToBuffer(*depthTarget.image,vk::ImageLayout::eTransferSrcOptimal,*output,
            vk::BufferImageCopy(4,0,0,{vk::ImageAspectFlagBits::eDepth,0,0,1},{32,32,0},{1,1,1}));
         vk::MemoryBarrier read(vk::AccessFlagBits::eTransferWrite,vk::AccessFlagBits::eHostRead);
         cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},read,{},{});cmd.end();submit(cmd);
         const auto *bytes=reinterpret_cast<const uint8_t *>(mapped);
         std::memcpy(&compositeDepth,bytes+4,sizeof(float));
         return std::array<int,4>{{bytes[0],bytes[1],bytes[2],bytes[3]}};
      };
      auto placement=dark::GetWorldScene().instances.at(receiverInstance);placement.visible=0;
      check(RenderSceneUpdateInstance(receiverInstance,&placement),"hide receiver for sky test");
      instanceData.visible=0;check(RenderSceneUpdateInstance(caster,&instanceData),"hide caster for sky test");
      const auto sky=compositePixel(0);check(sky==std::array<int,4>{{0,64,255,255}},"native sky preserved through empty HDR coverage");
      light.enabled=0;check(RenderSceneUpdateLight(lamp,&light),"disable direct light for isolated tone-map test");
      placement.visible=1;check(RenderSceneUpdateInstance(receiverInstance,&placement),"show emissive receiver");
      auto emissive=material;emissive.baseColorTexture=0;emissive.opacity=RenderSceneOpaque;
      emissive.baseColor[0]=emissive.baseColor[1]=emissive.baseColor[2]=0;emissive.baseColor[3]=1;
      emissive.emission[0]=1;check(RenderSceneUpdateMaterial(receiverMaterial,&emissive),"emissive composition material");
      emissive.baseColorTexture=texture;emissive.emissionUsesBaseTexture=1;
      pixel[0]=pixel[1]=pixel[2]=128;pixel[3]=255;
      check(RenderSceneUpdateTexture(texture,pixel,4),"emissive base texture");
      check(RenderSceneUpdateMaterial(receiverMaterial,&emissive),"textured emission material");
      const float texturedEmission=sample(0);
      check(texturedEmission>.20f && texturedEmission<.23f,"emission uses linearized base texture RGB");
      // Retain an unused animation frame beyond both slots, then force LRU
      // eviction with a zero retention budget and verify clean re-upload.
      const uint8_t cachePixelA[]={30,60,90,255},cachePixelB[]={90,60,30,255};
      const auto cacheA=RenderSceneCreateTexture(1,1,cachePixelA,4),cacheB=RenderSceneCreateTexture(1,1,cachePixelB,4);
      auto cacheDescription=material;cacheDescription.baseColorTexture=cacheA;
      const auto cacheMaterial=RenderSceneCreateMaterial(&cacheDescription);check(cacheA && cacheB && cacheMaterial,"texture cache fixtures");
      sample(0);cacheDescription.baseColorTexture=cacheB;check(RenderSceneUpdateMaterial(cacheMaterial,&cacheDescription),"switch animation frame");
      sample(1);sample(0);
      cacheDescription.baseColorTexture=cacheA;check(RenderSceneUpdateMaterial(cacheMaterial,&cacheDescription),"return to retained animation frame");
      sample(1);check(renderer.GetTextureWorkload(1).createdImages==0 && renderer.GetTextureWorkload(1).uploads==0,"unused animation frame reuses immutable GPU image");
      renderer.SetTextureCacheBudget(0);
      cacheDescription.baseColorTexture=cacheB;check(RenderSceneUpdateMaterial(cacheMaterial,&cacheDescription),"release animation frame A");
      sample(0);sample(1);check(renderer.GetTextureWorkload(1).evictedImages>0,"unreferenced images obey retention budget");
      cacheDescription.baseColorTexture=cacheA;check(RenderSceneUpdateMaterial(cacheMaterial,&cacheDescription),"request evicted animation frame");
      sample(0);check(renderer.GetTextureWorkload(0).createdImages==1 && renderer.GetTextureWorkload(0).uploads==1,"evicted image uploads again");
      check(RenderSceneDestroy(cacheMaterial) && RenderSceneDestroy(cacheA) && RenderSceneDestroy(cacheB),"remove texture cache fixtures");
      renderer.SetTextureCacheBudget(64ull*1024*1024);
      // A queued draw must still sample its old revision after another slot
      // records and submits an updated version of the same texture handle.
      pixel[0]=pixel[1]=pixel[2]=64;check(RenderSceneUpdateTexture(texture,pixel,4),"old queued texture revision");
      const float queuedReference=sample(0);
      const auto queued=renderer.Record(1,dark::GetWorldScene(),64,64,error);check(queued!=VK_NULL_HANDLE,error.c_str());
      pixel[0]=pixel[1]=pixel[2]=192;check(RenderSceneUpdateTexture(texture,pixel,4),"new texture revision with queued old draw");
      check(sample(0)>queuedReference*5,"new slot observes changed texture");
      submit(queued);check(std::abs(readSample(1)-queuedReference)<.001f,"queued slot retains old immutable texture revision");
      emissive.baseColorTexture=0;emissive.emissionUsesBaseTexture=0;
      check(RenderSceneUpdateMaterial(receiverMaterial,&emissive),"restore constant emission");
      const auto opaque=compositePixel(1);check(std::abs(opaque[0]-232)<=1 && opaque[1]==0 && opaque[2]==0,"opaque modern image replaces native background");
      emissive.opacity=RenderSceneMasked;emissive.baseColorTexture=texture;pixel[3]=0;
      check(RenderSceneReplaceTexture(texture,1,1,pixel,4),"transparent receiver texture");
      check(RenderSceneUpdateMaterial(receiverMaterial,&emissive),"masked receiver for depth prepass");
      check(compositePixel(0)==sky,"masked depth prepass leaves native background visible through cutout");
      pixel[3]=255;check(RenderSceneUpdateTexture(texture,pixel,4),"opaque receiver texel");
      check(compositePixel(1)==opaque,"masked depth prepass retains opaque receiver texels");
      emissive.baseColorTexture=0;
      emissive.opacity=RenderSceneBlended;emissive.baseColor[3]=.25f;
      check(RenderSceneUpdateMaterial(receiverMaterial,&emissive),"quarter coverage material");
      const auto partial=compositePixel(0);
      check(std::abs(partial[0]-58)<=1 && std::abs(partial[1]-48)<=1 && std::abs(partial[2]-191)<=1,"premultiplied HDR recovers straight color before tone mapping");
      std::puts("PASS native background preservation, opaque tone mapping and partial HDR coverage composition");
      if(queues[family].timestampValidBits) {
         const auto timings=renderer.GetShadowWorkload(0);
         const auto depth=timings.depthLightingGpuMilliseconds[0],lighting=timings.depthLightingGpuMilliseconds[1];
         check(std::isfinite(depth) && std::isfinite(lighting) && depth>=0 && lighting>=0,"retired slot exposes depth and lighting GPU intervals");
         check(depth+lighting<=timings.worldGpuMilliseconds+.001,"GPU phase intervals fit inside world interval");
      }
      const auto depthExportFeatures=physical.getFormatProperties(vk::Format::eR32Sfloat).optimalTilingFeatures;
      const auto depthExportNeeded=vk::FormatFeatureFlagBits::eSampledImage|vk::FormatFeatureFlagBits::eColorAttachment|vk::FormatFeatureFlagBits::eColorAttachmentBlend;
      if((depthExportFeatures&depthExportNeeded)==depthExportNeeded) {
         // Read back the native depth attachment after composition, independently
         // checking linear and reciprocal encodings in both frame slots.
         emissive.opacity=RenderSceneOpaque;emissive.baseColor[3]=1;
         check(RenderSceneUpdateMaterial(receiverMaterial,&emissive),"opaque depth bridge receiver");
         for(const bool reciprocal:{false,true})for(const float distance:{2.f,5.f}) {
            const float position[]={0,0,-distance};
            check(RenderSceneCameraFromRays(&camera,position,forward,right,bottom,.1f,100),"depth bridge perspective camera");
            camera.nativeDepth[0]=reciprocal?0:.01f;
            camera.nativeDepth[1]=reciprocal?-1.f:0;
            camera.nativeDepth[2]=reciprocal?1.f:0;camera.nativeDepth[3]=1;
            check(RenderSceneSetCamera(&camera),"depth bridge camera encoding");
            for(uint32_t slot=0;slot<2;++slot) {
               compositePixel(slot);
               const float expected=reciprocal?1-1/distance:distance*.01f;
               check(std::abs(compositeDepth-expected)<1e-5f,"modern surface maps to native depth in both slots");
            }
         }
         {
            auto transparent=emissive;transparent.opacity=RenderSceneBlended;transparent.baseColor[3]=.25f;
            const auto surface=RenderSceneCreateMaterial(&transparent);check(surface!=0,"transparent native depth material");
            auto frontMesh=receiverMesh;frontMesh.material=surface;
            const auto mesh=RenderSceneCreateMesh(&frontMesh);check(mesh!=0,"transparent native depth mesh");
            RenderSceneInstance front{};front.mesh=mesh;front.visible=1;
            front.transform[0]=front.transform[5]=front.transform[10]=front.transform[15]=1;front.transform[14]=-1;
            std::copy(front.transform,front.transform+16,front.previousTransform);
            const auto instance=RenderSceneCreateInstance(&front);check(instance!=0,"transparent native depth instance");
            for(uint32_t slot=0;slot<2;++slot) {
               compositePixel(slot);
               check(std::abs(compositeDepth-.8f)<1e-5f,"transparent modern surface preserves opaque native depth behind it");
            }
            check(RenderSceneDestroy(instance) && RenderSceneDestroy(mesh) && RenderSceneDestroy(surface),"retire transparent native depth surface");
         }
         // Translated/pitched cameras exercise eye-W rounding in the stored
         // projection, which is absent from the axis-aligned origin fixture.
         {
            const auto previousCamera=camera;
            const float q=std::sqrt(.5f),position[]={376,-8,-8};
            const float look[]={q,0,q},edge[]={q+.1f*q,0,q-.1f*q},lower[]={q,.1f,q};
            check(RenderSceneCameraFromRays(&camera,position,look,edge,lower,.05f,65536),"translated pitched depth camera");
            camera.nativeDepth[0]=0;camera.nativeDepth[1]=-1;camera.nativeDepth[2]=1;camera.nativeDepth[3]=1;
            check(RenderSceneSetCamera(&camera),"translated native encoding");
            const auto previous=placement;
            std::fill(placement.transform,placement.transform+16,0.f);
            placement.transform[0]=q;placement.transform[2]=-q;placement.transform[5]=1;
            placement.transform[8]=q;placement.transform[10]=q;placement.transform[15]=1;
            for(const float distance:{2.f,5.f,20.f}) {
               placement.transform[12]=position[0]+q*distance;placement.transform[13]=position[1];placement.transform[14]=position[2]+q*distance;
               check(RenderSceneUpdateInstance(receiverInstance,&placement),"translated depth receiver");
               for(uint32_t slot=0;slot<2;++slot) {
                  compositePixel(slot);
                  std::printf("Translated native depth distance=%.1f slot=%u actual=%.9f expected=%.9f\n",distance,slot,compositeDepth,1-1/distance);
                  check(std::abs(compositeDepth-(1-1/distance))<.0001f,"translated pitched native depth remains accurate");
               }
            }
            placement=previous;
            check(RenderSceneUpdateInstance(receiverInstance,&placement),"restore depth receiver transform");
            camera=previousCamera;check(RenderSceneSetCamera(&camera),"restore depth camera");
         }
         emissive.opacity=RenderSceneMasked;emissive.baseColorTexture=texture;pixel[3]=0;
         check(RenderSceneReplaceTexture(texture,1,1,pixel,4),"native depth cutout texel");
         check(RenderSceneUpdateMaterial(receiverMaterial,&emissive),"native depth cutout material");
         for(uint32_t slot=0;slot<2;++slot){check(compositePixel(slot)==sky,"native depth cutout preserves sky");check(compositeDepth==1,"native depth cutout preserves far depth");}
         pixel[3]=255;check(RenderSceneUpdateTexture(texture,pixel,4),"native depth opaque texel");
         for(uint32_t slot=0;slot<2;++slot){compositePixel(slot);check(std::abs(compositeDepth-.8f)<1e-5f,"opaque masked texel restores native depth");}
         emissive.opacity=RenderSceneOpaque;emissive.baseColorTexture=0;
         check(RenderSceneUpdateMaterial(receiverMaterial,&emissive),"restore native depth material");
         placement.visible=0;check(RenderSceneUpdateInstance(receiverInstance,&placement),"empty depth bridge coverage");
         for(uint32_t slot=0;slot<2;++slot) {
            check(compositePixel(slot)==sky,"depth bridge preserves native sky color");
            check(compositeDepth==1,"uncovered modern depth leaves native far depth");
         }
         camera.nativeDepth[3]=0;check(RenderSceneSetCamera(&camera),"disable depth bridge after use");
         placement.visible=1;check(RenderSceneUpdateInstance(receiverInstance,&placement),"restore receiver after depth bridge");
         for(uint32_t slot=0;slot<2;++slot){compositePixel(slot);check(compositeDepth==1,"disabled depth bridge leaves native depth unchanged");}
         std::puts("PASS native linear/reciprocal depth composition, empty coverage and disabling both slots");
      }else std::puts("SKIP native depth bridge: R32_SFLOAT sampling/color blending unavailable");
      // Keep the renderer alive across scene resets, as during mission changes.
      // Cached material indices/textures must refer to the replacement scene
      // in each frame slot, even when its material count is unchanged.
      for(float emission:{.25f,.75f}) {
         RenderSceneClear();
         RenderSceneMaterial freshMaterial{};freshMaterial.baseColor[3]=1;
         freshMaterial.emission[0]=emission;freshMaterial.roughness=.8f;freshMaterial.doubleSided=1;
         const auto freshMaterialId=RenderSceneCreateMaterial(&freshMaterial);
         check(freshMaterialId!=0,"replacement scene material");
         RenderSceneMesh freshMesh{receiverVertices,3,indices,3,freshMaterialId,0};
         const auto freshMeshId=RenderSceneCreateMesh(&freshMesh);check(freshMeshId!=0,"replacement scene geometry");
         RenderSceneInstance freshInstance{};freshInstance.mesh=freshMeshId;freshInstance.visible=1;
         for(unsigned axis=0;axis<4;++axis)freshInstance.transform[axis*5]=freshInstance.previousTransform[axis*5]=1;
         check(RenderSceneCreateInstance(&freshInstance)!=0,"replacement scene instance");
         auto freshCamera=camera;freshCamera.ambient=0;freshCamera.nativeDepth[3]=0;
         check(RenderSceneSetCamera(&freshCamera),"replacement scene camera");
         for(uint32_t slot=0;slot<2;++slot)
            check(std::abs(sample(slot)-emission)<.001f,"material cache refreshes after scene reset in both slots");
      }
      RenderSceneClear();
      auto sceneMaterial=material;sceneMaterial.baseColorTexture=0;sceneMaterial.opacity=RenderSceneOpaque;
      const auto sceneMaterialId=RenderSceneCreateMaterial(&sceneMaterial);check(sceneMaterialId!=0,"scene switch material");
      RenderSceneMesh sceneReceiver{receiverVertices,3,indices,3,sceneMaterialId,0};
      const auto sceneReceiverId=RenderSceneCreateMesh(&sceneReceiver);check(sceneReceiverId!=0,"scene switch receiver");
      // Unused anchors keep identical valid bounds after moving the blocker.
      RenderSceneVertex sceneBlockerVertices[5]={
         {{.5f,-.5f,-1},{0,0,-1},{0,0}},{{1.5f,-.5f,-1},{0,0,-1},{1,0}},{{1,.5f,-1},{0,0,-1},{.5f,1}},
         {{-20,-20,-20},{0,0,-1},{0,0}},{{20,20,20},{0,0,-1},{0,0}}};
      RenderSceneMesh sceneBlocker{sceneBlockerVertices,5,indices,3,sceneMaterialId,0};
      const auto sceneBlockerId=RenderSceneCreateMesh(&sceneBlocker);check(sceneBlockerId!=0,"scene switch blocker");
      RenderSceneInstance scenePlacement{};scenePlacement.mesh=sceneReceiverId;scenePlacement.visible=1;
      for(unsigned axis=0;axis<4;++axis)scenePlacement.transform[axis*5]=scenePlacement.previousTransform[axis*5]=1;
      check(RenderSceneCreateInstance(&scenePlacement)!=0,"scene switch receiver placement");
      scenePlacement.mesh=sceneBlockerId;scenePlacement.visible=0;scenePlacement.castsShadow=1;
      check(RenderSceneCreateInstance(&scenePlacement)!=0,"scene switch blocker placement");
      auto sceneLight=pointLight;sceneLight.position[0]=2;sceneLight.position[1]=0;sceneLight.position[2]=-2;
      const auto sceneLightId=RenderSceneCreateLight(&sceneLight);check(sceneLightId!=0,"scene switch light");
      auto sceneCamera=camera;sceneCamera.ambient=.01f;sceneCamera.nativeDepth[3]=0;
      check(RenderSceneSetCamera(&sceneCamera),"scene switch camera");
      for(unsigned type:{uint32_t(RenderScenePointLight),uint32_t(RenderSceneSpotLight),uint32_t(RenderSceneDirectionalLight)})for(bool cached:{false,true}) {
         if(type==RenderSceneDirectionalLight && cached)continue;
         sceneLight.type=type;sceneLight.direction[0]=-float(std::sqrt(.5));sceneLight.direction[2]=float(std::sqrt(.5));
         sceneLight.innerCos=.9f;sceneLight.outerCos=.5f;
         check(RenderSceneUpdateLight(sceneLightId,&sceneLight),"scene switch light type");
         auto otherScene=dark::GetWorldScene();
         for(unsigned i=0;i<3;++i)otherScene.meshes.at(sceneBlockerId).vertices[i].position[0]+=10;
         for(bool staticDepth:{false,true}) {
            dark::ShadowSettings settings;settings.staticDepthCache=staticDepth;
            if(cached) {settings.localLights=settings.localFaces=0;settings.cachedLights=1;settings.cachedFaces=6;settings.cachedResolution=128;}
            renderer.SetShadowSettings(settings);
            renderScene=&dark::GetWorldScene();
            for(unsigned warm=0;warm<6;++warm)for(uint32_t slot=0;slot<2;++slot)
               check(sample(slot)<.006f,"warm source scene shadow");
            const auto queuedShadow=renderer.Record(1,dark::GetWorldScene(),64,64,error);
            check(queuedShadow!=VK_NULL_HANDLE,error.c_str());
            renderScene=&otherScene;
            check(sample(0)>.1f,"scene identity refreshes retained shadow depth");
            submit(queuedShadow);check(readSample(1)<.006f,"queued frame retains its original scene shadow depth");
            check(sample(1)>.1f,"second slot refreshes retained shadow depth");
            renderScene=&dark::GetWorldScene();
            for(uint32_t slot=0;slot<2;++slot)check(sample(slot)<.006f,"returning scene restores original shadow");
         }
         std::printf("PASS scene shadow identity type=%u cached=%d with stable-depth off/on and queued frame isolation\n",type,int(cached));
      }
      // Matching texture handles/revisions in another scene must not reuse
      // the previous scene's image. Keep geometry and camera identical.
      RenderSceneClear();const uint8_t whiteTexture[]={255,255,255,255};
      const auto sceneTexture=RenderSceneCreateTexture(1,1,whiteTexture,4);
      RenderSceneMaterial textureMaterial{};textureMaterial.baseColorTexture=sceneTexture;
      textureMaterial.baseColor[3]=1;textureMaterial.emission[0]=1;textureMaterial.emissionUsesBaseTexture=1;textureMaterial.doubleSided=1;
      const auto textureMaterialId=RenderSceneCreateMaterial(&textureMaterial);check(textureMaterialId!=0,"scene texture material");
      sceneReceiver.material=textureMaterialId;scenePlacement.mesh=RenderSceneCreateMesh(&sceneReceiver);
      scenePlacement.visible=1;scenePlacement.castsShadow=0;
      check(RenderSceneCreateInstance(&scenePlacement)!=0,"scene texture receiver");
      sceneCamera.ambient=0;check(RenderSceneSetCamera(&sceneCamera),"scene texture camera");
      auto otherTextureScene=dark::GetWorldScene();otherTextureScene.textures.at(sceneTexture).rgba[0]=0;
      renderScene=&dark::GetWorldScene();for(uint32_t slot=0;slot<2;++slot)check(sample(slot)>.99f,"source scene texture white");
      const auto queuedScene=renderer.Record(1,dark::GetWorldScene(),64,64,error);
      check(queuedScene!=VK_NULL_HANDLE,error.c_str());
      renderScene=&otherTextureScene;check(sample(0)<.001f,"scene identity refreshes texture image");
      submit(queuedScene);check(readSample(1)>.99f,"queued frame retains its original scene texture image");
      check(sample(1)<.001f,"second frame slot receives replacement scene texture");
      renderScene=&dark::GetWorldScene();for(uint32_t slot=0;slot<2;++slot)check(sample(slot)>.99f,"returning scene restores texture image");
      std::puts("PASS scene identity invalidation of working/static shadows and textures, queued frame isolation and return transitions");
      renderer.Shutdown();device->unmapMemory(*allocation);output.reset();RenderSceneClear();return 0;
   }
   std::puts("SKIP: no texture-indexing capable Vulkan device");return 77;
 }catch(const std::exception &error){std::fprintf(stderr,"World lighting regression failed: %s\n",error.what());return 1;}
}
