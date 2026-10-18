#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>

#include <lg.h>
#include <vernum.h>
#include <tagfile_.h>
#include <dataguard.h>

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

static TagFileBlockObserver gBlockObserver = nullptr;
static void* gBlockObserverContext = nullptr;
static TagFileErrorObserver gErrorObserver = nullptr;
static void* gErrorObserverContext = nullptr;

void TagFileSetBlockObserver(TagFileBlockObserver observer, void* context)
{
	gBlockObserver = observer;
	gBlockObserverContext = context;
}

void TagFileSetErrorObserver(TagFileErrorObserver observer, void* context)
{
	gErrorObserver = observer;
	gErrorObserverContext = context;
}

void TagFileNotifyError(ITagFile* file, const TagFileTag* tag,
	const char* operation, const char* detail)
{
	if (gErrorObserver != nullptr)
		gErrorObserver(file, tag, operation, detail, gErrorObserverContext);
}

template<typename T>
static bool read_exact(FILE* file, T& value)
{
	return fread(&value, sizeof(value), 1, file) == 1;
}

void TagFileNotifyBlockObserver(ITagFile* file, const TagFileTag* tag,
	const TagVersion* version, ulong size, ulong consumed, BOOL closing)
{
	if (gBlockObserver != nullptr)
		gBlockObserver(file, tag, version, size, consumed, closing,
			gBlockObserverContext);
}

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

bool TagFileTable::Read(FILE* file, uint64_t fileSize, uint64_t tableOffset)
{
	decltype(GetCount()) items{};
	if (!read_exact(file, items))
		return false;

	uint64_t directoryBytes = 0;
	uint64_t directoryStart = 0;
	if (!DarkCheckedAddU64(tableOffset, sizeof(items), &directoryStart) ||
		items > 1024U * 1024U ||
		!DarkCheckedMulU64(items, sizeof(TagTableEntry), &directoryBytes) ||
		!DarkRangeWithinU64(directoryStart, directoryBytes, fileSize))
		return false;

	for (decltype(items) i = 0; i < items; ++i)
	{
		auto e = new TagTableEntry{};
		if (!read_exact(file, *e) ||
			!DarkBufferHasNull(e->key.label,sizeof(e->key.label)) || e->size < 0 ||
			e->offset < sizeof(TagFileHeader) ||
			!DarkRangeWithinU64(e->offset, sizeof(TagFileBlockHeader), tableOffset) ||
			!DarkRangeWithinU64(e->Start(), static_cast<uint64_t>(e->size), tableOffset) ||
			Search(&e->key) != nullptr)
		{
			delete e;
			return false;
		}

		const int64_t directoryPosition = DarkFileTell64(file);
		TagFileBlockHeader header{};
		if (directoryPosition < 0 || DarkFileSeek64(file, e->offset, SEEK_SET) != 0 ||
			!read_exact(file, header) ||
			!DarkBufferHasNull(header.tag.label,sizeof(header.tag.label)) ||
			std::memcmp(&header.tag, &e->key, sizeof(e->key)) != 0 ||
			DarkFileSeek64(file, directoryPosition, SEEK_SET) != 0)
		{
			delete e;
			return false;
		}

		if (!Insert(e))
		{
			delete e;
			return false;
		}
	}
	return true;
}

//
// Base tag file class
//

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(TagFileBase, ITagFile);

TagFileBase::~TagFileBase()
{
	if (block != nullptr)
	{
		Warning(("Tag file closing with block %s still open\n", block->key.label));
		SetCurBlock(nullptr);
	}
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

	uint64_t base = 0;
	switch (mode)
	{
	case kTagSeekFromStart: base = block->Start(); break;
	case kTagSeekFromHere:
	{
		const int64_t current = DarkFileTell64(file);
		if (current < 0) return E_FAIL;
		base = static_cast<uint64_t>(current);
		break;
	}
	case kTagSeekFromEnd: base = block->End(); break;
	default: return E_FAIL;
	}
	uint64_t absolute = 0;
	if (!DarkCheckedAddU64(base, offset, &absolute) ||
		absolute < block->Start() ||
		(OpenMode() == TagFileOpenMode::kTagOpenRead && absolute > block->End()) ||
		absolute > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
	{
		Warning(("Seeking outside of tag file block\n"));
		return E_FAIL;
	}

	if (DarkFileSeek64(file, static_cast<int64_t>(absolute), SEEK_SET) != 0)
		return E_FAIL;
	blockptr = static_cast<ulong>(absolute - block->Start());

	if (OpenMode() == TagFileOpenMode::kTagOpenWrite)
		block->size = std::max(block->size, static_cast<slong>(blockptr));

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

	return blockptr <= static_cast<ulong>(block->size) ?
		static_cast<ulong>(block->size) - blockptr : 0;
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

	size_t len = fwrite(&header, 1, sizeof(header), file);
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
	blockptr += static_cast<ulong>(len);

	block->size = std::max(static_cast<slong>(blockptr), block->size);

	return static_cast<long>(len);
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
	if (DarkFileSeek64(file, 0, SEEK_END) != 0)
	{
		TagFileNotifyError(this, nullptr, "open", "could not determine file size");
		fclose(file); file = nullptr; return;
	}
	const int64_t physicalSize = DarkFileTell64(file);
	if (physicalSize < static_cast<int64_t>(sizeof(TagFileHeader)) ||
		DarkFileSeek64(file, 0, SEEK_SET) != 0)
	{
		TagFileNotifyError(this, nullptr, "header", "file is shorter than the tag-file header");
		fclose(file); file = nullptr; return;
	}
	fileSize = static_cast<uint64_t>(physicalSize);

	// read in header
	TagFileHeader header{};
	if (!read_exact(file, header))
	{
		TagFileNotifyError(this, nullptr, "header", "truncated tag-file header");
		fclose(file); file = nullptr; return;
	}

	// verify header.
	if (memcmp(header.deadbeef, Deadbeef, sizeof(header.deadbeef)) != 0)
	{
		Warning(("Tag file %s is corrupt!\n", fn));
		TagFileNotifyError(this, nullptr, "header", "invalid tag-file signature");
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
		// Do not discard a newer container. Individual block codecs decide
		// compatibility after the complete directory has been inventoried.
	}

	// Read and validate the complete directory before any codec sees it.
	if (header.table < sizeof(TagFileHeader) ||
		!DarkRangeWithinU64(header.table, sizeof(ulong), fileSize) ||
		DarkFileSeek64(file, header.table, SEEK_SET) != 0 ||
		!table.Read(file, fileSize, header.table))
	{
		TagFileNotifyError(this, nullptr, "directory",
			"invalid count, duplicate tag, block range, or block header");
		fclose(file); file = nullptr; return;
	}
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
	if (DarkFileSeek64(file, block->offset, SEEK_SET) != 0)
	{
		TagFileNotifyError(this, tag, "open block", "block offset is not seekable");
		SetCurBlock(nullptr);
		return E_FAIL;
	}

	TagFileBlockHeader header{};
	auto len = fread(&header, 1, sizeof(header), file);
	if (len != sizeof(header))
	{
		Warning(("TagFileRead::OpenBlock(): read only %d of %d bytes\n", len, sizeof(header)));
		TagFileNotifyError(this, tag, "open block", "truncated block header");
		SetCurBlock(nullptr);
		return E_FAIL;
	}
	if (!DarkBufferHasNull(header.tag.label,sizeof(header.tag.label)) ||
		std::memcmp(&header.tag, &block->key, sizeof(header.tag)) != 0)
	{
		TagFileNotifyError(this, tag, "open block", "block header disagrees with directory");
		SetCurBlock(nullptr);
		return E_FAIL;
	}

	// The caller supplies its read ceiling in *version.  Inventory callers use
	// UINT_MAX.UINT_MAX, while real codecs pass the newest version they own.
	auto delta = VersionNumsCompare(version, &header.version);
	if (delta != 0)
	{
#ifdef DBG_ON
		char buf[256];
		std::strncpy(buf, VersionNum2String(version), std::size(buf));
		ConfigSpew("tagfile_spew", ("Tag data %s is not current version.\nOld: %s, New: %s\n",
			tag->label, VersionNum2String(&header.version), buf));
#endif 
		if (delta < 0)
		{
			ConfigSpew("tagfile_spew", ("Version in file is newer than the codec read ceiling.\n"));
			SetCurBlock(nullptr);
			return E_FAIL;
		}
	}

	*version = header.version;
	blockVersion = header.version;
	blockptr = 0;
	TagFileNotifyBlockObserver(this, &block->key, &blockVersion,
		static_cast<ulong>(block->size), 0, FALSE);

	return S_OK;
}

STDMETHODIMP_(HRESULT) TagFileRead::CloseBlock()
{
	if (block == nullptr)
	{
		Warning(("TagFileRead::CloseBlock() No Block to Close\n"));
		return E_FAIL;
	}

	TagFileNotifyBlockObserver(this, &block->key, &blockVersion,
		static_cast<ulong>(block->size), blockptr, TRUE);
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

	if (buflen < 0)
		return -1;
	if (blockptr > static_cast<ulong>(block->size))
	{
		TagFileNotifyError(this, &block->key, "read", "block cursor is outside the validated range");
		return -1;
	}
	ulong bytesleft = static_cast<ulong>(block->size) - blockptr;
	if (static_cast<ulong>(buflen) > bytesleft)
		buflen = static_cast<int>(bytesleft);

	auto len = fread(buf, 1, buflen, file);
	blockptr += static_cast<ulong>(len);
	if (len != static_cast<size_t>(buflen))
		TagFileNotifyError(this, &block->key, "read", "physical file ended inside a block");

	return static_cast<long>(len);
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
