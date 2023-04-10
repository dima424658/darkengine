/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/link.cpp,v 1.5 1998/10/05 17:26:25 mahk Exp $
#include <link.h>
#include <linkknow.h>
#include <linkint.h>
#include <lnkquery.h>
// must be last header
#include <dbmem.h>

//
// Yet another gruesome abuse of "protected"
//

class ManglerKnower : public cLinkManagerKnower
{
public:
   static ILinkManagerInternal* Mangler(void) { return LinkMan();};
};

#define MANGLER (ManglerKnower::Mangler())

LinkID LinkCreate(ObjID source, ObjID dest, RelationID id)
{
   return MANGLER->Add(source,dest,id);
}

LinkID  LinkCreateFull(ObjID source, ObjID dest, RelationID id, void* data)
{
   return MANGLER->AddFull(source,dest,id,data);
}

HRESULT LinkDestroy(LinkID id)
{
   return MANGLER->Remove(id);
} 

BOOL    LinkGet(LinkID id, sLink* out)
{
   return MANGLER->Get(id,out);
}

HRESULT LinkSetData(LinkID id, void* data)
{
   return MANGLER->SetData(id,data);
}

void* LinkGetData(LinkID id)
{
   return MANGLER->GetData(id);
}

ILinkQuery* LinkQuery(ObjID source, ObjID dest, RelationID relation)
{
   return MANGLER->Query(source,dest,relation);
}

typedef cAutoIPtr<ILinkQuery> QueryPtrBase;


class QueryPtr: public QueryPtrBase
{
public:
   QueryPtr(ILinkQuery* q) : QueryPtrBase(q) {};
   
};


HRESULT LinkDestroyMany(ObjID source, ObjID dest, RelationID relation)
{
   QueryPtr q(LinkQuery(source,dest,relation));
   for (; !q->Done(); q->Next())
   {
      LinkDestroy(q->ID());
   }
   return S_OK;
}

BOOL LinkAnyLinks(ObjID source, ObjID dest, RelationID relation)
{
   QueryPtr q(LinkQuery(source,dest,relation));
   return !q->Done();
}

int LinkCount(ObjID source, ObjID dest, RelationID relation)
{
   QueryPtr q(LinkQuery(source,dest,relation));
   int retval = 0;
   for (; !q->Done(); q->Next(), retval++);
   return retval;
}

BOOL    LinkGetSingleton(ObjID source, ObjID dest, RelationID rel, LinkID* id, sLink* link)
{
   QueryPtr q(LinkQuery(source,dest,rel));
   if (q->Done()) return FALSE;
   *id = q->ID();
   q->Link(link);
#ifdef DBG_ON
   q->Next();
   Assert(q->Done(),("More than one link from %d to %d, Relation %d\n",source,dest,rel));
#endif 

   return TRUE;
}

