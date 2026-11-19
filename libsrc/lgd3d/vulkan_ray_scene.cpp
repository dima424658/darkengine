#include "vulkan_ray_scene.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <stdexcept>
#include <vector>
namespace dark {
namespace {
void checked(VkResult result,const char *what) {if(result!=VK_SUCCESS)throw std::runtime_error(what);}
struct Buffer {
   vk::Device device;
   vk::UniqueDeviceMemory memory;
   vk::UniqueBuffer buffer;
   void *mapped=nullptr;
   VkDeviceSize capacity=0,address=0;
   ~Buffer(){if(mapped)device.unmapMemory(*memory);}
};
struct Acceleration {
   VkDevice device{};
   PFN_vkDestroyAccelerationStructureKHR destroy{};
   std::unique_ptr<Buffer> storage,scratch;
   VkAccelerationStructureKHR handle{};
   VkDeviceAddress address=0;
   uint32_t primitiveCapacity=0,maxVertex=0;
   VkAccelerationStructureTypeKHR type{};
   ~Acceleration(){if(handle)destroy(device,handle,nullptr);}
};
struct Triangle {float positionU[3][4],normalV[3][4];};
static_assert(sizeof(Triangle)==96,"ray triangle ABI");
struct InstanceData {uint32_t triangle,material;int32_t object;uint32_t flags;};
struct MeshCache {
   uint64_t revision=0;
   std::unique_ptr<Buffer> vertices,indices;
   std::shared_ptr<Acceleration> blas;
};
struct Snapshot {
   std::shared_ptr<Acceleration> tlas;
   std::shared_ptr<Buffer> triangles,instances;
   std::vector<std::shared_ptr<Acceleration>> meshes;
};
struct Frame {
   const WorldScene *scene=nullptr;
   uint64_t sceneReset=0;
   std::map<RenderSceneHandle,MeshCache> meshes;
   std::unique_ptr<Buffer> input;
   std::shared_ptr<Buffer> triangles,instances;
   std::shared_ptr<Acceleration> tlas;
   std::shared_ptr<Snapshot> snapshot,previousSnapshot;
   std::vector<VkAccelerationStructureInstanceKHR> previous;
   std::vector<InstanceData> previousMetadata;
   std::vector<Triangle> packedTriangles;
   std::vector<RenderSceneHandle> metadataMeshes;
   std::map<RenderSceneHandle,uint32_t> offsets;
   std::map<RenderSceneHandle,size_t> metadataCounts;
   VulkanRayScene::Workload stats;
};
}
struct VulkanRayScene::Impl {
   vk::Device device;
   vk::PhysicalDeviceMemoryProperties memory;
   VkPhysicalDeviceAccelerationStructurePropertiesKHR properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
   PFN_vkCreateAccelerationStructureKHR create{};
   PFN_vkDestroyAccelerationStructureKHR destroy{};
   PFN_vkGetAccelerationStructureBuildSizesKHR sizes{};
   PFN_vkCmdBuildAccelerationStructuresKHR build{};
   PFN_vkGetAccelerationStructureDeviceAddressKHR asAddress{};
   PFN_vkGetBufferDeviceAddressKHR bufferAddress{};
   std::array<Frame,2> frames;
   std::shared_ptr<Snapshot> acceptedSnapshot;
   int pendingSlot=-1;
   std::vector<std::shared_ptr<Acceleration>> accelerationCache;
   std::vector<std::shared_ptr<Buffer>> metadataCache;
   uint32_t memoryType(uint32_t bits,vk::MemoryPropertyFlags wanted) {
      for(uint32_t i=0;i<memory.memoryTypeCount;++i)
         if((bits&(1u<<i)) && (memory.memoryTypes[i].propertyFlags&wanted)==wanted)return i;
      throw std::runtime_error("no ray scene memory type");
   }
   std::unique_ptr<Buffer> buffer(VkDeviceSize bytes,vk::BufferUsageFlags usage,bool host) {
      auto result=std::make_unique<Buffer>();result->device=device;result->capacity=std::max<VkDeviceSize>(bytes,256);
      usage|=vk::BufferUsageFlagBits::eShaderDeviceAddress;
      result->buffer=device.createBufferUnique(vk::BufferCreateInfo({},result->capacity,usage,vk::SharingMode::eExclusive));
      const auto requirements=device.getBufferMemoryRequirements(*result->buffer);
      const auto wanted=host?(vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent):vk::MemoryPropertyFlagBits::eDeviceLocal;
      vk::MemoryAllocateFlagsInfo flags(vk::MemoryAllocateFlagBits::eDeviceAddress);
      vk::MemoryAllocateInfo allocation(requirements.size,memoryType(requirements.memoryTypeBits,wanted));allocation.pNext=&flags;
      result->memory=device.allocateMemoryUnique(allocation);device.bindBufferMemory(*result->buffer,*result->memory,0);
      if(host)result->mapped=device.mapMemory(*result->memory,0,VK_WHOLE_SIZE);
      VkBufferDeviceAddressInfo info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};info.buffer=*result->buffer;
      result->address=bufferAddress(device,&info);
      return result;
   }
   void upload(std::unique_ptr<Buffer> &target,const void *data,size_t bytes,vk::BufferUsageFlags usage) {
      if(!target || target->capacity<std::max<size_t>(bytes,1))target=buffer(bytes,usage,true);
      if(bytes)std::memcpy(target->mapped,data,bytes);
   }
   void upload(std::shared_ptr<Buffer> &target,const void *data,size_t bytes,vk::BufferUsageFlags usage) {
      // Compare/update CPU mirrors before calling. Reading host-visible device
      // memory to compare or clone it can be much slower than uploading it.
      if(!target || target.use_count()>1 || target->capacity<std::max<size_t>(bytes,1)) {
         if(target)metadataCache.push_back(std::move(target));
         target=metadataBuffer(bytes);
      }
      if(bytes)std::memcpy(target->mapped,data,bytes);
   }
   std::shared_ptr<Buffer> metadataBuffer(size_t bytes) {
      auto best=metadataCache.end();
      for(auto it=metadataCache.begin();it!=metadataCache.end();++it)
         if(it->use_count()==1 && (*it)->capacity>=std::max<size_t>(bytes,1) &&
            (best==metadataCache.end() || (*it)->capacity<(*best)->capacity))best=it;
      if(best==metadataCache.end())return buffer(bytes,vk::BufferUsageFlagBits::eStorageBuffer,true);
      auto result=std::move(*best);metadataCache.erase(best);return result;
   }
   void replaceAcceleration(std::shared_ptr<Acceleration> &target,VkAccelerationStructureTypeKHR type,
      const VkAccelerationStructureGeometryKHR &geometry,uint32_t count) {
      if(target)accelerationCache.push_back(std::move(target));
      auto best=accelerationCache.end();
      for(auto it=accelerationCache.begin();it!=accelerationCache.end();++it)
         if(it->use_count()==1 && (*it)->type==type && (*it)->primitiveCapacity>=count &&
            (type!=VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR || (*it)->maxVertex>=geometry.geometry.triangles.maxVertex) &&
            (best==accelerationCache.end() || (*it)->primitiveCapacity<(*best)->primitiveCapacity))best=it;
      if(best==accelerationCache.end())target=acceleration(type,geometry,count);
      else {target=std::move(*best);accelerationCache.erase(best);}
   }
   void trimCache() {
      // Snapshots keep in-flight resources shared. Only unreferenced idle
      // allocations count against this limit and may be destroyed/recycled.
      constexpr uint64_t limit=64ull*1024*1024;
      uint64_t idle=0;
      for(const auto &a:accelerationCache)if(a.use_count()==1)idle+=a->storage->capacity+a->scratch->capacity;
      for(const auto &b:metadataCache)if(b.use_count()==1)idle+=b->capacity;
      for(auto it=accelerationCache.begin();idle>limit && it!=accelerationCache.end();)
         if(it->use_count()==1){idle-=(*it)->storage->capacity+(*it)->scratch->capacity;it=accelerationCache.erase(it);}else ++it;
      for(auto it=metadataCache.begin();idle>limit && it!=metadataCache.end();)
         if(it->use_count()==1){idle-=(*it)->capacity;it=metadataCache.erase(it);}else ++it;
   }
   std::unique_ptr<Acceleration> acceleration(VkAccelerationStructureTypeKHR type,
      const VkAccelerationStructureGeometryKHR &geometry,uint32_t count) {
      VkAccelerationStructureBuildGeometryInfoKHR info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
      info.type=type;info.flags=VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
      info.geometryCount=1;info.pGeometries=&geometry;
      VkAccelerationStructureBuildSizesInfoKHR size{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
      sizes(device,VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,&info,&count,&size);
      auto result=std::make_unique<Acceleration>();result->device=device;result->destroy=destroy;result->primitiveCapacity=count;result->type=type;
      if(type==VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR)result->maxVertex=geometry.geometry.triangles.maxVertex;
      result->storage=buffer(size.accelerationStructureSize,vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR,false);
      result->scratch=buffer(size.buildScratchSize+properties.minAccelerationStructureScratchOffsetAlignment,vk::BufferUsageFlagBits::eStorageBuffer,false);
      VkAccelerationStructureCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
      createInfo.buffer=*result->storage->buffer;createInfo.size=size.accelerationStructureSize;createInfo.type=type;
      checked(create(device,&createInfo,nullptr,&result->handle),"create acceleration structure");
      VkAccelerationStructureDeviceAddressInfoKHR addressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};addressInfo.accelerationStructure=result->handle;
      result->address=asAddress(device,&addressInfo);return result;
   }
   void record(vk::CommandBuffer command,Acceleration &target,VkAccelerationStructureTypeKHR type,
      const VkAccelerationStructureGeometryKHR &geometry,uint32_t count) {
      VkAccelerationStructureBuildGeometryInfoKHR info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
      info.type=type;info.flags=VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
      info.mode=VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;info.dstAccelerationStructure=target.handle;
      info.geometryCount=1;info.pGeometries=&geometry;
      const auto alignment=properties.minAccelerationStructureScratchOffsetAlignment;
      info.scratchData.deviceAddress=(target.scratch->address+alignment-1)&~VkDeviceAddress(alignment-1);
      VkAccelerationStructureBuildRangeInfoKHR range{};range.primitiveCount=count;const auto *ranges=&range;
      build(command,1,&info,&ranges);
   }
   void barrier(vk::CommandBuffer command,vk::PipelineStageFlags destination,vk::AccessFlags access) {
      const vk::MemoryBarrier barrier(vk::AccessFlagBits::eAccelerationStructureWriteKHR,access);
      command.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,destination,{},barrier,{},{});
   }
};
VulkanRayScene::VulkanRayScene():m(std::make_unique<Impl>()){}
VulkanRayScene::~VulkanRayScene()=default;
bool VulkanRayScene::Initialize(vk::PhysicalDevice physical,vk::Device device,std::string &error) {
 try {
   m->device=device;m->memory=physical.getMemoryProperties();
   VkPhysicalDeviceProperties2 properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};properties.pNext=&m->properties;
   vkGetPhysicalDeviceProperties2(physical,&properties);
#define LOAD(member,name) m->member=reinterpret_cast<PFN_##name>(vkGetDeviceProcAddr(device,#name));if(!m->member)throw std::runtime_error("missing " #name)
   LOAD(create,vkCreateAccelerationStructureKHR);LOAD(destroy,vkDestroyAccelerationStructureKHR);
   LOAD(sizes,vkGetAccelerationStructureBuildSizesKHR);LOAD(build,vkCmdBuildAccelerationStructuresKHR);
   LOAD(asAddress,vkGetAccelerationStructureDeviceAddressKHR);LOAD(bufferAddress,vkGetBufferDeviceAddressKHR);
#undef LOAD
   return true;
 }catch(const std::exception &failure){error=failure.what();return false;}
}
bool VulkanRayScene::Record(vk::CommandBuffer command,uint32_t slot,const WorldScene &scene,
   const std::unordered_map<uint64_t,uint32_t> &materials,std::string &error) {
 try {
   auto &frame=m->frames.at(slot);frame.stats={};
   if(frame.scene!=&scene || frame.sceneReset!=scene.resetRevision){frame=Frame{};frame.scene=&scene;frame.sceneReset=scene.resetRevision;}
   frame.snapshot.reset();
   frame.previousSnapshot=m->acceptedSnapshot;
   std::vector<RenderSceneHandle> instanceIds,meshIds;
   for(const auto &entry:scene.instances)if(entry.second.castsShadow || entry.second.visible) {
      const auto mesh=scene.meshes.find(entry.second.mesh);
      if(mesh!=scene.meshes.end() && !mesh->second.indices.empty())instanceIds.push_back(entry.first);
   }
   std::sort(instanceIds.begin(),instanceIds.end());
   for(const auto id:instanceIds)meshIds.push_back(scene.instances.at(id).mesh);
   std::sort(meshIds.begin(),meshIds.end());meshIds.erase(std::unique(meshIds.begin(),meshIds.end()),meshIds.end());
   for(auto it=frame.meshes.begin();it!=frame.meshes.end();)if(!std::binary_search(meshIds.begin(),meshIds.end(),it->first))it=frame.meshes.erase(it);else ++it;
   bool changed=false;
   bool triangleChanges=false;
   std::vector<Triangle> triangles;
   bool pack=!frame.triangles || frame.metadataMeshes!=meshIds;
   for(const auto id:meshIds)if(frame.metadataCounts[id]!=scene.meshes.at(id).indices.size())pack=true;
   auto &offsets=frame.offsets;
   if(pack){offsets.clear();frame.metadataCounts.clear();}
   const auto input=vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR;
   for(const auto id:meshIds) {
      const auto &mesh=scene.meshes.at(id);auto &cached=frame.meshes[id];
      if(!pack)triangles.clear();
      if(pack || cached.revision!=mesh.revision) {
      if(triangles.size()>0xffffffu)throw std::runtime_error("ray scene triangle offset overflow");
      if(pack)offsets[id]=uint32_t(triangles.size());
      frame.metadataCounts[id]=mesh.indices.size();
      for(size_t i=0;i<mesh.indices.size();i+=3) {
         Triangle triangle{};
         for(unsigned j=0;j<3;++j) {
            const auto &vertex=mesh.vertices.at(mesh.indices.at(i+j));
            std::copy(vertex.position,vertex.position+3,triangle.positionU[j]);triangle.positionU[j][3]=vertex.uv[0];
            std::copy(vertex.normal,vertex.normal+3,triangle.normalV[j]);triangle.normalV[j][3]=vertex.uv[1];
         }
         triangles.push_back(triangle);
      }
      }
      if(!pack && !triangles.empty()) {
         std::copy(triangles.begin(),triangles.end(),frame.packedTriangles.begin()+offsets.at(id));
         triangleChanges=true;
      }
      if(!cached.blas || cached.revision!=mesh.revision) {
         // Reuse this retired slot's build inputs. Keep snapshot-owned AS
         // storage immutable and recycle free allocations when capacity fits.
         m->upload(cached.vertices,mesh.vertices.data(),mesh.vertices.size()*sizeof(RenderSceneVertex),input);
         m->upload(cached.indices,mesh.indices.data(),mesh.indices.size()*4,input);
         VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
         geometry.geometryType=VK_GEOMETRY_TYPE_TRIANGLES_KHR;
         auto &data=geometry.geometry.triangles;data.sType=VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
         data.vertexFormat=VK_FORMAT_R32G32B32_SFLOAT;data.vertexData.deviceAddress=cached.vertices->address;
         data.vertexStride=sizeof(RenderSceneVertex);data.maxVertex=uint32_t(mesh.vertices.size()-1);
         data.indexType=VK_INDEX_TYPE_UINT32;data.indexData.deviceAddress=cached.indices->address;
         if(!cached.blas || cached.blas.use_count()>1 || cached.blas->primitiveCapacity<mesh.indices.size()/3 || cached.blas->maxVertex<data.maxVertex)m->replaceAcceleration(cached.blas,VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,geometry,uint32_t(mesh.indices.size()/3));
         m->record(command,*cached.blas,VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,geometry,uint32_t(mesh.indices.size()/3));
         cached.revision=mesh.revision;changed=true;++frame.stats.blasBuilds;
      }
   }
   // Instances reference per-mesh metadata, so alpha tests and emitter exclusion
   // remain correct for shared meshes with different poses/object identities.
   std::vector<VkAccelerationStructureInstanceKHR> instances;
   std::vector<InstanceData> metadata;
   for(const auto id:instanceIds) {
      const auto &source=scene.instances.at(id);const auto &mesh=scene.meshes.at(source.mesh);
      const auto &material=scene.materials.at(mesh.material);
      VkAccelerationStructureInstanceKHR instance{};
      for(unsigned row=0;row<3;++row)for(unsigned col=0;col<4;++col)instance.transform.matrix[row][col]=source.transform[col*4+row];
      if(instances.size()>0xffffffu)throw std::runtime_error("ray scene instance offset overflow");
      instance.instanceCustomIndex=uint32_t(instances.size());instance.mask=(source.castsShadow?1:0)|2;
      // Keep BLAS geometry reusable across material changes. Opaque instances
      // can commit hits in hardware; cutouts and emitter glass still reach the
      // candidate shader for coverage and per-emitter exclusion.
      instance.flags=material.doubleSided?VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR:0;
      if(material.opacity==RenderSceneOpaque && !material.emitterSurface)
         instance.flags|=VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
      instance.accelerationStructureReference=frame.meshes.at(source.mesh).blas->address;instances.push_back(instance);
      metadata.push_back({offsets.at(source.mesh),materials.at(mesh.material),source.objectId,material.emitterSurface});
   }
   if(pack) {
      frame.packedTriangles=std::move(triangles);
      triangleChanges=true;
      frame.metadataMeshes=meshIds;
   }
   if(triangleChanges)m->upload(frame.triangles,frame.packedTriangles.data(),frame.packedTriangles.size()*sizeof(Triangle),vk::BufferUsageFlagBits::eStorageBuffer);
   if(!frame.instances || metadata.size()!=frame.previousMetadata.size() ||
      (!metadata.empty() && std::memcmp(metadata.data(),frame.previousMetadata.data(),metadata.size()*sizeof(InstanceData)))) {
      m->upload(frame.instances,metadata.data(),metadata.size()*sizeof(InstanceData),vk::BufferUsageFlagBits::eStorageBuffer);
      frame.previousMetadata=metadata;
   }
   if(changed || !frame.tlas || instances.size()!=frame.previous.size() ||
      (!instances.empty() && std::memcmp(instances.data(),frame.previous.data(),instances.size()*sizeof(instances[0])))) {
      if(changed)m->barrier(command,vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,vk::AccessFlagBits::eAccelerationStructureReadKHR);
      m->upload(frame.input,instances.data(),instances.size()*sizeof(instances[0]),input);
      VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};geometry.geometryType=VK_GEOMETRY_TYPE_INSTANCES_KHR;
      geometry.geometry.instances.sType=VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
      geometry.geometry.instances.data.deviceAddress=frame.input->address;
      if(!frame.tlas || frame.tlas.use_count()>1 || frame.tlas->primitiveCapacity<instances.size())m->replaceAcceleration(frame.tlas,VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,geometry,uint32_t(instances.size()));
      m->record(command,*frame.tlas,VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,geometry,uint32_t(instances.size()));
      frame.previous=instances;++frame.stats.tlasBuilds;
      m->barrier(command,vk::PipelineStageFlagBits::eFragmentShader|vk::PipelineStageFlagBits::eComputeShader,vk::AccessFlagBits::eAccelerationStructureReadKHR);
   }
   frame.stats.instances=uint32_t(instances.size());
   auto bytes=[](const auto &b){return b?b->capacity:0;};
   for(const auto &entry:frame.meshes) {
      const auto &c=entry.second;frame.stats.bytes+=bytes(c.vertices)+bytes(c.indices)+bytes(c.blas->storage)+bytes(c.blas->scratch);
   }
   frame.stats.bytes+=bytes(frame.input)+bytes(frame.triangles)+bytes(frame.instances)+bytes(frame.tlas->storage)+bytes(frame.tlas->scratch);
   frame.snapshot=std::make_shared<Snapshot>();
   frame.snapshot->tlas=frame.tlas;frame.snapshot->triangles=frame.triangles;frame.snapshot->instances=frame.instances;
   for(const auto &mesh:frame.meshes)frame.snapshot->meshes.push_back(mesh.second.blas);
   if(!frame.previousSnapshot)frame.previousSnapshot=frame.snapshot;
   m->pendingSlot=int(slot);
   m->trimCache();
   return true;
 }catch(const std::exception &failure){error=failure.what();return false;}
}
VkAccelerationStructureKHR VulkanRayScene::TopLevel(uint32_t slot) const {return m->frames.at(slot).tlas->handle;}
vk::DescriptorBufferInfo VulkanRayScene::Triangles(uint32_t slot) const {auto &b=m->frames.at(slot).triangles;return {*b->buffer,0,b->capacity};}
vk::DescriptorBufferInfo VulkanRayScene::Instances(uint32_t slot) const {auto &b=m->frames.at(slot).instances;return {*b->buffer,0,b->capacity};}
VulkanRayScene::Workload VulkanRayScene::Stats(uint32_t slot) const {return m->frames.at(slot).stats;}
VkAccelerationStructureKHR VulkanRayScene::PreviousTopLevel(uint32_t slot) const {return m->frames.at(slot).previousSnapshot->tlas->handle;}
vk::DescriptorBufferInfo VulkanRayScene::PreviousTriangles(uint32_t slot) const {auto &b=m->frames.at(slot).previousSnapshot->triangles;return {*b->buffer,0,b->capacity};}
vk::DescriptorBufferInfo VulkanRayScene::PreviousInstances(uint32_t slot) const {auto &b=m->frames.at(slot).previousSnapshot->instances;return {*b->buffer,0,b->capacity};}
void VulkanRayScene::CompleteSubmission(bool accepted) {
   if(accepted && m->pendingSlot>=0)m->acceptedSnapshot=m->frames.at(m->pendingSlot).snapshot;
   else if(!accepted)m->acceptedSnapshot.reset();
   m->pendingSlot=-1;
}
void VulkanRayScene::Invalidate(uint32_t slot){m->frames.at(slot)=Frame{};}
}
