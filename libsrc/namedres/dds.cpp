#include <dds.h>

#include <allocapi.h>
#include <bitmap.h>
#include <resbastm.h>

#include <cstring>
#include <limits>
#include <vector>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

namespace
{
constexpr uint32 MakeFourCc(char a, char b, char c, char d)
{
   return static_cast<uint32>(static_cast<uint8>(a)) |
      (static_cast<uint32>(static_cast<uint8>(b)) << 8) |
      (static_cast<uint32>(static_cast<uint8>(c)) << 16) |
      (static_cast<uint32>(static_cast<uint8>(d)) << 24);
}

uint16 ReadLe16(const uint8* source)
{
   return static_cast<uint16>(source[0] |
      (static_cast<uint16>(source[1]) << 8));
}

uint32 ReadLe32(const uint8* source)
{
   return static_cast<uint32>(source[0]) |
      (static_cast<uint32>(source[1]) << 8) |
      (static_cast<uint32>(source[2]) << 16) |
      (static_cast<uint32>(source[3]) << 24);
}

struct RgbaPixel
{
   uint8 r;
   uint8 g;
   uint8 b;
   uint8 a;
};

RgbaPixel Decode565(uint16 value)
{
   RgbaPixel pixel{};
   pixel.r = static_cast<uint8>(((value >> 11) & 31) * 255 / 31);
   pixel.g = static_cast<uint8>(((value >> 5) & 63) * 255 / 63);
   pixel.b = static_cast<uint8>((value & 31) * 255 / 31);
   pixel.a = 255;
   return pixel;
}

uint8 ExpandMasked(uint32 value, uint32 mask, uint8 defaultValue)
{
   if (!mask)
      return defaultValue;

   unsigned shift = 0;
   while (((mask >> shift) & 1) == 0)
      ++shift;
   const uint32 normalizedMask = mask >> shift;
   const uint32 component = (value & mask) >> shift;
   return static_cast<uint8>((static_cast<uint64_t>(component) * 255u +
      normalizedMask / 2u) / normalizedMask);
}

void DecodeColorBlock(const uint8* block, bool dxt1,
                      RgbaPixel* output)
{
   const uint16 color0 = ReadLe16(block);
   const uint16 color1 = ReadLe16(block + 2);
   RgbaPixel colors[4] = { Decode565(color0), Decode565(color1), {}, {} };

   if (!dxt1 || color0 > color1)
   {
      colors[2] = {
         static_cast<uint8>((2 * colors[0].r + colors[1].r) / 3),
         static_cast<uint8>((2 * colors[0].g + colors[1].g) / 3),
         static_cast<uint8>((2 * colors[0].b + colors[1].b) / 3), 255 };
      colors[3] = {
         static_cast<uint8>((colors[0].r + 2 * colors[1].r) / 3),
         static_cast<uint8>((colors[0].g + 2 * colors[1].g) / 3),
         static_cast<uint8>((colors[0].b + 2 * colors[1].b) / 3), 255 };
   }
   else
   {
      colors[2] = {
         static_cast<uint8>((colors[0].r + colors[1].r) / 2),
         static_cast<uint8>((colors[0].g + colors[1].g) / 2),
         static_cast<uint8>((colors[0].b + colors[1].b) / 2), 255 };
      colors[3] = { 0, 0, 0, 0 };
   }

   const uint32 indices = ReadLe32(block + 4);
   for (unsigned i = 0; i < 16; ++i)
      output[i] = colors[(indices >> (i * 2)) & 3];
}

void DecodeDxt3Alpha(const uint8* block, RgbaPixel* output)
{
   for (unsigned i = 0; i < 16; ++i)
   {
      const uint8 nibble = static_cast<uint8>(
         (block[i / 2] >> ((i & 1) * 4)) & 15);
      output[i].a = static_cast<uint8>(nibble * 17);
   }
}

void DecodeDxt5Alpha(const uint8* block, RgbaPixel* output)
{
   uint8 alpha[8] = { block[0], block[1] };
   if (alpha[0] > alpha[1])
   {
      for (unsigned i = 1; i <= 6; ++i)
         alpha[i + 1] = static_cast<uint8>(
            ((7 - i) * alpha[0] + i * alpha[1]) / 7);
   }
   else
   {
      for (unsigned i = 1; i <= 4; ++i)
         alpha[i + 1] = static_cast<uint8>(
            ((5 - i) * alpha[0] + i * alpha[1]) / 5);
      alpha[6] = 0;
      alpha[7] = 255;
   }

   uint64_t indices = 0;
   for (unsigned i = 0; i < 6; ++i)
      indices |= static_cast<uint64_t>(block[i + 2]) << (i * 8);
   for (unsigned i = 0; i < 16; ++i)
      output[i].a = alpha[(indices >> (i * 3)) & 7];
}

bool DecodeBlocks(const uint8* source, size_t sourceSize, uint32 fourCc,
                  unsigned width, unsigned height,
                  std::vector<RgbaPixel>* pixels)
{
   const bool dxt1 = fourCc == MakeFourCc('D', 'X', 'T', '1');
   const bool dxt3 = fourCc == MakeFourCc('D', 'X', 'T', '3');
   const bool dxt5 = fourCc == MakeFourCc('D', 'X', 'T', '5');
   if (!dxt1 && !dxt3 && !dxt5)
      return false;

   const size_t blockBytes = dxt1 ? 8 : 16;
   const size_t blocksWide = (static_cast<size_t>(width) + 3) / 4;
   const size_t blocksHigh = (static_cast<size_t>(height) + 3) / 4;
   if (blocksWide > std::numeric_limits<size_t>::max() / blocksHigh ||
       blocksWide * blocksHigh > sourceSize / blockBytes)
      return false;

   for (size_t blockY = 0; blockY < blocksHigh; ++blockY)
   {
      for (size_t blockX = 0; blockX < blocksWide; ++blockX)
      {
         const uint8* block = source +
            (blockY * blocksWide + blockX) * blockBytes;
         RgbaPixel decoded[16]{};
         DecodeColorBlock(block + (dxt1 ? 0 : 8), dxt1, decoded);
         if (dxt3)
            DecodeDxt3Alpha(block, decoded);
         else if (dxt5)
            DecodeDxt5Alpha(block, decoded);

         for (unsigned y = 0; y < 4; ++y)
         {
            const size_t destinationY = blockY * 4 + y;
            if (destinationY >= height)
               break;
            for (unsigned x = 0; x < 4; ++x)
            {
               const size_t destinationX = blockX * 4 + x;
               if (destinationX < width)
                  (*pixels)[destinationY * width + destinationX] =
                     decoded[y * 4 + x];
            }
         }
      }
   }
   return true;
}

bool DecodeRgb(const uint8* source, size_t sourceSize, unsigned width,
               unsigned height, unsigned bitsPerPixel, uint32 redMask,
               uint32 greenMask, uint32 blueMask, uint32 alphaMask,
               std::vector<RgbaPixel>* pixels)
{
   if (bitsPerPixel != 24 && bitsPerPixel != 32)
      return false;
   const size_t bytesPerPixel = bitsPerPixel / 8;
   const size_t rowBytes = (static_cast<size_t>(width) * bytesPerPixel + 3) &
                           ~static_cast<size_t>(3);
   if (rowBytes > sourceSize || height > sourceSize / rowBytes)
      return false;

   for (unsigned y = 0; y < height; ++y)
   {
      const uint8* row = source + static_cast<size_t>(y) * rowBytes;
      for (unsigned x = 0; x < width; ++x)
      {
         const uint8* encoded = row + static_cast<size_t>(x) * bytesPerPixel;
         uint32 value = static_cast<uint32>(encoded[0]) |
            (static_cast<uint32>(encoded[1]) << 8) |
            (static_cast<uint32>(encoded[2]) << 16);
         if (bytesPerPixel == 4)
            value |= static_cast<uint32>(encoded[3]) << 24;
         (*pixels)[static_cast<size_t>(y) * width + x] = {
            ExpandMasked(value, redMask, 0),
            ExpandMasked(value, greenMask, 0),
            ExpandMasked(value, blueMask, 0),
            ExpandMasked(value, alphaMask, 255) };
      }
   }
   return true;
}

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

grs_bitmap* ResDdsReadImage(IStoreStream* pStream,
                            IResMemOverride* pResMem)
{
   if (!pStream || !pResMem || pStream->GetSize() < 128)
      return nullptr;

   uint8 header[128]{};
   pStream->SetPos(0);
   if (pStream->Read(sizeof(header), reinterpret_cast<char*>(header)) !=
          sizeof(header) ||
       std::memcmp(header, "DDS ", 4) != 0 || ReadLe32(header + 4) != 124 ||
       ReadLe32(header + 76) != 32)
      return nullptr;

   const uint32 height = ReadLe32(header + 12);
   const uint32 width = ReadLe32(header + 16);
   if (!width || !height ||
       width > static_cast<uint32>(std::numeric_limits<int>::max()) ||
       height > static_cast<uint32>(std::numeric_limits<int>::max()) ||
       static_cast<size_t>(width) >
          std::numeric_limits<size_t>::max() / height)
      return nullptr;

   const long streamSize = pStream->GetSize();
   const size_t encodedSize = static_cast<size_t>(streamSize - 128);
   std::vector<uint8> encoded(encodedSize);
   if (pStream->Read(static_cast<long>(encodedSize),
                     reinterpret_cast<char*>(encoded.data())) !=
       static_cast<long>(encodedSize))
      return nullptr;

   const size_t pixelCount = static_cast<size_t>(width) * height;
   std::vector<RgbaPixel> pixels(pixelCount);
   const uint32 pixelFlags = ReadLe32(header + 80);
   bool decoded = false;
   if (pixelFlags & 0x4)
   {
      const uint32 fourCc = ReadLe32(header + 84);
      decoded = DecodeBlocks(encoded.data(), encoded.size(), fourCc,
                             width, height, &pixels);
      if (!decoded)
         Warning(("Unsupported DDS compression %.4s\n", header + 84));
   }
   else if (pixelFlags & 0x40)
   {
      decoded = DecodeRgb(encoded.data(), encoded.size(), width, height,
         ReadLe32(header + 88), ReadLe32(header + 92),
         ReadLe32(header + 96), ReadLe32(header + 100),
         ReadLe32(header + 104), &pixels);
   }
   if (!decoded)
      return nullptr;

   bool hasAlpha = false;
   bool hasPartialAlpha = false;
   for (const auto& pixel : pixels)
   {
      if (pixel.a != 255)
      {
         hasAlpha = true;
         if (pixel.a != 0)
            hasPartialAlpha = true;
      }
   }

   if (pixelCount >
       (std::numeric_limits<size_t>::max() - sizeof(grs_bitmap)) /
          sizeof(uint16))
      return nullptr;
   const size_t allocationSize = sizeof(grs_bitmap) +
                                 pixelCount * sizeof(uint16);
   if (allocationSize > std::numeric_limits<ulong>::max())
      return nullptr;

   LGALLOC_PUSH_CREDIT();
   auto* bitmap = reinterpret_cast<grs_bitmap*>(
      pResMem->ResMalloc(static_cast<ulong>(allocationSize)));
   LGALLOC_POP_CREDIT();
   if (!bitmap)
      return nullptr;

   auto* output = reinterpret_cast<uint16*>(&bitmap[1]);
   for (size_t i = 0; i < pixelCount; ++i)
      output[i] = hasPartialAlpha ? Pack4444(pixels[i]) :
                  hasAlpha ? Pack1555(pixels[i]) : Pack565(pixels[i]);

   gr_init_bitmap(bitmap, reinterpret_cast<uint8*>(output), BMT_FLAT16,
      hasPartialAlpha ? BMF_RGB_4444 :
      hasAlpha ? BMF_RGB_1555 : BMF_RGB_565,
      static_cast<int>(width), static_cast<int>(height));
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
