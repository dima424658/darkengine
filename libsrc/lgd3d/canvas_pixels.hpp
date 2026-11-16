#ifndef DARK_CANVAS_PIXELS_HPP
#define DARK_CANVAS_PIXELS_HPP
#include <array>
#include <cstdint>
#include <cstddef>
#include <cstring>
namespace dark {
using CanvasPixel=std::array<uint8_t,4>;
static_assert(sizeof(CanvasPixel)==4,"canvas RGBA pixel stride");
inline const std::array<CanvasPixel,65536> &CanvasColorTable(bool rgb565) {
   static const auto table555=[] {
      std::array<CanvasPixel,65536> table{};
      for(uint32_t value=0;value<table.size();++value)
         table[value]={{uint8_t(((value>>10)&31)*255/31),uint8_t(((value>>5)&31)*255/31),uint8_t((value&31)*255/31),255}};
      return table;
   }();
   static const auto table565=[] {
      std::array<CanvasPixel,65536> table{};
      for(uint32_t value=0;value<table.size();++value)
         table[value]={{uint8_t(((value>>11)&31)*255/31),uint8_t(((value>>5)&63)*255/63),uint8_t((value&31)*255/31),255}};
      return table;
   }();
   return rgb565?table565:table555;
}
inline CanvasPixel CanvasConvert16(uint16_t value,const CanvasPixel *table,bool sceneActive) {
   CanvasPixel color=table[value];
   if(sceneActive && value==0)color[3]=0;
   return color;
}

// Source and destination are separate rows. memcpy permits unaligned canvases.
inline void CanvasConvert16Row(const uint8_t *source,uint8_t *destination,std::size_t count,const CanvasPixel *table,bool sceneActive) {
   std::size_t x=0;
   if(sceneActive)for(;count-x>=4;x+=4) {
      uint64_t words;std::memcpy(&words,source+x*2,sizeof(words));
      if(words==0) {std::memset(destination+x*4,0,16);continue;}
      for(std::size_t j=0;j<4;++j) {
         uint16_t value;std::memcpy(&value,source+(x+j)*2,sizeof(value));
         const auto color=CanvasConvert16(value,table,true);
         std::memcpy(destination+(x+j)*4,color.data(),color.size());
      }
   }
   for(;x<count;++x) {
      uint16_t value;std::memcpy(&value,source+x*2,sizeof(value));
      const auto color=CanvasConvert16(value,table,sceneActive);
      std::memcpy(destination+x*4,color.data(),color.size());
   }
}
}
#endif
