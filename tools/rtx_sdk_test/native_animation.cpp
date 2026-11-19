// Read-only observer, invoked on the stopped engine thread by native_animation.py.
#include "../../libsrc/lgd3d/world_scene.hpp"
#include <cmath>
#include <cstdio>
#include <unordered_set>
#include <algorithm>
#include <cstring>
#include <dlfcn.h>
#include <vulkan/vulkan.h>
namespace {
std::unordered_map<RenderSceneHandle,std::vector<RenderSceneVertex>> previous;
std::unordered_set<int> creatures,animated;
unsigned frames=0,changes=0,visibleChanges=0;
float maximumDelta=0;
float captureCenter[3]{};
RenderSceneHandle fixtureLight=0;
int (*linkFixtureLight)(RenderSceneHandle,const RenderSceneHandle *,uint32_t)=nullptr;
struct Binding {VkDeviceMemory memory;VkDeviceSize offset;};
struct Mapping {const char *data;VkDeviceSize offset;};
std::unordered_map<VkBuffer,Binding> bindings;
std::unordered_map<VkDeviceMemory,Mapping> mappings;
std::unordered_map<VkDeviceAddress,VkBuffer> addresses;
std::unordered_map<RenderSceneHandle,std::vector<RenderSceneVertex>> expected;
std::unordered_set<RenderSceneHandle> changedMeshes;
std::unordered_map<VkAccelerationStructureKHR,VkDeviceAddress> accelerationAddresses;
std::unordered_set<VkDeviceAddress> creatureAcceleration;
unsigned matchedBuilds=0,matchedChangedBuilds=0,matchedInstances=0;
PFN_vkCmdBuildAccelerationStructuresKHR buildNext=nullptr;
PFN_vkGetBufferDeviceAddress addressNext=nullptr;
PFN_vkGetAccelerationStructureDeviceAddressKHR accelerationAddressNext=nullptr;
const char *host(VkDeviceAddress address) {
   const auto a=addresses.find(address);if(a==addresses.end())return nullptr;
   const auto b=bindings.find(a->second);if(b==bindings.end())return nullptr;
   const auto m=mappings.find(b->second.memory);if(m==mappings.end() || b->second.offset<m->second.offset)return nullptr;
   return m->second.data+b->second.offset-m->second.offset;
}
VKAPI_ATTR VkDeviceAddress VKAPI_CALL observeAddress(VkDevice device,const VkBufferDeviceAddressInfo *info) {
   const auto address=addressNext(device,info);addresses[address]=info->buffer;return address;
}
VKAPI_ATTR VkDeviceAddress VKAPI_CALL observeAccelerationAddress(VkDevice device,const VkAccelerationStructureDeviceAddressInfoKHR *info) {
   const auto address=accelerationAddressNext(device,info);accelerationAddresses[info->accelerationStructure]=address;return address;
}
VKAPI_ATTR void VKAPI_CALL observeBuild(VkCommandBuffer command,uint32_t count,
   const VkAccelerationStructureBuildGeometryInfoKHR *infos,const VkAccelerationStructureBuildRangeInfoKHR *const *ranges) {
   for(uint32_t n=0;n<count;++n)for(uint32_t g=0;g<infos[n].geometryCount;++g) {
      const auto &geometry=infos[n].pGeometries?infos[n].pGeometries[g]:*infos[n].ppGeometries[g];
      if(geometry.geometryType==VK_GEOMETRY_TYPE_TRIANGLES_KHR) {
         const auto &triangles=geometry.geometry.triangles;
         const char *data=host(triangles.vertexData.deviceAddress);
         bool creature=false;
         if(data && triangles.vertexStride==sizeof(RenderSceneVertex))for(const auto &entry:expected) {
            if(entry.second.size()!=size_t(triangles.maxVertex)+1)continue;
            if(std::memcmp(data,entry.second.data(),entry.second.size()*sizeof(RenderSceneVertex)))continue;
            creature=true;++matchedBuilds;
            if(changedMeshes.count(entry.first))++matchedChangedBuilds;
            break;
         }
         const auto a=accelerationAddresses.find(infos[n].dstAccelerationStructure);
         if(a!=accelerationAddresses.end()) {
            if(creature)creatureAcceleration.insert(a->second);
            else creatureAcceleration.erase(a->second);
         }
      } else if(geometry.geometryType==VK_GEOMETRY_TYPE_INSTANCES_KHR && !geometry.geometry.instances.arrayOfPointers) {
         const char *data=host(geometry.geometry.instances.data.deviceAddress);
         if(data)for(uint32_t i=0;i<ranges[n][g].primitiveCount;++i) {
            VkAccelerationStructureInstanceKHR instance;
            std::memcpy(&instance,data+ranges[n][g].primitiveOffset+i*sizeof(instance),sizeof(instance));
            if(creatureAcceleration.count(instance.accelerationStructureReference))++matchedInstances;
         }
      }
   }
   buildNext(command,count,infos,ranges);
}
}
extern "C" VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory(VkDevice device,VkBuffer buffer,VkDeviceMemory memory,VkDeviceSize offset) {
   static auto next=reinterpret_cast<PFN_vkBindBufferMemory>(dlsym(RTLD_NEXT,"vkBindBufferMemory"));
   const auto result=next(device,buffer,memory,offset);if(result==VK_SUCCESS)bindings[buffer]={memory,offset};return result;
}
extern "C" VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory(VkDevice device,VkDeviceMemory memory,VkDeviceSize offset,VkDeviceSize size,VkMemoryMapFlags flags,void **data) {
   static auto next=reinterpret_cast<PFN_vkMapMemory>(dlsym(RTLD_NEXT,"vkMapMemory"));
   const auto result=next(device,memory,offset,size,flags,data);
   if(result==VK_SUCCESS)mappings[memory]={static_cast<const char *>(*data),offset};return result;
}
extern "C" VKAPI_ATTR void VKAPI_CALL vkUnmapMemory(VkDevice device,VkDeviceMemory memory) {
   static auto next=reinterpret_cast<PFN_vkUnmapMemory>(dlsym(RTLD_NEXT,"vkUnmapMemory"));
   mappings.erase(memory);next(device,memory);
}
extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device,const char *name) {
   static auto next=reinterpret_cast<PFN_vkGetDeviceProcAddr>(dlsym(RTLD_NEXT,"vkGetDeviceProcAddr"));
   const auto function=next(device,name);if(!function)return function;
   if(!std::strcmp(name,"vkCmdBuildAccelerationStructuresKHR")) {
      buildNext=reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(function);return reinterpret_cast<PFN_vkVoidFunction>(observeBuild);
   }
   if(!std::strcmp(name,"vkGetBufferDeviceAddress") || !std::strcmp(name,"vkGetBufferDeviceAddressKHR")) {
      addressNext=reinterpret_cast<PFN_vkGetBufferDeviceAddress>(function);return reinterpret_cast<PFN_vkVoidFunction>(observeAddress);
   }
   if(!std::strcmp(name,"vkGetAccelerationStructureDeviceAddressKHR")) {
      accelerationAddressNext=reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(function);return reinterpret_cast<PFN_vkVoidFunction>(observeAccelerationAddress);
   }
   return function;
}
extern "C" void dark_observe_animation(const dark::WorldScene *scene,int (*isCreature)(int,long *)) {
   ++frames;
   // Preserve the mission's native receiver restrictions, adding only the
   // explicitly requested fixture source. Absent lists already allow all lights.
   if(fixtureLight && linkFixtureLight)for(const auto &entry:scene->instances) {
      const auto links=scene->instanceLights.find(entry.first);
      if(links==scene->instanceLights.end() || std::find(links->second.begin(),links->second.end(),fixtureLight)!=links->second.end())continue;
      auto list=links->second;list.push_back(fixtureLight);
      linkFixtureLight(entry.first,list.data(),uint32_t(list.size()));
   }
   expected.clear();changedMeshes.clear();
   for(const auto &entry:scene->instances) {
      const auto &instance=entry.second;long handle=0;
      if(!instance.objectId || !isCreature(instance.objectId,&handle))continue;
      creatures.insert(instance.objectId);
      const auto found=scene->meshes.find(instance.mesh);
      if(found==scene->meshes.end())continue;
      const auto &vertices=found->second.vertices;
      auto &old=previous[instance.mesh];float delta=0;
      if(old.size()==vertices.size())for(size_t v=0;v<vertices.size();++v)
         for(unsigned axis=0;axis<3;++axis)
            delta=std::max(delta,std::abs(vertices[v].position[axis]-old[v].position[axis]));
      if(delta>.0001f) {
         ++changes;if(instance.visible)++visibleChanges;
         maximumDelta=std::max(maximumDelta,delta);animated.insert(instance.objectId);
         changedMeshes.insert(instance.mesh);
      }
      expected[instance.mesh]=vertices;
      old=vertices;
   }
}
extern "C" void dark_report_animation() {
   std::printf("Fixture animation frames=%u creatures=%zu animated=%zu changes=%u visible_changes=%u max_delta=%.6f\n",
      frames,creatures.size(),animated.size(),changes,visibleChanges,maximumDelta);
   std::fflush(stdout);
   std::printf("Fixture animation ray_builds=%u changed_ray_builds=%u tlas_creature_instances=%u\n",matchedBuilds,matchedChangedBuilds,matchedInstances);
   std::fflush(stdout);
}
// Optional capture aid: choose an actually animated creature, then place a
// detached Linux x86-64 native camera beside its captured world-space bounds.
extern "C" int dark_aim_at_creature(void *camera,const dark::WorldScene *scene) {
   int object=0;double nearest=1e30;
   for(const auto &entry:scene->instances) {
      const auto &instance=entry.second;
      if(!animated.count(instance.objectId))continue;
      double distance=0;
      for(unsigned a=0;a<3;++a) {
         const double delta=instance.transform[12+a]-scene->camera.position[a];distance+=delta*delta;
      }
      if(distance<nearest){nearest=distance;object=instance.objectId;}
   }
   if(!object)return 0;
   float lower[3]={1e30f,1e30f,1e30f},upper[3]={-1e30f,-1e30f,-1e30f};
   for(const auto &entry:scene->instances)if(entry.second.objectId==object) {
      const auto &instance=entry.second;const auto mesh=scene->meshes.find(instance.mesh);
      if(mesh==scene->meshes.end())continue;
      std::printf("Fixture creature surface object=%d instance=%llu mesh=%llu\n",object,
         static_cast<unsigned long long>(entry.first),static_cast<unsigned long long>(instance.mesh));
      for(const auto &vertex:mesh->second.vertices)for(unsigned a=0;a<3;++a) {
         float point=instance.transform[12+a];
         for(unsigned b=0;b<3;++b)point+=instance.transform[b*4+a]*vertex.position[b];
         lower[a]=std::min(lower[a],point);upper[a]=std::max(upper[a],point);
      }
   }
   float eye[3];for(unsigned a=0;a<3;++a)captureCenter[a]=eye[a]=(lower[a]+upper[a])*.5f;
   eye[1]-=10;
   const uint16_t angles[3]={0,0,16384};
   std::memcpy(static_cast<char *>(camera)+8,eye,sizeof(eye));
   std::memcpy(static_cast<char *>(camera)+20,angles,sizeof(angles));
   std::printf("Fixture creature camera object=%d eye=%.3f,%.3f,%.3f\n",object,eye[0],eye[1],eye[2]);
   std::fflush(stdout);return object;
}
extern "C" int dark_add_creature_light(RenderSceneHandle (*create)(const RenderSceneLight *),
   int (*link)(RenderSceneHandle,const RenderSceneHandle *,uint32_t)) {
   RenderSceneLight light{};
   std::copy(captureCenter,captureCenter+3,light.position);
   light.position[0]+=.5f;light.position[1]-=5;light.position[2]+=2;
   light.color[0]=light.color[1]=light.color[2]=1;
   light.intensity=100;light.range=30;light.sourceRadius=.12f;light.enabled=light.castsShadow=1;
   fixtureLight=create(&light);linkFixtureLight=link;
   std::printf("Fixture added creature light=%llu position=%.3f,%.3f,%.3f intensity=100 radius=0.12\n",
      static_cast<unsigned long long>(fixtureLight),light.position[0],light.position[1],light.position[2]);
   std::fflush(stdout);return fixtureLight!=0;
}
