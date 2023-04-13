/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/linkscpt.cpp,v 1.17 1999/02/16 16:05:53 Justin Exp $


#include <comtools.h>
#include <appagg.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <linkscpt.h>

#include <bintrait.h>

#include <linkbase.h>
#include <linkman.h>
#include <relation.h>

#include <sdesbase.h>
#include <sdestool.h>
#include <sdesparm.h>

#include <objlpars.h>

#include <dbmem.h>

////////////////////////////////////////////////////////////
// LINK SCRIPT SERVICE
//

static ILinkManager* LinkMan()
{
   static ILinkManager* da_man = NULL;
   if (!da_man)
   {
      da_man = AppGetObj(ILinkManager);
   }
   return da_man; 
}

static IStructDescTools* SdescTools()
{
   static IStructDescTools* tools = NULL;
   if (tools == NULL)
   {
      tools = AppGetObj(IStructDescTools);
   }
   return tools;
} 

//
// Link Kind class
//

linkkind::linkkind(const char* name)
{
   IRelation* rel = LinkMan()->GetRelationNamed(name);
   id = rel->GetID(); 

   AssertMsg1(id != RELID_NULL, "%s is not a link flavor", name); 
   SafeRelease(rel); 
}

linkkind::operator string () const 
{
   cAutoIPtr<IRelation> rel ( LinkMan()->GetRelation(id) );
   return rel->Describe()->name; 
}

//
// Link class
//

object link::From() const
{
   sLink link;
   LinkMan()->Get(id,&link); 
   return link.source;
}

object link::To() const
{
   sLink link;
   LinkMan()->Get(id,&link); 
   return link.dest;
}

cMultiParm link::GetData(const char* field ) const 
{
   cAutoIPtr<IRelation> rel ( LinkMan()->GetRelation(LINKID_RELATION(id)) ); 
   void* data = rel->GetData(id); 
   if (!data) return 0; 

   const sRelationDataDesc* ddesc = rel->DescribeData(); 
   const sStructDesc* sdesc = SdescTools()->Lookup(ddesc->type); 
   if (!sdesc) return 0; 

   AssertMsg1(field != NULL || sdesc->nfields == 1,"%s links have more than one data field.", rel->Describe()->name);

   const sFieldDesc* fdesc = NULL; 
   if (field)
   {
      fdesc = NULL;
      for (int i = 0; i < sdesc->nfields; i++)
         if (stricmp(field,sdesc->fields[i].name) == 0)
         {
            fdesc = &sdesc->fields[i]; 
            break; 
         }
      AssertMsg2(fdesc,"%s links have no data field named %s.",rel->Describe()->name,field); 
   }
   else
      fdesc = &sdesc->fields[0];
   
   return GetParmFromField(fdesc,data); 
}


void link::SetData(const char* field, const cMultiParm& value)
{
   cAutoIPtr<IRelation> rel ( LinkMan()->GetRelation(LINKID_RELATION(id)) ); 
   const sRelationDataDesc* ddesc = rel->DescribeData(); 

   void* data = rel->GetData(id); 

   BOOL deldata = FALSE; 
   if (!data) 
   {
      data = new char[ddesc->size]; 
      deldata = TRUE;
   }

   const sStructDesc* sdesc = SdescTools()->Lookup(ddesc->type); 
   AssertMsg1(sdesc, "%s links have unknown data type.", rel->Describe()->name); 
   if (!sdesc) return ;

   AssertMsg1(field != NULL || sdesc->nfields == 1,"Relation %s has more than one data field.", rel->Describe()->name);

   const sFieldDesc* fdesc = NULL; 
   if (field)
   {
      fdesc = NULL;
      for (int i = 0; i < sdesc->nfields; i++)
         if (stricmp(field,sdesc->fields[i].name) == 0)
         {
            fdesc = &sdesc->fields[i]; 
            break; 
         }
      AssertMsg2(fdesc,"%s links have no field named %s.",rel->Describe()->name,field); 
   }
   else
      fdesc = &sdesc->fields[0];
   
   SetFieldFromParm(fdesc,data,value); 
   rel->SetData(id,data); 

   if (deldata)
      delete [] (char*)data; 
}

//
// THE LINK SERVICE PROPER
//

DECLARE_SCRIPT_SERVICE_IMPL(cLinkSrv, Link)
{
   STDMETHOD_(void, Init)()
   {
   }

   STDMETHOD_(link,Create)(linkkind kind, object from, object to) 
   {
      return LinkMan()->Add(ScriptObjID(from),ScriptObjID(to),kind);
   }

   STDMETHOD(Destroy)(link destroy_me)
   {
      return LinkMan()->Remove(destroy_me); 
   }

   STDMETHOD_(boolean,AnyExist)(linkkind kind, object from, object to) 
   {
      cAutoIPtr<ILinkQuery> q ( LinkMan()->Query(ScriptObjID(from),ScriptObjID(to),kind) ); 
      return !q->Done(); 
   }

   //
   // Get all the links that match a particular pattern
   //
   STDMETHOD_(linkset,GetAll)(linkkind kind, object from, object to) 
   {
      cAutoIPtr<ILinkQuery> q ( LinkMan()->Query(ScriptObjID(from),ScriptObjID(to),kind) );
      return linkset(q); 
   }

   //
   // Get just one link.  
   //
   STDMETHOD_(link,GetOne)(linkkind kind, object from, object to)  
   {
      cAutoIPtr<ILinkQuery> q ( LinkMan()->Query(ScriptObjID(from),ScriptObjID(to),kind) ); 
      
      return (q->Done()) ? 0 : q->ID(); 
   }

   // @NOTE (justin 2/16/99): As of this writing, networking only supports
   // ObjIDs as the type for linkdata. This will be fixed some time when
   // I have time to properly network cMultiParms...
   STDMETHOD(BroadcastOnAllLinks)(const object& SelfObj,
                                  const char *Message, 
 				  linkkind recipients,
 				  const cMultiParm& linkdata)
   {
      AutoAppIPtr(LinkManager);
      AutoAppIPtr(ScriptMan);

      ObjID self = ScriptObjID(SelfObj);

      if (pLinkManager->AnyLinks(recipients,self,LINKOBJ_WILDCARD))
      {
         ILinkQuery *Controls = pLinkManager->Query(self,LINKOBJ_WILDCARD,recipients);
         
         while (!Controls->Done())
         {
            // so maybe the linkdata should be interepreted in a questbit-specific way someday?
            if (Controls->Data() == NULL || link(Controls->ID()).GetData() == linkdata)
            {
               sLink slink;

               Controls->Link(&slink);

               // Debug.MPrint(SelfObj," sending message ",Message," to ",thislink.To());
               //pScriptMan->PostMessage2(SelfObj,slink.dest,Message,linkdata,NULL,NULL);
               // We currently need to set up the message, to get the flag in:
               sScrMsg * pMsg = new sScrMsg(self, 
                                            slink.dest, 
                                            Message,
                                            linkdata);
               pMsg->flags |= kSMF_MsgPostToOwner;
               pScriptMan->PostMessage(pMsg);
               pMsg->Release();
            }
            Controls->Next();
         }
         SafeRelease(Controls);
      }   

      return(S_OK);
   }

   STDMETHOD(BroadcastOnAllLinks)(const object& SelfObj,
                                  const char *Message, 
 				  linkkind recipients)
   {
      AutoAppIPtr(LinkManager);
      AutoAppIPtr(ScriptMan);

      ObjID self = ScriptObjID(SelfObj);

      if (pLinkManager->AnyLinks(recipients,self,LINKOBJ_WILDCARD))
      {
         ILinkQuery *Controls = pLinkManager->Query(self,LINKOBJ_WILDCARD,recipients);
         
         while (!Controls->Done())
         {
            sLink slink;
            Controls->Link(&slink);

            //pScriptMan->PostMessage2(SelfObj,slink.dest,Message,NULL,NULL,NULL);
            // We currently need to set up the message, to get the flag in:
            sScrMsg * pMsg = new sScrMsg(self, 
                                         slink.dest, 
                                         Message,
                                         NULL);
            pMsg->flags |= kSMF_MsgPostToOwner;
            pScriptMan->PostMessage(pMsg);
            pMsg->Release();
            Controls->Next();
         }
         SafeRelease(Controls);
      }   

      return(S_OK);
   }

   //
   // Create multiple links.
   //
   STDMETHOD(CreateMany)(linkkind kind, const string& FromSet, const string& ToSet)
   {
      cObjListParser   parser;
      cDynArray<ObjID> sources;
      cDynArray<ObjID> destinations;
      IRelation *      pRelation = LinkMan()->GetRelation(kind);

      if (!pRelation)
         return E_FAIL;
      
      parser.Parse(FromSet, &sources);
      parser.Parse(ToSet, &destinations);
      
      for (int i = 0; i < sources.Size(); i++)
         for (int j = 0; j < destinations.Size(); j++)
            pRelation->Add(sources[i], destinations[j]);

      pRelation->Release();
      return S_OK;
   }

   //
   // Destroy multiple link
   //
   STDMETHOD(DestroyMany)(linkkind kind, const string& FromSet, const string& ToSet)
   {
      cObjListParser   parser;
      cDynArray<ObjID> sources;
      cDynArray<ObjID> destinations;
      IRelation *      pRelation = LinkMan()->GetRelation(kind);
      ILinkQuery *     pQuery;

      if (!pRelation)
         return E_FAIL;
      
      parser.Parse(FromSet, &sources);
      parser.Parse(ToSet, &destinations);
      
      for (int i = 0; i < sources.Size(); i++)
         for (int j = 0; j < destinations.Size(); j++)
         {
            pQuery = pRelation->Query(sources[i], destinations[j]);

            for (; !pQuery->Done(); pQuery->Next())
               pRelation->Remove(pQuery->ID());
               
            pQuery->Release();
         }

      pRelation->Release();
      return S_OK;
   }

   STDMETHOD_(linkset,GetAllInherited)(linkkind kind, object from, object to)
   {
      cAutoIPtr<IRelation> rel = LinkMan()->GetRelation(kind);
      cAutoIPtr<ILinkQuery> q  = QueryInheritedLinksUncached(rel, from, to);
      return linkset(q); 
   }
   
   STDMETHOD_(linkset,GetAllInheritedSingle)(linkkind kind, object from, object to)
   {
      cAutoIPtr<IRelation> rel = LinkMan()->GetRelation(kind);
      cAutoIPtr<ILinkQuery> q  = QueryInheritedLinksSingleUncached(rel, from, to);
      return linkset(q); 
   }

}; 

IMPLEMENT_SCRIPT_SERVICE_IMPL(cLinkSrv, Link);


DECLARE_SCRIPT_SERVICE_IMPL(cLinkToolsSrv, LinkTools)
{
   STDMETHOD_(long, LinkKindNamed)(const char* name)
   {
      return linkkind(name); 
   }

   STDMETHOD_(string, LinkKindName)(long id)
   {
      cAutoIPtr<IRelation> rel (LinkMan()->GetRelation(id) ); 
      AssertMsg1(rel != 0,"A script referenced non-existent link kind %d",id); 
      return rel->Describe()->name; 
   }

   STDMETHOD(LinkGet)(long id, sLink& link)
   {
      VerifyMsg1(LinkMan()->Get(id,&link),"A script tried to access non-existent link %d",id); 
      return S_OK;
   }

   STDMETHOD_(cMultiParm,LinkGetData)(long id, const char* field)
   {
      return link(id).GetData(field); 
   }

   STDMETHOD(LinkSetData)(long id, const char* field, const cMultiParm& val)
   {
      link(id).SetData(field,val); 
      return S_OK;
   }
}; 

IMPLEMENT_SCRIPT_SERVICE_IMPL(cLinkToolsSrv, LinkTools); 



