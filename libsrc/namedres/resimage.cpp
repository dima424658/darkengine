#include <resimage.h>
#include <resbastm.h>
#include <tmgr.h>
#include <mode.h>
#include <mdutil.h>

#include <bmp.h>
#include <cel.h>
#include <gif.h>
#include <pcx.h>
#include <tga.h>
#include <bitmap.h>

ulong gResImageFlat16Format = 0;
ushort gResImageChromaKey = 0;

void ImageHackRemap16Bit(grs_bitmap* pBm, uint8 mip);
void ImageHackAutoSetTransparency(grs_bitmap* pBm, uint8 mip);
void FlagsXorMip(grs_bitmap* pBm, int16 flag);
void FlagsOrMip(grs_bitmap* pBm, int16 flag);

cImageResource::cImageResource(IStore* pStore, const char* pName, IResType* pType, eImgKind kind)
	: cResourceBase{ pStore, pName, pType }, m_Kind{ kind } {}

void* cImageResource::LoadData(ulong* pSize, ulong* pTimestamp, IResMemOverride* pResMem)
{
	cAutoResThreadLock lock{};

	if (!pResMem)
		return nullptr;

	auto* pStream = OpenStream();
	if (!pStream)
	{
		CriticalMsg1("Unable to open stream: %s", GetName());
		return nullptr;
	}

	grs_bitmap* bm = nullptr;
	switch (m_Kind)
	{
	case eImgKind::kImgBMP:
		bm = ResBmpReadImage(pStream, pResMem);
		break;
	case eImgKind::kImgCEL:
		bm = ResCelReadImage(pStream, pResMem);
		break;
	case eImgKind::kImgGIF:
		bm = ResGifReadImage(pStream, pResMem);
		break;
	case eImgKind::kImgPCX:
		bm = ResPcxReadImage(pStream, pResMem);
		break;
	case eImgKind::kImgTGA:
		bm = ResTgaReadImage(pStream, pResMem);
		break;
	default:
		break;
	}

	if (pTimestamp)
		*pTimestamp = pStream->LastModified();

	pStream->Close();
	pStream->Release();
	if (!bm)
		return nullptr;

	ImageHackRemap16Bit(bm, 0);
	ImageHackAutoSetTransparency(bm, 0);
	if (pSize)
		*pSize = pResMem->GetSize(bm);

	return bm;
}

BOOL cImageResource::FreeData(void* pUntypedData, ulong nSize, IResMemOverride* pResMem)
{
	cAutoResThreadLock lock{};

	if (!pUntypedData)
		CriticalMsg("FreeData: nothing to free!");

	auto* pData = reinterpret_cast<grs_bitmap*>(pUntypedData);

	if (!m_pStream && (pData->flags & 0x40) && g_tmgr)
		g_tmgr->unload_texture(pData);

	pResMem->ResFree(pData);

	return TRUE;
}

int cImageResource::ExtractPartial(const long nStart, const long nEnd, void* pBuf)
{
	cAutoResThreadLock lock{};
	long nNumToRead = 0;
	long nSize = 0;
	auto bLocked = false;

	if (!pBuf)
		return 0;

	auto* pData = m_pResMan->FindResource(this, &nSize);
	if (!pData || !nSize)
	{
		pData = m_pResMan->LockResource(this);
		if (!pData)
			return 0;

		bLocked = true;

		nSize = m_pResMan->GetResourceSize(this);
	}

	if (nEnd < nSize)
	{
		nNumToRead = nEnd - nStart + 1;
		memmove(pBuf, (char*)pData + nStart, nNumToRead);
	}
	else
	{
		nNumToRead = nSize - nStart;
		memmove(pBuf, (char*)pData + nStart, nSize - nStart);
	}

	if (bLocked)
		m_pResMan->UnlockResource(this);

	return nNumToRead;
}

void cImageResource::ExtractBlocks(void* pBuf, const long nSize, tResBlockCallback callback, void* pCallbackData)
{
	long nRemain = 0, nMove = 0, nIx = 0;
	auto bLocked = false;

	auto* pData = m_pResMan->FindResource(this, &nRemain);

	if (!pData || !nRemain)
	{
		pData = m_pResMan->LockResource(this);
		if (!pData)
			return;

		bLocked = true;

		nRemain = m_pResMan->GetResourceSize(this);
	}

	while (nRemain > 0)
	{
		if (nRemain >= nSize)
		{
			nMove = nSize;
			memmove(pBuf, pData, nSize);
		}
		else
		{
			nMove = nRemain;
			memmove(pBuf, pData, nRemain);
		}

		callback(this, pBuf, nMove, nIx++, pCallbackData);

		pData = static_cast<char*>(pData) + nMove;
		nRemain -= nMove;
	}

	if (bLocked)
		m_pResMan->UnlockResource(this);
}

void ImageHackRemap16Bit(grs_bitmap* pBm, uint8 mip)
{
	if (pBm->type != BMT_FLAT16)
		return;

	auto format16 = gResImageFlat16Format;
	if (!gResImageFlat16Format && grd_mode_info[grd_mode].bitDepth > 8u)
		format16 = grd_mode_info[grd_mode].bitDepth != 15 ? 1024 : 768;
	if (format16 && (pBm->flags & format16 & 0x700) == 0)
	{
		auto* pStart = pBm->bits;
		auto* pBits = (uint16*)&pBm->bits[2 * pBm->h * pBm->w];
		if (mip)
			pBits = (uint16*)&pStart[md_sizeof_mipmap_bits(pBm)];
		if ((pBm->flags & 0x400) != 0)
		{
			while (--pBits >= (uint16*)pStart)
			{
				*pBits = *pBits;
				*pBits = ((uint8)HIBYTE(*pBits) >> 3 << 10) | *pBits & 0x83FF;
				*pBits = (32 * ((((*pBits >> 5) & 0x3F) >> 1) & 0x1F)) | *pBits & 0xFC1F;
			}
		}
		else
		{
			while (--pBits >= (uint16*)pStart)
			{
				*pBits = *pBits;
				*pBits = (((*pBits >> 10) & 0x1F) << 11) | *pBits & 0x7FF;
				*pBits = (32 * ((63 * ((*pBits >> 5) & 0x1F) / 31) & 0x3F)) | *pBits & 0xF81F;
			}
		}
		if (mip)
			FlagsXorMip(pBm, 1792);
		else
			pBm->flags ^= 0x700u;
	}
}

void ImageHackAutoSetTransparency(grs_bitmap* pBm, uint8 mip)
{
	int16 flag = 0;

	if (pBm->type == BMT_FLAT8)
	{
		auto* pBits = &pBm->bits[pBm->h * pBm->w];
		while (--pBits >= pBm->bits)
		{
			if (!*pBits)
			{
				flag = 1;
				break;
			}
		}
	}
	else if (pBm->type == BMT_FLAT16)
	{
		auto* pBits = (uint16*)&pBm->bits[2 * pBm->h * pBm->w];
		while (--pBits >= (uint16*)pBm->bits)
		{
			if (*pBits == gResImageChromaKey)
			{
				flag = 1;
				break;
			}
		}
	}

	if (flag)
	{
		if (mip)
			FlagsOrMip(pBm, flag);
		else
			pBm->flags |= flag;
	}
}

void FlagsXorMip(grs_bitmap* pBm, int16 flag)
{
	while (true)
	{
		pBm->flags ^= flag;
		if (pBm->w == 1 && pBm->h == 1)
			break;
		++pBm;
	}
}

void FlagsOrMip(grs_bitmap* pBm, int16 flag)
{
	while (true)
	{
		pBm->flags |= flag;
		if (pBm->w == 1 && pBm->h == 1)
			break;
		++pBm;
	}
}