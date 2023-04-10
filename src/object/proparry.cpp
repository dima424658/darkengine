/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/proparry.cpp,v 1.11 1997/12/22 18:25:06 mahk Exp $

#include <lg.h>
#include <proparry.h>
#include <osysbase.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
//  
// cPImpCompactArray
//
////////////////////////////////////////////////////////////


//============================================================
// Construction and deconstruction
//

void cPImpCompactArray::NullOut()
{
   for (int i = min_idx; i < vecsize; i++)
   {
      elem_vec[i] = NULL;
      id_vec[i] = OBJ_NULL;
   }
}

cPImpCompactArray::cPImpCompactArray(const sCompactArrayDesc* desc, cPropertyImpl* impl, BOOL sponsor)
   : sCompactArrayDesc(*desc),
     id2idx_(impl),
     delete_(sponsor),
     maxidx_(0),
     cPropertyImpl(desc->elemsize)
{
   Assrt(id2idx_->ElemSize() >= sizeof(int));
   NullOut();
}

////////////////////////////////////////

cPImpCompactArray::cPImpCompactArray(const sCompactArrayDesc* desc, ePropertyImpl impl) 
   : sCompactArrayDesc(*desc),
     id2idx_(CreatePropertyImpl(impl,sizeof(int))),
     delete_(TRUE),
     cPropertyImpl(desc->elemsize),
     maxidx_(0)
{
   NullOut();
}

////////////////////////////////////////

cPImpCompactArray::~cPImpCompactArray()
{
   if (delete_)
      delete id2idx_;
}


//============================================================
// cPropertyImpl methods
//

BOOL cPImpCompactArray::Get(ObjID obj, PropertyID prop, void** pptr) const 
{
   int idx = ObjID2idx(obj,prop);
   if (idx >= 0)
   {
      *pptr = elem_vec[idx];
      return TRUE;
   }
   return FALSE;
}

////////////////////////////////////////

BOOL cPImpCompactArray::Set(ObjID obj, PropertyID prop, void* ptr) 
{
   BOOL retval = FALSE;
   int idx = ObjID2idx(obj,prop);
   if (idx < 0)
   {
      idx = AddObj(obj,prop);
      retval = TRUE;
   }
   if (idx >= 0)
   {
      void* elem = elem_vec[idx];
      Copy(obj,elem,ptr);
   }
   return retval;
}

////////////////////////////////////////

BOOL cPImpCompactArray::Delete(ObjID obj, PropertyID prop)
{
   int idx = ObjID2idx(obj,prop);
   if (idx >= 0)
   {
      Term(obj,elem_vec[idx]);
      FreeElem(elem_vec[idx]);
      elem_vec[idx] = NULL;
      id_vec[idx] = OBJ_NULL;
      id2idx_->Delete(obj,prop);
   }
   return idx >= 0;
}

////////////////////////////////////////

void* cPImpCompactArray::Create(ObjID obj , PropertyID prop)
{
   int i = AddObj(obj,prop);
   return elem_vec[i];
}



////////////////////////////////////////

void cPImpCompactArray::Reset(PropertyID id)
{
   sPropertyObjIter iter;
   ObjID obj;
   int* idx;

   // Free and null out elements in the arrat
   id2idx_->IterStart(id,&iter);

   while(id2idx_->IterNext(id,&iter,&obj,(void**)&idx))
   {
      if (elem_vec[*idx] != NULL)
      {
         Term(obj,elem_vec[*idx]);
         FreeElem(elem_vec[*idx]);
      }
      elem_vec[*idx] = NULL;
      id_vec[*idx] = OBJ_NULL;
   }

   // Now wipe out the id mapping
   id2idx_->Reset(id);
}


////////////////////////////////////////

void cPImpCompactArray::IterStart(PropertyID id, sPropertyObjIter* iter) const
{
   id2idx_->IterStart(id,iter);
}

////////////////////////////////////////

BOOL cPImpCompactArray::IterNext(PropertyID id, sPropertyObjIter* iter, ObjID* next, void** value) const
{
   int* idx;
   BOOL retval = id2idx_->IterNext(id,iter,next,(void**)&idx);
   if (retval)
   {
      *value = elem_vec[*idx]; 
   }
   return retval;
}

////////////////////////////////////////

void cPImpCompactArray::IterStop(PropertyID id, sPropertyObjIter* iter) const
{
   id2idx_->IterStart(id,iter);
}

////////////////////////////////////////

BOOL cPImpCompactArray::Relevant(ObjID obj, PropertyID prop) const
{
   return id2idx_->Relevant(obj,prop);
}

//============================================================
// ALLOCATION
//

void* cPImpCompactArray::NewElem(ObjID, int )
{
   return new char[elem_size_];
}

void cPImpCompactArray::FreeElem(void* elem)
{
   delete [] (char*) elem;
}


//============================================================
// Private methods
//

int cPImpCompactArray::AddObj(ObjID obj, PropertyID propid)
{
   for (int i = min_idx; i < vecsize; i++)
   {
      if (elem_vec[i] == NULL)
      {
         id_vec[i] = obj;
         id2idx_->Set(obj,propid,&i);
         elem_vec[i] = NewElem(obj,i);
         Init(obj,elem_vec[i]);
         if (i >= maxidx_)
            maxidx_ = i + 1;
         return i; 
      }
   }
   return -1;
}




//============================================================
// C INTERFACE ROUTINES 
//

int ObjID2IDX(sCompactArrayProp* array, ObjID id)
{
   cPImpCompactArray* a = (cPImpCompactArray*)array->impl;

   return a->ObjID2idx(id,array->id);
}

////////////////////////////////////////

int CompactArrayMaxIdx(sCompactArrayProp* array)
{
   cPImpCompactArray* a = (cPImpCompactArray*)array->impl;

   return a->MaxIdx();
}


