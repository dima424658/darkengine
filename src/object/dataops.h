/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/dataops.h,v 1.4 2000/01/29 13:22:58 adurant Exp $
#pragma once

#ifndef __DATAOPS_H
#define __DATAOPS_H

#include <dataopst.h>

////////////////////////////////////////////////////////////
// GENERAL DATA MANIPULATION WRAPPER INTERFACES (C++ only)
//

#undef INTERFACE
#define INTERFACE IDataOps
DECLARE_INTERFACE_(IDataOps, IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   // Construction
   STDMETHOD_(sDatum,New)(THIS) PURE; 
   STDMETHOD_(sDatum,CopyNew)(THIS_ sDatum) PURE; 

   // Destruction 
   STDMETHOD(Delete)(THIS_ sDatum val) PURE; 

   
   // Copy a value, *targ must have come from New()
   STDMETHOD(Copy)(THIS_ sDatum* targ, sDatum src) PURE; 

   // if value is a pointer, return the size of the block it points to 
   // negative if value is a non-pointer, 
   STDMETHOD_(long, BlockSize)(THIS_ sDatum val) PURE; 

   STDMETHOD_(int,Version)(THIS) PURE; 

   // Read a value of the specified version 
   // *dat may NOT have come from New()
   STDMETHOD(Read)(THIS_ sDatum* dat, IDataOpsFile* file, int version) PURE; 

   // Write a value of the current version
   STDMETHOD(Write)(THIS_ sDatum, IDataOpsFile* file) PURE; 

}; 

//------------------------------------------------------------
// IDataOpsFile, simple file interface
//

enum eDataOpSeek
{ 
   kDataOpSeekFromStart, 
   kDataOpSeekFromHere, 
   kDataOpSeekFromEnd 
}; 

#undef INTERFACE  
#define INTERFACE IDataOpsFile
DECLARE_INTERFACE_(IDataOpsFile, IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 
   // returns bytes read/written
   STDMETHOD_(long,Read)(THIS_ void* buf, int len) PURE;   
   STDMETHOD_(long,Write)(THIS_ const void* buf, int len) PURE; 

   STDMETHOD(Seek)(THIS_ eDataOpSeek seek, ulong where) PURE; 
   STDMETHOD_(ulong,Tell)(THIS) PURE; 
}; 

#undef INTERFACE  
#define INTERFACE IDataOpsMemFile
DECLARE_INTERFACE_(IDataOpsMemFile, IDataOpsFile)
{
   // Version of IDataOpsFile that write/read the information to/from a memory
   // buffer instead of a file.  It is for getting the flattened version of some
   // properties in order to send them over the network.

   DECLARE_UNKNOWN_PURE(); 

   // Methods from IDataOpsFile
   STDMETHOD_(long,Read)(THIS_ void* buf, int len) PURE;   
   STDMETHOD_(long,Write)(THIS_ const void* buf, int len) PURE; 
   STDMETHOD(Seek)(THIS_ eDataOpSeek seek, ulong where) PURE; 
   STDMETHOD_(ulong,Tell)(THIS) PURE; 

   // IDataOpsMemFile methods
   // Return pointer to the buffer that was filled by Write() calls.  The pointer may
   // not be valid after subsequent calls to write (the buffer has to grow).
   STDMETHOD_(void *,GetBuffer)(THIS) PURE;
}; 

#undef INTERFACE

#endif // __DATAOPS_H
