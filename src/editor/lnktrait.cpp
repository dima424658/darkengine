/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <lnktrait.h>
#include <edtrait.h>
#include <edtraist.h>
#include <linkbase.h>
#include <linkman.h>
#include <relation.h>
#include <lnkquery.h>
#include <lazyagg.h>
#include <string.h>

#include <edittool.h>
#include <linkedit.h>
#include <linkedst.h>
#include <linkid.h>
#include <lnktrai_.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


////////////////////////////////////////////////////////////
//
// EDIT TRAIT FOR LINKS
//


//
// Link manager ptr
//

static LazyAggMember(ILinkManager) pLinkMan; 

//
// default descriptors 
//

static sEditTraitDesc def_descs[2] =
{
   // Normal
   {
      kRelationTrait,
      {
         "anylinks",
         "All Links",
         "Links", 
      },
   },
   // Hidden
   {
      kRelationTrait,
      {
         "hiddenlinks",
         "Hidden",
         "Links", 
      },
   }
};

//
// Implementation class
//



int cLinkEditTrait::LinkCount(ObjID obj)
{
   cAutoIPtr<ILinkQuery> pQuery (pLinkMan->Query(obj,LINKOBJ_WILDCARD,mID)); 
   int cnt = 0; 
   for (; !pQuery->Done(); pQuery->Next())
   {
      // special wildcard case 
      if (mID == RELID_WILDCARD)
      {
         // if invisible, only show invisible links.  Does this make sense? 
         BOOL invisible = (mDesc.flags & kTraitInvisible) != 0; 
         LinkID linkid = pQuery->ID(); 
         cAutoIPtr<IRelation> rel ( pLinkMan->GetRelation(LINKID_RELATION(linkid)) ); 
         BOOL no_edit = (rel->Describe()->flags & kRelationNoEdit) != 0; 
         if (invisible != no_edit)
            continue; 
      }

      cnt++; 
            
   }

   return cnt; 

}

void cLinkEditTrait::LinkDialog(ObjID obj)
{
   sLinkEditorDesc desc = {"", 0}; 
   if (mDesc.flags & kTraitInvisible)
      desc.flags |= kLinkEditShowAll; 
   EditLinks(&desc,obj,LINKOBJ_WILDCARD,mID); 
}


cLinkEditTrait::cLinkEditTrait(RelationID id, sEditTraitDesc* desc )
   : mID(id),
     mDesc(*desc),
     mCaps(gCaps)
{

   if (mID != LINKID_NULL)
      mCaps.flags |= kTraitCanAdd; 


}




STDMETHODIMP cLinkEditTrait::Unparse(ObjID obj, char* buf, int buflen)
{
   AutoAppIPtr_(EditTools,pTools); 

   // special "all" case

   if (mID == RELID_NULL)
   {
      sprintf(buf,"%d",LinkCount(obj));
      return S_OK; 
   }


   cAutoIPtr<ILinkQuery> pQuery (pLinkMan->Query(obj,LINKOBJ_WILDCARD,mID)); 
   const char* prefix = "";
   buf[0] = '\0'; 

   for (; !pQuery->Done(); pQuery->Next())
   {
      int len;
      // append prefix
      len = strlen(prefix);
      if (len + 1 > buflen)
         goto too_long;
         
      strcpy(buf,prefix);
      buf += len; 
      buflen -= len; 

      // after the first time through, comma-delimit
      prefix = ", "; 
         
      // append object name
      sLink link;
      pQuery->Link(&link);
      const char* name = pTools->ObjName(link.dest); 
      len = strlen(name);
      if (len + 1 > buflen)
         goto too_long;
      strcpy(buf,name);
      buf += len;
      buflen -= len; 
   }
   return S_OK; 
         
too_long:
   if (buflen > 0)
   {
      strncpy(buf,"...",buflen); 
      buf[buflen-1] = '\0'; 
   }
   return S_OK; 

}

sEditTraitCaps cLinkEditTrait::gCaps = { kTraitCanEdit|kTraitCanUnparse};

//
// Factory
//

IEditTrait* CreateLinkEditTrait(RelationID id, BOOL hidden)
{
   sEditTraitDesc desc = def_descs[hidden != 0]; 

   cAutoIPtr<IRelation> rel = pLinkMan->GetRelation(id); 
   if (rel->GetID() != LINKID_NULL)
   {
      const sRelationDesc* reldesc = rel->Describe();

      // uniquify the name
      char uniquename[64];
      sprintf(uniquename,"L$%s",reldesc->name); 

      strncpy(desc.strings.name,uniquename,sizeof(desc.strings.name));
      desc.strings.name[sizeof(desc.strings.name)-1] = '\0'; 

      strncpy(desc.strings.friendly_name,reldesc->name,sizeof(desc.strings.friendly_name));
      desc.strings.friendly_name[sizeof(desc.strings.friendly_name)-1] = '\0'; 

      if (reldesc->flags & kRelationNoEdit)
         desc.flags |= kTraitInvisible;
   }

   if (hidden)
      desc.flags |= kTraitInvisible;


   return new cLinkEditTrait(id,&desc); 
}

