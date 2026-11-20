#pragma once
#include "vulkan_scene_renderer.h"

struct cVulkanSceneRenderer::sRestoreState
{
   sDrawState & target;
   sDrawState saved;
   ~sRestoreState() { target = saved; }
};
