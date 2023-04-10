/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////
//
// $Header: x:/prj/tech/libsrc/namedres/rcs/resistr.h 1.1 1998/07/23 11:57:44 JUSTIN Exp $
//
// String resource interface
//
// String resources are a specialized version of the basic IRes. They
// contain a table of named strings -- from the name, you can get the
// full string. This is necessary for internationalization: you load
// the string resource appropriate to your locale, then do lookups to
// get the right string for each circumstance.
//
// String resources provide extra methods for locking and unlocking
// (that is, looking up) specific strings in the table. The entire
// table can be passed around as an IRes, but you will generally get
// at the data through the String methods.
//

#ifndef __RESISTR_H
#define __RESISTR_H

#include <lg.h>
#include <comtools.h>
#include <resapi.h>

F_DECLARE_INTERFACE(IStringRes);

// The maximum length of the name of a string:
#define STRINGRES_MAXNAMELEN 64

DECLARE_INTERFACE_(IStringRes, IRes)
{
   //
   // Try to make sure that a given string is loaded into memory. The actual
   // semantics here are fairly implementation-specific.
   //
   STDMETHOD_(void, StringPreload)(THIS_ const char *pStrName) PURE;

   //
   // Lock and unlock a specific string in the table.
   //
   // You pass in the "name" of the string, which is a fairly arbitrary
   // word that identifies this string in the table. If that named string
   // exists, the full string is returned. It is valid until it is
   // Unlocked. The returned string is a normal, null-delimited char *.
   //
   // While any single string in the table is Locked, the entire table is
   // Locked. Thus, subsequent Locks are likely to be relatively fast, once
   // any string is Locked. To guarantee speed, you can Lock the full table;
   // this ensures that the table is loaded and ready.
   //
   STDMETHOD_(char *, StringLock)(THIS_ const char *pStrName) PURE;
   STDMETHOD_(void, StringUnlock)(THIS_ const char *pStrName) PURE;

   //
   // Extract a string into the given buffer.
   //
   // This places the text of the named string into the given buffer,
   // without Locking. Memory management of the buffer is entirely up
   // to the caller. nSize should be the size of the buffer; if the string
   // is longer than the buffer, it will be truncated.
   //
   // Returns TRUE iff the named string existed; otherwise, it returns
   // FALSE and makes the given buffer hold the empty string.
   //
   STDMETHOD_(BOOL, StringExtract)(THIS_ 
                                   const char *pStrName,
                                   char *pBuf,
                                   int nSize) PURE;
};

//////////
//
// C Accessor Macros
//

#define IStringRes_StringLock(p, a)       COMCall1(p, StringLock, a)
#define IStringRes_StringUnlock(p, a)     COMCall1(p, StringUnlock, a)
#define IStringRes_StringExtract(p, a, b) COMCall2(p, StringExtract, a, b)

#endif // !__RESISTR_H
