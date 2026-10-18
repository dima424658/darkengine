#include <pcx.h>
#include <allocapi.h>
#include <bitmap.h>
#include <lgassert.h>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

struct PCXHEAD
{
	int8 manufacturer;
	int8 version;
	int8 encoding;
	int8 bits_per_pixel;
	int16 xmin;
	int16 ymin;
	int16 xmax;
	int16 ymax;
	int16 hres;
	int16 vres;
	int8 palette[48];
	int8 reserved;
	int8 colour_planes;
	int16 bytes_per_line;
	int16 palette_type;
	int8 filler[58];
};

static_assert(sizeof(PCXHEAD) == 128, "Invalid size");

static int readBuffIndex;
static int readBuffValid;
static int readRunRemaining;
static uint8 readRunValue;
static uint8* readBuff;

bool PcxReadRow(IStoreStream* pStream, uint8* p, int npixels);
bool PcxReadBody(IStoreStream* pStream, uint8* bits, uint8 type, int16 w, int16 h, uint16 row, int16 bpl);

bool PcxReadHeader(IStoreStream* pStream, PCXHEAD* phead)
{
	pStream->SetPos(0);
	pStream->Read(sizeof(PCXHEAD), reinterpret_cast<char*>(phead));

	if (phead->version != 5)
	{
		Warning(("Bad PCX header: not version 3.0\n"));
		return false;
	}

	if (phead->encoding != 1)
	{
		Warning(("Bad PCX header: not compressed\n"));
		return false;
	}

	if (phead->xmin != 0 || phead->ymin != 0)
	{
		Warning(("Bad PCX header: picture not at 0,0\n"));
		return false;
	}

	if (phead->bits_per_pixel != 8)
	{
		Warning(("PCX file must have 8 bits per pixel\n"));
		return false;
	}

	return true;
}

static bool PcxReadByte(IStoreStream* pStream, uint8* value)
{
	if (readBuffIndex >= readBuffValid)
	{
		readBuffValid = pStream->Read(4096, reinterpret_cast<char*>(readBuff));
		readBuffIndex = 0;
		if (readBuffValid <= 0)
			return false;
	}

	*value = readBuff[readBuffIndex++];
	return true;
}

bool PcxReadRow(IStoreStream* pStream, uint8* p, int npixels)
{
	auto* pend = &p[npixels];
	while (p < pend)
	{
		if (readRunRemaining)
		{
			const auto count = static_cast<int>(pend - p) < readRunRemaining ?
				static_cast<int>(pend - p) : readRunRemaining;
			memset(p, readRunValue, count);
			p += count;
			readRunRemaining -= count;
			continue;
		}

		uint8 value;
		if (!PcxReadByte(pStream, &value))
			return false;
		if ((value & 0xC0) == 0xC0)
		{
			auto count = value & 0x3F;
			uint8 valuea;
			if (!count || !PcxReadByte(pStream, &valuea))
			{
				Warning(("PCX decompression error\n"));
				return false;
			}
			const auto available = static_cast<int>(pend - p);
			const auto copied = count < available ? count : available;
			memset(p, valuea, copied);
			p += copied;
			readRunValue = valuea;
			readRunRemaining = count - copied;
		}
		else
		{
			*p++ = value;
		}
	}

	return 1;
}

bool PcxReadBody(IStoreStream* pStream, uint8* bits, uint8 type, int16 w, int16 h, uint16 row, int16 bpl)
{
	pStream->SetPos(128);

	const int planes = type == BMT_FLAT8 ? 1 : 3;
	auto* scanline = static_cast<uint8*>(malloc(planes * bpl));
	if (!scanline)
	{
		Warning(("PCX reader can't alloc memory\n"));
		return false;
	}

	bool result = true;
	for (int y = 0; y < h; ++y)
	{
		for (int plane = 0; plane < planes; ++plane)
		{
			if (!PcxReadRow(pStream, &scanline[bpl * plane], bpl))
			{
				Warning(("PCX decompression error on line: %d\n", y));
				result = false;
				break;
			}
		}
		if (!result)
			break;

		if (type == BMT_FLAT8)
			memcpy(bits, scanline, w);
		else if (type == BMT_FLAT16)
		{
			auto* pd = reinterpret_cast<uint16*>(bits);
			for (int i = 0; i < w; ++i)
			{
				const auto red = scanline[i];
				const auto green = scanline[bpl + i];
				const auto blue = scanline[2 * bpl + i];
				*pd++ = static_cast<uint16>(((red >> 3) << 11) |
					((green >> 2) << 5) | (blue >> 3));
			}
		}
		else
		{
			Warning(("PCX file has %d planes!\n", type));
			result = false;
			break;
		}

		bits += row;
	}

	free(scanline);
	return result;
}


grs_bitmap* ResPcxReadImage(IStoreStream* pStream, IResMemOverride* pResMem)
{
	PCXHEAD pcxHeader{};
	if (!PcxReadHeader(pStream, &pcxHeader))
		return nullptr;

	auto w = pcxHeader.xmax - pcxHeader.xmin + 1;
	auto h = pcxHeader.ymax - pcxHeader.ymin + 1;
	if (w <= 0 || h <= 0 || pcxHeader.bytes_per_line < w)
	{
		Warning(("Invalid PCX dimensions or scanline length\n"));
		return nullptr;
	}

	uint8 type;
	uint16 row;

	switch (pcxHeader.colour_planes)
	{
	case 1:
		type = 2;
		row = pcxHeader.xmax - pcxHeader.xmin + 1;
		break;
	case 3:
		// Dark's 2D blitters do not provide every BMT_FLAT24 source/dest
		// combination used by menus.  Decode true-colour PCX art to the
		// engine's native opaque 16-bit representation.
		type = BMT_FLAT16;
		row = 2 * w;
		break;
	default:
		Warning(("PCX file has %d planes!\n", pcxHeader.colour_planes));
		return nullptr;
	}

	LGALLOC_PUSH_CREDIT();

	auto* pbm = reinterpret_cast<grs_bitmap*>(pResMem->ResMalloc(
		static_cast<unsigned long>(h) * row + sizeof(grs_bitmap)));

	LGALLOC_POP_CREDIT();

	if (!pbm)
	{
		Warning(("Can't allocate bitmap bits in pcx reader\n"));
		return nullptr;
	}

	auto* pImg = reinterpret_cast<uint8*>(&pbm[1]);
	readBuff = static_cast<uint8*>(malloc(0x1000));
	if (!readBuff)
	{
		Warning(("Can't allocate buffer in pcx reader\n"));
		pResMem->ResFree(pbm);
		return nullptr;
	}

	readBuffIndex = 4096;
	readBuffValid = 0;
	readRunRemaining = 0;
	if (!PcxReadBody(pStream, pImg, type, w, h, row, pcxHeader.bytes_per_line))
	{
		Warning(("Unable to read PCX file!\n"));
		free(readBuff);
		pResMem->ResFree(pbm);
		return nullptr;
	}

	free(readBuff);
	gr_init_bitmap(pbm, pImg, type,
		type == BMT_FLAT16 ? BMF_RGB_565 : 0, w, h);

	return pbm;
}

uint8* ResPcxReadPalette(IStoreStream* pStream, IResMemOverride* pResMem)
{
	constexpr auto PalleteSize = 3 * 256;

	PCXHEAD pcxHeader{};
	if (!PcxReadHeader(pStream, &pcxHeader))
		return nullptr;

	if (pcxHeader.colour_planes != 1 && pcxHeader.colour_planes != 3)
		return nullptr;

	LGALLOC_PUSH_CREDIT();

	auto* pPall = reinterpret_cast<uint8*>(pResMem->ResMalloc(PalleteSize));

	LGALLOC_POP_CREDIT();

	if (!pPall)
	{
		Warning(("Can't allocate palette in pcx reader\n"));
		return nullptr;
	}

	// True-colour PCX files have no indexed palette.  Screen modes still bind
	// their background image as a palette resource, though, so provide the
	// same stable fallback used by the true-colour TGA reader.  The palette is
	// irrelevant to the decoded true-colour pixels but keeps a modded menu from
	// turning an optional palette lookup into a fatal resource-load assertion.
	if (pcxHeader.colour_planes == 3)
	{
		for (unsigned i = 0; i < 256; ++i)
		{
			pPall[i * 3] = static_cast<uint8>(i);
			pPall[i * 3 + 1] = static_cast<uint8>(i);
			pPall[i * 3 + 2] = static_cast<uint8>(i);
		}
		return pPall;
	}

	pStream->SetPos(pStream->GetSize() - PalleteSize - 1);

	uint8 pallPresent = 0;
	pStream->Read(1, reinterpret_cast<char*>(&pallPresent));
	if (pallPresent != 12)
	{
		Warning(("Bad PCX trailer: pallette not found\n"));
		pResMem->ResFree(pPall);
		return nullptr;
	}

	pStream->Read(PalleteSize, reinterpret_cast<char*>(pPall));

	return pPall;
}
