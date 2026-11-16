#ifndef DARK_WORLD_SCENE_LIGHTS_HPP
#define DARK_WORLD_SCENE_LIGHTS_HPP
#include <render_scene.h>
#include <mlight.h>
#include <objlight.h>
#include <missrend.h>
#include <algorithm>
// Native point/spot values already contain the engine's light_scale divisor.
// Legacy diffuse is B*cos(theta)/distance; modern diffuse is I*cos(theta)/(pi*d^2).
// Preserve approximate diffuse brightness at 16 engine feet, then use physical
// inverse-square falloff. Native sunlight uses a synthetic point at 125 feet;
// remove that distance convention before applying the directional Lambert pi.
// This adapter does not change the native records used by AI or the light gem.
inline RenderSceneLight WorldSceneConvertNativeLight(const mls_multi_light &source,
   const sMissionRenderParams &mission,bool sunlight,bool dynamic) {
   constexpr float pi=3.14159265358979323846f,referenceDistance=16;
   RenderSceneLight light{};
   light.castsShadow=1;light.sourceRadius=.25f;light.angularRadius=.27f*pi/180;
   light.type=sunlight?RenderSceneDirectionalLight:((dynamic || source.inner==-1)?RenderScenePointLight:RenderSceneSpotLight);
   light.position[0]=source.loc.x;light.position[1]=source.loc.y;light.position[2]=source.loc.z;
   if(light.type!=RenderScenePointLight) {
      const auto &direction=sunlight?mission.sunlight_vector:source.dir;
      light.direction[0]=direction.x;light.direction[1]=direction.y;light.direction[2]=direction.z;
   }
#ifdef RGB_LIGHTING
   const auto &color=sunlight?mission.sun_scaled_rgb:source.bright;
   light.color[0]=std::max(0.0f,color.x);light.color[1]=std::max(0.0f,color.y);light.color[2]=std::max(0.0f,color.z);
#else
   for(float &color:light.color)color=std::max(0.0f,sunlight?mission.sun_b:source.bright);
#endif
   light.intensity=pi*(sunlight?1.0f/OBJLIGHT_SUNLIGHT_DISTANCE:referenceDistance);
   // add_dynamic_light writes only position/brightness. Its other fields can
   // retain an old mission's spotlight cone/radius and must never be read here.
   light.range=(dynamic || sunlight)?0:std::max(0.0f,source.radius);
   if(light.type==RenderSceneSpotLight){light.innerCos=source.inner;light.outerCos=source.outer;}
   light.enabled=light.color[0]>0 || light.color[1]>0 || light.color[2]>0;
   return light;
}
#endif
