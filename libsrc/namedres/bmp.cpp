#include <bmp.h>

#include <allocapi.h>
#include <bitmap.h>

#include <cstdlib>
#include <cstring>
#include <limits>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

namespace
{
struct BmpInfo
{
	uint32 pixelOffset;
	uint32 dibSize;
	int32 width;
	int32 height;
	uint16 bitsPerPixel;
	uint32 compression;
	uint32 colorsUsed;
};

uint16 ReadLe16(const uint8* p)
{
	return static_cast<uint16>(p[0] | (static_cast<uint16>(p[1]) << 8));
}

uint32 ReadLe32(const uint8* p)
{
	return static_cast<uint32>(p[0]) |
		(static_cast<uint32>(p[1]) << 8) |
		(static_cast<uint32>(p[2]) << 16) |
		(static_cast<uint32>(p[3]) << 24);
}

bool ReadExact(IStoreStream* pStream, void* pData, long size)
{
	return size >= 0 && pStream->Read(size, static_cast<char*>(pData)) == size;
}

bool ReadHeader(IStoreStream* pStream, BmpInfo* pInfo)
{
	uint8 fileHeader[14];
	uint8 dibHeader[40];
	if (!pStream || !pInfo)
		return false;

	pStream->SetPos(0);
	if (!ReadExact(pStream, fileHeader, sizeof(fileHeader)) ||
		fileHeader[0] != 'B' || fileHeader[1] != 'M' ||
		!ReadExact(pStream, dibHeader, sizeof(dibHeader)))
		return false;

	pInfo->pixelOffset = ReadLe32(fileHeader + 10);
	pInfo->dibSize = ReadLe32(dibHeader);
	pInfo->width = static_cast<int32>(ReadLe32(dibHeader + 4));
	pInfo->height = static_cast<int32>(ReadLe32(dibHeader + 8));
	pInfo->bitsPerPixel = ReadLe16(dibHeader + 14);
	pInfo->compression = ReadLe32(dibHeader + 16);
	pInfo->colorsUsed = ReadLe32(dibHeader + 32);
	return pInfo->dibSize >= sizeof(dibHeader) &&
		ReadLe16(dibHeader + 12) == 1;
}

uint16 Pack565(uint8 red, uint8 green, uint8 blue)
{
	return static_cast<uint16>(((red >> 3) << 11) |
		((green >> 2) << 5) | (blue >> 3));
}

void FillFallbackPalette(uint8* pPalette)
{
	for (unsigned int i = 0; i < 256; ++i)
	{
		pPalette[i * 3] = static_cast<uint8>(i);
		pPalette[i * 3 + 1] = static_cast<uint8>(i);
		pPalette[i * 3 + 2] = static_cast<uint8>(i);
	}
}
}

grs_bitmap* ResBmpReadImage(IStoreStream* pStream, IResMemOverride* pResMem)
{
	BmpInfo header{};
	if (!pResMem || !ReadHeader(pStream, &header) || header.width <= 0 ||
		!header.height || header.height == std::numeric_limits<int32>::min())
		return nullptr;

	// BI_RGB is the original uncompressed BMP representation used by Dark
	// assets. RLE and bitfield layouts need format-specific mask handling and
	// must not be mistaken for the byte layouts below.
	if (header.compression != 0 ||
		(header.bitsPerPixel != 8 && header.bitsPerPixel != 16 &&
		 header.bitsPerPixel != 24 && header.bitsPerPixel != 32))
	{
		Warning(("Unsupported BMP format: compression %u, depth %u\n",
			header.compression, header.bitsPerPixel));
		return nullptr;
	}

	const auto width = static_cast<size_t>(header.width);
	const auto height = static_cast<size_t>(
		header.height < 0 ? -header.height : header.height);
	if (width > std::numeric_limits<size_t>::max() / height)
		return nullptr;
	const auto pixelCount = width * height;
	if (width > (std::numeric_limits<size_t>::max() - 31) /
		header.bitsPerPixel)
		return nullptr;
	const auto sourceBitsPerRow = width * header.bitsPerPixel;
	const auto sourceRow = ((sourceBitsPerRow + 31) / 32) * 4;
	const auto outputBytesPerPixel = header.bitsPerPixel == 8 ? 1u : 2u;
	if (sourceRow > static_cast<size_t>(std::numeric_limits<long>::max()) ||
		pixelCount > (std::numeric_limits<ulong>::max() - sizeof(grs_bitmap)) /
			outputBytesPerPixel)
		return nullptr;

	const auto allocationSize = sizeof(grs_bitmap) +
		pixelCount * outputBytesPerPixel;
	LGALLOC_PUSH_CREDIT();
	auto* pBitmap = reinterpret_cast<grs_bitmap*>(
		pResMem->ResMalloc(static_cast<ulong>(allocationSize)));
	LGALLOC_POP_CREDIT();
	if (!pBitmap)
		return nullptr;

	auto* pSourceRow = static_cast<uint8*>(std::malloc(sourceRow));
	if (!pSourceRow)
	{
		pResMem->ResFree(pBitmap);
		return nullptr;
	}

	auto* pOutput = reinterpret_cast<uint8*>(&pBitmap[1]);
	pStream->SetPos(header.pixelOffset);
	for (size_t sourceY = 0; sourceY < height; ++sourceY)
	{
		if (!ReadExact(pStream, pSourceRow, static_cast<long>(sourceRow)))
		{
			Warning(("Unable to read BMP image data\n"));
			std::free(pSourceRow);
			pResMem->ResFree(pBitmap);
			return nullptr;
		}

		const auto destY = header.height < 0 ? sourceY : height - 1 - sourceY;
		if (header.bitsPerPixel == 8)
		{
			std::memcpy(pOutput + destY * width, pSourceRow, width);
			continue;
		}

		auto* pDest = reinterpret_cast<uint16*>(pOutput) + destY * width;
		for (size_t x = 0; x < width; ++x)
		{
			if (header.bitsPerPixel == 16)
			{
				// BI_RGB 16-bit BMP is RGB555. Expand it to the engine's
				// explicitly tagged opaque RGB565 representation.
				const auto pixel = ReadLe16(pSourceRow + x * 2);
				const auto red = static_cast<uint8>(((pixel >> 10) & 31) * 255 / 31);
				const auto green = static_cast<uint8>(((pixel >> 5) & 31) * 255 / 31);
				const auto blue = static_cast<uint8>((pixel & 31) * 255 / 31);
				pDest[x] = Pack565(red, green, blue);
			}
			else
			{
				const auto bytesPerPixel = header.bitsPerPixel / 8;
				const auto* pixel = pSourceRow + x * bytesPerPixel;
				// BI_RGB stores B, G, R and an unused fourth byte for 32-bit
				// images. Treat that byte as padding rather than turning the
				// usual zero value into transparent pixels.
				pDest[x] = Pack565(pixel[2], pixel[1], pixel[0]);
			}
		}
	}
	std::free(pSourceRow);

	gr_init_bitmap(pBitmap, pOutput,
		header.bitsPerPixel == 8 ? BMT_FLAT8 : BMT_FLAT16,
		header.bitsPerPixel == 8 ? 0 : BMF_RGB_565,
		static_cast<int>(width), static_cast<int>(height));
	return pBitmap;
}

uint8* ResBmpReadPalette(IStoreStream* pStream, IResMemOverride* pResMem)
{
	constexpr auto PaletteEntries = 256u;
	constexpr auto PaletteSize = PaletteEntries * 3u;
	BmpInfo header{};
	if (!pResMem || !ReadHeader(pStream, &header))
		return nullptr;

	LGALLOC_PUSH_CREDIT();
	auto* pPalette = static_cast<uint8*>(pResMem->ResMalloc(PaletteSize));
	LGALLOC_POP_CREDIT();
	if (!pPalette)
		return nullptr;
	FillFallbackPalette(pPalette);

	if (header.bitsPerPixel > 8)
		return pPalette;

	const auto availableEntries = header.colorsUsed ? header.colorsUsed :
		(1u << header.bitsPerPixel);
	const auto entries = availableEntries < PaletteEntries ?
		availableEntries : PaletteEntries;
	if (header.dibSize >
		static_cast<uint32>(std::numeric_limits<long>::max() - 14))
	{
		pResMem->ResFree(pPalette);
		return nullptr;
	}
	pStream->SetPos(14 + static_cast<long>(header.dibSize));
	for (unsigned int i = 0; i < entries; ++i)
	{
		uint8 entry[4];
		if (!ReadExact(pStream, entry, sizeof(entry)))
		{
			pResMem->ResFree(pPalette);
			return nullptr;
		}
		pPalette[i * 3] = entry[2];
		pPalette[i * 3 + 1] = entry[1];
		pPalette[i * 3 + 2] = entry[0];
	}
	return pPalette;
}
