// Native rigid/articulated and weighted character model adapter. Geometry stays
// in object space; instance transforms carry object movement. MD scale follows
// the native convention (scale vertices/pivots before applying joint motion).
#include <render_scene.h>
#include <render_backend.h>
#include <appagg.h>
#include <iobjsys.h>
#include <objquery.h>
#include <objdef.h>
#include <objpos.h>
#include <wrtype.h>
#include <wrdb.h>
#include <wrfunc.h>
#include <mnumprop.h>
#include <objmodel.h>
#include <objscale.h>
#include <rendprop.h>
#include <litprop.h>
#include <litprops.h>
#include <vhot.h>
#include <plycbllm.h>
#include <playrobj.h>
#include <camera.h>
#include <weapbow.h>
#include <md.h>
#include <mm.h>
#include <creatext.h>
#include <meshtex.h>
#include <meshatt.h>
#include <meshprop.h>
#include <contain.h>
#include <invtype.h>
#include <matrix.h>
#include <grd.h>
#include <2d.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <tracy/Tracy.hpp>
#include "worldscene.h"
#include "worldscene_properties.hpp"
extern "C" BOOL FindVhotLocation(mxs_vector *loc,ObjID obj,int vhot);
extern "C" RenderSceneHandle lgd3d_scene_copy_texture(grs_bitmap *bitmap);
extern "C" unsigned long long lgd3d_scene_palette_revision(grs_bitmap *bitmap);
namespace {
struct Part {
   RenderSceneHandle material=0, mesh=0, instance=0;
   uint64_t lightRevision=0;
   RenderSceneMaterial description{};
   RenderSceneInstance placement{};
   std::vector<RenderSceneVertex> vertices;
   std::vector<uint32_t> indices;
};
struct SegmentPose {mxs_trans transform;quat rotation;};
struct Model {
   const void *source=nullptr;size_t sourceSize=0;unsigned long long sourceRevision=0;
   int index=-1, type=OM_TYPE_INVALID;
   bool seen=false,receivesFog=false,lightCellKnown=false;
   int lightCell=-1;mxs_vector lightCellPosition{};
   mxs_vector scale{1,1,1};
   std::array<float,MAX_REND_JOINTS> joints{};
   std::vector<SegmentPose> segments;
   WorldSceneMaterialProperties materialProperties;
   std::map<unsigned,Part> parts;
};
// These caches must survive legacy AppOnExit database-reset callbacks.
struct CopiedTexture {RenderSceneHandle handle;unsigned long long paletteRevision;};
struct Models {
   std::unordered_map<ObjID,Model> objects;
   std::unordered_map<grs_bitmap *,CopiedTexture> textures;
   bool reported=false,captureFog=false;
   std::unordered_map<ObjID,unsigned char> objectFog;
};
Models &cache(){static auto *value=new Models;return *value;}
bool objectReceivesFog(ObjID obj,bool fallback) {
   const auto found=cache().objectFog.find(obj);
   // Separate portal fragments can disagree. Until geometry is split for fog,
   // a clear fragment keeps the complete instance clear regardless of order.
   return found==cache().objectFog.end()?fallback:found->second==2;
}
void destroy(Model &model) {
   for(auto &entry:model.parts) {
      auto &p=entry.second;
      RenderSceneDestroy(p.instance);RenderSceneDestroy(p.mesh);RenderSceneDestroy(p.material);
   }
   model=Model{};
}
RenderSceneHandle texture(grs_bitmap *bitmap) {
   if(!bitmap)return 0;
   auto &textures=cache().textures;
   const auto revision=lgd3d_scene_palette_revision(bitmap);
   auto it=textures.find(bitmap);
   if(it!=textures.end() && it->second.paletteRevision==revision)return it->second.handle;
   auto handle=lgd3d_scene_copy_texture(bitmap);
   if(handle)textures[bitmap]={handle,revision};
   return handle;
}
void refreshProperties(ObjID obj,Model &model) {
   model.materialProperties.Refresh(RenderMaterialPropertyRevision(),[&](float &alpha,float &illum,bool &shadow) {
      if(gAlphaRenderProp)gAlphaRenderProp->Get(obj,&alpha);
      model.materialProperties.selfIllumPresent=g_pSelfIlluminationProp && g_pSelfIlluminationProp->Get(obj,&illum);
      BOOL nativeShadow=TRUE;if(g_pObjShadowProp)g_pObjShadowProp->Get(obj,&nativeShadow);
      shadow=nativeShadow!=FALSE;
      if(alpha!=model.materialProperties.alpha || illum!=model.materialProperties.selfIllum || shadow!=model.materialProperties.castsShadow)
         RenderBackendTrace("world-material-properties obj=%d alpha=%.6f self_illum=%.6f casts_shadow=%u",int(obj),double(alpha),double(illum),unsigned(shadow));
   });
}
float linear(float value){return value<=0.04045f?value/12.92f:std::pow((value+0.055f)/1.055f,2.4f);}
void color(RenderSceneMaterial &m,unsigned packed) {
   for(unsigned i=0;i<3;++i)m.baseColor[i]=linear(float((packed>>(8*i))&255)/255);
}
template<class T> const T *list(const mds_model *m,uint32_t offset,size_t count) {
   if(offset>m->mod_size || count>(m->mod_size-offset)/sizeof(T))return nullptr;
   return reinterpret_cast<const T *>(reinterpret_cast<const unsigned char *>(m)+offset);
}
// No r3 global state is changed: compose the same subobject transform chain
// as md_start_subobj, including the fixed pivot followed by local X motion.
bool transforms(const mds_model *m,const std::array<float,MAX_REND_JOINTS> &joints,const mxs_vector &scale,
                std::vector<mxs_trans> &points,std::vector<mxs_trans> &lights) {
   auto *subs=list<mds_subobj>(m,m->subobj_off,m->subobjs);if(!subs)return false;
   mxs_trans identity;mx_identity_trans(&identity);
   points.assign(m->verts,identity);
   if(m->norm_off<m->light_off)return false;
   lights.assign((m->norm_off-m->light_off)/sizeof(mds_light),identity);
   std::vector<bool> visited(m->subobjs);
   auto visit=[&](auto &&self,int index,const mxs_trans &parent)->bool {
      if(index==-1)return true;
      if(index<0 || index>=m->subobjs || visited[index])return false;
      visited[index]=true;const auto &s=subs[index];mxs_trans transform=parent;
      if(s.type!=MD_SUB_NONE) {
         if(s.parm<0 || s.parm>=MAX_REND_JOINTS || s.type>MD_SUB_SLIDE)return false;
         mxs_trans motion;mx_identity_trans(&motion);
         if(s.type==MD_SUB_ROT) {
            mxs_angvec angle{};
            angle.tx=mxs_ang(int32_t(std::remainder(joints[s.parm],360.0f)*(65536.0f/360.0f)));
            mx_ang2mat(&motion.mat,&angle);
         }else motion.vec.x=joints[s.parm];
         mxs_trans local=s.trans;
         for(unsigned axis=0;axis<3;++axis)local.vec.el[axis]*=scale.el[axis];
         mxs_trans pivot;mx_mul_trans(&pivot,&local,&motion);mx_mul_trans(&transform,&parent,&pivot);
      }
      if(size_t(s.point_start)+s.point_num>points.size() || size_t(s.light_start)+s.light_num>lights.size())return false;
      std::fill(points.begin()+s.point_start,points.begin()+s.point_start+s.point_num,transform);
      std::fill(lights.begin()+s.light_start,lights.begin()+s.light_start+s.light_num,transform);
      return self(self,s.child,transform) && self(self,s.next,parent);
   };
   return !m->subobjs || visit(visit,0,identity);
}
bool geometry(Model &model,const mds_model *m) {
   if(m->ver<MD_COMPATIBLE_VER || m->ver>MD_CUR_VER || m->pgon_off>m->node_off || m->node_off>m->mod_size ||
      m->uv_off>m->vhot_off || m->light_off>m->norm_off)return false;
   std::vector<mxs_trans> pointTransforms,lightTransforms;
   if(!transforms(m,model.joints,model.scale,pointTransforms,lightTransforms))return false;
   auto *points=list<mxs_vector>(m,m->point_off,m->verts);
   const size_t lightCount=(m->norm_off-m->light_off)/sizeof(mds_light);
   const size_t uvCount=(m->vhot_off-m->uv_off)/sizeof(mds_uv);
   auto *lights=list<mds_light>(m,m->light_off,lightCount);auto *uvs=list<mds_uv>(m,m->uv_off,uvCount);
   if(!points || !lights || !uvs)return false;
   for(auto &entry:model.parts){entry.second.vertices.clear();entry.second.indices.clear();}
   size_t offset=m->pgon_off;
   for(unsigned polygon=0;polygon<m->pgons;++polygon) {
      if(offset>m->node_off || sizeof(mds_pgon)>m->node_off-offset)return false;
      auto *p=list<mds_pgon>(m,uint32_t(offset),1);if(!p)return false;
      const unsigned primitive=p->type&MD_PGON_PRIM_MASK;
      const unsigned arrays=primitive==MD_PGON_PRIM_TMAP?3:2;
      const size_t bytes=sizeof(*p)+p->num*arrays*sizeof(ushort)+(m->ver>3?sizeof(mds_pgon_aux):0);
      if(p->num>64 || primitive>MD_PGON_PRIM_TMAP || bytes>m->node_off-offset)return false;
      offset+=bytes;
      if(p->num<3 || (primitive!=MD_PGON_PRIM_TMAP && primitive!=MD_PGON_PRIM_SOLID))continue;
      unsigned material=0;
      if(m->ver>3)material=reinterpret_cast<const mds_pgon_aux *>(p->verts+p->num*arrays)->mat;
      else {
         auto *mats=list<mds_mat>(m,m->mat_off,m->mats);if(!mats)return false;
         for(unsigned i=0;i<m->mats;++i)if(mats[i].num==p->data){material=i;break;}
      }
      if(material>=m->mats)continue;
      auto &part=model.parts[material];const uint32_t base=uint32_t(part.vertices.size());
      for(unsigned corner=0;corner<p->num;++corner) {
         const unsigned vi=p->verts[corner],li=p->verts[p->num+corner];
         if(vi>=m->verts || li>=lightCount)return false;
         const auto packed=lights[li].norm;
         mxs_vector normal{X_NORM(packed),Y_NORM(packed),Z_NORM(packed)},position,rotated;
         mxs_vector scaled=points[vi];
         for(unsigned axis=0;axis<3;++axis) {
            scaled.el[axis]*=model.scale.el[axis];
            normal.el[axis]*=model.scale.el[(axis+1)%3]*model.scale.el[(axis+2)%3];
         }
         mx_trans_mul_vec(&position,&pointTransforms[vi],&scaled);
         mx_mat_mul_vec(&rotated,&lightTransforms[li].mat,&normal);
         const float length=std::sqrt(rotated.x*rotated.x+rotated.y*rotated.y+rotated.z*rotated.z);
         if(length>0)for(float &component:rotated.el)component/=length;
         RenderSceneVertex vertex{{position.x,position.y,position.z},{rotated.x,rotated.y,rotated.z},{0,0}};
         if(primitive==MD_PGON_PRIM_TMAP) {
            const unsigned ui=p->verts[2*p->num+corner];if(ui>=uvCount)return false;
            vertex.uv[0]=uvs[ui].u;vertex.uv[1]=uvs[ui].v;
         }
         part.vertices.push_back(vertex);
      }
      for(unsigned corner=2;corner<p->num;++corner){part.indices.push_back(base);part.indices.push_back(base+corner-1);part.indices.push_back(base+corner);}
   }
   return true;
}
bool mdMaterialLighting(const mds_model *m,unsigned index,float &trans,float &illum) {
   // Older version-4 assets store only this eight-byte prefix; MaxTU/MaxTV
   // are optional trailing fields. Read the unaligned serialized floats safely.
   if(m->ver<=3 || index>=m->mats || m->amat_size<2*sizeof(float))return false;
   const uint64_t offset=uint64_t(m->amat_off)+uint64_t(index)*m->amat_size;
   if(offset>UINT32_MAX)return false;
   const auto *bytes=list<unsigned char>(m,uint32_t(offset),2*sizeof(float));if(!bytes)return false;
   std::memcpy(&trans,bytes,sizeof(float));std::memcpy(&illum,bytes+sizeof(float),sizeof(float));return true;
}
void materials(ObjID obj,int index,mds_model *m,Model &model,bool unlit) {
   // Setup supports animated frames and per-object replacement properties.
   // Restore the native table so scene extraction has no renderer side effects.
   std::array<r3s_texture,MD_TAB_SIZE> saved;
   std::copy(mdd_vtext_tab,mdd_vtext_tab+MD_TAB_SIZE,saved.begin());
   struct Restore {const std::array<r3s_texture,MD_TAB_SIZE> &saved;~Restore(){std::copy(saved.begin(),saved.end(),mdd_vtext_tab);}} restore{saved};
   objmodelSetupMdTextures(obj,m,index);
   auto *mats=list<mds_mat>(m,m->mat_off,m->mats);if(!mats)return;
   refreshProperties(obj,model);
   const float objectAlpha=model.materialProperties.alpha;
   for(auto &entry:model.parts) {
      auto &p=entry.second;const unsigned i=entry.first;const auto &source=mats[i];
      RenderSceneMaterial desc{};desc.doubleSided=1;desc.baseColor[0]=desc.baseColor[1]=desc.baseColor[2]=1;
      desc.baseColor[3]=std::clamp(objectAlpha,0.0f,1.0f);desc.roughness=.8f;desc.alphaCutoff=.5f;
      if(source.type==MD_MAT_TMAP && source.num<MD_TAB_SIZE) {
         auto *bitmap=mdd_vtext_tab[source.num];desc.baseColorTexture=texture(bitmap);
         if(bitmap && (bitmap->flags&BMF_TRANS))desc.opacity=RenderSceneMasked;
      }else color(desc,source.handle);
      float illum=0;
      float trans=0,authoredIllum=0;
      if(mdMaterialLighting(m,i,trans,authoredIllum)) {
         if((m->mat_flags&MD_MAT_TRANS) && trans>0)desc.baseColor[3]*=std::clamp(trans,0.0f,1.0f);
         if(m->mat_flags&MD_MAT_ILLUM){illum=model.materialProperties.MdIllumination(authoredIllum);desc.emitterSurface=authoredIllum>0;}
      }
      if(desc.baseColor[3]<1)desc.opacity=RenderSceneBlended;
      if(unlit)illum=1;
      desc.emissionUsesBaseTexture=1;
      for(unsigned axis=0;axis<3;++axis)desc.emission[axis]=desc.baseColor[axis]*std::max(0.0f,illum);
      if(!p.material)p.material=RenderSceneCreateMaterial(&desc);
      else if(std::memcmp(&desc,&p.description,sizeof(desc)))RenderSceneUpdateMaterial(p.material,&desc);
      p.description=desc;
   }
}
template<class T> const T *meshList(const mms_model *m,size_t size,size_t offset,size_t count) {
   if(offset>size || count>(size-offset)/sizeof(T))return nullptr;
   return reinterpret_cast<const T *>(reinterpret_cast<const unsigned char *>(m)+offset);
}
bool meshPose(ObjID obj,const mms_model *m,size_t size,const ObjPos &position,std::vector<SegmentPose> &poses) {
   if(size<sizeof(*m) || (m->version!=1 && m->version!=2))return false;
   auto *segments=meshList<mms_segment>(m,size,m->seg_off,m->segs);if(!segments)return false;
   poses.resize(m->segs);
   long handle=0;const bool creature=CreatureGetRenderHandle(obj,&handle)!=FALSE;
   const int joints=creature?CreatureGetRenderJointCount(obj):0;
   // Callbacks only read app_data from the header; never mutate the shared asset.
   mms_model header=*m;header.app_data=creature?ulong(handle):ulong(obj);
   mxs_matrix orientation;mx_ang2mat(&orientation,&position.fac);
   for(unsigned i=0;i<m->segs;++i) {
      auto &pose=poses[i];mx_identity_trans(&pose.transform);quat_identity(&pose.rotation);
      if(!creature)continue; // Non-creature MM joints use the object transform.
      if(segments[i].joint_id>=joints)return false;
      mxs_trans world;
      if(segments[i].flags&MMSEG_FLAG_STRETCHY)
         CreatureMeshStretchyJointCallback(&header,segments[i].joint_id,&world,&pose.rotation);
      else CreatureMeshJointPosCallback(&header,segments[i].joint_id,&world);
      // Keep vertices local to the object, so instance motion remains separate
      // from deformation and shadow/raster geometry use the identical pose.
      mxs_vector delta;mx_sub_vec(&delta,&world.vec,&position.loc.vec);
      mx_mat_tmul_vec(&pose.transform.vec,&orientation,&delta);
      mx_tmul_mat(&pose.transform.mat,&orientation,&world.mat);
      for(float value:pose.transform.mat.el)if(!std::isfinite(value))return false;
      for(float value:pose.transform.vec.el)if(!std::isfinite(value))return false;
      for(float value:pose.rotation.el)if(!std::isfinite(value))return false;
   }
   return true;
}
using AttachmentGraph=std::map<ObjID,std::vector<sMeshAttachInstance>>;
std::vector<ObjID> attachmentOrder(const AttachmentGraph &graph) {
   std::map<ObjID,unsigned> incoming;
   for(const auto &entry:graph)incoming.emplace(entry.first,0);
   for(const auto &entry:graph)for(const auto &link:entry.second)
      if(link.m_Object!=entry.first && incoming.count(link.m_Object))++incoming[link.m_Object];
   std::vector<ObjID> order;
   for(const auto &entry:incoming)if(!entry.second)order.push_back(entry.first);
   for(size_t i=0;i<order.size();++i)for(const auto &link:graph.at(order[i])) {
      auto child=incoming.find(link.m_Object);
      if(child!=incoming.end() && child->second && --child->second==0)order.push_back(child->first);
   }
   // Malformed cyclic components still terminate in deterministic ID order.
   for(const auto &entry:incoming)if(entry.second)order.push_back(entry.first);
   return order;
}

bool attachmentTransform(const mms_model *mesh,size_t size,const std::vector<SegmentPose> &poses,
                         const mxs_trans &root,const sMeshAttachInstance &attached,mxs_trans &world) {
   if(size<sizeof(*mesh))return false;
   const auto *segments=meshList<mms_segment>(mesh,size,mesh->seg_off,mesh->segs);
   if(!segments)return false;
   for(unsigned i=0;i<mesh->segs;++i) {
      if(segments[i].joint_id!=attached.m_iJoint || (segments[i].flags&MMSEG_FLAG_STRETCHY))continue;
      if(i>=poses.size())return false;
      mxs_trans relative,local;mx_ang2mat(&relative.mat,&attached.m_Angles);relative.vec=attached.m_Offset;
      mx_mul_trans(&local,&poses[i].transform,&relative);mx_mul_trans(&world,&root,&local);
      for(float value:world.mat.el)if(!std::isfinite(value))return false;
      for(float value:world.vec.el)if(!std::isfinite(value))return false;
      return true;
   }
   return false;
}

bool meshGeometry(Model &model,const mms_model *m,size_t size) {
   if(size<sizeof(*m) || (m->version!=1 && m->version!=2) || model.segments.size()!=m->segs)return false;
   const auto *segments=meshList<mms_segment>(m,size,m->seg_off,m->segs);
   const auto *chunks=meshList<mms_smatseg>(m,size,m->smatseg_off,m->smatsegs);
   const auto *points=meshList<mxs_vector>(m,size,m->vert_vec_off,m->verts);
   const auto *uvns=meshList<mms_uvn>(m,size,m->vert_uvn_off,m->verts);
   const auto *weights=meshList<float>(m,size,m->weight_off,m->weights);
   const auto *polygons=meshList<mms_pgon>(m,size,m->pgon_off,m->pgons);
   if(!segments || !chunks || !points || !uvns || !weights || !polygons)return false;
   if(!meshList<unsigned char>(m,size,m->smatr_off,m->smatrs*(m->version==1?sizeof(mms_ssmatr):sizeof(mms_smatr))))return false;
   std::vector<RenderSceneVertex> vertices(m->verts);
   std::vector<bool> assigned(m->verts);
   for(unsigned segment=0;segment<m->segs;++segment) {
      const auto &s=segments[segment];const auto &pose=model.segments[segment];
      const auto *mapping=meshList<mms_mapping>(m,size,size_t(m->map_off)+s.map_start,s.smatsegs);
      if(!mapping)return false;
      for(unsigned chunk=0;chunk<s.smatsegs;++chunk) {
         if(mapping[chunk]>=m->smatsegs)return false;
         const auto &data=chunks[mapping[chunk]].data;
         if(size_t(data.vert_start)+data.verts>m->verts)return false;
         if((s.flags&MMSEG_FLAG_STRETCHY) && size_t(data.weight_start)+data.verts>m->weights)return false;
         for(unsigned v=0;v<data.verts;++v) {
            const unsigned vi=data.vert_start+v;const auto packed=uvns[vi].norm;
            mxs_vector point=points[vi],normal{X_NORM(packed),Y_NORM(packed),Z_NORM(packed)};
            if(s.flags&MMSEG_FLAG_STRETCHY) {
               const float weight=weights[data.weight_start+v];if(!std::isfinite(weight) || weight<0 || weight>1)return false;
               quat identity,rotation,joint=pose.rotation;quat_identity(&identity);
               quat_slerp(&rotation,&identity,&joint,weight);
               mxs_matrix delta;quat_to_matrix(&delta,&rotation);
               mxs_vector rotated;mx_mat_mul_vec(&rotated,&delta,&point);point=rotated;
               mx_mat_mul_vec(&rotated,&delta,&normal);normal=rotated;
            }
            mxs_vector transformed,rotated;
            mx_trans_mul_vec(&transformed,&pose.transform,&point);
            mx_mat_mul_vec(&rotated,&pose.transform.mat,&normal);
            const float length=std::sqrt(rotated.x*rotated.x+rotated.y*rotated.y+rotated.z*rotated.z);
            if(length>0)for(float &component:rotated.el)component/=length;
            vertices[vi]={{transformed.x,transformed.y,transformed.z},{rotated.x,rotated.y,rotated.z},{uvns[vi].u,uvns[vi].v}};
            assigned[vi]=true;
         }
      }
   }
   for(auto &entry:model.parts){entry.second.vertices.clear();entry.second.indices.clear();}
   for(unsigned polygon=0;polygon<m->pgons;++polygon) {
      const auto &p=polygons[polygon];if(p.smatr_id>=m->smatrs)return false;
      auto &part=model.parts[p.smatr_id];
      for(unsigned i=0;i<3;++i) {
         if(p.v[i]>=m->verts || !assigned[p.v[i]])return false;
         part.indices.push_back(uint32_t(part.vertices.size()));part.vertices.push_back(vertices[p.v[i]]);
      }
   }
   return true;
}
void meshMaterials(ObjID obj,int index,mms_model *m,size_t size,Model &model,bool unlit) {
   const size_t stride=m->version==1?sizeof(mms_ssmatr):sizeof(mms_smatr);
   if(!meshList<unsigned char>(m,size,m->smatr_off,m->smatrs*stride))return;
   auto *begin=reinterpret_cast<unsigned char *>(m)+m->smatr_off;
   std::vector<unsigned char> saved(begin,begin+m->smatrs*stride);
   struct Restore {
      unsigned char *begin;const std::vector<unsigned char> &saved;bool overrides=false;
      ~Restore(){if(overrides)MeshTexPostrender();std::copy(saved.begin(),saved.end(),begin);}
   } restore{begin,saved};
   objmodelSetupMeshTextures(obj,m,index);
   // Custom skins/faces use the v2 material layout in the native engine.
   if(m->version==2){MeshTexPrerender(obj,m);restore.overrides=true;}
   refreshProperties(obj,model);
   const float objectAlpha=model.materialProperties.alpha,selfIllum=model.materialProperties.selfIllum;
   for(auto &entry:model.parts) {
      auto &p=entry.second;const unsigned i=entry.first;
      RenderSceneMaterial desc{};desc.doubleSided=1;desc.baseColor[0]=desc.baseColor[1]=desc.baseColor[2]=1;
      desc.baseColor[3]=std::clamp(objectAlpha,0.0f,1.0f);desc.roughness=.8f;desc.alphaCutoff=.5f;
      unsigned type=0,handle=0;float illum=selfIllum;
      if(m->version==1) {
         const auto &source=*reinterpret_cast<const mms_ssmatr *>(begin+i*stride);type=source.type;handle=source.handle;
      }else {
         const auto &source=*reinterpret_cast<const mms_smatr *>(begin+i*stride);type=source.type;handle=source.handle;
         if(source.dwCaps&kUseAlpha)desc.baseColor[3]*=std::clamp(source.fAlpha,0.0f,1.0f);
         if(source.dwCaps&kUseSelfIllumination){illum=std::max(illum,source.fSelfIllumination);desc.emitterSurface=source.fSelfIllumination>0;}
      }
      if(type==MM_MAT_TMAP) {
         auto *bitmap=mm_resolve_texture(handle);desc.baseColorTexture=texture(bitmap);
         if(bitmap && (bitmap->flags&BMF_TRANS))desc.opacity=RenderSceneMasked;
      }else color(desc,handle);
      if(desc.baseColor[3]<1)desc.opacity=RenderSceneBlended;
      if(unlit)illum=1;
      desc.emissionUsesBaseTexture=1;
      for(unsigned axis=0;axis<3;++axis)desc.emission[axis]=desc.baseColor[axis]*std::max(0.0f,illum);
      if(!p.material)p.material=RenderSceneCreateMaterial(&desc);
      else if(std::memcmp(&desc,&p.description,sizeof(desc)))RenderSceneUpdateMaterial(p.material,&desc);
      p.description=desc;
   }
}

}
extern "C" void WorldSceneBeginObjectCapture(int enabled) {cache().objectFog.clear();cache().captureFog=enabled!=0;}
extern "C" void WorldSceneCaptureObjectFog(int object,int enabled) {
   if(cache().captureFog)cache().objectFog[object]|=enabled?2:1;
}
extern "C" int WorldSceneHasModel(int object) {
   const auto found=cache().objects.find(object);
   if(found==cache().objects.end() || !found->second.seen)return 0;
   bool any=false;
   for(const auto &entry:found->second.parts) {
      const auto &part=entry.second;
      if(part.vertices.empty())continue;
      if(!part.instance)return 0;
      any=true;
   }
   return any;
}
extern "C" void WorldSceneModelsReset() {cache().objects.clear();cache().textures.clear();cache().reported=false;WorldSceneBeginObjectCapture(0);}
extern "C" void WorldSceneCaptureModels() {
   ZoneScopedN("Capture world-space object models");
   auto &objects=cache().objects;for(auto &entry:objects)entry.second.seen=false;
   AutoAppIPtr(ObjectSystem);
   IObjectQuery *query=pObjectSystem->Iter(kObjectConcrete);
   struct Release {IObjectQuery *query;~Release(){query->Release();}} release{query};
   struct AssetInfo {int type;void *source;size_t size;unsigned long long revision;};
   std::unordered_map<int,AssetInfo> assets;
   auto capture=[&](ObjID obj,const mxs_trans *attachment,bool ownerFog=false) {
      int index=-1;
      const auto *camera=PlayerCamera();
      // Match Dark's first-person callback: the player world model is a
      // placeholder, while arms and the nocked arrow are rendered as overlays.
      if((camera && camera->mode==FIRST_PERSON && IsPlayerObj(obj)) ||
         (!attachment && !ObjHasRefs(obj)) || obj==PlayerArm() || obj==PlayerArrow() ||
         !ObjGetModelNumber(obj,&index) || index<0)return;
      auto asset=assets.find(index);
      if(asset==assets.end())asset=assets.emplace(index,AssetInfo{objmodelGetModelType(index),objmodelGetModel(index),objmodelGetModelDataSize(index),objmodelGetModelRevision(index)}).first;
      const auto &info=asset->second;const int type=info.type;if(type!=OM_TYPE_MD && type!=OM_TYPE_MESH)return;
      const auto renderType=ObjRenderType(obj);
      if(renderType==kRenderNotAtAll || renderType==kRenderEditorOnly)return;
      auto *position=ObjPosGet(obj);auto *source=info.source;
      ObjPos attachedPosition{};
      if(attachment) {
         attachedPosition.loc.vec=attachment->vec;mx_mat2ang(&attachedPosition.fac,&attachment->mat);
         position=&attachedPosition;
      }
      const size_t size=info.size;
      if(!position || !source || size<(type==OM_TYPE_MD?sizeof(mds_model):sizeof(mms_model)))return;
      const bool cellFog=position->loc.cell>=0 && position->loc.cell<wr_num_cells &&
         WR_CELL(position->loc.cell) && (WR_CELL(position->loc.cell)->flags&CELL_FOG)!=0;
      const bool receivesFog=objectReceivesFog(obj,attachment?ownerFog:cellFog);
      auto &model=objects[obj];
      const bool rebuild=model.source!=source || model.sourceSize!=size ||
         model.sourceRevision!=info.revision || model.index!=index || model.type!=type;
      if(rebuild && model.source)RenderBackendTrace("world-model-reload obj=%d index=%d revision=%llu->%llu bytes=%zu->%zu",
         obj,index,model.sourceRevision,info.revision,model.sourceSize,size);
      if(rebuild)destroy(model);
      bool pose=rebuild;
      if(type==OM_TYPE_MD) {
         std::array<float,MAX_REND_JOINTS> joints{};
         if(auto *values=ObjJointPos(obj))std::copy(values,values+MAX_REND_JOINTS,joints.begin());
         if(!std::all_of(joints.begin(),joints.end(),[](float value){return std::isfinite(value);}))return;
         mxs_vector scale{1,1,1};ObjGetScale(obj,&scale);
         if(!std::all_of(scale.el,scale.el+3,[](float value){return std::isfinite(value);}))return;
         auto *md=static_cast<mds_model *>(source);if(md->mod_size>size)return;
         pose=pose || model.joints!=joints || std::memcmp(&model.scale,&scale,sizeof(scale));
         model.joints=joints;model.scale=scale;
         if(pose && !geometry(model,md)){destroy(model);return;}
         materials(obj,index,md,model,renderType==kRenderUnlit);
      }else {
         auto *mesh=static_cast<mms_model *>(source);std::vector<SegmentPose> segments;
         if(!meshPose(obj,mesh,size,*position,segments))return;
         pose=pose || segments.size()!=model.segments.size() ||
            (!segments.empty() && std::memcmp(segments.data(),model.segments.data(),segments.size()*sizeof(SegmentPose)));
         model.segments=std::move(segments);
         if(pose && !meshGeometry(model,mesh,size)){destroy(model);return;}
         meshMaterials(obj,index,mesh,size,model,renderType==kRenderUnlit);
      }
      // Offscreen native objects can retain CELL_INVALID even while stationary.
      // Cache our read-only lookup instead of repeating their BSP traversal.
      const bool moved=!model.lightCellKnown || std::memcmp(&model.lightCellPosition,&position->loc.vec,sizeof(mxs_vector));
      if(!attachment && position->loc.cell>=0)model.lightCell=position->loc.cell;
      else if(moved) {
         Location lightLocation=position->loc;
         if(attachment)MakeLocationFromVector(&lightLocation,&position->loc.vec);
         model.lightCell=CellFromLoc(&lightLocation);
      }
      model.lightCellKnown=true;model.lightCellPosition=position->loc.vec;
      const int lightCell=model.lightCell;
      model.receivesFog=receivesFog;model.seen=true;model.source=source;model.sourceSize=size;model.sourceRevision=info.revision;model.index=index;model.type=type;
      mxs_matrix rotation;
      if(attachment)rotation=attachment->mat;else mx_ang2mat(&rotation,&position->fac);
      const bool rasterVisible=attachment || WorldSceneObjectVisible(obj)!=0;
      for(auto &entry:model.parts) {
         auto &p=entry.second;if(p.vertices.empty() || !p.material)continue;
         if(!p.mesh) {
            RenderSceneMesh mesh{p.vertices.data(),uint32_t(p.vertices.size()),p.indices.data(),uint32_t(p.indices.size()),p.material,1};
            p.mesh=RenderSceneCreateMesh(&mesh);
         }else if(pose)RenderSceneUpdateVertices(p.mesh,p.vertices.data(),uint32_t(p.vertices.size()));
         if(!p.mesh)continue;
         RenderSceneInstance instance{};instance.mesh=p.mesh;instance.objectId=obj;
         instance.visible=rasterVisible;instance.castsShadow=model.materialProperties.castsShadow;instance.receivesFog=receivesFog;
         for(unsigned col=0;col<3;++col)for(unsigned row=0;row<3;++row)instance.transform[col*4+row]=rotation.el[col*3+row];
         for(unsigned row=0;row<3;++row)instance.transform[12+row]=position->loc.vec.el[row];
         instance.transform[15]=1;
         std::copy(p.instance?p.placement.transform:instance.transform,(p.instance?p.placement.transform:instance.transform)+16,instance.previousTransform);
         if(!p.instance)p.instance=RenderSceneCreateInstance(&instance);
         else if(std::memcmp(&instance,&p.placement,sizeof(instance)))RenderSceneUpdateInstance(p.instance,&instance);
         p.placement=instance;
         WorldSceneLinkInstance(p.instance,lightCell,&p.lightRevision);
      }
      bool emitter=false;for(const auto &entry:model.parts)emitter=emitter || entry.second.description.emitterSurface!=0;
      auto *emitterPosition=emitter?ObjPosGet(obj):nullptr;
      if(emitterPosition) {
         sLightProp *ordinary=nullptr;sAnimLightProp *animated=nullptr;
         const bool hasOrdinary=ObjLightGet(obj,&ordinary)!=FALSE;
         const bool hasAnimated=ObjAnimLightGet(obj,&animated)!=FALSE;
         mxs_vector point{};
         const bool hasVhot=FindVhotLocation(&point,obj,kVHotLightSource)!=FALSE;
         if(hasVhot || hasOrdinary || hasAnimated) {
            if(!hasVhot)point=position->loc.vec;
            const sLightPropCore *core=hasOrdinary?static_cast<sLightPropCore*>(ordinary):
               (hasAnimated?static_cast<sLightPropCore*>(animated):nullptr);
            if(core) {
               mxs_matrix orientation;mx_ang2mat(&orientation,&emitterPosition->fac);
               mxs_vector offset;mx_mat_mul_vec(&offset,&orientation,&core->offset);mx_addeq_vec(&point,&offset);
            }
            WorldSceneRegisterEmitter(obj,hasAnimated?animated->animation.light_data_index:-1,point.el);
         }
      }
   };
   for(;!query->Done();query->Next())capture(query->Object(),nullptr);
   AutoAppIPtr(ContainSys);
   auto readAttachments=[&](ObjID owner) {
      std::vector<sMeshAttachInstance> nativeAttachments;
      sMeshAttach *property=nullptr;
      if(ObjMeshAttachGet(owner,&property) && property)
         nativeAttachments.assign(property->m_Attachment,property->m_Attachment+kMeshAttachMax);
      // Dark creates these containment attachments only during legacy drawing.
      // Read the same links without modifying native properties or positions.
      for(const auto type:{kContainTypeBelt,kContainTypeAlt}) {
         auto *iter=pContainSys->IterStartType(owner,type,type);ObjID item=OBJ_NULL;
         for(;!iter->finished;pContainSys->IterNext(iter))item=iter->containee;
         pContainSys->IterEnd(iter);
         if(item) {
            auto attached=type==kContainTypeBelt?GetBeltLinkAttachment(owner):GetAltLinkAttachment(owner);
            attached.m_Object=item;nativeAttachments.push_back(attached);
         }
      }
      return nativeAttachments;
   };
   AttachmentGraph graph;std::vector<ObjID> pending;
   for(const auto &entry:objects)if(entry.second.seen && entry.second.type==OM_TYPE_MESH)pending.push_back(entry.first);
   for(size_t i=0;i<pending.size();++i) {
      const ObjID owner=pending[i];if(graph.count(owner))continue;
      auto links=readAttachments(owner);
      for(const auto &link:links)if(link.m_Object && !graph.count(link.m_Object))pending.push_back(link.m_Object);
      graph.emplace(owner,std::move(links));
   }
   const auto owners=attachmentOrder(graph);std::unordered_set<ObjID> visited;
   for(const auto owner:owners) {
      if(!visited.insert(owner).second)continue;
      const auto found=objects.find(owner);if(found==objects.end())continue;
      const auto &model=found->second;if(!model.seen || model.type!=OM_TYPE_MESH)continue;
      const auto &nativeAttachments=graph.at(owner);
      const auto *mesh=static_cast<const mms_model *>(model.source);
      const auto *segments=meshList<mms_segment>(mesh,model.sourceSize,mesh->seg_off,mesh->segs);
      const auto *position=ObjPosGet(owner);if(!segments || !position)continue;
      mxs_trans root;mx_ang2mat(&root.mat,&position->fac);root.vec=position->loc.vec;
      // Nested owners use their captured attachment placement rather than a
      // native position left over from the last legacy render.
      if(!model.parts.empty() && model.parts.begin()->second.instance) {
         const auto &placement=model.parts.begin()->second.placement;
         for(unsigned col=0;col<3;++col)for(unsigned row=0;row<3;++row)root.mat.el[col*3+row]=placement.transform[col*4+row];
         for(unsigned row=0;row<3;++row)root.vec.el[row]=placement.transform[12+row];
      }
      for(const auto &attached:nativeAttachments) {
         if(!attached.m_Object || attached.m_Object==owner || visited.count(attached.m_Object))continue;
         mxs_trans world;
         if(!attachmentTransform(mesh,model.sourceSize,model.segments,root,attached,world))continue;
         capture(attached.m_Object,&world,model.receivesFog);

      }
   }
   for(auto it=objects.begin();it!=objects.end();) {
      if(!it->second.seen){destroy(it->second);it=objects.erase(it);}else ++it;
   }
   if(!cache().reported) {
      size_t parts=0,characters=0;for(const auto &entry:objects) {
         parts+=entry.second.parts.size();if(entry.second.type==OM_TYPE_MESH)++characters;
      }
      std::fprintf(stderr,"World-space models: %zu objects (%zu MM meshes), %zu material parts\n",objects.size(),characters,parts);
      size_t fogged=0,mixed=0;
      for(const auto &entry:objects)if(entry.second.receivesFog)++fogged;
      for(const auto &entry:cache().objectFog)if(entry.second==3)++mixed;
      std::fprintf(stderr,"World-space object fog: %zu receiving, %zu queued states, %zu mixed-fragment objects\n",
         fogged,cache().objectFog.size(),mixed);
      cache().reported=true;
   }
}
