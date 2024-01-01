//		Datapath.H		Data file finder
//		Rex E. Bradford (REX)

/*
* $Header: x:/prj/tech/libsrc/lg/RCS/datapath.h 1.19 1998/05/07 10:05:39 DAVET Exp $
* $Log: datapath.h $
 * Revision 1.19  1998/05/07  10:05:39  DAVET
 * putting back version prior to Justin changes
 * 
 * Revision 1.17  1997/11/25  10:57:55  TOML
 * const-ed arguments
 * 
 * Revision 1.16  1997/09/27  12:21:02  JAEMZ
 * Added length arg to datapathfind
 * 
 * Revision 1.15  1997/09/25  14:44:28  JAEMZ
 * added Datapath Copy
 * 
 * Revision 1.14  1996/12/12  18:12:43  JAEMZ
 * Added datapathFindFlags to allow searching for files, directories, or whatever
 * 
 * Revision 1.13  1996/12/10  14:58:49  JAEMZ
 * Made msvc crunchy
 * 
 * Revision 1.12  1996/12/10  13:53:50  JAEMZ
 * Added dos.h and bunches of datapathdir commands
 * 
 * Revision 1.11  1996/08/08  14:05:07  TOML
 * Added tag to datapath declaration so we can use forward decls
 * 
 * Revision 1.10  1996/01/24  10:48:53  DAVET
 * Added cplusplus stuff
 *
 * Revision 1.9  1994/02/26  16:26:21  dc
 * bump max from 8 to 16
 *
 * Revision 1.8  1993/10/21  15:02:57  rex
 * Modified struct to include 'last' path; added prototype for DatapathLastPath()
 *
 * Revision 1.7  1993/06/17  09:54:27  rex
 * Added DatapathClear() macro and DatapathFree() prototype
 *
 * Revision 1.6  1993/06/15  17:45:49  rex
 * Put parens around pdp in macros
 *
 * Revision 1.5  1993/05/27  11:33:07  rex
 * Added prototype for Greg's DatapathFDOpen() func
 *
 * Revision 1.4  1993/05/12  11:54:25  rex
 * Added prototype for DatapathFind()
 *
 * Revision 1.3  1993/04/22  09:48:11  rex
 * Added <stdlib.h> so getenv() processes correctly.
 *
 * Revision 1.2  1993/03/10  14:43:34  rex
 * Renamed DatapathAdd() to DatapathAddDir()
 * Added new DatapathAdd() to do semicolon-delimited paths
 *
 * Revision 1.1  1993/01/29  09:47:50  rex
 * Initial revision
 *
 *
*/

#ifndef DATAPATH_H
#define DATAPATH_H

#include <stdio.h>
#include <stdlib.h>

#if defined(__WATCOMC__) || defined(__SC__)
// for find_t
#include <dos.h>
#else 
#include <io.h>
#endif

#include <types.h>

#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus

//	The Datapath structure maintains a list of data paths.  It should
//	be cleared to 0 prior to use (static instances are clear at init already).

#define NUM_DATAPATHS 16			   // was 8, which should have been plenty
                                    // however, defdep needs more, for instance
#pragma pack(4)
typedef struct Datapath
{
	uchar numDatapaths;					// # valid paths
	uchar last;								// path of last opened file + 1, 0 = curr
	bool noCurrent;						// if TRUE, don't try current dir
	char *datapath[NUM_DATAPATHS];	// ptrs to paths, malloc'ed
   int   find_flags;                // flags for find
} Datapath;
#pragma pack()

//	DatapathClear() clears a datapath (not needed for static/global ones)
//	DatapathFree() frees all alloc'ed paths and clears a datapath
//	DatapathOpen() tries to open a file, returns file ptr or NULL
//	DatapathFDOpen() tries to open a file, returns file descriptor or -1
//	DatapathFind() finds file, fills in buffer with full pathname
//	DatapathAdd() adds a path (semicolon-delimited) to a data path
//	DatapathAddEnv() adds a path named by an environment variable
//	DatapathAddDir() adds a single directory to a data path
//
//	DatapathTryCurrent() and DatapathNoCurrent() set whether the current
//	directory is tried before directories in the data path.  By default,
//	it is tried.
//
// DatapathFindDir(bool) set whether or not to find directories in all
// the subsequent searches and opens.  Note that msvc has this hardwired
// to TRUE.

#define DatapathClear(pdp) memset(pdp, 0, sizeof(Datapath))
void DatapathFree(Datapath *pdp);

FILE *DatapathOpen(Datapath *pdp, const char *fname, const char *mode);
int DatapathFDOpen(Datapath *pdp, const char *fname, int access, ... );
int DatapathFind(Datapath *pdp, const char *fname, char *buff, int len);
bool DatapathAdd(Datapath *pdp, const char *path);
bool DatapathAddDir(Datapath *pdp, const char *pdir);
const char *DatapathLastPath(Datapath *pdp);

// Copies to dst from src, does not clear or free dst, so beware...
void DatapathCopy(Datapath *dst, const Datapath *src);

#define DatapathAddEnv(pdp,envname) DatapathAdd(pdp,getenv(envname))
#define DatapathTryCurrent(pdp) ((pdp)->noCurrent=0)
#define DatapathNoCurrent(pdp) ((pdp)->noCurrent=1)


// Datapath dir stuff.  This is very similar to 
// opendir and readdir and those.  You can wild
// card search for all the matching files in all
// the paths.  So for instance, you can make lists
// of the files matching some criteria in several
// directories.

// This is a flag to be passed into DatapathOpenDir, it makes
// the system pass over "." and ".." which invariably get found
// when you do a wildcard search in a directory.  Omit this flag
// if you want those found.
#define DP_SCREEN_DOT  1

typedef struct {
   Datapath *dp;
   char path[128];   // name with path and wild cards added
   int curp;         // current datapath entry
   int cur;
#if defined(__WATCOMC__) || defined(__SC__)
	struct find_t find;
#else
	struct _finddata_t find;		// phs, 7/1/96
   long findfp;
#endif
   int flags;      
} DatapathDir;

// Makes a new DatapathDir, initializes it and returns it.  Pass in
// the matching string, ie "*.res", or "regions\*" or whatever, and 
// any flags.  
DatapathDir *DatapathOpenDir(Datapath *dpath,const char *name,int flags);

// Reads the next match and returns the name or passes back NULL if
// done
char *DatapathReadDir(DatapathDir *dpd);

// Frees all the resources, including storage for dpd
void DatapathCloseDir(DatapathDir *dpd);

// Returnscurrently matching name
char *DatapathDirGetName(DatapathDir *dpd);

// stuffs currently path into buffer 
void DatapathDirGetPath(DatapathDir *dpd,char *buffer);

// sets whether or not to find directories
void DatapathFindFlags(Datapath *pdp,bool files,bool dirs);


#ifdef __cplusplus
}
#endif  // cplusplus

#endif

