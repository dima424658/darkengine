/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/object/prcniter.h,v 1.3 2000/02/24 23:41:01 mahk Exp $
//

#ifndef __PRCNITER_H
#define __PRCNITER_H

#include <string.h>

#include <property.h>
#include <propbase.h>
#include <slistpp.h>
#include <osysbase.h>

#pragma once
#pragma pack(4)

F_DECLARE_INTERFACE(ITrait);
F_DECLARE_INTERFACE(IObjectQuery);

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cConcreteIter
//
// Iterates across concrete objects with a property
//

class cConcreteIter
{
public:
   cConcreteIter(IProperty * pProperty);
   ~cConcreteIter();
   
   void Start(); 
   BOOL Next(ObjID *);
   void Stop();

private:
   enum
   {
      kMaxObjs = HACK_MAX_OBJ,
   };

   BOOL RawNext(ObjID *);
   
   IProperty *      m_pProperty;
   ITrait *         m_pTrait;
   IObjectQuery *   m_pObjectQuery;
   sPropertyObjIter m_iter;
   sbool            m_visits[kMaxObjs];
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cOptConcreteIter
//
// Maintains a list for iterating across concrete objects with a property. One
// must keep a copy of the iterator around for it to really be optimized.
//

#if 0
class cOptConcreteIter;
struct sOptConcreteIterNode;

typedef cSList<cOptConcreteIter, 1>         cOptConcreteIterBase;
typedef cSListNode<sOptConcreteIterNode, 1> sOptConcreteIterNodeBase;

///////////////////////////////////////

struct sOptConcreteIterNode : private sOptConcreteIterNodeBase
{
private:
   friend class cOptConcreteIter;
   
   ObjID object;
};

///////////////////////////////////////

class cOptConcreteIter : private cOptConcreteIterBase
{
public:
   cOptConcreteIter(IProperty * pProperty);
   ~cOptConcreteIter();
   
   void Start(); 
   BOOL Next(ObjID *);
   void Stop();
};
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cConcreteIter, inline functions
//

inline cConcreteIter::cConcreteIter(IProperty * pProperty)
 : m_pProperty(pProperty),
   m_pTrait(NULL),
   m_pObjectQuery(NULL)
{
   m_pProperty->AddRef();
   m_pProperty->QueryInterface(IID_ITrait, (void **)&m_pTrait);
}

///////////////////////////////////////

inline cConcreteIter::~cConcreteIter()
{
   m_pProperty->Release();
   SafeRelease(m_pTrait);
}

///////////////////////////////////////

inline void cConcreteIter::Start()
{
   m_pProperty->IterStart(&m_iter);
   memset(m_visits, 0, sizeof(m_visits));
}

///////////////////////////////////////

inline BOOL cConcreteIter::Next(ObjID * pNext)
{
   do
   {
      if (!RawNext(pNext))
         return FALSE;
   } while (OBJ_IS_ABSTRACT(*pNext) || m_visits[*pNext]);
   
   m_visits[*pNext] = TRUE;
   return TRUE;
}

///////////////////////////////////////

inline void cConcreteIter::Stop()
{
   m_pProperty->IterStop(&m_iter);
   SafeRelease(m_pObjectQuery);
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__PRCNITER_H */
