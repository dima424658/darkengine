// $Source: x:/prj/tech/libsrc/res/RCS/pkzip.cpp $
// $Author: TOML $
// $Date: 1997/01/17 11:04:24 $
// $Revision: 1.6 $
//

#ifdef _WIN32

#include <windows.h>

#include <string.h>
#include <io.h>

#include <lg.h>
#include <dynfunc.h>

#include <pkzip.h>
#include <lzw.h>
#include <implode.h>

///////////////////////////////////////////////////////////////////////////////
//
// Dynamic loading
//

static void PkFail()
{
    CriticalMsg("Failed to locate and load implode.dll!");
    exit(1);
}

typedef unsigned int (__cdecl *tPkReadFunc)(char *buf, unsigned int *size, void *param);
typedef void         (__cdecl *tPkWriteFunc)(char *buf, unsigned int *size, void *param);

DeclDynFunc_(unsigned int, __cdecl, implode, (tPkReadFunc, tPkWriteFunc, char *, void *, unsigned int *, unsigned int *));
DeclDynFunc_(unsigned int, __cdecl, explode, (tPkReadFunc, tPkWriteFunc, char *, void *));

ImplDynFunc(implode, "implode.dll", "implode", PkFail);
ImplDynFunc(explode, "implode.dll", "explode", PkFail);

#define DynImplode (DynFunc(implode).GetProcAddress())
#define DynExplode (DynFunc(explode).GetProcAddress())

///////////////////////////////////////////////////////////////////////////////

extern "C" void *lzwBuffer;
#define PkMallocBuffer() LzwMallocBuffer()
#define pPkBuffer        lzwBuffer

///////////////////////////////////////////////////////////////////////////////
//
// Compress memory-to-memory
//

struct sPkImplodeInfo
{
    const BYTE * pSource;
    const BYTE * pSourceLimit;
    BYTE *       pDest;
    const BYTE * pDestLimit;
    BOOL         fFailedSafe;
};

///////////////////////////////////////

unsigned int __cdecl PkImplodeRead(char *buf, unsigned int *size, void *param)
{
    if (!param)
        return 0;

    sPkImplodeInfo * const pImplodeInfo = (sPkImplodeInfo *) param;

    const unsigned readLimit = pImplodeInfo->pSourceLimit - pImplodeInfo->pSource;
    if (readLimit)
    {
        const unsigned readSize = (readLimit > *size) ? *size : readLimit;

        memcpy(buf, pImplodeInfo->pSource, readSize);
        pImplodeInfo->pSource += readSize;
        *size = readSize;     // Example code shows setting size.  Is it really needed ? (toml 01-13-97)
        return readSize;
    }
    return 0;
}

///////////////////////////////////////

void __cdecl PkImplodeWrite(char *buf, unsigned int *size, void *param)
{
    sPkImplodeInfo * const pImplodeInfo = (sPkImplodeInfo *) param;

    if (!pImplodeInfo->fFailedSafe && pImplodeInfo->pDest + *size < pImplodeInfo->pDestLimit)
    {
        memcpy(pImplodeInfo->pDest, buf, *size);
        pImplodeInfo->pDest += *size;
    }
    else
    {
        // Probably trying to compress something really small, or uncompressible.  Compressed version is bigger...
        pImplodeInfo->fFailedSafe = TRUE;
    }
}

///////////////////////////////////////

const double   kMinCompression   = 0.05;
const unsigned kSmallDictionary  = 0x400;
const unsigned kMediumDictionary = 0x800;
const unsigned kLargeDictionary  = 0x1000;

long PkImplodeMemToMem(const void * pSource, long sizeSource,
                       void * pDest, long destMax)
{
    if (!pPkBuffer)
        PkMallocBuffer();

    char *          pWorkBuf = (char *) pPkBuffer;
    sPkImplodeInfo  implodeInfo;
    unsigned int    type;
    unsigned int    dictSize;

    implodeInfo.pSource      = (const BYTE *) pSource;
    implodeInfo.pSourceLimit = implodeInfo.pSource + sizeSource;
    implodeInfo.pDest        = (BYTE *) pDest;
    implodeInfo.pDestLimit   = implodeInfo.pDest + destMax;
    implodeInfo.fFailedSafe  = FALSE;

    type = CMP_BINARY;

    dictSize = (sizeSource > kLargeDictionary) ?  kLargeDictionary :
               (sizeSource > kMediumDictionary) ? kMediumDictionary :
                                                  kSmallDictionary;


    unsigned result = DynImplode(PkImplodeRead,
                                 PkImplodeWrite,
                                 pWorkBuf,
                                 &implodeInfo,
                                 &type,
                                 &dictSize);

    unsigned compressedSize = implodeInfo.pDest - (BYTE *) pDest;
    if (((double)(sizeSource - compressedSize)/(double)sizeSource) >= kMinCompression)
    {
        if (!implodeInfo.fFailedSafe && result == 0 && implodeInfo.pDest < implodeInfo.pDestLimit)
            return (compressedSize);

        AssertMsg1(implodeInfo.fFailedSafe, "Compression failed (%d)!", result);
    }
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
//
// Expand file-to-memory
//

const unsigned kReadBufSize = 0x2000;
const unsigned kMinReadSize = 512;

struct sPkExplodeInfo
{
    int          fdSource;

    const BYTE * pSource;
    const BYTE * pSourceLimit;
    BYTE *       pDest;
    const BYTE * pDestLimit;
    ulong        skip;

    BYTE *       pReadBuf;
    BOOL         fComplete;
};

///////////////////////////////////////

unsigned int __cdecl PkExplodeRead(char *buf, unsigned int *size, void *param)
{
    // @TBD (toml 01-13-97): We're buffering... is it good?
    sPkExplodeInfo * const pExplodeInfo = (sPkExplodeInfo *) param;

    if (!pExplodeInfo || pExplodeInfo->fComplete)
        return 0;

    if (pExplodeInfo->pSource >= pExplodeInfo->pSourceLimit)
    {
        // Worst case is 0% compression (degenerate cases are eliminated by implosion)
        // the most we'll ever have to read is the size of the uncompressed data
        unsigned targetReadSize, actualReadSize;

        targetReadSize = (pExplodeInfo->skip) ? pExplodeInfo->skip :
                                                pExplodeInfo->pDestLimit - pExplodeInfo->pDest;

        if (targetReadSize > kReadBufSize)
            targetReadSize = kReadBufSize;
        else if (targetReadSize < kMinReadSize)
            targetReadSize = kMinReadSize;

        actualReadSize = read(pExplodeInfo->fdSource, pExplodeInfo->pReadBuf, targetReadSize);

        pExplodeInfo->pSource         = pExplodeInfo->pReadBuf;
        pExplodeInfo->pSourceLimit    = pExplodeInfo->pSource + actualReadSize;
    }

    const unsigned readLimit = pExplodeInfo->pSourceLimit - pExplodeInfo->pSource;
    if (readLimit)
    {
        const unsigned readSize = (readLimit > *size) ? *size : readLimit;

        memcpy(buf, pExplodeInfo->pSource, readSize);
        pExplodeInfo->pSource += readSize;
        *size = readSize;     // Example code shows setting size.  Is it really needed ? (toml 01-13-97)
        return readSize;
    }
    return 0;
}

///////////////////////////////////////

void __cdecl PkExplodeWrite(char *buf, unsigned int *size, void *param)
{
    sPkExplodeInfo * const pExplodeInfo = (sPkExplodeInfo *) param;

    unsigned actualSize = *size;

    // If we have more to skip
    if (pExplodeInfo->skip)
    {
        if (pExplodeInfo->skip > actualSize)
        {
            // And it's greater than the present amount, skip and return
            pExplodeInfo->skip -= actualSize;
            return;
        }
        else
        {
            // Otherwise, skip the remaining amount
            actualSize -= pExplodeInfo->skip;
            buf += pExplodeInfo->skip;
            pExplodeInfo->skip = 0;
        }
    }

    // If we're writing past the desired amount, set write to the limit
    if ((pExplodeInfo->pDest + actualSize) > pExplodeInfo->pDestLimit)
        actualSize = pExplodeInfo->pDestLimit - pExplodeInfo->pDest;

    memcpy(pExplodeInfo->pDest, buf, actualSize);
    pExplodeInfo->pDest += actualSize;

    // If we're past the desired amount, skip the rest...
    if (pExplodeInfo->pDest > pExplodeInfo->pDestLimit)
        pExplodeInfo->fComplete = TRUE;
}

///////////////////////////////////////

const int kPkExplodeMaxDest = 0x8000000L;

long PkExplodeFileToMem(int fdSource, void * pDest,
                        long skip, long destMax)
{
    if (!pPkBuffer)
        PkMallocBuffer();

    char *          pWorkBuf = (char *) pPkBuffer;
    sPkExplodeInfo  explodeInfo;

    if (!destMax)
        destMax = kPkExplodeMaxDest;

    explodeInfo.fdSource     = fdSource;
    explodeInfo.pReadBuf     = (BYTE *) pPkBuffer + EXP_BUFFER_SIZE;
    explodeInfo.pSource      = explodeInfo.pReadBuf;
    explodeInfo.pSourceLimit = explodeInfo.pReadBuf;
    explodeInfo.pDest        = (BYTE *) pDest;
    explodeInfo.pDestLimit   = explodeInfo.pDest + destMax;
    explodeInfo.skip         = skip;
    explodeInfo.fComplete    = FALSE;

    unsigned result = DynExplode(PkExplodeRead,
                                 PkExplodeWrite,
                                 pWorkBuf,
                                 &explodeInfo);

    if ((result == 0 ||
         (result == CMP_ABORT && explodeInfo.fComplete)) &&
        explodeInfo.pDest <= explodeInfo.pDestLimit)
    {
        return (explodeInfo.pDest - (BYTE *) pDest);
    }

    CriticalMsg1("Expansion failed (%d)!", result);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

#endif
