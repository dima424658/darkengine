/*
 * $Source: x:/prj/tech/libsrc/dump/RCS/filefind.c $
 * $Revision: 1.5 $
 * $Author: dc $
 * $Date: 1997/08/11 08:17:54 $
 *
 * Screen dumping file finder (actually quite generic)
 *
 */

#include <string.h>
#include <stdlib.h>     // _MAX_PATH
#include <fcntl.h>
#include <io.h>
#include <sys\stat.h>

/* Finds the free file in a sequence like
 * "<prefix>000.<suff>", tries to open it,
 * and returns you a pointer to it.
 * maximum prefix is 5 letters long.
 * maximum suffix is 3 letters long.
 * numbers the files in decimal.
 * if successful and buff non zero copies the name into buff
 */

// modified by dc to use max path
int dmp_find_free_file(char *buff,char *prefix,char *suffix)
{
   char fname[_MAX_PATH];
   int fp, ps;
   int num=0;
   
   ps = strlen(prefix);
   strcpy(fname,prefix);
   strcat(fname,"000.");
   strcat(fname,suffix);
   
   /* Look for files like uwpic000.gif */
   while ( (fp = open(fname,O_BINARY|O_RDONLY)) != -1)
   {  /* Check next slot */
      close(fp);                       // good idea to, like, close the opened file
      ++num;
      fname[ps+2] = '0'+(num%10);
      fname[ps+1] = '0'+((num/10)%10);
      fname[ps] = '0'+((num/100)%10);
   }
   
   if ((fp = open(fname,O_CREAT|O_BINARY|O_WRONLY,S_IREAD|S_IWRITE))==-1)
      return -1;
   if (buff != NULL)
      strcpy(buff,fname);
   return fp;
}


