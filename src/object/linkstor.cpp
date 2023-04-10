/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkstor.cpp,v 1.17 1999/08/19 16:54:53 Justin Exp $
#include <appagg.h>
#include <string.h>

#include <linksto_.h>
#include <linkint.h>
#include <lnkquery.h>
#include <linkbase.h>
#include <relation.h>
#include <linkid.h>
#include <lnkremap.h>

#include <tagfile.h>
#include <vernum.h>
#include <dbasemsg.h>

#include <objremap.h>
#include <osysbase.h>

#include <dataops.h>
#include <dataops_.h>

#include <iobjsys.h>
#include <lazyagg.h>

#include <hashpp.h>
#include <hshpptem.h>

// Must be last header
#include <dbmem.h>

static LazyAggMember(IObjectSystem) gpObjSys; 

////////////////////////////////////////////////////////////
//
// cBaseLinkStore
//
//////////////////////////////////////////////////////////////

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cBaseLinkStore,ILinkStore);

// Historical order.  Sorry.  
static eObjPartition partition_masks[] = { kObjPartAbstract, kObjPartMission, kObjPartTerrain, kObjPartBriefcase, kObjPartConcrete }; 

#define NUM_PARTITION_MASKS (sizeof(partition_masks)/sizeof(partition_masks[0]))

static BOOL IsLinkSavePartition(LinkID id, ObjID src, ObjID dest, int partition)
{
   // currently, we ignore the partition field of the link id. 
   // that's because we're going to recompute it anyway. 
   //   if (partition_masks[LINKID_PARTITION(id)] & partition)
   //      return TRUE; 

   // otherwise, look at the endpoints 
   int num_objs_in = 0; 
   if (gpObjSys->IsObjSavePartition(src,partition))
      num_objs_in++; 
   if (gpObjSys->IsObjSavePartition(dest,partition))
   {
      // swap src and dest, so we're sure that src is in the partition
      ObjID temp = src; 
      src = dest;
      dest = src; 
      num_objs_in++; 
   }
   
   if (num_objs_in == 0) return FALSE;
   if (num_objs_in == 2) return TRUE; 

   // num_objs_in == 1


   // src is in partition, if dest is in a LOWER partition, we win.  
   eObjPartition destpart = gpObjSys->ObjDefaultPartition(dest);  

   // Special briefcase hack.  
   // Don't save links out from the briefcase to other concrete partitions
   if ((partition & kObjPartBriefcase) && OBJ_IS_CONCRETE(dest))
      return FALSE; 

   
   // mask out the major-concrete case 
   destpart &= ~kObjectConcrete; 
   if (destpart == 0) return FALSE; 

   return destpart < partition; 
}

//------------------------------------------------------------
// Default Save/Load
//


enum eMajorVersions
{
   kInitial,
   kRelationVersioning, 
   kLinkIDFormat, 
};

// Horrible 14-bit sign-extension math 
#define OLD_LINK_IDX(id) ((short)(((id) & 0x3FFF) << 2) /4) 

static BOOL setup_file(IRelation* rel, ITagFile* file, const char* prefix, TagVersion* ver)
{

   const sRelationDesc* reldesc = rel->Describe();
   
   // stuff relation version into minor
   ver->minor = reldesc->version; 
   TagVersion relver = *ver;
   
   // I am so lazy 
   TagVersion& v = *ver;


   // build the tag
   TagFileTag tag;
   strcpy(tag.label,prefix);
   strncat(tag.label,reldesc->name,sizeof(tag.label)-strlen(prefix)-1);
   tag.label[sizeof(tag.label)-1] = '\0';

   // Set up the tag block
   HRESULT err = file->OpenBlock(&tag,&v);
   if (err != S_OK)
      return FALSE;

   if (!VersionNumsEqual(&v,&relver))
   {
      // Major version zero had no relation versioning, account for that.
      tRelationVersion minor = (v.major < kRelationVersioning) ? 0 : v.minor;
      tRelationVersion oldest = reldesc->oldest;
      if (reldesc->oldest == 0)
         oldest = relver.minor;

      if (minor < oldest)
      {
         Warning(("Old link tag version, purging %s\n",tag.label));
         file->CloseBlock();
         return FALSE;
      }
   }
   return TRUE;
}

static void cleanup_file(ITagFile* file)
{
   file->CloseBlock();
}

#define RELATION_TAG_PREFIX "L$"
static const TagVersion LinkVer = { kLinkIDFormat, 0}; 


STDMETHODIMP cBaseLinkStore::Save(RelationID id, IUnknown* fileunk, int partition)
{
   HRESULT retval = S_OK;
   ITagFile* file;
   IRelation* rel = LinkMan()->GetRelation(id);
   Verify(SUCCEEDED(COMQueryInterface(fileunk,IID_ITagFile,(void**)&file)));

   TagVersion v = LinkVer; 
   if (setup_file(rel,file,RELATION_TAG_PREFIX,&v))
   {
      ILinkQuery* query = GetAll(id);
      // Just write 'em out in order
      for (; !query->Done(); query->Next())
      {
         LinkID id = query->ID(); 
         sLink link;
         query->Link(&link);

         if (IsLinkSavePartition(id, link.source,link.dest, partition) 
             && gpObjSys->Exists(link.source) && gpObjSys->Exists(link.dest))
         {
            file->Write((char*)&id,sizeof(id));
            file->Write((char*)&link,sizeof(link));
         }
      }
      SafeRelease(query);

      cleanup_file(file);
   }
   else 
      retval = E_FAIL;

   SafeRelease(file);
   SafeRelease(rel);
   
   return retval;
}

// @TODO: Here we are moving links into their SuggestedLinkPartition on load. 
// This is for back-compatibility purposes.  Once all the levels get over, 
// we should probably punt this.  

STDMETHODIMP cBaseLinkStore::Load(RelationID id, IUnknown* fileunk, int partition)
{
   HRESULT retval = S_OK;
   ITagFile* file;
   IRelation* rel = LinkMan()->GetRelation(id);
   Verify(SUCCEEDED(COMQueryInterface(fileunk,IID_ITagFile,(void**)&file)));

   TagVersion v = LinkVer; 
   if (setup_file(rel,file,RELATION_TAG_PREFIX,&v))
   {
      // read 'em and add 'em
      while (file->TellFromEnd() > 0)
      {  
         LinkID linkid;
         sLink link; 
         file->Read((char*)&linkid,sizeof(linkid));
         file->Read((char*)&link,sizeof(link));
         link.source = ObjRemapOnLoad(link.source);
         link.dest   = ObjRemapOnLoad(link.dest);


         if (!gpObjSys->Exists(link.source) 
             || !gpObjSys->Exists(link.dest))
            continue; 

         // at least one of the objects must be in our partition 
         if (!gpObjSys->IsObjLoadPartition(link.source,(eObjPartition)partition)
             && !gpObjSys->IsObjLoadPartition(link.dest,(eObjPartition)partition))
            continue; 
            
         // build the link id based on format 
         if (v.major < kLinkIDFormat)  
         {
            int part = SuggestedLinkPartition(link.source,link.dest); 
            linkid = LINKID_MAKE(id,part,OLD_LINK_IDX(linkid));
         }
         else
         {
            linkid = LINKID_MAKE2(id,LINKID_NON_RELATION(linkid)); 
         }

         link.flavor = id;
         // decide whether or not we need to remap. 

         sLink dummy; 
         if (Get(linkid,&dummy)) // link id collision, remap
         {
            LinkID newlink = Add(&link); 
            AddLinkMappingToTable(newlink,linkid);
            linkid = newlink; 
         }
         else 
            AddAtID(linkid,&link);            
      }

      cleanup_file(file);
   }
   else 
      retval = E_FAIL;

   SafeRelease(file);
   SafeRelease(rel);

   return retval;
}

STDMETHODIMP cBaseLinkStore::Reset(RelationID id)
{
   ILinkQuery* q = GetAll(id);
   for (; !q->Done(); q->Next())
   {
      Remove(q->ID());
   }
   return S_OK;
}

////////////////////////////////////////////////////////////

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cBaseLinkDataStore,ILinkDataStore);

STDMETHODIMP cBaseLinkDataStore::Add(LinkID)
{
   return S_OK;
}

STDMETHODIMP cBaseLinkDataStore::Remove(LinkID)
{
   return S_OK;
}

STDMETHODIMP cBaseLinkDataStore::AddRelation(RelationID)
{
   return S_OK;
}

#define DATA_TAG_PREFIX "LD$"


static const TagVersion LinkDataVer = { kLinkIDFormat, 0}; 

STDMETHODIMP cBaseLinkDataStore::Save(RelationID id , IUnknown* fileunk, int filetype)
{
   HRESULT retval = S_OK;
   ITagFile* file;
   IRelation* rel = LinkMan()->GetRelation(id);
   Verify(SUCCEEDED(COMQueryInterface(fileunk,IID_ITagFile,(void**)&file)));

   TagVersion v = LinkDataVer; 
   if (setup_file(rel,file,DATA_TAG_PREFIX,&v))
   {

      const sRelationDataDesc* desc = rel->DescribeData();
      IDataOps *ops = desc->data_ops;
      
      if (ops)
         ops->AddRef(); 
      else
         ops = new cFixedSizeDataOps(desc->size);

      // Write out the data version
      ulong version = ops->Version();
      file->Write((char*)&version,sizeof(version));

      // wrap a "cWriteable" around the tagfile
      IDataOpsFile* opfile = new cDataOpsTagFile(file);

      ILinkQuery* q = LinkMan()->Query(LINKOBJ_WILDCARD, LINKOBJ_WILDCARD, id);
      for (; !q->Done(); q->Next())
      {
         LinkID id = q->ID();

         sLink link;
         q->Link(&link); 

         if (IsLinkSavePartition(id, link.source, link.dest, filetype))
         {
            void* data = Get(id);
            if (data != NULL)
            {
               // write the id
               file->Write((char*)&id,sizeof(id));
               // write the data, using its write op
               ops->Write(sDatum(data),opfile); 
            }
         }
      }
      SafeRelease(q);

      SafeRelease(opfile);
      SafeRelease(ops);
   
      cleanup_file(file);
   }
   else 
      retval = E_FAIL;
   SafeRelease(file);
   SafeRelease(rel);
   
   return retval;
}

STDMETHODIMP cBaseLinkDataStore::Load(RelationID id , IUnknown* fileunk, int filetype)
{
   HRESULT retval = S_OK;
   ITagFile* file;
   IRelation* rel = LinkMan()->GetRelation(id);
   Verify(SUCCEEDED(COMQueryInterface(fileunk,IID_ITagFile,(void**)&file)));

   TagVersion v = LinkDataVer; 
   if (setup_file(rel,file,DATA_TAG_PREFIX,&v))
   {
      const sRelationDataDesc* desc = rel->DescribeData();
      
      IDataOps *ops = desc->data_ops;
      if (ops)
         ops->AddRef();
      else
         ops = new cFixedSizeDataOps(desc->size); 

      // Read in data version;
      ulong version;
      file->Read((char*)&version,sizeof(version));
      Assert(version == ops->Version(), 
             ("Reading in different version data for type %s (old:%d new:%d)\n",
              desc->type,version,ops->Version()));

      // wrap a DataOpsFile around the tagfile
      IDataOpsFile* opfile = new cDataOpsTagFile(file); 

      while(file->TellFromEnd() > 0)
      {
         LinkID link;
         // read in the link id
         file->Read((char*)&link,sizeof(link));
         // read in the data
         sDatum dat = ops->New();
         ops->Read(&dat,opfile,version);

         // re-swizzle the link id.
         if (v.major < kLinkIDFormat) // old link id format
         {
            int idx = OLD_LINK_IDX(link); // old-format index

            // Search the partitions for this index
            for (int i = 0; i < kNumLinkPartitions; i++)
            {
               link = LinkRemapOnLoad(LINKID_MAKE(id,i,idx)); 

               sLink dummy;  
               if (rel->Get(link,&dummy) && Get(link) == NULL)
               {
                  break; 
               }
            }
         }
         else 
         {
            link = LinkRemapOnLoad(LINKID_MAKE2(id,LINKID_NON_RELATION(link))); 
         }

         // check for existence
         sLink dummy;
         if (rel->Get(link,&dummy))
            Set(link,dat);

         ops->Delete(dat); 
      }
      SafeRelease(opfile);
      SafeRelease(ops); 
      cleanup_file(file);
   }
   else 
      retval = E_FAIL;
   SafeRelease(file);
   SafeRelease(rel);
   
   return retval;
}

STDMETHODIMP cBaseLinkDataStore::Reset(RelationID id)
{
   ILinkQuery* q = LinkMan()->Query(LINKOBJ_WILDCARD, LINKOBJ_WILDCARD, id);
   for (; !q->Done(); q->Next())
   {
      Remove(q->ID());
   }
   SafeRelease(q);
   return S_OK;
}

////////////////////////////////////////////////////////////
// 
// LINK MAPPING TABLE
//

class cLinkMap : public cHashTable<LinkID,LinkID,cHashFunctions>
{

}; 

static cLinkMap* gpLinkMap = NULL; 

void ClearLinkMappingTable(void)
{
   delete gpLinkMap;
   gpLinkMap = NULL; 
}

void AddLinkMappingToTable(LinkID targ, LinkID src)
{
   if (!gpLinkMap)
      gpLinkMap = new cLinkMap; 
   gpLinkMap->Set(src,targ); 
}

LinkID LinkRemapOnLoad(LinkID link)
{
   LinkID retval = link; 
   if (gpLinkMap)
      gpLinkMap->Lookup(link,&retval);
   return retval; 
}
