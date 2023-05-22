#include <algorithm>


#include <lg.h>
#include <vernum.h>
#include <memtfile.h>
#include <memtfil_.h>

#include <config.h>
#include <cfgdbg.h>

// implement hash sets
#include <hshsttem.h>

// Must be last header
#include <dbmem.h>

//
// Memory Tag File Iterator
//

class cMemFileIter : public ITagFileIter
{
public:
	cMemFileIter(cMemTagTable* table) : mpTable{ table }, mpEntry{ nullptr }, mHandle{} { }
	virtual ~cMemFileIter() = default;

	DECLARE_UNAGGREGATABLE();

	STDMETHOD(Start)() override
	{
		mpEntry = mpTable->GetFirst(mHandle);
		return S_OK;
	}

	STDMETHOD_(BOOL, Done)() override
	{
		return mpEntry == nullptr;
	}

	STDMETHOD(Next)()
	{
		if (mpEntry)
			mpEntry = mpTable->GetNext(mHandle);
		return S_OK;
	}

	STDMETHOD_(const TagFileTag*, Tag)(THIS) override
	{
		if (mpEntry)
			return &mpEntry->tag;
		else
			return nullptr;
	}

protected:
	tHashSetHandle mHandle;
	cMemTagTable* mpTable;
	const sMemTagEntry* mpEntry;
};

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cMemFileIter, ITagFileIter);

//
// Memory Tag File Table
//

cMemTagTable::~cMemTagTable()
{
	// TODO
}

sMemTagEntry* cMemTagTable::AddTag(const TagFileTag* tag, const TagVersion* version) // line 32
{
	auto entry = new sMemTagEntry{};

	entry->tag = *tag;
	entry->version = *version;
	entry->block_size = 0;
	entry->buf_size = kDefaultBufSize;
	entry->buf = new uchar[entry->buf_size];

	Insert(entry);

	return entry;
}

//
// Tag File Base 
//

STDMETHODIMP_(ulong) cMemFile::BlockSize(const TagFileTag* tag)
{
	auto entry = mpTable->Search(tag->label);
	if (entry)
		return entry->block_size;
	else
		return 0;
}

STDMETHODIMP_(const TagFileTag*) cMemFile::CurrentBlock()
{
	if (mpCurBlock)
		return &mpCurBlock->tag;
	else
		return nullptr;
}

STDMETHODIMP_(HRESULT) cMemFile::Seek(ulong offset, TagFileSeekMode from)
{
	if (!mpCurBlock)
		return E_FAIL;

	if (from == TagFileSeekMode::kTagSeekFromHere)
		mCursor += offset;
	else if (from == TagFileSeekMode::kTagSeekFromEnd)
		mCursor = mpCurBlock->block_size + mCursor;

	return S_OK;
}

STDMETHODIMP_(ulong) cMemFile::Tell()
{
	return mCursor;
}

STDMETHODIMP_(ulong) cMemFile::TellFromEnd()
{
	return mpCurBlock->block_size - mCursor;
}

STDMETHODIMP_(ITagFileIter*) cMemFile::Iterate()
{
	return new cMemFileIter{ mpTable };
}

//
// Read/write tag file class
//

STDMETHODIMP_(HRESULT) cMemFileReadWrite::OpenBlock(const TagFileTag* tag, TagVersion* version)
{
	auto entry = mpTable->Search(tag->label);

	if (this->mMode == TagFileOpenMode::kTagOpenWrite && entry == nullptr)
		entry = mpTable->AddTag(tag, version);
	else if (mMode == TagFileOpenMode::kTagOpenRead && entry != nullptr)
		*version = entry->version;
	else
		return E_FAIL;

	mpCurBlock = entry;
	mCursor = 0;

	return S_OK;
}

STDMETHODIMP_(HRESULT) cMemFileReadWrite::CloseBlock()
{
	if (mpCurBlock)
	{
		mpCurBlock = nullptr;
		return S_OK;
	}

	return E_FAIL;
}

STDMETHODIMP_(long) cMemFileReadWrite::Read(char* buf, int buflen)
{
	Assert_(mMode == TagFileOpenMode::kTagOpenRead);

	if (!mpCurBlock)
		return 0;

	if (mCursor + buflen > mpCurBlock->block_size)
		buflen = mpCurBlock->block_size - mCursor;

	std::copy(buf, buf + buflen, &mpCurBlock->buf[mCursor]);

	mCursor += buflen;
	return buflen;
}

STDMETHODIMP_(long) cMemFileReadWrite::Write(const char* buf, int buflen)
{
	Assert_(mMode == TagFileOpenMode::kTagOpenWrite);

	if (!mpCurBlock)
		return 0;

	if (mCursor + buflen > mpCurBlock->buf_size)
	{
		while (mCursor + buflen > mpCurBlock->buf_size)
			mpCurBlock->buf_size *= 2;

		mpCurBlock->buf = reinterpret_cast<uchar*>(Realloc(mpCurBlock->buf, mpCurBlock->buf_size));
	}

	std::copy(buf, buf + buflen, &mpCurBlock->buf[mCursor]);

	mCursor += buflen;

	if (mCursor > mpCurBlock->block_size)
		mpCurBlock->block_size = mCursor;

	return buflen;
}

STDMETHODIMP_(long) cMemFileReadWrite::Move(char* buf, int buflen)
{
	if (mMode == TagFileOpenMode::kTagOpenRead)
		return Read(buf, buflen);
	else if (mMode == TagFileOpenMode::kTagOpenWrite)
		return Write(buf, buflen);
	else
		return 0; // TODO
}

class cMemFileFactory : public cTagFileFactory
{
public:
	cMemFileFactory() : mTable{} { }

	ITagFile* Open(const char* filename, TagFileOpenMode mode) override
	{
		return new cMemFileReadWrite{ &mTable, mode };
	}

protected:
	cMemTagTable mTable;
};

cTagFileFactory* CreateMemoryTagFileFactory()
{
	return new cMemFileFactory{};
}