#include <gif.h>

#include <allocapi.h>
#include <bitmap.h>

#include <cstring>
#include <cstdio>
#include <limits>

namespace
{

#pragma pack(push, 1)
struct GifHeader
{
	char signature[6];
	uint16 width;
	uint16 height;
	uint8 flags;
	uint8 background;
	uint8 aspect;
};

struct GifImageBlock
{
	uint16 left;
	uint16 top;
	uint16 width;
	uint16 height;
	uint8 flags;
};
#pragma pack(pop)

static_assert(sizeof(GifHeader) == 13, "Invalid GIF header size");
static_assert(sizeof(GifImageBlock) == 9, "Invalid GIF image block size");

bool ReadExact(IStoreStream* pStream, void* pData, long size)
{
	return pStream->Read(size, static_cast<char*>(pData)) == size;
}

bool SkipBytes(IStoreStream* pStream, long size)
{
	uint8 buffer[255];
	while (size > 0)
	{
		const auto chunk = size > static_cast<long>(sizeof(buffer)) ? static_cast<long>(sizeof(buffer)) : size;
		if (!ReadExact(pStream, buffer, chunk))
			return false;
		size -= chunk;
	}
	return true;
}

bool ReadExtension(IStoreStream* pStream, int label, int* pTransparentIndex)
{
	bool firstBlock = true;
	while (true)
	{
		const auto size = pStream->Getc();
		if (size < 0)
			return false;
		if (size == 0)
			return true;

		uint8 block[255];
		if (!ReadExact(pStream, block, size))
			return false;
		if (firstBlock && label == 0xf9 && size >= 4 &&
			(block[0] & 1) != 0 && pTransparentIndex)
			*pTransparentIndex = block[3];
		firstBlock = false;
	}
}

class cGifDataReader
{
public:
	explicit cGifDataReader(IStoreStream* pStream)
		: m_pStream(pStream), m_Pos(0), m_Size(0), m_BitBuffer(0), m_BitCount(0), m_Ended(false)
	{}

	bool ReadCode(int codeSize, int* pCode)
	{
		while (m_BitCount < codeSize)
		{
			int value;
			if (!ReadByte(&value))
				return false;
			m_BitBuffer |= static_cast<uint32>(value) << m_BitCount;
			m_BitCount += 8;
		}

		*pCode = static_cast<int>(m_BitBuffer & ((1u << codeSize) - 1));
		m_BitBuffer >>= codeSize;
		m_BitCount -= codeSize;
		return true;
	}

private:
	bool ReadByte(int* pValue)
	{
		if (m_Pos >= m_Size)
		{
			if (m_Ended)
				return false;

			const auto size = m_pStream->Getc();
			if (size <= 0)
			{
				m_Ended = true;
				return false;
			}

			m_Size = size;
			m_Pos = 0;
			if (!ReadExact(m_pStream, m_Block, m_Size))
			{
				m_Ended = true;
				return false;
			}
		}

		*pValue = m_Block[m_Pos++];
		return true;
	}

	IStoreStream* m_pStream;
	uint8 m_Block[255];
	int m_Pos;
	int m_Size;
	uint32 m_BitBuffer;
	int m_BitCount;
	bool m_Ended;
};

bool DecodeImage(IStoreStream* pStream, int minimumCodeSize, uint8* pBits, const GifImageBlock& image)
{
	if (minimumCodeSize < 2 || minimumCodeSize > 8 || !image.width || !image.height)
	{
		std::fprintf(stderr, "GIF decode: invalid header values code=%d width=%u height=%u\n", minimumCodeSize, image.width, image.height);
		std::fflush(stderr);
		return false;
	}

	const auto pixelCount = static_cast<unsigned long>(image.width) * image.height;
	int16 prefix[4096]{};
	uint8 suffix[4096]{};
	uint8 stack[4096]{};

	const int clearCode = 1 << minimumCodeSize;
	const int endCode = clearCode + 1;
	int nextCode = endCode + 1;
	int codeSize = minimumCodeSize + 1;
	int codeLimit = 1 << codeSize;
	int oldCode = -1;
	uint8 firstValue = 0;
	unsigned long pixelsWritten = 0;
	uint16 x = 0;
	uint16 row = 0;
	int pass = 0;
	static const uint8 interlaceStep[] = { 8, 8, 4, 2 };
	static const uint8 interlaceStart[] = { 0, 4, 2, 1 };
	cGifDataReader reader(pStream);

	auto writePixel = [&](uint8 value) -> bool
	{
		if (pixelsWritten >= pixelCount || row >= image.height)
			return false;

		pBits[static_cast<unsigned long>(row) * image.width + x] = value;
		++pixelsWritten;
		if (++x == image.width)
		{
			x = 0;
			if ((image.flags & 0x40) == 0)
			{
				++row;
			}
			else
			{
				row = static_cast<uint16>(row + interlaceStep[pass]);
				while (row >= image.height && pass < 3)
					row = interlaceStart[++pass];
			}
		}
		return true;
	};

	while (true)
	{
		int code;
		if (!reader.ReadCode(codeSize, &code))
		{
			std::fprintf(stderr, "GIF decode: truncated data pixels=%lu next=%d size=%d\n", pixelsWritten, nextCode, codeSize);
			std::fflush(stderr);
			return false;
		}

		if (code == clearCode)
		{
			nextCode = endCode + 1;
			codeSize = minimumCodeSize + 1;
			codeLimit = 1 << codeSize;
			oldCode = -1;
			continue;
		}
		if (code == endCode)
		{
			if (pixelsWritten != pixelCount)
			{
				std::fprintf(stderr, "GIF decode: early EOI pixels=%lu expected=%lu\n", pixelsWritten, pixelCount);
				std::fflush(stderr);
			}
			return pixelsWritten == pixelCount;
		}
		if (code > nextCode || code >= 4096)
		{
			std::fprintf(stderr, "GIF decode: invalid code=%d next=%d size=%d pixels=%lu\n", code, nextCode, codeSize, pixelsWritten);
			std::fflush(stderr);
			return false;
		}

		const int inputCode = code;
		int stackSize = 0;
		if (code == nextCode)
		{
			if (oldCode < 0)
			{
				std::fprintf(stderr, "GIF decode: next code without old code\n");
				std::fflush(stderr);
				return false;
			}
			stack[stackSize++] = firstValue;
			code = oldCode;
		}

		while (code >= clearCode)
		{
			if (code <= endCode || code >= nextCode || stackSize >= 4096)
			{
				std::fprintf(stderr, "GIF decode: invalid chain code=%d next=%d stack=%d pixels=%lu\n", code, nextCode, stackSize, pixelsWritten);
				std::fflush(stderr);
				return false;
			}
			stack[stackSize++] = suffix[code];
			code = prefix[code];
		}

		firstValue = static_cast<uint8>(code);
		stack[stackSize++] = firstValue;
		while (stackSize > 0)
		{
			if (!writePixel(stack[--stackSize]))
			{
				std::fprintf(stderr, "GIF decode: pixel overflow pixels=%lu expected=%lu row=%u\n", pixelsWritten, pixelCount, row);
				std::fflush(stderr);
				return false;
			}
		}

		if (oldCode >= 0 && nextCode < 4096)
		{
			prefix[nextCode] = static_cast<int16>(oldCode);
			suffix[nextCode] = firstValue;
			++nextCode;
			if (nextCode == codeLimit && codeSize < 12)
			{
				++codeSize;
				codeLimit <<= 1;
			}
		}
		oldCode = inputCode;
	}
}

bool ReadHeader(IStoreStream* pStream, GifHeader* pHeader)
{
	pStream->SetPos(0);
	return ReadExact(pStream, pHeader, sizeof(*pHeader)) &&
		std::memcmp(pHeader->signature, "GIF", 3) == 0;
}

int PaletteEntries(uint8 flags)
{
	return 1 << ((flags & 7) + 1);
}

} // namespace

grs_bitmap* ResGifReadImage(IStoreStream* pStream, IResMemOverride* pResMem)
{
	GifHeader header{};
	if (!ReadHeader(pStream, &header))
		return nullptr;

	if ((header.flags & 0x80) != 0 && !SkipBytes(pStream, 3 * PaletteEntries(header.flags)))
		return nullptr;

	GifImageBlock image{};
	int transparentIndex = -1;
	while (true)
	{
		const auto marker = pStream->Getc();
		if (marker == 0x2c)
		{
			if (!ReadExact(pStream, &image, sizeof(image)))
				return nullptr;
			break;
		}
		if (marker == 0x21)
		{
			const int label = pStream->Getc();
			if (label < 0 || !ReadExtension(pStream, label, &transparentIndex))
				return nullptr;
			continue;
		}
		if (marker == 0)
			continue;
		return nullptr;
	}

	if ((image.flags & 0x80) != 0 && !SkipBytes(pStream, 3 * PaletteEntries(image.flags)))
		return nullptr;

	const auto minimumCodeSize = pStream->Getc();
	if (minimumCodeSize < 0 || !image.width || !image.height)
		return nullptr;

	const auto pixelCount = static_cast<unsigned long>(image.width) * image.height;
	if (pixelCount > std::numeric_limits<unsigned long>::max() - sizeof(grs_bitmap))
		return nullptr;

	LGALLOC_PUSH_CREDIT();
	auto* pBitmap = reinterpret_cast<grs_bitmap*>(pResMem->ResMalloc(pixelCount + sizeof(grs_bitmap)));
	LGALLOC_POP_CREDIT();
	if (!pBitmap)
		return nullptr;

	auto* pBits = reinterpret_cast<uint8*>(&pBitmap[1]);
	if (!DecodeImage(pStream, minimumCodeSize, pBits, image))
	{
		pResMem->ResFree(pBitmap);
		return nullptr;
	}

	// Dark's indexed transparency convention reserves palette index zero.
	// GIF permits any palette entry to be transparent, so exchange the two
	// indices while retaining all opaque index-zero pixels at their old colour.
	if (transparentIndex > 0 && transparentIndex < 256)
	{
		for (unsigned long i = 0; i < pixelCount; ++i)
		{
			if (pBits[i] == 0)
				pBits[i] = static_cast<uint8>(transparentIndex);
			else if (pBits[i] == transparentIndex)
				pBits[i] = 0;
		}
	}

	gr_init_bitmap(pBitmap, pBits, BMT_FLAT8, 0, image.width, image.height);
	return pBitmap;
}

uint8* ResGifReadPalette(IStoreStream* pStream, IResMemOverride* pResMem)
{
	GifHeader header{};
	if (!ReadHeader(pStream, &header))
		return nullptr;

	uint8 globalPalette[3 * 256]{};
	const int globalEntries = (header.flags & 0x80) != 0 ?
		PaletteEntries(header.flags) : 0;
	if (globalEntries != 0 &&
		!ReadExact(pStream, globalPalette, 3 * globalEntries))
		return nullptr;

	GifImageBlock image{};
	int transparentIndex = -1;
	while (true)
	{
		const auto marker = pStream->Getc();
		if (marker == 0x2c)
		{
			if (!ReadExact(pStream, &image, sizeof(image)))
				return nullptr;
			break;
		}
		if (marker == 0x21)
		{
			const int label = pStream->Getc();
			if (label < 0 || !ReadExtension(pStream, label, &transparentIndex))
				return nullptr;
			continue;
		}
		if (marker == 0)
			continue;
		return nullptr;
	}

	const int localEntries = (image.flags & 0x80) != 0 ?
		PaletteEntries(image.flags) : 0;
	if (localEntries == 0 && globalEntries == 0)
		return nullptr;

	LGALLOC_PUSH_CREDIT();
	auto* pPalette = static_cast<uint8*>(pResMem->ResMalloc(3 * 256));
	LGALLOC_POP_CREDIT();
	if (!pPalette)
		return nullptr;

	std::memset(pPalette, 0, 3 * 256);
	if (localEntries != 0)
	{
		// Pixel indices in an image with a local colour table refer to that
		// table, even when the file also supplies a global one.  Returning the
		// global table here produces the characteristic rainbow colours seen
		// when such replacement textures are decoded with the wrong palette.
		if (!ReadExact(pStream, pPalette, 3 * localEntries))
		{
			pResMem->ResFree(pPalette);
			return nullptr;
		}
	}
	else
		std::memcpy(pPalette, globalPalette, 3 * globalEntries);

	if (transparentIndex > 0 && transparentIndex < 256)
	{
		for (int channel = 0; channel < 3; ++channel)
		{
			const uint8 value = pPalette[channel];
			pPalette[channel] = pPalette[transparentIndex * 3 + channel];
			pPalette[transparentIndex * 3 + channel] = value;
		}
	}

	return pPalette;
}
