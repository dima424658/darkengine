#ifndef DARK_ENGINE_WORLD_SCENE_H
#define DARK_ENGINE_WORLD_SCENE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void WorldSceneReset(void);
void WorldSceneModelsReset(void);
void WorldSceneCaptureModels(void);
/* Read-only native cell reach; revision avoids repeating unchanged list copies. */
void WorldSceneLinkInstance(uint64_t instance,int cell,uint64_t *revision);
void WorldSceneRegisterEmitter(int object,int nativeLightIndex,const float position[3]);
/* True only when every nonempty model part has a live modern instance. */
int WorldSceneHasModel(int object);
/* Frame-local native queue state; object IDs are never retained across frames. */
void WorldSceneBeginObjectCapture(int enabled);
void WorldSceneCaptureObjectFog(int object,int enabled);
/* Diagnostic import of terrain/materials and live mission lights. */
void WorldSceneCapture(void);
/* Composite once per camera pass before native translucent effects. Returns
   success on repeated calls too, allowing portal cleanup to be a fallback. */
int WorldSceneRender(void);
/* Call inside the active r3 camera context, after zoom/canvas setup. */
void WorldSceneCaptureCamera(void);
/* Frame-local copy, before native draw cleanup frees cell render data. */
void WorldSceneCaptureVisibleCells(const int *cells,unsigned count);
/* Any native object cell reference reaches the copied visible set. */
int WorldSceneObjectVisible(int object);
#ifdef __cplusplus
}
#endif
#endif
