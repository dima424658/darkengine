#include <bmp.h>

struct BmpHead
{
	uint16 bfType;
	uint32 bfSize;
	uint16 bfReserved1;
	uint16 bfReserved2;
	uint32 bmOffBits;
};

struct BmpInfo
{
	uint32 biSize;
	uint32 biWidth;
	uint32 biHeight;
	uint16 biPlanes;
	uint16 biBitCount;
	uint32 biCompression;
	uint32 biSizeImage;
	uint32 biXPelsPerMeter;
	uint32 biYPelsPerMeter;
	uint32 biClrUsed;
	uint32 biClrImportant;
};

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