#include <tga.h>

#include <allocapi.h>
#include <bitmap.h>

#include <cstring>
#include <limits>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

namespace
{
struct TgaHeader
{
	uint8 idLength;
	uint8 colorMapType;
	uint8 imageType;
	uint16 colorMapFirst;
	uint16 colorMapLength;
	uint8 colorMapDepth;
	uint16 xOrigin;
	uint16 yOrigin;
	uint16 width;
	uint16 height;
	uint8 pixelDepth;
	uint8 descriptor;
};

bool ReadExact(IStoreStream* pStream, void* pData, long size)
{
	return pStream->Read(size, static_cast<char*>(pData)) == size;
}

bool SkipBytes(IStoreStream* pStream, long size)
{
	if (size < 0)
		return false;
	return pStream->SetPos(pStream->GetPos() + size) != FALSE;
}

uint16 ReadLe16(const uint8* p)
{
	return static_cast<uint16>(p[0] | (static_cast<uint16>(p[1]) << 8));
}

bool ReadHeader(IStoreStream* pStream, TgaHeader* pHeader)
{
	uint8 raw[18];
	pStream->SetPos(0);
	if (!ReadExact(pStream, raw, sizeof(raw)))
		return false;

	pHeader->idLength = raw[0];
	pHeader->colorMapType = raw[1];
	pHeader->imageType = raw[2];
	pHeader->colorMapFirst = ReadLe16(raw + 3);
	pHeader->colorMapLength = ReadLe16(raw + 5);
	pHeader->colorMapDepth = raw[7];
	pHeader->xOrigin = ReadLe16(raw + 8);
	pHeader->yOrigin = ReadLe16(raw + 10);
	pHeader->width = ReadLe16(raw + 12);
	pHeader->height = ReadLe16(raw + 14);
	pHeader->pixelDepth = raw[16];
	pHeader->descriptor = raw[17];
	return true;
}

struct RgbaPixel
{
	uint8 r;
	uint8 g;
	uint8 b;
	uint8 a;
};

bool ReadTrueColorPixel(IStoreStream* pStream, uint8 depth,
	uint8 alphaBits, RgbaPixel* pPixel)
{
	uint8 raw[4];
	const auto bytes = static_cast<long>((depth + 7) / 8);
	if ((depth != 16 && depth != 24 && depth != 32) ||
		!ReadExact(pStream, raw, bytes))
		return false;

	if (depth == 16)
	{
		const auto value = ReadLe16(raw);
		pPixel->b = static_cast<uint8>((value & 0x1f) * 255 / 31);
		pPixel->g = static_cast<uint8>(((value >> 5) & 0x1f) * 255 / 31);
		pPixel->r = static_cast<uint8>(((value >> 10) & 0x1f) * 255 / 31);
		pPixel->a = alphaBits ? ((value & 0x8000) ? 255 : 0) : 255;
	}
	else
	{
		pPixel->b = raw[0];
		pPixel->g = raw[1];
		pPixel->r = raw[2];
		pPixel->a = depth == 32 ? raw[3] : 255;
	}
	return true;
}

uint16 Pack4444(const RgbaPixel& pixel)
{
	return static_cast<uint16>(
		((pixel.a >> 4) << 12) | ((pixel.r >> 4) << 8) |
		((pixel.g >> 4) << 4) | (pixel.b >> 4));
}
}

grs_bitmap* ResTgaReadImage(IStoreStream* pStream, IResMemOverride* pResMem)
{
	TgaHeader header{};
	if (!ReadHeader(pStream, &header) || !header.width || !header.height)
		return nullptr;

	// Dark's shipped sky art is uncompressed 32-bit true-color TGA.  RLE
	// true-color is inexpensive to support here as well and uses the same
	// packet layout.
	if (header.colorMapType != 0 ||
		(header.imageType != 2 && header.imageType != 10) ||
		(header.pixelDepth != 16 && header.pixelDepth != 24 &&
		 header.pixelDepth != 32))
	{
		Warning(("Unsupported TGA format: type %u, map %u, depth %u\n",
			header.imageType, header.colorMapType, header.pixelDepth));
		return nullptr;
	}

	if (!SkipBytes(pStream, header.idLength))
		return nullptr;

	const auto pixelCount = static_cast<unsigned long>(header.width) * header.height;
	if (pixelCount >
		(std::numeric_limits<unsigned long>::max() - sizeof(grs_bitmap)) / sizeof(uint16))
		return nullptr;

	LGALLOC_PUSH_CREDIT();
	auto* pBitmap = reinterpret_cast<grs_bitmap*>(
		pResMem->ResMalloc(sizeof(grs_bitmap) + pixelCount * sizeof(uint16)));
	LGALLOC_POP_CREDIT();
	if (!pBitmap)
		return nullptr;

	auto* pBits = reinterpret_cast<uint16*>(&pBitmap[1]);
	const bool topOrigin = (header.descriptor & 0x20) != 0;
	const bool rightOrigin = (header.descriptor & 0x10) != 0;
	const auto alphaBits = static_cast<uint8>(header.descriptor & 0x0f);
	unsigned long sourcePixel = 0;

	while (sourcePixel < pixelCount)
	{
		unsigned long packetPixels = 1;
		bool repeat = false;
		if (header.imageType == 10)
		{
			const auto packet = pStream->Getc();
			if (packet < 0)
				goto read_failed;
			packetPixels = static_cast<unsigned long>((packet & 0x7f) + 1);
			repeat = (packet & 0x80) != 0;
			if (packetPixels > pixelCount - sourcePixel)
				goto read_failed;
		}

		RgbaPixel repeated{};
		if (repeat && !ReadTrueColorPixel(pStream, header.pixelDepth,
			alphaBits, &repeated))
			goto read_failed;

		for (unsigned long i = 0; i < packetPixels; ++i, ++sourcePixel)
		{
			RgbaPixel pixel = repeated;
			if (!repeat && !ReadTrueColorPixel(pStream, header.pixelDepth,
				alphaBits, &pixel))
				goto read_failed;

			const auto sourceY = sourcePixel / header.width;
			const auto sourceX = sourcePixel % header.width;
			const auto destY = topOrigin ? sourceY : header.height - 1 - sourceY;
			const auto destX = rightOrigin ? header.width - 1 - sourceX : sourceX;
			pBits[destY * header.width + destX] = Pack4444(pixel);
		}
	}

	// A number of widely used Thief enhancement packs contain X1R5G5B5
	// textures whose headers incorrectly advertise one alpha bit.  Since the
	// unused high bit is clear in every pixel, interpreting that header
	// literally makes otherwise valid AI and object textures fully invisible.
	// Preserve genuine 1-bit transparency, but treat an entirely transparent
	// 16-bit image as opaque, which is also how legacy Dark handled these files.
	if (header.pixelDepth == 16 && alphaBits)
	{
		bool hasOpaquePixel = false;
		for (unsigned long i = 0; i < pixelCount; ++i)
		{
			if (pBits[i] & 0xf000)
			{
				hasOpaquePixel = true;
				break;
			}
		}

		if (!hasOpaquePixel)
		{
			for (unsigned long i = 0; i < pixelCount; ++i)
				pBits[i] |= 0xf000;
		}
	}

	gr_init_bitmap(pBitmap, reinterpret_cast<uint8*>(pBits), BMT_FLAT16,
		BMF_RGB_4444, header.width, header.height);
	return pBitmap;

read_failed:
	Warning(("Unable to read TGA image data\n"));
	pResMem->ResFree(pBitmap);
	return nullptr;
}

uint8* ResTgaReadPalette(IStoreStream* pStream, IResMemOverride* pResMem)
{
	constexpr auto PaletteEntries = 256u;
	constexpr auto PaletteSize = PaletteEntries * 3u;

	if (!pStream || !pResMem)
		return nullptr;

	TgaHeader header{};
	if (!ReadHeader(pStream, &header) || !SkipBytes(pStream, header.idLength))
		return nullptr;

	LGALLOC_PUSH_CREDIT();
	auto* pPalette = static_cast<uint8*>(pResMem->ResMalloc(PaletteSize));
	LGALLOC_POP_CREDIT();
	if (!pPalette)
		return nullptr;

	std::memset(pPalette, 0, PaletteSize);
	if (header.colorMapType == 1)
	{
		const auto first = static_cast<unsigned long>(header.colorMapFirst);
		const auto count = static_cast<unsigned long>(header.colorMapLength);
		if (!count || first >= PaletteEntries || count > PaletteEntries - first ||
			(header.colorMapDepth != 16 && header.colorMapDepth != 24 &&
			 header.colorMapDepth != 32))
		{
			pResMem->ResFree(pPalette);
			return nullptr;
		}

		for (unsigned long i = 0; i < count; ++i)
		{
			RgbaPixel pixel{};
			if (!ReadTrueColorPixel(pStream, header.colorMapDepth, 0, &pixel))
			{
				pResMem->ResFree(pPalette);
				return nullptr;
			}

			auto* pEntry = &pPalette[(first + i) * 3];
			pEntry[0] = pixel.r;
			pEntry[1] = pixel.g;
			pEntry[2] = pixel.b;
		}
	}
	else if (header.colorMapType == 0)
	{
		// Enhanced-sky code asks for a palette even for true-color TGAs.
		// Their decoded BMT_FLAT16 pixels do not use the palette slot, but a
		// stable fallback keeps that optional lookup from becoming a hard
		// resource-load failure.
		for (unsigned int i = 0; i < PaletteEntries; ++i)
		{
			pPalette[i * 3] = static_cast<uint8>(((i >> 5) & 7) * 255 / 7);
			pPalette[i * 3 + 1] = static_cast<uint8>(((i >> 2) & 7) * 255 / 7);
			pPalette[i * 3 + 2] = static_cast<uint8>((i & 3) * 255 / 3);
		}
	}
	else
	{
		pResMem->ResFree(pPalette);
		return nullptr;
	}

	return pPalette;
}
