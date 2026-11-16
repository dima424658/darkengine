#include "world_shadows.hpp"
#include <algorithm>
#include <cmath>
namespace dark {
ShadowCameraData BuildShadowCameraData(const ShadowPlan &plan,const ShadowSettings &settings) {
   ShadowCameraData data{};
   std::copy(plan.cameraDepthPlane,plan.cameraDepthPlane+4,data.depthPlane);
   data.filter=static_cast<uint32_t>(settings.filter);
   data.blockerSamples=std::clamp(settings.blockerSamples,1u,64u);
   data.filterSamples=std::clamp(settings.filterSamples,1u,64u);
   return data;
}
uint32_t ShadowLightFaceCount(const RenderSceneLight &light) {
   return light.type==RenderScenePointLight || (light.type==RenderSceneSpotLight && light.outerCos<=0)?6u:1u;
}
ShadowSelection SelectShadowLights(const WorldScene &scene,const ShadowSettings &settings,const ShadowSelection &previous) {
   if(!scene.hasCamera)return {};
   struct Candidate {RenderSceneHandle id;double score;uint32_t faces;};
   std::vector<Candidate> local,directional;
   const std::unordered_set<RenderSceneHandle> retained(previous.localLights.begin(),previous.localLights.end());
   // Native cell reach is also a shadow-budget constraint. Otherwise bright
   // unbounded lights behind walls can displace the torch illuminating a visible
   // receiver. An unrestricted visible receiver keeps the conservative fallback.
   std::unordered_set<RenderSceneHandle> receiverLights;
   bool restrictReceivers=!scene.instanceLights.empty();
   if(restrictReceivers) {
      const auto frustum=SceneExtractFrustum(scene.camera.viewProjection);
      for(const auto &entry:scene.instances) {
         const auto &instance=entry.second;if(!instance.visible)continue;
         const auto &mesh=scene.meshes.at(instance.mesh);
         if(!SceneBoundsVisible(SceneTransformBounds(mesh.bounds,instance.transform),frustum))continue;
         const auto links=scene.instanceLights.find(entry.first);
         if(links==scene.instanceLights.end()){restrictReceivers=false;break;}
         receiverLights.insert(links->second.begin(),links->second.end());
      }
   }
   constexpr float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
   for(const auto &entry:scene.lights) {
      const auto &l=entry.second;
      if(!l.enabled || !l.castsShadow || l.intensity<=0)continue;
      const double luminance=.2126*l.color[0]+.7152*l.color[1]+.0722*l.color[2];
      if(luminance<=0)continue;
      if(l.type==RenderSceneDirectionalLight) {
         directional.push_back({entry.first,luminance*l.intensity*(previous.sunlight==entry.first?1.2:1.0),0});continue;
      }
      if(restrictReceivers && !receiverLights.count(entry.first))continue;
      if(scene.hasCamera && l.range>0) {
         SceneBounds sphere;
         for(unsigned i=0;i<3;++i){sphere.minimum[i]=l.position[i]-l.range;sphere.maximum[i]=l.position[i]+l.range;}
         if(!SceneBoundsVisible(sphere,identity,scene.camera.viewProjection))continue;
      }
      double distance=0;for(unsigned i=0;i<3;++i){const double delta=double(l.position[i])-scene.camera.position[i];distance+=delta*delta;}
      // Intensity/distance estimates the contribution to nearby visible surfaces.
      // A finite influence radius also weights projected extent conservatively.
      double area=l.range>0?std::min(1.0,double(l.range)*l.range/std::max(distance,1.0)):1;
      const double score=luminance*l.intensity*area/(1+distance)*(retained.count(entry.first)?1.2:1.0);
      local.push_back({entry.first,score,ShadowLightFaceCount(l)});
   }
   auto order=[](const Candidate &a,const Candidate &b){return a.score!=b.score?a.score>b.score:a.id<b.id;};
   std::sort(local.begin(),local.end(),order);std::sort(directional.begin(),directional.end(),order);
   ShadowSelection result;
   result.eligibleLocalLights=uint32_t(local.size());
   if(settings.sunCascades && !directional.empty())result.sunlight=directional.front().id;
   for(const auto &candidate:local) {
      if(result.localLights.size()>=settings.localLights)break;
      if(candidate.faces>settings.localFaces-result.localFaces)continue;
      result.localLights.push_back(candidate.id);result.localFaces+=candidate.faces;
   }
   uint32_t cachedFaces=0;
   for(const auto &candidate:local) {
      if(result.cachedLights.size()>=settings.cachedLights)break;
      if(std::find(result.localLights.begin(),result.localLights.end(),candidate.id)!=result.localLights.end())continue;
      if(candidate.faces>settings.cachedFaces-cachedFaces)continue;
      result.cachedLights.push_back(candidate.id);cachedFaces+=candidate.faces;
   }
   return result;
}
namespace {
struct Vec {
   double x=0,y=0,z=0;
   Vec operator+(Vec b) const{return {x+b.x,y+b.y,z+b.z};}
   Vec operator-(Vec b) const{return {x-b.x,y-b.y,z-b.z};}
   Vec operator*(double v) const{return {x*v,y*v,z*v};}
};
double dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
Vec cross(Vec a,Vec b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
Vec unit(Vec v){const double length=std::sqrt(dot(v,v));return length>1e-12?v*(1/length):Vec{};}
Vec vec(const float *v){return {v[0],v[1],v[2]};}
void store(float *v,Vec value){v[0]=float(value.x);v[1]=float(value.y);v[2]=float(value.z);}
void basis(Vec forward,Vec &right,Vec &down) {
   const Vec up=std::abs(forward.z)<.99?Vec{0,0,1}:Vec{0,1,0};
   right=unit(cross(forward,up));down=unit(cross(forward,right));
}
bool inverse(const float *matrix,double result[16]) {
   double rows[4][8]{};
   for(int r=0;r<4;++r)for(int c=0;c<4;++c){rows[r][c]=matrix[c*4+r];rows[r][c+4]=r==c;}
   for(int col=0;col<4;++col) {
      int pivot=col;for(int r=col+1;r<4;++r)if(std::abs(rows[r][col])>std::abs(rows[pivot][col]))pivot=r;
      if(std::abs(rows[pivot][col])<1e-20)return false;
      for(int c=0;c<8;++c)std::swap(rows[pivot][c],rows[col][c]);
      const double divisor=rows[col][col];for(double &value:rows[col])value/=divisor;
      for(int r=0;r<4;++r)if(r!=col) {
         const double factor=rows[r][col];for(int c=0;c<8;++c)rows[r][c]-=factor*rows[col][c];
      }
   }
   for(int r=0;r<4;++r)for(int c=0;c<4;++c)result[c*4+r]=rows[r][c+4];
   return true;
}
Vec unproject(const double *matrix,double x,double y,double z) {
   const double clip[4]={x,y,z,1};double world[4]{};
   for(int r=0;r<4;++r)for(int c=0;c<4;++c)world[r]+=matrix[c*4+r]*clip[c];
   return {world[0]/world[3],world[1]/world[3],world[2]/world[3]};
}
struct CasterBounds {SceneOrientedBounds oriented;double extent[3];};
std::vector<CasterBounds> casterBounds(const WorldScene &scene) {
   std::vector<CasterBounds> result;result.reserve(scene.instances.size());
   for(const auto &entry:scene.instances) {
      const auto &instance=entry.second;if(!instance.castsShadow && !instance.visible)continue;
      const auto &mesh=scene.meshes.at(instance.mesh);
      if(!instance.visible && scene.materials.at(mesh.material).opacity==RenderSceneBlended)continue;
      CasterBounds box{};box.oriented=SceneTransformBounds(mesh.bounds,instance.transform);
      for(unsigned axis=0;axis<3;++axis)for(const auto &vector:box.oriented.axes)box.extent[axis]+=std::abs(vector[axis]);
      result.push_back(box);
   }return result;
}
// A hierarchy over exact oriented bounds accelerates farthest-corner queries.
// It is shared by active/cached pools within a plan, never retained across poses.
class CasterBoundsQuery {
   struct Node {
      double low[3],high[3];size_t begin,end,left=0,right=0;
   };
   std::vector<Node> nodes;
   const WorldScene &scene;
   std::vector<CasterBounds> boxes;
   bool collected=false,indexed=false;
   size_t build(size_t begin,size_t end) {
      Node node{};node.begin=begin;node.end=end;
      for(unsigned axis=0;axis<3;++axis){node.low[axis]=INFINITY;node.high[axis]=-INFINITY;}
      for(size_t i=begin;i<end;++i)for(unsigned axis=0;axis<3;++axis) {
         node.low[axis]=std::min(node.low[axis],double(boxes[i].oriented.center[axis])-boxes[i].extent[axis]);
         node.high[axis]=std::max(node.high[axis],double(boxes[i].oriented.center[axis])+boxes[i].extent[axis]);
      }
      const size_t index=nodes.size();nodes.push_back(node);
      if(end-begin>8) {
         unsigned axis=0;for(unsigned i=1;i<3;++i)if(node.high[i]-node.low[i]>node.high[axis]-node.low[axis])axis=i;
         const size_t middle=begin+(end-begin)/2;
         std::nth_element(boxes.begin()+begin,boxes.begin()+middle,boxes.begin()+end,
            [axis](const auto &a,const auto &b){return a.oriented.center[axis]<b.oriented.center[axis];});
         const auto left=build(begin,middle),right=build(middle,end);
         nodes[index].left=left;nodes[index].right=right;
      }
      return index;
   }
   static double upper(const Node &node,Vec origin) {
      const double p[3]={origin.x,origin.y,origin.z};double squared=0;
      for(unsigned axis=0;axis<3;++axis) {
         const double distance=std::max(std::abs(node.low[axis]-p[axis]),std::abs(node.high[axis]-p[axis]));
         squared+=distance*distance;
      }
      return squared;
   }
   void corners(size_t begin,size_t end,Vec origin,double &farthest) const {
      for(size_t i=begin;i<end;++i) {
         const auto &box=boxes[i];const auto &b=box.oriented;
         const double delta[3]={b.center[0]-origin.x,b.center[1]-origin.y,b.center[2]-origin.z};
         double upperSquared=0;
         for(unsigned axis=0;axis<3;++axis) {
            const double maximum=std::abs(delta[axis])+box.extent[axis];upperSquared+=maximum*maximum;
         }
         if(upperSquared<farthest*(1-1e-12))continue;
         for(unsigned corner=0;corner<8;++corner) {
            double squared=0;
            for(unsigned axis=0;axis<3;++axis) {
               double value=delta[axis];
               for(unsigned j=0;j<3;++j)value+=(corner&(1u<<j)?1:-1)*b.axes[j][axis];
               squared+=value*value;
            }
            farthest=std::max(farthest,squared);
         }
      }
   }
   void query(size_t index,Vec origin,double &farthest) const {
      const auto &node=nodes[index];
      // Retain the rounding margin used by the exact leaf pruning.
      if(upper(node,origin)<farthest*(1-1e-12))return;
      if(!node.left){corners(node.begin,node.end,origin,farthest);return;}
      auto first=node.left,second=node.right;
      if(upper(nodes[first],origin)<upper(nodes[second],origin))std::swap(first,second);
      query(first,origin,farthest);query(second,origin,farthest);
   }
public:
   explicit CasterBoundsQuery(const WorldScene &source):scene(source) {}
   const std::vector<CasterBounds> &all() {
      if(!collected){boxes=casterBounds(scene);collected=true;}
      return boxes;
   }
   double farthestSquared(Vec origin) {
      all();
      if(!indexed) {
         if(boxes.size()>=32){nodes.reserve(boxes.size()/2);build(0,boxes.size());}
         indexed=true;
      }
      double farthest=0;
      if(nodes.empty())corners(0,boxes.size(),origin,farthest);else query(0,origin,farthest);
      return farthest;
   }
};
bool perspective(ShadowView &view,Vec origin,Vec forward,Vec right,Vec down,double extent,float nearDistance,float farDistance) {
   RenderSceneCamera camera{};
   float p[3],f[3],r[3],b[3];store(p,origin);store(f,forward);store(r,forward+right*extent);store(b,forward+down*extent);
   if(!RenderSceneCameraFromRays(&camera,p,f,r,b,nearDistance,farDistance))return false;
   std::copy(camera.viewProjection,camera.viewProjection+16,view.viewProjection);
   store(view.originNear,origin);view.originNear[3]=nearDistance;
   store(view.depthPlane,forward);view.depthPlane[3]=float(-dot(origin,forward));view.parameters[0]=farDistance;return true;
}
}
static bool buildShadowPlan(const WorldScene &scene,const ShadowSettings &settings,const ShadowSelection &selection,ShadowPlan &plan,const ShadowPlan *previous,CasterBoundsQuery &bounds) {
   ShadowPlan cachedPrevious;
   if(previous && !selection.cachedLights.empty()) {
      cachedPrevious=*previous;
      for(auto &view:cachedPrevious.views)view.metadata[1]&=~ShadowCachedLayer;
      for(auto it=cachedPrevious.firstView.begin();it!=cachedPrevious.firstView.end();) {
         const auto index=it->second;
         if(index>=previous->views.size() || !(previous->views[index].metadata[1]&ShadowCachedLayer))it=cachedPrevious.firstView.erase(it);
         else ++it;
      }
   }
   std::unordered_map<RenderSceneHandle,uint32_t> layers;
   std::vector<unsigned char> used(std::min(settings.localFaces,ShadowSettings::maxCachedFaces),0);
   if(previous)for(const auto id:selection.localLights) {
      const auto light=scene.lights.find(id);const auto old=previous->firstView.find(id);
      if(light==scene.lights.end() || old==previous->firstView.end() || old->second>=previous->views.size())continue;
      const auto &view=previous->views[old->second];const auto count=ShadowLightFaceCount(light->second);
      const uint32_t layer=view.metadata[1];
      if(view.metadata[0]==2 || view.metadata[3]!=count || layer>used.size() || count>used.size()-layer)continue;
      if(std::any_of(used.begin()+layer,used.begin()+layer+count,[](auto value){return value!=0;}))continue;
      std::fill(used.begin()+layer,used.begin()+layer+count,1);layers[id]=layer;
   }
   plan={};
   if(!scene.hasCamera || !settings.localResolution || !settings.sunResolution || settings.sunCascades>4 ||
      !std::isfinite(settings.shadowDistance) || settings.shadowDistance<=.1f)return false;
   std::vector<RenderSceneHandle> locals=selection.localLights;std::sort(locals.begin(),locals.end());
   if(std::adjacent_find(locals.begin(),locals.end())!=locals.end())return false;
   uint32_t total=0;
   for(const auto id:locals) {
      const auto light=scene.lights.find(id);if(light==scene.lights.end())return false;
      const auto count=ShadowLightFaceCount(light->second);
      if(total>used.size() || count>used.size()-total)return false;total+=count;
      if(layers.count(id))continue;
      const auto free=std::search_n(used.begin(),used.end(),count,0);
      if(free==used.end()){layers.clear();break;}
      layers[id]=uint32_t(free-used.begin());std::fill(free,free+count,1);
   }
   // Retained one-face spots can fragment the free space needed by a cube.
   // A compact fallback always fits a selection whose total face count fits.
   if(layers.size()!=locals.size()) {
      layers.clear();uint32_t next=0;
      for(const auto id:locals) {
         const auto light=scene.lights.find(id);if(light==scene.lights.end())return false;
         const auto count=ShadowLightFaceCount(light->second);
         if(next>used.size() || count>used.size()-next)return false;
         layers[id]=next;next+=count;
      }
   }
   uint32_t layer=0;
   for(const auto id:locals) {
      const auto found=scene.lights.find(id);if(found==scene.lights.end())return false;
      const auto &light=found->second;const Vec origin=vec(light.position);
      const uint32_t count=ShadowLightFaceCount(light),first=uint32_t(plan.views.size());
      layer=layers.at(id);
      double farDistance=1;
      if(light.range>0)farDistance=std::max(.001,double(light.range));
      else {
         farDistance=std::sqrt(bounds.farthestSquared(origin))+1;
      }
      const float nearDistance=float(std::min(.05,farDistance*.01));
      static constexpr Vec directions[6]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
      for(unsigned face=0;face<count;++face) {
         ShadowView view{};Vec forward=count==6?directions[face]:unit(vec(light.direction)),right,down;basis(forward,right,down);
         const double extent=count==6?1:std::tan(std::acos(std::clamp(double(light.outerCos),.0001,.999999)));
         if(!perspective(view,origin,forward,right,down,extent,nearDistance,float(farDistance)))return false;
         view.parameters[1]=float(settings.localResolution);view.parameters[2]=light.sourceRadius;
         view.metadata[0]=count==6?0:1;view.metadata[1]=layer++;view.metadata[2]=first;view.metadata[3]=count;
         plan.views.push_back(view);plan.emitterObjects.push_back(light.emitterObjectId);
      }
      plan.firstView.emplace(id,first);
   }
   const float *camera=scene.camera.viewProjection;
   Vec cameraForward=unit({camera[3],camera[7],camera[11]});
   if(dot(cameraForward,cameraForward)<.5)return false;
   store(plan.cameraDepthPlane,cameraForward);plan.cameraDepthPlane[3]=float(-dot(cameraForward,vec(scene.camera.position)));
   if(!selection.cachedLights.empty()) {
      auto cachedSettings=settings;cachedSettings.localLights=settings.cachedLights;
      cachedSettings.localFaces=settings.cachedFaces;cachedSettings.localResolution=settings.cachedResolution;
      cachedSettings.cachedLights=cachedSettings.cachedFaces=0;cachedSettings.sunCascades=0;
      ShadowSelection cachedSelection;cachedSelection.localLights=selection.cachedLights;
      ShadowPlan cachedPlan;
      if(!buildShadowPlan(scene,cachedSettings,cachedSelection,cachedPlan,&cachedPrevious,bounds))return false;
      const uint32_t offset=uint32_t(plan.views.size());
      for(auto view:cachedPlan.views) {
         view.metadata[1]|=ShadowCachedLayer;view.metadata[2]+=offset;plan.views.push_back(view);
      }
      plan.emitterObjects.insert(plan.emitterObjects.end(),cachedPlan.emitterObjects.begin(),cachedPlan.emitterObjects.end());
      for(const auto &entry:cachedPlan.firstView)if(!plan.firstView.emplace(entry.first,entry.second+offset).second)return false;
   }
   if(!selection.sunlight || !settings.sunCascades)return true;
   const auto light=scene.lights.find(selection.sunlight);if(light==scene.lights.end())return false;
   const Vec forward=unit(vec(light->second.direction));Vec right,down;basis(forward,right,down);
   if(dot(forward,forward)<.5)return false;
   double inv[16];if(!inverse(camera,inv))return false;
   const Vec eye=vec(scene.camera.position);
   Vec rays[4];for(unsigned i=0;i<4;++i)rays[i]=unproject(inv,i&1?1:-1,i&2?1:-1,0)-eye;
   const Vec nearPoint=unproject(inv,0,0,0),farPoint=unproject(inv,0,0,1);
   const double nearDistance=std::max(.001,dot(nearPoint-eye,cameraForward));
   const double farDistance=std::min(double(settings.shadowDistance),dot(farPoint-eye,cameraForward));
   if(!std::isfinite(farDistance) || farDistance<=nearDistance)return false;
   double casterLow=INFINITY,casterHigh=-INFINITY;
   for(const auto &box:bounds.all()) {
      const auto &b=box.oriented;
      const double center=b.center[0]*forward.x+b.center[1]*forward.y+b.center[2]*forward.z;
      double radius=0;
      for(const auto &axis:b.axes)radius+=std::abs(axis[0]*forward.x+axis[1]*forward.y+axis[2]*forward.z);
      casterLow=std::min(casterLow,center-radius);casterHigh=std::max(casterHigh,center+radius);
   }
   const uint32_t first=uint32_t(plan.views.size());double splitNear=nearDistance;
   for(unsigned cascade=0;cascade<settings.sunCascades;++cascade) {
      const double fraction=double(cascade+1)/settings.sunCascades;
      const double splitFar=.7*nearDistance*std::pow(farDistance/nearDistance,fraction)+.3*(nearDistance+(farDistance-nearDistance)*fraction);
      // Overlap cascades for a smooth receiver transition at their boundary.
      const double overlapNear=cascade?std::max(nearDistance,splitNear-(splitFar-splitNear)*.1):splitNear;
      Vec receiver[8],center;
      for(unsigned i=0;i<8;++i) {
         const Vec ray=rays[i%4];const double depth=dot(ray,cameraForward);if(depth<=0)return false;
         receiver[i]=eye+ray*((i<4?overlapNear:splitFar)/depth);center=center+receiver[i]*(1.0/8);
      }
      double radius=0;for(auto point:receiver)radius=std::max(radius,std::sqrt(dot(point-center,point-center)));
      radius=std::ceil(radius*16)/16;
      // One texel border keeps the sphere inside the snapped footprint.
      radius*=1+2.0/settings.sunResolution;const double texel=2*radius/settings.sunResolution;
      const double x=std::floor(dot(center,right)/texel+.5)*texel,y=std::floor(dot(center,down)/texel+.5)*texel;
      double low=dot(center,forward)-radius,high=dot(center,forward)+radius;
      low=std::min(low,casterLow);high=std::max(high,casterHigh);
      const Vec origin=right*x+down*y+forward*(low-1);
      ShadowView view{};const double depth=high-low+2;
      for(unsigned c=0;c<3;++c) {
         const double r[3]={right.x,right.y,right.z},d[3]={down.x,down.y,down.z},f[3]={forward.x,forward.y,forward.z};
         view.viewProjection[c*4]=float(r[c]/radius);view.viewProjection[c*4+1]=float(d[c]/radius);view.viewProjection[c*4+2]=float(f[c]/depth);
      }
      view.viewProjection[12]=float(-x/radius);view.viewProjection[13]=float(-y/radius);
      view.viewProjection[14]=float(-(low-1)/depth);view.viewProjection[15]=1;
      store(view.originNear,origin);store(view.depthPlane,forward);view.depthPlane[3]=float(-dot(origin,forward));
      view.parameters[0]=float(depth);view.parameters[1]=float(settings.sunResolution);view.parameters[2]=light->second.angularRadius;view.parameters[3]=float(splitFar);
      view.metadata[0]=2;view.metadata[1]=cascade;view.metadata[2]=first;view.metadata[3]=settings.sunCascades;
      plan.views.push_back(view);plan.emitterObjects.push_back(light->second.emitterObjectId);splitNear=splitFar;
   }
   plan.firstView.emplace(selection.sunlight,first);return true;
}
bool BuildShadowPlan(const WorldScene &scene,const ShadowSettings &settings,const ShadowSelection &selection,ShadowPlan &plan,const ShadowPlan *previous) {
   if(!scene.hasCamera || !settings.localResolution || !settings.sunResolution || settings.sunCascades>4 ||
      !std::isfinite(settings.shadowDistance) || settings.shadowDistance<=.1f){plan={};return false;}
   CasterBoundsQuery bounds(scene);
   return buildShadowPlan(scene,settings,selection,plan,previous,bounds);
}

}
