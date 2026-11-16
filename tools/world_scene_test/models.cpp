// Internal decoder regression. Link with section garbage collection so engine
// integration entry points are discarded; the exercised decoder is production code.
#include "../../src/render/worldscene_models.cpp"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <limits>
#include "../../src/render/worldscene_lights.hpp"
// Engine forced headers define NDEBUG even when the compiler flag is removed.
#define CHECK(expression) do { if(!(expression)) { std::cerr << "CHECK failed: " #expression << " at " << __LINE__ << "\n"; std::abort(); } } while(false)
namespace {
struct Asset {
   std::vector<unsigned char> bytes;
   template<class T> unsigned append(unsigned count=1) {
      auto offset=unsigned(bytes.size());bytes.resize(offset+sizeof(T)*count);return offset;
   }
   template<class T> T &at(unsigned offset){return *reinterpret_cast<T *>(bytes.data()+offset);}
   Asset() {
      mds_model header{};header.ver=4;header.verts=3;header.pgons=1;header.mats=1;header.subobjs=2;
      append<mds_model>();
      header.subobj_off=append<mds_subobj>(2);header.mat_off=append<mds_mat>();
      header.uv_off=append<mds_uv>(3);header.vhot_off=unsigned(bytes.size());
      header.point_off=append<mxs_vector>(3);header.light_off=append<mds_light>(3);
      header.norm_off=append<mxs_vector>();header.pgon_off=append<mds_pgon>();
      append<ushort>(9);append<mds_pgon_aux>();header.node_off=header.mod_size=unsigned(bytes.size());
      at<mds_model>(0)=header;
      auto &root=at<mds_subobj>(header.subobj_off);root.type=MD_SUB_NONE;root.child=1;root.next=-1;
      auto &child=at<mds_subobj>(header.subobj_off+sizeof(mds_subobj));
      child.type=MD_SUB_ROT;child.parm=0;child.child=child.next=-1;child.point_num=child.light_num=3;
      mx_identity_trans(&child.trans);child.trans.vec.x=2;
      at<mxs_vector>(header.point_off)={1,0,0};at<mxs_vector>(header.point_off+sizeof(mxs_vector))={0,1,0};
      at<mxs_vector>(header.point_off+2*sizeof(mxs_vector))={0,0,1};
      for(unsigned i=0;i<3;++i) {
         auto &light=at<mds_light>(header.light_off+i*sizeof(mds_light));light.point=i;light.norm=0x400;
         auto &uv=at<mds_uv>(header.uv_off+i*sizeof(mds_uv));uv={float(i),.5f};
      }
      auto &polygon=at<mds_pgon>(header.pgon_off);polygon.type=MD_PGON_PRIM_TMAP;polygon.num=3;
      for(unsigned i=0;i<9;++i)polygon.verts[i]=i%3;
   }
   mds_model *model(){return reinterpret_cast<mds_model *>(bytes.data());}
};
void near(float a,float b){if(std::abs(a-b)>=.001f)std::cerr<<"actual="<<a<<" expected="<<b<<"\n";CHECK(std::abs(a-b)<.001f);}
struct MeshAsset : Asset {
   MeshAsset() {
      bytes.clear();mms_model header{};header.version=2;header.verts=6;header.pgons=2;
      header.smatrs=2;header.segs=2;header.smatsegs=2;header.weights=3;
      append<mms_model>();header.map_off=append<mms_mapping>(2);header.seg_off=append<mms_segment>(2);
      header.smatr_off=append<mms_smatr>(2);header.smatseg_off=append<mms_smatseg>(2);
      header.pgon_off=append<mms_pgon>(2);header.norm_off=append<mxs_vector>(2);
      header.vert_vec_off=append<mxs_vector>(6);header.vert_uvn_off=append<mms_uvn>(6);header.weight_off=append<float>(3);
      at<mms_model>(0)=header;
      at<mms_mapping>(header.map_off)=1;at<mms_mapping>(header.map_off+1)=0;
      for(unsigned i=0;i<2;++i) {
         auto &seg=at<mms_segment>(header.seg_off+i*sizeof(mms_segment));seg.joint_id=i;seg.smatsegs=1;seg.map_start=i;
         if(i==0)seg.flags=MMSEG_FLAG_STRETCHY;
         auto &chunk=at<mms_smatseg>(header.smatseg_off+(1-i)*sizeof(mms_smatseg));
         chunk.seg_id=i;chunk.smatr_id=i;chunk.data.vert_start=i*3;chunk.data.verts=3;
         auto &polygon=at<mms_pgon>(header.pgon_off+i*sizeof(mms_pgon));polygon.smatr_id=i;
         for(unsigned j=0;j<3;++j) {
            polygon.v[j]=i*3+j;
            at<mms_uvn>(header.vert_uvn_off+(i*3+j)*sizeof(mms_uvn))={float(j),.25f,0x00100000};
         }
         at<mxs_vector>(header.vert_vec_off+(i*3)*sizeof(mxs_vector))={0,1,0};
         at<mxs_vector>(header.vert_vec_off+(i*3+1)*sizeof(mxs_vector))={0,0,1};
         at<mxs_vector>(header.vert_vec_off+(i*3+2)*sizeof(mxs_vector))={1,0,0};
      }
      at<float>(header.weight_off)=0;at<float>(header.weight_off+4)=.5f;at<float>(header.weight_off+8)=1;
   }
   mms_model *mesh(){return reinterpret_cast<mms_model *>(bytes.data());}
};
void testAttachments() {
   AttachmentGraph graph;
   sMeshAttachInstance child{},grandchild{};child.m_Object=10;grandchild.m_Object=20;
   graph[30]={child};graph[10]={grandchild};graph[20]={};
   CHECK(attachmentOrder(graph)==std::vector<ObjID>({30,10,20}));
   graph[20]={sMeshAttachInstance{}};graph[20][0].m_Object=30;
   const auto cycle=attachmentOrder(graph);
   CHECK(cycle==std::vector<ObjID>({10,20,30}));
   MeshAsset asset;std::vector<SegmentPose> poses(2);
   for(auto &pose:poses)mx_identity_trans(&pose.transform);
   poses[1].transform.vec={2,3,4};
   mxs_trans root,world;mx_identity_trans(&root);root.vec={10,20,30};
   sMeshAttachInstance attached{};attached.m_iJoint=1;attached.m_Offset={1,0,0};
   CHECK(attachmentTransform(asset.mesh(),asset.bytes.size(),poses,root,attached,world));
   near(world.vec.x,13);near(world.vec.y,23);near(world.vec.z,34);
   mxs_angvec turn{};turn.tz=16384;mx_ang2mat(&root.mat,&turn);
   CHECK(attachmentTransform(asset.mesh(),asset.bytes.size(),poses,root,attached,world));
   near(world.vec.x,7);near(world.vec.y,23);near(world.vec.z,34);
   // Attachment-local rotation composes after the joint and owner rotations.
   attached.m_Angles.tz=16384;
   CHECK(attachmentTransform(asset.mesh(),asset.bytes.size(),poses,root,attached,world));
   mxs_vector x{1,0,0},rotated;mx_mat_mul_vec(&rotated,&world.mat,&x);
   near(rotated.x,-1);near(rotated.y,0);near(rotated.z,0);
   near(world.vec.x,7);near(world.vec.y,23);near(world.vec.z,34);
   attached.m_Angles.tz=0;
   auto &segment=asset.at<mms_segment>(asset.mesh()->seg_off+sizeof(mms_segment));
   segment.flags|=MMSEG_FLAG_STRETCHY;
   CHECK(!attachmentTransform(asset.mesh(),asset.bytes.size(),poses,root,attached,world));
   segment.flags&=~MMSEG_FLAG_STRETCHY;
   attached.m_iJoint=99;
   CHECK(!attachmentTransform(asset.mesh(),asset.bytes.size(),poses,root,attached,world));
   attached.m_iJoint=1;poses.resize(1);
   CHECK(!attachmentTransform(asset.mesh(),asset.bytes.size(),poses,root,attached,world));
   poses.resize(2);mx_identity_trans(&poses[1].transform);
   poses[1].transform.vec.x=std::numeric_limits<float>::quiet_NaN();
   CHECK(!attachmentTransform(asset.mesh(),asset.bytes.size(),poses,root,attached,world));
   asset.mesh()->seg_off=UINT32_MAX;
   CHECK(!attachmentTransform(asset.mesh(),asset.bytes.size(),poses,root,attached,world));
   std::cout<<"PASS attachment joint selection, owner rotation, missing poses and malformed/nonfinite data\n";
}
void testMesh() {
   MeshAsset asset;Model decoded;decoded.segments.resize(2);
   for(auto &segment:decoded.segments){mx_identity_trans(&segment.transform);quat_identity(&segment.rotation);}
   decoded.segments[0].transform.vec={2,3,4};decoded.segments[0].rotation={float(std::sqrt(.5)),float(std::sqrt(.5)),0,0};
   decoded.segments[1].transform.vec={-5,0,2};
   CHECK(meshGeometry(decoded,asset.mesh(),asset.bytes.size()));CHECK(decoded.parts.size()==2);
   auto &a=decoded.parts.at(0),&b=decoded.parts.at(1);
   near(a.vertices[0].position[0],2);near(a.vertices[0].position[1],4);near(a.vertices[0].position[2],4);
   near(a.vertices[1].position[1],3+float(std::sqrt(.5)));near(a.vertices[1].position[2],4+float(std::sqrt(.5)));
   near(a.vertices[2].normal[1],0);near(a.vertices[2].normal[2],-1);
   near(a.vertices[2].uv[0],2);near(a.vertices[2].uv[1],.25f);
   near(b.vertices[0].position[0],-5);near(b.vertices[0].position[1],1);near(b.vertices[0].position[2],2);
   CHECK(a.indices==std::vector<uint32_t>({0,1,2}));
   // Current skeletal poses, including offscreen movement, replace the same topology.
   decoded.segments[0].transform.vec.x=9;
   CHECK(meshGeometry(decoded,asset.mesh(),asset.bytes.size()));near(a.vertices[0].position[0],9);
   asset.at<float>(asset.mesh()->weight_off)=1.1f;CHECK(!meshGeometry(decoded,asset.mesh(),asset.bytes.size()));
   asset.at<float>(asset.mesh()->weight_off)=0;
   asset.at<mms_mapping>(asset.mesh()->map_off)=2;CHECK(!meshGeometry(decoded,asset.mesh(),asset.bytes.size()));
   asset.at<mms_mapping>(asset.mesh()->map_off)=1;
   asset.at<mms_pgon>(asset.mesh()->pgon_off).v[0]=6;CHECK(!meshGeometry(decoded,asset.mesh(),asset.bytes.size()));
   asset.at<mms_pgon>(asset.mesh()->pgon_off).v[0]=0;
   CHECK(!meshGeometry(decoded,asset.mesh(),asset.bytes.size()-1));
   asset.mesh()->map_off=UINT32_MAX;CHECK(!meshGeometry(decoded,asset.mesh(),asset.bytes.size()));
   std::cout<<"PASS MM mapped segments, rigid/weighted poses, normals, UVs, material groups and malformed bounds/weights/indices\n";
}
void writeMeshShadowFixture(const char *path) {
   MeshAsset asset;Model model;model.segments.resize(2);
   for(auto &segment:model.segments){mx_identity_trans(&segment.transform);quat_identity(&segment.rotation);}
   const mxs_vector triangle[3]={{-1,-1,0},{1,-1,0},{0,1,0}};
   for(unsigned i=0;i<3;++i)asset.at<mxs_vector>(asset.mesh()->vert_vec_off+i*sizeof(mxs_vector))=triangle[i];
   std::ofstream output(path,std::ios::binary);CHECK(bool(output));
   for(unsigned pose=0;pose<2;++pose) {
      model.segments[0].transform.vec.x=float(pose)*3;
      if(pose)model.segments[0].rotation={std::cos(.39269908f),std::sin(.39269908f),0,0};
      CHECK(meshGeometry(model,asset.mesh(),asset.bytes.size()));
      const auto &vertices=model.parts.at(0).vertices;CHECK(vertices.size()==3);
      output.write(reinterpret_cast<const char *>(vertices.data()),vertices.size()*sizeof(RenderSceneVertex));
   }
   CHECK(bool(output));
}


void testNativeLights() {
   mls_multi_light source{};sMissionRenderParams mission{};
   source.loc={1,2,3};source.bright={8,4,2};source.inner=-1;source.radius=40;
   const auto point=WorldSceneConvertNativeLight(source,mission,false,false);
   CHECK(point.castsShadow==1);near(point.sourceRadius,.25f);near(point.angularRadius,.27f*3.14159265f/180);
   CHECK(point.type==RenderScenePointLight && point.range==40 && point.enabled);
   near(point.intensity*point.color[0]/(3.14159265f*16*16),source.bright.x/16);
   source.inner=.8f;source.outer=.5f;source.dir={1,0,0};
   const auto spot=WorldSceneConvertNativeLight(source,mission,false,false);
   CHECK(spot.type==RenderSceneSpotLight);near(spot.innerCos,.8f);near(spot.outerCos,.5f);
   source.inner=source.radius=std::numeric_limits<float>::quiet_NaN();
   const auto dynamic=WorldSceneConvertNativeLight(source,mission,false,true);
   CHECK(dynamic.type==RenderScenePointLight && dynamic.range==0 && dynamic.innerCos==0);
   mission.sunlight_vector={0,0,-100};mission.sun_scaled_rgb={1,.5f,.25f};
   const auto sun=WorldSceneConvertNativeLight(source,mission,true,false);
   CHECK(sun.type==RenderSceneDirectionalLight && sun.range==0);
   near(sun.intensity*sun.color[1]/3.14159265f,.5f/125.0f);near(sun.direction[2],-100);
   CHECK(std::isnan(source.radius)); // Native/gameplay light records remain untouched.
   std::cout<<"PASS native point/spot/sun light conversion and stale dynamic-light metadata\n";
}

}
int main(int argc,char **argv) {
   // Native alpha suppression requires a complete, current modern import.
   CHECK(!WorldSceneHasModel(12345));
   auto &coverage=cache().objects[12345];
   coverage.seen=true;
   CHECK(!WorldSceneHasModel(12345));
   auto &first=coverage.parts[0];first.vertices.resize(3);
   CHECK(!WorldSceneHasModel(12345));
   first.instance=1;
   CHECK(WorldSceneHasModel(12345));
   auto &second=coverage.parts[1];second.vertices.resize(3);
   CHECK(!WorldSceneHasModel(12345));
   second.instance=2;
   CHECK(WorldSceneHasModel(12345));
   coverage.seen=false;
   CHECK(!WorldSceneHasModel(12345));
   cache().objects.erase(12345);
   std::puts("PASS native alpha fallback for absent, stale and incomplete modern imports");

   {
      Asset prefix;const unsigned offset=prefix.append<unsigned char>(8);
      float values[]={.7f,1};std::memcpy(prefix.bytes.data()+offset,values,sizeof(values));
      auto *header=reinterpret_cast<mds_model*>(prefix.bytes.data());header->amat_off=offset;header->amat_size=8;header->mod_size=unsigned(prefix.bytes.size());
      float trans=0,glow=0;CHECK(mdMaterialLighting(header,0,trans,glow));CHECK(trans==.7f && glow==1);
      header->amat_size=7;CHECK(!mdMaterialLighting(header,0,trans,glow));header->amat_size=8;
      CHECK(!mdMaterialLighting(header,header->mats,trans,glow));
      --header->mod_size;CHECK(!mdMaterialLighting(header,0,trans,glow));++header->mod_size;
      header->amat_off=UINT32_MAX;CHECK(!mdMaterialLighting(header,0,trans,glow));
   }
   std::puts("PASS eight-byte native MD material prefix, transparency, illumination and truncated bounds");
   WorldSceneMaterialProperties nativeGlow;
   CHECK(nativeGlow.MdIllumination(0)==0 && nativeGlow.MdIllumination(1)==1);
   nativeGlow.selfIllumPresent=true;nativeGlow.selfIllum=1;
   CHECK(nativeGlow.MdIllumination(0)==0 && nativeGlow.MdIllumination(1)==1);
   nativeGlow.selfIllum=.25f;
   CHECK(nativeGlow.MdIllumination(0)==0 && nativeGlow.MdIllumination(.8f)==.2f);
   nativeGlow.selfIllum=0;CHECK(nativeGlow.MdIllumination(1)==0);
   nativeGlow.selfIllumPresent=false;CHECK(nativeGlow.MdIllumination(1)==1);
   std::puts("PASS native MD material glow scaling preserves dark metal, emissive bulbs and missing-property defaults");
   WorldSceneMaterialProperties properties;
   unsigned reads=0;float liveAlpha=.25f,liveIllum=.5f;bool hasAlpha=true,hasIllum=true,hasShadow=true,liveShadow=false;
   auto readProperties=[&](float &alpha,float &illum,bool &shadow){++reads;if(hasAlpha)alpha=liveAlpha;if(hasIllum)illum=liveIllum;if(hasShadow)shadow=liveShadow;};
   properties.Refresh(1,readProperties);CHECK(reads==1 && properties.alpha==.25f && properties.selfIllum==.5f && !properties.castsShadow);
   properties.Refresh(1,readProperties);CHECK(reads==1);
   liveAlpha=.75f;liveIllum=.125f;
   properties.Refresh(2,readProperties);CHECK(reads==2 && properties.alpha==.75f && properties.selfIllum==.125f);
   hasAlpha=hasIllum=hasShadow=false;properties.Refresh(3,readProperties);
   CHECK(reads==3 && properties.alpha==1 && properties.selfIllum==0 && properties.castsShadow);
   hasAlpha=true;liveAlpha=.5f;properties.Refresh(4,readProperties);
   CHECK(reads==4 && properties.alpha==.5f && properties.selfIllum==0);
   std::puts("PASS material property revision reuse, live updates and removal defaults");

   WorldSceneBeginObjectCapture(1);
   CHECK(objectReceivesFog(100,true) && !objectReceivesFog(100,false));
   WorldSceneCaptureObjectFog(100,1);CHECK(objectReceivesFog(100,false));
   WorldSceneCaptureObjectFog(101,0);CHECK(!objectReceivesFog(101,true));
   WorldSceneCaptureObjectFog(100,0);CHECK(!objectReceivesFog(100,true));
   WorldSceneCaptureObjectFog(101,1);CHECK(!objectReceivesFog(101,true));
   WorldSceneBeginObjectCapture(1);CHECK(objectReceivesFog(101,true));
   WorldSceneCaptureObjectFog(101,1);CHECK(objectReceivesFog(101,false));
   WorldSceneBeginObjectCapture(0);WorldSceneCaptureObjectFog(101,1);
   CHECK(!objectReceivesFog(101,false));
   std::puts("PASS frame-local object fog, cell fallback, mixed fragments and disabled capture");

   testNativeLights();
   testAttachments();
   testMesh();
   Asset asset;Model decoded;
   CHECK(geometry(decoded,asset.model()));CHECK(decoded.parts.size()==1);
   auto &part=decoded.parts.begin()->second;
   CHECK(part.indices==std::vector<uint32_t>({0,1,2}));near(part.vertices[0].position[0],3);
   near(part.vertices[2].uv[0],2);near(part.vertices[2].uv[1],.5f);
   decoded.joints[0]=90;CHECK(geometry(decoded,asset.model()));
   near(part.vertices[1].position[1],0);near(part.vertices[1].position[2],1);
   decoded.scale={2,3,4};CHECK(geometry(decoded,asset.model()));
   near(part.vertices[0].position[0],6);near(part.vertices[1].position[2],3);
   near(part.vertices[2].position[1],-4);
   auto &child=asset.at<mds_subobj>(asset.model()->subobj_off+sizeof(mds_subobj));
   child.type=MD_SUB_SLIDE;decoded.joints[0]=5;CHECK(geometry(decoded,asset.model()));
   near(part.vertices[0].position[0],11);
   child.child=1;CHECK(!geometry(decoded,asset.model()));child.child=-1;
   child.parm=MAX_REND_JOINTS;CHECK(!geometry(decoded,asset.model()));child.parm=0;
   asset.at<mds_pgon>(asset.model()->pgon_off).verts[0]=3;CHECK(!geometry(decoded,asset.model()));
   asset.at<mds_pgon>(asset.model()->pgon_off).verts[0]=0;
   --asset.model()->node_off;CHECK(!geometry(decoded,asset.model()));
   std::cout<<"PASS MD polygons, UVs, joint pivots, scale, slide, cyclic joints and malformed indices/bounds\n";
   if(argc==2)writeMeshShadowFixture(argv[1]);
}
