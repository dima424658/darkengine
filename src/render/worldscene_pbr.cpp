#include "worldscene_pbr.hpp"
#include <image_pixels.hpp>
#include <render_backend.h>
#include <unordered_map>
#include <array>
extern "C" RenderSceneHandle lgd3d_scene_copy_texture(grs_bitmap *);
namespace {
struct Maps {uint64_t revision=0;RenderSceneHandle base=0;std::array<RenderSceneHandle,6> maps{};};
auto &cache(){static auto *value=new std::unordered_map<grs_bitmap *,Maps>;return *value;}
RenderSceneHandle copy(const std::shared_ptr<const ResImagePixels> &image,bool linear) {
   if(!image)return 0;
   return linear?RenderSceneCreateDataTexture(image->width,image->height,image->rgba.data(),image->width*4):
      RenderSceneCreateTexture(image->width,image->height,image->rgba.data(),image->width*4);
}
}
void WorldSceneApplyPbrMaps(grs_bitmap *bitmap,RenderSceneMaterial &material) {
   // Animation may replace an authored frame with an unregistered/procedural
   // bitmap. Never inherit the previous frame's optional maps in that case.
   material.normalTexture=material.roughnessTexture=material.metallicTexture=0;
   material.occlusionTexture=material.emissionTexture=material.ormTexture=0;
   if(!bitmap)return;
   const auto revision=ResImageSourceRevision(bitmap);if(!revision)return;
   auto &entry=cache()[bitmap];
   if(entry.revision!=revision) {
      entry={};entry.revision=revision;
      entry.base=copy(ResImageExactPixels(bitmap),false);
      constexpr const char *suffixes[]={"_normal","_roughness","_metallic","_ao","_emissive","_orm"};
      for(unsigned i=0;i<6;++i)entry.maps[i]=copy(ResImageCompanion(bitmap,suffixes[i]),i!=4);
      unsigned count=0;for(auto id:entry.maps)count+=id!=0;
      RenderBackendTrace("world-pbr source=%s source_revision=%llu maps=%u",ResImageSourceName(bitmap).c_str(),static_cast<unsigned long long>(revision),count);
   }
   if(entry.base)material.baseColorTexture=entry.base;
   material.normalTexture=entry.maps[0];material.roughnessTexture=entry.maps[1];material.metallicTexture=entry.maps[2];
   material.occlusionTexture=entry.maps[3];material.emissionTexture=entry.maps[4];material.ormTexture=entry.maps[5];
}
void WorldScenePbrReset(){cache().clear();}
RenderSceneHandle WorldSceneCopyTexture(grs_bitmap *bitmap) {
   RenderSceneMaterial material{};WorldSceneApplyPbrMaps(bitmap,material);
   return material.baseColorTexture?material.baseColorTexture:lgd3d_scene_copy_texture(bitmap);
}
