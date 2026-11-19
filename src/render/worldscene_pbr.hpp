#pragma once
#include <render_scene.h>
struct grs_bitmap;
void WorldSceneApplyPbrMaps(grs_bitmap *,RenderSceneMaterial &);
RenderSceneHandle WorldSceneCopyTexture(grs_bitmap *);
void WorldScenePbrReset();
