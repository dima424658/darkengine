//		Datapath.C		Searches for files in data path
//		Rex E. Bradford (REX)
//
/*
* $Header: x:/prj/tech/libsrc/lg/RCS/datapath.cpp 1.22 1998/05/07 10:05:36 DAVET Exp $
* $Log: datapath.cpp $
 * Revision 1.22  1998/05/07  10:05:36  DAVET
 * putting back version prior to Justin changes
 * 
 * Revision 1.20  1997/11/25  10:57:44  TOML
 * const-ed arguments
 * 
 * Revision 1.19  1997/11/25  10:35:11  TOML
 * C++ compile
 * 
 * Revision 1.18  1997/09/27  12:20:52  JAEMZ
 * Added length arg to datapathfind
 * 
 * Revision 1.17  1997/09/25  14:44:34  JAEMZ
 * added Datapath Copy
 * 
 * Revision 1.16  1997/08/24  17:53:45  NJT
 * balance _findfirst with _closefirst in DatapathFind to avoid windows
 * reboot after program terminates
 * 
 * Revision 1.15  1996/12/12  18:12:25  JAEMZ
 * Added datapathFindFlags to allow searching for files, directories, or whatever
 * 
 * Revision 1.14  1996/12/10  13:24:47  JAEMZ
 * Changed "" includes to <> includes, cause watcom's weird
 * 
 * Revision 1.13  1996/10/21  09:33:23  TOML
 * msvc
 * 
 * Revision 1.12  1996/10/10  11:20:41  TOML
 * msvc
 *
 * Revision 1.11  1995/09/27  16:36:39  mahk
 * Added '+' as a path separator.
 *
 * Revision 1.10  1993/10/21  15:03:30  rex
 * Added DatapathLastPath() routine and update of 'last' var.
 *
 * Revision 1.9  1993/06/17  09:54:44  rex
 * Added DatapathFree() function
 *
 * Revision 1.8  1993/05/27  11:45:05  rex
 * Fixed bug in DatapathFDOpen()
 *
 * Revision 1.7  1993/05/27  11:32:51  rex
 * Added Greg's DatapathFDOpen() func
 *
 * Revision 1.6  1993/05/12  11:54:39  rex
 * Added DatapathFind() function
 *
 * Revision 1.5  1993/03/11  14:39:53  rex
 * Fixed bug in semicolon-delimited paths in DatapathAdd().
 *
 * Revision 1.4  1993/03/10  16:17:32  rex
 * Took out printf() that I put in for debugging--whoops
 *
 * Revision 1.3  1993/03/10  15:49:31  rex
 * Renamed DatapathAdd() to DatapathAddDir()
 * Added DatapathAdd() to add a semicolon-delimited path
 *
 * Revision 1.2  1993/01/29  10:03:20  rex
 * Changed to Malloc() from malloc()
 *
 * Revision 1.1  1993/01/29  09:47:55  rex
 * Initial revision
 *
*/

#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <dos.h>
#include <io.h>
#include <lg.h>

#include <datapath.h>
#include <memall.h>

#define min(a, b) ((a < b) ? a : b)

// Use this for watcom
#if defined(__WATCOMC__) || defined(__SC__)
EXTERN int _dp_find_flags_tab[4] = {_A_NORMAL,0,_A_SUBDIR|_A_NORMAL,_A_SUBDIR};
#endif


//	-------------------------------------------------------------
//
//	DatapathFree() frees all paths and clears a datapath.
//
//		pdp = ptr to datapath

void DatapathFree(Datapath *pdp)
{
	int i;

//	Free all allocated paths

	for (i = 0; i < pdp->numDatapaths; i++)
		{
		if (pdp->datapath[i])
			Free(pdp->datapath[i]);
		}

//	Clear structure to 0

	DatapathClear(pdp);
}

//	-------------------------------------------------------------
//
//	DatapathOpen() tries to open a data file for reading.
//	It tries the current directory if the datapath specifies,
//	then all dirs in data path.
//	Should only be used for read modes.
//
//		pdp   = ptr to data path struct
//		fname = filename
//		mode  = file open mode ("r" or "rb", usually)
//
//	Returns: ptr to opened file, or NULL

FILE *DatapathOpen(Datapath *pdp, const char *fname, const char *mode)
{
	FILE *fp;
	int i;
	char buff[128];

//	0 means either current directory or file not found

	pdp->last = 0;

//	Try current directory unless turned off

	if (!pdp->noCurrent)
		{
		fp = fopen(fname, mode);
		if (fp)
			return(fp);
		}

//	Try all dirs

	for (i = 0; i < pdp->numDatapaths; i++)
		{
		strcpy(buff, pdp->datapath[i]);
		strcat(buff, fname);
		fp = fopen(buff, mode);
		if (fp)
			{
			pdp->last = i + 1;
			return(fp);
			}
		}

//	Can't open, return NULL

	return(NULL);
}

//	-------------------------------------------------------------
//
//	DatapathFDOpen() tries to open a data file for reading.
//	It returns a file descriptor, rather than a FILE* structure.
//
//	If access&O_CREAT, a fourth argument must be passed that
//	specifies the permissions of the created file (see open()
//	docs for details)
//
//	It tries the current directory if the datapath specifies,
//	then all dirs in data path.
//	Should only be used for read modes.
//
//		pdp    = ptr to data path struct
//		fname  = filename
//		access = open() access flags
//	(opt)	permis = open() create permission flags.
//
//	Returns: descriptor of opened file, or -1

int DatapathFDOpen(Datapath *pdp, const char *fname, int access, ... )
{
	int fd;
	int i;
	int permis;
	char buff[128];
	va_list curr_arg;

//	If creating, set permissions

	if (access & O_CREAT)
		{
		// Get permis.
		va_start( curr_arg, access );
		permis = va_arg( curr_arg, int );
		}

//	0 means either current directory or file not found

	pdp->last = 0;

//	Try current directory unless turned off

	if (!pdp->noCurrent)
		{
		if (access & O_CREAT)
		   fd = open(fname, access, permis);
		else
		   fd = open(fname, access);

		if (fd != -1)
			return(fd);
		}

//	Try all dirs

	for (i = 0; i < pdp->numDatapaths; i++)
		{
		strcpy(buff, pdp->datapath[i]);
		strcat(buff, fname);

		if (access & O_CREAT)
			fd = open(buff, access, permis);
		else
		   fd = open(buff, access);

		if (fd != -1)
			{
			pdp->last = i + 1;
			return(fd);
			}
		}

//	Can't open, return -1

	return(-1);
}

//	--------------------------------------------------------------
//
//	DatapathFind() fills in a buffer with the full pathname of
//		a file, datapath plus filename.  If the file is not found
//		anywhere along the datapath, the buffer will be a null string.
//
//		pdp   = ptr to datapath
//		fname = filename
//		buff  = buffer to fill in
//    len   = length of buffer
//
//	Returns: TRUE if file found (and buffer filled in), FALSE if not found
//				(in which case buffer[0] will be 0.


int DatapathFind(Datapath *pdp, const char *fname, char *buff, int len)
{
	int i;
#if defined(__WATCOMC__) || defined(__SC__)
	struct find_t find;
	int err;
#else
	struct _finddata_t find;		// phs, 7/1/96
	long handle;
#endif

	for (i = -1; i < pdp->numDatapaths; i++)
		{
		if (i == -1)	// first, possibly try the current directory
			{
			if (pdp->noCurrent)
				continue;	// ... but not if noCurrent
			else
				{
               if (strlen(fname) >= len) {
                  Error(1,"buff len of %d exceeded by %d:\n%s",len,strlen(fname),fname);
               } else {
      				strcpy(buff, fname);
               }
				}
			}
		else
			{
            if (strlen(fname)+strlen(pdp->datapath[i]) >= len) {
               Error(1,"buff len of %d exceeded by %d:\n%s\n%s",len,strlen(fname)+strlen(pdp->datapath[i]),fname,pdp->datapath[i]);
            } else {
      			strcpy(buff, pdp->datapath[i]);
	      		strcat(buff, fname);
            }
			}
#if defined(__WATCOMC__) || defined(__SC__)
		err = _dos_findfirst(buff, _dp_find_flags_tab[pdp->find_flags], &find);
		if (err == 0)
			{
			_dos_findclose(&find);
			return TRUE;
			}
#else
		handle = _findfirst(buff, &find);	// MSVC _findfirst acts just a bit differently - phs, 7/1/96
		if (handle != -1)
			{
			_findclose(handle);
			return TRUE;
			}
#endif
		}

//	Can't find!

	buff[0] = 0;
	return FALSE;
}

//	--------------------------------------------------------------
//
//	DatapathAdd() adds a path to the list of data paths.
//
//		pdp  = ptr to data path
//		path = ptr to path string
//
//	Returns: TRUE if added, FALSE otherwise

bool DatapathAdd(Datapath *pdp, const char *path)
{
	bool any;
	const char *p;
	const char *psemi;
	char buff[128];

//	If path NULL, can't add

	if (path == NULL)
		return FALSE;

//	Assume no dirs added yet, start at top of path

	any = FALSE;
	p = path;

//	While any left in string, do DatapathAddDir() with string up to ';'

	while (*p)
		{
         const char* pplus = strchr(p,'+');
		psemi = strchr(p, ';');
      if (pplus != NULL) psemi = (psemi == NULL) ? pplus : min(psemi,pplus);
		if (psemi == NULL)
			{
			any |= DatapathAddDir(pdp, p);
			break;
			}
		strncpy(buff, p, psemi - p);
		buff[psemi - p] = 0;
		any |= DatapathAddDir(pdp, buff);
		p = psemi + 1;
		}

	return any;
}

//	--------------------------------------------------------------
//
//	DatapathAddDir() adds a directory to the list of data paths.
//
//		pdp  = ptr to data path
//		pdir = ptr to directory string
//
//	Returns: TRUE if added, FALSE otherwise

bool DatapathAddDir(Datapath *pdp, const char *pdir)
{
	int len,addSlash,c;
	char *p;

//	If path ptr list full, can't add

	if (pdp->numDatapaths == NUM_DATAPATHS)
		return FALSE;

//	If dir NULL, can't add

	if (pdir == NULL)
		return FALSE;

//	Check char at end, see if need to add trailing slash

	len = strlen(pdir);
	addSlash = 0;
	c = pdir[len - 1];
	if ((c != '/') && (c != '\\') && (c != ':'))
		++addSlash;

//	Allocate space for string, copy into it

	p = pdp->datapath[pdp->numDatapaths] = (char *)Malloc(len + addSlash + 1);
	strcpy(p, pdir);
	if (addSlash)
		strcat(p, "\\");

//	Bump current path counter and return

	++pdp->numDatapaths;
	return TRUE;
}

//	----------------------------------------------------------
//
//	DatapathLastPath() returns ptr to string for path used in file
//	last opened.  This is just path without filename portion.
//
//		pdp = ptr to datapath
//
//	Returns: ptr to path string, or ptr to "" if last file was opened
//				in current directory or couldn't be opened at all.

const char *DatapathLastPath(Datapath *pdp)
{
static const char *emptyString = "";

	if (pdp->last)
		return(pdp->datapath[pdp->last - 1]);
	else
		return(emptyString);
}

// Copies to dst from src, does not clear or free dst, so beware...
void DatapathCopy(Datapath *dst,const Datapath *src)
{
   int i;
   int len;
    
   // Copy all flags and stuff
   *dst = *src;

   for (i=0;i<src->numDatapaths;++i) {
      len = strlen(src->datapath[i]);
      dst->datapath[i]=(char *)Malloc(len+1); // extra one for null term
      strcpy(dst->datapath[i],src->datapath[i]);
   }
}



// ------------------------------------------------------------
// 
// DatapathFindDir() sets whether or not all subsequent calls will
// find directories or not.  
// We can eventually make this use Tom's new stuff directly
// 

void DatapathFindFlags(Datapath *pdp,bool files,bool dirs)
{
   // okay, zero means files, no dir
   pdp->find_flags = (files?0:1) | (dirs?2:0);
}
