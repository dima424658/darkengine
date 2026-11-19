#ifndef DARK_WORLD_HISTORY_HPP
#define DARK_WORLD_HISTORY_HPP
#include "world_scene.hpp"
#include <cmath>
#include <algorithm>
#include <memory>
namespace dark {
// CPU history follows accepted GPU submissions, not frame-slot reuse. Preparing
// a frame never advances history: failed recording/submission cannot poison it.
class WorldHistory {
public:
   struct Snapshot {
      const WorldScene *scene=nullptr;
      uint64_t reset=0;
      std::unordered_map<RenderSceneHandle,RenderSceneMaterial> materials;
      uint32_t width=0,height=0;
      RenderSceneCamera camera{};
      std::unordered_map<RenderSceneHandle,RenderSceneInstance> instances;
      struct Mesh {uint64_t revision;RenderSceneHandle material;std::shared_ptr<const std::vector<RenderSceneVertex>> vertices;};
      std::unordered_map<RenderSceneHandle,Mesh> meshes;
   };
   bool Prepare(const WorldScene &scene,uint32_t width,uint32_t height) {
      pending=std::make_unique<Snapshot>();
      pending->scene=&scene;pending->reset=scene.resetRevision;pending->materials=scene.materials;
      pending->width=width;pending->height=height;pending->camera=scene.camera;
      pending->instances=scene.instances;
      for(const auto &entry:scene.meshes) {
         const auto old=submitted?submitted->meshes.find(entry.first):pending->meshes.end();
         std::shared_ptr<const std::vector<RenderSceneVertex>> vertices;
         if(submitted && submitted->scene==&scene && submitted->reset==scene.resetRevision &&
            old!=submitted->meshes.end() && old->second.revision==entry.second.revision)vertices=old->second.vertices;
         else vertices=std::make_shared<const std::vector<RenderSceneVertex>>(entry.second.vertices);
         pending->meshes.emplace(entry.first,Snapshot::Mesh{entry.second.revision,entry.second.material,std::move(vertices)});
      }
      valid=submitted && submitted->scene==&scene && submitted->reset==scene.resetRevision &&
         submitted->width==width && submitted->height==height;
      if(valid) {
         double displacement=0;
         for(unsigned i=0;i<3;++i){const double d=scene.camera.position[i]-submitted->camera.position[i];displacement+=d*d;}
         valid=displacement<=64.0*64.0;
      }
      return valid;
   }
   bool MaterialMatches(RenderSceneHandle id,const RenderSceneMaterial &material) const {
      if(!valid)return false;
      const auto found=submitted->materials.find(id);if(found==submitted->materials.end())return false;
      const auto &old=found->second;
      return old.baseColorTexture==material.baseColorTexture && old.opacity==material.opacity &&
         old.normalTexture==material.normalTexture && old.roughnessTexture==material.roughnessTexture &&
         old.metallicTexture==material.metallicTexture && old.occlusionTexture==material.occlusionTexture &&
         old.emissionTexture==material.emissionTexture && old.ormTexture==material.ormTexture &&
         old.doubleSided==material.doubleSided && old.roughness==material.roughness && old.metallic==material.metallic &&
         old.alphaCutoff==material.alphaCutoff && std::equal(old.baseColor,old.baseColor+4,material.baseColor);
   }
   void CompleteSubmission(bool accepted) {
      if(accepted && pending)submitted=std::move(pending);
      else pending.reset();
      valid=false;
   }
   const Snapshot *Previous() const {return valid?submitted.get():nullptr;}
   void Reset(){pending.reset();submitted.reset();valid=false;}
private:
   bool valid=false;
   std::unique_ptr<Snapshot> pending,submitted;
};
}
#endif
