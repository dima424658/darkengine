#include "world_scene.hpp"
#include <cmath>
#include <cstdio>

// Retail Mission 2: STRSPOT object 234, with its light at the light-source vhot.
extern "C" int CheckNativeLamppost(void *world, const float *point)
{
   const auto &scene = *static_cast<dark::WorldScene *>(world);
   unsigned housing = 0, lenses = 0, owners = 0;
   for (const auto &entry : scene.instances) {
      const auto &instance = entry.second;
      if (instance.objectId != 234) continue;
      const auto &mesh = scene.meshes.at(instance.mesh);
      const auto &material = scene.materials.at(mesh.material);
      if (!instance.castsShadow) return 0;
      if (material.emitterSurface) {
         if (material.emission[0] <= 0) return 0;
         ++lenses;
      } else {
         for (float emission : material.emission)
            if (std::abs(emission) > 1e-6f) return 0;
         ++housing;
      }
   }
   for (const auto &entry : scene.lights) {
      const auto &light = entry.second;
      if (light.type == RenderSceneDirectionalLight || !light.enabled) continue;
      double distance = 0;
      for (unsigned axis = 0; axis < 3; ++axis) {
         const double delta = double(point[axis]) - light.position[axis];
         distance += delta * delta;
      }
      if (distance <= 1e-6 && light.emitterObjectId == 234) ++owners;
   }
   std::printf("Fixture lamppost housing=%u lenses=%u owners=%u\n", housing, lenses, owners);
   return housing == 5 && lenses == 2 && owners > 0;
}
