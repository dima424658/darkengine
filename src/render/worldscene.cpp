#include <render_scene.h>
#include <render_backend.h>
#include <portal.h>
#include <port.h>
#include <wrfunc.h>
#include <r3d.h>
#include <wrdbrend.h>
#include <config.h>
#include <mlight.h>
#include <objlight.h>
#include <missrend.h>
#include <texmem.h>
#include <refsys.h>
#include <osysbase.h>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <map>
#include <tuple>
#include <set>
#include <vector>
#include <tracy/Tracy.hpp>
#include "worldscene.h"
#include "worldscene_pbr.hpp"
#include <image_pixels.hpp>
#include "worldscene_lights.hpp"
extern "C" int portal_sky_id;
extern "C" BOOL bRenderNewSky;
extern "C" int mCellRefSys;
extern int num_light, num_dyn;
extern mls_multi_light light_data[];
extern "C" unsigned long long lgd3d_scene_palette_revision(grs_bitmap *bitmap);
extern "C" void lgd3d_scene_reset_texture_snapshots();
namespace {
bool imported;
bool worldRenderAttempted,worldRendered;
std::chrono::steady_clock::time_point nativePassStart{};
auto &lightHandles=*new std::vector<RenderSceneHandle>;
auto &lightOwners=*new std::vector<int32_t>;
auto &lightOwnerSeen=*new std::vector<unsigned char>;
struct TerrainMesh {RenderSceneHandle handle;std::vector<RenderSceneVertex> vertices;int width,height;};
// width/height are virtual UV dimensions; pixel dimensions describe storage.
struct TerrainMaterial {RenderSceneHandle handle;RenderSceneMaterial description;grs_bitmap *frame;int width,height,pixelWidth,pixelHeight;uint32_t terrainWidth,terrainHeight;std::vector<TerrainMesh> meshes;};
bool terrainSize(unsigned texture,grs_bitmap *frame,uint32_t &width,uint32_t &height) {
   // NewDark assigns material metadata to an animation's root, not each frame.
   auto *root=texture<TEXMEM_MAX && texmem_hnd[texture]?static_cast<grs_bitmap *>(texmem_hnd[texture]->DataPeek()):nullptr;
   if(root && root->w>0 && root->h>0 && ResImageTerrainSize(root,root->w,root->h,width,height))return true;
   return ResImageTerrainSize(frame,frame->w,frame->h,width,height);
}
// Database reset also runs from legacy AppOnExit after static destructors.
auto &terrainMaterials=*new std::unordered_map<unsigned,TerrainMaterial>;
struct TerrainFrame {RenderSceneHandle handle;unsigned long long paletteRevision;};
auto &terrainFrames=*new std::unordered_map<grs_bitmap *,TerrainFrame>;
struct CellLinks {uint64_t frame=0,revision=0,tableRevision=0;int staticCount=-1;bool valid=false;std::vector<unsigned short> source;std::vector<RenderSceneHandle> lights,scratch;};
auto &cellLinks=*new std::vector<CellLinks>;
uint64_t lightFrame=0,lightRevision=0,lightHandleRevision=0;
unsigned modelLinkUpdates=0;
bool cellLightLinking=true;
struct TerrainReceiver {RenderSceneHandle instance;std::vector<int> cells;std::vector<uint64_t> revisions;RenderSceneInstance description;bool cellRaster=false;};
auto &terrainReceivers=*new std::vector<TerrainReceiver>;
auto &visibleCells=*new std::vector<unsigned char>;
bool visibleCellsKnown=false;
bool portalVisibilityEnabled=false;
bool cellRasterReady=false;
CellLinks *nativeCellLinks(int cell) {
   if(!cellLightLinking || cell<0 || cell>=wr_num_cells || size_t(cell)>=cellLinks.size() || !WR_CELL(cell))return nullptr;
   auto &cached=cellLinks[cell];
   if(cached.frame==lightFrame)return cached.valid?&cached:nullptr;
   cached.frame=lightFrame;
   const auto *indices=WR_CELL(cell)->light_indices;
   bool valid=indices && indices[0]<=std::max(0,num_light);
   if(valid && cached.valid && cached.tableRevision==lightHandleRevision && cached.staticCount==num_light &&
      cached.source.size()==indices[0] && std::equal(cached.source.begin(),cached.source.end(),indices+1))return &cached;
   cached.tableRevision=lightHandleRevision;cached.staticCount=num_light;
   if(valid)cached.source.assign(indices+1,indices+1+indices[0]);else cached.source.clear();
   auto &linked=cached.scratch;linked.clear();
   if(valid)for(unsigned i=1;i<=indices[0];++i) {
      const auto index=indices[i];
      if(index>=unsigned(std::max(0,num_light)) || index>=lightHandles.size()){valid=false;break;}
      if(lightHandles[index])linked.push_back(lightHandles[index]);
   }
   if(valid)for(unsigned i=unsigned(std::max(0,num_light));i<lightHandles.size();++i)
      if(lightHandles[i])linked.push_back(lightHandles[i]);
   std::sort(linked.begin(),linked.end());linked.erase(std::unique(linked.begin(),linked.end()),linked.end());
   if(cached.revision==0 || cached.valid!=valid || cached.lights!=linked) {
      cached.revision=++lightRevision;cached.valid=valid;cached.lights.swap(linked);
   }
   return valid?&cached:nullptr;
}
void linkTerrain() {
   unsigned updated=0;
   for(auto &receiver:terrainReceivers) {
      bool visible=!receiver.cellRaster;
      if(portalVisibilityEnabled && visibleCellsKnown) {
         visible=false;
         if(receiver.cellRaster || !cellRasterReady)for(const int cell:receiver.cells)
            if(cell>=0 && size_t(cell)<visibleCells.size() && visibleCells[cell]){visible=true;break;}
      }
      if(receiver.description.visible!=unsigned(visible)) {
         receiver.description.visible=visible;
         if(!RenderSceneUpdateInstance(receiver.instance,&receiver.description))throw 1;
      }
      bool changed=receiver.revisions.size()!=receiver.cells.size(),valid=true;
      for(unsigned i=0;i<receiver.cells.size();++i) {
         auto *links=nativeCellLinks(receiver.cells[i]);valid=valid && links;
         const uint64_t revision=links?links->revision:0;
         changed=changed || i>=receiver.revisions.size() || receiver.revisions[i]!=revision;
      }
      if(!changed)continue;
      if(!valid) {
         if(!RenderSceneClearInstanceLights(receiver.instance))throw 1;
      }else {
         std::vector<RenderSceneHandle> lights;
         for(const auto cell:receiver.cells) {
            const auto &list=cellLinks.at(cell).lights;lights.insert(lights.end(),list.begin(),list.end());
         }
         if(!RenderSceneSetInstanceLights(receiver.instance,lights.data(),uint32_t(lights.size())))throw 1;
      }
      receiver.revisions.resize(receiver.cells.size());
      for(unsigned i=0;i<receiver.cells.size();++i) {
         auto *links=nativeCellLinks(receiver.cells[i]);receiver.revisions[i]=links?links->revision:0;
      }
      ++updated;
   }
   RenderBackendTrace("world-light-links cells=%zu terrain=%zu terrain_updates=%u model_updates=%u",
      cellLinks.size(),terrainReceivers.size(),updated,modelLinkUpdates);
}
float dot(const mxs_vector &a,const mxs_vector &b){return a.x*b.x+a.y*b.y+a.z*b.z;}
void importTerrain() {
   ZoneScopedN("Import world-space terrain");
   std::unordered_map<unsigned,RenderSceneHandle> materials;
   size_t nativeMediumBoundaries=0;
   struct Chunk {std::vector<RenderSceneVertex> vertices;std::vector<uint32_t> indices;std::set<int> cells;};
   // Spatial/material chunks avoid one BLAS and Vulkan allocation per polygon.
   std::map<std::tuple<int,int,int,RenderSceneHandle,bool>,Chunk> chunks;
   std::map<std::tuple<int,RenderSceneHandle,bool>,Chunk> cellChunks;
   for(int cellIndex=0;cellIndex<wr_num_cells;++cellIndex) {
      const auto *cell=WR_CELL(cellIndex);if(!cell)continue;
      size_t offset=0;
      for(unsigned polygonIndex=0;polygonIndex<cell->num_render_polys;++polygonIndex) {
         const auto &poly=cell->poly_list[polygonIndex];
         const size_t begin=offset;offset+=poly.num_vertices;
         if(poly.num_vertices<3 || offset>cell->num_vlist || poly.planeid>=cell->num_planes)continue;
         // Native translucent medium boundaries use animated mapping and an
         // alpha palette in the water queue. Importing them as opaque terrain
         // fills the water and makes it an incorrect shadow caster/receiver.
         if(poly.flags&PORTAL_SPLITS_OBJECT){++nativeMediumBoundaries;continue;}
         const auto &render=cell->render_list[polygonIndex];
         // Sky is an opening, never a wall in the shadow acceleration structure.
         if((bRenderNewSky && render.texture_id==0) || render.texture_id==portal_sky_id ||
            (render.texture_id>=BACKHACK_IDX && render.texture_id<TEXMEM_RESERVED_LIMIT))continue;
         auto *bitmap=portal_get_texture(render.texture_id);if(!bitmap || bitmap->w<=0 || bitmap->h<=0)continue;
         auto material=materials.find(render.texture_id);
         if(material==materials.end()) {
            RenderSceneMaterial desc{};desc.doubleSided=1;desc.baseColorTexture=WorldSceneCopyTexture(bitmap);
            desc.baseColor[0]=desc.baseColor[1]=desc.baseColor[2]=desc.baseColor[3]=1;
            desc.roughness=0.8f;desc.alphaCutoff=0.5f;
            desc.opacity=(bitmap->flags & BMF_TRANS)?RenderSceneMasked:RenderSceneOpaque;
            WorldSceneApplyPbrMaps(bitmap,desc);
            const auto handle=RenderSceneCreateMaterial(&desc);if(!handle)continue;
            uint32_t width=bitmap->w,height=bitmap->h;
            const bool authored=terrainSize(render.texture_id,bitmap,width,height);
            terrainMaterials.emplace(render.texture_id,TerrainMaterial{handle,desc,bitmap,int(width),int(height),bitmap->w,bitmap->h,authored?width:0,authored?height:0,{}});
            if(authored)RenderBackendTrace("world-terrain-scale texture=%u source=%s pixels=%dx%d virtual=%ux%u",unsigned(render.texture_id),ResImageSourceName(bitmap).c_str(),bitmap->w,bitmap->h,width,height);
            if(desc.baseColorTexture)terrainFrames.emplace(bitmap,TerrainFrame{desc.baseColorTexture,lgd3d_scene_palette_revision(bitmap)});
            material=materials.emplace(render.texture_id,handle).first;
         }
         if(render.texture_anchor>=poly.num_vertices)continue;
         const auto anchorIndex=cell->vertex_list[begin+render.texture_anchor];
         if(anchorIndex>=cell->num_vertices)continue;
         const auto &anchor=cell->vpool[anchorIndex];
         const float uu=dot(render.tex_u,render.tex_u),vv=dot(render.tex_v,render.tex_v),uv=dot(render.tex_u,render.tex_v);
         const float determinant=uu*vv-uv*uv;if(std::abs(determinant)<1e-15f)continue;
         const float textureScale=portal_worldrep_texture_scale>0?portal_worldrep_texture_scale:1;
         const auto &dimensions=terrainMaterials.at(render.texture_id);
         const float us=64.0f/(dimensions.width*textureScale),vs=64.0f/(dimensions.height*textureScale);
         const float ub=portal_wrext_u_base[cellIndex]?portal_wrext_u_base[cellIndex][polygonIndex]:render.u_base/4096.0f;
         const float vb=portal_wrext_v_base[cellIndex]?portal_wrext_v_base[cellIndex][polygonIndex]:render.v_base/4096.0f;
         std::vector<RenderSceneVertex> vertices;std::vector<uint32_t> indices;
         for(unsigned j=0;j<poly.num_vertices;++j) {
            const auto index=cell->vertex_list[begin+j];if(index>=cell->num_vertices)break;
            const auto &position=cell->vpool[index];
            mxs_vector delta;delta.x=position.x-anchor.x;delta.y=position.y-anchor.y;delta.z=position.z-anchor.z;
            const float du=dot(delta,render.tex_u),dv=dot(delta,render.tex_v);
            const auto &normal=cell->plane_list[poly.planeid].normal;
            vertices.push_back({{position.x,position.y,position.z},{normal.x,normal.y,normal.z},
               {us*(ub+(vv*du-uv*dv)/determinant),vs*(vb+(uu*dv-uv*du)/determinant)}});
         }
         if(vertices.size()!=poly.num_vertices)continue;
         for(unsigned j=2;j<vertices.size();++j){indices.push_back(0);indices.push_back(j-1);indices.push_back(j);}
         float center[3]{};
         for(const auto &v:vertices)for(int axis=0;axis<3;++axis)center[axis]+=v.position[axis]/vertices.size();
         auto &chunk=chunks[{int(std::floor(center[0]/64)),int(std::floor(center[1]/64)),int(std::floor(center[2]/64)),material->second,(cell->flags&CELL_FOG)!=0}];
         chunk.cells.insert(cellIndex);
         const auto base=uint32_t(chunk.vertices.size());
         chunk.vertices.insert(chunk.vertices.end(),vertices.begin(),vertices.end());
         for(const auto index:indices)chunk.indices.push_back(base+index);
         if(portalVisibilityEnabled) {
            auto &receiver=cellChunks[{cellIndex,material->second,(cell->flags&CELL_FOG)!=0}];
            receiver.cells.insert(cellIndex);
            const auto receiverBase=uint32_t(receiver.vertices.size());
            receiver.vertices.insert(receiver.vertices.end(),vertices.begin(),vertices.end());
            for(const auto index:indices)receiver.indices.push_back(receiverBase+index);
         }
      }
   }
   auto create=[&](Chunk &chunk,RenderSceneHandle materialId,bool fog,bool cellRaster) {
      RenderSceneMesh mesh{chunk.vertices.data(),uint32_t(chunk.vertices.size()),chunk.indices.data(),uint32_t(chunk.indices.size()),materialId,1};
      const auto handle=RenderSceneCreateMesh(&mesh);if(!handle)throw 1;
      for(auto &material:terrainMaterials)if(material.second.handle==mesh.material) {
         material.second.meshes.push_back({handle,chunk.vertices,material.second.width,material.second.height});break;
      }
      RenderSceneInstance instance{};instance.mesh=handle;instance.visible=!cellRaster;instance.castsShadow=!cellRaster;instance.receivesFog=fog;
      for(int j=0;j<4;++j)instance.transform[j*5]=instance.previousTransform[j*5]=1;
      const auto receiver=RenderSceneCreateInstance(&instance);
      if(!receiver)throw 1;
      terrainReceivers.push_back({receiver,{chunk.cells.begin(),chunk.cells.end()},{},instance,cellRaster});
   };
   for(auto &entry:chunks)create(entry.second,std::get<3>(entry.first),std::get<4>(entry.first),false);
   for(auto &entry:cellChunks)create(entry.second,std::get<1>(entry.first),std::get<2>(entry.first),true);
   cellRasterReady=!cellChunks.empty();
   RenderBackendTrace("world-terrain-layout casters=%zu cell_receivers=%zu",chunks.size(),cellChunks.size());
   RenderBackendTrace("world-terrain native_medium_boundaries=%zu",nativeMediumBoundaries);
   const auto fogChunks=std::count_if(chunks.begin(),chunks.end(),[](const auto &entry){return std::get<4>(entry.first);});
   std::fprintf(stderr,"World-space terrain fog: %zu flagged chunks, enabled=%d distance=%.3f\n",
      size_t(fogChunks),int(portal_fog_on!=0),double(portal_fog_dist));
}
}
extern "C" void WorldSceneLinkInstance(uint64_t instance,int cell,uint64_t *revision) {
   if(!instance || !revision)return;
   auto *links=nativeCellLinks(cell);
   if(!links) {
      if(*revision && RenderSceneClearInstanceLights(instance))*revision=0;
      return;
   }
   if(*revision!=links->revision && RenderSceneSetInstanceLights(instance,links->lights.data(),uint32_t(links->lights.size()))) {
      *revision=links->revision;++modelLinkUpdates;
   }
}
extern "C" void WorldSceneRegisterEmitter(int object,int nativeLightIndex,const float position[3]) {
   if(!object)return;
   const auto *mission=GetMissionRenderParams();
   for(unsigned i=0;i<lightHandles.size();++i) {
      if(!lightHandles[i] || (i==0 && mission->use_sun))continue;
      double squared=0;
      if(position)for(unsigned axis=0;axis<3;++axis) {
         const double delta=double(position[axis])-light_data[i].loc.el[axis];squared+=delta*delta;
      }
      if(int(i)!=nativeLightIndex && (!position || squared>1e-6))continue;
      lightOwnerSeen[i]=1;
      if(lightOwners[i]!=object){lightOwners[i]=object;RenderSceneSetLightEmitter(lightHandles[i],object);}
   }
}
extern "C" void WorldSceneReset() {
   cellRasterReady=false;
   portalVisibilityEnabled=false;
   visibleCellsKnown=false;visibleCells.clear();
   worldRenderAttempted=worldRendered=false;
   cellLinks.clear();terrainReceivers.clear();lightFrame=lightRevision=lightHandleRevision=0;
   WorldScenePbrReset();WorldSceneModelsReset();lgd3d_scene_reset_texture_snapshots();RenderSceneClear();imported=false;lightHandles.clear();lightOwners.clear();lightOwnerSeen.clear();terrainMaterials.clear();terrainFrames.clear();
}
extern "C" void WorldSceneCapture() {
   if((!config_is_defined("renderer_scene_capture") && !config_is_defined("renderer_scene_acceleration") && !RenderBackendWorldEnabled()) || wr_num_cells<=0)return;
   ZoneScopedN("Capture world-space scene");
   try {
      ++lightFrame;modelLinkUpdates=0;
      if(cellLinks.size()!=size_t(wr_num_cells))cellLinks.resize(size_t(wr_num_cells));
      int linkCells=1;config_get_int("renderer_cell_lights",&linkCells);cellLightLinking=linkCells!=0;
      const unsigned count=unsigned(std::max(0,num_light+num_dyn));
      lightOwners.resize(count);lightOwnerSeen.assign(count,0);
      for(unsigned i=0;i<count;++i) {
         const auto *mission=GetMissionRenderParams();
         auto light=WorldSceneConvertNativeLight(light_data[i],*mission,
            i==0 && mission->use_sun,i>=unsigned(std::max(0,num_light)));
         light.emitterObjectId=lightOwners[i];
         if(i>=lightHandles.size()){lightHandles.push_back(RenderSceneCreateLight(&light));++lightHandleRevision;}
         else if(!lightHandles[i]) {
            lightHandles[i]=RenderSceneCreateLight(&light);if(lightHandles[i])++lightHandleRevision;
         }else RenderSceneUpdateLight(lightHandles[i],&light);
      }
      while(lightHandles.size()>count){RenderSceneDestroy(lightHandles.back());lightHandles.pop_back();++lightHandleRevision;}
      if(!imported) {
         importTerrain();imported=true;
         RenderSceneStats stats{};RenderSceneGetStats(&stats);
         std::fprintf(stderr,"World-space scene: %u meshes, %llu triangles, %u materials, %u textures\n",
            stats.meshes,static_cast<unsigned long long>(stats.triangles),stats.materials,stats.textures);
      }
      WorldSceneCaptureModels();
      for(unsigned i=0;i<lightOwners.size();++i)if(lightOwners[i] && !lightOwnerSeen[i]) {
         lightOwners[i]=0;if(lightHandles[i])RenderSceneSetLightEmitter(lightHandles[i],0);
      }
      for(auto &entry:terrainMaterials) {
         auto &material=entry.second;auto *bitmap=portal_get_texture(entry.first);
         if(!bitmap || bitmap->w<=0 || bitmap->h<=0)continue;
         uint32_t width=bitmap->w,height=bitmap->h;
         if(material.terrainWidth){width=material.terrainWidth;height=material.terrainHeight;}
         else if(terrainSize(entry.first,bitmap,width,height)) {
            material.terrainWidth=width;material.terrainHeight=height;
         }
         const bool resized=int(width)!=material.width || int(height)!=material.height;
         const bool pixelsResized=bitmap->w!=material.pixelWidth || bitmap->h!=material.pixelHeight;
         const auto revision=lgd3d_scene_palette_revision(bitmap);
         auto found=terrainFrames.find(bitmap);
         const bool fresh=found!=terrainFrames.end() && found->second.paletteRevision==revision;
         if(bitmap==material.frame && !resized && !pixelsResized && fresh)continue;
         if(!fresh || pixelsResized) {
            const auto texture=WorldSceneCopyTexture(bitmap);if(!texture)continue;
            terrainFrames[bitmap]={texture,revision};found=terrainFrames.find(bitmap);
         }
         if(resized) {
            // Always derive UVs from import values, avoiding accumulated error
            // when animation repeatedly alternates between frame dimensions.
            for(const auto &mesh:material.meshes) {
               auto vertices=mesh.vertices;
               for(auto &vertex:vertices){vertex.uv[0]*=float(mesh.width)/width;vertex.uv[1]*=float(mesh.height)/height;}
               if(!RenderSceneUpdateVertices(mesh.handle,vertices.data(),uint32_t(vertices.size())))throw 1;
            }
         }
         auto description=material.description;description.baseColorTexture=found->second.handle;
         description.opacity=(bitmap->flags&BMF_TRANS)?RenderSceneMasked:RenderSceneOpaque;
         WorldSceneApplyPbrMaps(bitmap,description);
         if(RenderSceneUpdateMaterial(material.handle,&description)){material.description=description;material.frame=bitmap;material.width=width;material.height=height;material.pixelWidth=bitmap->w;material.pixelHeight=bitmap->h;}
      }
      linkTerrain();
   }catch(...){WorldSceneReset();std::fprintf(stderr,"World-space scene import failed\n");}
}

extern "C" int WorldSceneRender() {
   if(worldRenderAttempted)return worldRendered;
   const auto start=std::chrono::steady_clock::now();
   WorldSceneCapture();
   const auto captured=std::chrono::steady_clock::now();
   const bool rendered=RenderBackendRenderWorld()!=0;
   if(rendered)RenderBackendTrace("world-capture cpu_ms=%.3f preceding_native_ms=%.3f",
      std::chrono::duration<double,std::milli>(captured-start).count(),
      nativePassStart.time_since_epoch().count()?std::chrono::duration<double,std::milli>(start-nativePassStart).count():-1);
   nativePassStart={};
   worldRenderAttempted=true;worldRendered=rendered;
   return rendered;
}
extern "C" void WorldSceneCaptureCamera() {
   visibleCellsKnown=false;
   int visibility=0;config_get_int("renderer_portal_visibility",&visibility);portalVisibilityEnabled=visibility!=0;
   worldRenderAttempted=worldRendered=false;
   nativePassStart=std::chrono::steady_clock::now();
   const bool enabled=config_is_defined("renderer_scene_capture") || config_is_defined("renderer_scene_acceleration") || RenderBackendWorldEnabled();
   WorldSceneBeginObjectCapture(enabled);
   if(!enabled)return;
   auto *canvas=r3_get_view_canvas();if(!canvas || canvas->bm.w<=0 || canvas->bm.h<=0)return;
   mxs_vector forward,right,bottom;
   r3_get_view_vec(&forward,canvas->bm.w*0.5f,canvas->bm.h*0.5f);
   r3_get_view_vec(&right,float(canvas->bm.w),canvas->bm.h*0.5f);
   r3_get_view_vec(&bottom,canvas->bm.w*0.5f,float(canvas->bm.h));
   const auto *position=r3_get_view_pos();
   const float p[]={position->x,position->y,position->z};
   const float f[]={forward.x,forward.y,forward.z},r[]={right.x,right.y,right.z},b[]={bottom.x,bottom.y,bottom.z};
   RenderSceneCamera camera{};const bool previous=RenderSceneGetCamera(&camera)!=0;
   float previousMatrix[16];std::copy(camera.viewProjection,camera.viewProjection+16,previousMatrix);
   camera.exposure=1;camera.ambient=0.02f;camera.lightScale=1;
   config_get_float("renderer_exposure",&camera.exposure);
   config_get_float("renderer_ambient",&camera.ambient);
   config_get_float("renderer_light_scale",&camera.lightScale);
   if(!RenderSceneCameraFromRays(&camera,p,f,r,b,0.05f,65536.0f))return;
   int portalDrawing=1;config_get_int("renderer_portal_rendering",&portalDrawing);
   if(!portalDrawing) {
      const float scale=r3d_state.cur_con->trans.cspace_vec.z;
      if(std::isfinite(scale) && scale>0) {
         RenderBackendGetNativeDepthEncoding(scale,camera.nativeDepth);
         camera.nativeDepth[3]=1;
      }
   }

   std::copy(previous?previousMatrix:camera.viewProjection,(previous?previousMatrix:camera.viewProjection)+16,camera.previousViewProjection);
   camera.fogDistance=portal_fog_on && std::isfinite(portal_fog_dist) && portal_fog_dist>0?portal_fog_dist:0;
   for(unsigned i=0;i<3;++i) {
      const float value=std::min<ulong>(255,portal_fog_color[i])/255.0f;
      camera.fogColor[i]=value<=.04045f?value/12.92f:std::pow((value+.055f)/1.055f,2.4f);
   }
   RenderSceneSetCamera(&camera);
}
extern "C" void WorldSceneCaptureVisibleCells(const int *cells,unsigned count) {
   visibleCellsKnown=false;
   if(!portalVisibilityEnabled)return;
   if(!cells || !count || wr_num_cells<=0 || count>unsigned(wr_num_cells))return;
   visibleCells.assign(size_t(wr_num_cells),0);
   for(unsigned i=0;i<count;++i) {
      if(cells[i]<0 || cells[i]>=wr_num_cells){visibleCells.clear();return;}
      visibleCells[cells[i]]=1;
   }
   visibleCellsKnown=true;
}
extern "C" int WorldSceneObjectVisible(int object) {
   if(!portalVisibilityEnabled || !visibleCellsKnown || object<=0 || object>=gMaxObjID ||
      mCellRefSys<0 || mCellRefSys>=mNumRefSystems || !gFirstRefs || !gFirstRefs[mCellRefSys] ||
      !ObjRefInfos || !mRefSystems || mRefSystems[mCellRefSys].bin_size!=sizeof(int))return 1;
   const auto first=OBJ_FIRST_REF(object,mCellRefSys);
   // Native portal object discovery cannot draw an object with no cell refs.
   // Attachments are kept conservatively by the caller instead.
   if(!first)return 0;
   auto ref=first;
   for(int checked=0;checked<gMaxRefs;++checked) {
      if(!ref || ref>=unsigned(gMaxRefs))return 1;
      const auto *entry=OBJREFID_TO_PTR(ref);
      if(!entry || entry->obj!=object || entry->refsys!=unsigned(mCellRefSys))return 1;
      int cell;std::memcpy(&cell,entry->bin,sizeof(cell));
      if(cell<0 || size_t(cell)>=visibleCells.size())return 1;
      if(visibleCells[cell])return 1;
      ref=entry->next_of_obj;if(ref==first)return 0;
   }
   return 1; // Malformed cycles retain conservative drawing.
}
