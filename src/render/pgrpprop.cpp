/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/render/pgrpprop.cpp,v 1.27 2000/02/20 11:57:19 adurant Exp $
//
//  particle group property
//

//#define PROFILE_ON

#include <comtools.h>
#include <appagg.h>
#include <autolink.h>
#include <config.h>
#include <timings.h>
#include <wrtype.h>
#include <dynarray.h>

#include <property.h>
#include <propert_.h>
#include <prophash.h>
#include <dataops.h>
#include <dataops_.h>
#include <bintrait.h>
#include <traitman.h>
#include <contain.h>

#include <iobjsys.h>
#include <osysbase.h>
#include <objnotif.h>
#include <objsys.h>
#include <objpos.h>
#include <posprop.h>
#include <dmgmodel.h>

#include <traitman.h>
#include <trait.h>
#include <traitbas.h>

#include <relation.h>
#include <linkbase.h>
#include <lnkquery.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <rendprop.h>
#include <creature.h>
#include <crjoint.h>
#include <pgroup.h>
#include <pgrpprop.h>
#include <objedit.h>
#include <physapi.h>

#include <simtime.h>
#include <mprintf.h>

// Must be last header
#include <dbmem.h>

static IParticleGroupProperty *particle_group_prop = NULL;
static IPGroupLaunchInfoProperty *particle_launch_info_prop = NULL;

static ITrait *attach_inv_trait = NULL;
static IRelation *particle_attach_rel=NULL;
static IRelation *attach_inv_rel = NULL; 

////////////////////////////////////////////////////////////////////////////////

struct sOpsParticleGroup : public ParticleGroup
{
   const sOpsParticleGroup& operator = (const sOpsParticleGroup& that)
   {
      if (this == &that)
         return *this;

      memcpy(this, &that, sizeof(*this));

      pl.location = NULL;
      pl.velocity = NULL;
      pl.time_info = NULL;

      return *this;
   }

   sOpsParticleGroup(const sOpsParticleGroup& that)
   {
      operator =(that);
   }

   sOpsParticleGroup()
   {
      pl.location = NULL;
      pl.velocity = NULL;
      pl.time_info = NULL;

   }
};

class cParticleGroupOps : public cClassDataOps<sOpsParticleGroup> 
{ 
public:
   STDMETHOD_(int, Version)() { return 1006; };
};

class cParticleGroupStore : public cHashPropertyStore<cParticleGroupOps>
{ 
   STDMETHOD(ReadObj) (ObjID obj, IDataOpsFile* file, int version)
   {
      sDatum dat = Create(obj);

      if (!dat.value)
         dat.value = mOps.New();

      ParticleGroup *pg = (ParticleGroup *)dat.value;

      file->Read(&pg->pc, sizeof(ParticleClass));
      file->Read(&pg->obj, sizeof(ObjID));

      file->Read(&pg->render_type, sizeof(enum ParticleRenderTypes));
      file->Read(&pg->motion_type, sizeof(enum ParticleGroupMotionType));
      file->Read(&pg->anim_type, sizeof(enum ParticleAnimationType));
      file->Read(&pg->d, sizeof(int));
      file->Read(&pg->e, sizeof(int));

      file->Read(&pg->n, sizeof(int));
      file->Read(&pg->pl, sizeof(ParticleList));
      file->Read(&pg->velocity, sizeof(mxs_vector));
      file->Read(&pg->gravity, sizeof(mxs_vector));

      file->Read(&pg->cr, sizeof(uchar));
      file->Read(&pg->cg, sizeof(uchar));
      file->Read(&pg->cb, sizeof(uchar));
      file->Read(&pg->ca, sizeof(uchar));
      if (version < 1003) pg->cg = pg->cb = 0;

      file->Read(&pg->always_simulate, sizeof(bool));
      file->Read(&pg->always_simulate_group, sizeof(bool));
      file->Read(&pg->cell_sort, sizeof(bool));
      file->Read(&pg->zsort, sizeof(bool));
   
      file->Read(&pg->terrain_collide, sizeof(bool));
      file->Read(&pg->accelerate_cell, sizeof(bool));
      file->Read(&pg->ignore_attach_refs, sizeof(bool));
      file->Read(&pg->pad2, sizeof(bool));

      if (version < 1000)
      {
         ParticleLaunchInfo *launch_info;

         if (!particle_launch_info_prop->Get(obj, &launch_info))
         {
            particle_launch_info_prop->Create(obj);
            particle_launch_info_prop->Get(obj, &launch_info);
         }

         file->Read(launch_info->start_loc_bbox, sizeof(fix) * 6);
         file->Read(launch_info->start_vel_bbox, sizeof(fix) * 6);
         file->Read(launch_info->time_range, sizeof(fix) * 2);

         launch_info->type = 0;

         for (int i=0; i<3; i++)
         {
            launch_info->loc_min.el[i] = ((float)(launch_info->start_loc_bbox[0][i])) / 65536.0;
            launch_info->loc_max.el[i] = ((float)(launch_info->start_loc_bbox[1][i])) / 65536.0;

            launch_info->vel_min.el[i] = ((float)(launch_info->start_vel_bbox[0][i])) / 65536.0;
            launch_info->vel_max.el[i] = ((float)(launch_info->start_vel_bbox[1][i])) / 65536.0;
         }
         
         launch_info->min_time = ((float)(launch_info->time_range[0])) / 65536.0;
         launch_info->max_time = ((float)(launch_info->time_range[1])) / 65536.0;

         launch_info->min_radius = 0;
         launch_info->max_radius = 0;

         launch_info->vel_unrotated = pg->ignore_attach_refs;
         launch_info->loc_unrotated = pg->pad2;

         particle_launch_info_prop->Set(obj, launch_info);
      }
      else
      {
         // read in launch pointer
         ParticleLaunchInfo *dummy;
         file->Read(&dummy, sizeof(ParticleLaunchInfo *));
      }

      if (version < 1005)
         pg->ignore_attach_refs = FALSE;

      file->Read(&pg->spin, sizeof(mxs_vector));
      file->Read(&pg->pulse_period, sizeof(int));
      file->Read(&pg->pulse_percentage, sizeof(float));
      file->Read(&pg->fixed_scale, sizeof(float));

      file->Read(&pg->pre_launch, sizeof(bool));
      file->Read(&pg->spin_group, sizeof(bool));
      file->Read(&pg->tiny_alpha, sizeof(bool));
      file->Read(&pg->tiny_dropout, sizeof(bool));

      file->Read(&pg->shared_list, sizeof(bool));
      file->Read(&pg->worldspace, sizeof(bool));
      file->Read(&pg->launching, sizeof(bool));

      file->Read(&pg->active, sizeof(bool));
      if (version < 1001)
         pg->active = TRUE;

      file->Read(&pg->ms_offset, sizeof(int));
      file->Read(&pg->size, sizeof(float));
      file->Read(&pg->reserved, sizeof(int));
      file->Read(&pg->reserved2, sizeof(int));

      file->Read(&pg->prev_loc, sizeof(mxs_vector));
      file->Read(&pg->scale_vel, sizeof(float));

      file->Read(&pg->render_datum, sizeof(int));
      file->Read(&pg->bmin, sizeof(mxs_vector));
      file->Read(&pg->bmax, sizeof(mxs_vector));
      file->Read(&pg->radius, sizeof(float));

      file->Read(&pg->cur_scale, sizeof(float));
      file->Read(&pg->points, sizeof(r3s_point *));
      file->Read(&pg->sort_lists, sizeof(int **));

      file->Read(&pg->locs_worldspace, sizeof(bool));
      file->Read(&pg->free_locs, sizeof(bool));
      file->Read(&pg->seen, sizeof(bool));
      file->Read(&pg->need_flags, sizeof(unsigned char));

      file->Read(&pg->list_length, sizeof(int));
      file->Read(&pg->delete_count, sizeof(int));
      file->Read(&pg->next_launch, sizeof(fix));
      file->Read(&pg->launch_period, sizeof(fix));
      
      file->Read(&pg->modelname, sizeof(char) * 16);
      file->Read(&pg->modelnum, sizeof(int));
      file->Read(&pg->fade_time, sizeof(fix));
      
      file->Read(&pg->obj_rot_mat, sizeof(mxs_matrix));
      if (version >= 1002) {
         file->Read(&pg->last_sim_time, sizeof(int));
      } else {
         pg->last_sim_time = 0;
      }

      if (version >= 1004) {
         file->Read(&pg->attach_obj, sizeof(ObjID));
      } else {
         LinkID linkID = particle_attach_rel->GetSingleLink(obj, LINKOBJ_WILDCARD);

         if (linkID != 0)
         {
            sLink link;
            particle_attach_rel->Get(linkID, &link);
            pg->attach_obj = link.dest;
         }
         else
            pg->attach_obj = OBJ_NULL;
      }


	  if (version >= 1006)
		  file->Read(&pg->force_match_unrefs, sizeof(BOOL));
	  else
		  pg->force_match_unrefs = FALSE;  //safe.

      // clear out any stale, saved pointers and state
      pg->pl.location = NULL;
      pg->pl.velocity = NULL;
      pg->pl.time_info = NULL;
      pg->pl.cell = NULL;
      pg->free_locs = FALSE;
      pg->sort_lists = NULL;

      Set(obj,dat);

      return S_OK;
   };
};

class cParticleGroupProperty : public cSpecificProperty<IParticleGroupProperty,
   &IID_IParticleGroupProperty, ParticleGroup *, cParticleGroupStore>
{
  typedef cSpecificProperty<IParticleGroupProperty, &IID_IParticleGroupProperty,
       ParticleGroup*, cParticleGroupStore> cParent; 

public:
   cParticleGroupProperty(const sPropertyDesc* desc) 
      : cParent(desc)
   {
   }

   // use the standard DescribeType method. 
   STANDARD_DESCRIBE_TYPE(ParticleGroup); 

   STDMETHOD_(void, Notify) (THIS_ ePropertyNotifyMsg msg, PropNotifyData data)
   {
      // make sure we delete all groups on reset
      if (NOTIFY_MSG(msg) == kObjNotifyReset)
      {
         sPropertyObjIter iter;
         ObjID obj;

         IterStart(&iter);
         while (IterNext(&iter, &obj))
         {
            if (OBJ_IS_CONCRETE(obj))
               particle_group_delete(obj);
         }
         IterStop(&iter);
      }

      cParent::Notify(msg, data);
   };

   void OnListenMsg(ePropertyListenMsg msg, ObjID obj,
                      uPropListenerValue val)
   {
      cParent::OnListenMsg(msg, obj, val);

      if (msg & (kListenPropModify | kListenPropSet))
      {
         ParticleGroup *pg = ObjGetParticleGroup(obj);

         if (pg)
            pg->launch = ObjGetParticleGroupLaunchInfo(obj);

         if (OBJ_IS_CONCRETE(obj))
            particle_group_update(obj);
      }

      if (msg & kListenPropUnset)
         particle_group_delete(obj);
   };
};

///////////////////////////////////

static sPropertyConstraint particle_group_constraints[] = 
{
   { kPropertyRequires, PROP_PARTICLE_LAUNCH_INFO_NAME },
   { kPropertyNullConstraint }
};

static sPropertyDesc particle_group_desc = 
{ 
   PROP_PARTICLE_GROUP_NAME, 
   kPropertyInstantiate | kPropertyNoInherit,
   particle_group_constraints,
   0, 0,
   { "SFX", "Particles" }, 
};

////////////////////////////////////////////////////////////////////////////////

static const char *prend_type_strings[] =
{
   "Single-colored pixels",
   "Multi-colored pixels",
   "Single-colored squares",
   "Multi-colored squares",
   "Single-colored disk",
   "Scaled bitmap",
};

const char *motion_type_strings[] =
{
   "Immobile",
   "Move with velocity",
   "Move with vel&gravity",
   "Trailing object",
   "Attached to object",
   "Fire and forget",
};

const char *anim_type_strings[] =
{
   "Launched one-shot",
   "Launched continually",
   "Rotating hollow sphere",
   "Rotating sphere",
   "Rotating hollow cube",
   "Rotating cube",
   "Rotating disk",
   "Rotating circle",
   "Rotating pyramid",
   "Rotating line",
};

// macro to make the sdesc field descriptors fit on one line
#define FL(a)  FieldLocation(ParticleGroup, a)

static sFieldDesc particle_group_fields [] =
{
   { "Active", kFieldTypeBool, FL(active), },

   { "Particle Render Type",  kFieldTypeEnum, FL(render_type), 0,
      0, PRT_NUM_USER_EDITTABLE, PRT_NUM_USER_EDITTABLE, prend_type_strings },
   { "Particle Animation",    kFieldTypeEnum, FL(anim_type), 0,
      0, PAT_NUM_USER_EDITTABLE, PAT_NUM_USER_EDITTABLE, anim_type_strings },
   { "Particle Group Motion", kFieldTypeEnum, FL(motion_type), 0,
      0, PGMT_NUM_USER_EDITTABLE, PGMT_NUM_USER_EDITTABLE,motion_type_strings},

   { "number of particles",      kFieldTypeInt,    FL(n)        },
   { "size of particle",         kFieldTypeFloat,  FL(size)     },
   { "bitmap name",              kFieldTypeString, FL(modelname) },
   { "velocity",                 kFieldTypeVector, FL(velocity) },
   { "gravity vector",           kFieldTypeVector, FL(gravity)  },
   { "color (palettized)",       kFieldTypeInt,    FL(cr), kFieldFlagUnsigned },
// { "blue",                     kFieldTypeInt,    FL(cg), kFieldFlagUnsigned },
// { "green",                    kFieldTypeInt,    FL(cb), kFieldFlagUnsigned },
   { "2nd color",                kFieldTypeInt,    FL(cg), kFieldFlagUnsigned },
   { "3rd color",                kFieldTypeInt,    FL(cb), kFieldFlagUnsigned },
   { "alpha",                    kFieldTypeInt,    FL(ca), kFieldFlagUnsigned },
   { "fixed-group radius",       kFieldTypeFloat,  FL(fixed_scale) },
   { "spin / pulse group",       kFieldTypeBool,   FL(spin_group)  },
   { "spin speed",               kFieldTypeVector, FL(spin) },
   { "pulse magnitude",          kFieldTypeFloat,  FL(pulse_percentage) },
   { "pulse cycle time ms",      kFieldTypeInt,    FL(pulse_period) },
#if 0
   { "launch velocity min", kFieldTypeFixVec,FL(launch.start_vel_bbox[0]) },
   { "launch velocity max", kFieldTypeFixVec,FL(launch.start_vel_bbox[1]) },
   { "launch location min", kFieldTypeFixVec,FL(launch.start_loc_bbox[0]) },
   { "launch location max", kFieldTypeFixVec,FL(launch.start_loc_bbox[1]) },
   { "launch vel unrotated",     kFieldTypeBool,   FL(launch_vel_unrotated) },
   { "launch loc unrotated",     kFieldTypeBool,   FL(launch_loc_unrotated) },
   { "min particle time",        kFieldTypeFix,    FL(launch.time_range[0]) },
   { "max particle time",        kFieldTypeFix,    FL(launch.time_range[1]) },
#endif
   { "particle fade time",       kFieldTypeFix,    FL(fade_time) },
   { "launch period",            kFieldTypeFix,    FL(launch_period) },
   { "animation offset ms",      kFieldTypeInt,    FL(ms_offset) },
   { "Group-scale velocity",     kFieldTypeFloat,  FL(scale_vel) },

   { "always simulate (slow)",     kFieldTypeBool, FL(always_simulate) },
   { "always simulate group",      kFieldTypeBool, FL(always_simulate_group) },
// { "terrain sort (slow)",        kFieldTypeBool, FL(cell_sort) },
   { "sort particles (slow)",      kFieldTypeBool, FL(zsort), kFieldFlagNotEdit },
// { "collide with terrain (slow)",kFieldTypeBool, FL(terrain_collide) },
// { "precompute static emitter",  kFieldTypeBool, FL(accelerate_cell) },
   { "particles start launched",   kFieldTypeBool, FL(pre_launch) },
   { "alpha subpixel particles",   kFieldTypeBool, FL(tiny_alpha) },
   { "skip subpixel particles",    kFieldTypeBool, FL(tiny_dropout) },
   { "ignore attachment refs",     kFieldTypeBool, FL(ignore_attach_refs) },
   { "force matching unrefs",      kFieldTypeBool, FL(force_match_unrefs) },
};

#undef FL

static sStructDesc particle_group_struct
   = StructDescBuild(ParticleGroup, kStructFlagNone, particle_group_fields);

////////////////////////////////////////////////////////////////////////////////

class cPGroupLaunchInfoOps: public cClassDataOps<ParticleLaunchInfo>
{
public:
   STDMETHOD_(int, Version)() { return 1000; };

   STDMETHOD(Read)(sDatum *pdat, IDataOpsFile *file, int verison);
};

STDMETHODIMP cPGroupLaunchInfoOps::Read(sDatum *pdat, IDataOpsFile *file, int verison)
{
   if (!pdat->value)
      pdat->value = New();

   ParticleLaunchInfo *launch_info = (ParticleLaunchInfo *)pdat->value;

   file->Read(&launch_info->type, sizeof(int));
   
   file->Read(&launch_info->loc_min, sizeof(mxs_vector));
   file->Read(&launch_info->loc_max, sizeof(mxs_vector));

   file->Read(&launch_info->vel_min, sizeof(mxs_vector));
   file->Read(&launch_info->vel_max, sizeof(mxs_vector));

   file->Read(&launch_info->min_radius, sizeof(mxs_real));
   file->Read(&launch_info->max_radius, sizeof(mxs_real));

   file->Read(&launch_info->min_time, sizeof(mxs_real));
   file->Read(&launch_info->max_time, sizeof(mxs_real));

   file->Read(&launch_info->vel_unrotated, sizeof(bool));
   file->Read(&launch_info->loc_unrotated, sizeof(bool));

   file->Read(launch_info->start_loc_bbox, sizeof(fix) * 2 * 3);
   file->Read(launch_info->start_vel_bbox, sizeof(fix) * 2 * 3);
   file->Read(launch_info->radius, sizeof(fix) * 2);
   file->Read(launch_info->time_range, sizeof(fix) * 2);

   return S_OK;
};

class cPGroupLaunchInfoStore: public cHashPropertyStore<cPGroupLaunchInfoOps>
{ };

typedef cSpecificProperty<IPGroupLaunchInfoProperty, 
   &IID_IPGroupLaunchInfoProperty, ParticleLaunchInfo *, 
   cPGroupLaunchInfoStore> cPGroupLaunchInfoPropertyBase;

class cPGroupLaunchInfoProperty : public cPGroupLaunchInfoPropertyBase
{
public:
   cPGroupLaunchInfoProperty(const sPropertyDesc *desc)
      : cPGroupLaunchInfoPropertyBase(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(ParticleLaunchInfo);

   #define float_to_fix(x)  ((fix)(x * 65536))

   void OnListenMsg(ePropertyListenMsg msg, ObjID obj, uPropListenerValue val)
   {
      if (msg & (kListenPropModify | kListenPropSet))
      {
         ParticleLaunchInfo *launch_info = (ParticleLaunchInfo *)val.ptrval;

         // Don't allow times to be zero
         if (launch_info->min_time == 0)
            launch_info->min_time = 0.001;
         if (launch_info->max_time == 0)
            launch_info->max_time = 0.001;

         // Convert our vectors to fixed
         for (int i=0; i<3; i++)
         {
            launch_info->start_loc_bbox[0][i] = float_to_fix(launch_info->loc_min.el[i]);
            launch_info->start_loc_bbox[1][i] = float_to_fix(launch_info->loc_max.el[i]);

            launch_info->start_vel_bbox[0][i] = float_to_fix(launch_info->vel_min.el[i]);
            launch_info->start_vel_bbox[1][i] = float_to_fix(launch_info->vel_max.el[i]);
         }

         launch_info->time_range[0] = float_to_fix(launch_info->min_time);
         launch_info->time_range[1] = float_to_fix(launch_info->max_time);

         launch_info->radius[0] = float_to_fix(launch_info->min_radius);
         launch_info->radius[1] = float_to_fix(launch_info->max_radius);

         // Point our particle group at us, just in case the pointer has gone stale
         ParticleGroup *pgroup;

         if (particle_group_prop && (particle_group_prop->Get(obj, &pgroup)))
         {
            pgroup->launch = launch_info;
            //            particle_group_prop->Set(obj, pgroup);
         }
      }

      cPGroupLaunchInfoPropertyBase::OnListenMsg(msg, obj, val);
   }
};

////////////////////////////////////////

static sPropertyDesc particle_launch_info_desc = 
{
   PROP_PARTICLE_LAUNCH_INFO_NAME,
   kPropertyInstantiate | kPropertyNoInherit,
   NULL, 0, 0,
   { "SFX", "Particle Launch Info" },
   kPropertyChangeLocally,
};

////////////////////////////////////////////////////////////////////////////////

const char *launch_type[] =
{
   "Bounding Box",
   "Sphere",
   "Cylinder",
};

static sFieldDesc particle_launch_info_fields[] = 
{
   { "Launch Type", kFieldTypeEnum, FieldLocation(ParticleLaunchInfo, type), kFieldFlagNone, 0, 3, 3, launch_type },
   
   { "Box Min", kFieldTypeVector, FieldLocation(ParticleLaunchInfo, loc_min), kFieldFlagNone },
   { "Box Max", kFieldTypeVector, FieldLocation(ParticleLaunchInfo, loc_max), kFieldFlagNone },

   { "Min Radius", kFieldTypeFloat, FieldLocation(ParticleLaunchInfo, min_radius), kFieldFlagNone },
   { "Max Radius", kFieldTypeFloat, FieldLocation(ParticleLaunchInfo, max_radius), kFieldFlagNone },

   { "Loc unrotated?", kFieldTypeBool, FieldLocation(ParticleLaunchInfo, loc_unrotated), kFieldFlagNone },

   { "Velocity Min", kFieldTypeVector, FieldLocation(ParticleLaunchInfo, vel_min), kFieldFlagNone },
   { "Velocity Max", kFieldTypeVector, FieldLocation(ParticleLaunchInfo, vel_max), kFieldFlagNone },

   { "Vel unrotated?", kFieldTypeBool, FieldLocation(ParticleLaunchInfo, vel_unrotated), kFieldFlagNone },

   { "Min time", kFieldTypeFloat, FieldLocation(ParticleLaunchInfo, min_time), kFieldFlagNone },
   { "Max time", kFieldTypeFloat, FieldLocation(ParticleLaunchInfo, max_time), kFieldFlagNone },
};

static sStructDesc particle_launch_info_struct =        
          StructDescBuild(ParticleLaunchInfo, kStructFlagNone, particle_launch_info_fields);

////////////////////////////////////////////////////////////////////////////////

const char *attach_type[] =
{
   "Object", 
   "Vhot",
   "Joint",
   "Submodel"
};

static sRelationDesc attach_desc =
{
   LINK_PARTICLE_ATTACHMENT_NAME,
   0,
};

static sRelationDataDesc attach_ddesc = LINK_DATA_DESC_FLAGS(sParticleAttachLinkData, kRelationDataAutoCreate);

static sFieldDesc particle_attach_data_fields[] =
{
   {"Type",     kFieldTypeEnum, FieldLocation(sParticleAttachLinkData, type),     kFieldFlagNone, 0, 4, 4, attach_type},
   {"vhot #",   kFieldTypeInt,  FieldLocation(sParticleAttachLinkData, vhot),     kFieldFlagNone},
   {"joint",    kFieldTypeEnum, FieldLocation(sParticleAttachLinkData, joint),    kFieldFlagNone, 0, 22, 22, g_pJointNames},
   {"submod #", kFieldTypeInt,  FieldLocation(sParticleAttachLinkData, submodel), kFieldFlagNone}
};

static sStructDesc particle_attach_sdesc = StructDescBuild(sParticleAttachLinkData, kStructFlagNone, particle_attach_data_fields);

#define ATTACH_QUERY_CASES  (kQCaseSetSourceKnown|kQCaseSetDestKnown)

////////////////////////////////////////////////////////////////////////////////

static BOOL has_linker(ObjID obj, void *data)
{
   ILinkQuery *query = ((IRelation *) data)->Query(LINKOBJ_WILDCARD, obj);
   BOOL result = !(query->Done());
   SafeRelease(query);
   return result;
}

void object_listen_func(ObjID obj, eObjNotifyMsg msg, void* data)
{
   if (msg != kObjNotifyCreate)
      return;
   if (!OBJ_IS_CONCRETE(obj))
      return;

   // instantiate all the objects dangling off this archetype
   ILinkQuery *query = QueryInheritedLinksSingle(attach_inv_trait, attach_inv_rel, obj, LINKOBJ_WILDCARD);
   ObjID result = OBJ_NULL;

   for(; !query->Done(); query->Next()) {
      sLink link;
      query->Link(&link);
      // instantiate a copy of link.src
      // 
      if (OBJ_IS_CONCRETE(link.dest)) {
         Warning(("ParticleGroup attached to archetype is concrete!\n"));
      } else {
         ObjID newobj = BeginObjectCreate(link.dest, kObjectConcrete);
         particle_attach_rel->AddFull(newobj, obj, query->Data());
         EndObjectCreate(newobj);
      }
   }
   SafeRelease(query);
}

static void LGAPI pgroup_listener(sPropertyListenMsg *msg, PropListenerData data)
{
   cAutoLinkQuery query(particle_attach_rel, msg->obj, LINKOBJ_WILDCARD);

   for (; !query->Done(); query->Next())
   {
      ParticleGroup *pg = ObjGetParticleGroup(msg->obj);

      if (pg)
         pg->attach_obj = query.GetDest();
   }
}

static void LGAPI link_listener(sRelationListenMsg *msg, RelationListenerData dummy)
{
   if (msg->type & kListenLinkDeath)
   {
      if (OBJ_IS_CONCRETE(msg->link.source))
      {
         AutoAppIPtr(ObjectSystem);
         ParticleGroupShutdown(msg->link.source);
         // we may be deleting this link because the source obj is being deleted...
         if (pObjectSystem->Exists(msg->link.source))
            pObjectSystem->Destroy(msg->link.source);
      }

      ParticleGroup *pg = ObjGetParticleGroup(msg->link.source);
      if (pg)
         pg->attach_obj = OBJ_NULL;
   }

   if (msg->type & kListenLinkBirth)
   {
      if (OBJ_IS_CONCRETE(msg->link.source) && OBJ_IS_CONCRETE(msg->link.dest))
      {
         ObjPosUpdate(msg->link.source, &ObjPosGet(msg->link.dest)->loc.vec, &ObjPosGet(msg->link.dest)->fac);
         // If somebody is trying to attach us to a joint of something that isn't a creature, then slam the
         // attachment type to object.
         sParticleAttachLinkData* pData = (sParticleAttachLinkData *)(particle_attach_rel->GetData(msg->id));
         if ((pData->type == kPAT_joint) && !CreatureFromObj(msg->link.dest))
            pData->type = kPAT_object;
      }
   }

   if (msg->type & (kListenLinkBirth | kListenLinkModify))
   {
      ParticleGroup *pg = ObjGetParticleGroup(msg->link.source);
      if (pg)
         pg->attach_obj = msg->link.dest;
   }
}

static sObjListenerDesc attach_listen_desc =
{
   object_listen_func, NULL
};

#ifdef EDITOR
void ParticleGroupRefreshLinks()
{
   // Iterate over all concrete objects, checking if they have particle-
   // attachement links up the hierarchy

   AutoAppIPtr_(TraitManager, pTraitMan);
   AutoAppIPtr_(ContainSys, pContainSys);
   ObjID objID;

   int num_updated_links = 0;
   int num_created_links = 0;

   mprintf("Refreshing Particle Attachment links...\n");

   for (objID = 1; objID < gMaxObjID; objID++)
   {
      ILinkQuery *arch_query = QueryInheritedLinksSingle(attach_inv_trait, attach_inv_rel, objID, LINKOBJ_WILDCARD);

      BOOL has_links = FALSE;

      for (; !arch_query->Done(); arch_query->Next())
      {
         has_links = TRUE;

         sLink arch_link;
         arch_query->Link(&arch_link);

         // Skip concretes in our archetype query; they're in our concrete query
         if (OBJ_IS_CONCRETE(arch_link.dest))
            continue;

         ILinkQuery *conc_query = particle_attach_rel->Query(LINKOBJ_WILDCARD, objID);

         // See we already have an instantiation of our current archetype link
         for (; !conc_query->Done(); conc_query->Next())
         {
            sLink conc_link;
            conc_query->Link(&conc_link);

            ObjID source_arch = pTraitMan->GetArchetype(conc_link.source);

            if ((source_arch == arch_link.dest) && 
                (memcmp(conc_query->Data(), arch_query->Data(), sizeof(sParticleAttachLinkData)) == 0))
            {
               // We've found an obj-obj match, so blow away link (it will get rebiult)
               particle_attach_rel->Remove(conc_query->ID());

               num_updated_links++;
            }
         }

         SafeRelease(conc_query);

         // Create the particle group object and link 
         #ifndef SHIP
         if (config_is_defined("ParticleRefreshSpew"))
            mprintf("  Creating particle attachement of %s on %s\n", ObjWarnName(arch_link.dest), ObjWarnName(objID));
         #endif

         num_created_links++;

         ObjID newobj = BeginObjectCreate(arch_link.dest, kObjectConcrete);
         particle_attach_rel->AddFull(newobj, objID, arch_query->Data());
         EndObjectCreate(newobj);

         // Deactivate it if its attachment is contained
         ParticleGroup *pg = ObjGetParticleGroup(newobj);
         if (pg->attach_obj && pContainSys->IsHeld(LINKOBJ_WILDCARD, pg->attach_obj) != ECONTAIN_NULL)
            ObjParticleSetActive(newobj, FALSE);
      }

      SafeRelease(arch_query);

      if (has_links)
         ObjPosUpdate(objID, &ObjPosGet(objID)->loc.vec, &ObjPosGet(objID)->fac);
   }

   mprintf(" Updated links: %d\n", num_updated_links);
   mprintf(" Created links: %d\n", num_created_links);
}        
#endif
////////////////////////////////////////////////////////////////////////////////

EXTERN void FindVhotLocation(mxs_vector *offset, ObjID obj, int vhot);

static BOOL getAttachmentLocation(Location *loc, ParticleGroup *pg)
{
   if (pg == NULL)
   {
      mx_zero_vec(&loc->vec);
      return FALSE;
   }

   sParticleAttachLinkData *data;
   ObjID obj2;

   ObjGetParticleAttachment(pg->obj, &obj2, &data);

   if (obj2 == OBJ_NULL) return FALSE;

   switch (data->type)
   {
      case kPAT_object:
      {
         ObjPos *pos2 = ObjPosGet(obj2);
         if (pos2 == NULL) return FALSE;
         *loc = pos2->loc;
         break;
      }

      case kPAT_vhot:
      {
         loc->vec.x = loc->vec.y = loc->vec.z = 0;
         FindVhotLocation(&loc->vec, obj2, data->vhot);
         UpdateChangedLocation(loc);
         break;
      }

      case kPAT_joint:
      {
         GetCreatureJointPosC(obj2, data->joint, &loc->vec);
         UpdateChangedLocation(loc);
         break;
      }

      case kPAT_submodel:
      {
         PhysGetSubModLocation(obj2, data->submodel, &loc->vec);
         UpdateChangedLocation(loc);
         break;
      }

      default:
      {
         Warning(("Unkown particle group attachment type: %d\n", data->type));
         break;
      }
   }
   return TRUE;
}

static void PosPropListener(ObjID obj, const ObjPos *pos, void *data)
{
   if (OBJ_IS_ABSTRACT(obj))
      return;

   // see if this object has attached particle groups
   cAutoLinkQuery query(particle_attach_rel, LINKOBJ_WILDCARD, obj);
   Location loc;
   ObjID    source;

   for (; !query->Done(); query->Next())
   {
      source = query.GetSource();

      if (getAttachmentLocation(&loc, ObjGetParticleGroup(source)))
         ObjPosUpdate(source, &loc.vec, &pos->fac);
   }
}

////////////////////////////////////////////////////////////////////////////////

void ParticleGroupPropInit(void)
{
   ITraitManager *trm = AppGetObj(ITraitManager);
   IObjectSystem *obs = AppGetObj(IObjectSystem);
   AutoAppIPtr_(PropertyManager, pPropMan);
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Register(&particle_group_struct);
   pTools->Register(&particle_launch_info_struct);
   pTools->Register(&particle_attach_sdesc);

   particle_group_prop = new cParticleGroupProperty(&particle_group_desc); 
   particle_launch_info_prop = new cPGroupLaunchInfoProperty(&particle_launch_info_desc);

   particle_attach_rel = CreateStandardRelation(&attach_desc,
                              &attach_ddesc, ATTACH_QUERY_CASES);

   attach_inv_rel = particle_attach_rel->Inverse(); 
   attach_inv_trait = MakeTraitFromRelation(attach_inv_rel);

   particle_group_prop->Listen(kListenPropSet | kListenPropModify, pgroup_listener, NULL);
   
   particle_attach_rel->Listen(kListenLinkDeath | kListenLinkBirth | kListenLinkModify,
           link_listener, NULL);
   obs->Listen(&attach_listen_desc);
   SafeRelease(trm);
   SafeRelease(obs);

   ObjPosListen(PosPropListener, NULL);
}

void ParticleGroupPropTerm(void)
{
   SafeRelease(particle_group_prop);
   SafeRelease(particle_attach_rel);
   SafeRelease(attach_inv_trait); 
   SafeRelease(attach_inv_rel); 
}

ParticleGroup *ObjGetParticleGroup(ObjID obj)
{
   ParticleGroup *res;
   if (particle_group_prop->Get(obj, &res))
      return res;
   return NULL;
}

ParticleLaunchInfo *ObjGetParticleGroupLaunchInfo(ObjID obj)
{
   ParticleLaunchInfo *res;
   if (particle_launch_info_prop->Get(obj, &res))
      return res;
   return NULL;
}

BOOL ObjIsParticleGroup(ObjID obj)
{
   return particle_group_prop->IsRelevant(obj);
}

BOOL ObjParticleIsActive(ObjID obj)
{
   ParticleGroup *pg;

   return (particle_group_prop->Get(obj, &pg) && pg->active);
}

void ObjParticleSetActive(ObjID obj, BOOL active)
{
   ParticleGroup *pg;

   if (particle_group_prop->Get(obj, &pg))
   {
      pg->active = active;
      particle_group_prop->Set(obj, pg);
   }
   else
      Warning(("obj %s is not a particle group, can't set active.\n", ObjWarnName(obj)));
}

void ObjGetParticleAttachment(ObjID obj, ObjID *attach_obj, sParticleAttachLinkData **data)
{
   ILinkQuery *query = particle_attach_rel->Query(obj, LINKOBJ_WILDCARD);
   sParticleAttachLinkData *result_data = NULL;
   *attach_obj = OBJ_NULL;

   for(; !query->Done(); query->Next()) {
      sLink link;
      query->Link(&link);
      if (obj == link.source)
      {
         *attach_obj = link.dest;
         *data = (sParticleAttachLinkData *)query->Data();
         break;
      }
   }
   SafeRelease(query);
}

void ObjSetParticleGroup(ObjID obj, ParticleGroup *val)
{
   particle_group_prop->Set(obj,val); 
   return;
}

EXTERN void UpdateBitmapAnimations(int time);
EXTERN void pgroup_show_counts(void);

#define LAUNCHING(x)  \
               ((x) == PAT_LAUNCH_ONE_SHOT || (x) == PAT_LAUNCH_CONTINUOUS)


DECLARE_TIMER(PGRP_Total, Average);
EXTERN void ParticleGroupUpdate(void)
{
   AUTO_TIMER(PGRP_Total);

   AutoAppIPtr_(DamageModel, pDmgModel);
   AutoAppIPtr_(ObjectSystem, pObjSys);

   cDynArray<ObjID> deferredSlayList;

   sPropertyObjIter iter;
   ObjID obj;

   particle_group_prop->IterStart(&iter);
   while (particle_group_prop->IterNext(&iter, &obj)) {
      if (OBJ_IS_CONCRETE(obj)) {
         
         ParticleGroup *pg = ObjGetParticleGroup(obj);

         if (LAUNCHING(pg->anim_type) || pg->active) {
            if (ParticleGroupSim(obj)) {
               deferredSlayList.Append(obj);
               continue;
            }
         }

         BOOL force_unreffed = FALSE;

		 if (!pg->force_match_unrefs)
		 {
			 if (pg->attach_obj != OBJ_NULL)
				 if (!ObjHasRefs(pg->attach_obj))
				 {
					 if (ObjHasRefs(obj))
						 ObjSetHasRefs(obj, FALSE);
					 force_unreffed = TRUE;
				 }
		 }

         if (!pg->ignore_attach_refs)
         {
            if ((pg->motion_type == PGMT_OBJECT_TRAILING) ||
                (pg->motion_type == PGMT_OBJECT_ATTACHED))
            {
               if (pg->attach_obj != OBJ_NULL)
               {
                  if (!ObjHasRefs(pg->attach_obj))
                  {
                     if (ObjHasRefs(obj))
                        ObjSetHasRefs(obj, FALSE);
                     force_unreffed = TRUE;
                  }
               }
            }
         }

         if (!force_unreffed)
         {
            if (LAUNCHING(pg->anim_type)) {
               if (ObjHasRefs(obj)) {
                  if (!pg->active && pg->pl.n == 0)
                     ObjSetHasRefs(obj, FALSE);
               } else {
                  if (pg->active || pg->pl.n != 0)
                     ObjSetHasRefs(obj, TRUE);
               }
            } else {
               if (ObjHasRefs(obj)) {
                  if (!pg->active)
                     ObjSetHasRefs(obj, FALSE);
               } else {
                  if (pg->active)
                     ObjSetHasRefs(obj, TRUE);
               }
            }
         }
      }
   }

   // technically, we should call Set on the property since
   // we probably updated it, but unfortunately, if we did,
   // we'd get a modify message on the property, which would
   // make us think we'd editted it, because we're fools
   particle_group_prop->IterStop(&iter);

   pObjSys->Lock();
   for (int i=0; i<deferredSlayList.Size(); i++)
      pDmgModel->SlayObject(deferredSlayList[i], OBJ_NULL);
   pObjSys->Unlock();
   
   deferredSlayList.SetSize(0);

   // umm, this is a hack
   UpdateBitmapAnimations(GetSimFrameTime());

#ifndef SHIP
   pgroup_show_counts();
#endif
}

#ifndef SHIP
EXTERN void ParticleGroupCountSim(void)
{
   int n;
   sPropertyObjIter iter;
   ObjID obj;

   mprintf("ALWAYS SIM PARTICLE GROUP ANALYSIS\n");
   particle_group_prop->IterStart(&iter);
   while (particle_group_prop->IterNext(&iter, &obj)) {
      if (!OBJ_IS_CONCRETE(obj)) {
         ParticleGroup *pg = ObjGetParticleGroup(obj);
         if (pg->always_simulate)
            mprintf("ObjID %d is set to always sim %d particles\n", obj, pg->n);
      }
   }
   particle_group_prop->IterStop(&iter);

   n = 0;
   particle_group_prop->IterStart(&iter);
   while (particle_group_prop->IterNext(&iter, &obj)) {
      if (OBJ_IS_CONCRETE(obj)) {
         ParticleGroup *pg = ObjGetParticleGroup(obj);
         if (pg->attach_obj == OBJ_NULL && pg->always_simulate)
            mprintf("ObjID %d is set to always sim %d particles\n", obj, pg->n);
         if (pg->always_simulate)
            n += pg->n;
      }
   }
   particle_group_prop->IterStop(&iter);
   mprintf("Total of %d concrete always-sim particles\n", n);
}
#endif


EXTERN void ParticleGroupEnterMode(void)
{
   sPropertyObjIter iter;
   ObjID obj;

   ParticleGroupUpdateModeStart();
   particle_group_prop->IterStart(&iter);
   while (particle_group_prop->IterNext(&iter, &obj))
      if (OBJ_IS_CONCRETE(obj))
         ParticleGroupUpdateMode(obj);
   particle_group_prop->IterStop(&iter);
}

EXTERN void ParticleGroupExitMode(void)
{
   ParticleGroupUpdateModeFinish(); 
}

///////////////////////////////////////////////

EXTERN void ParticleGroupCreateFromArchetype(ObjID archetypeID, ObjID ownerID)
{
   ILinkQuery *pQuery = QueryInheritedLinksSingle(attach_inv_trait, attach_inv_rel, archetypeID, LINKOBJ_WILDCARD);

   while (!pQuery->Done())
   {
      sLink link;
      pQuery->Link(&link);
      // instantiate a copy of link.src
      if (OBJ_IS_CONCRETE(link.dest)) 
         Warning(("ParticleGroup attached to archetype is concrete!\n"));
      else 
      {
         AutoAppIPtr(ObjectSystem);
         ObjID newobj = pObjectSystem->BeginCreate(link.dest, kObjectConcrete);
         particle_attach_rel->AddFull(newobj, ownerID, pQuery->Data());
         pObjectSystem->EndCreate(newobj);
      }
      pQuery->Next();
   }
   SafeRelease(pQuery);

}

///////////////////////////////////////////////

EXTERN void ParticleGroupDestroyAttached(ObjID ownerID)
{
   ILinkQuery *pQuery = attach_inv_rel->Query(ownerID, LINKOBJ_WILDCARD);

   while (!pQuery->Done())
   {
      sLink link;
      pQuery->Link(&link);
      if (OBJ_IS_CONCRETE(link.dest)) 
      {
         AutoAppIPtr(ObjectSystem);
         pObjectSystem->Destroy(link.dest);
      }
      pQuery->Next();
   }
   SafeRelease(pQuery);

}
