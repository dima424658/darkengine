#include <algorithm>
#include <cstdio>

#include <sdesc.h>
#include <dbg.h>
#include <mprintf.h>
#include <mxang.h>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

const char* type_strings[] =
{
  "int", // kFieldTypeInt
  "BOOL", // kFieldTypeBool
  "short", // kFieldTypeShort
  "bitfield", // kFieldTypeBits
  "enum", // kFieldTypeEnum
  "char[]", // kFieldTypeString
  "char*", // kFieldTypeStringPtr
  "void*", // kFieldTypeVoidPtr
  "Point", // kFieldTypePoint
  "mxs_vector", // kFieldTypeVector
  "float", // kFieldTypeFloat
  "fix", // kFieldTypeFix
  "fixvec", // kFieldTypeFixVec
  "double", // kFieldTypeDouble
  "rgba", // kFieldTypeRGBA
  "doublevec", // kFieldTypeDoubleVec
  "ang", // kFieldTypeAng
  "angvec" // kFieldTypeAngVec
};

const char* true_names[] = { "t", "y", "true", "yes", "on", nullptr };
const char* false_names[] = { "f", "n", "false", "no", "off", nullptr };

void parse_int(const sFieldDesc* desc, void* val, const char* in)
{
	auto radix = 10;
	if ((desc->flags & 0x40) != 0)
		radix = 16;
	auto ival = strtol(in, 0, radix);
	sd_stuff_from_long(val, ival, desc->size);
}

void unparse_int(const sFieldDesc* desc, const void* val, char* out)
{
	auto ival = sd_cast_to_long(val, desc->size, (desc->flags & 0x20) == 0);
	auto radix = 10;
	if ((desc->flags & 0x40) != 0)
		radix = 16;
	ltoa(ival, out, radix);
}

void parse_bool(const sFieldDesc* desc, void* val, const char* in)
{
	for (auto* name = true_names; *name; ++name)
	{
		if (!strnicmp(in, *name, strlen(*name)))
		{
			sd_stuff_from_long(val, 1, desc->size);
			return;
		}
	}

	for (auto* name = false_names; *name; ++name)
	{
		if (!strnicmp(in, *name, strlen(*name)))
		{
			sd_stuff_from_long(val, 0, desc->size);
			return;
		}
	}

	parse_int(desc, val, in);
}

void unparse_bool(const sFieldDesc* desc, const void* val, char* out)
{
	if (sd_cast_to_long(val, desc->size, 0))
		sprintf(out, "%s", "TRUE");
	else
		sprintf(out, "%s", "FALSE");
}

void parse_bits(const sFieldDesc* desc, void* val, const char* in)
{
	auto ival = 0;
	if (desc->datasize)
	{
		auto strings = (char**)desc->data;
		const char* s;
		for (s = in; isspace(*s); ++s)
			;
		while (*s)
		{
			auto p = strchr(s, ',');
			if (!p)
				p = &s[strlen(s) + 1];
			auto tmp = *p;
			*const_cast<char*>(p) = '\0';

			for (int i = 0; i < desc->datasize; ++i)
			{
				if (!strcmp(s, strings[i]))
				{
					ival |= 1 << (LOBYTE(desc->min) + i);
					break;
				}
			}
			*const_cast<char*>(p) = tmp;
			if (!*p)
				break;
			for (s = p + 1; isspace(*s); ++s)
				;
		}
	}
	else
	{
		ival = strtol(in, 0, 2);
	}

	sd_stuff_from_long(val, ival, desc->size);
}

void unparse_bits(const sFieldDesc* desc, const void* val, char* out)
{
	if ((desc->flags & 0x40) != 0)
	{
		unparse_int(desc, val, out);
	}
	else if (desc->datasize)
	{
		auto ival = sd_cast_to_long(val, desc->size, 0);
		auto comma = 0;
		auto s = out;
		auto strings = (char**)desc->data;
		*out = 0;
		for (int i = 0; i < desc->datasize; ++i)
		{
			if (((1 << (LOBYTE(desc->min) + i)) & ival) != 0)
			{
				if (comma)
				{
					strcpy(s, ",");
					s += strlen(s);
				}
				strcpy(s, strings[i]);
				s += strlen(s);
				comma = 1;
			}
		}
		if (!comma)
			strcpy(out, "[None]");
	}
	else
	{
		auto Value = sd_cast_to_long(val, desc->size, 0);
		if (desc->max)
			Value &= (1 << desc->max) - 1;
		ltoa(Value, out, 2);
	}
}

void parse_enum(const sFieldDesc* desc, void* val, const char* in)
{
	auto vec = (char**)desc->data;
	auto found = false;
	int i;
	for (i = 0; i < desc->datasize; ++i)
	{
		if (!_strcmpi(in, vec[i]))
		{
			found = true;
			i += desc->min;
			break;
		}
	}

	if (!found)
	{
		parse_int(desc, val, in);

		i = std::clamp(static_cast<int>(sd_cast_to_long(val, desc->size, 1)), desc->min, desc->max);
	}

	sd_stuff_from_long(val, i, desc->size);
}

void unparse_enum(const sFieldDesc* desc, const void* val, char* out)
{
	auto ival = sd_cast_to_long(val, desc->size, (desc->flags & 0x20) == 0);
	auto i = std::clamp(static_cast<int>(ival), desc->min, desc->max);

	if (i - desc->min <= desc->datasize)
		strcpy(out, *((const char**)desc->data + i - desc->min));
	else
		unparse_int(desc, val, out);
}

void parse_string(const sFieldDesc* desc, void* val, const char* in)
{
	strncpy(static_cast<char*>(val), in, desc->size);
	static_cast<char*>(val)[desc->size - 1] = '\0';
}

void unparse_string(const sFieldDesc* desc, const void* val, char* out)
{
	strncpy(out, static_cast<const char*>(val), desc->size);
}

void parse_stringptr(const sFieldDesc* desc, void* val, const char* in)
{
	strcpy(*static_cast<char**>(val), in);
}

void unparse_stringptr(const sFieldDesc* desc, const void* val, char* out)
{
	strcpy(out, *static_cast<char* const*>(val));
}

void parse_voidptr(const sFieldDesc* desc, void* val, const char* in)
{
	sscanf(in, "%p", val);
}

void unparse_voidptr(const sFieldDesc* desc, const void* val, char* out)
{
	sprintf(out, "%p", *static_cast<void* const*>(val));
}

void parse_point(const sFieldDesc* desc, void* val, const char* in)
{
	auto idesc = *desc;
	idesc.size >>= 1;

	parse_int(&idesc, val, in);

	auto comma = strchr(in, ',');
	if (comma)
		parse_int(&idesc, (int*)((char*)val + idesc.size), comma + 1);
}

void unparse_point(const sFieldDesc* desc, const void* val, char* out)
{
	auto idesc = *desc;
	idesc.size >>= 1;

	unparse_int(&idesc, val, out);
	strcat(out, ", ");
	unparse_int(&idesc, (int*)((char*)val + idesc.size), &out[strlen(out)]);
}

void parse_vector(const sFieldDesc* desc, void* rawVal, const char* in)
{
	auto* val = reinterpret_cast<float*>(rawVal);
	sscanf(in, "%f, %f, %f", &val[0], &val[1], &val[2]);
}

void unparse_vector(const sFieldDesc* desc, const void* rawVal, char* out)
{
	auto* val = reinterpret_cast<const float*>(rawVal);
	sprintf(out, "%0.2f, %0.2f, %0.2f", val[0], val[1], val[2]);
}

void parse_float(const sFieldDesc* desc, void* val, const char* in)
{
	sscanf(in, "%f", reinterpret_cast<float*>(val));
}

void unparse_float(const sFieldDesc* desc, const void* val, char* out)
{
	sprintf(out, "%0.2f", *reinterpret_cast<const float*>(val));
}

void parse_fix(const sFieldDesc* desc, void* val, const char* in)
{
	float num{};
	sscanf(in, "%f", &num);
	*reinterpret_cast<double*>(val) = num * 65536.0;
}

void unparse_fix(const sFieldDesc* desc, const void* val, char* out)
{
	sprintf(out, "%0.2f", *reinterpret_cast<const double*>(val) / 65536.0);
}

void parse_fixvec(const sFieldDesc* desc, void* rawVal, const char* in)
{
	float vec[3]{};
	auto* val = reinterpret_cast<double*>(rawVal);

	sscanf(in, "%f %f %f", &vec[0], &vec[1], &vec[2]);
	val[0] = vec[0] * 65536.0;
	val[1] = vec[1] * 65536.0;
	val[2] = vec[2] * 65536.0;
}

void unparse_fixvec(const sFieldDesc* desc, const void* rawVal, char* out)
{
	auto* val = reinterpret_cast<const double*>(rawVal);
	sprintf(out, "%0.2f %0.2f %0.2f", val[0] / 65536.0, val[1] / 65536.0, val[2] / 65536.0);
}

void parse_double(const sFieldDesc* desc, void* val, const char* in)
{
	sscanf(in, "%lf", reinterpret_cast<double*>(val));
}

void unparse_double(const sFieldDesc* desc, const void* val, char* out)
{
	sprintf(out, "%0.8lf", *reinterpret_cast<const double*>(val));
}

void parse_rgba(const sFieldDesc* desc, void* rawVal, const char* in)
{
	auto* val = reinterpret_cast<int*>(rawVal);
	sscanf(in, "%d, %d, %d", &val[1], &val[2], &val[3]);
}

void unparse_rgba(const sFieldDesc* desc, const void* rawVal, char* out)
{
	auto* val = reinterpret_cast<const int*>(rawVal);
	sprintf(out, "%d, %d, %d", val[1], val[2], val[3]);
}

void parse_double_vec(const sFieldDesc* desc, void* rawVal, const char* in)
{
	auto* val = reinterpret_cast<double*>(rawVal);
	sscanf(in, "%lf, %lf %lf", &val[0], &val[1], &val[2]);
}

void unparse_double_vec(const sFieldDesc* desc, const void* rawVal, char* out)
{
	auto* val = reinterpret_cast<const double*>(rawVal);
	sprintf(out, "%0.4lf, %0.4lf, %0.4lf", val[0], val[1], val[2]);
}

void parse_ang(const sFieldDesc* desc, void* val, const char* in)
{
	*reinterpret_cast<double*>(val) = floor(atof(in) * 32768.0 / 180.0 + 0.5);
}

void unparse_ang(const sFieldDesc* desc, const void* val, char* out)
{
	sprintf(out, "%0.2f", *reinterpret_cast<const double*>(val) * 180.0 / 32768.0);
}

void parse_ang_vec(const sFieldDesc* desc, void* rawVal, const char* in)
{
	auto* val = reinterpret_cast<mxs_angvec*>(rawVal);

	float vec[3]{};
	sscanf(in, "%lf, %lf %lf", &vec[0], &vec[1], &vec[2]);
	val->tz = floor(vec[0] * 32768.0 / 180.0 + 0.5);
	val->ty = floor(vec[1] * 32768.0 / 180.0 + 0.5);
	val->tx = floor(vec[2] * 32768.0 / 180.0 + 0.5);
}

void unparse_ang_vec(const sFieldDesc* desc, const void* rawVal, char* out)
{
	auto* val = reinterpret_cast<const mxs_angvec*>(rawVal);
	sprintf(out, "%0.2lf, %0.2lf, %0.2lf",
		val->tz * 180.0 / 32768.0,
		val->ty * 180.0 / 32768.0,
		val->tx * 180.0 / 32768.0);
}

void(*parse_funcs[])(const sFieldDesc*, void*, const char*) =
{
  &parse_int,			// kFieldTypeInt
  &parse_bool,			// kFieldTypeBool
  &parse_int,			// kFieldTypeShort
  &parse_bits,			// kFieldTypeBits
  &parse_enum,			// kFieldTypeEnum
  &parse_string,		// kFieldTypeString
  &parse_stringptr,		// kFieldTypeStringPtr
  &parse_voidptr,		// kFieldTypeVoidPtr
  &parse_point,			// kFieldTypePoint
  &parse_vector,		// kFieldTypeVector
  &parse_float,			// kFieldTypeFloat
  &parse_fix,			// kFieldTypeFix
  &parse_fixvec,		// kFieldTypeFixVec
  &parse_double,		// kFieldTypeDouble
  &parse_rgba,			// kFieldTypeRGBA
  &parse_double_vec,	// kFieldTypeDoubleVec
  &parse_ang,			// kFieldTypeAng
  &parse_ang_vec		// kFieldTypeAngVec
};

void(*unparse_funcs[])(const sFieldDesc*, const void*, char*) =
{
  &unparse_int,
  &unparse_bool,
  &unparse_int,
  &unparse_bits,
  &unparse_enum,
  &unparse_string,
  &unparse_stringptr,
  &unparse_voidptr,
  &unparse_point,
  &unparse_vector,
  &unparse_float,
  &unparse_fix,
  &unparse_fixvec,
  &unparse_double,
  &unparse_rgba,
  &unparse_double_vec,
  &unparse_ang,
  &unparse_ang_vec
};

const sFieldDesc* StructDescFindField(const sStructDesc* desc, const char* name)
{
	int which;
	for (which = 0; which < desc->nfields && strnicmp(name, desc->fields[which].name, 0x20u); ++which)
		;

	if (which < desc->nfields)
		return &desc->fields[which];

	Warning(("StructDescFindField: Field %s not found in struct %s\n", name, desc->name));
	return nullptr;
}

BOOL StructFieldToString(const char* struc, const sStructDesc* desc, const sFieldDesc* field, char* dest)
{
	if (!field || !desc || !struc || !dest)
		return FALSE;

	unparse_funcs[field->type](field, &struc[field->offset], dest);

	return TRUE;
}

int StructStringToField(char* struc, const sStructDesc* desc, const sFieldDesc* field, const char* src)
{
	if (!field || !desc || !struc || !src)
		return FALSE;

	parse_funcs[field->type](field, &struc[field->offset], src);

	return TRUE;
}

int StructSetField(char* struc, const sStructDesc* desc, const sFieldDesc* field, void* val)
{
	if (!desc || !struc || !val || !field)
	{
		Warning(("StructSetField: Failed because of NULL pointer parameter\n"));
		return FALSE;
	}

	set_field(struc, val, field);
	return TRUE;
}

int StructGetField(char* struc, const sStructDesc* desc, const sFieldDesc* field, void* val)
{
	if (!desc || !struc || !val || !field)
	{
		Warning(("StructGetField: Failed because of NULL pointer parameter\n"));
		return FALSE;
	}

	get_field(struc, val, field);
	return TRUE;
}

void StructDumpStruct(const char* struc, const sStructDesc* desc)
{
	if (!desc || !struc)
		return;

	mprintf("Struct %s, size %d, flags %x, nfields %d\n", desc->name, desc->size, desc->flags, desc->nfields);

	for (int i = 0; i < desc->nfields; ++i)
	{
		char str[256];

		StructFieldToString(struc, desc, &desc->fields[i], str);
		mprintf(
			"  ->%s (type %s, size %d) = %s\n",
			desc->fields[i].name,
			type_strings[desc->fields[i].type],
			desc->fields[i].size,
			str);
	}
}

BOOL StructDescIsSimple(const sStructDesc* desc)
{
	return desc->nfields <= 1;
}

BOOL StructToSimpleString(const char* struc, const sStructDesc* desc, char* out)
{
	if (desc->nfields == 0)
	{
		strcpy(out, "-");
		return TRUE;
	}
	else if (desc->nfields == 1)
	{
		unparse_funcs[desc->fields->type](desc->fields, (char*)struc + desc->fields->offset, out);
		return TRUE;
	}
	else
	{
		strcpy(out, "...");
		return FALSE;
	}
}

BOOL StructFromSimpleString(char* struc, const sStructDesc* desc, const char* in)
{
	if (!desc->nfields)
		return TRUE;

	if (desc->nfields != 1)
		return FALSE;

	parse_funcs[desc->fields->type](desc->fields, &struc[desc->fields->offset], in);

	return TRUE;
}

BOOL StructToFullString(const char* struc, const sStructDesc* desc, char* out, int len)
{
	bool too_long = false;

	if (desc->nfields)
	{
		auto bufsiz = 256;
		auto* buf = static_cast<char*>(Malloc(0x100));
		auto* p = out;
		if (desc->nfields > 1)
		{
			strcpy(out, "{ ");
			p = &out[strlen(out)];
		}
		for (int i = 0; i < desc->nfields; ++i)
		{
			if (desc->fields[i].size > bufsiz)
			{
				while (bufsiz < desc->fields[i].size)
					bufsiz *= 2;

				buf = static_cast<char*>(Realloc(buf, bufsiz));
			}
			if (i)
			{
				if (&p[strlen("; ") + 1] > &out[len])
				{
					too_long = 1;
					break;
				}
				strcpy(p, "; ");
				p += strlen("; ");
			}
			unparse_funcs[desc->fields[i].type](&desc->fields[i], &struc[desc->fields[i].offset], buf);
			strncpy(p, buf, &out[len] - p);
			if (&p[strlen(buf) + 1] > &out[len])
			{
				too_long = 1;
				break;
			}
			p += strlen(buf);
		}

		Free(buf);

		if (desc->nfields > 1 && !too_long)
		{
			if (p + 2 <= &out[len])
				strcat(out, "}");
			else
				too_long = 1;
		}

		if (too_long)
		{
			auto* elipsis = "...";
			strncpy(&out[len - 1 - strlen(elipsis)], elipsis, strlen(elipsis) + 1);
		}
	}
	else
	{
		strncpy(out, "-", len);
	}

	return TRUE;
}

BOOL StructFromFullString(char* struc, const sStructDesc* desc, const char* in)
{
	if (desc->nfields == 1)
	{
		parse_funcs[desc->fields->type](desc->fields, &struc[desc->fields->offset], in);
	}
	else if (desc->nfields > 1)
	{
		auto* p = strchr(in, 123);
		for (int i = 0; p && i < desc->nfields; ++i)
		{
			const char* pa;
			for (pa = p + 1; isspace(*pa); ++pa)
				;
			auto delim = &pa[strcspn(pa, ";}")];
			auto tmp = *delim;
			*const_cast<char*>(delim) = '\0';
			parse_funcs[desc->fields[i].type](&desc->fields[i], &struc[desc->fields[i].offset], pa);
			*const_cast<char*>(delim) = tmp;
			p = delim;
		}
	}

	return TRUE;
}


void get_field(char* struc, void* newdata, const sFieldDesc* desc)
{
	memcpy(newdata, &struc[desc->offset], desc->size);
}

void set_field(char* struc, void* newdata, const sFieldDesc* desc)
{
	memcpy(&struc[desc->offset], newdata, desc->size);
}

void sd_stuff_from_long(void* val, long in, ulong size)
{
	memcpy(val, &in, size);
}

long sd_cast_to_long(const void* val, ulong size, BOOL is_signed)
{
	auto sz = size;
	if (is_signed)
		sz = size + 4;

	switch (sz)
	{
	case 1:
		return *reinterpret_cast<const uint8*>(val);
	case 2:
		return *reinterpret_cast<const uint16*>(val);
	case 4:
		return *reinterpret_cast<const uint32*>(val);
	case 5:
		return *reinterpret_cast<const int8*>(val);
	case 6:
		return *reinterpret_cast<const int16*>(val);
	case 8:
		return *reinterpret_cast<const int32*>(val);
	default:
		Warning(("Invalid size for integer %d\n", size % 4));
		return 0;
	}
}