/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/tagbase.h,v 1.2 2000/01/31 09:48:44 adurant Exp $
#pragma once

#ifndef __TAGBASE_H
#define __TAGBASE_H

#include <tagtype.h>
#include <str.h>
#include <tagconst.h>

typedef class cTagValue
{
public:
   void Set(long val);
   inline void Set(eTagType newType, long val) {type=newType; Set(val);};
   inline BOOL Set(int val) {if (type!=kTagInt) return FALSE; intVal = val; return TRUE;};
   inline BOOL Set(float val) {if (type!=kTagFloat) return FALSE; floatVal = val; return TRUE;};
   long Get() const;
   void Move(ITagFile *pFile);
   inline void SetType(eTagType newType) {type=newType;};
   inline eTagType Type() const {return type;};
private:
   eTagType type;              
   union
   {
      int intVal;
      float floatVal;
   };
} cTagValue;

typedef struct sTag
{
   cStr name;
   eTagFlags flags;            
   cTagValue val;
} sTag;

typedef struct sTagConstraint
{
   eTagOp op;    
   sTag *pTag;
   cTagValue value;
   sTagConstraint *pNext;
} sTagConstraint;

#endif


