//		FNAME.C		Filename manipulator
//		Rex E. Bradford (REX)
//
//		This module contains routines to split a filename string into
//		the four pieces which make it up (drive, path, name, extension),
//		and to build a filename string from these pieces.  The header
//		file defines an Fname struct which holds the four pieces; it
//		also contains macros to replace any individual piece in the
//		structure, and to add a piece only if it is empty (handing for
//		adding default extensions unless one already present, for instance).
/*
* $Header: r:/prj/lib/src/dstruct/rcs/fname.c 1.1 1993/05/03 10:53:38 rex Exp $
* $Log: fname.c $
 * Revision 1.1  1993/05/03  10:53:38  rex
 * Initial revision
 * 
*/

#include <string.h>
#include "fname.h"

//	-------------------------------------------------------
//
//	FnameExtract() extracts parts from string into fname struct.
//
//		fname = ptr to fname struct
//		str   = ptr to string
//
//	Returns: 0 if ok
//				-1 if some part truncated

int FnameExtract(Fname *fname, char *str)
{
	char *p;
	int i,len,ret;

//	Assume a happy journey

	ret = 0;

//	Get drive

	if (*str && (*(str + 1) == ':'))
		{
		fname->drive = *str;
		str += 2;
		}
	else
		fname->drive = 0;

//	Get path (find last / or \, copy up through it)

	p = str + strlen(str) - 1;
	while (p >= str)
		{
		if ((*p == '/') || (*p == '\\'))
			break;
		--p;
		}
	i = 0;
	len = (p + 1) - str;
	if (len >= sizeof(fname->path))
		{
		len = sizeof(fname->path) - 1;
		ret = -1;
		}
	memcpy(fname->path, str, len);
	fname->path[len] = 0;
	str = p + 1;

//	Get name

	p = strchr(str, '.');
	if (p == NULL)
		p = str + strlen(str);
	len = p - str;
	if (len >= sizeof(fname->name))
		{
		len = sizeof(fname->name) - 1;
		ret = -1;
		}
	memcpy(fname->name, str, len);
	fname->name[len] = 0;
	str = p;

//	Get ext

	if (*str == 0)
		{
		fname->ext[0] = 0;
		ret = -1;
		}
	else
		{
		++str;
		len = strlen(str);
		if (len >= sizeof(fname->ext))
			{
			len = sizeof(fname->ext) - 1;
			ret = -1;
			}
		memcpy(fname->ext, str, len);
		fname->ext[len] = 0;
		}

//	Return our return code

	return(ret);
}

//	----------------------------------------------------------
//
//	FnameBuild() builds a filename string from a Fname struct.
//
//		buff  = ptr to text buffer (should be 128 bytes long)
//		fname = ptr to fname struct
//
//	Returns: ptr to buffer, for convenience

char *FnameBuild(char *buff, Fname *fname)
{
	char *p,*p2;

//	Get ready

	p = buff;

//	Put drive: if present

	if (fname->drive)
		{
		*p++ = fname->drive;
		*p++ = ':';
		}

//	Add path, with trailing '/' if not present

	p2 = fname->path;
	while (*p2)
		*p++ = *p2++;
	if ((fname->path[0]) && (*(p - 1) != '/') && (*(p - 1) != '\\'))
		*p++ = '/';

//	Add name

	p2 = fname->name;
	while (*p2)
		*p++ = *p2++;
	*p = 0;

//	Add ext, with preceding '.'

	if (fname->ext[0])
		{
		*p++ = '.';
		strcpy(p, fname->ext);
		}

//	Return ptr to buffer

	return(buff);
}

//	------------------------------------------------------------
//
//	FnameReplacePart() replaces a part of an fname, with overflow checking.
//
//		part   = ptr to part
//		str    = replacement text
//		maxlen = maximum length

void FnameReplacePart(char *part, char *str, int maxlen)
{
	strncpy(part, str, maxlen);
	part[maxlen] = 0;
}

