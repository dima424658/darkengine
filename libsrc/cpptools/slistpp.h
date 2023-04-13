/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/slist.h $
// $Author: TOML $
// $Date: 1998/08/21 11:45:11 $
// $Revision: 1.1 $
//
// (c) Copyright 1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//

#ifndef __SLIST_H
#define __SLIST_H

#include <lgassert.h>
#include <templexp.h>

///////////////////////////////////////////////////////////////////////////////
//
// ===================
// Singly-linked lists
// ===================
//
// This file defines classes to support singly-linked lists. There are
// two kinds of lists: ones where the list item is considered intrisically
// "listed," where derivation can be used, or the item is an type
// which is not necessarily "listed", but is occationally.
//
// Ownership
// ---------
//
// A cSList does not automatically clean up  after itself when it is destroyed:
// that is, the cSList destructor does nothing.  The reason for this is that
// nodes may be threaded onto multiple lists.
//
// A cSList that owns its data nodes should provide a destructor which calls
// cSList::DestroyAll to delete all the data nodes.  Also, a cSList can call
// cSList::Clear to zero the link pointers in the forward and backward links.
// For example:
//
// class MyList : public cSList<MyThing,1>
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
// be on a list.  Also, unless cSList::Clear is called, a node's links may be
// not equal to kSListInvalidPtr even if it is not on a list.
//
// Usage 1: Intrinsically Listed Types
// -----------------------------------
//
// The cSListNode parameterized class template is used as a base type from
// which to derive a data class.
//
// The cSList parameterized class template is used to define a list of a
// particular element type; it requires two formals, the node class it will list
// and the a numeral to disambiguate similar instantiations.
//
// Normal style is to declare two typedefs.  For example:
//
// class cMyThing;
// typedef cSList<cMyThing, 1> cMyList;
// typedef cSListNode<cMyThing, 1> cMyThingNode;
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
// the node pointers directy, use the "container" form of the SList:
//
// class cMyThing
// {
// public:
//     char Name[32];
// };
//
// typedef cContainerSList<cMyThing *, 0> cMyList;
// typedef cContSListNode<cMyThing *, 0> cMyNode;
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

#define kSListInvalidPtr ((cSListNodeBase *)0xffffffff)
#ifndef SHIP
extern const char * g_pszSlistInsertError;
#define SLValidateNodeInsert(p) AssertMsg((p)->m_pNext == kSListInvalidPtr, g_pszSlistInsertError);
#define SLInvalidateNode(p) ((p)->m_pNext = kSListInvalidPtr)
#else
#define SLValidateNodeInsert(p)
#define SLInvalidateNode(p)
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Generic expansion mechanism.
// @Q (toml 06-24-97): is this used, does it work?
//

#define SLIST_EXPAND(type, id) \
    TEMPLATE_EXPAND2(cSList, type, id); \
    TEMPLATE_EXPAND2(cSListNode, type, id)


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cSListNodeBase
//
// List node base class
//

class __CPPTOOLSAPI cSListNodeBase
{
friend class cSListBase;

public:
    cSListNodeBase * GetNext() const;

protected:
    cSListNodeBase();

private:
    cSListNodeBase * m_pNext;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cSListBase
//
// List template base class
//

class __CPPTOOLSAPI cSListBase
{
public:
    void Clear();
    void SetEmpty();

#ifndef __WATCOMC__
    // @Note (toml 05-18-96): The Watcom 10.5 parser has a bug handling
    // declarations of instances of cSListNodeBase derivations,
    // but only when optimizing is enabled (!). It claims a client of
    // a class derived from cSListBase cannot create an instance of
    // that derivation because this constructor is protected.  This
    // is incorrect behavior and should be tested against future versions
    // of Watcom.  Microsoft and Symantec handle it ok.
protected:
#endif
    cSListBase();

protected:
    cSListNodeBase * GetFirst() const;
    cSListNodeBase * GetLast() const;

    void             Append(cSListNodeBase *);
    void             Prepend(cSListNodeBase *);
    cSListNodeBase * Remove(cSListNodeBase *, cSListNodeBase * pPrev);
    void             InsertBefore(cSListNodeBase *, cSListNodeBase *, cSListNodeBase *pAfterPrev);
    void             InsertAfter(cSListNodeBase *, cSListNodeBase *);

private:
    cSListNodeBase * m_pFirst;
    cSListNodeBase * m_pLast;
};

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cSListNode
//
// Template for the base class of a list node of type NODE
//

template <class NODE, int ID>
class cSListNode : public cSListNodeBase
{
public:
    typedef NODE * cNodePtr;
    typedef NODE & cNodeRef;

    NODE * GetNext() const;
};

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cSList
//
// Template for a list of nodes of type NODE
//

template <class NODE, int ID>
class cSList : public cSListBase
{
public:
    typedef NODE * cNodePtr;

    void   DestroyAll();
    void   Append(NODE *);
    void   Prepend(NODE *);
    NODE * Remove(NODE *, NODE * pPrev);
    NODE * GetFirst() const;
    NODE * GetLast() const;
    void   InsertBefore(NODE *, NODE *, NODE *pAfterPrev);
    void   InsertAfter(NODE *, NODE *);
};

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cContSListNode
//

template <class CONTAINED_TYPE, int ID>
class cContSListNode : public cSListNode< cContSListNode<CONTAINED_TYPE, ID>, ID>
{
public:
   CONTAINED_TYPE item;
};

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cContainerSList
//

template <class CONTAINED_TYPE, int ID>
class cContainerSList : public cSList< cContSListNode<CONTAINED_TYPE, ID>, ID>
{
};

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cSerialSList
//
// Serializable list
//

class cIStore;
class cOStore;

template <class NODE, int ID>
class cSerialSList : public cSList<NODE, ID>
{
public:
    BOOL FromStream(cIStore &);
    BOOL ToStream(cOStore &) const;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cSListNodeBase, inline functions
//

inline cSListNodeBase::cSListNodeBase()
  : m_pNext(kSListInvalidPtr)
{
    // The next is set to bogus pointers
}

///////////////////////////////////////

inline cSListNodeBase *cSListNodeBase::GetNext() const
{
    return m_pNext;
}

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cSListNode, inline functions
//

template <class NODE, int ID>
inline NODE *cSListNode<NODE, ID>::GetNext() const
{
    return (NODE *)(cSListNode<NODE, ID> *)cSListNodeBase::GetNext();
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cSListBase, inline functions
//

inline void cSListBase::SetEmpty()
{
    m_pFirst = m_pLast = 0;
}

///////////////////////////////////////

inline cSListBase::cSListBase()
{
    SetEmpty();
}

///////////////////////////////////////

inline cSListNodeBase *cSListBase::GetFirst() const
{
    return m_pFirst;
}

///////////////////////////////////////

inline cSListNodeBase *cSListBase::GetLast() const
{
    return m_pLast;
}

///////////////////////////////////////
//
// Prepend a node to the beginning of this list
//
inline void cSListBase::Prepend(cSListNodeBase *pNode)
{
    SLValidateNodeInsert(pNode);
    
    if (!m_pFirst)
        m_pLast = pNode;

    pNode->m_pNext = m_pFirst;
    m_pFirst = pNode;
}

///////////////////////////////////////
//
// Append a node to the end of this list
//

inline void cSListBase::Append(cSListNodeBase * pNode)
{
    SLValidateNodeInsert(pNode);

    // If the list contains something...
    if (m_pLast)
        // ...then link the last node in this list before the new node
        m_pLast->m_pNext = pNode;
    else
        // ...else set the head of the list to the new node
        m_pFirst = pNode;

    pNode->m_pNext = 0;
    m_pLast = pNode;
}

///////////////////////////////////////
//
// Remove a node from this list
//

inline cSListNodeBase *cSListBase::Remove(cSListNodeBase *pNode, cSListNodeBase *pPrev)
{
    Assert_(!pPrev || pPrev->m_pNext == pNode);
    if (!pNode->m_pNext)
        m_pLast = pPrev;

    if (pPrev)
        pPrev->m_pNext = pNode->m_pNext;
    else
        m_pFirst = pNode->m_pNext;

    SLInvalidateNode(pNode);

    return pNode;
}

///////////////////////////////////////
//
// Insert a node before the specified node
//

inline void cSListBase::InsertBefore(cSListNodeBase *pAfter, cSListNodeBase *pNode, cSListNodeBase *pAfterPrev)
{
    Assert_(!pAfterPrev || pAfterPrev->m_pNext == pNode);
    SLValidateNodeInsert(pNode);
    
    pNode->m_pNext = pAfter;

    if (pAfterPrev)
        pAfterPrev->m_pNext = pNode;
    else
        m_pFirst = pNode;
}

///////////////////////////////////////
//
// Insert a node after another node
//

inline void cSListBase::InsertAfter(cSListNodeBase *pBefore, cSListNodeBase *pNode)
{
    SLValidateNodeInsert(pNode);
    
    pNode->m_pNext = pBefore->m_pNext;

    if (!pBefore->m_pNext)
        m_pLast = pNode;

    pBefore->m_pNext = pNode;
}

///////////////////////////////////////

inline void cSListBase::Clear()
{
    cSListNodeBase * p;
    while ((p = GetFirst()) != NULL)
    {
        Remove(p, NULL);
        p->m_pNext = kSListInvalidPtr;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cSList, inline functions
//

template <class NODE, int ID>
inline void cSList<NODE, ID>::Append(NODE *Node)
{
    cSListBase::Append((cSListNode<NODE, ID> *)Node);
}

///////////////////////////////////////

template <class NODE, int ID>
inline void cSList<NODE, ID>::Prepend(NODE *Node)
{
    cSListBase::Prepend((cSListNode<NODE, ID> *)Node);
}

///////////////////////////////////////

template <class NODE, int ID>
inline NODE *cSList<NODE, ID>::Remove(NODE *Node, NODE * pPrev)
{
    return (NODE *)(cSListNode<NODE, ID> *)cSListBase::Remove((cSListNode<NODE, ID> *)Node, (cSListNode<NODE, ID> *)pPrev);
}

///////////////////////////////////////

template <class NODE, int ID>
inline NODE *cSList<NODE, ID>::GetFirst() const
{
    return (NODE *)(cSListNode<NODE, ID> *)cSListBase::GetFirst();
}

///////////////////////////////////////

template <class NODE, int ID>
inline NODE *cSList<NODE, ID>::GetLast() const
{
    return (NODE *)(cSListNode<NODE, ID> *)cSListBase::GetLast();
}

///////////////////////////////////////

template <class NODE, int ID>
inline void cSList<NODE, ID>::InsertAfter(NODE *Before, NODE *Node)
{
    cSListBase::InsertAfter((cSListNode<NODE, ID> *)Before, (cSListNode<NODE, ID> *)Node);
}

///////////////////////////////////////

template <class NODE, int ID>
inline void cSList<NODE, ID>::InsertBefore(NODE *After, NODE *Node, NODE *pAfterPrev)
{
    cSListBase::InsertBefore((cSListNode<NODE, ID> *)After, (cSListNode<NODE, ID> *)Node, (cSListNode<NODE, ID> *)pAfterPrev);
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__SLIST_H */

