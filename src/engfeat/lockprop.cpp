/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/lockprop.cpp,v 1.21 2000/01/29 20:22:33 adurant Exp $
// generic "locked" support

#include <string.h>

#include <mprintf.h>
#include <lg.h>
#include <appagg.h>

#include <rendprop.h>
#include <property.h>
#include <propface.h>
#include <propbase.h>
#include <propert_.h>
#include <dataops_.h>
#include <engfeat.h>

#include <relation.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <autolink.h>

#include <sdesc.h>
#include <sdesbase.h>

// for script service/persistance
#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <lockprop.h>
#include <lockscpt.h>
#include <keyprop.h>
#include <tweqctrl.h> // lock anim

// for checkaccess and stuff
#include <contain.h>  // findusefulkey
#include <frobprop.h>

#include <iobjsys.h>
#include <lazyagg.h>

#include <report.h>
#include <objedit.h>

#ifdef THIEF
#include <picklock.h>
#endif

//must be last header
#include <dbmem.h>



#ifdef LOUD_LOCK_MPRINT
#define lock_mprint(x) mprintf x
#else
#define lock_mprint(x)
#endif

/////////////
// Key properties

// look, a whole pile of identical stuff
#define KEY_IMPL kPropertyImplSparseHash

typedef struct {
   bool   master_bit;
   ushort region_mask;
   uchar  lock_id;
} sOldKeyInfo;

typedef cGenericProperty<IKeyInfoProperty,&IID_IKeyInfoProperty,sKeyInfo*> cKeyInfoPropertyBase;

class cKeyOps : public cClassDataOps<sKeyInfo>
{
public:
   STDMETHOD(Read)(sDatum* dat, IDataOpsFile* file, int version)
   {
      if (version > sizeof(sOldKeyInfo))
         return cClassDataOps<sKeyInfo>::Read(dat,file,version);
      else 
      {
         sOldKeyInfo old; 
         if (!dat->value) *dat = New(); 
         sKeyInfo& young = *(sKeyInfo*)dat->value; 
         file->Read(&old,sizeof(old)); 

         // now actually do the conversion.
         young.master_bit = old.master_bit;
         young.region_mask = old.region_mask;
         young.lock_id = old.lock_id;

         return S_OK; 
      }
   }
}; 

class cKeyInfoProperty : public cKeyInfoPropertyBase
{
   //cClassDataOps<sKeyInfo> mOps; 
   cKeyOps mOps;

public: 
   cKeyInfoProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cKeyInfoPropertyBase(desc,store)
   {
      SetOps(&mOps); 
   }

   cKeyInfoProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cKeyInfoPropertyBase(desc,CreateGenericPropertyStore(impl))
   {
      SetOps(&mOps); 
   }

   STANDARD_DESCRIBE_TYPE(sKeyInfo);
};


static sPropertyDesc keySrcPropDesc = 
   {PROP_KEYSRC_NAME,kPropertyNoInherit|kPropertyInstantiate,NULL,0,0,{ENGFEAT_PROP_GROUP,"KeySrc"}};
static sPropertyDesc keyDstPropDesc = 
   {PROP_KEYDST_NAME,kPropertyNoInherit|kPropertyInstantiate,NULL,0,0,{ENGFEAT_PROP_GROUP,"KeyDst"}};

// structure descriptor fun
static sFieldDesc keyinfo_fields [] =
{
   { "MasterBit",  kFieldTypeBool, FieldLocation(sKeyInfo,master_bit) },
   { "RegionMask", kFieldTypeBits, FieldLocation(sKeyInfo,region_mask), kFieldFlagUnsigned },
   { "LockID",     kFieldTypeInt,  FieldLocation(sKeyInfo,lock_id),     kFieldFlagUnsigned },
};

static sStructDesc keyinfo_struct = StructDescBuild(sKeyInfo,kStructFlagNone,keyinfo_fields);

BOOL KeyWorksOnLock(sKeyInfo *key, sKeyInfo *lock)
{
   BOOL rv, region_match;
   region_match=((key->region_mask&lock->region_mask)!=0);
   if (key->master_bit)
      rv=region_match;
   else
      rv=region_match && key->lock_id==lock->lock_id;
   return rv;
}

// looks in contains links for a key which would work on the door
ObjID FindUsefulKey(ObjID container, ObjID door)
{
   sKeyInfo *key, *lock;
   if (!KeyDstProp->Get(door,&lock))
      return OBJ_NULL;
   AutoAppIPtr(ContainSys);
   // now iterate over container objs, trying each as a key
   sContainIter *iter=pContainSys->IterStart(container);
   ObjID use_obj=OBJ_NULL;
   for (;!iter->finished;pContainSys->IterNext(iter))
   {
      ObjID obj=iter->containee;
      if (KeySrcProp->Get(obj,&key))
         if (KeyWorksOnLock(key,lock))
         {
            use_obj=obj;
            break;
         }
   }
   pContainSys->IterEnd(iter);
   return use_obj;
}

IKeyInfoProperty *KeySrcProp;
IKeyInfoProperty *KeyDstProp;
   
BOOL KeyInfoInit(void)
{
   StructDescRegister(&keyinfo_struct);
   KeySrcProp=new cKeyInfoProperty(&keySrcPropDesc,KEY_IMPL);
   KeyDstProp=new cKeyInfoProperty(&keyDstPropDesc,KEY_IMPL);
   return TRUE;
}

void KeyInfoTerm(void)
{
   SafeRelease(KeySrcProp);
   SafeRelease(KeyDstProp);
}

/////////////
// Locked Property and Links

static IBoolProperty *pLockedProp  = NULL;  // is my locked state true or not
static IIntProperty  *pLockCntProp = NULL;  // total lock count on me
static IRelation     *pLockRel     = NULL;  // lock links to my friends

// internal state maintenance code
// sets valid to FALSE if there were no locked objs around
static BOOL _ComputeLockedCount(ObjID obj, ObjID obj_to_ignore, int *cnt)
{  // is the lock valid so far... 
   BOOL lock_valid=obj==obj_to_ignore?FALSE:pLockedProp->IsRelevant(obj);
   int lock_cnt=(obj==obj_to_ignore)?0:(ObjSelfLocked(obj)?1:0);
   ILinkQuery *lock_links=pLockRel->Query(obj,LINKOBJ_WILDCARD);  // find all things i lock link too...
   for (; !lock_links->Done(); lock_links->Next())
   {
      sLink link;
      lock_links->Link(&link);
      if (link.dest!=obj_to_ignore)
      {
         lock_valid=TRUE;
         if (ObjSelfLocked(link.dest)) // if this obj i lock link to is locked, im locked
            lock_cnt++;
      }
   }
   SafeRelease(lock_links);
   *cnt=lock_cnt;
   return lock_valid;
}

// initialize lock count, ie. just go set it, dont script message or update or anything
// this is only called for ones not in the level, so it is really an error
static void _QuestionableInitLockCnt(ObjID obj)
{
   int cnt;
   if (!_ComputeLockedCount(obj,OBJ_NULL,&cnt))
      Warning(("No valid locks attached to %d - why are we here?\n",obj));
   else if (cnt>0)
   {
      Warning(("Hey, %d has lock cnt %d, not in file\n",obj,cnt));
      pLockCntProp->Set(obj,cnt);  // only bother if there is a count, i guess
   }
   else
      ;  // was 0, still is, who cares...
}

void PostLoadLockBuild(void)
{
   sPropertyObjIter iter;
   int cur_count;
   BOOL locked;
   ObjID obj;

   pLockCntProp->IterStart(&iter);
   while (pLockCntProp->IterNextValue(&iter,&obj,&cur_count))
   {
      int new_count;
      BOOL valid;
      valid=_ComputeLockedCount(obj,OBJ_NULL,&new_count);
      if (!valid)
      {
         Warning(("Loaded LockCnt for %d, no valid locks\n",obj));
         pLockCntProp->Delete(obj);
      }
      else
      {
         if (new_count!=cur_count)
            Warning(("New count %d != Disk count %d\n",new_count,cur_count));
         pLockCntProp->Set(obj,new_count);
      }
   }
   pLockCntProp->IterStop(&iter);
   
   pLockedProp->IterStart(&iter);
   while (pLockedProp->IterNextValue(&iter,&obj,&locked))
   {
      if (!pLockCntProp->IsRelevant(obj))
         _QuestionableInitLockCnt(obj);
      ILinkQuery *lock_links=pLockRel->Query(obj,LINKOBJ_WILDCARD);  // find all things i lock link too...
      for (; !lock_links->Done(); lock_links->Next())
      {
         sLink link;
         lock_links->Link(&link);
         if (!pLockCntProp->IsRelevant(link.dest))
            _QuestionableInitLockCnt(link.dest);
      }
      SafeRelease(lock_links);
   }
   pLockedProp->IterStop(&iter);
}

#define kLockUnlock 0x0001
#define kLockLock   0x0002

// go recompute the lockcount
static void _RecomputeLockCnt(ObjID obj, ObjID obj_to_ignore)
{
   int old_cnt, cnt;
   int what_to_do=0;
   BOOL valid;

   valid=_ComputeLockedCount(obj,obj_to_ignore,&cnt);
   if (!pLockCntProp->Get(obj,&old_cnt))
      old_cnt=0;
   if ((old_cnt==0)&&(cnt>0))
      what_to_do=kLockLock;
   if ((old_cnt>0)&&(cnt==0))
      what_to_do=kLockUnlock;
   if (what_to_do!=0)
   {
      AutoAppIPtr_(ScriptMan,pScriptMan);
      if (what_to_do==kLockUnlock)
      {
         sScrMsg unlock_msg(obj,SCR_UNLOCK_MSG);
         pScriptMan->SendMessage(&unlock_msg);
         lock_mprint(("Sent unlock to %d\n",obj));
      }
      else if (what_to_do==kLockLock)
      {
         sScrMsg lock_msg(obj,SCR_LOCK_MSG);
         pScriptMan->SendMessage(&lock_msg);
         lock_mprint(("Sent lock to %d\n",obj));
      }
   }
   else
      lock_mprint(("From %d to %d on %d, not bothering\n",old_cnt,cnt,obj));
   if (!valid)
      if (cnt!=0)
         Warning(("No valid links, but real count %d\n",cnt));
      else
         pLockCntProp->Delete(obj); // delete the property
   else
      pLockCntProp->Set(obj,cnt);
}

static void _SlamAllLocksOn(ObjID obj, ObjID ignore_obj, BOOL locked)
{  // go to all locks that lock me - if they are not currently "locked", change that
   cAutoLinkQuery query(pLockRel,obj,LINKOBJ_WILDCARD);
   for (; !query->Done(); query->Next())
      if (query.GetDest()!=ignore_obj)
         if (ObjSelfLocked(query.GetDest())!=locked)
            ObjSetSelfLocked(query.GetDest(),locked);
}

// listeners
#define LOCKED_PROP_LISTENTO (kListenPropModify|kListenPropSet|kListenPropUnset|kListenPropLoad)

#define kActionRecount 0x0001
#define kActionIgnore  0x0002

static LazyAggMember(IObjectSystem) gpObjSys; 

// the property listener
void LGAPI LockedPropListener(sPropertyListenMsg *msg, PropListenerData data)
{

   BOOL cur_val=ObjSelfLocked(msg->obj);   // what is the current value
   int lock_action=0;

   lock_mprint(("PropListen: msg %d obj %d val %d\n",msg->type,msg->obj,cur_val));
   if (msg->type & kListenPropLoad)
      return;  // we dont care, we are loading

   // note: have to check modify first, since it has stricter needs
   if (msg->type & kListenPropUnset)
   { 
      if (!gpObjSys->Exists(msg->obj))
         return; 
      if (cur_val) lock_action=kActionRecount|kActionIgnore; 
   }
   else if (msg->type & kListenPropModify)
      lock_action=kActionRecount;
   else if (msg->type & kListenPropSet)
    { if (cur_val) lock_action=kActionRecount; }
   if (lock_action)
   {
      int ignore_obj=(lock_action&kActionIgnore)?msg->obj:OBJ_NULL;
      _RecomputeLockCnt(msg->obj,ignore_obj);
      BOOL im_locked=ObjSelfLocked(msg->obj);
      ILinkQuery *lock_links=pLockRel->Query(LINKOBJ_WILDCARD,msg->obj);
      for (;!lock_links->Done();lock_links->Next())
      {  // find all things that lock link to me ...
         sLink link;
         lock_links->Link(&link);
         if (lock_links->Data()&&(*(int *)lock_links->Data()==1))
            _SlamAllLocksOn(link.source,msg->obj,im_locked);
         _RecomputeLockCnt(link.source,ignore_obj);
      }
      SafeRelease(lock_links);
   }
   // set lock animation frame here?
   LockAnimSetTarget(msg->obj,ObjSelfLocked(msg->obj)?0.0:1.0);
}

// @TODO: what does modify mean
#define LOCKED_LINK_LISTENTO (kListenLinkModify|kListenLinkBirth|kListenLinkPostMortem)
// the link listener
void LGAPI LockedLinkListener(sRelationListenMsg *msg, RelationListenerData data)
{
   int lock_try=0;
   lock_mprint(("LinkListen: msg %d id %d\n",msg->type,msg->id));
   if (ObjSelfLocked(msg->link.dest))  // if the dest isnt locked, who cares...
      _RecomputeLockCnt(msg->link.source,OBJ_NULL);
   // note, we dont do fancy ignore stuff, cause we listen to postmort not death
}  // if we listened to death, wed need to do an if death the ignore_obj is msg->link.dest

/////////////////
// report system attachment
#ifdef REPORT

static void _lock_report(int WarnLevel, void *data, char *buffer)
{
   sPropertyObjIter iter;
   int cur_count;
   ObjID obj;
   char *p=buffer;

   if (WarnLevel<kReportLevel_DumpAll) return;
   
   pLockCntProp->IterStart(&iter);
   while (pLockCntProp->IterNextValue(&iter,&obj,&cur_count))
      if (ReportCheckObjActive(obj))
         rsprintf(&p,"Obj %d lockCntProp %d\n",obj,cur_count);
   pLockCntProp->IterStop(&iter);
}

#define MAX_KEY_CNT 256
static void _key_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
{
   BOOL loud=(WarnLevel==kReportLevel_DumpAll);
   char *p=buffer;
   static ObjID *src_list, *dst_list;
   static int    src_cnt,   dst_cnt;

   if (WarnLevel<kReportLevel_Info) return;

   switch (phase)
   {
      case kReportPhase_Init:      // set up our memory
         rsprintf(&p,"\nKey Report\n");
         src_cnt=dst_cnt=0;
         src_list=(ObjID *) malloc (MAX_KEY_CNT*sizeof(ObjID));
         dst_list=(ObjID *) malloc (MAX_KEY_CNT*sizeof(ObjID));
         break;
      case kReportPhase_Loop:      // get our private data
         sKeyInfo *pKeyData;
         if (KeySrcProp->Get(obj,&pKeyData))
            if (src_cnt>=MAX_KEY_CNT)
               Warning(("Out of source slots for keys while reporting\n"));
            else
               src_list[src_cnt++]=obj;
         if (KeyDstProp->Get(obj,&pKeyData))
            if (dst_cnt>=MAX_KEY_CNT)
               Warning(("Out of dest slots for keys while reporting\n"));
            else
               dst_list[dst_cnt++]=obj;
         break;
      case kReportPhase_Term:      // now, the horror, cross index everything with its brother
         if (loud)                 // go through and list all the keys
            for (int src_idx=0; src_idx<src_cnt; src_idx++)
            {  // lets talk about this key
               sKeyInfo *pKeyData;
               if (KeySrcProp->Get(src_list[src_idx],&pKeyData))
                  if (pKeyData->master_bit)
                     rsprintf(&p,"%s is a Master Key for region mask %x\n",ObjEditName(src_list[src_idx]),pKeyData->region_mask);
                  else
                     rsprintf(&p,"%s is a normal Key region %x lock id %d\n",ObjEditName(src_list[src_idx]),pKeyData->region_mask,pKeyData->lock_id);
               else
                  Warning(("In Key Report, have a key w/o key property\n"));
            }
         for (int dst_idx=0; dst_idx<dst_cnt; dst_idx++)
         {
            sKeyInfo *pKeyDst, *pKeySrc;
            BOOL any_work=FALSE;
            KeyDstProp->Get(dst_list[dst_idx],&pKeyDst);
            if (loud)  // lets talk about this lock
               rsprintf(&p,"%s is a lock for region %x lock id %d\n  opened by ",ObjEditName(dst_list[dst_idx]),pKeyDst->region_mask,pKeyDst->lock_id);
            for (int src_idx=0; src_idx<src_cnt; src_idx++)
            {          // now lets go through each of the keys looking for a match
               KeySrcProp->Get(src_list[src_idx],&pKeySrc);
               if (KeyWorksOnLock(pKeySrc,pKeyDst))
               {
                  any_work=TRUE;
                  if (loud)
                     rsprintf(&p,"%s ",ObjEditName(src_list[src_idx]));
               }
            }
            if (!any_work)
            {
               if (loud)
                  rsprintf(&p,"Nothing\n");
               rsprintf(&p,"Potential Error, lock %s has no keys which open it\n",ObjEditName(dst_list[dst_idx]));
            }
            else if (loud)
               rsprintf(&p,"\n");
         }
         free(src_list);
         free(dst_list);
         break;      
   }
}
#endif

/////////////////
// actual initialization

#define LOCKED_PROP_IMPL kPropertyImplSparseHash
static sPropertyDesc     LockedPropDesc  = { "Locked",  0, NULL, 0, 0, {ENGFEAT_PROP_GROUP,"Locked"}};
static sPropertyDesc     LockCntPropDesc = { "LockCnt", kPropertyNoEdit };

#define LOCK_RELATION_QCASES (kQCaseSetSourceKnown|kQCaseSetDestKnown|kQCaseSetBothKnown)
static sRelationDesc     lockRelDesc     = { "Lock",      };
static sRelationDataDesc lockDataDesc    = LINK_DATA_DESC(int);

void EngineLockInit(void)
{  
   pLockedProp  = CreateBoolProperty(&LockedPropDesc,LOCKED_PROP_IMPL);
   pLockedProp->Listen(LOCKED_PROP_LISTENTO,LockedPropListener,NULL);
   pLockCntProp = CreateIntProperty(&LockCntPropDesc,LOCKED_PROP_IMPL);
   pLockRel=CreateStandardRelation(&lockRelDesc,&lockDataDesc,LOCK_RELATION_QCASES);
   pLockRel->Listen(LOCKED_LINK_LISTENTO,LockedLinkListener,(RelationListenerData)0);
   KeyInfoInit();
   ReportRegisterGenCallback(_lock_report,kReportAllObj,"Lock System",NULL);
   ReportRegisterObjCallback(_key_report,"Key System",NULL);
}  // games will also install there own key properties, i assume

void EngineLockTerm(void)
{
   KeyInfoTerm();
   ReportUnRegisterGenCallback(_lock_report,kReportAllObj,NULL);
   ReportUnRegisterObjCallback(_key_report,NULL);
   SafeRelease(pLockCntProp);
   SafeRelease(pLockedProp);
   SafeRelease(pLockRel);
}

///////////////
// actually use/get at the data

// simple accessors to the locked property, ie. my own locks
BOOL ObjSelfLocked(ObjID obj)
{
   BOOL locked=FALSE;
   Assert_(pLockedProp);
   pLockedProp->Get(obj,&locked);
   return locked;
}

void ObjSetSelfLocked(ObjID obj, BOOL locked)
{
   Assert_(pLockedProp);
#ifdef THIEF
   ResetPickState(obj);
#endif
   pLockedProp->Set(obj,locked);
}

BOOL ObjComputeLockedState(ObjID obj)
{
   int lock_cnt=0;
   Assert_(pLockCntProp);
   pLockCntProp->Get(obj,&lock_cnt);
   return lock_cnt>0;
}

///////////////
// API for the AI to try and use stuff

BOOL SolveLock(ObjID lock_obj, ObjID user, sObjAccess *access)
{
   sKeyInfo *pDst;   
   if (KeyDstProp->Get(lock_obj,&pDst))
   {
      ObjID use_obj=FindUsefulKey(user,lock_obj);
      if (use_obj)
      {
         access->frobee=lock_obj;
         access->frober=use_obj;
         access->flags=kObjAccessAnd;
         return TRUE;
      }
   }
   return FALSE;
}

// pass in an array of MAX_OBJ_ACCESS sObjAccess'
int CheckObjectAccess(ObjID accessor, ObjID target, sObjAccess *accessList, BOOL getAllLocks)
{
   int cnt=0;

   if (!ObjComputeLockedState(target))
   {
      // am i world frobbable
      sFrobInfo *frobData;
      if (!pFrobInfoProp->Get(target,&frobData)||
          (frobData->actions[kFrobLocWorld]==0))
      {  // for now we assume if actions NULL, you are FrobInert - so find control devices
         cAutoLinkQuery query("ControlDevice",LINKOBJ_WILDCARD,target);
         for (; !query->Done(); query->Next())     // find everything that locks me
         {
            accessList[cnt].frobee=query.GetSource();
            accessList[cnt].frober=OBJ_NULL;
            accessList[cnt].flags=kObjAccessOr;
            cnt++;
         }
      }
      else if (!getAllLocks)
      {
         accessList[cnt].frobee=target;
         accessList[cnt].frober=OBJ_NULL;
         accessList[cnt].flags=0;
         cnt++;
      }
      if (!getAllLocks)
         return cnt;
   }
   if (ObjSelfLocked(target)||getAllLocks)
   {
      if (SolveLock(target,accessor,&accessList[cnt]))
         cnt++;
      else if (!getAllLocks)
         return 0;
   }

   cAutoLinkQuery query(pLockRel,target,LINKOBJ_WILDCARD);
   for (; !query->Done(); query->Next())     // find everything that locks me
      if (ObjSelfLocked(query.GetDest())||getAllLocks)    // it is locked
         if (SolveLock(query.GetDest(),accessor,&accessList[cnt]))
         {
            if (query->Data() && (*(int *)query->Data()==1))
               accessList[cnt].flags=kObjAccessOr;
            else
               accessList[cnt].flags=kObjAccessAnd;
            cnt++;
            AssertMsg(cnt<MAX_OBJ_ACCESS,"CheckObjAccess: Too Many Access Objects\n");
         }
         else if (!getAllLocks)
            return 0;

   if (getAllLocks && cnt==0)
   {
      accessList[cnt].frobee=target;
      accessList[cnt].frober=OBJ_NULL;
      accessList[cnt].flags=0;
      cnt++;
   }
   
   // @TODO
   //   check locked objects
   //     - picks (not that AIs can pick, mind you)
   //   check frobproxies
   //     - check them in the same way
   
   return cnt;
}
