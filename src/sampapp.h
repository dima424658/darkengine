/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sampapp.h,v 1.3 2000/01/29 12:41:43 adurant Exp $
#pragma once

#ifndef __SAMPAPP_H
#define __SAMPAPP_H

//
// @INSTRUCTIONS: 1) Rename this function to something appropriate to your system.
//                2) Change MY_CREATEFUNC in sampapp.c to use the same name
//
EXTERN void LGAPI SampSysCreate(void);

//
// @INSTRUCTIONS: 1) Check out camguids.c and grab a GUID (instructions are in that file)
//                2) Name the GUID and put it's definition below.
//                3) Change my_guid in sampapp.c to point to your new GUID.
//
DEFINE_LG_GUID(UUID_SampApp, SOME_NUMERIC_CONSTANT);

//
// KEEP THIS FILE RELATIVELY CLEAR OF JUNK!  DO NOT PUT YOUR SYSTEM'S API HERE! 
// DO NOT INCLUDE OTHER HEADER FILES FROM THIS FILE!
//

#endif // __SAMPAPP_H

