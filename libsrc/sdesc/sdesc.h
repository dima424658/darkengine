#pragma once
#include <sdesbase.h>

const sFieldDesc* StructDescFindField(const sStructDesc* desc, const char* name);

BOOL StructFieldToString(const char* struc, const sStructDesc* desc, const sFieldDesc* field, char* dest);
BOOL StructStringToField(char* struc, const sStructDesc* desc, const sFieldDesc* field, const char* src);

BOOL StructSetField(char* struc, const sStructDesc* desc, const sFieldDesc* field, void* val);
BOOL StructGetField(char* struc, const sStructDesc* desc, const sFieldDesc* field, void* val);

void StructDumpStruct(const char* struc, const sStructDesc* desc);

BOOL StructDescIsSimple(const sStructDesc* desc);
BOOL StructToSimpleString(const char* struc, const sStructDesc* desc, char* out);
BOOL StructFromSimpleString(char* struc, const sStructDesc* desc, const char* in);

BOOL StructToFullString(const char* struc, const sStructDesc* desc, char* out, int len);
BOOL StructFromFullString(char* struc, const sStructDesc* desc, const char* in);

void sd_stuff_from_long(void* val, long in, ulong size);
long sd_cast_to_long(const void* val, ulong size, BOOL is_signed);

void get_field(char* struc, void* newdata, const sFieldDesc* desc);
void set_field(char* struc, void* newdata, const sFieldDesc* desc);