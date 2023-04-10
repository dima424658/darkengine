/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////
// $Header: x:/prj/tech/libsrc/namedres/RCS/zipstfct.h 1.3 1998/09/15 18:52:40 JUSTIN Exp $
//
// Zip file Storage Factory.
//

#ifndef _ZIPSTFCT_H
#pragma once
#define _ZIPSTFCT_H

#include <comtools.h>

F_DECLARE_INTERFACE(IStoreFactory);

//
// Use this method to get the storage type; register that with the Resource
// Manager to use that type from here on out.
//
EXTERN IStoreFactory *MakeZipStorageFactory();

#endif

//////////////////////////////////////////////////////////////////////////
