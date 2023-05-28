#include <algorithm>

#include <sdestool.h>
#include <sdesc.h>
#include <aggmemb.h>
#include <appagg.h>

#include <hashpp.h>
#include <hshpptem.h>

#include <cfgdbg.h>

class cSdescTools : public IStructDescTools
{
	DECLARE_AGGREGATION(cSdescTools);

public:
	cSdescTools(IUnknown* pOuter);

	//
	// Lookup a field by name
	//
	STDMETHOD_(const sFieldDesc*, GetFieldNamed)(const sStructDesc* desc, const char* field) override
	{
		return StructDescFindField(desc, field);
	}

	//
	// Parse and unparse a field
	//
	STDMETHOD(ParseField)(const sFieldDesc* fdesc, const char* string, void* struc) override
	{
		return StructStringToField(reinterpret_cast<char*>(struc), reinterpret_cast<const sStructDesc*>(1), fdesc, string) == FALSE ? S_OK : S_FALSE;
	}
	STDMETHOD(UnparseField)(const sFieldDesc* fdesc, const void* struc, char* string, int buflen) override
	{
		auto size = std::max(std::max(1024, static_cast<int>(fdesc->size)), buflen);

		auto* buf = new char[size] {};
		auto retval = S_FALSE;
		if (StructFieldToString(reinterpret_cast<const char*>(struc), reinterpret_cast<const sStructDesc*>(1), fdesc, buf))
		{
			strncpy(string, buf, buflen);
			string[buflen - 1] = '\0';
			retval = S_OK;
		}

		delete[] buf;

		return retval;
	}

	// 
	// Parse & Unparse a "simple" structure (e.g. a one-field structure)
	//
	STDMETHOD_(BOOL, IsSimple)(const sStructDesc* desc) override
	{
		return StructDescIsSimple(desc);
	}
	STDMETHOD_(BOOL, ParseSimple)(const sStructDesc* desc, const char* string, void* struc) override
	{
		return StructFromSimpleString(reinterpret_cast<char*>(struc), desc, string);
	}
	STDMETHOD_(BOOL, UnparseSimple)(const sStructDesc* desc, const void* struc, char* out, int len) override
	{
		auto* buf = new char[std::max(len, 1024)];
		auto retval = StructToSimpleString(reinterpret_cast<const char*>(struc), desc, buf);

		strncpy(out, buf, len);
		out[len - 1] = '\0';

		delete[] buf;

		return retval;
	}


	//
	// Parse & unparse the "full" representation of a structure
	//

	STDMETHOD_(BOOL, ParseFull)(const sStructDesc* sdesc, const char* string, void* struc) override
	{
		return StructFromFullString(reinterpret_cast<char*>(struc), sdesc, string);
	}
	STDMETHOD_(BOOL, UnparseFull)(const sStructDesc* sdesc, const void* struc, char* out, int buflen) override
	{
		return StructToFullString(reinterpret_cast<const char*>(struc), sdesc, out, buflen);
	}

	//
	// Dump a struct to the mono 
	//
	STDMETHOD(Dump)(const sStructDesc* sdesc, const void* struc) override
	{
		StructDumpStruct(reinterpret_cast<const char*>(struc), sdesc);
		return S_OK;
	}

	//
	// Set and Get an integral field
	// 
	STDMETHOD(SetIntegral)(const sFieldDesc* fdesc, long value, void* struc) override
	{
		sd_stuff_from_long(&reinterpret_cast<char*>(struc)[fdesc->offset], value, fdesc->size);
		return S_OK;
	}
	STDMETHOD(GetIntegral)(const sFieldDesc* fdesc, const void* struc, long* value) override
	{
		*value = sd_cast_to_long(&reinterpret_cast<const char*>(struc)[fdesc->offset], fdesc->size, (fdesc->flags & 0x20) == 0);
		return S_OK;
	}

	//
	// Lookup an sdesc in the registry.  NULL if none exists
	//
	STDMETHOD_(const sStructDesc*, Lookup)(const char* name) override
	{
		return mRegistry.Search(name);
	}
	STDMETHOD(Register)(const sStructDesc* desc) override
	{
		if (mRegistry.HasKey(desc->name))
			ConfigSpew("sdesctab", ("A struct desc has already been registered for %s\n", desc->name));

		mRegistry.Set(desc->name, desc);

		return S_OK;
	}
	STDMETHOD(ClearRegistry)() override
	{
		mRegistry.Clear();

		return S_OK;
	}

protected:
	//
	// Aggregate protocol
	//

	HRESULT Init()
	{
		return S_OK;
	}
	HRESULT End()
	{
		ClearRegistry();
		return S_OK;
	}

private:
	cHashTable<const char*, const sStructDesc*, cHashFunctions> mRegistry;
};

IMPLEMENT_AGGREGATION_SELF_DELETE(cSdescTools);

static sRelativeConstraint Constraints[] =
{
   { kNullConstraint }
};

cSdescTools::cSdescTools(IUnknown* pOuter)
{
	INIT_AGGREGATION_1(pOuter, IID_IStructDescTools, this, kPriorityNormal, Constraints);
}

void SdescToolsCreate(void)
{
	AutoAppIPtr(Unknown);
	new cSdescTools{ pUnknown };
}