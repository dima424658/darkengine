#include "../../libsrc/lgd3d/world_history.hpp"
#include <cstdio>
#include <stdexcept>
static void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(){try {
   dark::WorldScene scene;
   dark::SceneMesh mesh{};mesh.deformable=true;mesh.revision=1;mesh.vertices.resize(3);
   scene.meshes.emplace(1,mesh);
   RenderSceneMaterial material{};material.roughness=.8f;scene.materials.emplace(3,material);
   RenderSceneInstance instance{};instance.mesh=1;instance.visible=1;
   for(unsigned i=0;i<4;++i)instance.transform[i*5]=1;
   scene.instances.emplace(2,instance);
   dark::WorldHistory history;
   check(!history.Prepare(scene,128,64),"first frame has no history");
   history.CompleteSubmission(true);
   scene.instances.at(2).transform[12]=1;
   scene.meshes.at(1).vertices[0].position[0]=3;++scene.meshes.at(1).revision;
   check(history.Prepare(scene,128,64),"next submitted frame has history");
   check(history.Previous()->instances.at(2).transform[12]==0,"previous pose is copied");
   check(history.Previous()->meshes.at(1).vertices->at(0).position[0]==0,"previous deformed vertices are owned");
   history.CompleteSubmission(false);
   check(history.Prepare(scene,128,64),"failed submission preserves accepted history");
   check(history.Previous()->instances.at(2).transform[12]==0,"failed pose never becomes history");
   history.CompleteSubmission(true);
   check(history.Prepare(scene,128,64),"history advances after accepted submission");
   check(history.Previous()->instances.at(2).transform[12]==1,"accepted pose becomes previous frame");
   check(history.MaterialMatches(3,material),"unchanged material is reprojectable");
   auto changed=material;changed.roughness=.2f;
   check(!history.MaterialMatches(3,changed),"changed material rejects only its own history");
   const auto vertices=history.Previous()->meshes.at(1).vertices;
   check(vertices->at(0).position[0]==3,"accepted deformation becomes previous frame");
   history.CompleteSubmission(true);history.Prepare(scene,128,64);
   check(history.Previous()->meshes.at(1).vertices==vertices,"unchanged meshes share immutable snapshots");
   history.CompleteSubmission(false);
   check(!history.Prepare(scene,256,64),"resize invalidates reprojection");history.CompleteSubmission(false);
   check(history.Prepare(scene,128,64),"failed resize retains accepted dimensions");history.CompleteSubmission(false);
   ++scene.materialsRevision;scene.materials.at(3).roughness=.2f;
   check(history.Prepare(scene,128,64),"unrelated surface history survives material revision changes");
   check(!history.MaterialMatches(3,scene.materials.at(3)),"changed surface material is rejected");history.CompleteSubmission(false);
   scene.camera.position[0]=65;
   check(!history.Prepare(scene,128,64),"camera cut invalidates history");history.CompleteSubmission(true);
   check(history.Prepare(scene,128,64),"camera cut becomes valid after accepted frame");history.CompleteSubmission(false);
   ++scene.resetRevision;
   check(!history.Prepare(scene,128,64),"world reset invalidates history");history.CompleteSubmission(true);
   auto other=scene;
   check(!history.Prepare(other,128,64),"different world owner invalidates history");
   history.Reset();check(!history.Previous(),"explicit reset releases temporal state");
   std::puts("PASS submitted-frame poses, deformation, failed submits, resize, camera cuts and world identity");return 0;
 }catch(const std::exception &failure){std::fprintf(stderr,"history: %s\n",failure.what());return 1;}}
