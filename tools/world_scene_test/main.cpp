#include <render_scene.h>
#include "../../libsrc/lgd3d/canvas_pixels.hpp"
#include "../../libsrc/lgd3d/world_scene.hpp"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <limits>
static void check(bool pass) {
   if(!pass){std::fputs("world scene regression failed\n",stderr);std::exit(1);}
}
static void identity(float *m){for(int i=0;i<16;++i)m[i]=(i%5==0)?1.0f:0.0f;}
int main() {
   // Register before constructing the store, matching legacy AppOnExit order.
   std::atexit([]{RenderSceneClear();RenderSceneStats stats{};RenderSceneGetStats(&stats);check(stats.meshes==0);});
   for(unsigned depth:{15u,16u})for(uint32_t value=0;value<65536;++value) {
      const auto &pixel=dark::CanvasColorTable(depth==16)[value];
      check(pixel[0]==((value>>(depth==16?11:10))&31)*255/31);
      check(pixel[1]==((value>>5)&(depth==16?63:31))*255/(depth==16?63:31));
      check(pixel[2]==(value&31)*255/31 && pixel[3]==255);
      const auto opaque=dark::CanvasConvert16(uint16_t(value),dark::CanvasColorTable(depth==16).data(),false);
      const auto overlay=dark::CanvasConvert16(uint16_t(value),dark::CanvasColorTable(depth==16).data(),true);
      check(opaque==pixel);
      check(overlay[0]==pixel[0] && overlay[1]==pixel[1] && overlay[2]==pixel[2] && overlay[3]==(value==0?0:255));
   }
   // Check block/tail paths against scalar conversion, including unaligned
   // rows, mixed zero runs, the RGB555 high bit, and opaque menu canvases.
   for(bool rgb565:{false,true})for(bool scene:{false,true})for(size_t count=0;count<130;++count)for(unsigned pattern=0;pattern<4;++pattern) {
      std::vector<uint8_t> source(count*2+2,0x6d),actual(count*4+6,0xad),expected=actual;
      const auto *table=dark::CanvasColorTable(rgb565).data();
      for(size_t x=0;x<count;++x) {
         const uint16_t value=pattern==0?0:pattern==1?uint16_t(x%7==0?0x8000:0):pattern==2?uint16_t(x*1297):uint16_t(x%9<6?0:x*749);
         std::memcpy(source.data()+1+x*2,&value,sizeof(value));
         const auto color=dark::CanvasConvert16(value,table,scene);
         std::memcpy(expected.data()+3+x*4,color.data(),4);
      }
      dark::CanvasConvert16Row(source.data()+1,actual.data()+3,count,table,scene);
      check(actual==expected);
   }
   RenderSceneClear();
   uint8_t pixels[12]={255,0,0,255, 0,0,0,0, 0,255,0,255};
   const auto texture=RenderSceneCreateTexture(1,2,pixels,8);
   check(texture!=0);pixels[0]=0;
   check(dark::GetWorldScene().textures.at(texture).rgba[0]==255);
   check(dark::GetWorldScene().textures.at(texture).rgba[5]==255);
   check(!RenderSceneCreateTexture(UINT32_MAX,1,pixels,4));
   check(!RenderSceneCreateTexture(2,1,pixels,4));
   RenderSceneMaterial material{};material.baseColorTexture=texture;
   for(float &c:material.baseColor)c=1;
   material.roughness=0.8f;material.alphaCutoff=0.5f;
   const auto materialRevision=dark::GetWorldScene().materialsRevision;
   const auto mat=RenderSceneCreateMaterial(&material);check(mat!=0);
   check(dark::GetWorldScene().materialsRevision==materialRevision+1);
   check(RenderSceneUpdateMaterial(mat,&material));
   check(dark::GetWorldScene().materialsRevision==materialRevision+1);
   auto editedMaterial=material;editedMaterial.roughness=.7f;
   check(RenderSceneUpdateMaterial(mat,&editedMaterial));
   check(dark::GetWorldScene().materialsRevision==materialRevision+2);
   check(RenderSceneUpdateMaterial(mat,&material));
   const auto stableMaterialRevision=dark::GetWorldScene().materialsRevision;
   const auto oldTextureRevision=dark::GetWorldScene().textures.at(texture).revision;
   check(RenderSceneReplaceTexture(texture,2,1,pixels,8));
   check(dark::GetWorldScene().textures.at(texture).width==2 && dark::GetWorldScene().textures.at(texture).height==1);
   check(dark::GetWorldScene().textures.at(texture).revision>oldTextureRevision);
   check(dark::GetWorldScene().materials.at(mat).baseColorTexture==texture);
   check(dark::GetWorldScene().materialsRevision==stableMaterialRevision);
   const auto resizedRevision=dark::GetWorldScene().textures.at(texture).revision;
   check(!RenderSceneReplaceTexture(texture,3,1,pixels,8));
   check(!RenderSceneReplaceTexture(texture,0,1,pixels,8));
   check(!RenderSceneReplaceTexture(texture,UINT32_MAX,1,pixels,8));
   check(!RenderSceneReplaceTexture(99999,1,1,pixels,4));
   check(RenderSceneReplaceTexture(texture,2,1,pixels,8));
   check(dark::GetWorldScene().textures.at(texture).revision==resizedRevision);
   check(dark::GetWorldScene().textures.at(texture).alphaRevision==resizedRevision);
   uint8_t padded[16]={1,2,3,255,77,77,77,77,4,5,6,255,77,77,77,77};
   const auto paddedTexture=RenderSceneCreateTexture(1,2,padded,8);check(paddedTexture!=0);
   const auto paddedRevision=dark::GetWorldScene().textures.at(paddedTexture).revision;
   padded[4]=12;check(RenderSceneUpdateTexture(paddedTexture,padded,8));
   check(dark::GetWorldScene().textures.at(paddedTexture).revision==paddedRevision);
   padded[11]=64;check(RenderSceneUpdateTexture(paddedTexture,padded,8));
   check(dark::GetWorldScene().textures.at(paddedTexture).alphaRevision>paddedRevision);
   check(RenderSceneDestroy(paddedTexture));
   const auto alphaRevision=dark::GetWorldScene().textures.at(texture).alphaRevision;
   pixels[0]^=0x7f;check(RenderSceneUpdateTexture(texture,pixels,8));
   check(dark::GetWorldScene().textures.at(texture).revision>resizedRevision);
   check(dark::GetWorldScene().textures.at(texture).alphaRevision==alphaRevision);
   pixels[7]=128;check(RenderSceneUpdateTexture(texture,pixels,8));
   check(dark::GetWorldScene().textures.at(texture).alphaRevision>alphaRevision);
   const auto changedAlphaRevision=dark::GetWorldScene().textures.at(texture).alphaRevision;
   check(RenderSceneUpdateTexture(texture,pixels,8));
   check(dark::GetWorldScene().textures.at(texture).alphaRevision==changedAlphaRevision);
   auto invalid=material;invalid.baseColorTexture=99999;check(!RenderSceneCreateMaterial(&invalid));
   invalid=material;invalid.roughness=std::numeric_limits<float>::quiet_NaN();check(!RenderSceneUpdateMaterial(mat,&invalid));
   RenderSceneVertex vertices[3]={{{0,0,0},{0,0,1},{0,0}},{{1,0,0},{0,0,1},{1,0}},{{0,1,0},{0,0,1},{0,1}}};
   uint32_t indices[]={0,1,2};
   RenderSceneMesh mesh{vertices,3,indices,3,mat,1};
   indices[2]=3;check(!RenderSceneCreateMesh(&mesh));indices[2]=2;
   mesh.indexCount=2;check(!RenderSceneCreateMesh(&mesh));mesh.indexCount=3;
   const auto geometry=RenderSceneCreateMesh(&mesh);check(geometry!=0);
   vertices[1].position[0]=2;check(dark::GetWorldScene().meshes.at(geometry).vertices[1].position[0]==1);
   const auto revision=dark::GetWorldScene().meshes.at(geometry).revision;
   check(!RenderSceneUpdateVertices(geometry,vertices,2));
   check(RenderSceneUpdateVertices(geometry,vertices,3));
   check(dark::GetWorldScene().meshes.at(geometry).vertices[1].position[0]==2);
   check(dark::GetWorldScene().meshes.at(geometry).bounds.maximum[0]==2);
   check(dark::GetWorldScene().meshes.at(geometry).revision>revision);
   RenderSceneInstance instance{};instance.mesh=geometry;instance.visible=instance.castsShadow=1;
   identity(instance.transform);identity(instance.previousTransform);
   auto badTransform=instance;badTransform.transform[15]=0;
   check(!RenderSceneCreateInstance(&badTransform));
   badTransform=instance;badTransform.transform[0]=0;check(!RenderSceneCreateInstance(&badTransform));
   const auto object=RenderSceneCreateInstance(&instance);check(object!=0);
   instance.transform[12]=4;check(RenderSceneUpdateInstance(object,&instance));
   check(!RenderSceneDestroy(geometry) && !RenderSceneDestroy(mat) && !RenderSceneDestroy(texture));
   RenderSceneLight light{};light.type=RenderScenePointLight;light.enabled=1;light.intensity=3;
   light.color[0]=1;light.range=10;
   const auto lamp=RenderSceneCreateLight(&light);check(lamp!=0);
   RenderSceneHandle linked[]={lamp,lamp};
   check(RenderSceneSetInstanceLights(object,linked,2));linked[0]=0;
   check(dark::GetWorldScene().instanceLights.at(object)==std::vector<RenderSceneHandle>{lamp});
   const auto linkedRevision=dark::GetWorldScene().revision;
   check(RenderSceneSetInstanceLights(object,&lamp,1));check(dark::GetWorldScene().revision==linkedRevision);
   check(!RenderSceneSetInstanceLights(object,&texture,1));
   check(dark::GetWorldScene().instanceLights.at(object)==std::vector<RenderSceneHandle>{lamp});
   check(!RenderSceneSetInstanceLights(lamp,&lamp,1));check(!RenderSceneSetInstanceLights(object,nullptr,1));
   check(RenderSceneSetInstanceLights(object,nullptr,0));check(dark::GetWorldScene().instanceLights.at(object).empty());
   check(RenderSceneClearInstanceLights(object));check(!dark::GetWorldScene().instanceLights.count(object));
   check(RenderSceneSetInstanceLights(object,&lamp,1));
   auto badLight=light;badLight.sourceRadius=-1;check(!RenderSceneUpdateLight(lamp,&badLight));
   badLight=light;badLight.angularRadius=std::numeric_limits<float>::quiet_NaN();check(!RenderSceneCreateLight(&badLight));
   badLight=light;badLight.angularRadius=2;check(!RenderSceneUpdateLight(lamp,&badLight));
   light.type=RenderSceneDirectionalLight;check(!RenderSceneUpdateLight(lamp,&light));
   light.direction[2]=-1;check(RenderSceneUpdateLight(lamp,&light));
   RenderSceneCamera camera{};identity(camera.viewProjection);identity(camera.previousViewProjection);
   camera.exposure=1;camera.lightScale=1;check(RenderSceneSetCamera(&camera));
   camera.fogDistance=-1;check(!RenderSceneSetCamera(&camera));
   camera.fogDistance=std::numeric_limits<float>::infinity();check(!RenderSceneSetCamera(&camera));
   camera.fogDistance=10;camera.fogColor[0]=-.1f;check(!RenderSceneSetCamera(&camera));
   camera.fogColor[0]=std::numeric_limits<float>::quiet_NaN();check(!RenderSceneSetCamera(&camera));
   camera.fogColor[0]=.2f;check(RenderSceneSetCamera(&camera));
   camera.nativeDepth[1]=std::numeric_limits<float>::quiet_NaN();check(!RenderSceneSetCamera(&camera));
   camera.nativeDepth[1]=-1;camera.nativeDepth[2]=1;camera.nativeDepth[3]=1;check(RenderSceneSetCamera(&camera));
   camera.position[0]=std::numeric_limits<float>::infinity();check(!RenderSceneSetCamera(&camera));
   RenderSceneStats stats{};RenderSceneGetStats(&stats);
   check(stats.meshes==1 && stats.instances==1 && stats.materials==1 && stats.lights==1 && stats.triangles==1 && stats.vertices==3);
   check(RenderSceneDestroy(object));check(!dark::GetWorldScene().instanceLights.count(object));check(RenderSceneDestroy(geometry));
   const auto beforeMaterialRemoval=dark::GetWorldScene().materialsRevision;
   check(RenderSceneDestroy(mat));check(RenderSceneDestroy(texture));
   check(dark::GetWorldScene().materialsRevision==beforeMaterialRemoval+1);
   RenderSceneCamera projection{};
   const float eye[]={12,-8,3},forward[]={1,0,0},right[]={1,-0.5f,0},bottom[]={1,0,-0.25f};
   check(RenderSceneCameraFromRays(&projection,eye,forward,right,bottom,0.1f,100));
   auto project=[&](float x,float y,float z,int row) {
      const float point[]={x,y,z,1};float clip=0,w=0;
      for(int col=0;col<4;++col){clip+=projection.viewProjection[col*4+row]*point[col];w+=projection.viewProjection[col*4+3]*point[col];}
      return clip/w;
   };
   check(std::abs(project(22,-8,3,0))<1e-5f && std::abs(project(22,-8,3,1))<1e-5f);
   check(std::abs(project(22,-13,3,0)-1)<1e-5f);
   check(std::abs(project(22,-8,0.5f,1)-1)<1e-5f);
   check(std::abs(project(12.1f,-8,3,2))<1e-4f && std::abs(project(112,-8,3,2)-1)<1e-5f);
   check(!RenderSceneCameraFromRays(&projection,eye,forward,forward,bottom,0.1f,100));
   check(!RenderSceneCameraFromRays(&projection,eye,forward,right,bottom,0,100));
   {
      const float origin[]={0,0,0},f[]={1,0,0},r[]={1,-1,0},b[]={1,0,-1};
      check(RenderSceneCameraFromRays(&projection,origin,f,r,b,.1f,100));
      dark::SceneBounds box{{-.5f,-.5f,-.5f},{.5f,.5f,.5f}};
      float transform[16];identity(transform);
      auto visible=[&](float x,float y,float z) {
         transform[12]=x;transform[13]=y;transform[14]=z;
         const bool reference=dark::SceneBoundsVisible(box,transform,projection.viewProjection);
         const bool prepared=dark::SceneBoundsVisible(dark::SceneTransformBounds(box,transform),dark::SceneExtractFrustum(projection.viewProjection));
         check(reference==prepared);return reference;
      };
      check(visible(5,0,0));check(!visible(-2,0,0));check(!visible(102,0,0));
      check(!visible(5,7,0) && !visible(5,-7,0));
      check(!visible(5,0,7) && !visible(5,0,-7));
      check(visible(5,5,0) && visible(.1f,0,0) && visible(100,0,0));
      // A box surrounding the camera cannot be rejected by an individual
      // clip plane, even when all original corners lie outside the viewport.
      box={{-100,-100,-100},{100,100,100}};check(visible(0,0,0));
      // A rotated, mirrored, nonuniformly scaled object uses local bounds.
      box={{-10,-.5f,-.5f},{10,.5f,.5f}};identity(transform);
      transform[0]=0;transform[1]=2;transform[4]=-.5f;transform[5]=0;transform[10]=-3;
      check(visible(5,15,0));check(!visible(5,40,0));
      for(unsigned angle=0;angle<12;++angle) {
         const float a=float(angle)*.37f;identity(transform);
         transform[0]=2*std::cos(a);transform[1]=2*std::sin(a);
         transform[4]=.5f*std::sin(a);transform[5]=-.5f*std::cos(a);transform[10]=3;
         for(int x=-3;x<15;++x)for(int y=-15;y<15;++y)visible(x*7.13f,y*6.29f,2.17f);
      }
   }
   const auto oldRevision=stats.revision;
   const auto beforeMaterialClear=dark::GetWorldScene().materialsRevision;
   RenderSceneClear();check(!RenderSceneDestroy(lamp));
   check(dark::GetWorldScene().materialsRevision==beforeMaterialClear+1);
   RenderSceneGetStats(&stats);check(!stats.meshes && !stats.instances && !stats.lights && stats.revision>oldRevision);
   light.type=RenderScenePointLight;
   const auto newLamp=RenderSceneCreateLight(&light);check(newLamp>lamp);
   check(!RenderSceneUpdateLight(lamp,&light));
   RenderSceneClear();
   std::puts("PASS: copied assets, invalid input, handles, pose/bounds updates, Vulkan frustum planes, transforms, references and mission reset");
}
