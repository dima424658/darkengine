#ifndef DARK_WORLD_SHADOWS_HPP
#define DARK_WORLD_SHADOWS_HPP
#include "world_scene.hpp"
#include <unordered_set>
namespace dark {
enum class ShadowFilter:uint32_t {Hard,PCF,PCSS};
struct ShadowSettings {
   bool indirect=true;
   bool staticDepthCache=false;
   static constexpr uint32_t maxLocalLights=64,maxLocalFaces=384;
   static constexpr uint32_t maxCachedLights=128,maxCachedFaces=768;
   ShadowFilter filter=ShadowFilter::PCSS;
   uint32_t localLights=16,localFaces=48,localResolution=512;
   uint32_t cachedLights=0,cachedFaces=0,cachedResolution=128;
   uint32_t sunCascades=4,sunResolution=2048;
   uint32_t blockerSamples=16,filterSamples=32;
   float shadowDistance=256;
};
struct ShadowSelection {
   RenderSceneHandle sunlight=0;
   std::vector<RenderSceneHandle> localLights;
   uint32_t localFaces=0;
   uint32_t eligibleLocalLights=0;
   std::vector<RenderSceneHandle> cachedLights;
};
// A spotlight wider than a forward hemisphere cannot use one perspective map.
uint32_t ShadowLightFaceCount(const RenderSceneLight &light);
// Selection uses stable IDs and 20% priority hysteresis. It controls shadow
// resources only; all enabled lights still contribute to direct lighting.
// Copied receiver lists exclude lights that cannot reach any visible receiver;
// unrestricted visible receivers retain conservative selection.
ShadowSelection SelectShadowLights(const WorldScene &scene,const ShadowSettings &settings,const ShadowSelection &previous);
// std430 ABI shared by depth rendering and PCSS sampling (128 bytes).
struct ShadowView {
   float viewProjection[16];
   float originNear[4],depthPlane[4],parameters[4]; // far, resolution, emitter size, cascade far
   uint32_t metadata[4]; // light type, layer, first view for light, view count
};
static_assert(sizeof(ShadowView)==128,"shadow view shader ABI");
constexpr uint32_t ShadowCachedLayer=0x80000000u;
struct ShadowPlan {
   std::vector<ShadowView> views;
   std::vector<int32_t> emitterObjects; // CPU-only caster exclusion per view.
   std::unordered_map<RenderSceneHandle,uint32_t> firstView;
   float cameraDepthPlane[4]{};
};
// Additional std140 camera data consumed by world_mapped.frag, following the
// existing 96-byte camera prefix. Keeping this explicit avoids bool/enum ABI drift.
struct ShadowCameraData {
   float depthPlane[4];
   uint32_t filter,blockerSamples,filterSamples,globalLightCount;
};
static_assert(sizeof(ShadowCameraData)==32,"shadow camera shader ABI");
ShadowCameraData BuildShadowCameraData(const ShadowPlan &plan,const ShadowSettings &settings);
// Retains all world casters when fitting depth ranges, including offscreen ones.
// Optional previous plan preserves contiguous local array ranges for retained
// lights. It may alias plan; fragmentation falls back to a compact allocation.
bool BuildShadowPlan(const WorldScene &scene,const ShadowSettings &settings,const ShadowSelection &selection,ShadowPlan &plan,const ShadowPlan *previous=nullptr);
}
#endif
