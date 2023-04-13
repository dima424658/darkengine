/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/sdesc/RCS/sdesbase.h 1.13 1999/04/21 14:39:45 JAEMZ Exp $

#ifndef SDESBASE_H
#define SDESBASE_H
#include <sdestype.h>

//
// Generic structure description macros used when actually declaring a
// structure description
//

// expands to the actual field in a struct
#define FieldSelect(type, field)    (((type*)NULL)->field)

// expands to the size of a field in a struct
#define FieldSize(type, field)      (sizeof(FieldSelect(type, field)))

// expands to the byte offset of a field in a struct
#define FieldOffset(type, field)    ((ulong) &FieldSelect(type, field))

// expands to the size and the offset of a field in a struct
#define FieldLocation(type, field)  FieldSize(type, field), FieldOffset(type, field)

// for naming enums or bitfields, full sets the constraints, normal lets you do it
#define FieldNames(names)           sizeof(names)/sizeof(names[0]), sizeof(names)/sizeof(names[0]), names
#define FullFieldNames(names)       0, 0, FieldNames(names)

// expands to an entire sStructDesc struct
#define StructDescBuild(type, flags, fields)    \
      { #type, sizeof(type), flags, sizeof(fields)/sizeof(fields[0]), fields }

//
// Generic structure description enums
//

// sFieldDesc flags
enum _eFieldFlags
{
   kFieldFlagNone       = (0<<0),   // no flags (avoid warning)
   kFieldFlagInvisible  = (1<<0),   // is this field invisible to the user?
   kFieldFlagNotEdit    = (1<<1),   // is this field not editable by the user?
   kFieldFlagIrrelevant = (1<<2),   // is this field irrelevant?
   kFieldFlagUIRanged   = (1<<3),   // is the value ranged?
   kFieldFlagUIWrap     = (1<<4),   // does the value wrap around at the ends?
   kFieldFlagUnsigned   = (1<<5),   // is this field unsigned?
   kFieldFlagHex        = (1<<6),   // is this field displayed as hex?
};

// sStructDesc flags
enum _eStructFlags
{
   kStructFlagNone      = (0<<0),   // no flags (avoid warning)
   kStructFlagInvisible = (1<<0),   // is this struct invisible to the user?
   kStructFlagNotEdit   = (1<<1),   // is this struct not editable by the user?
};

// atomic field types      // Type              
enum _eFieldType
{
   kFieldTypeInt,          // signed integer
   kFieldTypeBool,         // boolean
   kFieldTypeShort,        // signed short
   kFieldTypeBits,         // bitmask (max = first bit, min = last bit, data = array of string names of bits) 
   kFieldTypeEnum,         // enum (actually integer)
   kFieldTypeString,       // character string
   kFieldTypeStringPtr,    // character pointer
   kFieldTypeVoidPtr,      // void pointer
   kFieldTypePoint,        // struct Point (x,y)
   kFieldTypeVector,       // struct mxs_vector (x,y,z)
   kFieldTypeFloat,        // float
   kFieldTypeFix,          // fix (integer interpreted as 16.16)
   kFieldTypeFixVec,       // vector of fixes
   kFieldTypeDouble,       // double-precision float
   kFieldTypeRGBA,         // rgba color sRGBA from srgba.h
   kFieldTypeDoubleVec,    // double precision vector
   kFieldTypeAng,          // lg angle (0-0xFFFF), shown as degrees
   kFieldTypeAngVec,       // lg angle vector, shown as degrees heading, pitch, bank (tz,ty,tx)
};

//
// Generic structure description structures
//


// the actual field description
struct sFieldDesc
{
   char        name[32];      // name of field
   eFieldType  type;          // type of field
   ulong       size;          // size in bytes of field
   ulong       offset;        // offset in bytes of field
   eFieldFlags flags;         // field flags
   int         min;           // min value if ranged
   int         max;           // max value if ranged
   ulong       datasize;      // number of data elements
   void*       data;          // actual data elements (could be enum names
                              // or whatever)
};

// the actual structure description
struct sStructDesc
{
   char           name[32];   // name of struct
   ulong          size;       // size in bytes of struct
   eStructFlags   flags;      // struct flags
   int            nfields;    // number of fields in struct
   sFieldDesc*    fields;     // actual field descriptions
};

// Build a complete structdesc for a single-field data type
#define StructDescDefineSingleton(varname,type,field,flags) \
sFieldDesc varname##_fields = { "", field, sizeof(type), 0, flags }; \
sStructDesc varname = { #type, sizeof(type), kStructFlagNone, 1, &(varname##_fields) } 


#endif   // SDESBASE_H






