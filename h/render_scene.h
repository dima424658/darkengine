#ifndef DARK_RENDER_SCENE_H
#define DARK_RENDER_SCENE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Native renderer interface. No pointers into mission data are retained. All
   arrays are copied. Handles are process-local and never serialized. Matrices
   are column-major; geometry and lights use engine world coordinates. */
typedef uint64_t RenderSceneHandle;
typedef struct RenderSceneVertex {
   float position[3], normal[3], uv[2];
} RenderSceneVertex;
typedef enum RenderSceneOpacity {
   RenderSceneOpaque, RenderSceneMasked, RenderSceneBlended
} RenderSceneOpacity;
typedef struct RenderSceneMaterial {
   RenderSceneHandle baseColorTexture;
   float baseColor[4], emission[3], roughness, metallic, alphaCutoff;
   uint32_t opacity, doubleSided; // Zero culls clockwise faces in the Vulkan viewport.
   uint32_t emissionUsesBaseTexture; // Modulate emission by the base texture RGB.
   uint32_t emitterSurface; // Transmits its associated emitter light; still occludes other lights.
   // Optional PBR maps. Zero keeps native scalar/default values. Scalar maps
   // use R; packed ORM uses RGB = occlusion/roughness/metallic. Separate maps win.
   RenderSceneHandle normalTexture, roughnessTexture, metallicTexture;
   RenderSceneHandle occlusionTexture, emissionTexture, ormTexture;
} RenderSceneMaterial;
typedef struct RenderSceneMesh {
   const RenderSceneVertex *vertices;
   uint32_t vertexCount;
   const uint32_t *indices;
   uint32_t indexCount;
   RenderSceneHandle material;
   uint32_t deformable;
} RenderSceneMesh;
typedef struct RenderSceneInstance {
   RenderSceneHandle mesh;
   float transform[16], previousTransform[16];
   uint32_t visible, castsShadow;
   uint32_t receivesFog; // Apply camera fog to this instance only.
   int32_t objectId;
} RenderSceneInstance;
typedef enum RenderSceneLightType {
   RenderScenePointLight, RenderSceneSpotLight, RenderSceneDirectionalLight
} RenderSceneLightType;
typedef struct RenderSceneLight {
   float position[3], direction[3], color[3];
   float intensity, range, innerCos, outerCos;
   uint32_t type, enabled;
   /* Area-emitter dimensions for ray-traced/map shadows; zero requests sharp shadows. */
   float sourceRadius, angularRadius;
   uint32_t castsShadow;
   int32_t emitterObjectId; // Associated instance.objectId; zero leaves self-occlusion unchanged.
} RenderSceneLight;
typedef struct RenderSceneCamera {
   float viewProjection[16], previousViewProjection[16], position[3];
   float exposure, ambient, lightScale;
   float fogColor[3], fogDistance; // Linear RGB; zero distance disables fog.
   // Optional native depth encoding: a*viewDistance+b/viewDistance+c, enabled.
   float nativeDepth[4];
} RenderSceneCamera;
typedef struct RenderSceneStats {
   uint64_t revision;
   uint32_t textures, materials, meshes, instances, lights;
   uint64_t vertices, triangles;
} RenderSceneStats;
RenderSceneHandle RenderSceneCreateTexture(uint32_t width, uint32_t height,
                                           const void *rgba, uint32_t rowBytes);
/* Linear UNORM sampling for normals/scalars; ordinary textures sample sRGB. */
RenderSceneHandle RenderSceneCreateDataTexture(uint32_t width,uint32_t height,const void *rgba,uint32_t rowBytes);
int RenderSceneUpdateTexture(RenderSceneHandle texture, const void *rgba, uint32_t rowBytes);
/* Replace dimensions/pixels while preserving material references. */
int RenderSceneReplaceTexture(RenderSceneHandle texture,uint32_t width,uint32_t height,const void *rgba,uint32_t rowBytes);
RenderSceneHandle RenderSceneCreateMaterial(const RenderSceneMaterial *material);
int RenderSceneUpdateMaterial(RenderSceneHandle handle, const RenderSceneMaterial *material);
RenderSceneHandle RenderSceneCreateMesh(const RenderSceneMesh *mesh);
/* Pose updates preserve topology and material. */
int RenderSceneUpdateVertices(RenderSceneHandle handle, const RenderSceneVertex *vertices, uint32_t count);
RenderSceneHandle RenderSceneCreateInstance(const RenderSceneInstance *instance);
int RenderSceneUpdateInstance(RenderSceneHandle handle, const RenderSceneInstance *instance);
/* Optional copied receiver light list. Unrestricted by default; an empty list
   receives ambient/emission only. Destroying a light removes it from lists. */
int RenderSceneSetInstanceLights(RenderSceneHandle instance,const RenderSceneHandle *lights,uint32_t count);
int RenderSceneClearInstanceLights(RenderSceneHandle instance);
RenderSceneHandle RenderSceneCreateLight(const RenderSceneLight *light);
int RenderSceneUpdateLight(RenderSceneHandle handle, const RenderSceneLight *light);
int RenderSceneSetLightEmitter(RenderSceneHandle light,int32_t objectId);
/* Referenced textures, materials and meshes cannot be destroyed. */
int RenderSceneDestroy(RenderSceneHandle handle);
int RenderSceneSetCamera(const RenderSceneCamera *camera);
int RenderSceneGetCamera(RenderSceneCamera *camera);
/* Construct Vulkan [0,1] depth projection from world-space camera rays. The
   edge rays point at NDC (1,0) and (0,1); forward points at the image center. */
int RenderSceneCameraFromRays(RenderSceneCamera *camera, const float position[3],
   const float forward[3], const float rightEdge[3], const float bottomEdge[3],
   float nearPlane, float farPlane);
void RenderSceneGetStats(RenderSceneStats *stats);
/* Invalidates every handle, including across mission transitions. */
void RenderSceneClear(void);
#ifdef __cplusplus
}
#endif
#endif
