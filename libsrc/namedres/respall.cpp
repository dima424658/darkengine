#include <respall.h>
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

cPaletteResource::cPaletteResource(IStore* pStore, const char* pName, IResType* pType, ePalKind kind)
	: cResourceBase{ pStore, pName, pType }, m_Kind{ kind } {}

void* cPaletteResource::LoadData(ulong* pSize, ulong* pTimestamp, IResMemOverride* pResMem)
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

	void* pData = nullptr;
	switch (m_Kind)
	{
	case ePalKind::kPalBMP:
		pData = ResBmpReadPalette(pStream, pResMem);
		break;
	case ePalKind::kPalCEL:
		pData = ResCelReadPalette(pStream, pResMem);
		break;
	case ePalKind::kPalGIF:
		pData = ResGifReadPalette(pStream, pResMem);
		break;
	case ePalKind::kPalPCX:
		pData = ResPcxReadPalette(pStream, pResMem);
		break;
	case ePalKind::kPalTGA:
		pData = ResTgaReadPalette(pStream, pResMem);
		break;
	default:
		break;
	}

	if (pTimestamp)
		*pTimestamp = pStream->LastModified();

	pStream->Close();
	pStream->Release();

	if (pSize)
		*pSize = pResMem->GetSize(pData);

	return pData;
}

int cPaletteResource::ExtractPartial(const long nStart, const long nEnd, void* pBuf)
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

void cPaletteResource::ExtractBlocks(void* pBuf, const long nSize, tResBlockCallback callback, void* pCallbackData)
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