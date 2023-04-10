/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/lresname.h,v 1.10 2000/01/29 13:21:13 adurant Exp $
// protos for the local hacked resname stupidity
#pragma once

#ifndef __LRESNAME_H
#define __LRESNAME_H

// basic type of resname handles
typedef int rn_hnd;
#define LRES_NO_HND (-1)

// a resname setup - currently 20 bytes, defined in lress.h
typedef struct _rn_name rn_name;

// if you want to actually do type stuff, you need lress.h, but if not just use this
#define RN_ALL      (0x1f)   // @TODO: hateful secret knowledge, must be fixed

// for doing callbacks on each found resource, return whether to keep it
// if you change chunk, rn stores yours, frees its own copy, and sets flags right
typedef BOOL (*RN_ScanCallback)(char *name, rn_hnd handle, int type, char *user_data, void **chunk);

// horrible palette hacks for trying to do 16 bit right...
// just stuff this... woo-woo, so on, so forth... bitmap is really a grs_bitmap.. neat, eh?
EXTERN void (*pResnamePaletteCallback)(uchar *pal, void *bitmap);

////////////////////
// ways to look up data from lresname

// actually get what is there
EXTERN rn_hnd   ResNameLookupName(char *name);

//  NOTE: though this takes a rn_name, it assumes where is NO_WHERE - since where is a lres internal
EXTERN rn_hnd   ResNameLookupTokenandName(rn_name *rname);

// get the full rn_name for this handle
EXTERN rn_name *ResNameGetNameData(rn_hnd handle);

// get the actual void * data chunk for this handle
EXTERN void    *ResNameGetbyHandle(rn_hnd handle);

// get size only works on resname memory managed by resname, if you callback
// modified it, it will return 0, sorry
EXTERN int      ResNameGetSize(rn_hnd handle);

// get or set the user data associated with a given handle
EXTERN char     ResNameGetUserData(rn_hnd handle);
EXTERN void     ResNameSetUserData(rn_hnd handle, char user_data);

// returns FALSE if these tokens are not around
EXTERN BOOL     ResNameBuildTokens(char tokens[], char *str);


//////////////////////////
// system level interface

// init/free for the system, size is number of resname handles per alloc
EXTERN BOOL ResNameInit(int size);
EXTERN BOOL ResNameShutdown(void);

// open adds a place to the possible location of res'
EXTERN BOOL ResNameOpen(char *base);

// with sets a base to add to any names passed in
EXTERN BOOL ResNameWith(char *with);

//////////////////////////
// actual handle manipulation, res load/free 

// register yourself as a "user" of this res, tell it to load its info
EXTERN BOOL ResNameLock(rn_hnd hnd);
EXTERN BOOL ResNameUnlock(rn_hnd hnd);

#define ResNameNameLock(name)   ResNameLock(ResNameLookupName(name))
#define ResNameNameUnlock(name) ResNameUnlock(ResNameLookupName(name))

// for all load calls, the "Simple" flavor assumes any type and no callback

// The basic Load call takes a path and a actual file/res name to load in
EXTERN rn_hnd ResNameLoad(char *with, char *name, int type, RN_ScanCallback cback);
#define ResNameLoadSimple(with,name) ResNameLoad(with,name,RN_ALL,NULL)

// The ScanLoad calls are used when you want to pass in a directory
//   @TODO: currently no way to pass a wildcard in as part of this, sadly
//          clearly a reasonably high priority
EXTERN int  ResNameScanLoad(char *with, int type, RN_ScanCallback cback);
#define ResNameScanLoadSimple(with) ResNameScanFancy(with,RN_ALL,NULL)

// free this handle
EXTERN BOOL ResNameFreeHandle(rn_hnd handle);


//////////////////////////
// zany swizzle/deswizzle copy to file ready memory block stuff

// build a diskwriteable block , which is Malloced by lres (lresdisk, in particular)
EXTERN void *ResNameBuildBlock(int *handles, int cnt);

// read in and deswizzle a formerly built block
// it is your job to make sure handles is big enough (you can call ResNameBlockHndCnt)
// fills handles with the new dynamic handles
EXTERN BOOL ResNameParseBlock(void *datablock, int *handles, RN_ScanCallback cback);

// for users finding out about block contents
#define ResNameBlockSize(blockptr)   (*(((int *)blockptr)+0))
#define ResNameBlockHndCnt(blockptr) (*(((int *)blockptr)+1))

#endif  //  __LRESNAME_H
