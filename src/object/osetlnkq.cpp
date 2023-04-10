/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <osetlnkq.h>
#include <relation.h>
#include <lnkquer_.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// 
// OBJECT SET LINK QUERY
//


//
// The real class
//
class cObjSetLinkQuery : public cBaseLinkQuery 
{
   IObjectQuery* ObjSet;
   ILinkQuery* Links; 
   cLinkQueryFactory* Factory;

   void NewLinks()
   {
      if (!ObjSet->Done())
         while (Links->Done())
         {
            ObjSet->Next();

            if (ObjSet->Done())
               break;

            // Move on to the next stimulus
            SafeRelease(Links);
            Links = Factory->Query(ObjSet->Object());
         }
   }

public:
   cObjSetLinkQuery(IObjectQuery* objset, cLinkQueryFactory* fact)
      : ObjSet(objset),
        Links(NULL),
        Factory(fact)
   {
      if (ObjSet->Done())
         Links = CreateEmptyLinkQuery();
      else
      {
         Links = Factory->Query(ObjSet->Object());
         if (Links->Done())
            NewLinks();
      }
      ObjSet->AddRef();

      if (!Done() && !VerifyLink(ID()))
         Next();
   }

   ~cObjSetLinkQuery()
   {
      delete Factory;
      SafeRelease(ObjSet);
      SafeRelease(Links);
   }

   STDMETHOD_(BOOL,Done)() const { return ObjSet->Done();};

   STDMETHOD(Next)() 
   {
      if (!Done())
      {
         do
         {
            Links->Next();         
            NewLinks();
         }
         while (!Done() && !VerifyLink(ID()));
      }
      return S_OK;
   }

   STDMETHOD_(LinkID,ID)() const
   {
      return Links->ID();
   }

   STDMETHOD_(void*,Data)() const { return Links->Data();};
   STDMETHOD(Link)(sLink* link) { return Links->Link(link);};
};


ILinkQuery* CreateObjSetLinkQuery(IObjectQuery* objset, cLinkQueryFactory* fact)
{
   return new cObjSetLinkQuery(objset,fact);
}

////////////////////////////////////////

class cDestSetQueryFactory : public cLinkQueryFactory
{
   IRelation* Rel;
   ObjID Source;

public:
   cDestSetQueryFactory(ObjID src, IRelation* rel)
      : Rel(rel),
        Source(src)
   {
      Rel->AddRef();
   }

   ~cDestSetQueryFactory() { SafeRelease(Rel);};

   ILinkQuery* Query(ObjID obj) { return Rel->Query(Source,obj); }; 
   
};

cLinkQueryFactory* CreateDestSetQueryFactory(IRelation* R, ObjID o)
{
   return new cDestSetQueryFactory(o,R);
}

/////////////////////////////////////////////////

class cSourceSetQueryFactory : public cLinkQueryFactory
{
   IRelation* Rel;
   ObjID Dest;

public:
   cSourceSetQueryFactory(ObjID dst, IRelation* rel)
      : Rel(rel),
        Dest(dst)
   {
      Rel->AddRef();
   }

   ~cSourceSetQueryFactory() { SafeRelease(Rel);};

   ILinkQuery* Query(ObjID obj) { return Rel->Query(obj,Dest); }; 
   
};

cLinkQueryFactory* CreateSourceSetQueryFactory(IRelation* R, ObjID o)
{
   return new cSourceSetQueryFactory(o,R);
}
