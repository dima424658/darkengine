#include <dds.h>
#include "dds_decode.hpp"
#include <allocapi.h>
#include <bitmap.h>
#include <resbastm.h>
#include <mprintf.h>
#include <cstring>
#include <limits>
#include <vector>
#ifndef NO_DB_MEM
#include <memall.h>
#include <dbmem.h>
#endif
namespace {
struct RgbaPixel {uint8 r,g,b,a;};
uint16 Pack565(const RgbaPixel& pixel)
{
   return static_cast<uint16>(((pixel.r >> 3) << 11) |
                              ((pixel.g >> 2) << 5) |
                              (pixel.b >> 3));
}

uint16 Pack1555(const RgbaPixel& pixel)
{
   if (pixel.a < 128)
      return 0;
   return static_cast<uint16>(0x8000 | ((pixel.r >> 3) << 10) |
                              ((pixel.g >> 3) << 5) | (pixel.b >> 3));
}

uint16 Pack4444(const RgbaPixel& pixel)
{
   if (pixel.a < 8)
      return 0;
   return static_cast<uint16>(((pixel.a >> 4) << 12) |
                              ((pixel.r >> 4) << 8) |
                              ((pixel.g >> 4) << 4) | (pixel.b >> 4));
}
}

grs_bitmap* ResDdsReadImage(IStoreStream* pStream,IResMemOverride* pResMem) {
   if(!pStream || !pResMem)return nullptr;
   const long size=pStream->GetSize();
   if(size<128 || size>512*1024*1024)return nullptr;
   std::vector<uint8> bytes(size);pStream->SetPos(0);
   if(pStream->Read(size,reinterpret_cast<char *>(bytes.data()))!=size)return nullptr;
   ResImagePixels image;
   if(!ResDecodeDds(bytes.data(),bytes.size(),image)) {
      mprintf("DDS: unsupported format, invalid dimensions or truncated image\n");return nullptr;
   }
   bool hasAlpha=false,partial=false;
   const size_t count=size_t(image.width)*image.height;
   for(size_t i=0;i<count;++i){const auto a=image.rgba[i*4+3];hasAlpha|=a!=255;partial|=a!=0 && a!=255;}
   const size_t allocation=sizeof(grs_bitmap)+count*2;
   if(allocation>std::numeric_limits<ulong>::max())return nullptr;
   LGALLOC_PUSH_CREDIT();
   auto *bitmap=static_cast<grs_bitmap *>(pResMem->ResMalloc(static_cast<ulong>(allocation)));
   LGALLOC_POP_CREDIT();
   if(!bitmap)return nullptr;
   auto *output=reinterpret_cast<uint16 *>(bitmap+1);
   for(size_t i=0;i<count;++i) {
      RgbaPixel pixel;std::memcpy(&pixel,image.rgba.data()+i*4,4);
      output[i]=partial?Pack4444(pixel):hasAlpha?Pack1555(pixel):Pack565(pixel);
   }
   gr_init_bitmap(bitmap,reinterpret_cast<uint8 *>(output),BMT_FLAT16,
      partial?BMF_RGB_4444:hasAlpha?BMF_RGB_1555:BMF_RGB_565,image.width,image.height);
   ResImageRememberPixels(bitmap,image.width,image.height,image.rgba.data());
   return bitmap;
}
uint8* ResDdsReadPalette(IStoreStream*, IResMemOverride* pResMem)
{
   if (!pResMem)
      return nullptr;

   LGALLOC_PUSH_CREDIT();
   auto* palette = static_cast<uint8*>(pResMem->ResMalloc(3 * 256));
   LGALLOC_POP_CREDIT();
   if (!palette)
      return nullptr;

   for (unsigned i = 0; i < 256; ++i)
   {
      palette[i * 3] = static_cast<uint8>(((i >> 5) & 7) * 255 / 7);
      palette[i * 3 + 1] = static_cast<uint8>(((i >> 2) & 7) * 255 / 7);
      palette[i * 3 + 2] = static_cast<uint8>((i & 3) * 255 / 3);
   }
   return palette;
}
