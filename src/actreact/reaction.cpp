/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/reaction.cpp,v 1.11 2000/01/28 11:28:21 adurant Exp $
#include <appagg.h>
#include <reactio_.h>
#include <hshpptem.h>

// To deal with cutting off reactrons to proxies:
#include <netman.h>
#include <iobjnet.h>

// For "amplify" built-in
#include <stimbase.h>
#include <sdesc.h>
#include <sdesbase.h>

// for "weakpoint" temp
#include <chevkind.h>
#include <phcollev.h>
#include <matrix.h>
#include <objpos.h>
#include <physapi.h>
#include <phystyp2.h>
#include <phmod.h>
#include <phmods.h>
#include <weapon.h>
#include <dmgbase.h>
#include <phcore.h>

// for spew
#include <config.h>
#include <cfgdbg.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// cReactions Implementation
//

IMPLEMENT_AGGREGATION_SELF_DELETE(cReactions); 

static sRelativeConstraint Constraints[] = 
{
   { kNullConstraint}
};

//------------------------------------------------------------
// Construction/Deconstruction
//

cReactions::cReactions(IUnknown* pOuter)
{
   INIT_AGGREGATION_1(pOuter,IID_IReactions,this,kPriorityNormal,Constraints); 
}

cReactions::~cReactions()
{

}



//------------------------------------------------------------
// IReactions Methods
//

STDMETHODIMP_(ReactionID) cReactions::Add(sReactionDesc* desc, ReactionFunc func, tReactionFuncData data) 
{
   if (ByName.HasKey(desc->name))
   {
      Warning(("Duplicate reaction named %s\n",desc->name));
      return REACTION_NULL;
   }
   sReactionEntry entry;
   entry.desc = *desc;
   entry.func = func;
   entry.data = data;

   ReactionID id = ByID.Append(entry);
   if (id >= Names.Size())
      Names.SetSize(id+1); 
   Names[id] = desc->name; 
   ByName.Insert(Names[id],id);
   return id;
}

////////////////////////////////////////

STDMETHODIMP_(ReactionID) cReactions::GetReactionNamed(const char* name)
{
   ReactionID id;
   if (ByName.Lookup(name,&id))
      return id;
   else 
      return REACTION_NULL;
}

////////////////////////////////////////

STDMETHODIMP_(const sReactionDesc*) cReactions::DescribeReaction(ReactionID id) 
{
   if (id < 0 || id >= ByID.Size())
      return NULL;
   else return &ByID[id].desc; 
}

////////////////////////////////////////

STDMETHODIMP_(eReactionResult) cReactions::React(ReactionID id, sReactionEvent* event, const sReactionParam* param)
{
   if (id >= 0 && id < ByID.Size()) {
#ifdef NEW_NETWORK_ENABLED
      if (!(ByID[id].desc.flags & kReactionWorkOnProxy)) {
         // The reaction hasn't told us that it works on proxy objects. *Is*
         // this a proxy object?
         AutoAppIPtr(NetManager);
         if (pNetManager->Networking()) {
            AutoAppIPtr(ObjectNetworking);
            if (pObjectNetworking->ObjIsProxy(event->sensor_obj)) {
               // Yep, so don't do anything:
               return kReactionNormal;
            }
         }
      }
#endif
      return ByID[id].func(event,param,ByID[id].data);
   }
   else
   {
      Warning(("Firing nonexistent reaction %d\n",id));
      return kReactionNormal;
   }
      

}


//------------------------------------------------------------
// BUILT-IN REACTIONS
//

static eReactionResult LGAPI abort_func(sReactionEvent* , const sReactionParam* , tReactionFuncData )
{
   return kReactionAbort;
}

static sReactionDesc abort_desc = 
{
   "Abort",
   "Abort",
   NO_REACTION_PARAM,
   // Aborts want to abort even on proxies, so we cut off damage:
   kReactionWorkOnProxy
};


//----------------------------------------

struct sAmplifyParam
{
   tStimLevel slope;
   tStimLevel intercept;
};

static eReactionResult LGAPI amplify_func(sReactionEvent* ev, const sReactionParam* param, tReactionFuncData )
{
   sAmplifyParam* coeff = (sAmplifyParam*)&param->data;

   // Reference! 
   tStimLevel& level = ev->stim->intensity;

   level = level*coeff->slope + coeff->intercept;
   return kReactionMutate;
}

static sReactionDesc amplify_desc = 
{
   "Amplify",
   "Amplify",
   REACTION_PARAM_TYPE(sAmplifyParam),
   kReactionNoFlags
};

static sFieldDesc amplify_fields[] = 
{
   { "Multiply by", kFieldTypeFloat, FieldLocation(sAmplifyParam,slope) },
   { "Then add", kFieldTypeFloat, FieldLocation(sAmplifyParam,intercept) },
};

static sStructDesc amplify_sdesc = StructDescBuild(sAmplifyParam,kStructFlagNone,amplify_fields);


//****************************************
//****************************************
//****************************************
//****************************************

//prototype new "WeakPoint" reaction.

extern mxs_vector g_collision_location; //bring in the PHCORE hack
extern int g_CollisionObj1;

struct sWeakPointParam
{
   tStimLevel slope;
   tStimLevel intercept;

//These should stop being floats and become Coordtypes as appropriate
   mxs_real      xoffset;
   mxs_real      yoffset;
   mxs_real      zoffset;
   mxs_real      radius;
};

static eReactionResult LGAPI WeakPoint_func(sReactionEvent* ev, const sReactionParam* param, tReactionFuncData )
{
   //We're going to go up the chain looking for the collision event.
   eChainedEventKind collisionkind = kEventKindCollision;

   //get the relevant data from the receptron
   sWeakPointParam* coeff = (sWeakPointParam*)&param->data;

   // Reference! 
   tStimLevel& level = ev->stim->intensity;

   //grab the collision event and cast it to PhysClsn (which it is)
   const sChainedEvent* clsnev = ev->stim->Find(collisionkind);
   const sPhysClsnEvent* realclsnev = (const sPhysClsnEvent*)clsnev;

   //so, what exactly DID we hit?
   ObjID targetobj = ev->sensor_obj;

   //hey, if there is no collision event, then we Don't want to implement
   //the "weak point"  I'll need to check again to see if the collision
   //event also holds victim.  If so, need to make sure that the
   //collision target is also the victim.

//   if (realclsnev==NULL) return kReactionNormal; 

   
   //now for some tricky matrix stuff
   Position *targetPos = ObjPosGet(targetobj);
   mxs_angvec targetfac = targetPos->fac;
   mxs_matrix targetorien;
   mx_ang2mat(&targetorien,&targetfac);
   mxs_vector weakpointoldvec;
   weakpointoldvec.x=coeff->xoffset;
   weakpointoldvec.y=coeff->yoffset;
   weakpointoldvec.z=coeff->zoffset;
   mxs_vector weakpointnewvec;
   mx_mat_mul_vec(&weakpointnewvec,&targetorien,&weakpointoldvec);
   mxs_vector targetloc = targetPos->loc.vec;
   mxs_vector weakpointrealpos;
   mx_add_vec(&weakpointrealpos,&weakpointnewvec,&targetloc);

   //is there a straight line distance function?  Can I get my location?
   //ok right now this is just a proof of concept for putting in the
   //reaction.
   if (realclsnev!=NULL) 
     {
       
       ConfigSpew("WeakPointSpew",("WeakPointSpew: Collision position:(%g %g %g).  Vulnerable position:(%g %g %g)  Radius:%g\n",realclsnev->collision->clsn_pt.x,realclsnev->collision->clsn_pt.y,realclsnev->collision->clsn_pt.z,weakpointrealpos.x,weakpointrealpos.y,weakpointrealpos.z,coeff->radius));

       if (((coeff->radius)*(coeff->radius)) >= mx_dist2_vec(&weakpointrealpos,&(realclsnev->collision->clsn_pt)))
	 {
	   ConfigSpew("WeakPointSpew",("WeakPointSpew: Successful\n"));
	   level = level*coeff->slope + coeff->intercept;
	 }
       return kReactionMutate;
     }
   else
     //maybe it's an impact event caused by a physicalizing weapon.
     {
       //We're going to go up the chain looking for the impact event.
       eChainedEventKind collisionkind = kEventKindImpact;
       //grab the collision event and cast it to PhysClsn (which it is)
       const sChainedEvent* impactev = ev->stim->Find(collisionkind);
       const sDamageMsg* realimpactev = (const sDamageMsg*)impactev;

       if (realimpactev == NULL) //nope, not impact either, just stim :(
	 return kReactionNormal;

       ObjID culpritweapon = realimpactev->culprit;
       //this is only for weapons
       if (!IsWeapon(culpritweapon))
	 return kReactionNormal;
       ConfigSpew("WeakPointSpew",("WeakPointSpew: Tracing weapon impact.\n"));
       if (culpritweapon != g_CollisionObj1)
	 return kReactionNormal;

       ConfigSpew("WeakPointSpew",("WeakPointSpew: Impact position:(%g %g %g).  Vulnerable position:(%g %g %g)  Radius:%g\n",g_collision_location.x,g_collision_location.y,g_collision_location.z,weakpointrealpos.x,weakpointrealpos.y,weakpointrealpos.z,coeff->radius));

       if (((coeff->radius)*(coeff->radius)) >= mx_dist2_vec(&weakpointrealpos,&g_collision_location))
	 {
	   ConfigSpew("WeakPointSpew",("WeakPointSpew: Impact Successful\n"));
	   level = level*coeff->slope + coeff->intercept;
	   return kReactionMutate;
	 }
       else
	 return kReactionNormal;
       
     }
}

static sReactionDesc WeakPoint_desc = 
{
   "WeakPoint",
   "Weak Point",
   REACTION_PARAM_TYPE(sWeakPointParam),
   kReactionNoFlags
};

static sFieldDesc WeakPoint_fields[] = 
{
   { "Multiply by", kFieldTypeFloat, FieldLocation(sWeakPointParam,slope) },
   { "Then add", kFieldTypeFloat, FieldLocation(sWeakPointParam,intercept) },
   { "X Offset",kFieldTypeFloat,FieldLocation(sWeakPointParam,xoffset) },
   { "Y Offset",kFieldTypeFloat,FieldLocation(sWeakPointParam,yoffset) },
   { "Z Offset",kFieldTypeFloat,FieldLocation(sWeakPointParam,zoffset) },
   { "Radius",kFieldTypeFloat,FieldLocation(sWeakPointParam,radius) }
};

static sStructDesc WeakPoint_sdesc = StructDescBuild(sWeakPointParam,kStructFlagNone,WeakPoint_fields);

//****************************************
//****************************************
//****************************************
//****************************************

//----------------------------------------

static void install_built_ins(IReactions* r)
{
   r->Add(&abort_desc,abort_func,NULL);

   StructDescRegister(&amplify_sdesc);
   r->Add(&amplify_desc,amplify_func,NULL);

   StructDescRegister(&WeakPoint_sdesc);
   r->Add(&WeakPoint_desc,WeakPoint_func,NULL);
}


//------------------------------------------------------------
// Aggregate protocol 
//

HRESULT cReactions::Init()
{
   install_built_ins(this);
   return S_OK;
}

HRESULT cReactions::End()
{
   return S_OK;
}


//------------------------------------------------------------
// Reactions query 
//

class cReactionQuery : public IReactionQuery
{
   DECLARE_UNAGGREGATABLE();
public:
   cReactionQuery(cReactions::ReactionVec& vec)
      : Vec(vec), Idx(0)
   {
   }

   ~cReactionQuery() {};


   STDMETHOD(Start)() { return S_OK;};
   STDMETHOD_(BOOL,Done)() { return Idx >= Vec.Size();};
   STDMETHOD(Next)() { Idx++; return S_OK;}; 

   STDMETHOD_(ReactionID,ID)() { return Idx;};
   STDMETHOD_(const sReactionDesc*,Reaction)() { return &Vec[Idx].desc; }; 
   
private:
   cReactions::ReactionVec& Vec;
   int Idx; 
   

};

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cReactionQuery,IReactionQuery);


STDMETHODIMP_(IReactionQuery*) cReactions::QueryAll()
{
   return new cReactionQuery(ByID);
}


//------------------------------------------------------------
// Factory
//

void ReactionsCreate(void)
{
   AutoAppIPtr_(Unknown,pUnk);
   new cReactions(pUnk);
}











