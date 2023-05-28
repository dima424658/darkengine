#pragma once

#include <resbase.h>

enum class ePalKind
{
	kPalBMP = 0x0,
	kPalCEL = 0x1,
	kPalGIF = 0x2,
	kPalPCX = 0x3,
	kPalTGA = 0x4,
};

class cPaletteResource : public cResourceBase<IRes, &IID_IRes>
{
public:
	cPaletteResource(IStore* pStore,
		const char* pName,
		IResType* pType,
		ePalKind kind);

	void* STDMETHODCALLTYPE LoadData(ulong* pSize,
		ulong* pTimestamp,
		IResMemOverride* pResMem) override;

	int STDMETHODCALLTYPE ExtractPartial(const long nStart,
		const long nEnd,
		void* pBuf) override;

	void STDMETHODCALLTYPE ExtractBlocks(void* pBuf,
		const long nSize,
		tResBlockCallback,
		void* pCallbackData) override;

protected:
	ePalKind m_Kind;
};