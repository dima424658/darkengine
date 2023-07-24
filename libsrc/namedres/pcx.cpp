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

static int16 readBuffIndex;
static uint8* readBuff;

bool PcxReadRow(IStoreStream* pStream, uint8* p, int16 npixels);
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

bool PcxReadRow(IStoreStream* pStream, uint8* p, int16 npixels)
{
	auto* pstart = p;
	auto* pend = &p[npixels];
	while (p < pend)
	{
		if (readBuffIndex >= 4096)
		{
			pStream->Read(4096, reinterpret_cast<char*>(readBuff));
			readBuffIndex = 0;
		}

		auto value = readBuff[readBuffIndex++];
		if ((value & 0xC0) == 0xC0)
		{
			auto count = value & 0x3F;
			if (readBuffIndex >= 4096)
			{
				pStream->Read(4096, reinterpret_cast<char*>(readBuff));
				readBuffIndex = 0;
			}

			auto valuea = readBuff[readBuffIndex++];
			if (&p[count] > pend)
				CriticalMsg1("PCX decompression error, xpixel: %d\n", p - pstart);

			memset(p, valuea, count);
			p += count;
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

	uint8* buff24 = nullptr;
	if (type == 5)
	{
		buff24 = static_cast<uint8*>(malloc(3 * bpl));
		if (!buff24)
		{
			Warning(("PCX reader can't alloc memory\n"));
			return false;
		}
	}

	for (int y = 0; y < h; ++y)
	{
		if (type == 2)
		{
			if (!PcxReadRow(pStream, bits, bpl))
			{
				Warning(("PCX decompression error on line: %d\n", y));
				return false;
			}
		}
		else if (type == 5)
		{
			for (int plane = 0; plane < 3; ++plane)
			{
				if (!PcxReadRow(pStream, &buff24[w * plane], bpl))
				{
					Warning(("PCX decompression error on line: %d\n", y));
					return false;
				}
			}

			auto pd = bits;
			for (int i = 0; i < w; ++i)
			{
				*pd = buff24[i];
				auto pda = pd + 1;
				*pda++ = buff24[i + w];
				*pda = buff24[2 * w + i];
				pd = pda + 1;
				++i;
			}
		}
		else
		{
			Warning(("PCX file has %d planes!\n", type));
			return false;
		}

		bits += row;
	}

	if (type == 5)
		free(buff24);

	return true;
}


grs_bitmap* ResPcxReadImage(IStoreStream* pStream, IResMemOverride* pResMem)
{
	PCXHEAD pcxHeader{};
	if (!PcxReadHeader(pStream, &pcxHeader))
		return nullptr;

	auto w = pcxHeader.xmax - pcxHeader.xmin + 1;
	auto h = pcxHeader.ymax - pcxHeader.ymin + 1;

	uint8 type;
	uint16 row;
	int extra;

	switch (pcxHeader.colour_planes)
	{
	case 1:
		type = 2;
		row = pcxHeader.xmax - pcxHeader.xmin + 1;
		extra = pcxHeader.bytes_per_line - (uint16)w;
		break;
	case 3:
		type = 5;
		row = 3 * w;
		extra = 3 * pcxHeader.bytes_per_line - (uint16)(3 * w);
		break;
	default:
		Warning(("PCX file has %d planes!\n", pcxHeader.colour_planes));
		return nullptr;
	}

	LGALLOC_PUSH_CREDIT();

	auto* pbm = reinterpret_cast<grs_bitmap*>(pResMem->ResMalloc(h * row + extra + sizeof(grs_bitmap)));

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
	if (!PcxReadBody(pStream, pImg, type, w, h, row, pcxHeader.bytes_per_line))
	{
		Warning(("Unable to read PCX file!\n"));
		free(readBuff);
		pResMem->ResFree(pbm);
		return nullptr;
	}

	free(readBuff);
	gr_init_bitmap(pbm, pImg, type, 0, w, h);

	return pbm;
}

uint8* ResPcxReadPalette(IStoreStream* pStream, IResMemOverride* pResMem)
{
	constexpr auto PalleteSize = 3 * 256;

	PCXHEAD pcxHeader{};
	if (!PcxReadHeader(pStream, &pcxHeader))
		return nullptr;

	if (pcxHeader.colour_planes != 1)
		return nullptr;

	LGALLOC_PUSH_CREDIT();

	auto* pPall = reinterpret_cast<uint8*>(pResMem->ResMalloc(PalleteSize));

	LGALLOC_POP_CREDIT();

	if (!pPall)
	{
		Warning(("Can't allocate palette in pcx reader\n"));
		return nullptr;
	}

	pStream->SetPos(pStream->GetSize() - PalleteSize - 1);

	uint8 pallPresent = 0;
	pStream->Read(1, reinterpret_cast<char*>(&pallPresent));
	if (pallPresent != 12)
	{
		Warning(("Bad PCX trailer: pallette not found\n"));
		return nullptr;
	}

	pStream->Read(PalleteSize, reinterpret_cast<char*>(pPall));

	return pPall;
}