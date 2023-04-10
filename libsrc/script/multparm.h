/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/multparm.h,v 1.13 1999/12/01 13:24:39 Justin Exp $
//
// Multiple Purpose Parameter
//
// Rules of ownership: the code that instantiates and calls functions using
// a multiparameter is responsible for clean-up
//
// If you're in C++, use the class. Note because there are no virtual functions
// here, you can always downcast from an sMultiParm to a cMultiParm

#ifndef __MULTPARM_H
#define __MULTPARM_H

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <matrixs.h>
#include <scrptobj.h>

// because multiparms can pass memory across script boundaries, they must be 
// allocated directly from the master heap
#include <allocapi.h>

#ifndef SCRIPT
#define MPAlloc(n) malloc(n)
#define MPFree(p)  free(p)
#else
EXTERN IMalloc * g_pAppAlloc;
#define MPAlloc(n) g_pAppAlloc->Alloc(n)
#define MPFree(p)  g_pAppAlloc->Free(p)
#endif

#pragma once

///////////////////////////////////////////////////////////////////////////////

enum eMultiParmType
{
   // All types sizeof(int) or less
   kMT_Undef,
   kMT_Int,
   kMT_Float,

   // All mallocd types
   kMT_String,
   kMT_Vector,

   // The object type, at the end for historical reasons
   kMT_Obj,

   kMT_TypeMax = 0xffffffff // force it use an int
};

typedef enum eMultiParmType eMultiParmType;

///////////////////////////////////////

struct sMultiParm
{
   union
   {
      int          i;
      ObjID        o;
      float        f;
      char *       psz;
      mxs_vector * pVec;
   };

   eMultiParmType type;
};

typedef struct sMultiParm sMultiParm;

///////////////////////////////////////

#define InitParm(p) \
   do \
   { \
      ((sMultiParm *)p)->type = kMT_Undef; \
   } while (0)

#define InitIntParm(p, val) \
   do \
   { \
      ((sMultiParm *)(p))->i = (int)(val); \
      ((sMultiParm *)(p))->type = kMT_Int; \
   } while (0)

#define InitObjParm(p, val) \
   do \
   { \
      ((sMultiParm *)(p))->o = val.Get(); \
      ((sMultiParm *)(p))->type = kMT_Obj; \
   } while (0)

#define InitFloatParm(p, val) \
   do \
   { \
      ((sMultiParm *)(p))->f = (float)(val); \
      ((sMultiParm *)(p))->type = kMT_Float; \
   } while (0)

#define InitStrParm(p, val) \
   do \
   { \
      ((sMultiParm *)(p))->psz = (char *)MPAlloc(strlen(val) + 1); \
      strcpy(((sMultiParm *)(p))->psz, val); \
      ((sMultiParm *)(p))->type = kMT_String; \
   } while (0)

#define InitVecParm(p, val) \
   do \
   { \
      ((sMultiParm *)(p))->pVec = (mxs_vector *)MPAlloc(sizeof(mxs_vector)); \
      memcpy(((sMultiParm *)(p))->pVec, &(val), sizeof(mxs_vector)); \
      ((sMultiParm *)(p))->type = kMT_Vector; \
   } while (0)

#define _IsMallocedParm(p) \
   ((((sMultiParm *)(p))->type >= kMT_String) && \
    (((sMultiParm *)(p))->type != kMT_Obj))

#define _FreeParmData(p) \
   do \
   { \
      if (_IsMallocedParm(p)) \
        MPFree(((sMultiParm *)(p))->psz); \
   } while (0)

#define CopyParm(p1, p2) \
   do \
   { \
      _FreeParmData(p1); \
      ((sMultiParm *)(p1))->type = ((sMultiParm *)(p2))->type; \
      if (((sMultiParm *)(p2))->type == kMT_String) \
      { \
         ((sMultiParm *)(p1))->psz = (char *)MPAlloc(strlen(((sMultiParm *)(p2))->psz) + 1); \
         strcpy(((sMultiParm *)(p1))->psz, ((sMultiParm *)(p2))->psz); \
      } \
      else if (((sMultiParm *)(p2))->type == kMT_Vector) \
      { \
         ((sMultiParm *)(p1))->pVec = (mxs_vector *)MPAlloc(sizeof(mxs_vector)); \
         memcpy(((sMultiParm *)(p1))->pVec, ((sMultiParm *)(p2))->pVec, sizeof(mxs_vector)); \
      } \
      else \
         ((sMultiParm *)(p1))->i = ((sMultiParm *)(p2))->i; \
   } while (0)

#define ClearParm(p) \
   do \
   { \
      _FreeParmData(p); \
      ((sMultiParm *)(p))->type = kMT_Undef; \
   } while (0)

///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus

class cMultiParm : public sMultiParm
{
public:
   cMultiParm()                     { InitParm(this); }
   cMultiParm(int i)                { InitIntParm(this, i); }
   cMultiParm(const cObj & o)       { InitObjParm(this, o); }
   cMultiParm(float f)              { InitFloatParm(this, f); }
   cMultiParm(double d)             { InitFloatParm(this, d); }
   cMultiParm(const char * s)       { InitStrParm(this, s); }
   cMultiParm(const mxs_vector & v) { InitVecParm(this, v); }
   cMultiParm(true_bool b)          { InitIntParm(this, b); }
   cMultiParm(const sMultiParm & m) { InitParm(this); CopyParm(this, &m); }
   cMultiParm(const cMultiParm & m) { InitParm(this); CopyParm(this, &m); }

   ////////////////////////////////////

   ~cMultiParm() { _FreeParmData(this); }

   ////////////////////////////////////

   const cMultiParm & operator=(int i)                { _FreeParmData(this); InitIntParm(this, i); return (*this); }
   const cMultiParm & operator=(cObj & o)             { _FreeParmData(this); InitObjParm(this, o); return (*this); }
   const cMultiParm & operator=(true_bool b)          { _FreeParmData(this); InitIntParm(this, b); return (*this); }
   const cMultiParm & operator=(float f)              { _FreeParmData(this); InitFloatParm(this, f); return (*this); }
   const cMultiParm & operator=(double d)             { _FreeParmData(this); InitFloatParm(this, d); return (*this); }
   const cMultiParm & operator=(const char * s)       { _FreeParmData(this); InitStrParm(this, s); return (*this); }
   const cMultiParm & operator=(const mxs_vector & v) { _FreeParmData(this); InitVecParm(this, v); return (*this); }
   const cMultiParm & operator=(const sMultiParm & m) { CopyParm(this, &m); return (*this); }
   const cMultiParm & operator=(const cMultiParm & m) { CopyParm(this, &m); return (*this); }

   ////////////////////////////////////

   operator int() const
   {
      int result;
      switch (type)
      {
         case kMT_Undef:
            result = 0;
            break;
         case kMT_Int:
            result = i;
            break;
         case kMT_Obj:
            result = o;
            break;
         case kMT_Float:
            result = f;
            break;
         case kMT_String:
            result = atoi(psz);
            break;
         case kMT_Vector:
            result = 0;
            break;
      }
      return result;
   }

   operator cObj & () const
   {
      gm_scratch.obj.Set(0);
      switch (type)
      {
         case kMT_Int:
            gm_scratch.obj.Set(i);
            break;
         case kMT_Obj:
            gm_scratch.obj.Set(o);
            break;
         case kMT_Undef:
         case kMT_Float:
         case kMT_String:
         case kMT_Vector:
            // Just leave it as OBJ_NULL
            break;
      }
      return gm_scratch.obj;
   }

   operator true_bool() const
   {
      return !!operator int();
   }

   operator float() const
   {
      float result;
      switch (type)
      {
         case kMT_Undef:
            result = 0.0;
            break;
         case kMT_Int:
            result = i;
            break;
         case kMT_Obj:
            result = o;
            break;
         case kMT_Float:
            result = f;
            break;
         case kMT_String:
            result = (float)atof(psz);
            break;
         case kMT_Vector:
            result = 0.0;
            break;
      }
      return result;
   }

   operator double() const
   {
      return operator float();
   }

   operator const char *() const
   {
      const char * pszResult;
      switch (type)
      {
         case kMT_Undef:
            gm_scratch.szBuf[0] = 0;
            pszResult = gm_scratch.szBuf;
            break;
         case kMT_Int:
            itoa(i, gm_scratch.szBuf, 10);
            pszResult = gm_scratch.szBuf;
            break;
         case kMT_Obj:
            itoa(o, gm_scratch.szBuf, 10);
            pszResult = gm_scratch.szBuf;
            break;
         case kMT_Float:
            _gcvt(f, 32, gm_scratch.szBuf);
            pszResult = gm_scratch.szBuf;
            break;
         case kMT_String:
            pszResult = psz;
            break;
         case kMT_Vector:
            VecToTempString();
            break;
      }
      return pszResult;
   }

   operator const mxs_vector & () const
   {
      if (type == kMT_Vector)
         return *pVec;
      memset(&gm_scratch.vec, 0, sizeof(mxs_vector));
      return gm_scratch.vec;
   }

   ////////////////////////////////////

   BOOL operator==(int i) const                { return operator int() == i; }
   BOOL operator==(const cObj & o) const       { return operator cObj &() == o; }
   BOOL operator==(true_bool b) const          { return operator true_bool() == b; }
   BOOL operator==(float f) const              { return operator float() == f; }
   BOOL operator==(double d) const             { return operator double() == d; }
   BOOL operator==(const char * s) const       { return !strcmp(operator const char *(), s);}
   BOOL operator==(const mxs_vector & v) const { const mxs_vector & thisV = operator const mxs_vector &(); return (thisV.x == v.x && thisV.y == v.y && thisV.z == v.z); }

   // Our equality test between multiparms allows comparisons between
   // floats and ints, but otherwise treats cMultiParms of different
   // types as not equal.  Is it possible we want to warn when the
   // types are not comparable?
   BOOL operator==(const cMultiParm & m) const
   {
      // For all types, we return from our switch rather than breaking.
      switch (type)
      {
         case kMT_Undef:
         {
            return FALSE;
         }
         case kMT_Int:
         {
            if (m.type == kMT_Int)
               return (i == m.i);
            if (m.type == kMT_Float)
               return ((float) i == m.f);
            if (m.type == kMT_Obj)
               return (i == m.o);
            return FALSE;
         }
         case kMT_Obj:
         {
            if (m.type == kMT_Obj)
               return (o == m.o);
            if (m.type == kMT_Int)
               return (o == m.i);
            return FALSE;
         }
         case kMT_Float:
         {
            if (m.type == kMT_Float)
               return (f == m.f);
            if (m.type == kMT_Int)
               return (f == (float) m.i);
            return FALSE;
         }
         case kMT_String:
         {
            if (m.type != kMT_String)
               return FALSE;
            return (strcmp(psz, m.psz) == 0);
         }
         case kMT_Vector:
         {
            if (m.type != kMT_Vector
             || m.pVec->x != pVec->x
             || m.pVec->y != pVec->y
             || m.pVec->z != pVec->z)
               return FALSE;
            return TRUE;
         }
         default:
         {
            return FALSE;
         }
      }
   }

   ////////////////////////////////////

   BOOL IsNull() const              { return (type == kMT_Undef); }
   eMultiParmType GetType() const   { return type; }
   void Clear()                     { ClearParm(this); }

private:
   union uScratch
   {
      char       szBuf[64];
      mxs_vector vec;
      cObj       obj;
   };

   static uScratch gm_scratch;
   void VecToTempString() const;
};

#define NULL_PARM cMultiParm()

#endif /* __cplusplus */

///////////////////////////////////////////////////////////////////////////////

#endif /* !__MULTPARM_H */

