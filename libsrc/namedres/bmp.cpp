#include <bmp.h>

bool ResBmpReadHeader(IStoreStream* pStream, BmpHead* pBmpHead, BmpInfo* pBmpInfo)
{
	return false;
}

grs_bitmap* ResBmpReadImage(IStoreStream* pStream, IResMemOverride* pResMem)
{
	return nullptr;
}

uint8* ResBmpReadPalette(IStoreStream* pStream, IResMemOverride* pResMem)
{
	return nullptr;
}