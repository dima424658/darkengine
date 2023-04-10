/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/version.c,v 1.24 2000/03/27 12:31:19 adurant Exp $

#include <lgsprntf.h>
#include <version.h>

#include <string.h>
#include <memall.h>
#include <dbmem.h>   // must be last header!

#ifdef PLAYTEST
#define SUFFIX "playtest"
#else
#define SUFFIX ""
#endif

#ifndef SHIP
#ifdef DEBUG
#define FLAVOR "debug"
#else
#define FLAVOR "opt"
#endif
#else
#define FLAVOR ""
#endif

#ifndef SHIP
#ifdef __WATCOMC__
#define COMPILER_NAME "Watcom"
#else
#define COMPILER_NAME "MSVC"
#endif
#else
#define COMPILER_NAME ""
#endif

// For serial stamping, for evil pirates
// can't make it static, cause it whines
char gSerial[32] = "LABEL ME";

// really would like to have the actual compiler and version in here, eh?
#ifndef DEEPC
// Thief 2 version information.
static Version appVersion =
{
   "Final",
   {
      1,    // major
      18,   // minor
   },
   COMPILER_NAME " " FLAVOR " " SUFFIX,  // suffix
};
#else
// Deep Cover version information.
static Version appVersion =
{
   "Demo",
   {
      1,    // major
      12,   // minor
   },
   COMPILER_NAME " " FLAVOR " " SUFFIX,  // suffix
};
#endif

// this returns a pointer to the version structure
const Version* AppVersion(void)
{
   return &appVersion;
}

const char* Version2String(const Version* v)
{
   static char buf[64];

   const char* vn = VersionNum2String(&v->num);
   lg_sprintf(buf,"%s %s %s",v->phase,vn,v->suffix);
   return buf;
}

const char* VersionString(void)
{
   return Version2String(&appVersion);
}

bool VersionsEqual(const Version* v1, const Version* v2)
{
   if (strcmp(v1->phase,v2->phase) != 0)
      return FALSE;
   if (!VersionNumsEqual(&v1->num,&v2->num))
      return FALSE;
   if (strcmp(v1->suffix,v2->suffix) != 0)
      return FALSE;
   return TRUE;
}


