#include <algorithm>

#include <lg.h>
#include <vernum.h>
#include <tagfile_.h>

#include <config.h>
#include <cfgdbg.h>

// implement hash sets
#include <hshsttem.h>

// Must be last header
#include <dbmem.h>

//
// TagFileVersion
// 

static constexpr TagVersion MyVersion = { 0, 1 };

static constexpr uchar Deadbeef[] = { 0xDE, 0xAD, 0xBE, 0xEF };

//
// Table
//

tHashSetKey TagFileTable::GetKey(tHashSetNode node) const
{
	auto e = reinterpret_cast<TagTableEntry*>(node);

	return reinterpret_cast<tHashSetKey>(&e->key);
}

TagFileTable::~TagFileTable()
{
	tHashSetHandle handle;

	for (auto e = GetFirst(handle); e != nullptr; e = GetNext(handle))
		delete e;
}

template<typename T>
static void write(FILE* file, T&& x)
{
	Verify(fwrite(&(x), sizeof(x), 1, file) == 1);
}

void TagFileTable::Write(FILE* file)
{
	auto items = GetCount();
	write(file, items);

	tHashSetHandle idx;
	for (auto e = GetFirst(idx); e != nullptr; e = GetNext(idx))
		write(file, *e);
}

template<typename T>
static void read(FILE* file, T&& x)
{
	Verify(fread(&(x), sizeof(x), 1, file) == 1);
}

void TagFileTable::Read(FILE* file)
{
	decltype(GetCount()) items{};
	read(file, items);

	for (decltype(items) i = 0; i < items; ++i)
	{
		auto e = new TagTableEntry{};
		read(file, *e);

		Insert(e);
	}
}

//
// Base tag file class
//

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(TagFileBase, ITagFile);

TagFileBase::~TagFileBase()
{
	AssertMsg(block == nullptr, "Tag file closing with a block still open");
	if (file != nullptr)
		fclose(file);
	file = nullptr;
}

ITagFile* TagFileBase::Open(const char* filename, TagFileOpenMode mode)
{
	TagFileBase* result;

	switch (mode)
	{
	case TagFileOpenMode::kTagOpenRead:
		result = new TagFileRead{ filename };
		break;
	case TagFileOpenMode::kTagOpenWrite:
		result = new TagFileWrite{ filename };
		break;
	default:
		return nullptr;
	}

	if (result->file == nullptr)
	{
		delete result;
		result = nullptr;
	}

	return result;
}

STDMETHODIMP_(const TagVersion*) TagFileBase::GetVersion()
{
	return &MyVersion;
}

STDMETHODIMP_(ulong) TagFileBase::BlockSize(const TagFileTag* tag)
{
	auto e = table.Search(tag);
	if (e)
		return e->size;
	else
		return -1;
}

STDMETHODIMP_(const TagFileTag*) TagFileBase::CurrentBlock()
{
	if (block == nullptr)
		return nullptr;

	return &block->key;
}

STDMETHODIMP_(HRESULT) TagFileBase::Seek(ulong offset, TagFileSeekMode mode)
{
	if (block == nullptr)
	{
		Warning(("Seeking when no block is open\n"));
		return E_FAIL;
	}

	auto check = PrepSeek(offset, mode);
	// if out of bounds, fail.
	if (check < 0 || (check > 0 && OpenMode() == TagFileOpenMode::kTagOpenRead))
	{
		Warning(("Seeking outside of tag file block\n"));
		return E_FAIL;
	}

	// @TBD: change logic so that SEEK_CUR is preserved for performance
	fseek(file, offset, SEEK_SET);
	blockptr = offset - block->Start();

	if (OpenMode() == TagFileOpenMode::kTagOpenWrite)
		block->size = std::max(block->size, static_cast<long>(blockptr));

	return S_OK;
}

STDMETHODIMP_(ulong) TagFileBase::Tell()
{
	if (block == nullptr)
		return 0;

	return blockptr;
}

STDMETHODIMP_(ulong) TagFileBase::TellFromEnd()
{
	if (block == nullptr)
		return 0;

	return block->size - blockptr;
}


//------------------------------------------------------------
// Iteration
//

class cTagIter : public ITagFileIter
{
public:
	cTagIter(TagFileTable& t) : Entry{ nullptr }, Table{ t } { }
	virtual ~cTagIter() {};

	DECLARE_UNAGGREGATABLE();

	STDMETHOD(Start)() override
	{
		Entry = Table.GetFirst(Idx);
		return S_OK;
	}

	STDMETHOD_(BOOL, Done)() override
	{
		return Entry == nullptr;
	}

	STDMETHOD(Next)() override
	{
		Entry = Table.GetNext(Idx);
		return S_OK;
	}

	STDMETHOD_(const TagFileTag*, Tag)() override
	{
		if (Entry)
			return &Entry->key;
		else
			return nullptr;
	}

private:
	tHashSetHandle Idx;
	TagFileTable& Table;
	const TagTableEntry* Entry;
};

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cTagIter, ITagFileIter);

STDMETHODIMP_(ITagFileIter*) TagFileBase::Iterate()
{
	return new cTagIter(table);
}

void TagFileBase::SetCurBlock(const TagFileTag* tag)
{
	if (tag == nullptr)
		block = nullptr;
	else
		block = table.Search(tag);
}

int TagFileBase::PrepSeek(ulong& offset, TagFileSeekMode mode)
{
	if (block == nullptr)
		return 0;

	// convert offset to absolute
	switch (mode)
	{
	case kTagSeekFromStart:
		offset += block->Start();
		break;
	case kTagSeekFromHere:
		offset += ftell(file);
		break;
	case kTagSeekFromEnd:
		offset += block->End();
		break;
	}

	if (offset < block->Start())
		return block->offset - block->Start();
	else if (offset > block->End())
		return offset - block->End();

	return 0;
}

//
// WRITE TAG FILE CLASS
//

TagFileWrite::TagFileWrite(const char* fn) : TagFileBase{fn, "wb"}
{
	if (file)
	{
		TagFileHeader header{};
		write(file, header);
	}
	else
	{
		Warning(("TagFileWrite: opening %s for writing failed\n", fn));
	}
}

TagFileWrite::~TagFileWrite()
{
	if (file != nullptr)
	{
		TagFileHeader header;

		fseek(file, 0, SEEK_END); // seek to end 
		header.table = ftell(file);

		table.Write(file); // write out the table

		fseek(file, 0, SEEK_SET); // seek to front

		// fill out header
		header.version = MyVersion;
		memcpy(header.deadbeef, Deadbeef, sizeof(header.deadbeef));

		// write out actual data in header
		write(file, header);
	}
}

STDMETHODIMP_(TagFileOpenMode) TagFileWrite::OpenMode()
{
	return TagFileOpenMode::kTagOpenWrite;
}

STDMETHODIMP_(HRESULT) TagFileWrite::OpenBlock(const TagFileTag* tag, TagVersion* version)
{
	if (file == nullptr)
		return E_FAIL;

	if (block != nullptr)
	{
		Warning(("Opening Tag Block with a block already open\n"));
		return E_FAIL;
	}

	AssertMsg1(table.Search(tag) == nullptr, "TagFileWrite::NewBlock(): tag %s is already in use", tag->label);

	TagFileBlockHeader header{};
	header.tag = *tag;
	header.version = *version;

	// put the block at the end of the file!
	fseek(file, 0, SEEK_END);

	// store offset and tag in table
	auto entry = new TagTableEntry(*tag, ftell(file));
	auto result = table.Insert(entry);
	if (!result)
		return E_FAIL;

	SetCurBlock(tag);

	int len = fwrite(&header, 1, sizeof(header), file);
	if (len != sizeof(header))
	{
		Warning(("TagFileWrite::NewBlock(): wrote only %d out of %d bytes\n", len, sizeof(header)));
		return E_FAIL;
	}

	blockptr = 0;
	return S_OK;
}

STDMETHODIMP_(HRESULT) TagFileWrite::CloseBlock()
{
	if (block == nullptr)
	{
		Warning(("TagFileWrite::CloseBlock() No Block to Close\n"));
		return E_FAIL;
	}

	SetCurBlock(nullptr);
	return S_OK;
}

STDMETHODIMP_(long) TagFileWrite::Read(char*, int)
{
	Warning(("Reading a TagFile opened for writing\n"));
	return -1;
}

STDMETHODIMP_(long) TagFileWrite::Write(const char* buf, int buflen)
{
	AssertMsg(block != nullptr, "TagFileWrite::Write(): No block has been started\n");
	if (block == nullptr)
		return -1;

	if (file == nullptr)
		return -1;

	auto len = fwrite(buf, 1, buflen, file);
	blockptr += len;

	block->size = std::max(static_cast<long>(blockptr), block->size);

	return len;
}

STDMETHODIMP_(long) TagFileWrite::Move(char* buf, int buflen)
{
	return Write(buf, buflen);
}

// READ TAG FILE CLASS
// 

TagFileRead::TagFileRead(const char* fn)
	: TagFileBase{fn, "rb"}
{
	if (file == nullptr)
	{
		ConfigSpew("TagFileTrace", ("TagFileRead: opening %s for reading failed\n", fn));
		return;
	}
	// read in header
	TagFileHeader header{};
	read(file, header);

	// verify header.
	if (memcmp(header.deadbeef, Deadbeef, sizeof(header.deadbeef)) != 0)
	{
		Warning(("Tag file %s is corrupt!\n", fn));
		fclose(file);
		file = nullptr;
		return;
	}

	// warn about version
	int delta = VersionNumsCompare(&MyVersion, &header.version);
	if (delta)
	{
#ifdef DBG_ON
		char buf[256];
		strcpy(buf, VersionNum2String(&MyVersion));
		Warning(("Tag file %s is not current version.\nOld: %s, New: %s\n", fn, VersionNum2String(&header.version), buf));
#endif 
		// if it's NEWER, then discard
		if (delta < 0)
		{
			Warning(("Version in file is new.  Discarding tag\n"));
			fclose(file);
			file = nullptr;
			return;
		}
	}

	// read in tag table
	fseek(file, header.table, SEEK_SET);
	table.Read(file);
}

TagFileRead::~TagFileRead()
{
}

STDMETHODIMP_(TagFileOpenMode) TagFileRead::OpenMode()
{
	return TagFileOpenMode::kTagOpenRead;
}

STDMETHODIMP_(HRESULT) TagFileRead::OpenBlock(const TagFileTag* tag, TagVersion* version)
{
	if (file == nullptr)
		return E_FAIL;

	SetCurBlock(tag);

	if (block == nullptr)
	{
		ConfigSpew("tagfile_spew", ("Tag %s not in tag file\n", tag->label));
		return E_FAIL;
	}
	fseek(file, block->offset, SEEK_SET);

	TagFileBlockHeader header{};
	auto len = fread(&header, 1, sizeof(header), file);
	if (len != sizeof(header))
	{
		Warning(("TagFileRead::OpenBlock(): read only %d of %d bytes\n", len, sizeof(header)));
		SetCurBlock(nullptr);
		return E_FAIL;
	}

	// compare versons
	auto delta = VersionNumsCompare(version, &header.version);
	if (delta != 0)
	{
#ifdef DBG_ON
		char buf[256];
		std::strncpy(buf, VersionNum2String(version), std::size(buf));
		ConfigSpew("tagfile_spew", ("Tag data %s is not current version.\nOld: %s, New: %s\n",
			tag->label, VersionNum2String(&header.version), buf));
#endif 

		// if it's NEWER, then discard
		if (delta < 0)
		{
			ConfigSpew("tagfile_spew", ("Version in file is newer.  Discarding tag\n"));
			SetCurBlock(nullptr);
			return E_FAIL;
		}
	}

	*version = header.version;
	blockptr = 0;

	return S_OK;
}

STDMETHODIMP_(HRESULT) TagFileRead::CloseBlock()
{
	if (block == nullptr)
	{
		Warning(("TagFileRead::CloseBlock() No Block to Close\n"));
		return E_FAIL;
	}

	SetCurBlock(nullptr);
	return S_OK;
}

STDMETHODIMP_(long) TagFileRead::Read(char* buf, int buflen)
{
	if (file == nullptr)
		return -1;

	if (block == nullptr)
	{
		Warning(("BufTagFileRead::Read() called before ::Seek()\n"));
		return -1;
	}

	ulong bytesleft = block->size - blockptr; // TODO
	if (buflen > bytesleft)
		buflen = std::max(bytesleft, 0ul);

	auto len = fread(buf, 1, buflen, file);
	blockptr += len;

	return len;
}

STDMETHODIMP_(long) TagFileRead::Write(const char* buf, int buflen)
{
	Warning(("Writing to a tagfile opened for reading\n"));
	return -1;
}

STDMETHODIMP_(long) TagFileRead::Move(char* buf, int buflen)
{
	return Read(buf, buflen);
}

ITagFile* TagFileOpen(const char* filename, TagFileOpenMode mode)
{
	return TagFileBase::Open(filename, mode);
}