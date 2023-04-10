/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/sdesc.h,v 1.8 2000/01/29 13:24:58 adurant Exp $
#pragma once

#ifndef SDESC_H
#define SDESC_H

#include <comtools.h>
#include <sdestype.h>
#include <sdestool.h>
#include <appagg.h>

//
// THE SDESC API HAS MOVED TO SDESTOOL.H!  
// 
// This macro exists for back-compatibility, since most people only just register structs. 
//


#define StructDescRegister(str) do { \
   IStructDescTools* tools = AppGetObj(IStructDescTools); \
   COMCall1(tools,Register,str); \
   SafeRelease(tools); \
   } while (0)

#endif   // SDESC_H





