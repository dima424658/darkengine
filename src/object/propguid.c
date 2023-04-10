/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propguid.c,v 1.23 2000/02/19 12:32:05 toml Exp $
#include <comtools.h>
#include <initguid.h>
#include <propguid.h>
#include <pfaceiid.h>
#include <gunguid.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
//
// THIS FILE JUST EXISTS TO INITIALIZE PROPERTY GUIDS
// IT IS DIFFERENT FROM CAMGUIDS.C SO THAT THE PROPERTY SYSTEM CAN 
// BECOME A LIBRARY SOMEDAY.
//
// NOTE THAT THESE ARE NOT IN A HEADER FILE.  THIS IS TO ENCOURAGE 
// PEOPLE TO USE F_DECLARE_INTERFACE instead.
//


