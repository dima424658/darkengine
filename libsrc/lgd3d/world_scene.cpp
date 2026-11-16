#include "world_scene.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>
namespace dark {
SceneOrientedBounds SceneTransformBounds(const SceneBounds &bounds,const float model[16]) {
   SceneOrientedBounds result{};
   for(unsigned row=0;row<3;++row) {
      result.center[row]=model[12+row];
      for(unsigned axis=0;axis<3;++axis) {
         result.center[row]+=double(model[axis*4+row])*(double(bounds.minimum[axis])+bounds.maximum[axis])*.5;
         result.axes[axis][row]=double(model[axis*4+row])*(double(bounds.maximum[axis])-bounds.minimum[axis])*.5;
      }
   }
   return result;
}
SceneFrustum SceneExtractFrustum(const float projection[16]) {
   SceneFrustum result{};
   for(unsigned plane=0;plane<6;++plane)for(unsigned col=0;col<4;++col)
      result.planes[plane][col]=plane==4?projection[col*4+2]:double(projection[col*4+3])+((plane&1)?-1:1)*double(projection[col*4+plane/2]);
   return result;
}
bool SceneBoundsVisible(const SceneOrientedBounds &bounds,const SceneFrustum &frustum) {
   for(const auto &plane:frustum.planes) {
      double center=plane[3],radius=0,magnitude=std::abs(plane[3])+1;
      for(unsigned row=0;row<3;++row){center+=plane[row]*bounds.center[row];magnitude+=std::abs(plane[row]*bounds.center[row]);}
      for(const auto &axis:bounds.axes)radius+=std::abs(plane[0]*axis[0]+plane[1]*axis[1]+plane[2]*axis[2]);
      if(center+radius < -1e-6*(magnitude+radius))return false;
   }
   return true;
}
bool SceneBoundsVisible(const SceneBounds &bounds,const float model[16],const float projection[16]) {
   double clip[16]{};
   // Terrain chunks and many rigid instances use translation/axis scaling.
   // Avoid a general matrix multiply for those frequent shadow culling tests.
   const bool diagonal=model[1]==0 && model[2]==0 && model[3]==0 &&
      model[4]==0 && model[6]==0 && model[7]==0 && model[8]==0 &&
      model[9]==0 && model[11]==0 && model[15]==1;
   if(diagonal) {
      for(unsigned row=0;row<4;++row) {
         for(unsigned col=0;col<3;++col)clip[col*4+row]=double(projection[col*4+row])*model[col*5];
         clip[12+row]=double(projection[row])*model[12]+double(projection[4+row])*model[13]+
            double(projection[8+row])*model[14]+projection[12+row];
      }
   }else {
      for(unsigned col=0;col<4;++col)for(unsigned row=0;row<4;++row)
         for(unsigned k=0;k<4;++k)clip[col*4+row]+=double(projection[k*4+row])*model[col*4+k];
   }
   // Vulkan clip volume: -w <= x,y <= w and 0 <= z <= w.
   for(unsigned plane=0;plane<6;++plane) {
      double coefficients[4];
      for(unsigned col=0;col<4;++col) {
         const unsigned axis=plane/2;
         coefficients[col]=plane==4?clip[col*4+2]:clip[col*4+3]+((plane&1)?-1:1)*clip[col*4+axis];
      }
      double maximum=coefficients[3],magnitude=std::abs(coefficients[3])+1;
      for(unsigned axis=0;axis<3;++axis) {
         maximum+=coefficients[axis]*(coefficients[axis]>=0?bounds.maximum[axis]:bounds.minimum[axis]);
         magnitude+=std::abs(coefficients[axis])*std::max(std::abs(double(bounds.minimum[axis])),std::abs(double(bounds.maximum[axis])));
      }
      if(maximum < -1e-6*magnitude)return false;
   }
   return true;
}
WorldScene &GetWorldScene() {
   // Legacy AppOnExit runs after function-local static destructors registered
   // during gameplay. Keep the store alive through aggregate teardown, where
   // database reset still invalidates its handles and releases owned assets.
   static WorldScene *scene = new WorldScene;
   return *scene;
}
}
namespace {
using namespace dark;
bool finite(const float *values, size_t count) {
   for (size_t i=0; i<count; ++i) if (!std::isfinite(values[i])) return false;
   return true;
}
bool verticesValid(const RenderSceneVertex *vertices, uint32_t count) {
   if (!vertices || !count) return false;
   for (uint32_t i=0; i<count; ++i)
      if (!finite(vertices[i].position,3) || !finite(vertices[i].normal,3) || !finite(vertices[i].uv,2)) return false;
   return true;
}
SceneBounds boundsFor(const RenderSceneVertex *vertices,uint32_t count) {
   SceneBounds result{};
   std::copy(vertices[0].position,vertices[0].position+3,result.minimum);
   std::copy(vertices[0].position,vertices[0].position+3,result.maximum);
   for(uint32_t i=1;i<count;++i)for(unsigned axis=0;axis<3;++axis) {
      result.minimum[axis]=std::min(result.minimum[axis],vertices[i].position[axis]);
      result.maximum[axis]=std::max(result.maximum[axis],vertices[i].position[axis]);
   }
   return result;
}
bool valid(const RenderSceneMaterial *m) {
   if (m) {
      for(float color:m->baseColor)if(color<0 || color>1)return false;
      for(float emission:m->emission)if(emission<0)return false;
   }
   return m && finite(m->baseColor,4) && finite(m->emission,3) &&
      std::isfinite(m->roughness) && m->roughness>=0 && m->roughness<=1 &&
      std::isfinite(m->metallic) && m->metallic>=0 && m->metallic<=1 &&
      std::isfinite(m->alphaCutoff) && m->alphaCutoff>=0 && m->alphaCutoff<=1 &&
      m->opacity<=RenderSceneBlended && (!m->baseColorTexture || GetWorldScene().textures.count(m->baseColorTexture));
}
bool affine(const float *m) {
   if(!finite(m,16) || m[3]!=0 || m[7]!=0 || m[11]!=0 || m[15]!=1)return false;
   const double determinant=double(m[0])*(double(m[5])*m[10]-double(m[9])*m[6])-
      double(m[4])*(double(m[1])*m[10]-double(m[9])*m[2])+
      double(m[8])*(double(m[1])*m[6]-double(m[5])*m[2]);
   return std::isfinite(determinant) && determinant!=0;
}
bool valid(const RenderSceneInstance *i) {
   return i && GetWorldScene().meshes.count(i->mesh) && affine(i->transform) && affine(i->previousTransform);
}
bool valid(const RenderSceneLight *l) {
   if(l)for(float color:l->color)if(color<0)return false;
   if (!l || !finite(l->position,3) || !finite(l->direction,3) || !finite(l->color,3) ||
       !std::isfinite(l->intensity) || l->intensity<0 || !std::isfinite(l->range) || l->range<0 ||
       !std::isfinite(l->sourceRadius) || l->sourceRadius<0 ||
       !std::isfinite(l->angularRadius) || l->angularRadius<0 || l->angularRadius>1.57079633f ||
       l->type>RenderSceneDirectionalLight || !std::isfinite(l->innerCos) || !std::isfinite(l->outerCos)) return false;
   if (l->type!=RenderScenePointLight &&
       l->direction[0]*l->direction[0]+l->direction[1]*l->direction[1]+l->direction[2]*l->direction[2]<1e-12f) return false;
   return l->type!=RenderSceneSpotLight || (l->innerCos>=l->outerCos && l->innerCos<=1 && l->outerCos>=-1);
}
template<class Map, class Value> RenderSceneHandle create(Map &map, Value &&value) {
   auto &s=GetWorldScene();
   if (s.nextHandle==std::numeric_limits<uint64_t>::max()) return 0;
   const auto id=s.nextHandle++;
   map.emplace(id,std::forward<Value>(value)); ++s.revision; return id;
}
template<class Map,class Value> int update(Map &map,RenderSceneHandle id,const Value *value) {
   auto it=map.find(id); if(it==map.end() || !valid(value)) return 0;
   it->second=*value; ++GetWorldScene().revision; return 1;
}
}
extern "C" {
RenderSceneHandle RenderSceneCreateTexture(uint32_t width,uint32_t height,const void *rgba,uint32_t rowBytes) {
   if(!rgba || !width || !height || width>UINT32_MAX/4 || rowBytes<width*4 ||
      size_t(height)>SIZE_MAX/(size_t(width)*4) || size_t(height)>SIZE_MAX/rowBytes) return 0;
   try {
      dark::SceneTexture t{width,height,{},dark::GetWorldScene().revision+1,dark::GetWorldScene().revision+1}; t.rgba.resize(size_t(width)*height*4);
      for(uint32_t y=0;y<height;++y) std::memcpy(t.rgba.data()+size_t(y)*width*4,
         static_cast<const uint8_t *>(rgba)+size_t(y)*rowBytes,size_t(width)*4);
      return create(dark::GetWorldScene().textures,std::move(t));
   } catch(...) { return 0; }
}
int RenderSceneUpdateTexture(RenderSceneHandle id,const void *rgba,uint32_t rowBytes) {
   auto &s=dark::GetWorldScene();auto found=s.textures.find(id);
   if(found==s.textures.end() || !rgba)return 0;
   auto &texture=found->second;
   if(rowBytes<size_t(texture.width)*4 || size_t(texture.height)>SIZE_MAX/rowBytes)return 0;
   bool changed=false;
   for(uint32_t y=0;y<texture.height && !changed;++y)
      changed=std::memcmp(texture.rgba.data()+size_t(y)*texture.width*4,
         static_cast<const uint8_t *>(rgba)+size_t(y)*rowBytes,size_t(texture.width)*4)!=0;
   if(!changed)return 1;
   bool alphaChanged=false;
   for(uint32_t y=0;y<texture.height;++y) {
      auto *destination=texture.rgba.data()+size_t(y)*texture.width*4;
      const auto *source=static_cast<const uint8_t *>(rgba)+size_t(y)*rowBytes;
      if(!alphaChanged)for(size_t x=3;x<size_t(texture.width)*4;x+=4)
         if(destination[x]!=source[x]){alphaChanged=true;break;}
      std::memcpy(destination,source,size_t(texture.width)*4);
   }
   texture.revision=++s.revision;
   if(alphaChanged)texture.alphaRevision=texture.revision;
   return 1;
}
int RenderSceneReplaceTexture(RenderSceneHandle id,uint32_t width,uint32_t height,const void *rgba,uint32_t rowBytes) {
   auto &scene=dark::GetWorldScene();const auto found=scene.textures.find(id);
   if(found==scene.textures.end() || !rgba || !width || !height || width>UINT32_MAX/4 || rowBytes<width*4 ||
      size_t(height)>SIZE_MAX/(size_t(width)*4) || size_t(height)>SIZE_MAX/rowBytes)return 0;
   if(found->second.width==width && found->second.height==height)return RenderSceneUpdateTexture(id,rgba,rowBytes);
   try {
      dark::SceneTexture replacement{width,height,{},scene.revision+1,scene.revision+1};replacement.rgba.resize(size_t(width)*height*4);
      for(uint32_t y=0;y<height;++y)std::memcpy(replacement.rgba.data()+size_t(y)*width*4,
         static_cast<const uint8_t *>(rgba)+size_t(y)*rowBytes,size_t(width)*4);
      found->second=std::move(replacement);++scene.revision;return 1;
   }catch(...){return 0;}
}
RenderSceneHandle RenderSceneCreateMaterial(const RenderSceneMaterial *m) {
   if(!valid(m))return 0;
   try {
      auto &scene=dark::GetWorldScene();const auto id=create(scene.materials,RenderSceneMaterial(*m));
      if(id)++scene.materialsRevision;return id;
   }catch(...){return 0;}
}
int RenderSceneUpdateMaterial(RenderSceneHandle id,const RenderSceneMaterial *m) {
   auto &scene=dark::GetWorldScene();const auto found=scene.materials.find(id);
   if(found==scene.materials.end() || !valid(m))return 0;
   if(std::memcmp(&found->second,m,sizeof(*m))==0)return 1;
   found->second=*m;++scene.materialsRevision;++scene.revision;return 1;
}
RenderSceneHandle RenderSceneCreateMesh(const RenderSceneMesh *m) {
   auto &s=dark::GetWorldScene();
   if(!m || !s.materials.count(m->material) || !verticesValid(m->vertices,m->vertexCount) ||
      !m->indices || !m->indexCount || m->indexCount%3) return 0;
   for(uint32_t i=0;i<m->indexCount;++i)if(m->indices[i]>=m->vertexCount)return 0;
   try {
      dark::SceneMesh mesh{{m->vertices,m->vertices+m->vertexCount},
         {m->indices,m->indices+m->indexCount},m->material,m->deformable!=0,s.revision+1,boundsFor(m->vertices,m->vertexCount)};
      const auto id=create(s.meshes,std::move(mesh));if(id)s.recordMeshChange(id);return id;
   } catch(...) {return 0;}
}
int RenderSceneUpdateVertices(RenderSceneHandle id,const RenderSceneVertex *v,uint32_t count) {
   auto &s=dark::GetWorldScene();auto it=s.meshes.find(id);
   if(it==s.meshes.end() || !it->second.deformable || count!=it->second.vertices.size() || !verticesValid(v,count))return 0;
   if(std::memcmp(it->second.vertices.data(),v,size_t(count)*sizeof(*v))==0)return 1;
   std::copy(v,v+count,it->second.vertices.begin());it->second.bounds=boundsFor(v,count);
   it->second.revision=++s.revision;s.recordMeshChange(id);return 1;
}
RenderSceneHandle RenderSceneCreateInstance(const RenderSceneInstance *i) {
   if(!valid(i))return 0;
   try{return create(dark::GetWorldScene().instances,RenderSceneInstance(*i));}catch(...){return 0;}
}
int RenderSceneUpdateInstance(RenderSceneHandle id,const RenderSceneInstance *i) {
   return update(dark::GetWorldScene().instances,id,i);
}
int RenderSceneSetInstanceLights(RenderSceneHandle instance,const RenderSceneHandle *lights,uint32_t count) {
   auto &s=dark::GetWorldScene();
   if(!s.instances.count(instance) || (count && !lights))return 0;
   try {
      std::vector<RenderSceneHandle> copied;
      if(count)copied.assign(lights,lights+count);
      for(auto id:copied)if(!s.lights.count(id))return 0;
      std::sort(copied.begin(),copied.end());copied.erase(std::unique(copied.begin(),copied.end()),copied.end());
      const auto old=s.instanceLights.find(instance);
      if(old!=s.instanceLights.end() && old->second==copied)return 1;
      s.instanceLights[instance]=std::move(copied);++s.instanceLightsRevision;++s.revision;return 1;
   }catch(...){return 0;}
}
int RenderSceneClearInstanceLights(RenderSceneHandle instance) {
   auto &s=dark::GetWorldScene();if(!s.instances.count(instance))return 0;
   if(s.instanceLights.erase(instance)){++s.instanceLightsRevision;++s.revision;}
   return 1;
}
RenderSceneHandle RenderSceneCreateLight(const RenderSceneLight *l) {
   if(!valid(l))return 0;
   try{return create(dark::GetWorldScene().lights,RenderSceneLight(*l));}catch(...){return 0;}
}
int RenderSceneUpdateLight(RenderSceneHandle id,const RenderSceneLight *l) {
   return update(dark::GetWorldScene().lights,id,l);
}
int RenderSceneSetLightEmitter(RenderSceneHandle id,int32_t objectId) {
   auto &scene=dark::GetWorldScene();const auto found=scene.lights.find(id);
   if(found==scene.lights.end())return 0;
   if(found->second.emitterObjectId!=objectId){found->second.emitterObjectId=objectId;++scene.revision;}
   return 1;
}
int RenderSceneDestroy(RenderSceneHandle id) {
   auto &s=dark::GetWorldScene();
   if(s.textures.count(id))for(const auto &m:s.materials)if(m.second.baseColorTexture==id)return 0;
   if(s.materials.count(id))for(const auto &m:s.meshes)if(m.second.material==id)return 0;
   if(s.meshes.count(id))for(const auto &i:s.instances)if(i.second.mesh==id)return 0;
   const bool removingLight=s.lights.count(id)!=0;
   const bool removingMaterial=s.materials.count(id)!=0;
   const bool removingMesh=s.meshes.count(id)!=0;
   if(s.textures.erase(id)||s.materials.erase(id)||s.meshes.erase(id)||s.instances.erase(id)||s.lights.erase(id)) {
      if(removingMaterial)++s.materialsRevision;
      if(removingMesh)s.recordMeshChange(id);
      if(s.instanceLights.erase(id))++s.instanceLightsRevision;
      if(removingLight)for(auto &entry:s.instanceLights) {
         auto &list=entry.second;const auto found=std::lower_bound(list.begin(),list.end(),id);
         if(found!=list.end() && *found==id){list.erase(found);++s.instanceLightsRevision;}
      }
      ++s.revision;return 1;
   }return 0;
}
int RenderSceneSetCamera(const RenderSceneCamera *camera) {
   if(!camera || !finite(camera->viewProjection,16) || !finite(camera->previousViewProjection,16) ||
      !finite(camera->position,3) || !std::isfinite(camera->exposure) || camera->exposure<=0 ||
      !std::isfinite(camera->ambient) || camera->ambient<0 || !std::isfinite(camera->lightScale) || camera->lightScale<0 ||
      !finite(camera->fogColor,3) || !finite(camera->nativeDepth,4) || !std::isfinite(camera->fogDistance) || camera->fogDistance<0 ||
      std::any_of(camera->fogColor,camera->fogColor+3,[](float value){return value<0;}))return 0;
   auto &s=dark::GetWorldScene();s.camera=*camera;s.hasCamera=true;++s.revision;return 1;
}
int RenderSceneGetCamera(RenderSceneCamera *camera) {
   const auto &s=dark::GetWorldScene();if(!camera || !s.hasCamera)return 0;
   *camera=s.camera;return 1;
}
int RenderSceneCameraFromRays(RenderSceneCamera *camera,const float position[3],
   const float forward[3],const float rightEdge[3],const float bottomEdge[3],float nearPlane,float farPlane) {
   if(!camera || !position || !forward || !rightEdge || !bottomEdge ||
      !finite(position,3) || !finite(forward,3) || !finite(rightEdge,3) || !finite(bottomEdge,3) ||
      !std::isfinite(nearPlane) || !std::isfinite(farPlane) || nearPlane<=0 || farPlane<=nearPlane)return 0;
   float basis[3][3];float length[3]{};
   for(int j=0;j<3;++j) {
      basis[0][j]=rightEdge[j]-forward[j];basis[1][j]=bottomEdge[j]-forward[j];basis[2][j]=forward[j];
      for(int i=0;i<3;++i)length[i]+=basis[i][j]*basis[i][j];
   }
   for(int i=0;i<3;++i)if(!std::isfinite(length[i]) || length[i]<1e-20f)return 0;
   for(int i=0;i<3;++i)for(int k=i+1;k<3;++k) {
      float dot=0;for(int j=0;j<3;++j)dot+=basis[i][j]*basis[k][j];
      if(std::abs(dot)>0.001f*std::sqrt(length[i]*length[k]))return 0;
   }
   RenderSceneCamera result=*camera;
   std::fill(result.viewProjection,result.viewProjection+16,0.0f);
   for(int row=0;row<3;++row) {
      const int matrixRow=row==2?3:row;
      for(int col=0;col<3;++col) {
         const float value=basis[row][col]/length[row];
         result.viewProjection[col*4+matrixRow]=value;
         result.viewProjection[12+matrixRow]-=value*position[col];
      }
   }
   const float depth=farPlane/(farPlane-nearPlane);
   for(int col=0;col<4;++col)result.viewProjection[col*4+2]=result.viewProjection[col*4+3]*depth;
   result.viewProjection[14]-=nearPlane*depth;
   if(!finite(result.viewProjection,16))return 0;
   std::copy(position,position+3,result.position);*camera=result;return 1;
}
void RenderSceneGetStats(RenderSceneStats *stats) {
   if(!stats)return;auto &s=dark::GetWorldScene();*stats={s.revision,
      uint32_t(s.textures.size()),uint32_t(s.materials.size()),uint32_t(s.meshes.size()),
      uint32_t(s.instances.size()),uint32_t(s.lights.size()),0,0};
   for(const auto &m:s.meshes){stats->vertices+=m.second.vertices.size();stats->triangles+=m.second.indices.size()/3;}
}
void RenderSceneClear() {
   auto &s=dark::GetWorldScene();s.instances.clear();s.instanceLights.clear();s.meshes.clear();s.materials.clear();s.textures.clear();s.lights.clear();
   s.hasCamera=false;s.camera={};s.recordMeshChange(0);++s.instanceLightsRevision;++s.materialsRevision;++s.revision;
}
}
