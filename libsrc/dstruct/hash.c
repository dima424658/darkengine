#include <lg.h> 
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <hash.h> 
#include <mprintf.h>
#include <_dstruct.h>
/*
 * $Source: x:/prj/tech/libsrc/dstruct/RCS/hash.c $
 * $Revision: 1.7 $
 * $Author: mahk $
 * $Date: 1997/01/13 15:46:02 $
 *
 * $Log: hash.c $
 * Revision 1.7  1997/01/13  15:46:02  mahk
 * Fixed primacy test for perfect squares (oops!)
 * 
 * Revision 1.6  1996/08/15  19:11:25  mahk
 * Fixed prime test.
 * 
 * Revision 1.5  1994/01/18  08:16:06  mahk
 * Added hash_copy
 * 
 * Revision 1.4  1993/08/09  17:14:59  mahk
 * Fixed the libdbg.h thing.
 * 
 * Revision 1.3  1993/07/01  12:16:22  mahk
 * changed log2 to hashlog2 to avoid name conflict
 * 
 * Revision 1.2  1993/06/14  21:11:45  xemu
 * step func
 * 
 * Revision 1.1  1993/03/25  19:15:09  mahk
 * Initial revision
 * 
 *
 */

#define HASH_EMPTY     0
#define HASH_TOMBSTONE 1
#define HASH_FULL      2

#define INDEX_NOT_FOUND -1

#define FULLNESS_THRESHHOLD_PERCENT 80

#define ELEM(tbl,i)  ((void*)((tbl)->vec + (i)*(tbl)->elemsize))

int hashlog2(int x)
{
   if (x < 2) return 0;
   return 1+hashlog2(x/2);
}


int expmod(int b, int e, uint m)
{
   if (e == 0) return 1;
   if (e%2 == 0)
   {
      int tmp = expmod(b,e/2,m);
      return (tmp*tmp)%m;
   }
   else
   {
      int tmp = expmod(b,e-1,m);
      return (b*tmp)%m;
   }

}

bool is_fermat_prime(uint n, uint numtests)
{
   int i;
   if (n < 3) return FALSE;
   for (i = 0; i < numtests; i++)
   {
      int a = rand()%(n-2) + 2;
      if (expmod(a,n,n) != a) return FALSE;
   }
   return TRUE;
}


#define NUM_FERMAT_TESTS 30

static bool is_prime(uint n)
{
   if (n > NUM_FERMAT_TESTS*NUM_FERMAT_TESTS)
      return is_fermat_prime(n,NUM_FERMAT_TESTS);
   else
   {
      int i;
      for (i = 2; i*i <= n; i++)
         if ((n/i)*i == n)
            return FALSE;
   }
   return TRUE;
}

errtype hash_init(Hashtable* h, int elemsize, int vecsize, Hashfunc hfunc, Equfunc efunc)
{
   int i;
   Spew(DSRC_DSTRUCT_Hash,("hash_init(%x,%d,%d,%x,%x)\n",h,elemsize,vecsize,hfunc,efunc));
   while(!is_prime(vecsize)) vecsize++;
   h->elemsize = elemsize;
   h->size = vecsize;
   h->sizelog2 = hashlog2(vecsize);
   h->fullness = 0;
   h->hfunc = hfunc;
   h->efunc = efunc;
   h->statvec = (char*)Malloc(vecsize);
   if (h->statvec == NULL) return ERR_NOMEM;
   for (i = 0; i < vecsize; i++) h->statvec[i] = HASH_EMPTY;
   h->vec = (char*)Malloc(elemsize*vecsize);
   if (h->vec == NULL) return ERR_NOMEM;
   return OK;
}

errtype hash_copy(Hashtable* t, Hashtable* s)
{
   *t = *s;
   t->statvec = Malloc(t->size);
   if (t->statvec == NULL) return ERR_NOMEM;
   t->vec = Malloc(t->elemsize*t->size);
   if (t->vec == NULL) return ERR_NOMEM;
   memcpy(t->vec,s->vec,t->size*t->elemsize);
   memcpy(t->statvec,s->statvec,t->size);
   return OK;
}

static bool find_elem(Hashtable* h, void* elem, int* idx)
{
   bool found = FALSE;
   int hash = h->hfunc(elem);
   int index,j;
   Spew(DSRC_DSTRUCT_Hash,("find_elem(%x,%x,%x) hash is %d\n",h,elem,idx,hash));

//   mprintf ("hash is %d\n size is %d\n hash%size is %d\n",hash,h->size,hash%h->size);

   for (j = 0, index = hash%h->size;  j < h->size && h->statvec[index] != HASH_EMPTY;
         j++,index = (index + (1 << hash%h->sizelog2)) % h->size)
   {
      void* myelem = (void*) ELEM(h,index);
//      mprintf("comparing %x to %x using %x\n",elem,myelem,h->efunc);
      if (h->statvec[index] == HASH_FULL && h->efunc(elem,myelem) == 0)
      {
         found = TRUE;
         break;
      }
   }
   *idx = index;
   Spew(DSRC_DSTRUCT_Hash,("find_elem(): index is %d \n",index));
   return found;
}

static int find_index(Hashtable* h, void* elem)
{
   int hash = h->hfunc(elem);
   int j;
   int index;
   Spew(DSRC_DSTRUCT_Hash,("find_index(%x,%x) hash is %d\n",h,elem,hash));
   for (j = 0, index = hash%h->size;  j < h->size && h->statvec[index] == HASH_FULL;
       j++,index = (index + (1 << hash%h->sizelog2)) % h->size)
         Spew(DSRC_DSTRUCT_Hash,("find_index(): found status %d\n",h->statvec[index]));
   if (j >= h->size) 
   {
      Warning(("Hash table index not found!\n"));
      index = INDEX_NOT_FOUND;
   }
   Spew(DSRC_DSTRUCT_Hash,("find_index(): result is %d\n",index));
   return index;
}

static errtype grow(Hashtable* h, int newsize)
{
   char* oldvec = h->vec;
   char* oldstat = h->statvec;
   char *newvec, *newstat;
   int oldsize = h->size;
   int i;
   Spew(DSRC_DSTRUCT_Hash,("grow(%x,%d)\n",h,newsize));
   for (;!is_prime(newsize);newsize++);
   newvec = Malloc(newsize*h->elemsize);
   if (newvec == NULL) return ERR_NOMEM;
   newstat = Malloc(newsize);
   if (newstat == NULL)
   {
      Free (newvec);
      return ERR_NOMEM;
   }
   h->vec = newvec;
   h->statvec = newstat;
   h->size = newsize;
   h->sizelog2 = hashlog2(newsize);
   h->fullness = 0;
   for (i = 0; i < newsize; i++) newstat[i] = HASH_EMPTY;
   for (i = 0; i < oldsize; i++)
   {
      if (oldstat[i] == HASH_FULL)
      {
         hash_insert(h,(void*)(oldvec+i*h->elemsize));
      }
   }
   Free(oldvec);
   Free(oldstat);
   return OK;
}

errtype hash_set(Hashtable* h, void* elem)
{
   int i;
   Spew(DSRC_DSTRUCT_Hash,("hash_set(%x,%x)\n",h,elem));
   if (h->fullness*100/h->size > FULLNESS_THRESHHOLD_PERCENT)
      grow(h,h->size*2);
   if (!find_elem(h,elem,&i))
      i = find_index(h,elem);
   memcpy(ELEM(h,i),elem,h->elemsize);
   h->statvec[i] = HASH_FULL;
   h->fullness++;
   return OK;
}

errtype hash_insert(Hashtable* h, void* elem)
{
   int i;
   Spew(DSRC_DSTRUCT_Hash,("hash_insert(%x,%x)\n",h,elem));
   if (h->fullness*100/h->size > FULLNESS_THRESHHOLD_PERCENT)
      grow(h,h->size*2);
   i = find_index(h,elem);
   memcpy(ELEM(h,i),elem,h->elemsize);
   h->statvec[i] = HASH_FULL;
   h->fullness++;
   return OK;
}


errtype hash_delete(Hashtable* h, void* elem)
{
   int i;
   Spew(DSRC_DSTRUCT_Hash,("hash_delete(%x,%x)\n",h,elem));
   if (find_elem(h,elem,&i))
   {
      h->statvec[i] = HASH_TOMBSTONE;
      return OK;
   }
   return ERR_NOEFFECT;
}


errtype hash_lookup(Hashtable* h, void* elem, void** result)
{
   int i;
   Spew(DSRC_DSTRUCT_Hash,("hash_lookup(%x,%x,%x)\n",h,elem,result));
//   mprintf("Hash_lookup hashfunc = %x",h->hfunc);
   if (find_elem(h,elem,&i))
   {
      *result = ELEM(h,i);
   }
   else *result = NULL;
   Spew(DSRC_DSTRUCT_Hash,("hash_lookup(): value is %x\n",*result));
   return OK;
}

errtype hash_iter(Hashtable* h, HashIterFunc ifunc, void* data)
{
   int i;
   for (i = 0; i < h->size; i++)
      if (h->statvec[i] == HASH_FULL)
         if (ifunc(ELEM(h,i),data))
            break;
   return OK;
}

errtype hash_step(Hashtable *h, void **result, int *index)
{
   while ((h->statvec[*index] != HASH_FULL) && (*index < h->size))
      (*index)++;
   if (*index == h->size)
      *result = NULL;
   else
      *result = ELEM(h,*index);
   (*index)++;
   return(OK);
}

errtype hash_destroy(Hashtable* h)
{
   h->size = 0;
   h->fullness = 0;
   Free(h->statvec);
   Free(h->vec);
   return OK;
}

