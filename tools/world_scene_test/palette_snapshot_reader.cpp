#include "world_scene.hpp"
#include <cstring>
extern "C" unsigned ReadSceneTexture(void *scene,unsigned long long handle) {
 const auto &textures=static_cast<dark::WorldScene*>(scene)->textures;
 const auto found=textures.find(handle);if(found==textures.end() || found->second.rgba.size()<4)return 0;
 unsigned pixel;std::memcpy(&pixel,found->second.rgba.data(),4);return pixel;
}
