// Linux x86-64 retail diagnostic: move only the render camera, never the player.
#include "world_shadows.hpp"
#include <array>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <cstdint>
#include <cstring>
#pragma pack(push,1)
struct Location {float p[3];int16_t cell,hint;};
struct Position {Location location;uint16_t angles[3];};
#pragma pack(pop)
namespace {
using V=std::array<double,3>;
V minus(V a,V b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
V cross(V a,V b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
double dot(V a,V b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
int target=0;float height=0,extent=0;V direction{};bool initialized=false;
bool clear(const dark::WorldScene &scene,V origin,V end,int object) {
 const V delta=minus(end,origin);const double length=std::sqrt(dot(delta,delta));
 V ray{delta[0]/length,delta[1]/length,delta[2]/length};
 for(const auto &entry:scene.instances) {
  // A visible object can obstruct the camera even when it does not cast a shadow.
  const auto &instance=entry.second;if(instance.objectId==object)continue;
  const auto &mesh=scene.meshes.at(instance.mesh);const auto &material=scene.materials.at(mesh.material);
  if(material.opacity==RenderSceneBlended)continue;
  double low=.05,high=length-.05;
  for(unsigned row=0;row<3 && low<=high;++row) {
   double minimum=instance.transform[12+row],maximum=minimum;
   for(unsigned axis=0;axis<3;++axis) {
    double a=double(instance.transform[axis*4+row])*mesh.bounds.minimum[axis];
    double b=double(instance.transform[axis*4+row])*mesh.bounds.maximum[axis];
    minimum+=std::min(a,b);maximum+=std::max(a,b);
   }
   if(std::abs(ray[row])<1e-12){if(origin[row]<minimum || origin[row]>maximum)low=high+1;}
   else {double a=(minimum-origin[row])/ray[row],b=(maximum-origin[row])/ray[row];low=std::max(low,std::min(a,b));high=std::min(high,std::max(a,b));}
  }
  if(low>high)continue;
  auto vertex=[&](unsigned index){const auto *p=mesh.vertices[index].position;V result{};for(unsigned r=0;r<3;++r)result[r]=instance.transform[12+r]+instance.transform[r]*p[0]+instance.transform[4+r]*p[1]+instance.transform[8+r]*p[2];return result;};
  for(size_t i=0;i<mesh.indices.size();i+=3) {
   V a=vertex(mesh.indices[i]),b=vertex(mesh.indices[i+1]),c=vertex(mesh.indices[i+2]);
   V e1=minus(b,a),e2=minus(c,a),h=cross(ray,e2);double det=dot(e1,h);if(std::abs(det)<1e-12)continue;
   V s=minus(origin,a);double u=dot(s,h)/det;if(u<0 || u>1)continue;
   V q=cross(s,e1);double v=dot(ray,q)/det;if(v<0 || u+v>1)continue;
   const double distance=dot(e2,q)/det;if(distance<.05 || distance>=length-.05)continue;
   if(material.opacity==RenderSceneMasked && material.baseColorTexture) {
    const auto &texture=scene.textures.at(material.baseColorTexture);
    const auto &a=mesh.vertices[mesh.indices[i]],&b=mesh.vertices[mesh.indices[i+1]],&c=mesh.vertices[mesh.indices[i+2]];
    double uv[2];for(unsigned axis=0;axis<2;++axis)uv[axis]=a.uv[axis]*(1-u-v)+b.uv[axis]*u+c.uv[axis]*v;
    const unsigned x=std::min(texture.width-1,unsigned((uv[0]-std::floor(uv[0]))*texture.width));
    const unsigned y=std::min(texture.height-1,unsigned((uv[1]-std::floor(uv[1]))*texture.height));
    if(texture.rgba[(size_t(y)*texture.width+x)*4+3]/255.f*material.baseColor[3]<material.alphaCutoff)continue;
   }
   return false;
  }
 }
 return true;
}
}
extern "C" int InitTorchRoute(void *world,void *getPosition,void *computeCell) {
 const auto &scene=*static_cast<dark::WorldScene*>(world);
 auto get=reinterpret_cast<const float*(*)(int)>(getPosition);auto cell=reinterpret_cast<int(*)(void*)>(computeCell);
 for(float far:{80.f,60.f,40.f})for(int object:{448,805,876})for(float offset:{0.f,2.f,4.f})for(unsigned axis=0;axis<32;++axis) {
  const auto *torch=get(object);if(!torch)continue;const double angle=axis*6.283185307179586/32;
  V dir{std::cos(angle),std::sin(angle),0},origin{torch[0]+dir[0]*far,torch[1]+dir[1]*far,torch[2]+offset};
  Location probe{{float(origin[0]),float(origin[1]),float(origin[2])},-1,-1};if(cell(&probe)<0)continue;
  if(!clear(scene,origin,{torch[0],torch[1],torch[2]},object))continue;
  bool valid=true;
  for(float distance=2;distance<=far;distance+=.5f) {
   Location p{{float(torch[0]+dir[0]*distance),float(torch[1]+dir[1]*distance),torch[2]+offset},-1,-1};
   if(cell(&p)<0){valid=false;break;}
  }
  if(!valid)continue;
  target=object;height=offset;extent=far;direction=dir;initialized=true;
  std::printf("Visible torch route object=%d extent=%.1f height=%.1f angle=%.3f\n",target,extent,height,angle);std::fflush(stdout);return 1;
 }
 std::puts("No clear torch route found");return 0;
}
extern "C" int MoveTorchCamera(void *position,void *getPosition,void *computeCell,unsigned frame) {
 if(!initialized)return 1;
 auto get=reinterpret_cast<const float*(*)(int)>(getPosition);auto cell=reinterpret_cast<int(*)(void*)>(computeCell);
 const float *torch=get(target);if(!torch)return 0;
 const float distance=extent-(extent-2)*float(std::min(119u,frame-2))/119;
 Position camera{};camera.location.cell=camera.location.hint=-1;
 for(unsigned axis=0;axis<3;++axis)camera.location.p[axis]=float(torch[axis]+direction[axis]*distance);
 camera.location.p[2]+=height;const int found=cell(&camera.location);if(found<0)return 0;
 camera.location.cell=camera.location.hint=found;
 camera.angles[2]=uint16_t(int(std::atan2(-direction[1],-direction[0])*65536/6.283185307179586));
 camera.angles[1]=uint16_t(int(std::atan2(height,distance)*65536/6.283185307179586));
 std::memcpy(position,&camera,sizeof(camera));
 std::printf("Visible torch camera frame=%u distance=%.3f cell=%d\n",frame,distance,found);std::fflush(stdout);
 return 1;
}

extern "C" int ReportTorchSelection(void *world,void *selection,unsigned frame) {
 const auto &scene=*static_cast<dark::WorldScene*>(world);
 const auto &selected=*static_cast<dark::ShadowSelection*>(selection);
 RenderSceneHandle handle=0;
 for(const auto &entry:scene.lights)if(entry.second.emitterObjectId==target){handle=entry.first;break;}
 const bool active=std::find(selected.localLights.begin(),selected.localLights.end(),handle)!=selected.localLights.end();
 const bool cached=std::find(selected.cachedLights.begin(),selected.cachedLights.end(),handle)!=selected.cachedLights.end();
 std::printf("Rendered torch state frame=%u object=%d light=%llu active=%d cached=%d\n",frame,target,(unsigned long long)handle,int(active),int(cached));std::fflush(stdout);
 return int(active)+2*int(cached);
}
