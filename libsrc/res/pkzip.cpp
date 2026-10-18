// PKWARE Data Compression Library stream support.

#include <string.h>

#include <platform_io.h>
#include <lg.h>
#include <pkzip.h>

extern "C"
{
#include <blast.h>
}

namespace
{
const unsigned kReadBufferSize = 0x2000;
const long kMaxDestinationSize = 0x8000000L;

struct sPkExplodeInfo
{
   int fdSource;
   BYTE readBuffer[kReadBufferSize];
   BYTE *pDest;
   const BYTE *pDestLimit;
   ulong skip;
   BOOL complete;
};

unsigned PkExplodeRead(void *parameter, unsigned char **buffer)
{
   sPkExplodeInfo *info = static_cast<sPkExplodeInfo *>(parameter);
   int count;

   if (!info || info->complete)
      return 0;

   count = read(info->fdSource, info->readBuffer,
                static_cast<unsigned>(sizeof(info->readBuffer)));
   if (count <= 0)
      return 0;

   *buffer = info->readBuffer;
   return static_cast<unsigned>(count);
}

int PkExplodeWrite(void *parameter, unsigned char *buffer, unsigned size)
{
   sPkExplodeInfo *info = static_cast<sPkExplodeInfo *>(parameter);

   if (info->skip)
   {
      const unsigned skipped = info->skip < size ? info->skip : size;
      info->skip -= skipped;
      buffer += skipped;
      size -= skipped;
   }

   const size_t available = static_cast<size_t>(info->pDestLimit - info->pDest);
   const size_t copied = available < size ? available : size;
   if (copied)
   {
      memcpy(info->pDest, buffer, copied);
      info->pDest += copied;
   }

   if (copied != size || info->pDest == info->pDestLimit)
   {
      info->complete = TRUE;
      return 1;
   }
   return 0;
}
}

long PkExplodeFileToMem(int fdSource, void *pDest, long skip, long destMax)
{
   if (!pDest || skip < 0 || destMax < 0)
      return 0;
   if (!destMax)
      destMax = kMaxDestinationSize;

   sPkExplodeInfo info = {};
   info.fdSource = fdSource;
   info.pDest = static_cast<BYTE *>(pDest);
   info.pDestLimit = info.pDest + destMax;
   info.skip = static_cast<ulong>(skip);

   const int result = blast(PkExplodeRead, &info,
                            PkExplodeWrite, &info, nullptr, nullptr);
   if ((result == 0 || (result == 1 && info.complete)) &&
       info.pDest <= info.pDestLimit)
      return static_cast<long>(info.pDest - static_cast<BYTE *>(pDest));

   CriticalMsg1("Expansion failed (%d)!", result);
   return 0;
}
