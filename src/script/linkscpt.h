/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/linkscpt.h,v 1.10 1998/08/04 19:00:17 TOML Exp $
#pragma once  
#ifndef __LINKSCPT_H
#define __LINKSCPT_H

#include <lnkquery.h>
#include <linkbase.h>
#include <objscrpt.h>
#include <linkid.h>

////////////////////////////////////////
// 
// Link Services
//

//
// "Link Kind" type, can be constructed by name or id
//

class linkkind 
{
   long id;
public:
   linkkind(const char* name);
   linkkind(int i) :id(i) {}; 
   linkkind(const linkkind& f) :id(f.id) {}; 

   operator long() const { return id; }; 
   operator string() const; 

   operator ==(const linkkind& f) const { return id == f.id; }; 
   operator ==(const char* name) const { return id == linkkind(name).id; }; 

};

//
// Link type 
//


class link 
{
   long id; 
public:
   link(long i = 0 ) :id(i) {};
   link(const link& l) :id (l.id) {}; 
   ~link() {}; 
   operator long() const { return id; }; 
   
   // End points
   object From() const;   
   object To() const;     

   // what kind of link I am..
   linkkind Kind() const { return LINKID_RELATION(id); };  

   // Link data accessors
   cMultiParm GetData(const char* field = NULL) const; 
   void SetData(const char* field, const cMultiParm ref value);
   void SetData(const cMultiParm ref value) { SetData(NULL,value); }; 
};

//
// "Link Set" type for iterating across a query 
//


F_DECLARE_INTERFACE(ILinkQuery); 
class linkset 
{
   ILinkQuery* query; 

public:
   linkset(ILinkQuery* q) :query(q) { query->AddRef(); }; 
   linkset(const linkset& s) :query(s.query) { };
   ~linkset() { SafeRelease(query); }; 

   boolean AnyLinksLeft()  // Are there any links left? 
   { return query && !query->Done(); }; 

   link Link() // get the current link
   { return (query) ? query->ID() : 0; };   

   void NextLink() // go to the next link 
   { query->Next();};    

}; 

DECLARE_SCRIPT_SERVICE(Link, 0xee)
{
   //
   // Create a new link
   //
   STDMETHOD_(link,Create)(linkkind kind, object from, object to) PURE;

   //
   // Destroy a link
   //
   STDMETHOD(Destroy)(link destroy_me) PURE;


   // For various accessors, the following constants are useful

#define AnyObject (0)
#define AnyLink   (0)

   //
   // Check to see if any links exist
   //
   STDMETHOD_(boolean,AnyExist)(linkkind kind = AnyLink, object from = AnyObject, object to = AnyObject) PURE; 

   //
   // Get all the links that match a particular pattern
   //
   STDMETHOD_(linkset,GetAll)(linkkind kind = AnyLink, object from = AnyObject, object to = AnyObject) PURE; 

   //
   // Get just one link.  
   //
   STDMETHOD_(link,GetOne)(linkkind kind = AnyLink, object from = AnyObject, object to = AnyObject) PURE; 

   // Broadcast along all of your links
   STDMETHOD(BroadcastOnAllLinks)(const object ref SelfObj, const char *Message, linkkind recipients, const cMultiParm ref linkdata) PURE;
   STDMETHOD(BroadcastOnAllLinks)(const object ref SelfObj, const char *Message, linkkind recipients) PURE;


   //
   // Create multiple links.
   //
   STDMETHOD(CreateMany)(linkkind kind, const string ref FromSet, const string ref ToSet) PURE;

   //
   // Destroy a link
   //
   STDMETHOD(DestroyMany)(linkkind kind, const string ref FromSet, const string ref ToSet) PURE;
   
   
   //
   // Query all links inherited by one or two objs
   //
   STDMETHOD_(linkset,GetAllInherited)(linkkind kind = AnyLink, object from = AnyObject, object to = AnyObject) PURE; 
   STDMETHOD_(linkset,GetAllInheritedSingle)(linkkind kind = AnyLink, object from = AnyObject, object to = AnyObject) PURE; 
}; 

//////////////////////////////////////////////////////////////
// 
// "Link Tools" service for use by link class implementations.  
// Not intended for public use.
//


DECLARE_SCRIPT_SERVICE(LinkTools, 0xef)
{
   STDMETHOD_(long,LinkKindNamed)(const char* name) PURE; 
   STDMETHOD_(string,LinkKindName)(long id) PURE; 
   STDMETHOD(LinkGet)(long id, sLink& l) PURE; 
   STDMETHOD_(cMultiParm,LinkGetData)(long id, const char* field) PURE; 
   STDMETHOD(LinkSetData)(long id, const char* field, const cMultiParm ref val) PURE; 
}; 

#ifdef SCRIPT

inline linkkind::linkkind(const char* name) 
   : id(LinkTools.LinkKindNamed(name)) 
{
}

inline linkkind::operator string() const 
{
   return LinkTools.LinkKindName(id); 
}

inline object link::From() const 
{
   sLink l;
   LinkTools.LinkGet(id,l); 
   return l.source;
}

inline object link::To() const 
{
   sLink l;
   LinkTools.LinkGet(id,l); 
   return l.dest;
}

inline cMultiParm link::GetData(const char* field) const
{
   return LinkTools.LinkGetData(id,field); 
}

inline void link::SetData(const char* field, const cMultiParm ref value) 
{
   LinkTools.LinkSetData(id,field,value); 
}

#endif 

#endif // __LINKSCPT_H




