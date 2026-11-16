// Read-only copied-scene diagnostic; synthetic cameras do not move the native player.
#include "world_shadows.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
extern "C" void DiagnoseTorchSelection(void *world,void *getPosition,int cached) {
 auto scene=*static_cast<dark::WorldScene*>(world);
 auto position=reinterpret_cast<const float*(*)(int)>(getPosition);
 for(int object:{448,805,876}) {
  auto p=position(object);if(!p)continue;
  RenderSceneHandle nearest=0;double best=1e30;
  for(const auto &entry:scene.lights) {
   const auto &l=entry.second;if(!l.enabled || l.type==RenderSceneDirectionalLight)continue;
   double d=0;for(int a=0;a<3;++a)d+=(p[a]-l.position[a])*(p[a]-l.position[a]);
   if(d<best){best=d;nearest=entry.first;}
  }
  if(!nearest)continue;
  const auto &light=scene.lights.at(nearest);
  std::printf("Torch object=%d position=%.3f,%.3f,%.3f light=%llu delta=%.3f owner=%d\n",object,p[0],p[1],p[2],(unsigned long long)nearest,std::sqrt(best),light.emitterObjectId);
  for(unsigned budget:{16u,64u})for(float distance:{80.f,40.f,20.f,10.f,5.f,2.f}) {
   float eye[]={p[0]+distance,p[1],p[2]},forward[]={-1,0,0},right[]={-1,1,0},bottom[]={-1,0,-1};
   RenderSceneCameraFromRays(&scene.camera,eye,forward,right,bottom,.05f,65536);
   dark::ShadowSettings settings;settings.localLights=budget;settings.localFaces=budget==16?48:384;
   if(cached){settings.cachedLights=128;settings.cachedFaces=768;}
   auto selection=dark::SelectShadowLights(scene,settings,{});
   bool selected=std::find(selection.localLights.begin(),selection.localLights.end(),nearest)!=selection.localLights.end();
   const bool inCache=std::find(selection.cachedLights.begin(),selection.cachedLights.end(),nearest)!=selection.cachedLights.end();
   dark::ShadowPlan plan;const bool planned=dark::BuildShadowPlan(scene,settings,selection,plan) && plan.firstView.count(nearest);
   std::printf("Torch selection object=%d budget=%u distance=%.1f selected=%d cached=%d planned=%d eligible=%u used=%zu faces=%u\n",object,budget,distance,selected,inCache,planned,selection.eligibleLocalLights,selection.localLights.size(),selection.localFaces);
   std::fflush(stdout);
  }
 }
}
