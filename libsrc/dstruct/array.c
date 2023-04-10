#include <lg.h>
#include <string.h>
#include <array.h> 


#define FREELIST_EMPTY   -1
#define FREELIST_NOTFREE -2

errtype array_init(Array* initme, int elemsize, int vecsize)
{
   if (elemsize == 0) return ERR_RANGE;
   initme->elemsize = elemsize;
   initme->vecsize = vecsize;
   initme->fullness = 0;
   initme->freehead = FREELIST_EMPTY;
   initme->vec = (char*) Malloc(elemsize*vecsize+vecsize*sizeof(int));
   if (initme->vec == NULL) return ERR_NOMEM;
   initme->freevec = (int*)(initme->vec + elemsize*vecsize);
   return OK;
}

errtype array_grow(Array *a, int size)
{
   char* tmpvec;
   int* tmplist; 
   if (size <= a->vecsize) return OK;
   tmpvec = Malloc(a->elemsize*size + size*sizeof(int));
   if (tmpvec == NULL) return ERR_NOMEM;
   memcpy(tmpvec,a->vec,a->vecsize*a->elemsize);
   tmplist = (int*)(tmpvec + a->elemsize*size); 
   memcpy(tmplist,a->vec,a->vecsize*sizeof(int));
   Free(a->vec);
   a->vecsize = size;
   a->vec = tmpvec;
   a->freevec = tmplist;
   return OK;
}

errtype array_newelem(Array* a, int* index)
{
   if (a->freehead != FREELIST_EMPTY)
   {
      *index = a->freehead;
      a->freehead = a->freevec[*index];
      a->freevec[*index] = FREELIST_NOTFREE;
      return OK;
   }
   if (a->fullness >= a->vecsize)
   {
      errtype err = array_grow(a,a->vecsize*2);
      if (err != OK) return err;
   }
   *index = a->fullness++;
   a->freevec[*index] = FREELIST_NOTFREE;
   return OK;
}


errtype array_dropelem(Array* a, int index)
{
   if (index >= a->fullness || a->freevec[index] != FREELIST_NOTFREE) return ERR_NOEFFECT; // already freed. 
   a->freevec[index] = a->freehead;
   a->freehead = index;
   return OK;
}

errtype array_destroy(Array* a)
{
   a->elemsize = 0;
   a->vecsize = 0;
   a->freehead = FREELIST_EMPTY;
   if (a->vec != NULL)
      Free(a->vec);
   a->vec = NULL;
   return OK;
}


void array_iter_init(Array* a, array_iter* iter)
{
   int idx = 0;

   for (;idx < a->fullness; idx++)
      if (a->freevec[idx] == FREELIST_NOTFREE)
   	   break;
   iter->a = a;
   iter->next = idx;
}

int array_iter_next(array_iter* iter)
{
   Array* a = iter->a;
   int idx;

   for (idx = iter->next+1; idx < a->fullness; idx++)
      if (a->freevec[idx] == FREELIST_NOTFREE)
   	   break;
   iter->next = idx;
   return idx;
}


int array_iter_curr(array_iter* iter)
{
   return iter->next;
}

bool array_iter_done(array_iter* iter)
{
   return iter->next >= iter->a->fullness;
}


