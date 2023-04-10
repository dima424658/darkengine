/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkset.cpp,v 1.4 1998/01/27 18:49:35 mahk Exp $
#include <lnkquer_.h>
#include <linkset.h>
#include <lststtem.h>
#include <linkid.h>

// Must be last header
#include <dbmem.h>

#ifdef __MSVC
template cDList<cSimpleDListNode<LinkID>,1>;
template cSimpleDList<LinkID>;
template cSimpleListSet<LinkID>;
#endif


////////////////////////////////////////////////////////////
//
// INTERNAL CLASS: cSimpleLinkSetQuery
//
////////////////////////////////////////////////////////////

class cSimpleLinkSetQuery : public cBaseLinkQuery
{
   cLinkSet::cIter Iter;

   BOOL Eligible(LinkID id);
public:

   cSimpleLinkSetQuery(const cLinkSet* set,
                 tQueryDate bday = QUERY_DATE_NONE)
      : Iter(set->Iter()),
        cBaseLinkQuery(bday)
   { while (!Iter.Done() && !Eligible(Iter.Value())) Iter.Next();} ;

   virtual ~cSimpleLinkSetQuery() {};

   STDMETHOD_(BOOL,Done)() const;
   STDMETHOD(Link)(sLink* link) const;
   STDMETHOD(Next)();
   STDMETHOD_(LinkID,ID)() const { return (LinkID)Iter.Value();};
};


STDMETHODIMP_(BOOL) cSimpleLinkSetQuery::Done() const
{
   return Iter.Done();
}

STDMETHODIMP cSimpleLinkSetQuery::Link(sLink* link) const
{
   if (Iter.Done()) return E_FAIL;
   LinkID val = Iter.Value();
   LinkMan()->Get(val,link);
   return S_OK;
}

BOOL cSimpleLinkSetQuery::Eligible(LinkID link)
{
   return VerifyLink(link);
}

STDMETHODIMP cSimpleLinkSetQuery::Next()
{
   for (Iter.Next(); !Iter.Done(); Iter.Next())
   {
      if (Eligible(Iter.Value()))
         return S_OK;
   }
   return E_FAIL;
}

////////////////////////////////////////////////////////////
//
// INTERNAL CLASS: cLinkSetQuery
//
////////////////////////////////////////////////////////////

class cLinkSetQuery : public cBaseLinkQuery
{
   sLinkTemplate Pattern;
   cLinkSet::cIter Iter;

   BOOL Eligible(LinkID id);
public:

   cLinkSetQuery(const cLinkSet& set,
                 const sLinkTemplate& pattern,
                 tQueryDate bday = QUERY_DATE_NONE)
      : Iter(set.Iter()),
        Pattern(pattern),
        cBaseLinkQuery(bday)
   { while (!Iter.Done() && !Eligible(Iter.Value())) Iter.Next();} ;

   virtual ~cLinkSetQuery() {};

   STDMETHOD_(BOOL,Done)() const;
   STDMETHOD(Link)(sLink* link) const;
   STDMETHOD(Next)();
   STDMETHOD_(LinkID,ID)() const { return (LinkID)Iter.Value();};
};


STDMETHODIMP_(BOOL) cLinkSetQuery::Done() const
{
   return Iter.Done();
}

STDMETHODIMP cLinkSetQuery::Link(sLink* link) const
{
   if (Iter.Done()) return E_FAIL;
   LinkID val = Iter.Value();
   LinkMan()->Get(val,link);
   return S_OK;
}

BOOL cLinkSetQuery::Eligible(LinkID link)
{
   if (!VerifyLink(link)) return FALSE;

      // check relation
   RelationID relid = LINKID_RELATION(link);

   // don't yield inverse links on wildcard.
   // cuz that's the spec

   if (!RELID_MATCH(Pattern.flavor,relid))
      return FALSE;

      // we always assume the source is right.
   if (Pattern.dest != LINKOBJ_WILDCARD)  // must test dest
   {
      sLinkTemplate temp;
      LinkMan()->Get(link,(sLink*)&temp);
      if (temp != Pattern) return FALSE;
   }
   return TRUE;
}

STDMETHODIMP cLinkSetQuery::Next()
{
   for (Iter.Next(); !Iter.Done(); Iter.Next())
   {
      if (Eligible(Iter.Value()))
         return S_OK;
   }
   return E_FAIL;
}



////////////////////////////////////////////////////////////
//
// CLASS: cLinkSet
//
//

ILinkQuery* cLinkSet::Query(tQueryDate bday)
{
   return new cSimpleLinkSetQuery(this,bday);
}

ILinkQuery* cLinkSet::PatternQuery(const struct sLinkTemplate* pattern, tQueryDate bday)
{
   return new cLinkSetQuery(*this,*pattern,bday);
}





