/*
 * $Header: x:/prj/tech/libsrc/lg/RCS/dpathdir.c 1.8 1998/05/07 10:05:13 DAVET Exp $
 *
 * Datapath style opendir and readdir commands
 *
*/

#include <datapath.h>
#include <lg.h>
#include <string.h>

// stuffs current name
char *DatapathDirGetName(DatapathDir *dpd)
{
   return dpd->find.name;
}

// stuffs current path into buffer 
void DatapathDirGetPath(DatapathDir *dpd,char *s)
{
   char *p;

   if (dpd->dp->datapath[dpd->curp])
      strcpy(s,dpd->dp->datapath[dpd->curp]);
   else
      s[0]=0;

   strcat(s,dpd->path);

   // work back to start or delineation
   p=s+strlen(s);
   while (p>=s && *p!='/' && *p!='\\' && *p!=':')
      --p;

   ++p;
   strcpy(p,dpd->find.name);
}

DatapathDir *DatapathOpenDir(Datapath *dpath, const char *name,int flags)
{
   DatapathDir *dpd;

   dpd = (DatapathDir *)Malloc(sizeof(DatapathDir));
   memset(dpd, 0, sizeof(*dpd));
   
   // set data path and name
   dpd->dp = dpath;
   strcpy(dpd->path,name);
   dpd->curp = 0;
   dpd->cur = 0;
   dpd->flags = flags;

   return dpd;
}

char *DatapathReadDir(DatapathDir *dpd)
{
   int err;
   char path[128];

   // works on a null datapath
   while((dpd->dp->datapath[dpd->curp]!=NULL) || (dpd->curp==0))
   {
      // first time for this one
      if (dpd->cur == 0) {
         if (dpd->dp->datapath[dpd->curp]) {
            strcpy(path,dpd->dp->datapath[dpd->curp]);
         } else {
            path[0] = 0;
         }

         strcat(path,dpd->path);
         err = PlatformFindFirst(path, &dpd->find) ? 0 : 1;
      } else {
         err = PlatformFindNext(&dpd->find) ? 0 : 1;
      }

      // if there was not a read error
      if (err==0)
      {  // Screen out dot and double dot, because it's dumb.
	      dpd->cur++;
	      if (dpd->flags & DP_SCREEN_DOT)
	         if (strcmp(dpd->find.name,".")==0 || strcmp(dpd->find.name,"..")==0)
	            continue;   // got dot, so we want to scan past it
         break;            // break out, since we have found a real file
      }

      PlatformFindClose(&dpd->find);
      dpd->curp++;
      dpd->cur=0;
   }

   if (err!=0) return NULL;
   return dpd->find.name;
}

void DatapathCloseDir(DatapathDir *dpd)
{
   if (dpd->cur!=0) {
      PlatformFindClose(&dpd->find);
   }

   Free(dpd);
}


