/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/filetool.h,v 1.6 2000/01/29 13:20:50 adurant Exp $
// misc disk utility stuff
#pragma once

// does this directory exist, modifies path to be well-formed
EXTERN bool dir_exists(char *path); 

// munges base_path, base_2, and file together, adding appropriate /'s, into targ
// NOTE: there MUST BE A BASE_PATH, and targ must be long enough
EXTERN char *diskPathMunge(char *targ, const char *base_path, const char *base_2, const char *file);

// if there is no extension on the file, attaches it
// returns in targ, post is extra (with the "." included)
// if targ is NULL puts it back in base
EXTERN char *attachPostfix(char *targ, const char *base, const char *post);

// copies from current location in src_hnd to file named name
EXTERN bool CopyOpenFile(const char *name, int src_hnd);
// copies src to targ
EXTERN bool CopyFileNamed(const char *targ, const char *src);

// this is stupid, but might as well be here
#define is_slash(c) ((c=='\\')||(c=='/'))

// checks to see if a file is fully qualified
EXTERN BOOL is_full_path(const char* fn);



