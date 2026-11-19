#include "../../libsrc/namedres/dds_decode.hpp"
#include <array>
#include <cstdio>
#include <stdexcept>
#include <cstring>
static void check(bool ok,const char *reason){if(!ok)throw std::runtime_error(reason);}
static void put(std::vector<uint8_t> &b,size_t offset,uint32_t value){for(unsigned i=0;i<4;++i)b[offset+i]=uint8_t(value>>(8*i));}
static std::vector<uint8_t> header(unsigned width,unsigned height,unsigned format,unsigned payload) {
   std::vector<uint8_t> bytes(148+payload);std::memcpy(bytes.data(),"DDS ",4);
   put(bytes,4,124);put(bytes,12,height);put(bytes,16,width);put(bytes,76,32);put(bytes,80,4);
   put(bytes,84,0x30315844);put(bytes,128,format);put(bytes,132,3);put(bytes,140,1);return bytes;
}
int main() {
 try {
   ResImagePixels image;
   auto bytes=header(4,4,71,8);bytes[148]=0;bytes[149]=0xf8;
   check(ResDecodeDds(bytes.data(),bytes.size(),image),"DX10 BC1 decode");
   check(image.rgba[0]==255 && image.rgba[1]==0 && image.rgba[3]==255,"BC1 red endpoint");
   auto legacy=bytes;legacy.erase(legacy.begin()+128,legacy.begin()+148);put(legacy,84,0x31545844);
   check(ResDecodeDds(legacy.data(),legacy.size(),image) && image.rgba[0]==255,"legacy DXT1 decode");
   bytes=header(4,4,74,16);for(unsigned i=0;i<8;++i)bytes[148+i]=0x88;bytes[157]=0xf8;
   check(ResDecodeDds(bytes.data(),bytes.size(),image) && image.rgba[0]==255 && image.rgba[3]==136,"BC2 explicit alpha");
   bytes=header(4,4,77,16);bytes[148]=17;bytes[157]=0xf8;
   check(ResDecodeDds(bytes.data(),bytes.size(),image) && image.rgba[0]==255 && image.rgba[3]==17,"BC3 interpolated alpha endpoint");
   bytes=header(3,2,80,8);bytes[148]=37;
   check(ResDecodeDds(bytes.data(),bytes.size(),image) && image.rgba.size()==24,"partial BC4 block clips to extent");
   for(size_t i=0;i<image.rgba.size();i+=4)check(image.rgba[i]==37 && image.rgba[i+1]==37 && image.rgba[i+3]==255,"BC4 scalar channels preserved");
   bytes=header(4,4,83,16);bytes[148]=128;bytes[156]=192;
   check(ResDecodeDds(bytes.data(),bytes.size(),image),"BC5 decode");
   check(image.rgba[0]==128 && image.rgba[1]==192 && image.rgba[2]==0 && image.rgba[3]==255,"BC5 normal XY preserved");
   bytes=header(4,4,98,16);
   unsigned bit=0;
   auto write=[&](unsigned value,unsigned count){for(unsigned i=0;i<count;++i,++bit)bytes[148+bit/8]|=((value>>i)&1)<<(bit%8);};
   write(64,7);for(unsigned channel=0;channel<4;++channel){write(channel==0 || channel==3?127:0,7);write(channel==0 || channel==3?127:0,7);}
   write(1,1);write(1,1);write(0,3);for(unsigned i=1;i<16;++i)write(0,4);
   check(bit==128 && ResDecodeDds(bytes.data(),bytes.size(),image),"BC7 mode 6 decode");
   check(image.rgba[0]==255 && image.rgba[1]==1 && image.rgba[2]==1 && image.rgba[3]==255,"BC7 known constant endpoint");
   bytes=header(1,1,28,4);bytes[148]=13;bytes[149]=127;bytes[150]=243;bytes[151]=91;
   check(ResDecodeDds(bytes.data(),bytes.size(),image) && image.rgba==std::vector<uint8_t>({13,127,243,91}),"RGBA8 exact channels and alpha");
   bytes=header(1,2,28,16);put(bytes,8,8);put(bytes,20,8);bytes[148]=17;bytes[151]=255;bytes[156]=33;bytes[159]=255;
   check(ResDecodeDds(bytes.data(),bytes.size(),image) && image.rgba[0]==17 && image.rgba[4]==33,"padded DDS row pitch");
   bytes=header(4,4,83,15);check(!ResDecodeDds(bytes.data(),bytes.size(),image),"truncated compressed block rejected");
   bytes=header(4,4,83,16);put(bytes,140,2);check(!ResDecodeDds(bytes.data(),bytes.size(),image),"texture arrays rejected");
   put(bytes,140,1);put(bytes,136,4);check(!ResDecodeDds(bytes.data(),bytes.size(),image),"cubemap rejected");
   put(bytes,136,0);put(bytes,128,95);check(!ResDecodeDds(bytes.data(),bytes.size(),image),"HDR format rejected explicitly");
   put(bytes,128,83);put(bytes,16,0xffffffffu);check(!ResDecodeDds(bytes.data(),bytes.size(),image),"oversized dimensions rejected before allocation");
   std::puts("PASS DDS legacy/DX10, BC1/4/5/7, exact channels, pitch, clipping and malformed input");return 0;
 }catch(const std::exception &error){std::fprintf(stderr,"DDS: %s\n",error.what());return 1;}
}
