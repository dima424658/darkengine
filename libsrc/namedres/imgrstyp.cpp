#include <array>

#include <imgrstyp.h>
#include <resapi.h>
#include <resbastm.h>
#include <aggmemb.h>
#include <dbmem.h>

static const char *extMaps[] =
{
	".pcx",
	".tga",
	".cel",
	".gif",
	".bmp"
};

class cImageResource : public cResourceBase<IRes, &IID_IRes>
{
public:
	cImageResource(IStore* pStore,
				   const char* pName,
				   IResType* pType,
				   const char* pExt);

	void *STDMETHODCALLTYPE LoadData(ulong *pSize,
									 ulong *pTimestamp,
									 IResMemOverride *pResMem) override;

	BOOL STDMETHODCALLTYPE FreeData(void *pData,
									ulong nSize,
									IResMemOverride *pResMem) override;

	int STDMETHODCALLTYPE ExtractPartial(const long nStart,
										 const long nEnd,
										 void *pBuf) override;

	void STDMETHODCALLTYPE ExtractBlocks(void *pBuf,
										 const long nSize,
										 tResBlockCallback,
										 void *pCallbackData) override;

protected:
	const char* m_pExt;
};

cImageResource::cImageResource(IStore* pStore, const char* pName, IResType* pType, const char* pExt)
	: cResourceBase(pStore, pName, pType), m_pExt{ pExt } {}

void* cImageResource::LoadData(ulong* pSize, ulong* pTimestamp, IResMemOverride* pResMem)
{
	// TODO
	return nullptr;
}

BOOL cImageResource::FreeData(void* pData, ulong nSize, IResMemOverride* pResMem)
{
	// TODO
	return 0;
}

int cImageResource::ExtractPartial(const long nStart, const long nEnd, void* pBuf)
{
	// TODO
	return 0;
}

void cImageResource::ExtractBlocks(void* pBuf, const long nSize, tResBlockCallback, void* pCallbackData)
{
	// TODO
}

class cImageResourceType : public cCTDelegating<IResType>,
						   public cCTAggregateMemberControl<kCTU_Default>
{
public:
	// Get the name of this type. This is an arbitrary static string,
	// which should ideally be unique.
	const char *STDMETHODCALLTYPE GetName() override;

	// Get the extensions that this type supports. This will call the callback
	// once for each supported extension.
	void STDMETHODCALLTYPE EnumerateExts(tResEnumExtsCallback callback, void *pClientData) override;

	// Return TRUE iff the given extension is legal for this type. Arguably
	// redundant with EnumerateExts, but it is useful for efficiency reasons
	// to have both.
	BOOL STDMETHODCALLTYPE IsLegalExt(const char *pExt) override;

	// Create a new resource of this type from the specified IStore, with
	// the given name. If ppResMem is non-null, it gives the memory
	// manager for this data. Returns NULL iff it can't create the IRes
	// for some reason. It should make sure that the resource knows its
	// name and storage, from the params.
	//
	// Note that this does not actually read any data in, or even open
	// up the stream in the IStore; it just sets things up so that later
	// operations can do so.
	IRes *STDMETHODCALLTYPE CreateRes(
		IStore *pStore,
		const char *pName,
		const char *pExt,
		IResMemOverride **ppResMem) override;
};

const char *STDMETHODCALLTYPE cImageResourceType::GetName()
{
	return RESTYPE_IMAGE;
}

void STDMETHODCALLTYPE cImageResourceType::EnumerateExts(tResEnumExtsCallback callback, void *pClientData)
{
	for (int i = 0; i < std::size(extMaps); ++i)
		callback(extMaps[i], this, pClientData);
}

BOOL STDMETHODCALLTYPE cImageResourceType::IsLegalExt(const char *pExt)
{
	for (int i = 0; i < std::size(extMaps); ++i)
		if (stricmp(pExt, extMaps[i]) == 0)
			return 1;
	return 0;
}

IRes *STDMETHODCALLTYPE cImageResourceType::CreateRes(IStore *pStore, const char *pName, const char *pExt, IResMemOverride **ppResMem)
{
	if(!IsLegalExt(pExt))
	{
    	Warning(("Invalid extension %s given to create image resource!", pExt));
		return nullptr;
	}

	return new cImageResource(pStore, pName, this, pExt);
}

EXTERN IResType *MakeImageResourceType()
{
	return new cImageResourceType();
}