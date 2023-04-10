/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/lreshash.cpp,v 1.4 2000/02/19 13:16:22 toml Exp $
// lets try and speed up toHandleRaw

#include <lg.h>
#include <mprintf.h>

#include <hashset.h>
#include <hshsttem.h>

#include <lresname.h>
#include <lrestok.h>
#include <lress.h>

#include <lreshash.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define SIZEOF_KEY 20

typedef struct rnHash {
   char   key[SIZEOF_KEY];
   rn_hnd data;
} rnHash;


// Our hash table
class cRNHashTable : public cHashSet<rnHash *, char *, cHashFunctions>
{
public:
   ~cRNHashTable()
   {
      DestroyAll();
   }

private:
   BOOL IsEqual(tHashSetKey key1, tHashSetKey key2) const
   {
      return memcmp((void *)key1, (void *)key2, SIZEOF_KEY) == 0;
   }

   tHashSetKey GetKey(tHashSetNode p) const
   {
      return (tHashSetKey)(((rnHash *)p)->key);
   }

   unsigned Hash(tHashSetKey key) const
   {
      return HashThing((void *)key, SIZEOF_KEY);
   }

   void DestroyAll()
   {
      tHashSetHandle h;
      rnHash * pRnHash;

      while ((pRnHash = GetFirst(h)) != NULL)
      {
         Free(Remove(pRnHash));
      }
   }
};

cRNHashTable rnHashTab;

// this takes a data block of length N and looks for it in the hash table
//   needs to return the value associated with the block
rnHash *_ResNameHashRealLookup(char *key)
{
   // do the lookup
   // if success, return rnHash->data

   return rnHashTab.Search(key);
}

// this stores a data block
void   _ResNameHashRealStore(rnHash *tmp)
{
   // store rnHash into, in fact, the hash
   rnHashTab.Insert(tmp);
}

void   _ResNameHashInitialize(void)
{
   // lets set it on up
}

static void _build_rnHashBlock(char *block, int where, char *name, char *tokens)
{
   char *s=block;
   int i, st; // st is string state, 0 means in string, 1 means past end

   i=0;
   for (i=0, st=0; i<LRES_TOKENS_PER; i++)
   {
      if (tokens[i]==LRES_NO_TOKEN)
         st=1;
      if (st)
         *s++=0;
      else
         *s++=tokens[i];
   }
   *s++=(uchar)where;
   for (i=0, st=0; i<RN_NAME_LEN; i++)
   {
      if (name[i]=='\0')
         st=1;
      if (st)
         *s++=0;
      else
         *s++=name[i];
   }
}

extern "C" int rn_cur_paths;

// only called for NOWHERE - so need to try all wheres
rn_hnd _ResNametoHandleFast(char *name, char *tokens)
{
   int cur_where=rn_cur_paths-1;
   char try_me[20];
   rnHash *hsh;

   do {
      _build_rnHashBlock(try_me,cur_where,name,tokens);
      hsh=_ResNameHashRealLookup(try_me);
   } while ((hsh==NULL)&&(--cur_where>=0));
   return hsh?hsh->data:LRES_NO_HND;
}

void _ResNameHashStore(rn_hnd hnd)
{
   rn_name *nm=ResNameGetNameData(hnd);
   rnHash *pHash=(rnHash *)Malloc(sizeof(rnHash));
   _build_rnHashBlock(pHash->key,nm->where,nm->name,nm->tokens);

   pHash->data=hnd;
#ifdef DBG_ON
   if (_ResNameHashRealLookup(pHash->key))
      Warning(("rnHashStore: key already in hash table (rn %d - %s)\n",hnd,nm->name));
#endif
   _ResNameHashRealStore(pHash);
//   mprintf("RHS %x: %s %d %d %d %d - %d\n",hnd,
//      nm->name,nm->tokens[0],nm->tokens[1],nm->tokens[2],nm->tokens[3],nm->where);
}

void _ResNameHashFree(rn_hnd hnd)
{
   rn_name *nm=ResNameGetNameData(hnd);
   rnHash *hsh;
   char try_me[20];

//   mprintf("RHF %x: %s %d %d %d %d - %d\n",hnd,
//      nm->name,nm->tokens[0],nm->tokens[1],nm->tokens[2],nm->tokens[3],nm->where);

   _build_rnHashBlock(try_me,nm->where,nm->name,nm->tokens);
   hsh=_ResNameHashRealLookup(try_me);
   if (hsh && hsh->data==hnd)
      Free(rnHashTab.Remove(hsh));
   else
      Warning(("Hey - cant find myself to be freed\n"));
}
