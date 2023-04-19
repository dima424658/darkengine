///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/dlist.h $
// $Author: TOML $
// $Date: 1998/02/23 10:46:47 $
// $Revision: 1.9 $
//
// (c) Copyright 1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//

#ifndef __DLIST_H
#define __DLIST_H

#include <lgassert.h>
#include <templexp.h>

#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// ===================
// Doubly-linked lists
// ===================
//
// This file defines classes to support doubly-linked lists. There are
// two kinds of lists: ones where the list item is considered intrisically
// "listed," where derivation can be used, or the item is an type
// which is not necessarily "listed", but is occationally.
//
// Ownership
// ---------
//
// A cDList does not automatically clean up  after itself when it is destroyed:
// that is, the cDList destructor does nothing.  The reason for this is that
// nodes may be threaded onto multiple lists.
//
// A cDList that owns its data nodes should provide a destructor which calls
// cDList::DestroyAll to delete all the data nodes.  Also, a cDList can call
// cDList::Clear to zero the link pointers in the forward and backward links.
// For example:
//
// class MyList : public cDList<MyThing,1>
// {
// public:
//     ~MyList() { DestroyAll(); }
// };
//
// Membership
// ----------
//
// It is not possible to tell if a node is on a list or not.  If a node's
// links are zero, it may be the only element on a list, or it may not
// be on a list.  Also, unless cDList::Clear is called, a node's links may be
// not equal to kDListInvalidPtr even if it is not on a list.
//
// Usage 1: Intrinsically Listed Types
// -----------------------------------
//
// The cDListNode parameterized class template is used as a base type from
// which to derive a data class.
//
// The cDList parameterized class template is used to define a list of a
// particular element type; it requires two formals, the node class it will list
// and the a numeral to disambiguate similar instantiations.
//
// Normal style is to declare two typedefs.  For example:
//
// class cMyThing;
// typedef cDList<cMyThing, 1> cMyList;
// typedef cDListNode<cMyThing, 1> cMyThingNode;
//
// class cMyThing : public cMyThingNode
// {
// public:
//      char Name[32];
// };
//
// void foo()
// {
//     cMyList list;
//     cMyThing * pMyThing = new cMyThing;
//     list.Prepend(pMyThing);
// }
//
// Usage 2: Occationally Listed Types
// ----------------------------------
//
// For cases when the type to be listed cannot or does not want to carry
// the node pointers directy, use the "container" form of the DList:
//
// class cMyThing
// {
// public:
//     char Name[32];
// };
//
// typedef cContainerDList<cMyThing *, 0> cMyList;
// typedef cContDListNode<cMyThing *, 0> cMyNode;
//
// void foo()
// {
//     cMyList list;
//     cMyNode * pMyNode = new cMyNode;
//     pMyNode->item = new cMyThing;
//     list.Prepend(pMyNode);
// }
//

#define __CPPTOOLSAPI

#define kDListInvalidPtr ((cDListNodeBase *)0xffffffff)
#ifndef SHIP
extern const char * g_pszDlistInsertError;
#define ValidateNodeInsert(p) AssertMsg((p)->m_pNext == kDListInvalidPtr && (p)->m_pPrevious == kDListInvalidPtr, g_pszDlistInsertError);
#define InvalidateNode(p) ((p)->m_pNext = (p)->m_pPrevious = kDListInvalidPtr)
#else
#define ValidateNodeInsert(p)
#define InvalidateNode(p)
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Generic expansion mechanism.
// @Q (toml 06-24-97): is this used, does it work?
//

#define DLIST_EXPAND(type, id) \
    TEMPLATE_EXPAND2(cDList, type, id); \
    TEMPLATE_EXPAND2(cDListNode, type, id)


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDListNodeBase
//
// List node base class
//

class __CPPTOOLSAPI cDListNodeBase
{
friend class cDListBase;

public:
    cDListNodeBase * GetNext() const;
    cDListNodeBase * GetPrevious() const;
    long             CountBackwards();

protected:
    cDListNodeBase();

private:
    cDListNodeBase * m_pNext;
    cDListNodeBase * m_pPrevious;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDListBase
//
// List template base class
//

class __CPPTOOLSAPI cDListBase
{
public:
    void Clear();
    void SetEmpty();

#ifndef __WATCOMC__
    // @Note (toml 05-18-96): The Watcom 10.5 parser has a bug handling
    // declarations of instances of cDListNodeBase derivations,
    // but only when optimizing is enabled (!). It claims a client of
    // a class derived from cDListBase cannot create an instance of
    // that derivation because this constructor is protected.  This
    // is incorrect behavior and should be tested against future versions
    // of Watcom.  Microsoft and Symantec handle it ok.
protected:
#endif
    cDListBase();

protected:
    cDListNodeBase * GetFirst() const;
    cDListNodeBase * GetLast() const;

    void             Append(cDListNodeBase *);
    void             Prepend(cDListNodeBase *);
    cDListNodeBase * Remove(cDListNodeBase *);
    void             InsertBefore(cDListNodeBase *, cDListNodeBase *);
    void             InsertAfter(cDListNodeBase *, cDListNodeBase *);

private:
    cDListNodeBase * m_pFirst;
    cDListNodeBase * m_pLast;
};

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cDListNode
//
// Template for the base class of a list node of type NODE
//

template <class NODE, int ID>
class cDListNode : public cDListNodeBase
{
public:
    typedef NODE * cNodePtr;
    typedef NODE & cNodeRef;

    NODE * GetNext() const;
    NODE * GetPrevious() const;
};

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cDList
//
// Template for a list of nodes of type NODE
//

template <class NODE, int ID>
class cDList : public cDListBase
{
public:
    typedef NODE * cNodePtr;

    void   DestroyAll();
    void   Append(NODE *);
    void   Prepend(NODE *);
    NODE * Remove(NODE *);
    NODE * GetFirst() const;
    NODE * GetLast() const;
    void   InsertBefore(NODE *, NODE *);
    void   InsertAfter(NODE *, NODE *);
};

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cContDListNode
//

template <class CONTAINED_TYPE, int ID>
class cContDListNode : public cDListNode< cContDListNode<CONTAINED_TYPE, ID>, ID>
{
public:
   CONTAINED_TYPE item;
};

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cContainerDList
//

template <class CONTAINED_TYPE, int ID>
class cContainerDList : public cDList< cContDListNode<CONTAINED_TYPE, ID>, ID>
{
};

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cSerialDList
//
// Serializable list
//

class cIStore;
class cOStore;

template <class NODE, int ID>
class cSerialDList : public cDList<NODE, ID>
{
public:
    BOOL FromStream(cIStore &);
    BOOL ToStream(cOStore &) const;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDListNodeBase, inline functions
//

inline cDListNodeBase::cDListNodeBase()
  : m_pNext(kDListInvalidPtr),
    m_pPrevious(kDListInvalidPtr)
{
    // The next and previous links are set to bogus pointers
}

///////////////////////////////////////

inline cDListNodeBase *cDListNodeBase::GetNext() const
{
    return m_pNext;
}

///////////////////////////////////////

inline cDListNodeBase *cDListNodeBase::GetPrevious() const
{
    return m_pPrevious;
}

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cDListNode, inline functions
//

template <class NODE, int ID>
inline NODE *cDListNode<NODE, ID>::GetNext() const
{
    return (NODE *)(cDListNode<NODE, ID> *)cDListNodeBase::GetNext();
}

///////////////////////////////////////

template <class NODE, int ID>
inline NODE *cDListNode<NODE, ID>::GetPrevious() const
{
    return (NODE *)(cDListNode<NODE, ID> *)cDListNodeBase::GetPrevious();
}


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDListBase, inline functions
//

inline void cDListBase::SetEmpty()
{
    m_pFirst = m_pLast = 0;
}

///////////////////////////////////////

inline cDListBase::cDListBase()
{
    SetEmpty();
}

///////////////////////////////////////

inline cDListNodeBase *cDListBase::GetFirst() const
{
    return m_pFirst;
}

///////////////////////////////////////

inline cDListNodeBase *cDListBase::GetLast() const
{
    return m_pLast;
}

///////////////////////////////////////
//
// Prepend a node to the beginning of this list
//
inline void cDListBase::Prepend(cDListNodeBase *pNode)
{
    ValidateNodeInsert(pNode);
    
    // If the list contains something...
    if (m_pFirst)
        // ...then link the first node in this list after the new node
        m_pFirst->m_pPrevious = pNode;
    else
        // ...else set the tail of this list to the new node
        m_pLast = pNode;

    pNode->m_pNext = m_pFirst;
    pNode->m_pPrevious = 0;
    m_pFirst = pNode;
}

///////////////////////////////////////
//
// Append a node to the end of this list
//

inline void cDListBase::Append(cDListNodeBase * pNode)
{
    ValidateNodeInsert(pNode);

    // If the list contains something...
    if (m_pLast)
        // ...then link the last node in this list before the new node
        m_pLast->m_pNext = pNode;
    else
        // ...else set the head of the list to the new node
        m_pFirst = pNode;

    pNode->m_pPrevious = m_pLast;
    pNode->m_pNext = 0;
    m_pLast = pNode;
}

///////////////////////////////////////
//
// Remove a node from this list
//

inline cDListNodeBase *cDListBase::Remove(cDListNodeBase *pNode)
{
    if (pNode->m_pNext)
        pNode->m_pNext->m_pPrevious = pNode->m_pPrevious;
    else
        m_pLast = pNode->m_pPrevious;

    if (pNode->m_pPrevious)
        pNode->m_pPrevious->m_pNext = pNode->m_pNext;
    else
        m_pFirst = pNode->m_pNext;

    InvalidateNode(pNode);

    return pNode;
}

///////////////////////////////////////
//
// Insert a node before the specified node
//

inline void cDListBase::InsertBefore(cDListNodeBase *pAfter, cDListNodeBase *pNode)
{
    ValidateNodeInsert(pNode);
    
    pNode->m_pNext = pAfter;
    pNode->m_pPrevious = pAfter->m_pPrevious;

    if (pAfter->m_pPrevious)
        pAfter->m_pPrevious->m_pNext = pNode;
    else
        m_pFirst = pNode;

    pAfter->m_pPrevious = pNode;
}

///////////////////////////////////////
//
// Insert a node after another node
//

inline void cDListBase::InsertAfter(cDListNodeBase *pBefore, cDListNodeBase *pNode)
{
    ValidateNodeInsert(pNode);
    
    pNode->m_pPrevious = pBefore;
    pNode->m_pNext = pBefore->m_pNext;

    if (pBefore->m_pNext)
        pBefore->m_pNext->m_pPrevious = pNode;
    else
        m_pLast = pNode;

    pBefore->m_pNext = pNode;
}

///////////////////////////////////////

inline void cDListBase::Clear()
{
    cDListNodeBase * p;
    while ((p = GetFirst()) != NULL)
    {
        Remove(p);
        p->m_pNext = p->m_pPrevious = kDListInvalidPtr;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cDList, inline functions
//

template <class NODE, int ID>
inline void cDList<NODE, ID>::Append(NODE *Node)
{
    cDListBase::Append((cDListNode<NODE, ID> *)Node);
}

///////////////////////////////////////

template <class NODE, int ID>
inline void cDList<NODE, ID>::Prepend(NODE *Node)
{
    cDListBase::Prepend((cDListNode<NODE, ID> *)Node);
}

///////////////////////////////////////

template <class NODE, int ID>
inline NODE *cDList<NODE, ID>::Remove(NODE *Node)
{
    return (NODE *)(cDListNode<NODE, ID> *)cDListBase::Remove((cDListNode<NODE, ID> *)Node);
}

///////////////////////////////////////

template <class NODE, int ID>
inline NODE *cDList<NODE, ID>::GetFirst() const
{
    return (NODE *)(cDListNode<NODE, ID> *)cDListBase::GetFirst();
}

///////////////////////////////////////

template <class NODE, int ID>
inline NODE *cDList<NODE, ID>::GetLast() const
{
    return (NODE *)(cDListNode<NODE, ID> *)cDListBase::GetLast();
}

///////////////////////////////////////

template <class NODE, int ID>
inline void cDList<NODE, ID>::InsertAfter(NODE *Before, NODE *Node)
{
    cDListBase::InsertAfter((cDListNode<NODE, ID> *)Before, (cDListNode<NODE, ID> *)Node);
}

///////////////////////////////////////

template <class NODE, int ID>
inline void cDList<NODE, ID>::InsertBefore(NODE *After, NODE *Node)
{
    cDListBase::InsertBefore((cDListNode<NODE, ID> *)After, (cDListNode<NODE, ID> *)Node);
}

////////////////////////////////////////////////////////////
//
// "SIMPLE" DLIST TEMPLATE
//
// Wrapper around dlist.h
//
////////////////////////////////////////////////////////////

template <class ELEM> class cSimpleDList;
template <class ELEM> class cSimpleDListNode;
template <class ELEM> class cSimpleDListIter;

//------------------------------------------------------------
// LIST NODE
//

template <class ELEM>
class cSimpleDListNode : public cDListNode<cSimpleDListNode<ELEM>,1>
{
   ELEM Elm;
public:
   cSimpleDListNode(const ELEM& elem)
      : Elm(elem) {};
   operator ELEM() { return Elm;};
   ELEM& Value() { return Elm;};

};

//------------------------------------------------------------
// LIST ITERATOR
//

template <class ELEM>
class cSimpleDListIter
{
   typedef cSimpleDListNode<ELEM> cNode;
   cNode* ThisNode;
   cNode* NextNode;
   friend class cSimpleDList<ELEM>;

   cSimpleDListIter(cNode* node)
      : ThisNode(node),
        NextNode((node != NULL) ? node->GetNext() : NULL)
   {};
public:
   cSimpleDListIter() :ThisNode(NULL),NextNode(NULL) {};
   BOOL Done() const { return ThisNode == NULL;};
   ELEM& Value() const { return ThisNode->Value();};
   cNode& Node() const { return *ThisNode;};
   void Next()
   {
      if (ThisNode) ThisNode = NextNode;
      if (NextNode) NextNode = NextNode->GetNext();
   };
};

//------------------------------------------------------------
// THE LIST PROPER
//

template <class ELEM>
class cSimpleDList : public cDList<cSimpleDListNode<ELEM>,1>
{

public:
   // for convenience
   typedef cDList<cSimpleDListNode<ELEM>,1> cParent;
   typedef cSimpleDListNode<ELEM> cNode;
   typedef cSimpleDListIter<ELEM> cIter;

   void Append(const ELEM& e) { cParent::Append(new cNode(e));};
   void Prepend(const ELEM& e) { cParent::Prepend(new cNode(e));};
   void AppendNode(cNode* node) { cParent::Append(node); }; 
   void PrependNode(cNode* node) { cParent::Prepend(node); }; 

   void InsertBefore(cNode& before, const ELEM& e) { cParent::InsertBefore(&before,new cNode(e));};
   void InsertAfter(cNode& before, const ELEM& e) { cParent::InsertAfter(&before,new cNode(e));};
   void Delete(cNode& n) { if (&n != NULL) delete Remove(&n);};
   cIter Iter() const { return cIter(cDList::GetFirst()); };

   ~cSimpleDList() { cDList::DestroyAll();};
};

///////////////////////////////////////////////////////////////////////////////

#include <undbmem.h>

#endif /* !__DLIST_H */

