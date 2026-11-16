#include "../../libsrc/lgd3d/world_shadows.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
static void check(bool value){if(!value){std::fputs("shadow budget regression failed\n",stderr);std::abort();}}
static RenderSceneLight light(uint32_t type,float power) {
   RenderSceneLight result{};result.type=type;result.enabled=result.castsShadow=1;
   result.color[0]=result.color[1]=result.color[2]=1;result.intensity=power;result.direction[2]=-1;
   if(type==RenderSceneSpotLight){result.outerCos=.5f;result.innerCos=.8f;}return result;
}
int main() {
   dark::WorldScene scene;scene.hasCamera=true;
   const float eye[]={0,0,0},forward[]={1,0,0},right[]={1,-1,0},bottom[]={1,0,-1};
   check(RenderSceneCameraFromRays(&scene.camera,eye,forward,right,bottom,.1f,100));
   dark::ShadowSettings settings;check(settings.filter==dark::ShadowFilter::PCSS && settings.localLights==16 && settings.localFaces==48);
   dark::ShadowPlan plan;plan.cameraDepthPlane[2]=1;
   auto cameraData=dark::BuildShadowCameraData(plan,settings);
   check(cameraData.filter==2 && cameraData.blockerSamples==16 && cameraData.filterSamples==32 && cameraData.depthPlane[2]==1);
   auto invalidSamples=settings;invalidSamples.blockerSamples=0;invalidSamples.filterSamples=1000;
   cameraData=dark::BuildShadowCameraData(plan,invalidSamples);check(cameraData.blockerSamples==1 && cameraData.filterSamples==64);
   // Directional shadows have their own four-cascade allocation.
   scene.lights[100]=light(RenderSceneDirectionalLight,1);scene.lights[101]=light(RenderSceneDirectionalLight,2);
   for(unsigned i=1;i<=9;++i)scene.lights[i]=light(RenderScenePointLight,100-i);
   for(unsigned i=20;i<40;++i)scene.lights[i]=light(RenderSceneSpotLight,1);
   auto selected=dark::SelectShadowLights(scene,settings,{});
   check(selected.sunlight==101 && selected.localLights.size()==8 && selected.localFaces==48);
   check(selected.eligibleLocalLights==29);
   settings.cachedLights=2;settings.cachedFaces=12;
   selected=dark::SelectShadowLights(scene,settings,{});
   check(selected.localLights.size()==8 && selected.cachedLights.size()==2);
   for(const auto id:selected.cachedLights)check(std::find(selected.localLights.begin(),selected.localLights.end(),id)==selected.localLights.end());
   settings.cachedLights=settings.cachedFaces=0;
   // Spare faces go to lower-priority spotlights when a point light cannot fit.
   settings.localFaces=47;selected=dark::SelectShadowLights(scene,settings,{});
   check(selected.localLights.size()==12 && selected.localFaces==47);
   check(std::count_if(selected.localLights.begin(),selected.localLights.end(),[](auto id){return id<20;})==7);
   settings.localLights=2;selected=dark::SelectShadowLights(scene,settings,{});check(selected.localLights.size()==2 && selected.localFaces==12);
   settings.localFaces=0;settings.sunCascades=0;selected=dark::SelectShadowLights(scene,settings,{});
   check(selected.localLights.empty() && selected.sunlight==0);
   // Hysteresis retains a selected light across a small priority change.
   scene.lights.clear();scene.lights[1]=light(RenderScenePointLight,100);scene.lights[2]=light(RenderScenePointLight,105);
   settings=dark::ShadowSettings{};settings.localLights=1;
   dark::ShadowSelection previous;previous.localLights={1};
   selected=dark::SelectShadowLights(scene,settings,previous);check(selected.localLights[0]==1);
   scene.lights[2].intensity=121;selected=dark::SelectShadowLights(scene,settings,previous);check(selected.localLights[0]==2);
   scene.lights[2].castsShadow=0;selected=dark::SelectShadowLights(scene,settings,{});check(selected.localLights[0]==1);
   scene.lights[1].enabled=0;selected=dark::SelectShadowLights(scene,settings,{});check(selected.localLights.empty());
   // Finite lights outside the view cannot affect visible receivers.
   scene.lights.clear();scene.lights[1]=light(RenderScenePointLight,100);scene.lights[1].range=1;scene.lights[1].position[0]=-5;
   selected=dark::SelectShadowLights(scene,settings,{});check(selected.localLights.empty());
   scene.lights[1].position[0]=5;selected=dark::SelectShadowLights(scene,settings,{});check(selected.localLights.size()==1);
   // Ties are ordered by stable handles regardless of unordered_map insertion.
   scene.lights.clear();scene.lights[42]=light(RenderSceneSpotLight,1);scene.lights[11]=light(RenderSceneSpotLight,1);
   selected=dark::SelectShadowLights(scene,settings,{});check(selected.localLights[0]==11);
   check(scene.lights.size()==2 && scene.lights[42].enabled); // Budgeting never disables illumination.
   // A bright light in another native cell must not steal a visible torch's map.
   dark::SceneMesh mesh{};mesh.bounds={{4,-1,-1},{6,1,1}};scene.meshes[7]=mesh;
   RenderSceneInstance receiver{};receiver.mesh=7;receiver.visible=1;
   receiver.transform[0]=receiver.transform[5]=receiver.transform[10]=receiver.transform[15]=1;
   scene.instances[8]=receiver;scene.instanceLights[8]={42};scene.lights[11].intensity=10000;
   selected=dark::SelectShadowLights(scene,settings,{});check(selected.localLights.size()==1 && selected.localLights[0]==42);
   const float distantEye[]={-20,0,0};
   check(RenderSceneCameraFromRays(&scene.camera,distantEye,forward,right,bottom,.1f,100));
   selected=dark::SelectShadowLights(scene,settings,{});check(selected.localLights.size()==1 && selected.localLights[0]==42);
   check(RenderSceneCameraFromRays(&scene.camera,eye,forward,right,bottom,.1f,100));
   scene.instanceLights[8].clear();selected=dark::SelectShadowLights(scene,settings,{});check(selected.localLights.empty());
   // An unrestricted visible receiver restores fallback; an offscreen one does not.
   scene.instances[9]=receiver;scene.instances[9].transform[12]=-20;
   selected=dark::SelectShadowLights(scene,settings,{});check(selected.localLights.empty());
   scene.instances[9].transform[12]=0;
   selected=dark::SelectShadowLights(scene,settings,{});check(selected.localLights.size()==1 && selected.localLights[0]==11);
   scene.instances[9].visible=0;
   selected=dark::SelectShadowLights(scene,settings,{});check(selected.localLights.empty());
   auto wide=light(RenderSceneSpotLight,1);wide.outerCos=-.1f;check(dark::ShadowLightFaceCount(wide)==6);
   scene.hasCamera=false;check(dark::SelectShadowLights(scene,settings,{}).localLights.empty());
   std::puts("PASS shadow face/light limits, PCSS defaults, directional allocation, hysteresis, visibility and stable ties");
}
