//		LGALT.H		Alternate LG header file for those who don't have
//						the LookingGlass memory allocator & debug system
//
/*
* $Header: r:/prj/lib/src/lg/rcs/lgalt.h 1.3 1993/03/19 18:20:34 rex Exp $
* $Log: lgalt.h $
 * Revision 1.3  1993/03/19  18:20:34  rex
 * Added RCS header
 * 
*/

#ifndef LGALT_H
#define LGALT_H

//	Replaces memory allocator

#define MemSetAllocator(m,r,f)
#define MemPushAllocator(m,r,f)
#define MemPopAllocator()
#define Malloc(size) malloc(size)
#define Realloc(p,size) realloc(p,size)
#define Free(p) free(p)

//	Replaces debug system, don't forget to create Ignore() function
//	if you have Error, WarnUser, Warning, or Spew

#define DBGSRC(bank,bits) (0)
#define DBG(src,statements)
#define DBGBANK(src) (0)
#define Error(msg)
#define WarnUser(msg)
#define Warning(msg)
#define Spew(src,msg)
#define DbgSetLogPath(path)
#define DbgSetLogFile(src,fname)
#define DbgMonoConfig()
#define DbgAddConfigPath(path)
#define DbgLoadConfig(fname)
#define DbgSaveConfig(fname)

#define Exit(msg,errcode) exit(errcode)
#define Exit(f_exit)
#define SetExitMsg(msg)

#endif

