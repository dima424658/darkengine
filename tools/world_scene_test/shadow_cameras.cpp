#include "../../libsrc/lgd3d/world_shadows.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
static void check(bool value){if(!value){std::fputs("shadow camera regression failed\n",stderr);std::abort();}}
static std::array<double,3> project(const dark::ShadowView &view,float x,float y,float z) {
   const double p[4]={x,y,z,1};double clip[4]{};
   for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)clip[r]+=view.viewProjection[c*4+r]*p[c];
   return {clip[0]/clip[3],clip[1]/clip[3],clip[2]/clip[3]};
}
static void inside(const std::array<double,3> &p){check(std::abs(p[0])<=1.00001 && std::abs(p[1])<=1.00001 && p[2]>=-.00001 && p[2]<=1.00001);}
static void camera(dark::WorldScene &scene,float y=0) {
   const float eye[]={0,y,0},forward[]={1,0,0},right[]={1,-1,0},bottom[]={1,0,-1};
   check(RenderSceneCameraFromRays(&scene.camera,eye,forward,right,bottom,.1f,1000));scene.hasCamera=true;
}
static void fittedRanges() {
   dark::WorldScene scene;camera(scene);scene.materials[1]={};
   std::vector<std::array<double,3>> corners;
   for(unsigned i=0;i<160;++i) {
      dark::SceneMesh mesh{};mesh.material=1;
      mesh.bounds={{-2,-.5f,-1},{3,1,2}};scene.meshes[i+2]=mesh;
      const float angle=i*.731f,c=std::cos(angle),s=std::sin(angle),mirror=i&1?-1.f:1.f;
      RenderSceneInstance instance{};instance.mesh=i+2;instance.castsShadow=1;
      const float model[16]={c*mirror,s*mirror,0,0,-s*2,c*2,0,0,.3f,.1f,.7f,0,
         float(int(i*17%193)-96),float(int(i*37%211)-105),float(int(i*13%71)-35),1};
      std::copy(model,model+16,instance.transform);
      if(i%5==0){instance.transform[12]+=1000000;instance.transform[13]-=1000000;}
      scene.instances[i+200]=instance;
      for(unsigned corner=0;corner<8;++corner) {
         const double p[4]={mesh.bounds.minimum[0],mesh.bounds.minimum[1],mesh.bounds.minimum[2],1};
         double local[4];std::copy(p,p+4,local);
         for(unsigned axis=0;axis<3;++axis)if(corner&(1u<<axis))local[axis]=mesh.bounds.maximum[axis];
         std::array<double,3> world{};
         for(unsigned row=0;row<3;++row)for(unsigned col=0;col<4;++col)world[row]+=double(instance.transform[col*4+row])*local[col];
         corners.push_back(world);
      }
   }
   dark::ShadowSettings settings;dark::ShadowSelection selection;selection.localLights={9999};
   dark::ShadowPlan plan;RenderSceneLight point{};point.type=RenderScenePointLight;
   for(unsigned i=0;i<80;++i) {
      point.position[0]=float(int(i*23%97)-48);point.position[1]=float(int(i*41%101)-50);point.position[2]=float(int(i*31%73)-36);
      if(i%3==0){point.position[0]+=1000000;point.position[1]-=1000000;}
      scene.lights[9999]=point;check(dark::BuildShadowPlan(scene,settings,selection,plan));
      double farSquared=0;
      for(const auto &corner:corners) {
         double squared=0;for(unsigned axis=0;axis<3;++axis){const double delta=corner[axis]-point.position[axis];squared+=delta*delta;}
         farSquared=std::max(farSquared,squared);
      }
      const double expected=std::sqrt(farSquared)+1;
      for(const auto &view:plan.views)check(std::abs(view.parameters[0]-expected)<=std::max(.0001,expected*2e-7));
   }
   point.type=RenderSceneDirectionalLight;point.direction[0]=.3f;point.direction[1]=.4f;point.direction[2]=-.8660254f;
   scene.lights[9999]=point;selection.localLights.clear();selection.sunlight=9999;
   check(dark::BuildShadowPlan(scene,settings,selection,plan));
   for(const auto &view:plan.views)for(const auto &corner:corners) {
      const auto p=project(view,float(corner[0]),float(corner[1]),float(corner[2]));
      check(p[2]>=-.00001 && p[2]<=1.00001);
   }
   std::puts("PASS fitted point/sun ranges match brute-force transformed corners for rotation, mirror, shear and large coordinates");
}
int main() {
   fittedRanges();
   {
      dark::WorldScene scene;camera(scene);dark::ShadowSettings settings;settings.localFaces=8;
      RenderSceneLight point{};point.type=RenderScenePointLight;point.range=10;
      auto spot=point;spot.type=RenderSceneSpotLight;spot.direction[0]=1;spot.outerCos=.5f;
      scene.lights[1]=point;scene.lights[2]=spot;scene.lights[3]=spot;scene.lights[4]=point;
      dark::ShadowSelection selected;selected.localLights={1,2,3};dark::ShadowPlan plan;
      check(dark::BuildShadowPlan(scene,settings,selected,plan,&plan));
      check(plan.views[plan.firstView.at(2)].metadata[1]==6 && plan.views[plan.firstView.at(3)].metadata[1]==7);
      selected.localLights={2,3,4};check(dark::BuildShadowPlan(scene,settings,selected,plan,&plan));
      check(plan.firstView.at(2)==0 && plan.views[0].metadata[1]==6 && plan.views[1].metadata[1]==7);
      check(plan.views[plan.firstView.at(4)].metadata[1]==0);
      scene.lights.clear();selected.localLights.clear();plan={};
      for(unsigned i=1;i<=8;++i){scene.lights[i]=spot;selected.localLights.push_back(i);}
      check(dark::BuildShadowPlan(scene,settings,selected,plan,&plan));
      scene.lights[9]=point;selected.localLights={2,6,9};
      check(dark::BuildShadowPlan(scene,settings,selected,plan,&plan));
      check(plan.views[plan.firstView.at(9)].metadata[1]==2); // Fragmentation compacts safely.
      settings.localFaces=6;check(!dark::BuildShadowPlan(scene,settings,selected,plan,&plan));
      settings.localFaces=6;settings.cachedFaces=6;settings.cachedLights=1;
      scene.lights[10]=point;auto sun=point;sun.type=RenderSceneDirectionalLight;sun.direction[2]=-1;scene.lights[11]=sun;
      selected.localLights={9};selected.cachedLights={10};selected.sunlight=11;
      check(dark::BuildShadowPlan(scene,settings,selected,plan,&plan));
      check(plan.views.size()==16 && plan.firstView.at(10)==6 && plan.firstView.at(11)==12);
      for(unsigned i=6;i<12;++i)check((plan.views[i].metadata[1]&dark::ShadowCachedLayer)!=0 && plan.views[i].parameters[1]==128);
      for(unsigned i=12;i<16;++i)check(plan.views[i].metadata[0]==2 && plan.views[i].metadata[2]==12);
   }
   dark::WorldScene scene;camera(scene);dark::ShadowSettings settings;
   RenderSceneLight light{};light.type=RenderScenePointLight;light.range=20;light.sourceRadius=.25f;
   scene.lights[1]=light;dark::ShadowSelection selected;selected.localLights={1};selected.localFaces=6;
   dark::ShadowPlan plan;check(dark::BuildShadowPlan(scene,settings,selected,plan));check(plan.views.size()==6 && plan.firstView.at(1)==0);
   const float direction[6][3]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
   for(unsigned face=0;face<6;++face) {
      auto p=project(plan.views[face],direction[face][0]*5,direction[face][1]*5,direction[face][2]*5);inside(p);
      check(std::abs(p[0])<1e-6 && std::abs(p[1])<1e-6);
      check(plan.views[face].metadata[1]==face && plan.views[face].metadata[3]==6 && plan.views[face].parameters[0]==20);
   }
   // Verify the analytic sampling coordinates against the actual raster
   // matrices for every face, including ties at edges and cubemap corners.
   for(int x=-2;x<=2;++x)for(int y=-2;y<=2;++y)for(int z=-2;z<=2;++z) {
      if(!x && !y && !z)continue;
      const float ax=float(std::abs(x)),ay=float(std::abs(y)),az=float(std::abs(z));
      const unsigned face=ax>=ay && ax>=az?(x>=0?0:1):(ay>=az?(y>=0?2:3):(z>=0?4:5));
      float u=0,v=0;
      switch(face) {
         case 0:u=-y/ax;v=-z/ax;break;
         case 1:u=y/ax;v=-z/ax;break;
         case 2:u=x/ay;v=-z/ay;break;
         case 3:u=-x/ay;v=-z/ay;break;
         case 4:u=-x/az;v=-y/az;break;
         case 5:u=x/az;v=-y/az;break;
      }
      const auto projected=project(plan.views[face],float(x),float(y),float(z));
      check(std::abs(projected[0]-u)<1e-6 && std::abs(projected[1]-v)<1e-6);
   }
   check(std::abs(project(plan.views[0],2,-2,0)[0]-1)<1e-6);
   light.type=RenderSceneSpotLight;light.direction[0]=1;light.innerCos=.9f;light.outerCos=.5f;scene.lights[1]=light;
   selected.localFaces=1;check(dark::BuildShadowPlan(scene,settings,selected,plan));check(plan.views.size()==1 && plan.views[0].metadata[0]==1);
   inside(project(plan.views[0],2,0,0));check(std::abs(project(plan.views[0],2,float(-2*std::sqrt(3.0)),0)[0]-1)<1e-5);
   light.outerCos=-.1f;scene.lights[1]=light;selected.localFaces=6;
   check(dark::BuildShadowPlan(scene,settings,selected,plan));check(plan.views.size()==6);
   // A high offscreen caster must not be cropped out of the sunlight depth range.
   RenderSceneMaterial mat{};scene.materials[10]=mat;
   dark::SceneMesh mesh;mesh.material=10;mesh.bounds={{-1,-1,-1},{1,1,1}};scene.meshes[11]=mesh;
   RenderSceneInstance object{};object.mesh=11;object.castsShadow=1;
   for(unsigned i=0;i<4;++i)object.transform[i*5]=object.previousTransform[i*5]=1;
   object.transform[12]=5;object.transform[14]=900;scene.instances[12]=object;
   // Unbounded lights must cover every transformed corner, even with the
   // light away from the origin and a distant, invisible caster.
   light.range=0;light.position[0]=-7;light.position[1]=3;light.position[2]=2;
   scene.lights[1]=light;
   check(dark::BuildShadowPlan(scene,settings,selected,plan));
   double expectedFar=1;
   for(unsigned corner=0;corner<8;++corner) {
      const double x=(corner&1?1:-1)+5-light.position[0];
      const double y=(corner&2?1:-1)-light.position[1];
      const double z=(corner&4?1:-1)+900-light.position[2];
      expectedFar=std::max(expectedFar,std::sqrt(x*x+y*y+z*z)+1);
   }
   for(const auto &view:plan.views)check(std::abs(view.parameters[0]-expectedFar)<.0001);
   scene.instances.clear();check(dark::BuildShadowPlan(scene,settings,selected,plan));
   for(const auto &view:plan.views)check(view.parameters[0]==1);
   scene.instances[12]=object;
   light.type=RenderSceneDirectionalLight;light.direction[0]=0;light.direction[2]=-1;light.angularRadius=.0047f;
   scene.lights[2]=light;selected.localLights.clear();selected.sunlight=2;
   check(dark::BuildShadowPlan(scene,settings,selected,plan));check(plan.views.size()==4);
   double previous=.1;
   for(unsigned cascade=0;cascade<4;++cascade) {
      const auto &view=plan.views[cascade];const float far=view.parameters[3];check(far>previous);
      inside(project(view,float((previous+far)*.5),0,0));inside(project(view,far,far,far));inside(project(view,far,-far,-far));
      check(view.metadata[0]==2 && view.metadata[1]==cascade && view.metadata[3]==4);previous=far;
   }
   check(std::abs(previous-256)<.001);
   check(dark::SceneBoundsVisible(mesh.bounds,object.transform,plan.views[0].viewProjection));
   const auto stable=plan.views[0];camera(scene,.00001f);
   check(dark::BuildShadowPlan(scene,settings,selected,plan));
   check(std::abs(plan.views[0].viewProjection[12]-stable.viewProjection[12])<1e-6 && std::abs(plan.views[0].viewProjection[13]-stable.viewProjection[13])<1e-6);
   settings.sunCascades=5;check(!dark::BuildShadowPlan(scene,settings,selected,plan));
   std::puts("PASS six point faces, spot projection, wide-spot fallback, cascade coverage, offscreen caster range and subtexel stabilization");
}
