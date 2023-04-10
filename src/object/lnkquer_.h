/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/lnkquer_.h,v 1.7 2000/01/29 13:23:44 adurant Exp $
#pragma once

#ifndef __LNKQUER__H
#define __LNKQUER__H
#include <lnkquery.h>
#include <linktype.h>
#include <linkbase.h>
#include <linkqdb_.h>
#include <linkknow.h>
#include <linkint.h>

////////////////////////////////////////////////////////////
// LINK QUERY IMPLEMENTATION TOOLS
//

//------------------------------------------------------------
// Useful struct for matching links with wildcards
//

struct sLinkTemplate : public sLink
{
   sLinkTemplate(ObjID s = LINKOBJ_WILDCARD, ObjID d = LINKOBJ_WILDCARD, RelationID f = RELID_WILDCARD)
   { source = s; dest = d; flavor = f; } ;

   sLinkTemplate(const sLink& link) { *(sLink*)this = link;}; 
   
   BOOL operator==(const sLinkTemplate& rhs) 
   { return RELID_MATCH(flavor,rhs.flavor) &&
        LINKOBJ_MATCH(source,rhs.source) &&
        LINKOBJ_MATCH(dest,rhs.dest);};

   BOOL operator !=(const sLinkTemplate& rhs) { return !(*this == rhs);};
};
            
//------------------------------------------------------------
// Base Query Implementation 
//

class cUnknownLinkQuery : public ILinkQuery
{
public:
   DECLARE_UNAGGREGATABLE(); 

   virtual ~cUnknownLinkQuery() {};
};

class cBaseLinkQuery : public cUnknownLinkQuery, protected cLinkManagerKnower
{
protected:
   BOOL locked;
   tQueryDate Birthday; 

   BOOL VerifyLink(LinkID link) { return LinkMan()->LinkValid(link,Birthday); }; 
   
public:

   cBaseLinkQuery(tQueryDate birthday = QUERY_DATE_NONE);
   virtual ~cBaseLinkQuery();

   STDMETHOD_(void*,Data)() const { return LinkMan()->GetData(ID());};
   STDMETHOD(Link)(sLink* link) const 
   { return LinkMan()->Get(ID(),link) ? S_OK : E_FAIL;};
   STDMETHOD_(ILinkQuery*,Inverse)() { return CreateInverseLinkQuery(this);};
   

};

//------------------------------------------------------------
// The single-link query
//

class cSingleLinkQuery : public cBaseLinkQuery
{
protected:
   LinkID DaLink;

public:
   cSingleLinkQuery(LinkID link, tQueryDate bday = QUERY_DATE_NONE)
      : cBaseLinkQuery(bday),DaLink(link)
   {
      sLink dummy; // can we just use null?
      if (!VerifyLink(DaLink)) 
         DaLink = LINKID_NULL;
      else if (!LinkMan()->Get(DaLink,&dummy)) 
         DaLink = LINKID_NULL;
   }

   STDMETHOD_(BOOL,Done)() const { return DaLink == LINKID_NULL;}; 
   STDMETHOD(Next)() { DaLink = LINKID_NULL; return S_OK; } ; 


   STDMETHOD_(LinkID,ID)() const { return DaLink;};
};

////////////////////////////////////////

class cDelegatedLinkQuery : public ILinkQuery 
{
   ILinkQuery* Inner;


public: 
   DECLARE_DELEGATION();

   cDelegatedLinkQuery(IUnknown* out, ILinkQuery* in) : Inner(in) 
   {
      INIT_DELEGATION(out);
      in->AddRef();
   }

   virtual ~cDelegatedLinkQuery()
   {
      SafeRelease(Inner);
   }

   STDMETHOD_(BOOL,Done)() const { return Inner->Done();};
   STDMETHOD(Next)() { return Inner->Next();};
   STDMETHOD(Link)(sLink* link) const { return Inner->Link(link);};

   STDMETHOD_(LinkID,ID)() const { return Inner->ID();};
   STDMETHOD_(void*,Data)() const { return Inner->Data();};
   STDMETHOD_(ILinkQuery*,Inverse)() { return Inner->Inverse();};

};

#endif // __LNKQUER__H




