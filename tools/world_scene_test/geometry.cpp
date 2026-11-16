#include "../../libsrc/lgd3d/vulkan_world_geometry.hpp"
#include "../../libsrc/lgd3d/vulkan_world_features.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <cstdio>
#include <cstring>
#include <stdexcept>
static void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
int main() {
 try {
   vk::raii::Context context;vk::ApplicationInfo app("Raster world geometry regression",1,"Dark",1,VK_API_VERSION_1_1);
   auto instance=context.createInstance(vk::InstanceCreateInfo({},&app));
   for(const auto physical:(*instance).enumeratePhysicalDevices()) {
      dark::VulkanWorldFeatures features;if(!features.Probe(physical))continue;
      const auto queues=physical.getQueueFamilyProperties();uint32_t family=0;
      while(family<queues.size() && !(queues[family].queueFlags&vk::QueueFlagBits::eGraphics))++family;
      if(family==queues.size())continue;
      check(features.extensions.size()==1 && std::strcmp(features.extensions[0],VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)==0,"only texture-indexing extension requested");
      const vk::PhysicalDeviceFeatures disabled{};
      auto requested=features.features.features;
      check(requested.multiDrawIndirect==requested.drawIndirectFirstInstance,"indirect feature pair enabled together");
      requested.multiDrawIndirect=requested.drawIndirectFirstInstance=VK_FALSE;
      check(std::memcmp(&requested,&disabled,sizeof(disabled))==0,"no other core features including shaderInt64 requested");
      check(features.features.pNext==&features.descriptors && !features.descriptors.pNext,"no RT or address feature chain");
      const float priority=1;vk::DeviceQueueCreateInfo queueInfo({},family,1,&priority);
      vk::DeviceCreateInfo info({},1,&queueInfo,0,nullptr,uint32_t(features.extensions.size()),features.extensions.data());info.pNext=&features.features;
      auto device=physical.createDeviceUnique(info);auto queue=device->getQueue(family,0);
      dark::VulkanWorldGeometry geometry;check(geometry.Initialize(physical,*device),"raster geometry initialization");
      RenderSceneClear();RenderSceneMaterial material{};for(float &value:material.baseColor)value=1;
      auto materialId=RenderSceneCreateMaterial(&material);check(materialId!=0,"material");
      RenderSceneVertex vertices[3]={{{-1,-1,0},{0,0,1},{0,0}},{{1,-1,0},{0,0,1},{1,0}},{{0,1,0},{0,0,1},{0,1}}};
      uint32_t indices[]={0,1,2};RenderSceneMesh mesh{vertices,3,indices,3,materialId,1};
      auto meshId=RenderSceneCreateMesh(&mesh);check(meshId!=0,"mesh");std::string error;
      auto pool=device->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,family));
      auto commands=device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(*pool,vk::CommandBufferLevel::ePrimary,1));
      const size_t bytes=sizeof(vertices)+sizeof(indices);
      vk::UniqueDeviceMemory allocation;
      auto output=device->createBufferUnique(vk::BufferCreateInfo({},bytes,vk::BufferUsageFlagBits::eTransferDst,vk::SharingMode::eExclusive));
      auto requirements=device->getBufferMemoryRequirements(*output);auto memory=physical.getMemoryProperties();uint32_t type=0;
      const auto wanted=vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent;
      while(type<memory.memoryTypeCount && (!(requirements.memoryTypeBits&(1u<<type)) || (memory.memoryTypes[type].propertyFlags&wanted)!=wanted))++type;
      check(type<memory.memoryTypeCount,"readback memory");allocation=device->allocateMemoryUnique(vk::MemoryAllocateInfo(requirements.size,type));
      device->bindBufferMemory(*output,*allocation,0);const auto *mapped=static_cast<const unsigned char *>(device->mapMemory(*allocation,0,bytes));
      auto read=[&](uint32_t slot,const RenderSceneVertex *expected) {
         VkBuffer vb{},ib{};VkDeviceSize vo{},io{};check(geometry.MeshBuffers(slot,meshId,vb,ib,&vo,&io),"raster buffers available");
         auto command=*commands[0];command.reset();command.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
         command.copyBuffer(vb,*output,vk::BufferCopy(vo,0,sizeof(vertices)));
         command.copyBuffer(ib,*output,vk::BufferCopy(io,sizeof(vertices),sizeof(indices)));
         vk::MemoryBarrier barrier(vk::AccessFlagBits::eTransferWrite,vk::AccessFlagBits::eHostRead);
         command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,vk::PipelineStageFlagBits::eHost,{},barrier,{},{});command.end();
         auto fence=device->createFenceUnique({});vk::SubmitInfo submit;submit.commandBufferCount=1;submit.pCommandBuffers=&command;queue.submit(submit,*fence);
         check(device->waitForFences(*fence,VK_TRUE,UINT64_MAX)==vk::Result::eSuccess,"readback submission");
         check(std::memcmp(mapped,expected,sizeof(vertices))==0,"exact GPU vertex contents");
         check(std::memcmp(mapped+sizeof(vertices),indices,sizeof(indices))==0,"exact GPU index contents");
      };
      check(geometry.Update(0,dark::GetWorldScene(),error),error.c_str());check(geometry.Update(1,dark::GetWorldScene(),error),error.c_str());
      read(0,vertices);read(1,vertices);const auto original=std::array<RenderSceneVertex,3>{vertices[0],vertices[1],vertices[2]};
      VkBuffer firstV{},firstI{};check(geometry.MeshBuffers(0,meshId,firstV,firstI),"initial buffers");
      vertices[1].position[0]=7;check(RenderSceneUpdateVertices(meshId,vertices,3),"pose update");
      check(geometry.Update(0,dark::GetWorldScene(),error),error.c_str());read(0,vertices);read(1,original.data());
      VkBuffer nextV{},nextI{};check(geometry.MeshBuffers(0,meshId,nextV,nextI) && firstV==nextV && firstI==nextI,"deformation reuses buffer allocations");
      check(geometry.Update(1,dark::GetWorldScene(),error),error.c_str());read(1,vertices);
      const auto extra=RenderSceneCreateMesh(&mesh);check(extra!=0,"incremental mesh creation");
      check(geometry.Update(0,dark::GetWorldScene(),error),error.c_str());
      check(geometry.MeshCount(0)==2 && geometry.MeshCount(1)==1,"new geometry appears only in retired slot");
      check(RenderSceneDestroy(extra),"incremental mesh destruction");
      for(uint32_t slot=0;slot<2;++slot) {
         check(geometry.Update(slot,dark::GetWorldScene(),error),error.c_str());
         check(geometry.MeshCount(slot)==1 && !geometry.MeshBuffers(slot,extra,nextV,nextI),"removed geometry cannot survive journal replay");
      }
      const auto beforeOverflow=std::array<RenderSceneVertex,3>{vertices[0],vertices[1],vertices[2]};
      const auto forgotten=RenderSceneCreateMesh(&mesh);check(forgotten!=0,"overflow retirement fixture");
      for(uint32_t slot=0;slot<2;++slot)check(geometry.Update(slot,dark::GetWorldScene(),error),error.c_str());
      check(RenderSceneDestroy(forgotten),"deletion that will fall out of bounded history");
      for(size_t i=0;i<dark::WorldScene::meshChangeCapacity+3;++i) {
         vertices[0].position[1]=float(i%11);
         check(RenderSceneUpdateVertices(meshId,vertices,3),"journal overflow pose update");
      }
      check(geometry.Update(0,dark::GetWorldScene(),error),error.c_str());read(0,vertices);read(1,beforeOverflow.data());
      check(geometry.MeshCount(0)==1 && geometry.MeshCount(1)==2,"overflow reconciliation retires forgotten deletion per slot");
      check(!geometry.MeshBuffers(0,forgotten,nextV,nextI),"overwritten deletion cannot retain stale GPU mesh");
      check(geometry.Update(1,dark::GetWorldScene(),error),error.c_str());read(1,vertices);
      check(geometry.MeshCount(1)==1 && !geometry.MeshBuffers(1,forgotten,nextV,nextI),"second slot reconciles overwritten deletion");
      auto alternate=dark::GetWorldScene();
      auto alternateVertices=std::array<RenderSceneVertex,3>{vertices[0],vertices[1],vertices[2]};
      alternateVertices[0].position[0]+=3;
      alternate.meshes.at(meshId).vertices.assign(alternateVertices.begin(),alternateVertices.end());
      check(geometry.Update(0,alternate,error),error.c_str());read(0,alternateVertices.data());read(1,vertices);
      check(geometry.Update(0,dark::GetWorldScene(),error),error.c_str());read(0,vertices);
      check(!geometry.Update(2,dark::GetWorldScene(),error) && !error.empty(),"invalid frame slot rejected");
      RenderSceneClear();check(geometry.Update(0,dark::GetWorldScene(),error),error.c_str());check(geometry.MeshCount(0)==0 && geometry.MeshCount(1)==1,"slot retirement releases stale geometry independently");
      check(!geometry.MeshBuffers(0,meshId,nextV,nextI),"stale handle unavailable");
      check(geometry.Update(1,dark::GetWorldScene(),error),error.c_str());check(geometry.MeshCount(1)==0,"empty scene releases second slot");
      geometry.Shutdown();device->unmapMemory(*allocation);
      std::printf("PASS on %s: raster-only device features, GPU vertex/index readback, independent frame slots, reusable pose buffers, bounded change history, creation/removal and reset\n",physical.getProperties().deviceName.data());return 0;
   }
   std::puts("SKIP: sampled texture indexing unavailable");return 77;
 }catch(const std::exception &failure){std::fprintf(stderr,"Raster geometry regression failed: %s\n",failure.what());return 1;}
}
