///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/hashfns.h $
// $Author: TOML $
// $Date: 1998/02/24 12:55:06 $
// $Revision: 1.7 $
//
// Stock hash functions, using good hash algorithm
//

#ifndef __HASHFNS_H
#define __HASHFNS_H

//
// Hash a string
//
EXTERN unsigned LGAPI HashString(const char *);

//
// Hash a string in a case-insensitive way
//
EXTERN unsigned LGAPI HashStringCaseless(const char *);

//
// Hash a GUID
//
typedef struct _GUID GUID;

EXTERN unsigned LGAPI HashGUID(const GUID *);

//
// Hash a pointer
//
EXTERN unsigned LGAPI HashPtr(const void *);

//
// Hash a long
//
#define HashLong(i) HashPtr((void *)(i))

//
// Other integer variants
//
#define HashIntegerValue(i) HashPtr((void *)(i))

//
// Hash an thing of the given size
//
EXTERN unsigned LGAPI HashThing(const void *, unsigned size);

#endif /* !__HASHFNS_H */
