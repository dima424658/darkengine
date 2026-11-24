#include "dds_decode.hpp"
#include <algorithm>
#include <cstring>
#define BCDEC_STATIC
#define BCDEC_IMPLEMENTATION
#define BCDEC_BC4BC5_PRECISE
#include "../../3rdparty/bcdec/bcdec.h"
namespace {
uint32_t le32(const uint8_t *p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
constexpr uint32_t fourcc(char a,char b,char c,char d){return uint32_t(a)|(uint32_t(b)<<8)|(uint32_t(c)<<16)|(uint32_t(d)<<24);}
uint8_t masked(uint32_t value,uint32_t mask,uint8_t fallback) {
   if(!mask)return fallback;
   unsigned shift=0;while(!(mask&1)){mask>>=1;++shift;}
   return uint8_t((uint64_t((value>>shift)&mask)*255+mask/2)/mask);
}
}
bool ResDecodeDds(const uint8_t *bytes,size_t size,ResImagePixels &output) {
   output={};
   if(!bytes || size<128 || std::memcmp(bytes,"DDS ",4) || le32(bytes+4)!=124 || le32(bytes+76)!=32)return false;
   const uint32_t width=le32(bytes+16),height=le32(bytes+12),caps=le32(bytes+112);
   if(!width || !height || width>32767 || height>32767 || uint64_t(width)*height>512u*1024u*1024u/4 ||
      (caps&0x0020fe00u) || le32(bytes+24)>1)return false;
   uint32_t block=0,bits=le32(bytes+88);
   uint32_t masks[4]={le32(bytes+92),le32(bytes+96),le32(bytes+100),le32(bytes+104)};
   size_t offset=128;
   const uint32_t flags=le32(bytes+80),code=le32(bytes+84);
   if(flags&4) {
      switch(code) {
      case fourcc('D','X','T','1'):block=1;break;
      case fourcc('D','X','T','3'):block=2;break;
      case fourcc('D','X','T','5'):block=3;break;
      case fourcc('A','T','I','1'):case fourcc('B','C','4','U'):block=4;break;
      case fourcc('A','T','I','2'):case fourcc('B','C','5','U'):block=5;break;
      case fourcc('D','X','1','0'): {
         if(size<148 || le32(bytes+132)!=3 || (le32(bytes+136)&4) || le32(bytes+140)!=1)return false;
         offset=148;
         switch(le32(bytes+128)) {
         case 71:case 72:block=1;break;
         case 74:case 75:block=2;break;
         case 77:case 78:block=3;break;
         case 80:block=4;break;
         case 83:block=5;break;
         case 98:case 99:block=7;break;
         case 28:case 29:bits=32;masks[0]=0xff;masks[1]=0xff00;masks[2]=0xff0000;masks[3]=0xff000000;break;
         case 87:case 91:case 88:case 93:
            bits=32;masks[0]=0xff0000;masks[1]=0xff00;masks[2]=0xff;masks[3]=(le32(bytes+128)==88 || le32(bytes+128)==93)?0:0xff000000;break;
         case 61:bits=8;masks[0]=0xff;masks[1]=masks[2]=masks[3]=0;break;
         case 49:bits=16;masks[0]=0xff;masks[1]=0xff00;masks[2]=masks[3]=0;break;
         default:return false;
         }
         break;
      }
      default:return false;
      }
   } else if(!(flags&0x40) && !(flags&0x20000))return false;
   const size_t blocksWide=(size_t(width)+3)/4,blocksHigh=(size_t(height)+3)/4;
   const size_t blockBytes=(block==1 || block==4)?8:16;
   size_t pitch=size_t(width)*(bits/8);
   if(!block) {
      if(bits!=8 && bits!=16 && bits!=24 && bits!=32)return false;
      if(le32(bytes+8)&8) {
         if(le32(bytes+20)<pitch)return false;
         pitch=le32(bytes+20);
      }
      if(pitch>size-offset || height>(size-offset)/pitch)return false;
   } else if(blocksWide*blocksHigh>(size-offset)/blockBytes)return false;
   try {
      output.width=width;output.height=height;output.rgba.resize(size_t(width)*height*4);
      if(block) {
         for(size_t y=0;y<blocksHigh;++y)for(size_t x=0;x<blocksWide;++x) {
            // bcdec reads packed words; copy into aligned local storage.
            alignas(8) uint8_t encoded[16]{};alignas(8) uint8_t decoded[64]{};
            std::memcpy(encoded,bytes+offset+(y*blocksWide+x)*blockBytes,blockBytes);
            switch(block) {
            case 1:bcdec_bc1(encoded,decoded,16);break;
            case 2:bcdec_bc2(encoded,decoded,16);break;
            case 3:bcdec_bc3(encoded,decoded,16);break;
            case 4:bcdec_bc4(encoded,decoded,4,0);break;
            case 5:bcdec_bc5(encoded,decoded,8,0);break;
            case 7:bcdec_bc7(encoded,decoded,16);break;
            }
            for(unsigned by=0;by<4 && y*4+by<height;++by)for(unsigned bx=0;bx<4 && x*4+bx<width;++bx) {
               auto *destination=output.rgba.data()+((y*4+by)*width+x*4+bx)*4;
               const unsigned pixel=by*4+bx;
               if(block==4){destination[0]=destination[1]=destination[2]=decoded[pixel];destination[3]=255;}
               else if(block==5){destination[0]=decoded[pixel*2];destination[1]=decoded[pixel*2+1];destination[2]=0;destination[3]=255;}
               else std::memcpy(destination,decoded+pixel*4,4);
            }
         }
      } else for(uint32_t y=0;y<height;++y)for(uint32_t x=0;x<width;++x) {
         uint32_t pixel=0;for(unsigned byte=0;byte<bits/8;++byte)pixel|=uint32_t(bytes[offset+y*pitch+x*(bits/8)+byte])<<(8*byte);
         auto *destination=output.rgba.data()+(size_t(y)*width+x)*4;
         for(unsigned c=0;c<4;++c)destination[c]=masked(pixel,masks[c],c==3?255:0);
         if((flags&0x20000) && !masks[1] && !masks[2])destination[1]=destination[2]=destination[0];
      }
      return true;
   }catch(...){output={};return false;}
}
