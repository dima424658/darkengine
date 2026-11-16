#include "world_scene.hpp"
#include <cstdio>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <unordered_set>
#include <unordered_map>
extern "C" int CheckTerrainVisibility(void *world,int enabled)
{
   const auto &scene=*static_cast<dark::WorldScene *>(world);
   unsigned casters=0,visibleCasters=0,visibleReceivers=0,hiddenReceivers=0;
   uint64_t casterTriangles=0,receiverTriangles=0;
   std::unordered_map<uint64_t,int64_t> triangleBalance;
   for(const auto &entry:scene.instances) {
      const auto &instance=entry.second;
      if(instance.objectId) {
         if(!enabled && !instance.visible)return 0; // Missing snapshot restores models too.
         continue;
      }
      const auto triangles=scene.meshes.at(instance.mesh).indices.size()/3;
      const auto &mesh=scene.meshes.at(instance.mesh);
      for(size_t index=0;index<mesh.indices.size();index+=3) {
         uint64_t hash=1469598103934665603ull;
         auto add=[&](const void *data,size_t count) {
            const auto *bytes=static_cast<const unsigned char *>(data);
            for(size_t i=0;i<count;++i){hash^=bytes[i];hash*=1099511628211ull;}
         };
         add(instance.transform,sizeof(instance.transform));add(&instance.receivesFog,sizeof(instance.receivesFog));
         add(&mesh.material,sizeof(mesh.material));
         for(unsigned corner=0;corner<3;++corner)add(&mesh.vertices[mesh.indices[index+corner]],sizeof(RenderSceneVertex));
         triangleBalance[hash]+=instance.castsShadow?1:-1;
      }
      if(instance.castsShadow) {
         ++casters;casterTriangles+=triangles;if(instance.visible)++visibleCasters;
      }else {
         receiverTriangles+=triangles;
         if(instance.visible)++visibleReceivers;else ++hiddenReceivers;
      }
   }
   std::printf("Fixture terrain visibility enabled=%d casters=%u visible_casters=%u visible_receivers=%u hidden_receivers=%u triangles=%llu/%llu\n",
      enabled,casters,visibleCasters,visibleReceivers,hiddenReceivers,
      (unsigned long long)casterTriangles,(unsigned long long)receiverTriangles);
   if(receiverTriangles && receiverTriangles!=casterTriangles)return 0;
   if(receiverTriangles)for(const auto &entry:triangleBalance)if(entry.second)return 0;
   return casters>0 && (enabled?(visibleCasters==0 && visibleReceivers>0 && hiddenReceivers>0):
      (visibleCasters==casters && visibleReceivers==0));
}
// Independent native ref-list oracle. Objects whose captured transform differs
// from their native location may be attachments and retain conservative drawing.
extern "C" int CheckModelVisibility(void *world,const unsigned *firstRefs,const void *infos,
   unsigned maxRefs,const int *cells,unsigned count,void *getPosition)
{
   struct Ref {unsigned system;int object;unsigned nextInBin,nextOfObject;int cell;};
   const auto refs=static_cast<const Ref *const *>(infos);
   const auto position=reinterpret_cast<const float *(*)(int)>(getPosition);
   const auto &scene=*static_cast<dark::WorldScene *>(world);
   std::unordered_set<int> visible(cells,cells+count);
   unsigned checked=0,partial=0;
   for(const auto &entry:scene.instances) {
      const auto &instance=entry.second;const int object=instance.objectId;if(object<=0)continue;
      const auto *p=position(object);if(!p)continue;
      bool placed=true;for(unsigned axis=0;axis<3;++axis)
         if(std::abs(p[axis]-instance.transform[12+axis])>1e-4f)placed=false;
      if(!placed)continue;
      unsigned first=firstRefs[object];if(!first)continue;
      bool any=false,all=true;unsigned ref=first,walked=0;
      do {
         if(!ref || ref>=maxRefs || ++walked>maxRefs || !refs[ref] || refs[ref]->object!=object)return 0;
         const bool seen=visible.count(refs[ref]->cell)!=0;any|=seen;all&=seen;
         ref=refs[ref]->nextOfObject;
      }while(ref!=first);
      if(instance.visible!=unsigned(any))return 0;
      ++checked;if(any && !all)++partial;
   }
   std::printf("Fixture model visibility checked=%u spanning_visible_hidden_cells=%u\n",checked,partial);
   return checked>0 && partial>0;
}
