/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/motset.cpp,v 1.19 1999/12/27 15:26:01 BODISAFA Exp $

#include <motset.h>
#include <motdbase.h>
extern "C" {
#include <mp.h>
}
#include <string.h>
#include <appagg.h>
#include <objpos.h>
#include <resapi.h>
#include <binrstyp.h>
#include <motdmnge.h>
#include <motdmng_.h>
#include <motdesc.h>
#include <motmngr.h>
#include <creature.h> // for callbacks
#include <mvrflags.h>

#include <dynarray.h>
#include <namemap.h>
#include <cfgdbg.h>     

#include <dbmem.h> // must be last included header


#define E398_HACK

EXTERN void MotCompRotCallback(mps_motion_info *mi,mps_comp_motion *cm,float frame, quat *data);
static void MotCompXlatCallback(multiped *mp, mps_motion_info *mi,mps_comp_motion *cm,float frame, mxs_vector *data);
static void MotRootRotCallback(multiped *mp, mps_motion_info *mi,float frame, quat *data);
static void MotFilterCallback(multiped*mp, mps_motion_info *mi,quat *rot, mxs_vector *xlat);

class cMpsMotion : public mps_motion
{
public:
   cMpsMotion(mps_motion *mot=NULL);

// NOTE: destructor does not free the flags or components, just as
// constructor doesn't alloc them.  This data gets freed on
// mp_close if and only if the motion has been mp_add_motion-ed,
// which happens during runtime setup.
   ~cMpsMotion() {}

   void Read(ITagFile *pFile);
   void Write(ITagFile *pFile);
};

cMpsMotion::cMpsMotion(mps_motion *mot)
{
   if(!mot)
   {
      num_components=0;
      components=NULL;
      num_flags=0;
      flags=NULL;
      info.type=MT_CAPTURE;
      info.num_frames=0;
      info.name[0]='\0';
      return;
   }
   info=mot->info;
   if(info.type==MT_CAPTURE)
   {
      num_components=mot->num_components;
      components=mot->components;
   } else
   {
      virtual_update=mot->virtual_update;
   }
   num_flags=mot->num_flags;
   flags=mot->flags;
}

void cMpsMotion::Read(ITagFile *pFile)
{
   mps_motion mot;

   // delete any existing alloced stuff
   if(flags)
   {
      mp_free(flags);
      flags=NULL;
   }
   if(components)
   {
      mp_free(components);
      components=NULL;
   }

   ITagFile_Read(pFile,(char *)&mot,sizeof(mot));
   info=mot.info;
   AssertMsg(info.type==MT_CAPTURE,"can't read virtual motions");
   num_components=mot.num_components;
   num_flags=mot.num_flags;

   if(mot.num_components)
   {
      components=(mps_comp_motion *) mp_alloc(sizeof(mps_comp_motion) * mot.num_components,__FILE__,__LINE__);
      ITagFile_Read(pFile,(char *)components,sizeof(*mot.components)*mot.num_components);
   } else
   {
      components=NULL;
   }

   // read flags
   if(mot.num_flags)
	{
	   flags=(mps_motion_flag *) mp_alloc(sizeof(mps_motion_flag) * mot.num_flags,__FILE__,__LINE__);
      ITagFile_Read(pFile,(char *)flags,sizeof(*mot.flags)*mot.num_flags);
	} else
	{
	   flags=NULL;
	}
}

void cMpsMotion::Write(ITagFile *pFile)
{
   mps_motion mot;

   AssertMsg(info.type==MT_CAPTURE,"cannot write virtual motion");
   mot.info=info;
   mot.num_components=num_components;
   mot.components=NULL;
   mot.num_flags=num_flags;
   mot.flags=NULL;
   
   ITagFile_Write(pFile,(char *)&mot,sizeof(mot));
   ITagFile_Write(pFile,(char *)components,sizeof(*components)*num_components);
   ITagFile_Write(pFile,(char *)flags,sizeof(*flags)*num_flags);
}


class cMotStuffList : public cDynArray<sMotStuff> {};
class cMotIndexList : public cDynArray<int> {};


// ids here are same as in mp_motion_list, so you can pass them into
// creature->PlayMotion
class cMotionSet : public IMotionSet
{
public:
   cMotionSet();
   ~cMotionSet();

// run-time functions
//
   virtual BOOL NeckIsFixed(int motHandle);
   virtual int  BlendLength(int motHandle);
   virtual void GetStartEndFrames(int motHandle, int *pStart, int *pEnd);
   virtual int GetNumFrames(int motHandle);
   virtual mxs_ang GetEndHeading(int motHandle);
   virtual void GetTranslation(int motHandle, mxs_vector *pXlat);
   virtual float GetDuration(int motHandle);
   virtual void GetSlidingVelocity(int motHandle, mxs_vector *pVel);
   virtual BOOL WantInPlace(int motHandle);

   virtual Label *GetName(int motHandle);
   virtual int  GetMotHandleFromName(const Label *name);

   //this is correct even if motions not all loaded successfully
   virtual int  NumMotions() { return m_RunHandle2Index.Size(); }

// load/save functions
//
   virtual BOOL Load(ITagFile *pFile);
   virtual BOOL Save(ITagFile *pFile);
   virtual BOOL GetRunHandle(int index, int *pRunHandle);

// set-building functions
//
   virtual void Clear();
   virtual void StartBuild() { Clear(); }
   virtual void EndBuild() { SetupRunTimeData(); DippyComputeAllMotPhys(); }
   virtual BOOL AddMotion(const Label *name, const sMotStuff *pStuff, int *pIndex);

// virtual motion dealing.  These need to be registered before "add motion" gets called.
   void RegisterVirtualMotion(const Label *name,const mps_motion *pMotion);

private:
   BOOL SetupMotion(const Label *name,int id,int *runHandle);
   void SetupRunTimeData();
   void DippyComputeAllMotPhys();
   void HackComputeMotPhys(int runHandle, int index);


   cMotStuffList m_MotStuffList; // mapping from mot index to stuff
   // note: if all motions are successfully loaded, these will be the identity mappings
   cMotIndexList m_Index2RunHandle; // mapping from mot index to mot run-time handle
   cMotIndexList m_RunHandle2Index; // mapping from mot run-time handle to mot index
   cNameMap m_NameMap;
#ifdef E398_HACK
   BOOL m_HackSuspendInPlace;
#endif

   // virtual motion stuff - this never gets cleared out
   cNameMap m_VMotNameMap; 
   cDynArray<mps_motion> m_VMotList;
   cDynClassArray<cMpsMotion> m_cMoCapList;
};



cMotionSet::cMotionSet()
{
   mp_init(1);
   MotDmngeInit(1);
   mp_set_motion_start_end_callbacks(MotDmngeLock,MotDmngeUnlock);
   mp_set_capture_callbacks(MotCompXlatCallback,MotCompRotCallback,MotRootRotCallback);
   mp_set_rot_filter_callback(MotFilterCallback);
#ifdef E398_HACK
   m_HackSuspendInPlace=FALSE;
#endif
}

cMotionSet::~cMotionSet()
{
   mp_close();
   MotDmngeClose();
}

BOOL cMotionSet::NeckIsFixed(int motHandle)
{
   if(motHandle<0)
      return FALSE;
   Assert_(motHandle<m_RunHandle2Index.Size());
   return m_MotStuffList[m_RunHandle2Index[motHandle]].flags&kMSF_NECK_IS_FIXED;
}

void cMotionSet::GetStartEndFrames(int motHandle, int *pStart, int *pEnd)
{
   *pStart=0;
   *pEnd=-1;
   if(motHandle<0 || motHandle>mp_num_motions)
   {
      return;
   }
   mps_motion *pMot;
   int i;
   mps_motion_flag *pF;

   pMot=mp_motion_list+motHandle;
   pF=pMot->flags;
   for(i=0;i<pMot->num_flags;i++,pF++)
   {
      if(pF->flags&MF_START_MOT)
      {
         *pStart=pF->frame;
      }
      if(pF->flags&MF_END_MOT)
      {
         *pEnd=pF->frame;
      }
   }
}

int cMotionSet::GetNumFrames(int motHandle)
{
   if(motHandle<0 || motHandle>=mp_num_motions)
      return 0;

   return ((mps_motion *)(mp_motion_list+motHandle))->info.num_frames;
}

#define kMotDefaultBlendLength 500 // in msec

int cMotionSet::BlendLength(int motHandle)
{
   if(motHandle<0)
      return 0;
   Assert_(motHandle<m_RunHandle2Index.Size());

   return m_MotStuffList[m_RunHandle2Index[motHandle]].blendLength;
}

mxs_ang cMotionSet::GetEndHeading(int motHandle)
{
   int id=m_RunHandle2Index[motHandle];

   Assert_(id>=0&&id<m_MotStuffList.Size());
   return m_MotStuffList[id].endDirAction;
}

float cMotionSet::GetDuration(int motHandle)
{
   int id=m_RunHandle2Index[motHandle];

   Assert_(id>=0&&id<m_MotStuffList.Size());
   return m_MotStuffList[id].duration;
}

void cMotionSet::GetTranslation(int motHandle, mxs_vector *pXlat)
{
   int id=m_RunHandle2Index[motHandle];

   Assert_(id>=0&&id<m_MotStuffList.Size());

   mx_copy_vec(pXlat,&m_MotStuffList[id].xlat);
}

void cMotionSet::GetSlidingVelocity(int motHandle, mxs_vector *pVel)
{
   int id=m_RunHandle2Index[motHandle];

   Assert_(id>=0&&id<m_MotStuffList.Size());

   sMotStuff *pStuff=&m_MotStuffList[id];

   if(pStuff->flags&kMSF_IS_LOCO)
   {
      mx_copy_vec(pVel,&pStuff->xlat);
      mx_scaleeq_vec(pVel,1.0/m_MotStuffList[id].duration);
   } else
   {
      mx_zero_vec(pVel);
   }
}

BOOL cMotionSet::WantInPlace(int motHandle)
{
   int id=m_RunHandle2Index[motHandle];

   Assert_(id>=0&&id<m_MotStuffList.Size());

#ifdef E398_HACK
   if(m_HackSuspendInPlace)
      return FALSE;
#endif

   return m_MotStuffList[id].flags&kMSF_WANT_NO_XLAT;
}

Label *cMotionSet::GetName(int motHandle)
{
   return m_NameMap.NameFromID(m_RunHandle2Index[motHandle]);
}

int cMotionSet::GetMotHandleFromName(const Label *name)
{
   int index=m_NameMap.IDFromName(name);

   if(index==kNameMapIndexNotFound)
      return -1;
   return m_Index2RunHandle[index];
}

// this is a hack because this data should really be calculated in 
// motion processor, not on load, and should probably be more
// sophisticated.   KJ 5/19/98
void cMotionSet::HackComputeMotPhys(int runHandle, int index)
{
   Assert_(runHandle>=0&&runHandle<mp_num_motions);

   sMotStuff *pStuff=&m_MotStuffList[index];
   mps_motion_info *pInfo=&((mp_motion_list+(runHandle))->info);

   Assert_(pInfo);

   pStuff->duration=pInfo->num_frames/pInfo->freq;

   if(pInfo->type!=MT_CAPTURE)
   {
      pStuff->endDirAction=0;
      mx_zero_vec(&pStuff->xlat);
      return;
   }

   Assert_(pInfo->num_frames>0);

   MotDmngeLock(runHandle);

   // compute endDirAction
   if(pStuff->flags&kMSF_IS_TURN)
   {
      // end dir action is end butt facing

      mps_motion *m=mp_motion_list+runHandle;
      quat q;
      mxs_matrix mat;
      mxs_angvec ang;

      // first check if motion is an overlay.  
      // Full body motions always have xlat component first
      if(m->components[0].type!=CM_TRANS)
      {
         // then overlay
         pStuff->endDirAction=0;
      }
      else
      {
         // butt joint is same as xlat joint
         int joint=m->components[0].joint_id;
         int i;
         mps_comp_motion *pComp=m->components+1;

         for(i=1;i<m->num_components;i++,pComp++)
         {
            if(pComp->joint_id==joint)
               break;
         }
         AssertMsg1(i < m->num_components, "HackComputeMotPhys: joint id %d match failed.", pComp->joint_id);
         Assert_(pComp->type==CM_ROT);
         MotCompRotCallback(pInfo,pComp,pInfo->num_frames-1,&q);
         quat_to_matrix(&mat,&q);
         mx_mat2ang(&ang,&mat);
         pStuff->endDirAction=ang.tz;

         // assumption is that start dir action is zero.  sanity
         // check this.
         MotCompRotCallback(pInfo,pComp,0,&q);
         quat_to_matrix(&mat,&q);
         mx_mat2ang(&ang,&mat);
         if(ang.tz>0x1000&&ang.tz<0xf000)
         {
            Warning(("HackComputeMotPhys: turn %s has starting orient of %x\n",pInfo->name,ang.tz));
         }
      }
   } else
   {
      pStuff->endDirAction=0;
   }

#ifdef E398_HACK
   m_HackSuspendInPlace=TRUE;
#endif
   // compute xlat and average speed
   mp_get_xlat_at_frame(&pStuff->xlat,runHandle,pInfo->num_frames-1);
#ifdef E398_HACK
   m_HackSuspendInPlace=FALSE;
#endif
   MotDmngeUnlock(runHandle);
}

void cMotionSet::SetupRunTimeData()
{
   // try loading motions, and build index<->runHandle mappings
   int i,size,rh,last;
   Label *name;

   size=m_NameMap.Size();

   // setup multiped library and res locking stuff
   mp_init(size);
   MotDmngeInit(size);
   mp_set_motion_start_end_callbacks(MotDmngeLock,MotDmngeUnlock);
   mp_set_capture_callbacks(MotCompXlatCallback,MotCompRotCallback,MotRootRotCallback);

   m_Index2RunHandle.SetSize(size);
   for(i=0;i<size;i++)
   {
      name=m_NameMap.NameFromID(i);
      if(SetupMotion(name,i,&rh))
      {
         last=m_RunHandle2Index.Append(i);
         AssertMsg(rh==last,"MotionSet run time data out of synch");
         m_Index2RunHandle.SetItem(&last,i);
      } else
      {
         Warning(("Could not load motion %s\n",name));
         rh=-1;
         m_Index2RunHandle.SetItem(&rh,i);
      }
   }
   
}

void cMotionSet::DippyComputeAllMotPhys()
{
   int i,size,rh;

   size=m_Index2RunHandle.Size();
   for(i=0;i<size;i++)
   {
      rh=m_Index2RunHandle[i];
      if(rh>=0)
         HackComputeMotPhys(rh,i);
   }
}

BOOL cMotionSet::Load(ITagFile *pFile)
{
   // load namemap
   m_NameMap.Load(pFile);
   
   // load motstuff
   int i;
   ulong size;
   sMotStuff stuff;

   ITagFile_Read(pFile,(char *)&size,sizeof(size));
   m_MotStuffList.SetSize(size);
   for(i=0;i<size;i++)
   {
      ITagFile_Read(pFile,(char *)&stuff,sizeof(stuff));
      m_MotStuffList.SetItem(&stuff,i);
   }

   ITagFile_Read(pFile,(char *)&size,sizeof(size));
   m_cMoCapList.SetSize(size);
   for(i=0;i<size;i++)
   {
      cMpsMotion m;

      m.Read(pFile);
      m_cMoCapList.SetItem(&m,i);
   }

   // setup run-time data
   SetupRunTimeData();

   return TRUE;
}

BOOL cMotionSet::Save(ITagFile *pFile)
{
   // save off name map 
   m_NameMap.Save(pFile);

   // save off motstuff
   ulong size=m_MotStuffList.Size();
   int i;

   ITagFile_Write(pFile,(char *)&size,sizeof(size));
   for(i=0;i<size;i++)
   {
      ITagFile_Write(pFile,(char *)&m_MotStuffList[i],sizeof(m_MotStuffList[i]));
   }
   size=m_cMoCapList.Size(); // this should always be same size as motstufflist.
   ITagFile_Write(pFile,(char *)&size,sizeof(size));
   for(i=0;i<size;i++)
   {
      m_cMoCapList[i].Write(pFile);
   }

   return TRUE;
}

BOOL cMotionSet::GetRunHandle(int index, int *pRunHandle)
{
   int rh=m_Index2RunHandle[index];

   if(rh>=0)
   {
      *pRunHandle=rh;
      return TRUE;
   } else
   {
      return FALSE;
   }
}

// set-building functions
void cMotionSet::Clear()
{
   m_Index2RunHandle.SetSize(0);
   m_RunHandle2Index.SetSize(0);
   m_MotStuffList.SetSize(0);
   m_cMoCapList.SetSize(0);
   m_NameMap.Clear();

   mp_close(); // this frees up mp_motion_list
   MotDmngeClose();
}

// needs to be called between startbuild and endbuild calls, since doesn't setup
// run time data.
BOOL cMotionSet::AddMotion(const Label *name, const sMotStuff *pStuff, int *pIndex)
{
   int index;

   // first check if motion already in set.
   index=m_NameMap.IDFromName(name);
   if(index!=kNameMapIndexNotFound) // motion already added
   {
      // motstuff must be identical, otherwise fail
      if(m_MotStuffList[index].flags!=pStuff->flags||
         m_MotStuffList[index].blendLength!=pStuff->blendLength)
      {
         AssertMsg1(FALSE,"Motion %s already added with different stuff\n",name->text);
		   return FALSE;
      }
      *pIndex=index;
      return TRUE;
   }

   index=m_MotStuffList.Append(*pStuff);
   m_NameMap.AddNameAtIndex(name,index); // know these will be consecutive

   AssertMsg(m_NameMap.Size()==m_MotStuffList.Size(),"motion set is out of synch");

   *pIndex=index;

   // try loading the mo-cap .mi file, so don't need to do so at run time
   AutoAppIPtr(ResMan);
   IRes *pRes;
   char fname[32];
   mps_motion *mot;
   mps_comp_motion *components;
   mps_motion_flag *flags;

   strcpy(fname,name->text);
   strcat(fname,".mi");
   pRes = pResMan->Bind(fname, RESTYPE_BINARY, NULL, "motions\\");
   if (!pRes) {
      cMpsMotion m;

      ConfigSpew("yakspew",("motion %s .mi file not found, is virtual?\n",name->text));
      SafeRelease(pRes);

      // add an empty motion
      m_cMoCapList.SetSize(index+1); // same as motstufflist index
      m_cMoCapList.SetItem(&m,index);
      return TRUE;
   }
   mot=(mps_motion *) pRes->Lock();

   // @TODO: get rid of name field in mps_motion_info, since not used.
   // (or start using it?).  Right now, name is likely to be out of synch,
   // since motion files get renamed fairly frequently.

   // mp_alloc-ed stuff gets freed on mp_close, as long as motion was
   // mp_add_motion-ed.
   components=(mps_comp_motion *) mp_alloc(sizeof(mps_comp_motion) * mot->num_components,__FILE__,__LINE__);
   memcpy(components,mot+1,sizeof(mps_comp_motion) * mot->num_components);

   // read flags
   if(mot->num_flags)
	{
	   flags=(mps_motion_flag *) mp_alloc(sizeof(mps_motion_flag) * mot->num_flags,__FILE__,__LINE__);
	   memcpy(flags,((char *)mot)+sizeof(mps_motion)+sizeof(mps_comp_motion)*mot->num_components,sizeof(mps_motion_flag) * mot->num_flags);
	} else
	{
	   flags=NULL;
	}
   mot->components=components;
   mot->flags=flags;
   cMpsMotion m(mot);

   m_cMoCapList.SetSize(index+1); // same as motstufflist index
   m_cMoCapList.SetItem(&m,index);

   // put mot somewhere
   pRes->Unlock();
   SafeRelease(pRes);

   return TRUE;
}

// NOTE: This does not check whether motion was previously loaded,
// however AddMotion prevents a motion from appearing in list twice.
BOOL cMotionSet::SetupMotion(const Label *name,int id,int *runHandle)
{
   // check if motion is a virtual motion
   int index;

   index=m_VMotNameMap.IDFromName(name);
   if(index!=kNameMapIndexNotFound)
   {
      // add the virtual motion to motion list
      *runHandle=mp_add_motion(&m_VMotList[index]);
      return TRUE;
   }

   // check that mot loaded mi file properly at db build time
   if(!m_cMoCapList[id].components) // then mi info never set
   {
      Warning(("cannot setup motion %s. is .mi file in motion database?\n",name->text));
      return FALSE;
   }   
   *runHandle=mp_add_motion(&m_cMoCapList[id]);

   return TRUE;
}

// virtual motion registering, so loading works
void cMotionSet::RegisterVirtualMotion(const Label *name,const mps_motion *pMotion)
{
   int index;

   Assert_(pMotion->info.type==MT_VIRTUAL);
   index=m_VMotList.Append(*pMotion);
   m_VMotNameMap.AddNameAtIndex(name,index);
}

IMotionSet *NewMotionSet()
{
   return new cMotionSet;
}


///////////////////////////////////////////////////
///////////////////////////////////////////////////
///// Global functions
/////
///////////////////////////////////////////////////

#define E398_HACK

static void MotCompXlatCallback(multiped *mp, mps_motion_info *mi,mps_comp_motion *cm,float frame, mxs_vector *data)
{
   mxs_vector *v;
   IRes *pRes;
   MotDataCTable *cTable=NULL;
   BOOL lockedHere=FALSE;
   int iframe=(int)frame;

   if(mp && mp->app_ptr && ((cCreature *)mp->app_ptr)->IsBallistic())
   {
      // OLD - x_copy_vec(data,&mp->global_pos);  // use global pos
#if 0 //! MTR - This set an absolute position when an offset is wanted.
      Position *pPos=ObjPosGet(((cCreature *)mp->app_ptr)->GetObjID());
      if(pPos)
         mx_copy_vec(data,&pPos->loc.vec);
      else
         mx_zero_vec(data);
#endif
      AssertMsg(mp->frame_of_reference != MFRT_REL_START, "Expected ballistic to have global frame");
      mx_zero_vec(data);
      return;
   } 

   if(mi->mot_num<0)
   {
      Warning(("MotCompXlatCallback: invalid motion passed\n"));
      mx_zero_vec(data);
      return;
   }
   if(!MotDmngeIsLocked(mi->mot_num))
   {
      ConfigSpew("MotLockTrack",("MotCompXlatCallback: locking motion %d\n",mi->mot_num));
      MotDmngeLock(mi->mot_num);
      lockedHere=TRUE;
   }
   pRes=MotDmngeHandle(mi->mot_num);
   if(!pRes)
   {
      Warning(("MotCompXlatCallback: cannot get data handle\n"));
      mx_zero_vec(data);
      if(lockedHere)
         MotDmngeUnlock(mi->mot_num);
      return;
   }
   // @TBD (justin 6-26-98): DataPeek is an evil hack, and should be
   // replaced by a proper Lock by someone who understands this code
   // well...
   cTable=(MotDataCTable *) pRes->DataPeek();
   if(!cTable)
   {
      Warning(("MotCompXlatCallback: cannot get data from handle\n"));
      mx_zero_vec(data);
      if(lockedHere)
         MotDmngeUnlock(mi->mot_num);
      return;
   }
   if(cm->handle>=cTable->numEntries)
   {
      mx_zero_vec(data);
      if(lockedHere)
         MotDmngeUnlock(mi->mot_num);
      return;
   }
   v=(mxs_vector *)(((uchar *)cTable)+cTable->offset[cm->handle]);
   if(iframe+1==mi->num_frames)
      mx_copy_vec(data,&v[iframe]);
   else
      mx_interpolate_vec(data,&v[iframe],&v[iframe+1],frame-iframe);
#ifdef E398_HACK
   // keep motion in place if don't want it to have moved at end but
   // want to keep butt free during motion
   Assert_(g_pMotionSet);
   if(g_pMotionSet->WantInPlace(mi->mot_num))
   {
      mxs_vector totXlat;
      float frac;

      g_pMotionSet->GetTranslation(mi->mot_num,&totXlat);
      frac=(frame+1)/mi->num_frames;
      mx_scaleeq_vec(&totXlat,-frac);
      mx_addeq_vec(data,&totXlat);
   }
#endif

   if(lockedHere)
   {
      MotDmngeUnlock(mi->mot_num);
   }
}

///////////////////////////////////////////////////

#define E398_HACK

#ifdef E398_HACK

EXTERN BOOL g_DoRootHack=FALSE;
EXTERN int  g_HackButtJointID=0; 

#endif

// XXX this is really a very bad thing to do, but might still be
// workable if every creature type we invent used the same id for the
// neck joint. hmm.  The real solution is to do away with this neck hack,
// which means solving the capture distortion problem for real.
#define NECK 9

EXTERN BOOL g_mot_quat_debug=FALSE;

// @TODO: GET RID OF NECK HACK!
void MotCompRotCallback(mps_motion_info *mi,mps_comp_motion *cm,float frame, quat *data)
{
   quat *q;
   IRes *pRes;
   MotDataCTable *cTable=NULL;
   BOOL lockedHere=FALSE;
   int iframe=(int)frame;

   if(mi->mot_num<0)
   {
      Warning(("MotCompRotCallback: invalid motion passed\n"));
      quat_identity(data);
      return;
   }
   if(!MotDmngeIsLocked(mi->mot_num))
   {
      ConfigSpew("MotLockTrack",("MotCompRotCallback: locking motion %d\n",mi->mot_num));
      MotDmngeLock(mi->mot_num);
      lockedHere=TRUE;
   }
   pRes=MotDmngeHandle(mi->mot_num);
   if (!pRes)
   {
      Warning(("MotCompRotCallback: cannot get data from handle\n"));
      quat_identity(data);
      if(lockedHere)
         MotDmngeUnlock(mi->mot_num);
      return;
   }
   // @TBD (justin 6-26-98): This DataPeek should be replaced by a proper
   // Lock...
   cTable=(MotDataCTable *) pRes->DataPeek();
   if(!cTable)
   {
      Warning(("MotCompRotCallback: cannot get data from handle\n"));
      quat_identity(data);
      if(lockedHere)
         MotDmngeUnlock(mi->mot_num);
      return;
   }
   if(cm->handle>=cTable->numEntries)
   {
      quat_identity(data);
      if(lockedHere)
         MotDmngeUnlock(mi->mot_num);
      return;
   }
   // XXX this is a hack for fixing the neck in messed up motions.
   // still want to figure out how to deal with motion distortion problems..
   if(cm->joint_id==NECK && (MotDescNeckFixed(mi->mot_num)))
   {
      quat_identity(data);
      if(lockedHere)
         MotDmngeUnlock(mi->mot_num);
      return;
   }
   q=(quat *)(((uchar *)cTable)+cTable->offset[cm->handle]);
   if(iframe+1==mi->num_frames)
      memcpy(data,&q[iframe],sizeof(quat));
   else
      quat_slerp(data,&q[iframe],&q[iframe+1],frame-iframe);

   if (g_mot_quat_debug &&
       ((quat_mag(data) > 1.001)||(quat_mag(data) < 0.999)))
       CriticalMsg3("MotCompRotCallback: Bad quaternion in joint %i, motion %s, frame %g", 
         cm->joint_id, g_pMotionSet->GetName(mi->mot_num), frame);


   if(lockedHere)
   {
      MotDmngeUnlock(mi->mot_num);
   }
#ifdef E398_HACK
   if(g_DoRootHack && g_HackButtJointID==cm->joint_id)
   {
      quat qtmp,foo;
      mxs_matrix mat;
 
      MotRootRotCallback(NULL,mi,frame,&qtmp);
      quat_to_matrix(&mat,&qtmp);
      mx_transeq_mat(&mat);
      quat_from_matrix(&qtmp,&mat);
      // multiply data by inverse of qtmp
      quat_copy(&foo,data);
      quat_mul(data,&foo,&qtmp);
   }
#endif
}

///////////////////////////////////////////////////

void MotRootRotCallback(multiped *mp, mps_motion_info *mi,float frame, quat *data)
{
   if(mp && mp->app_ptr && ((cCreature *)mp->app_ptr)->IsBallistic())
   {
//      quat_identity(data);  // mo-cap doesn't determine root orientation
      Position *pPos=ObjPosGet(((cCreature *)mp->app_ptr)->GetObjID());
      if(pPos)
      {
         mxs_matrix mat; 

         mx_ang2mat(&mat,&pPos->fac);
         quat_from_matrix(data,&mat);
      } else
      {
         quat_identity(data);
      }
   } else
   {
      mxs_ang totRot=g_pMotionSet->GetEndHeading(mi->mot_num);
      quat q,qs;
      mxs_vector zaxis;
      float frac; 
   
      mx_unit_vec(&zaxis,2);
      if(!totRot)
      {
         quat_identity(data);   
         return;
      } 
   //   quat_create(&q,&zaxis,totRot);
      mxs_matrix mat;
      mx_mk_rot_z_mat(&mat,totRot);
      quat_from_matrix(&q,&mat);

      // slerp rotation by how far through motion you are.
      quat_identity(&qs);   
      frac=frame/mi->num_frames;
      quat_slerp(data,&qs,&q,frac);
   }
}

///////////////////////////////////////////////////

void MotFilterCallback(multiped *mp, mps_motion_info *mi,quat *rot, mxs_vector *xlat)
{
   if(mp->app_ptr)
      ((cCreature*)mp->app_ptr)->FilterMotionFrameData(mi,rot,xlat);
}

