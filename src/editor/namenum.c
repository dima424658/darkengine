/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/namenum.c,v 1.3 2000/02/19 13:11:07 toml Exp $
// simple system for creating/tracking/looking up/and save/loading numname associations

#include <lg.h>

#include <tagfile.h>
#include <vernum.h>

#include <namenum.h>
#include <simpwrap.h>

// the "control" info for the system
struct _namenum {
   char  *sys_name; // name of the namenum system - for file tags
   int    max_size; // overall size of system - total allowed
   int    cur_size; // current valid entries - how many entries
   char **names;    // actual names of entries - index by str_hnd
   int   *tags;     // tag list - index by str_hnd
};

//    The idea is the str_hnd is given to you if you have a known
// compact ID space, or want to manage a mapping from your ID space
// to the str_hnds, and need fast access.  Or you can just pass in
// the Tag, which will be slow (linear search) but easy.

/////////////////////
// init/free

// initializes a namenum table size cnt named "sys_name"
 sNameNum *NamenumInit(char *sys_name, int cnt)
{
   sNameNum *nSys=(sNameNum *)Malloc(sizeof(sNameNum));
   nSys->sys_name=sys_name;
   nSys->max_size=cnt;
   nSys->cur_size=0;
   nSys->names=(char **)Malloc(sizeof(char *)*cnt);
   memset(nSys->names,0,sizeof(char *)*cnt); // zero out the names
   nSys->tags=(int *)Malloc(sizeof(int)*cnt);
   memset(nSys->tags,0xff,sizeof(int)*cnt);  // and the tags (-1 is "free")
   return nSys;
}

// Free the NameNum and all its data
void      NamenumFree(sNameNum *ss)
{
   int i;
   for (i=0; i<ss->max_size; i++)
      if (ss->names[i])
         Free(ss->names[i]);
   Free(ss->names);
   Free(ss->tags);
   Free(ss);
}

/////////////////////
// file code

// helper functions to read and write data to the tagfile

// integers
static BOOL write_int(ITagFile *file, int val)
{
   if (ITagFile_Write(file,(char*)&val,sizeof(int)) == sizeof(int))
      return TRUE;
   Warning(("Couldnt write int %d to tagfile in nn\n",val));
   return FALSE;
}

static int read_int(ITagFile *file)
{
   int val;
   if (ITagFile_Read(file,(char*)&val,sizeof(int)) == sizeof(int))
      return val;
   else
   {
      Warning(("Couldnt read int from TagFile in nn\n"));
      return -1;
   }
}

// variable length strings (ie. an int then var len char array)
static BOOL write_str(ITagFile *file, char *str)
{
   int len=strlen(str)+1;
   write_int(file,len);
   if (ITagFile_Write(file,str,len) == len)
      return TRUE;
   Warning(("Couldnt write string %s to tagfile in nn\n",str));
   return FALSE;
}

static BOOL read_str(ITagFile *file, char **str)
{
   int len=read_int(file);
   if (len!=-1)
   {
      *str=(char *)Malloc(len);
      if (ITagFile_Read(file,*str,len) == len)
         return TRUE;
      Free(*str);
      Warning(("Couldnt read length %d from tagfile in nn\n",len));
   }
   else
      Warning(("Couldnt read length for string from tagfile in nn\n"));
   return FALSE;
}

// File Format is
//  integer max number of elements
//  [ integer sys name size
//    char *sys_name ]
//  for each element
//    integer tag
//    [ integer str_len
//      char *name ]

// saves ss to current TagFile (tagname "nn_"sys_name)
BOOL      NamenumSave(sNameNum *ss, ITagFile *file)
{
   TagVersion NamenumVer={1,0};
   TagFileTag NamenumTag={"nn*"};
   int i;

   if (!ss)
    { Warning(("No SS in NamenumSave!!")); return FALSE; }
   memset(NamenumTag.label,0,sizeof(NamenumTag.label));
   strncat(NamenumTag.label,ss->sys_name,sizeof(NamenumTag.label)-strlen(NamenumTag.label)-2);
   if (ITagFile_OpenBlock(file,&NamenumTag,&NamenumVer)==S_OK)
   {
      write_int(file,ss->max_size);
      write_str(file,ss->sys_name);
      for (i=0; i<ss->max_size; i++)
         if (ss->tags[i]!=NAMENUM_NO_TAG)
         {
            write_int(file,ss->tags[i]);
            write_str(file,ss->names[i]);
         }
         else
            write_int(file,NAMENUM_NO_TAG);
      ITagFile_CloseBlock(file);
      return TRUE;
   }
   return FALSE;
}

// loads and initializes sys_name as a namenum system, return NULL if not found
sNameNum *NamenumLoad(char *sys_name, ITagFile *file)
{
   TagVersion NamenumVer={1,0};
   TagFileTag NamenumTag={"nn*"};
   sNameNum *ss=NULL;

   memset(NamenumTag.label,0,sizeof(NamenumTag.label));   
   strncat(NamenumTag.label,sys_name,sizeof(NamenumTag.label)-strlen(NamenumTag.label)-2);
   if (ITagFile_OpenBlock(file,&NamenumTag,&NamenumVer)==S_OK)
   {
      int max_size=read_int(file), i;
      char *blk_name;
      read_str(file,&blk_name);
      if (strcmp(blk_name,sys_name)!=0)
         Warning(("nn Found Block %s which confuses me looking for %s\n",blk_name,sys_name));
      else
      {
         ss=NamenumInit(sys_name,max_size);
         for (i=0; i<max_size; i++)
         {
            int new_tag=read_int(file);
            if (new_tag!=NAMENUM_NO_TAG)
            {
               ss->tags[i]=new_tag;
               ss->cur_size++;
               read_str(file,&ss->names[i]);
            }
         }
      }
      Free(blk_name);
      ITagFile_CloseBlock(file);
   }
   return ss;
}

/////////////////////
// store/clear

static str_hnd _GetFreeHnd(sNameNum *ss)
{
   int i;
   if (ss)
      for (i=0; i<ss->max_size; i++)
         if (ss->tags[i]==NAMENUM_NO_TAG)
            return i;
   return NAMENUM_NO_HND;
}
   
// store "str" in the system with "tag", return the handle
str_hnd   NamenumStore(sNameNum *ss, char *str, int tag)
{
   str_hnd new_hnd=_GetFreeHnd(ss);
   AssertMsg1(ss,"No ss in NamenumStore %x",tag);
   if (new_hnd!=NAMENUM_NO_HND)
   {
      ss->tags[new_hnd]=tag;
      ss->names[new_hnd]=(char *)Malloc(strlen(str)+1);
      strcpy(ss->names[new_hnd],str);
      ss->cur_size++;
   }
   return new_hnd;
}

static str_hnd _GetHndForTag(sNameNum *ss, int tag)
{
   int i;
   for (i=0; i<ss->max_size; i++)
      if (ss->tags[i]==tag)
         return i;
   return NAMENUM_NO_HND;
}

static str_hnd _GetHndForString(sNameNum *ss, char *str)
{
   int i;
   for (i=0; i<ss->max_size; i++)
      if (ss->tags[i]!=NAMENUM_NO_TAG)
         if (stricmp(ss->names[i],str)==0)
            return i;
   return NAMENUM_NO_HND;
}

// store "str" in the system with "tag", return the handle
void      NamenumClear(sNameNum *ss, str_hnd hnd)
{
   AssertMsg1(ss,"No ss in NamenumClear %x",hnd);         
   if (hnd!=NAMENUM_NO_HND)
   {
      ss->tags[hnd]=NAMENUM_NO_TAG;
      Free(ss->names[hnd]);
      ss->names[hnd]=NULL;
      ss->cur_size--;
   }
}

// store "str" in the system with "tag", return the handle
void      NamenumClearByTag(sNameNum *ss, int tag)
{
   AssertMsg1(ss,"No ss in NamenumClearByTag %x",tag);      
   NamenumClear(ss,_GetHndForTag(ss,tag));
}

/////////////////
// actual data access

// retrieve the string using the system handle given by store
char     *NamenumFetch(sNameNum *ss, str_hnd hnd)
{
   AssertMsg1(ss,"No ss in NamenumFetchByHnd %x",hnd);
   if (hnd==NAMENUM_NO_HND)
      return NULL;
   return ss->names[hnd];
}

// retrieve the string using the tag given with creation
char     *NamenumFetchByTag(sNameNum *ss, int tag)
{
   AssertMsg1(ss,"No ss in NamenumFetchByTag %x",tag);   
   return NamenumFetch(ss,_GetHndForTag(ss,tag));
}

int       NamenumFetchTagForString(sNameNum *ss, char *name)
{
   int hnd=_GetHndForString(ss,name);
   if (hnd!=NAMENUM_NO_HND)
      return ss->tags[hnd];
   return -1;
}

// returns number of entries found - _newly mallocs_ *str_list
// if tags isn't NULL, it also fills it with a malloced list of tags
// NOTE: this seemed better to me than an iterator?  maybe not
int       NamenumAll(sNameNum *ss, char ***str_list, int **tags)
{
   int i, cur_pos=0;
   char **strs;

   AssertMsg(ss,"No ss in NamenumAll");
   strs=(char **)Malloc(ss->cur_size*sizeof(char *));
   if (strs)
   {
      if (tags)
         *tags=(int *)Malloc(ss->cur_size*sizeof(int));
      for (i=0; i<ss->max_size; i++)
         if (ss->names[i])
         {
            if (tags)
               (*tags)[cur_pos]=ss->tags[i];
            strs[cur_pos++]=ss->names[i];
         }
   }
   *str_list=strs;
   return cur_pos;
}

// applys mapping to all tags in ss
void      NamenumRetag(sNameNum *ss, int *mapping)
{
   int i;
   AssertMsg(ss,"No ss in NamenumRetag");
   for (i=0; i<ss->max_size; i++)
      if (ss->tags[i]!=NAMENUM_NO_TAG)
         ss->tags[i]=mapping[ss->tags[i]];
}

#ifdef MONO_MENUS
#include <mprintf.h>
#include <prompts.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static int PickMenu(char **list, int cnt)
{
   int i, rv;
   if (cnt<1)
      return NAMENUM_NO_TAG;
   mono_clear();
   for (i=0; i<cnt; i++)
      mprintf("%d> %s\n",i,list[i]);
   rv=prompt_int(NULL);
   mprintf("Choosing %d - %s\n",rv,rv==-1?"Cancel":list[rv]);
   return rv;
}
#endif

// return -1 if abort, else tag
int        NamenumSimpleMenuChooseTag(sNameNum *ss)
{
   char **list;
   int *tags;
   int choice, cnt;

   cnt=NamenumAll(ss,&list,&tags);
#ifdef MONO_MENUS
   choice=PickMenu(list,cnt);
#else
   choice=PickFromStringList(ss->sys_name,list,cnt);
#endif
   if (choice!=NAMENUM_NO_TAG)
      choice=tags[choice];
   Free(list);
   Free(tags);
   return choice;
}
