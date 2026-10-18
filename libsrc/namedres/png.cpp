#include <png.h>

#include <allocapi.h>
#include <bitmap.h>
#include <resbastm.h>

#ifdef _WIN32
#pragma pack(push, 8)
#include <objbase.h>
#include <wincodec.h>
#pragma pack(pop)
#else
#pragma pack(push, 8)
#include <libpng16/png.h>
#pragma pack(pop)
#endif

#include <cstdlib>
#include <limits>

#ifdef _WIN32
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "windowscodecs.lib")
#endif

namespace
{
#ifdef _WIN32
template <class T>
void ReleaseInterface(T*& value)
{
   if (value)
   {
      value->Release();
      value = nullptr;
   }
}
#endif

uint16 Pack565(const uint8* pixel)
{
   return static_cast<uint16>(((pixel[0] >> 3) << 11) |
                              ((pixel[1] >> 2) << 5) |
                              (pixel[2] >> 3));
}

uint16 Pack4444(const uint8* pixel)
{
   if (pixel[3] < 8)
      return 0;
   return static_cast<uint16>(((pixel[3] >> 4) << 12) |
                              ((pixel[0] >> 4) << 8) |
                              ((pixel[1] >> 4) << 4) |
                              (pixel[2] >> 4));
}

uint16 Pack1555(const uint8* pixel)
{
   if (pixel[3] < 128)
      return 0;
   return static_cast<uint16>(0x8000 |
                              ((pixel[0] >> 3) << 10) |
                              ((pixel[1] >> 3) << 5) |
                              (pixel[2] >> 3));
}
}

grs_bitmap* ResPngReadImage(IStoreStream* pStream,
                            IResMemOverride* pResMem)
{
   if (!pStream || !pResMem)
      return nullptr;

   const long streamSize = pStream->GetSize();
   if (streamSize <= 0)
      return nullptr;

#ifdef _WIN32
   const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
   const bool uninitialize = comResult == S_OK || comResult == S_FALSE;
   if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE)
      return nullptr;

   HGLOBAL storage = GlobalAlloc(GMEM_MOVEABLE, static_cast<SIZE_T>(streamSize));
   IStream* memoryStream = nullptr;
   IWICImagingFactory* factory = nullptr;
   IWICBitmapDecoder* decoder = nullptr;
   IWICBitmapFrameDecode* frame = nullptr;
   IWICFormatConverter* converter = nullptr;
   grs_bitmap* bitmap = nullptr;
   uint8* rgba = nullptr;
   void* bytes = nullptr;
   UINT width = 0;
   UINT height = 0;
   UINT stride = 0;
   UINT rgbaSize = 0;
   size_t pixelCount = 0;
   size_t allocationSize = 0;
   bool hasAlpha = false;
   size_t opaquePixels = 0;
   size_t partialAlphaPixels = 0;
   bool useBinaryAlpha = false;
   uint16* output = nullptr;

   bytes = storage ? GlobalLock(storage) : nullptr;
   pStream->SetPos(0);
   if (!bytes || pStream->Read(streamSize, static_cast<char*>(bytes)) != streamSize)
      goto cleanup;
   GlobalUnlock(storage);
   bytes = nullptr;

   if (FAILED(CreateStreamOnHGlobal(storage, TRUE, &memoryStream)))
      goto cleanup;
   storage = nullptr; // owned by memoryStream
   if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                               CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))) ||
       FAILED(factory->CreateDecoderFromStream(memoryStream, nullptr,
               WICDecodeMetadataCacheOnLoad, &decoder)) ||
       FAILED(decoder->GetFrame(0, &frame)) ||
       FAILED(factory->CreateFormatConverter(&converter)) ||
       FAILED(converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
               WICBitmapDitherTypeNone, nullptr, 0.0,
               WICBitmapPaletteTypeCustom)))
      goto cleanup;

   if (FAILED(converter->GetSize(&width, &height)) || !width || !height ||
       width > std::numeric_limits<UINT>::max() / 4 ||
       height > std::numeric_limits<UINT>::max() / (width * 4))
      goto cleanup;

   stride = width * 4;
   rgbaSize = stride * height;
   rgba = static_cast<uint8*>(std::malloc(rgbaSize));
   if (!rgba || FAILED(converter->CopyPixels(nullptr, stride, rgbaSize, rgba)))
      goto cleanup;

   pixelCount = static_cast<size_t>(width) * height;
   for (size_t i = 0; i < pixelCount; ++i)
   {
      const uint8 alpha = rgba[i * 4 + 3];
      if (alpha == 255)
         ++opaquePixels;
      else
      {
         hasAlpha = true;
         if (alpha != 0)
            ++partialAlphaPixels;
      }
   }

   // Menu animation frames are overwhelmingly opaque with alpha used only
   // for their antialiased outline.  Encoding the entire frame as ARGB4444
   // needlessly reduces every colour channel to four bits and visibly
   // posterizes assets such as The Black Parade's window and dial.  A1RGB5
   // retains five colour bits and a binary edge while genuinely translucent
   // PNGs continue to use ARGB4444.
   useBinaryAlpha = hasAlpha && opaquePixels * 5 >= pixelCount * 4 &&
                    partialAlphaPixels * 10 <= pixelCount;

   if (pixelCount >
       (std::numeric_limits<size_t>::max() - sizeof(grs_bitmap)) /
          sizeof(uint16))
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

   output = reinterpret_cast<uint16*>(&bitmap[1]);
   for (size_t i = 0; i < pixelCount; ++i)
      output[i] = useBinaryAlpha ? Pack1555(&rgba[i * 4]) :
                  hasAlpha ? Pack4444(&rgba[i * 4]) :
                             Pack565(&rgba[i * 4]);

   gr_init_bitmap(bitmap, reinterpret_cast<uint8*>(output), BMT_FLAT16,
                  useBinaryAlpha ? BMF_RGB_1555 :
                  hasAlpha ? BMF_RGB_4444 : BMF_RGB_565,
                  static_cast<int>(width), static_cast<int>(height));
cleanup:
   if (bytes)
      GlobalUnlock(storage);
   if (storage)
      GlobalFree(storage);
   std::free(rgba);
   ReleaseInterface(converter);
   ReleaseInterface(frame);
   ReleaseInterface(decoder);
   ReleaseInterface(factory);
   ReleaseInterface(memoryStream);
   if (uninitialize)
      CoUninitialize();
   return bitmap;
#else
   auto* encoded = static_cast<uint8*>(std::malloc(static_cast<size_t>(streamSize)));
   if (!encoded)
      return nullptr;

   pStream->SetPos(0);
   if (pStream->Read(streamSize, reinterpret_cast<char*>(encoded)) != streamSize)
   {
      std::free(encoded);
      return nullptr;
   }

   png_image image{};
   image.version = PNG_IMAGE_VERSION;
   if (!png_image_begin_read_from_memory(&image, encoded,
                                         static_cast<size_t>(streamSize)))
   {
      std::free(encoded);
      return nullptr;
   }

   image.format = PNG_FORMAT_RGBA;
   const size_t width = image.width;
   const size_t height = image.height;
   if (!width || !height ||
       width > static_cast<size_t>(std::numeric_limits<int>::max()) ||
       height > static_cast<size_t>(std::numeric_limits<int>::max()) ||
       width > std::numeric_limits<size_t>::max() / height ||
       width * height > std::numeric_limits<size_t>::max() / 4)
   {
      png_image_free(&image);
      std::free(encoded);
      return nullptr;
   }

   const size_t pixelCount = width * height;
   auto* rgba = static_cast<uint8*>(std::malloc(pixelCount * 4));
   const bool decoded = rgba && png_image_finish_read(&image, nullptr, rgba,
                                                       0, nullptr) != 0;
   png_image_free(&image);
   std::free(encoded);
   if (!decoded)
   {
      std::free(rgba);
      return nullptr;
   }

   bool hasAlpha = false;
   size_t opaquePixels = 0;
   size_t partialAlphaPixels = 0;
   for (size_t i = 0; i < pixelCount; ++i)
   {
      const uint8 alpha = rgba[i * 4 + 3];
      if (alpha == 255)
         ++opaquePixels;
      else
      {
         hasAlpha = true;
         if (alpha != 0)
            ++partialAlphaPixels;
      }
   }

   const bool useBinaryAlpha = hasAlpha &&
      opaquePixels * 5 >= pixelCount * 4 &&
      partialAlphaPixels * 10 <= pixelCount;
   if (pixelCount >
       (std::numeric_limits<size_t>::max() - sizeof(grs_bitmap)) /
          sizeof(uint16))
   {
      std::free(rgba);
      return nullptr;
   }

   const size_t allocationSize = sizeof(grs_bitmap) +
                                 pixelCount * sizeof(uint16);
   if (allocationSize > std::numeric_limits<ulong>::max())
   {
      std::free(rgba);
      return nullptr;
   }

   LGALLOC_PUSH_CREDIT();
   auto* bitmap = reinterpret_cast<grs_bitmap*>(
      pResMem->ResMalloc(static_cast<ulong>(allocationSize)));
   LGALLOC_POP_CREDIT();
   if (!bitmap)
   {
      std::free(rgba);
      return nullptr;
   }

   auto* output = reinterpret_cast<uint16*>(&bitmap[1]);
   for (size_t i = 0; i < pixelCount; ++i)
      output[i] = useBinaryAlpha ? Pack1555(&rgba[i * 4]) :
                  hasAlpha ? Pack4444(&rgba[i * 4]) :
                             Pack565(&rgba[i * 4]);
   std::free(rgba);

   gr_init_bitmap(bitmap, reinterpret_cast<uint8*>(output), BMT_FLAT16,
                  useBinaryAlpha ? BMF_RGB_1555 :
                  hasAlpha ? BMF_RGB_4444 : BMF_RGB_565,
                  static_cast<int>(width), static_cast<int>(height));
   return bitmap;
#endif
}

uint8* ResPngReadPalette(IStoreStream*, IResMemOverride* pResMem)
{
   if (!pResMem)
      return nullptr;

   LGALLOC_PUSH_CREDIT();
   auto* palette = static_cast<uint8*>(pResMem->ResMalloc(3 * 256));
   LGALLOC_POP_CREDIT();
   if (!palette)
      return nullptr;

   // True-colour PNG pixels do not consume this palette. Some legacy UI and
   // sky callers nevertheless retype every image as a palette.
   for (unsigned i = 0; i < 256; ++i)
   {
      palette[i * 3] = static_cast<uint8>(((i >> 5) & 7) * 255 / 7);
      palette[i * 3 + 1] = static_cast<uint8>(((i >> 2) & 7) * 255 / 7);
      palette[i * 3 + 2] = static_cast<uint8>((i & 3) * 255 / 3);
   }
   return palette;
}
