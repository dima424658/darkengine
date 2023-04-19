/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////
// $Header: x:/prj/tech/libsrc/res2/RCS/storcnst.h 1.1 1998/05/21 17:21:04 JUSTIN Exp $
//
// Common storage-related constants
//

#ifndef _STORCNST_H
#pragma once
#define _STORCNST_H

//
// The maximum length of a storage path. Implementations of IStore should
// make an effort to support at least this path length.
//
#define MAX_STOREPATH 512

//
// The maximum length of the name of a component in a storage path. Again,
// implementations should support this to some degree, even if their formats
// do not permit such long names -- substorages in other formats might
// allow longer ones.
//
#define MAX_STORENAME 32

//
// The maximum length of the extension to a component name.
//
#define MAX_EXTENSION 20

#endif
