#include "../../libsrc/namedres/material_metadata.hpp"
#include <cstdio>
#include <stdexcept>
static void check(bool value,const char *reason){if(!value)throw std::runtime_error(reason);}
int main() {
 try {
   auto scale=ParseTerrainScale("terrain_scale 64");check(scale.width==64 && scale.height==64,"square virtual dimensions");
   scale=ParseTerrainScale("\xef\xbb\xbf// pack metadata\r\nTERRAIN_SCALE 64 128 // rectangular\r\nillum_map 0.5 glow");
   check(scale.width==64 && scale.height==128,"BOM, comments, CRLF and rectangular dimensions");
   scale=ParseTerrainScale("# terrain_scale 16\nterrain_scale 128 # actual\n");check(scale.width==128,"commented directive ignored");
   for(const char *text:{"","roughness 1","terrain_scale","terrain_scale -1","terrain_scale 0","terrain_scale 63","terrain_scale 64 0","terrain_scale 64 128 256","terrain_scale 4294967296"})
      check(ParseTerrainScale(text).width==0,"invalid or missing dimensions retain image defaults");
   scale=ParseTerrainScale(".ifdef 0\nterrain_scale 16\n.else\nterrain_scale 64\n.endif");check(scale.width==64,"inactive conditional ignored");
   scale=ParseTerrainScale(".ifdef DX6\nterrain_scale 16\n.else\nterrain_scale 128 64\n.endif");check(scale.width==128 && scale.height==64,"modern renderer is not DX6");
   const auto enabled=[](std::string_view key){return key=="small_textures";};
   scale=ParseTerrainScale(".ifdef $small_textures\n.ifndef 0\nterrain_scale 32\n.endif\n.else\nterrain_scale 256\n.endif",enabled);
   check(scale.width==32,"nested material config condition");
   std::puts("PASS terrain_scale metadata, rectangular sizes, comments, conditions and malformed values");return 0;
 }catch(const std::exception &e){std::fprintf(stderr,"material metadata: %s\n",e.what());return 1;}
}
