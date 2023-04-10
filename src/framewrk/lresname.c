/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/lresname.c,v 1.36 2000/02/19 13:16:23 toml Exp $
// total hack initial resname like behavior, maybe
// ie. this will get tossed, but for now is kinda resnamey
// ack

#include <stdio.h>     // PATH_MAX???
#include <string.h>

#include <allocapi.h> // for Alloc size

#include <timer.h>
#include <config.h>
#include <gfile.h>
#include <mprintf.h>

#include <filetool.h>

#include <lres_.h>     // internal header includes all other needed lres headers
#include <lreshash.h>
#include <lr_cache.h>
#include <rn_cnvrt.h>

#include <findhack.h>  // all the abstracted lg_find stuff
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//#define RN_STATS

// stat gathering silliness
#ifdef RN_STATS
#define rnStatInc(x) (x++)
#define rnStatIncConditional(cond,x) if (cond) rnStatInc(x)
static int rnStatLoadRequests=0;
static int rnStatLoadedAlready=0;
static int rnStatLoadFailed=0;
static int rnStatLoadNewHandle=0;
static int rnStatHandleRawCalls=0;
static int rnStatHandleRawFails=0;
static int rnStatHandleRawNoWhere=0;
static int rnStatHandleRawNoTokens=0;
#else
#define rnStatInc(x)
#define rnStatIncConditional(cond,x)
#endif // RN_STATS

static int ectsResNameSafeTop=16*1024*1024;

//////////////////////
// sad Base Path support
#define MAX_PATHS 4

       int   rn_cur_paths=0;
static char *base_paths[MAX_PATHS];

static char  config_base[PATH_MAX];

//////////////////////
// token defines - the constants and such are in lrestok.h
char  lres_rn_tokens[LRES_MAX_TOKENS][LRES_MAX_TOK_LEN];
short lres_rn_tok_lock[LRES_MAX_TOKENS];

//////////////////////
// Now the actual table itself
#define RNTFLG_INUSE       0x1
#define RNTFLG_OURMEM      0x2

typedef struct _nametable_entry {
   void *chunk;            // 4
   char  flags;
   char  users;
   char  lock_cnt;
   char  user_data;        // 8
   short type;
   short data;             // 12
   rn_name Name;           // +20 is 32
} _ResNameTable;

static int            cur_table_size=0;
static int            base_table_size=0;
static _ResNameTable *nameTab, *endTab;

#define rnTableInUse(p)    ((p)->flags&RNTFLG_INUSE)
#define _rn_GetEntry(hnd)  (&nameTab[hnd])

///////////////////////////
// current sucky With system
static char           cur_with[256];      // the with as a relative path only
static char           cur_with_tokens[LRES_TOKENS_PER]; // tokens for offset

// type strings for debugging
#ifdef DBG_ON
static char *type_names[]={"Raw","Image","Anim","Pal","Snd"};
static char  curAbsStr[64];
static char *getAbsTypeStr(int type)
{
   int i;
   curAbsStr[0]='\0';
   for (i=0; i<sizeof(type_names)/sizeof(type_names[0]); i++)
      if (type&(1<<i))
      {
         if (curAbsStr[0]!='\0') // we already have added one type
            strcat(curAbsStr,",");
         strcat(curAbsStr,type_names[i]);
      }
   return curAbsStr;
}
#else
#define getAbsTypeStr(x) ""
#endif

// global behavioral control - should i not warn on failed loads
BOOL resnameOkToFail=FALSE;

// does what you expect, currently only implements grows, not shrinks
BOOL _ResNameTableResize(int size)
{
   int i;
   if (cur_table_size==size)
      return TRUE;
   if (cur_table_size<size)
   {                       // have to grow the table
      if (cur_table_size)  // really have to grow it
         nameTab=(_ResNameTable *)Realloc(nameTab,size*sizeof(_ResNameTable));
      else                 // this really means we just have to create
         nameTab=(_ResNameTable *)Malloc(size*sizeof(_ResNameTable));
      for (i=cur_table_size; i<size; i++)
         nameTab[i].flags=0; // so we know that it is allocatable
      cur_table_size=size;
   }
   else                    // shrink, perhaps
   {                       // for now, we dont bother
      Warning(("Someone is trying to shrink the ResTable to size %d from %d\n",size,cur_table_size));
   }
   if (cur_table_size)
      endTab=nameTab+cur_table_size-1;
   return nameTab!=NULL;
}

static void _ResNameTableFree(void)
{
   if (cur_table_size&&nameTab)
   {
#ifdef DBG_ON
      _ResNameTable *p=nameTab;

      while (p<=endTab)
      {
         if (rnTableInUse(p))
         {
            BOOL _ResNameFreeHandle(rn_hnd handle); // later
            if (p->users)
            {
               void _RN_PrintSingleTableEntry(_ResNameTable *p); // later in file
               mprintf("IN USE ",p-nameTab);
               _RN_PrintSingleTableEntry(p);
            }  // now fall through and free it?
            _ResNameFreeHandle(p-nameTab);
         }
         p++;
      }
#endif
      Free(nameTab);
   }
   else
      Warning(("Freeing a non-existant resnameTable\n"));
}

// for now, just scans the entry table for handle.  not fast, but very easy
rn_hnd _ResNameGetFreeHandle(void)
{
   rn_hnd loc=0;
   _ResNameTable *p=nameTab;

   while (1)
   {
      while (p<=endTab)
      {
         if (!rnTableInUse(p))
            return loc;
         p++;
         loc++;
      }
      // auto-grow the table
      if (!_ResNameTableResize(cur_table_size+base_table_size))
         break;
      p=nameTab+loc; // get back to where we were in the scan
   }
   return NO_HND;
}

///////// Tokenizing/deTokenize/Scanning pass

// scan for a free token index to use
static char _rn_GetFreeTokIdx(void)
{
   int i;
   for (i=0; i<LRES_MAX_TOKENS; i++)
      if (lres_rn_tokens[i][0]=='\0')     // look, it is free
         return i+1;
   return LRES_NO_TOKEN;
}

// these next three are used by lresdisk and exposed in lrestok.h

// release a usage of this idx, if this makes the count 0, free it
void _rn_RelTokIdx(char idx)
{
   if (--rn_GetTokenLock(idx)==0)
      rn_GetTokenStr(idx)[0]='\0';   // note that it is free
}

// looks in current token list for this string
char _rn_FindTokenStr(char *token)
{
   int i;
   if (token[0]!='\0')
      for (i=0; i<LRES_MAX_TOKENS; i++)
         if (strnicmp(token,lres_rn_tokens[i],LRES_MAX_TOK_LEN)==0)
            return i+1;
   return LRES_NO_TOKEN;
}

// say that i need to use a "token".  returns the tok idx for it.
// if it isnt already a token, makes it one
// if it is, incs its lock cnt, returns it
char _rn_UseTokenStr(char *token)
{
   char idx=_rn_FindTokenStr(token);
   if (idx==LRES_NO_TOKEN)
   {
      idx=_rn_GetFreeTokIdx();
      if (idx==LRES_NO_TOKEN)
         return LRES_NO_TOKEN;
#ifdef DBG_ON
      if (strlen(token)>=LRES_MAX_TOK_LEN)
         Warning(("Overlong token <%s> in UseTokenStr\n",token));
#endif
      strncpy(rn_GetTokenStr(idx),token,LRES_MAX_TOK_LEN);
   }
   _rn_UseTokIdx(idx);
   return idx;
}

static char *_getNextSlash(char *s)
{
   char *p=s;
   while ((*p)!='\0')
      if (is_slash(*p))
         return p;
      else
         p++;
   return NULL;
}

char *_ResNameParmTokenize(char *name, char *tokens, BOOL allocate)
{
   char *s=name, *p=_getNextSlash(s), tmpc;
   char  tokidx, tokcnt=0;

   while (p!=NULL)
   {
      tmpc=*p; *p='\0';
      if (allocate)
         tokidx=_rn_UseTokenStr(s);
      else
         tokidx=_rn_FindTokenStr(s);
      *p=tmpc;
      if (tokidx==LRES_NO_TOKEN)
         return NULL;
      tokens[tokcnt++]=tokidx;
      s=p+1; p=_getNextSlash(s);   // move to the next slash
      if ((p!=NULL)&&(tokcnt>=LRES_TOKENS_PER))
      {
         Warning(("ResName: %s has too many path (token) elements",name));
         return NULL;
      }
   }
   while (tokcnt<LRES_TOKENS_PER)
      tokens[tokcnt++]=LRES_NO_TOKEN;
   return s;
}

#define _ResNameTokenize(name, tokens) _ResNameParmTokenize(name,tokens,TRUE)

char *_ResNameDetokenizeRaw(char *buf, char *name, char *tokens)
{
   int i=0;
   strcpy(buf,"");
   while ((i<LRES_TOKENS_PER)&&(tokens[i]!=LRES_NO_TOKEN))
   {
      strcat(buf,rn_GetTokenStr(tokens[i]));
      strcat(buf,"/");
      i++;
   }
   buf[strlen(buf)+RN_NAME_LEN]='\0';    // in case we are full length
   strncat(buf,name,RN_NAME_LEN);        // if our strncat caps out
   return buf;
}

char *_ResNameDetokenize(char *buf, rn_name *nme)
{
   return _ResNameDetokenizeRaw(buf,nme->name,nme->tokens);
}

//////// Handle decoding
// remedial external hashing has been added, 
// if tokens is NULL it will not try and token match
// if where is LRES_NO_WHERE it will not try and where match

#define TokenCompare(a,b) (*((int *)a)==*((int *)b))
//#define TokenCompare(a,b) (strncmp(a,b,LRES_TOKENS_PER)==0)

#define USE_RN_HASH

rn_hnd _ResNametoHandleRaw(char *name, char *tokens, int where)
{
   _ResNameTable *p=nameTab;
   rn_hnd loc=0;
   char buf[20];
   
   strcpy(buf,name);
   strlwr(buf);

   rnStatInc(rnStatHandleRawCalls);

#ifdef USE_RN_HASH
   if ((where==LRES_NO_WHERE)&&(tokens!=NULL))
      return _ResNametoHandleFast(name,tokens);
#endif      
   
   rnStatIncConditional(where==LRES_NO_WHERE,rnStatHandleRawNoWhere);
   rnStatIncConditional(tokens==NULL,rnStatHandleRawNoTokens);

   if ((where==LRES_NO_WHERE)&&(tokens==NULL))  // easy version
      while (p<=endTab)
      {
         if (rnTableInUse(p))
            if (strncmp(p->Name.name,buf,RN_NAME_LEN)==0)
               return loc;
         p++;
         loc++;
      }
   else if (where==LRES_NO_WHERE)
      while (p<=endTab)
      {
         if (rnTableInUse(p))
            if ((tokens==NULL)||TokenCompare(p->Name.tokens,tokens))
               if (strncmp(p->Name.name,buf,RN_NAME_LEN)==0)
                  return loc;
         p++;
         loc++;
      }
   else
      while (p<=endTab)
      {
         if (rnTableInUse(p))
            if ((where==LRES_NO_WHERE)||(where==p->Name.where))
               if ((tokens==NULL)||TokenCompare(p->Name.tokens,tokens))
                  if (strncmp(p->Name.name,buf,RN_NAME_LEN)==0)
                     return loc;
         p++;
         loc++;
      }
   rnStatInc(rnStatHandleRawFails);
   return NO_HND;
}

rn_hnd _ResNameRNNametoHandle(rn_name *nme)
{
   return _ResNametoHandleRaw(nme->name,nme->tokens,nme->where);
}

rn_hnd _ResNameFullNametoHandle(char *name)
{
   char l_tok[LRES_TOKENS_PER];
   _ResNameTokenize(name,l_tok);
   return _ResNametoHandleRaw(name,l_tok,LRES_NO_WHERE);
}

////////// Alloc/Dealloc

// just set up a record
rn_hnd _ResNameAnonBuild(void)
{
   rn_hnd handle;
   handle=_ResNameGetFreeHandle();
   if (handle!=NO_HND)
   {
      _ResNameTable *p=_rn_GetEntry(handle);
      p->flags=RNTFLG_INUSE;
      p->users=p->lock_cnt=0;
      p->data=0;
   }
   return handle;
}

// build with a name
rn_hnd _ResNameNamedBuild(char *name, char *tokens, int where)
{
   rn_hnd handle=_ResNameAnonBuild();
   if (handle!=NO_HND)
   {
      _ResNameTable *p=_rn_GetEntry(handle);
      char buf[32];
      if (tokens==NULL)   // name tokenize does token usage/alloc increments
         name=_ResNameTokenize(name,p->Name.tokens);
      else
      {           // re-rewrite this if net crash hosed it....
         int i=0;
         while ((i<LRES_TOKENS_PER)&&(tokens[i]!=0))
         {
            p->Name.tokens[i]=tokens[i];
            _rn_UseTokIdx(tokens[i]);
            i++;
         }
         while (i<LRES_TOKENS_PER)
            p->Name.tokens[i++]=0;
      }
      strncpy(buf,name,RN_NAME_LEN);
      buf[RN_NAME_LEN]='\0';
      strlwr(buf);
      strncpy(p->Name.name,buf,RN_NAME_LEN);
      p->Name.where=where;
#ifdef USE_RN_HASH
      _ResNameHashStore(handle);
#endif      
   }
   return handle;
}

// really really free a handle
BOOL _ResNameFreeHandle(rn_hnd handle)
{
   _ResNameTable *p=_rn_GetEntry(handle);
   if (p->flags==0)
      return FALSE;
   if (p->flags&RNTFLG_OURMEM)
      Free(p->chunk);
   // free up token list
   p->flags=0;
#ifdef USE_RN_HASH
   _ResNameHashFree(handle);
#endif
   return TRUE;
}

static BOOL ResNameCacheKillCallback(rn_hnd hnd, void *data)
{
   return _ResNameFreeHandle(hnd);
}

// @TODO: locks need to work.  as part of that, there should probably be a callback
//   you can hook for full unlocks... at least some global one you can hook for now
//   would be good.... then the app can check user data/scan internals/whatever
BOOL ResNameFreeHandle(rn_hnd handle)
{
   _ResNameTable *p;

   if (handle==LRES_NO_HND)
   {
      Warning(("RNFreeHandle: trying to Free the Null Handle\n"));
      return FALSE;
   }
   p=_rn_GetEntry(handle);
   if (p->users)
      p->users--;
   else
      Warning(("RNFreeHandle: already have 0 users?\n"));
   if (p->users)
      return FALSE;
   if (p->flags&RNTFLG_OURMEM)  // so lets put it in the cache
   {  // is there a way for this to return that it blew the whole thing off?
#ifdef DBG_ON
      if (!lrCache_StoreFreedData(handle,p->chunk,MSize(p->chunk)))
         return _ResNameFreeHandle(handle);
#else
      lrCache_StoreFreedData(handle,p->chunk,MSize(p->chunk));
#endif
      return FALSE;
   }
   else
      return _ResNameFreeHandle(handle);
}

/////////// With System

// sets a subelement as the base of all accesses
BOOL ResNameWith(char *with)
{  // for now, this is all a hack, using the last entry in the base table
   if (with!=NULL)
      diskPathMunge(cur_with,with,NULL,NULL);
   else
      strcpy(cur_with,".\\");
   _ResNameTokenize(cur_with,cur_with_tokens);
   return TRUE;
}

/////////// Refer to ResNames

// actually get the data associated with handle
void    *ResNameGetbyHandle(rn_hnd handle)
{
   return  nameTab[handle].chunk;
}

int      ResNameGetSize(rn_hnd handle)
{
   if (nameTab[handle].flags&RNTFLG_OURMEM)
      return MSize(nameTab[handle].chunk);
   else
   {
      Warning(("ResNameGetSize: called on %d, which we didnt allocate\n"));
      return 0;  // ie. we dont know, it is the apps problem
   }
}

// return the whole rn struct
rn_name *ResNameGetNameData(rn_hnd handle)
{
   return &nameTab[handle].Name;
}

// return the user data field
char     ResNameGetUserData(rn_hnd handle)
{
   return  nameTab[handle].user_data;
}

// try and find the given name string
rn_hnd   ResNameLookupName(char *name)
{
   return _ResNametoHandleRaw(name,NULL,LRES_NO_WHERE);
}

// try and find a given name with a token in it
//  NOTE: Assumes where is LRES_NO_WHERE - since where is a lres internal
rn_hnd   ResNameLookupTokenandName(rn_name *rname)
{
   return _ResNametoHandleRaw(rname->name,rname->tokens,LRES_NO_WHERE);
}

// set the user data for our handle
void     ResNameSetUserData(rn_hnd handle, char user_data)
{
   nameTab[handle].user_data=user_data;
}

// returns FALSE if these tokens are not around
BOOL     ResNameBuildTokens(char *tokens, char *str)
{
   return _ResNameParmTokenize(str,tokens,FALSE)!=NULL;
}

//// bool? what am i thinking... hmmmmmmm
// currently they return if they are "the one"
BOOL     ResNameLock(rn_hnd hnd)
{
   _ResNameTable *p=_rn_GetEntry(hnd);
   if ((p->lock_cnt++)==0)  // no locks at the moment
      return TRUE;
   return FALSE;
}

BOOL     ResNameUnlock(rn_hnd hnd)
{
   _ResNameTable *p=_rn_GetEntry(hnd);
   if ((--p->lock_cnt)==0)  // unlocked final lock
      return TRUE;
   return FALSE;
}

// strips the extension off of a file in place
static void _strip_extension(char *inp)
{
   char *p=inp+strlen(inp);

   while (--p>inp)
      if (*p=='.')
      {
         *p='\0';       // found a dot, we are done, head home after toasting ext
         break;
      }
      else if (is_slash(*p))
         break;         // found a directory separator, dots before here are not extensions
}

// @HACK: an OUtRAGeous Hack for ECTS
extern BOOL ectsAnimTxtValidName(char *fname);

// fname is the raw, our_tokens is token of path or NULL, where is where
// base_p and our_tokens are redundant, base_p must be passed in (unless it really is NULL)
// if our_tokens isnt passed in, it will be generated
static rn_hnd _ResNameRawLoadSingle(char *fname, char *our_tokens, char *base_p,
                                    int where, int valid_types, RN_ScanCallback cback)
{
   char work_file[PATH_MAX], tok_buf[LRES_TOKENS_PER];
   char short_name[PATH_MAX];
   int this_type, old_hnd;
   void *chunk;

   rnStatInc(rnStatLoadRequests);

   // if our_tokens NULL, build from base_p
   if (our_tokens==NULL)
    { _ResNameTokenize(base_p,tok_buf); our_tokens=&tok_buf[0]; }
   
   strcpy(short_name,fname);
   _strip_extension(short_name);
   strlwr(short_name);

   // make sure we dont already have this loaded
   if ((old_hnd=_ResNametoHandleRaw(short_name,our_tokens,LRES_NO_WHERE))!=NO_HND)
   {
      _ResNameTable *p=_rn_GetEntry(old_hnd);
      if (p->type&valid_types)
      {
         rnStatInc(rnStatLoadedAlready);
         // until we do real lock/unlock, just inc user count
         if (++p->users==1)  // first new user, take back from cache
            AssertMsg2(lrCache_TakeBackData(old_hnd),
               "Lres tried to get %d [%s] from cache\n but it wasnt there to take back?",
               old_hnd,_ResNameDetokenizeRaw(work_file,fname,our_tokens));
         return old_hnd;
      }
      Warning(("We have a [ %s ] but it is of type %x\n",
               _ResNameDetokenizeRaw(work_file,fname,our_tokens),p->type));
      return NO_HND;
   }

   // we dont, lets path munge and try again
   diskPathMunge(work_file,base_paths[where],base_p,fname);
   if ((chunk=RN_ConvertFile(work_file,valid_types,&this_type))!=NULL)
   {  // found it, lets build an entry
      BOOL keep=TRUE, our_mem=TRUE;
      rn_hnd handle;
      char u_data=0;

      handle=_ResNameNamedBuild(short_name,our_tokens,where);
      if (cback)
      {
         void *och=(void *)chunk;
         keep=(*cback)(short_name,handle,this_type,&u_data,&chunk);
         our_mem=(och==chunk);
         if ((!our_mem)||(!keep))
            Free(och);
      }
      if (keep)
      {
         _ResNameTable *p=_rn_GetEntry(handle);
         p->chunk=chunk;
         p->type=this_type;
         p->user_data=u_data;
         if (our_mem)
            p->flags|=RNTFLG_OURMEM;
         p->users++;
         rnStatInc(rnStatLoadNewHandle);
         return handle;
      }
      else
      {
         _ResNameFreeHandle(handle);
         Warning(("App rejected %s\n",work_file));
      }
   }
   rnStatInc(rnStatLoadFailed);
   return NO_HND;
}

// actual thing which does the single load, looping through all wheres
// mostly modularized out for use by lresdisk and other stuff
rn_hnd _ResNameLoadSingle(char *name, char *tokens, char *base_p, int types, RN_ScanCallback cback)
{
   int where=rn_cur_paths-1;    // loop through all the where paths looking for us
   if ((types&RN_IMAGE)&&(ectsAnimTxtValidName(name))) return LRES_NO_HND; // @HACK for ECTS
   while (where>=0)
   {
      int handle=_ResNameRawLoadSingle(name,tokens,base_p,where,types,cback);
      if (handle!=NO_HND) return handle;
      where--;
   }
   if (!resnameOkToFail)
   {
#ifdef DBG_ON
      if (name==NULL) Warning(("rnLoadSingle: We have no name?"));
      if (strnicmp(name,"null",4)==0)
         Warning(("rnLoadSingle: our name is Null "));
#endif
      Warning(("%s not ResName convertable to %s\n",name,getAbsTypeStr(types)));
   }
   return LRES_NO_HND;
}

// takes a rn_name with tokenized path, generates data for scan single
// for now, this basically detokenizes, then RawLoad retokenizes
// clearly stupid, but it works
// the problem is base_p is needed for the diskpathMunge, so it has to be built
rn_hnd _ResNameLoadRnName(rn_name *rndat, int types, RN_ScanCallback cback)
{
   char new_base_p[PATH_MAX];
   _ResNameDetokenizeRaw(new_base_p,"",rndat->tokens);
   return _ResNameLoadSingle(rndat->name,rndat->tokens,new_base_p,types,cback);
}

// the basic single name load call, which goes through all Wheres looking for name
// can take a type and callback, in the resname way
rn_hnd ResNameLoad(char *with, char *name, int valid_types, RN_ScanCallback cback)
{
   char path_tokens[LRES_TOKENS_PER], *cur_tokens;

   if (with==NULL)        // if you scan NULL, you get current with context
    { with=cur_with; cur_tokens=&cur_with_tokens[0]; }
   else
    { _ResNameTokenize(with,path_tokens); cur_tokens=&path_tokens[0]; }

   return _ResNameLoadSingle(name,cur_tokens,with,valid_types,cback);
}

// really want a _A_VOLID, but MSVC doesnt seem to have one, argh

#define _A_NOT_OK (_A_HIDDEN|_A_SYSTEM|_A_SUBDIR)

// code zaniness ahead!
// the issue really is that we dont want to count multiply the same file in a scan
// ie. if you find bob.pcx in netpath/ and locpath/, you dont want count to be 2 or
//  it to have a lock count of 2.  so we currently do this hideous thing where we
//  build a cache as we go of all handles scanned as part of this call, and whenever
//  we see the same handle again, we go remove a user from it, else we dec count
// note this is heavily dependant onthe implementation of getSingle

// this scans through all directories in the where_path looking for the with string
//  for each find, it tries to resnamegetsingle it, and returns a count
int ResNameScanLoad(char *with, int valid_types, RN_ScanCallback cback)
{
   lg_find_declare(cur_file);
   char work_path[PATH_MAX];           // base to read from
   char path_tokens[LRES_TOKENS_PER], *cur_tokens;
   int cnt=0, where;
   rn_hnd *this_scan;

   if (with==NULL)        // if you scan NULL, you get current with context
    { with=cur_with; cur_tokens=&cur_with_tokens[0]; }
   else
    { _ResNameTokenize(with,path_tokens); cur_tokens=&path_tokens[0]; }
   this_scan=(rn_hnd *)Malloc(sizeof(rn_hnd)*256);

   // loop over all possible where's here, someday
   where=rn_cur_paths-1;
   while (where>=0)
   {
      diskPathMunge(work_path,base_paths[where],with,"*");    // NULL);
      if (lg_findfirst_p(work_path,&cur_file))
      {
         do {           // go ahead and parse/include this if it isnt "notok"
            if ((valid_types&RN_IMAGE)&&ectsAnimTxtValidName(lg_find_name(cur_file))) continue; // @HACK for ECTS
            if ((lg_find_attrib(cur_file)&_A_NOT_OK)==0)
            {
               rn_hnd new_hnd;
               new_hnd=_ResNameRawLoadSingle(lg_find_name(cur_file),cur_tokens,with,
                                          where,valid_types,cback);
               if (new_hnd!=NO_HND)
               {  // check to see if we have already used this
                  int loop;
                  for (loop=0; loop<cnt; loop++)
                     if (this_scan[loop]==new_hnd)
                     {  // already found it this scan, so skip it
                        _ResNameTable *p=_rn_GetEntry(new_hnd);
                        p->users--;
#ifdef DBG_ON
                        if (p->users==0)
                           Warning(("HEY: scanload set users to 0, for %d (%s)\n",
                                    new_hnd,lg_find_name(cur_file)));
#endif
                        break;
                     }
                  if (loop==cnt)
                  {
                     cnt++;
                     if ((cnt&0xff)==0)    // every 256 need to realloc up in size
                        this_scan=(rn_hnd *)Realloc(this_scan,sizeof(rn_hnd)*(cnt+256));
                     this_scan[loop]=new_hnd;
                  }
               }
               else if (!resnameOkToFail)
               {
                  char *s=lg_find_name(cur_file);
#ifdef DBG_ON
                  if (s==NULL) Warning(("rnLoadScanDir: We have no name?"));
                  if (strnicmp(s,"null",4)==0)
                     Warning(("rnLoadScanDir: our name is Null "));
#endif
                  Warning(("%s not ResName convertable to %s (%x)\n",
                           s,getAbsTypeStr(valid_types),valid_types));
               }
            }
         }
         while (lg_findnext_p(&cur_file));
         lg_findclose(&cur_file);
      }
      where--;
   }
   Free(this_scan);
   return cnt;
}

// set a new "home" for named resources, either a location for packages or raws
BOOL ResNameOpen(char *base)
{
   if (rn_cur_paths>=MAX_PATHS)
      return FALSE;
   if (dir_exists(base))
      base_paths[rn_cur_paths++]=base;
   else
      return FALSE;
   return TRUE;
}

#ifdef RN_STATS
void rnStatClear(void)
{
   rnStatLoadRequests=rnStatLoadedAlready=rnStatLoadFailed=0;
   rnStatHandleRawCalls=rnStatHandleRawFails=rnStatLoadNewHandle=0;
   rnStatHandleRawNoWhere=rnStatHandleRawNoTokens=0;
}

void rnStatShow(void)
{
   mprintf("RN: Req %d Already %d Failed %d New %d\nHandle Calls %d Fails %d NoWhere %d NoToken %d\n",
   	rnStatLoadRequests,rnStatLoadedAlready,rnStatLoadFailed,
      rnStatLoadNewHandle,rnStatHandleRawCalls,rnStatHandleRawFails,
      rnStatHandleRawNoWhere,rnStatHandleRawNoTokens);
}
#endif

// start up the system, reads config, so on
BOOL ResNameInit(int size)
{
   if (!_ResNameTableResize(size))
      return FALSE;
   base_table_size=size;
   if (config_get_raw("resname_base",config_base,sizeof(config_base)))
      ResNameOpen(config_base);
   ResNameOpen(".");
   config_get_int("ResNameCap",&ectsResNameSafeTop);
#ifdef USE_RN_HASH
   _ResNameHashInitialize();
#endif
   lrCache_Init(ResNameCacheKillCallback,-1);
   return TRUE;
}

// go through and close down all resources/free all memory?
BOOL ResNameShutdown(void)
{
   lrCache_Term();
   _ResNameTableFree();
   return TRUE;
}

#ifndef SHIP
static char n_buf[256];
char *_RN_DumpNameInfo(rn_name *nme, bool show_source)
{
   if (show_source)
      mprintf("    Src %d, tok %d %d %d %d.. ",
           nme->where,nme->tokens[0],nme->tokens[1],nme->tokens[2],nme->tokens[3]);
   // TODO: use where to initialize buf later
   strcpy(&n_buf[0],"");
   _ResNameDetokenize(n_buf+strlen(n_buf),nme);

   return n_buf;
}

void _RN_PrintSingleTableEntry(_ResNameTable *p)
{
   if (!rnTableInUse(p)) return;
   mprintf("%3.3d> flags %x, chunk %x, users %d, locks %d (type %d, data %d)\n",
           (int)(p-nameTab),p->flags,p->chunk,p->users,p->lock_cnt,p->type,p->data);
   mprintf(" full: %s\n",_RN_DumpNameInfo(&p->Name,TRUE));
}

int  _ResNameDumpTable(bool all)
{
   int loc=0, cnt=0;
   _ResNameTable *p=nameTab;
   while (p<=endTab)
   {
      if (rnTableInUse(p))
      {
         _RN_PrintSingleTableEntry(p);
         cnt++;
      }
      else if (all)
         mprintf("%d> unused\n",loc);
      p++;
      loc++;
   }
   mprintf("Table is %d active entries, out of %d total\n",cnt,loc);
   return cnt;
}
#endif
