#pragma once

#include <storeapi.h>

enum class eImageContent
{
   kUnknown,
   kGIF,
   kDDS,
   kPCX,
   kPNG,
   kPSD,
   kTIFF,
};

inline eImageContent DetectImageContent(IStoreStream* pStream)
{
   uint8 signature[12]{};
   if (!pStream)
      return eImageContent::kUnknown;

   pStream->SetPos(0);
   const long count = pStream->Read(sizeof(signature),
      reinterpret_cast<char*>(signature));
   pStream->SetPos(0);

   if (count >= 8 && signature[0] == 0x89 && signature[1] == 'P' &&
       signature[2] == 'N' && signature[3] == 'G' && signature[4] == 0x0d &&
       signature[5] == 0x0a && signature[6] == 0x1a && signature[7] == 0x0a)
      return eImageContent::kPNG;
   if (count >= 6 && signature[0] == 'G' && signature[1] == 'I' &&
       signature[2] == 'F' && signature[3] == '8')
      return eImageContent::kGIF;
   if (count >= 4 && signature[0] == 'D' && signature[1] == 'D' &&
       signature[2] == 'S' && signature[3] == ' ')
      return eImageContent::kDDS;
   if (count >= 4 && signature[0] == 0x0a && signature[2] == 1 &&
       signature[3] > 0 && signature[3] <= 8)
      return eImageContent::kPCX;
   if (count >= 4 && signature[0] == '8' && signature[1] == 'B' &&
       signature[2] == 'P' && signature[3] == 'S')
      return eImageContent::kPSD;
   if (count >= 4 &&
       ((signature[0] == 'I' && signature[1] == 'I' && signature[2] == 42 && signature[3] == 0) ||
        (signature[0] == 'M' && signature[1] == 'M' && signature[2] == 0 && signature[3] == 42)))
      return eImageContent::kTIFF;
   return eImageContent::kUnknown;
}
