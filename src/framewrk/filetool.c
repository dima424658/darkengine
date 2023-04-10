/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/filetool.c,v 1.8 2000/02/19 13:16:13 toml Exp $
// misc file tools for editor/resname stuff

#include <lg.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <filetool.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// does this directory exist
bool dir_exists(char *path)
{
   struct stat st;
   int strv;
   char endslash=-1;

   endslash=path[strlen(path)-1];
   if (is_slash(endslash))
      path[strlen(path)-1]='\0';
   strv=stat(path,&st);
   if (is_slash(endslash))
      path[strlen(path)-1]=endslash;
   if (strv==-1)
      return FALSE;
#ifndef _MSC_VER
   else if (S_ISDIR(st.st_mode))
      return TRUE;
   return FALSE;    // maybe its a file, or something else
#else
   return TRUE; // S_ISDIR does not exist in ms. (toml 12-05-96)
#endif
}

// munges base_path, base_2, and file together, adding appropriate /'s, into targ
// NOTE: there MUST BE A BASE_PATH, and targ must be long enough
char *diskPathMunge(char *targ, const char *base_path, const char *base_2, const char *file)
{  // first we need to get the base path, and see if it ends with a \ or not
   strcpy(targ,base_path);
   if (!(is_slash(targ[strlen(targ)-1])))
      strcat(targ,"\\");
   if (base_2!=NULL)
   {
      if (is_slash(base_2[0]))
         base_2++;
      strcat(targ,base_2);
   }
   if (!(is_slash(targ[strlen(targ)-1])))
      strcat(targ,"\\");
   if (file!=NULL)
   {
      if (is_slash(file[0]))
         file++;
      strcat(targ,file);
   }
   return targ;
}

// if there is no extension on the file, attaches it
// returns in targ, post is extra (with the "." included)
// if targ is NULL puts it back in base
char *attachPostfix(char *targ, char *base, const char *post)
{
   char *brkset="\\/.";
   char *flast=NULL, *tmp;
   
   if (targ==NULL)
      targ=base;
   else
      strcpy(targ,base);
   tmp=targ;
   while (tmp=strpbrk(tmp,brkset))
      flast=tmp++;
   if ((flast==NULL)||((*flast)!='.'))
      strcat(targ,post);
   return targ;
}

#define COPY_SIZE 4096
// copies from current location in src_hnd to file named name
bool CopyOpenFile(const char *name, int src_hnd)
{
   bool rv=TRUE;
   char *buf = Malloc(COPY_SIZE);
   int len, dst_hnd;

   if (!buf) return FALSE;
   dst_hnd = open(name, O_WRONLY|O_BINARY|O_CREAT|O_TRUNC, S_IWRITE);
   if (dst_hnd!=-1)
   {
      do {
         len = read(src_hnd, buf, COPY_SIZE);
         if (len) write(dst_hnd, buf, len);
      } while (len);
      close(dst_hnd);
   }
   else rv=FALSE;
   Free(buf);
   return rv;
}

// copies src to targ
bool CopyFileNamed(const char *targ, const char *src)
{
   int src_hnd;
   bool rv;

   src_hnd=open(src,O_RDONLY|O_BINARY);
   if (src_hnd==-1)
      return FALSE;
   rv=CopyOpenFile(targ,src_hnd);
   close(src_hnd);
   return rv;
}

BOOL is_full_path(const char* fn)
{
   if (fn == NULL) return FALSE;
   if (is_slash(fn[0])) return TRUE; // starting slash
   if (strchr(fn,':') != NULL) return TRUE; // drive letter
   return FALSE;
}
