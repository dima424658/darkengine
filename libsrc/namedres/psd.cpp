#include <psd.h>

#include <allocapi.h>
#include <bitmap.h>

#include <cstdlib>
#include <cstring>
#include <limits>

namespace
{
uint16 ReadBE16(const uint8* value)
{
   return static_cast<uint16>((value[0] << 8) | value[1]);
}

uint32 ReadBE32(const uint8* value)
{
   return (static_cast<uint32>(value[0]) << 24) |
          (static_cast<uint32>(value[1]) << 16) |
          (static_cast<uint32>(value[2]) << 8) | value[3];
}

bool ReadExact(IStoreStream* pStream, void* data, long size)
{
   return size >= 0 && pStream->Read(size, static_cast<char*>(data)) == size;
}

bool SkipSection(IStoreStream* pStream, long streamSize)
{
   uint8 encodedLength[4];
   if (!ReadExact(pStream, encodedLength, sizeof(encodedLength)))
      return false;
   const uint32 length = ReadBE32(encodedLength);
   const long position = pStream->GetPos();
   if (position < 0 || position > streamSize ||
       length > static_cast<uint32>(streamSize - position))
      return false;
   pStream->SetPos(position + static_cast<long>(length));
   return true;
}

bool DecodePackBitsRow(const uint8* source, size_t sourceSize,
                       uint8* destination, size_t destinationSize)
{
   size_t in = 0;
   size_t out = 0;
   while (in < sourceSize && out < destinationSize)
   {
      const int8 control = static_cast<int8>(source[in++]);
      if (control >= 0)
      {
         const size_t count = static_cast<size_t>(control) + 1;
         if (count > sourceSize - in || count > destinationSize - out)
            return false;
         std::memcpy(destination + out, source + in, count);
         in += count;
         out += count;
      }
      else if (control != -128)
      {
         const size_t count = static_cast<size_t>(1 - control);
         if (in >= sourceSize || count > destinationSize - out)
            return false;
         std::memset(destination + out, source[in++], count);
         out += count;
      }
   }
   return out == destinationSize;
}
}

grs_bitmap* ResPsdReadImage(IStoreStream* pStream,
                            IResMemOverride* pResMem)
{
   uint8 header[26];
   uint8 compressionBytes[2];
   uint8* planes = nullptr;
   uint8* encoded = nullptr;
   uint16* rowLengths = nullptr;
   grs_bitmap* bitmap = nullptr;
   size_t allocationSize = 0;

   if (!pStream || !pResMem)
      return nullptr;
   pStream->SetPos(0);
   if (!ReadExact(pStream, header, sizeof(header)) ||
       std::memcmp(header, "8BPS", 4) != 0 || ReadBE16(header + 4) != 1)
      return nullptr;

   const uint16 channels = ReadBE16(header + 12);
   const uint32 height = ReadBE32(header + 14);
   const uint32 width = ReadBE32(header + 18);
   const uint16 depth = ReadBE16(header + 22);
   const uint16 colorMode = ReadBE16(header + 24);
   const long streamSize = pStream->GetSize();
   if (channels < 3 || channels > 4 || !width || !height || depth != 8 ||
       colorMode != 3 || width > 32767 || height > 32767 ||
       width > std::numeric_limits<size_t>::max() / height)
      return nullptr;

   if (!SkipSection(pStream, streamSize) ||
       !SkipSection(pStream, streamSize) ||
       !SkipSection(pStream, streamSize) ||
       !ReadExact(pStream, compressionBytes, sizeof(compressionBytes)))
      return nullptr;

   const uint16 compression = ReadBE16(compressionBytes);
   const size_t pixelCount = static_cast<size_t>(width) * height;
   if (pixelCount > std::numeric_limits<size_t>::max() / channels)
      return nullptr;
   planes = static_cast<uint8*>(std::malloc(pixelCount * channels));
   if (!planes)
      return nullptr;

   if (compression == 0)
   {
      if (pixelCount * channels > static_cast<size_t>(std::numeric_limits<long>::max()) ||
          !ReadExact(pStream, planes, static_cast<long>(pixelCount * channels)))
         goto cleanup;
   }
   else if (compression == 1)
   {
      const size_t rowCount = static_cast<size_t>(channels) * height;
      if (rowCount > std::numeric_limits<size_t>::max() / sizeof(uint16))
         goto cleanup;
      rowLengths = static_cast<uint16*>(std::malloc(rowCount * sizeof(uint16)));
      if (!rowLengths)
         goto cleanup;
      for (size_t row = 0; row < rowCount; ++row)
      {
         uint8 length[2];
         if (!ReadExact(pStream, length, sizeof(length)))
            goto cleanup;
         rowLengths[row] = ReadBE16(length);
      }
      for (size_t row = 0; row < rowCount; ++row)
      {
         const size_t encodedSize = rowLengths[row];
         uint8* resized = static_cast<uint8*>(std::realloc(encoded, encodedSize));
         if (encodedSize && !resized)
            goto cleanup;
         encoded = resized;
         if ((encodedSize && !ReadExact(pStream, encoded, static_cast<long>(encodedSize))) ||
             !DecodePackBitsRow(encoded, encodedSize,
                                planes + row * width, width))
            goto cleanup;
      }
   }
   else
      goto cleanup;

   if (pixelCount >
       (std::numeric_limits<size_t>::max() - sizeof(grs_bitmap)) / sizeof(uint16))
      goto cleanup;
   allocationSize = sizeof(grs_bitmap) + pixelCount * sizeof(uint16);
   if (allocationSize > std::numeric_limits<ulong>::max())
      goto cleanup;
   LGALLOC_PUSH_CREDIT();
   bitmap = reinterpret_cast<grs_bitmap*>(
      pResMem->ResMalloc(static_cast<ulong>(allocationSize)));
   LGALLOC_POP_CREDIT();
   if (!bitmap)
      goto cleanup;

   {
      auto* output = reinterpret_cast<uint16*>(&bitmap[1]);
      const uint8* red = planes;
      const uint8* green = planes + pixelCount;
      const uint8* blue = planes + pixelCount * 2;
      const uint8* alpha = channels == 4 ? planes + pixelCount * 3 : nullptr;
      bool hasAlpha = false;
      size_t opaquePixels = 0;
      size_t partialAlphaPixels = 0;
      if (alpha)
      {
         for (size_t i = 0; i < pixelCount; ++i)
         {
            if (alpha[i] == 255)
               ++opaquePixels;
            else
            {
               hasAlpha = true;
               if (alpha[i] != 0)
                  ++partialAlphaPixels;
            }
         }
      }
      const bool binaryAlpha = hasAlpha && opaquePixels * 5 >= pixelCount * 4 &&
                               partialAlphaPixels * 10 <= pixelCount;
      for (size_t i = 0; i < pixelCount; ++i)
      {
         if (binaryAlpha)
            output[i] = alpha[i] < 128 ? 0 :
               static_cast<uint16>(0x8000 | ((red[i] >> 3) << 10) |
                                   ((green[i] >> 3) << 5) | (blue[i] >> 3));
         else if (hasAlpha)
            output[i] = alpha[i] < 8 ? 0 :
               static_cast<uint16>(((alpha[i] >> 4) << 12) |
                                   ((red[i] >> 4) << 8) |
                                   ((green[i] >> 4) << 4) | (blue[i] >> 4));
         else
            output[i] = static_cast<uint16>(((red[i] >> 3) << 11) |
                                            ((green[i] >> 2) << 5) |
                                            (blue[i] >> 3));
      }
      gr_init_bitmap(bitmap, reinterpret_cast<uint8*>(output), BMT_FLAT16,
                     binaryAlpha ? BMF_RGB_1555 :
                     hasAlpha ? BMF_RGB_4444 : BMF_RGB_565,
                     static_cast<int>(width), static_cast<int>(height));
   }

cleanup:
   std::free(rowLengths);
   std::free(encoded);
   std::free(planes);
   return bitmap;
}
