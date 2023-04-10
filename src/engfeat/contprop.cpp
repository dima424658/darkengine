/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <lg.h>
#include <relation.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <iobjsys.h>
#include <contprop.h>
#include <contain.h>
#include <appagg.h>
#include <proptype.h>
#include <propbase.h>
#include <objdef.h>
#include <traitman.h>

#include <mprintf.h>
#include <config.h>
#include <cfgdbg.h>

//Must be last header
#include <dbmem.h>

static IBoolProperty *g_pContainInheritProp = NULL;
static void RemoveContainInherit(ObjID o);

void LGAPI ContainInheritListener(sPropertyListenMsg *msg, PropListenerData data)
{
   if ((msg->type & kListenPropSet) && OBJ_IS_CONCRETE(msg->obj) 
      && (ShouldContainInherit(msg->obj)))    
   {   
      AutoAppIPtr(ContainSys);      
      AutoAppIPtr_(ObjectSystem, pObjSys);      
      AutoAppIPtr_(TraitManager, pTraitMan);      
      sContainIter *iter=NULL;            
      iter = pContainSys->IterStartInherits(msg->obj);
      for (; !iter->finished; pContainSys->IterNext(iter))	
      {	  
         //create a concrete
         ObjID containarch = iter->containee;	  
         BOOL skipthis = FALSE;	  
         //if concrete, skip it.	  
         if (OBJ_IS_CONCRETE(containarch))	                
            skipthis = TRUE;
         //make sure not already including a version of it in that same	  
         //contain slot (i.e, don't clone a healing potion to my belt
         //if I already have a healing potion on my belt, but ok if	  
         //I only have one in my general inventory or hand)	  
         sContainIter *itertwo=NULL;	  
         itertwo = pContainSys->IterStart(msg->obj);	  
         for (; !itertwo->finished; pContainSys->IterNext(itertwo))	    
         {	    
            //test not a duplicate and in the same slot.
            if ((pTraitMan->GetArchetype(itertwo->containee) == containarch)		  
               && (itertwo->type == iter->type))		
               skipthis = TRUE;	    
            //@TODO: Find a good way to deal with the fact that you might stick
            //two objects in the same rendered slot... sigh.
         }	  
         pContainSys->IterEnd(itertwo);
	  
         if (!skipthis)	    
         {	      
            ObjID new_obj = pObjSys->Create(iter->containee,kObjectConcrete);	      
            ConfigSpew("ContainSpew",("ContainSpew:Adding contain link from %d to %d(%d)\n",msg->obj,new_obj,containarch));
            //add contain links if they don't already exist	      
            pContainSys->Add(msg->obj,new_obj,iter->type,0);	    
         }	    
         else	      
         {		
            ConfigSpew("ContainSpew",("ContainSpew: Wanted to add link from %d to %d, but was already concrete\n",msg->obj,containarch));	      
         }	
      }      
      pContainSys->IterEnd(iter);    
      //ok, we updated, now remove the property.
      RemoveContainInherit(msg->obj);
   }
}

static sPropertyDesc ContainInheritDesc =
{
  PROP_CONTAIN_INHERIT_NAME,
  kPropertyInstantiate,
  NULL,
  1,
  0,
  {"Engine Features","Inherit Contains"},
};

#define CONTAIN_INHERIT_IMPL kPropertyImplDense

void InitContainInheritProp()
{
  g_pContainInheritProp = CreateBoolProperty(&ContainInheritDesc, CONTAIN_INHERIT_IMPL);
  g_pContainInheritProp->Listen(kListenPropSet, ContainInheritListener, NULL);
}

void TermContainInheritProp()
{
  SafeRelease(g_pContainInheritProp);
}

BOOL ShouldContainInherit(ObjID o)
{
  BOOL answerval;
  if (g_pContainInheritProp->Get(o,&answerval))
     return answerval;
  return FALSE;   
}

static void RemoveContainInherit(ObjID o)
{
   BOOL tempbool;
   if (g_pContainInheritProp->Get(o,&tempbool))
      g_pContainInheritProp->Delete(o);
   return;
}
