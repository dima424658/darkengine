#ifndef DARK_WORLD_SCENE_HPP
#define DARK_WORLD_SCENE_HPP
#include <render_scene.h>
#include <unordered_map>
#include <array>
#include <vector>
namespace dark {
static_assert(sizeof(RenderSceneVertex)==32, "world vertex layout must match GPU geometry stride");
struct SceneTexture { uint32_t width, height; std::vector<uint8_t> rgba; uint64_t revision,alphaRevision=0; bool linearData=false; };
struct SceneBounds {float minimum[3],maximum[3];};
struct SceneOrientedBounds {double center[3],axes[3][3];};
struct SceneFrustum {double planes[6][4];};
SceneOrientedBounds SceneTransformBounds(const SceneBounds &bounds,const float model[16]);
SceneFrustum SceneExtractFrustum(const float viewProjection[16]);
bool SceneBoundsVisible(const SceneOrientedBounds &bounds,const SceneFrustum &frustum);
struct SceneMesh {
   std::vector<RenderSceneVertex> vertices;
   std::vector<uint32_t> indices;
   RenderSceneHandle material;
   bool deformable;
   uint64_t revision;
   SceneBounds bounds;
};
// Conservative raster visibility only. Offscreen shadow casters remain in shadow passes.
bool SceneBoundsVisible(const SceneBounds &bounds,const float model[16],const float viewProjection[16]);
/* Access on the render/engine thread only. The scene owns CPU assets independently
   of Vulkan allocations; GPU caches key on scene identity, handles and revisions.
   Mutate an already-rendered scene through the RenderScene API so its cache
   revisions and bounded mesh history remain valid. */
struct WorldScene {
   std::unordered_map<RenderSceneHandle, SceneTexture> textures;
   std::unordered_map<RenderSceneHandle, RenderSceneMaterial> materials;
   std::unordered_map<RenderSceneHandle, SceneMesh> meshes;
   std::unordered_map<RenderSceneHandle, RenderSceneInstance> instances;
   std::unordered_map<RenderSceneHandle, RenderSceneLight> lights;
   std::unordered_map<RenderSceneHandle, std::vector<RenderSceneHandle>> instanceLights;
   RenderSceneCamera camera{};
   bool hasCamera = false;
   uint64_t resetRevision = 0;
   uint64_t revision = 0, nextHandle = 1, instanceLightsRevision = 0, materialsRevision = 0, lightsRevision = 0;
   uint64_t texturesRevision = 0;
   // Bounded, allocation-free history for independently retired GPU frame slots.
   // A zero handle marks a scene reset and requires a full cache reconciliation.
   struct MeshChange {uint64_t revision=0;RenderSceneHandle handle=0;};
   static constexpr size_t meshChangeCapacity=4096;
   std::array<MeshChange,meshChangeCapacity> meshChanges{};
   uint64_t meshesRevision=0;
   void recordMeshChange(RenderSceneHandle handle) {
      ++meshesRevision;meshChanges[(meshesRevision-1)%meshChangeCapacity]={meshesRevision,handle};
   }
};
WorldScene &GetWorldScene();
}
#endif
