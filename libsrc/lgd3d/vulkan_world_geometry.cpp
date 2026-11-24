#include "vulkan_world_geometry.hpp"
#include "geometry_ranges.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>
#include <tracy/Tracy.hpp>
namespace dark {
namespace {
void require(VkResult result,const char *operation) {
   if(result!=VK_SUCCESS)throw std::runtime_error(std::string(operation)+": Vulkan result "+std::to_string(result));
}
struct MemoryBlock : GeometryRanges {
   VkDevice device{};VkBuffer buffer{};VkDeviceMemory memory{};void *mapped{};uint32_t type{};
   ~MemoryBlock(){if(buffer)vkDestroyBuffer(device,buffer,nullptr);if(mapped)vkUnmapMemory(device,memory);if(memory)vkFreeMemory(device,memory,nullptr);}

};
struct Buffer {
   VkDevice device{};VkBuffer buffer{};
   std::shared_ptr<MemoryBlock> memory;
   VkDeviceSize size{},offset{},allocationSize{};void *mapped{};
   Buffer()=default;
   Buffer(const Buffer &)=delete;
   ~Buffer(){if(memory)memory->release(offset,allocationSize);}
};
struct Mesh {
   std::unique_ptr<Buffer> vertices,indices;
   uint64_t revision{};
};
struct Frame {
   std::unordered_map<RenderSceneHandle,std::unique_ptr<Mesh>> meshes;
   const WorldScene *scene=nullptr;uint64_t revision=0;
};
}
struct VulkanWorldGeometry::Impl {
   VkDevice device{};
   VkPhysicalDeviceMemoryProperties memory{};
   std::vector<std::weak_ptr<MemoryBlock>> blocks;
   std::array<Frame,2> frames;
   std::unique_ptr<Buffer> allocate(VkDeviceSize bytes,VkBufferUsageFlags usage) {
      auto b=std::make_unique<Buffer>();b->device=device;b->size=std::max<VkDeviceSize>(bytes,16);
      const VkDeviceSize alignment=(usage&VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)?sizeof(RenderSceneVertex):sizeof(uint32_t);
      for(auto it=blocks.begin();it!=blocks.end();) {
         auto block=it->lock();if(!block){it=blocks.erase(it);continue;}
         if(block->take(b->size,alignment,b->offset)){b->memory=block;break;}++it;
      }
      if(!b->memory) {
         auto block=std::make_shared<MemoryBlock>();block->device=device;
         VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};info.size=std::max<VkDeviceSize>(b->size,64ull*1024*1024);
         info.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_INDEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT;info.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
         require(vkCreateBuffer(device,&info,nullptr,&block->buffer),"create scene geometry arena");
         VkMemoryRequirements requirements;vkGetBufferMemoryRequirements(device,block->buffer,&requirements);
         const auto wanted=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
         uint32_t type=0;for(;type<memory.memoryTypeCount;++type)
            if((requirements.memoryTypeBits&(1u<<type)) && (memory.memoryTypes[type].propertyFlags&wanted)==wanted)break;
         if(type==memory.memoryTypeCount)throw std::runtime_error("no suitable memory type for world scene");
         VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirements.size;allocation.memoryTypeIndex=type;
         require(vkAllocateMemory(device,&allocation,nullptr,&block->memory),"allocate geometry arena");
         require(vkBindBufferMemory(device,block->buffer,block->memory,0),"bind geometry arena");
         require(vkMapMemory(device,block->memory,0,VK_WHOLE_SIZE,0,&block->mapped),"map geometry arena");
         block->free.push_back({0,info.size});
         if(!block->take(b->size,alignment,b->offset))throw std::runtime_error("geometry arena suballocation failed");
         b->memory=block;blocks.push_back(block);
      }
      b->buffer=b->memory->buffer;b->allocationSize=b->size;
      b->mapped=static_cast<unsigned char *>(b->memory->mapped)+b->offset;
      return b;
   }
};
VulkanWorldGeometry::VulkanWorldGeometry():m(std::make_unique<Impl>()){}
VulkanWorldGeometry::~VulkanWorldGeometry()=default;
bool VulkanWorldGeometry::Initialize(VkPhysicalDevice physical,VkDevice device) {
   if(m->device || !physical || !device)return false;
   m->device=device;vkGetPhysicalDeviceMemoryProperties(physical,&m->memory);return true;
}
void VulkanWorldGeometry::Shutdown(){m=std::make_unique<Impl>();}
bool VulkanWorldGeometry::Update(uint32_t slot,const WorldScene &scene,std::string &error) {
   ZoneScopedN("Update raster world geometry");
   error.clear();
   if(!m->device || slot>=m->frames.size()){error="invalid geometry device or frame slot";return false;}
   try {
      auto &frame=m->frames[slot];auto &meshes=frame.meshes;
      if(frame.scene!=&scene)meshes.clear();
      bool incremental=frame.scene==&scene && scene.meshesRevision>=frame.revision &&
         scene.meshesRevision-frame.revision<=WorldScene::meshChangeCapacity;
      if(incremental)for(uint64_t revision=frame.revision+1;revision<=scene.meshesRevision;++revision) {
         const auto &change=scene.meshChanges[(revision-1)%WorldScene::meshChangeCapacity];
         if(change.revision!=revision || !change.handle){incremental=false;break;}
      }
      auto updateMesh=[&](RenderSceneHandle id,const SceneMesh &source) {
         auto &mesh=meshes[id];
         if(mesh && mesh->revision==source.revision)return;
         const size_t vertices=source.vertices.size()*sizeof(RenderSceneVertex),indices=source.indices.size()*sizeof(uint32_t);
         // Stage both allocations before replacing a working cache entry.
         if(!mesh || mesh->vertices->size<vertices || mesh->indices->size<indices) {
            auto replacement=std::make_unique<Mesh>();
            replacement->vertices=m->allocate(vertices,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            replacement->indices=m->allocate(indices,VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
            mesh=std::move(replacement);
         }
         std::memcpy(mesh->vertices->mapped,source.vertices.data(),vertices);
         std::memcpy(mesh->indices->mapped,source.indices.data(),indices);
         mesh->revision=source.revision;
      };
      if(incremental) {
         for(uint64_t revision=frame.revision+1;revision<=scene.meshesRevision;++revision) {
            const auto id=scene.meshChanges[(revision-1)%WorldScene::meshChangeCapacity].handle;
            const auto found=scene.meshes.find(id);
            if(found==scene.meshes.end())meshes.erase(id);else updateMesh(id,found->second);
         }
      }else {
         for(auto it=meshes.begin();it!=meshes.end();)if(!scene.meshes.count(it->first))it=meshes.erase(it);else ++it;
         for(const auto &entry:scene.meshes)updateMesh(entry.first,entry.second);
      }
      frame.scene=&scene;frame.revision=scene.meshesRevision;
      return true;
   }catch(const std::exception &failure){error=failure.what();return false;}
}
bool VulkanWorldGeometry::MeshBuffers(uint32_t slot,RenderSceneHandle id,VkBuffer &vertices,VkBuffer &indices,VkDeviceSize *vertexOffset,VkDeviceSize *indexOffset) const {
   if(slot>=m->frames.size())return false;
   const auto &meshes=m->frames[slot].meshes;auto found=meshes.find(id);
   if(found==meshes.end() || !found->second)return false;
   vertices=found->second->vertices->buffer;indices=found->second->indices->buffer;
   if(vertexOffset)*vertexOffset=found->second->vertices->offset;if(indexOffset)*indexOffset=found->second->indices->offset;return true;
}
size_t VulkanWorldGeometry::MeshCount(uint32_t slot) const {return slot<m->frames.size()?m->frames[slot].meshes.size():0;}
}
