#pragma once

#include <resbase.h>

enum class eImgKind
{
	kImgBMP,
	kImgCEL,
	kImgGIF,
	kImgPCX,
	kImgTGA
};

class cImageResource : public cResourceBase<IRes, &IID_IRes>
{
public:
	cImageResource(IStore* pStore,
		const char* pName,
		IResType* pType,
		eImgKind kind);

	void* STDMETHODCALLTYPE LoadData(ulong* pSize,
		ulong* pTimestamp,
		IResMemOverride* pResMem) override;

	BOOL STDMETHODCALLTYPE FreeData(void* pData,
		ulong nSize,
		IResMemOverride* pResMem) override;

	int STDMETHODCALLTYPE ExtractPartial(const long nStart,
		const long nEnd,
		void* pBuf) override;

	void STDMETHODCALLTYPE ExtractBlocks(void* pBuf,
		const long nSize,
		tResBlockCallback,
		void* pCallbackData) override;

protected:
	eImgKind m_Kind;
};