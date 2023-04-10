/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/////////////////////////////////////////////////////////////////////////////
// $Header: x:/prj/tech/libsrc/namedres/rcs/datasrc.h 1.1 1998/07/30 07:35:50 JUSTIN Exp $
//
// Generic Data Source API
//
// This interface should be used for any sort of generic thing from which
// you obtain data blocks. By using this Lock/Unlock paradigm, we keep some
// flexibility to handle memory as appropriate, without worrying about
// the underlying implementation.
//

#ifndef __DATASRC_H
#define __DATASRC_H

#include <lg.h>
#include <comtools.h>

#pragma once
#pragma pack(4)

F_DECLARE_INTERFACE(IDataSource);

#undef INTERFACE
#define INTERFACE IDataSource

DECLARE_INTERFACE_(IDataSource, IUnknown)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

   //
   // Gets the data from this source, and increments the lock count.
   //
   STDMETHOD_(void *, Lock)(THIS) PURE;

   //
   // Decrement lock count
   //
   STDMETHOD_(void, Unlock)(THIS) PURE;
};

//////////
//
// C Accessor Macros
//

#define IDataSource_Lock(p)          COMCall0(p, Lock)
#define IDataSource_Unlock(p)        COMCall0(p, Unlock)

#pragma pack()

#endif // !__DATASRC_H
