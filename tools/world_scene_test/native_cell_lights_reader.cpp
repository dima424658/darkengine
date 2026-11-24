#include "world_scene.hpp"
#include <algorithm>
extern "C" unsigned CheckObjectLightList(void *world,int object,const RenderSceneHandle *lights,unsigned count) {
 const auto &scene=*static_cast<dark::WorldScene*>(world);
 std::vector<RenderSceneHandle> expected;if(count)expected.assign(lights,lights+count);
 std::sort(expected.begin(),expected.end());expected.erase(std::unique(expected.begin(),expected.end()),expected.end());
 unsigned parts=0;
 for(const auto &entry:scene.instances)if(entry.second.objectId==object) {
  const auto found=scene.instanceLights.find(entry.first);
  if(found==scene.instanceLights.end() || found->second!=expected)return 0;
  ++parts;
 }
 return parts;
}
extern "C" unsigned CheckDynamicLight(void *world,RenderSceneHandle light,unsigned present) {
 const auto &scene=*static_cast<dark::WorldScene*>(world);
 if(bool(scene.lights.count(light))!=bool(present))return 0;
 for(const auto &entry:scene.instanceLights)
  if(std::binary_search(entry.second.begin(),entry.second.end(),light)!=bool(present))return 0;
 return !scene.instanceLights.empty();
}
// Fixture-only Linux x86-64 PortalCell layout. Native cells remain read-only.
#include <cstring>
extern "C" int OtherCellCenter(void **cells,int count,int oldCell,void *compute,float *out) {
 struct Location {float point[3];short cell,hint;};
 auto locate=reinterpret_cast<int(*)(void*)>(compute);
 auto indices=[](void *cell) {const unsigned short *p;std::memcpy(&p,static_cast<char*>(cell)+104,sizeof(p));return p;};
 if(oldCell<0 || oldCell>=count || !cells[oldCell])return -1;
 const auto *old=indices(cells[oldCell]);if(!old)return -1;
 for(int i=0;i<count;++i) {
  if(!cells[i] || i==oldCell || static_cast<unsigned char*>(cells[i])[5]!=static_cast<unsigned char*>(cells[oldCell])[5])continue;
  const auto *list=indices(cells[i]);if(!list || list[0]==old[0])continue;
  Location loc{};std::memcpy(loc.point,static_cast<char*>(cells[i])+112,12);loc.cell=loc.hint=-1;
  const int cell=locate(&loc);
  if(cell<0 || cell>=count || cell==oldCell || !cells[cell])continue;
  list=indices(cells[cell]);if(!list || list[0]==old[0])continue;
  std::copy(loc.point,loc.point+3,out);return cell;
 }
 return -1;
}
