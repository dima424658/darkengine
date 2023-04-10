/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/mnumprop.cpp,v 1.15 1999/05/19 16:12:10 mahk Exp $

//
// Model number property
//

#include <mnumprop.h>
#include <mnamprop.h>

#include <property.h>
#include <propface.h>
#include <propbase.h>

#include <mprintf.h>

// For our property class
#include <osysbase.h>
#include <objnotif.h>
#include <propfac_.h>

// for objloadmodel
#include <objmodel.h>
#include <trait.h>

// needs to be out of report, so it gets the null registers
#include <report.h>

#ifdef REPORT
#include <traitman.h>
#include <traitbas.h>
#include <objscale.h>
#include <objquery.h>
#include <objedit.h>
#include <matrix.h>
#include <rendprop.h>
#endif

// Must be last header
#include <dbmem.h>

static IIntProperty* modelnumberprop = NULL;

////////////////////////////////////////////////////////////
// MODEL NUMBER PROPERTY CLASS
//
// This is, in many ways, totally gratuitous.  There ought to be a better
// way for systems with properties to hook object creation.
//

// our derived class, with overridden notify
class cModelNumProperty : public cGenericIntProperty
{
public:
   cModelNumProperty(sPropertyDesc* desc, ePropertyImpl impl)
      : cGenericIntProperty(desc,impl) 
   {
      mDesc.flags |= kPropertySendEndCreate; 
   };

   // our notify
   STDMETHOD_(void,Notify)(ePropertyNotifyMsg msg, PropNotifyData data)
   {
      switch (NOTIFY_MSG(msg))
      {
         case kObjNotifyReset:
         {
            sPropertyObjIter iter; 
            ObjID obj; 
            int idx; 

            IterStart(&iter); 
            while (IterNextValue(&iter,&obj,&idx))
               objmodelDecRef(idx); 
            IterStop(&iter); 
         }
         
         case kObjNotifyCreate:
         case kObjNotifyLoadObj:
         {
            uObjNotifyData info;
            info.raw = data;
            if (OBJ_IS_CONCRETE(info.obj))
               ObjLoadModel(info.obj);
         }
      }
      cGenericIntProperty::Notify(msg,data);
   }
};

////////////////////////
// report generatorObjID arch;

#ifdef REPORT
// void objmodelGetSize(int idx, int *model_mem, int *text_mem)

struct _sMnumInfo
{
   int idx;
   ObjID arch;
};

static void _mnum_report(int WarnLevel, void *data, char *buffer)
{
   _sMnumInfo *pMInfo=(_sMnumInfo *)Malloc(sizeof(_sMnumInfo)*gMaxObjID);
   sPropertyObjIter iter;
   char *p=buffer;
   ObjID prop_obj;
   int val, i, j;

   rsprintf(&p,"\nReport on Object Model Memory\n");
   memset(pMInfo,0,sizeof(_sMnumInfo)*gMaxObjID);
   AutoAppIPtr_(TraitManager,pTraitMan);   
   modelnumberprop->IterStart(&iter);
   while (modelnumberprop->IterNextValue(&iter,&prop_obj,&val))
   {
      if (OBJ_IS_CONCRETE(prop_obj))
      {
         if (pMInfo[prop_obj].arch==0)
            pMInfo[prop_obj].idx=val;
         else
            if (pMInfo[prop_obj].idx!=val)
            {  // overriding, i guess... do we want this?
               if (WarnLevel>=kReportLevel_Warning)
                  rsprintf(&p," %s overrides the model number of its archetype (%s)\n",ObjWarnName(prop_obj),ObjWarnName(pMInfo[prop_obj].arch));
               pMInfo[prop_obj].idx=val;
               pMInfo[prop_obj].arch=0;
            }
      }
      else
      {
         mxs_vector arch_scale;
         if (!ObjGetScale(prop_obj,&arch_scale))
            arch_scale.x=arch_scale.y=arch_scale.z=1.0;
         IObjectQuery *pQuery=pTraitMan->Query(prop_obj,kTraitQueryDescendents);
         for ( ; !pQuery->Done(); pQuery->Next() )
         {
            ObjID obj=pQuery->Object();
            if (OBJ_IS_CONCRETE(obj))
               if (pMInfo[obj].idx)
               {
                  if (pMInfo[obj].idx!=val)
                     if (WarnLevel>=kReportLevel_Warning)
                        rsprintf(&p," %s overrides the model number of its archetype (%s)\n",ObjWarnName(obj),ObjWarnName(pMInfo[obj].arch));
               }
               else
               {
                  pMInfo[obj].arch=prop_obj;
                  pMInfo[obj].idx=val;
               }
         }
         SafeRelease(pQuery);
      }
   }
   modelnumberprop->IterStop(&iter);

   int t_model_mem=0, t_txt_mem=0, t_used=0;;
   for (i=1; i<MAX_OBJMODELS; i++)
   {
      ObjID for_us[512], us_cnt=0, scale_obj[16], unscale_obj, notdrawn_obj;
      mxs_vector scales[16], cur_scale, scale_diff, arch_scale;
      int used=0, unscaled=0, scale_cnt=0, scale_used[16], k, diff_arch=0, notdrawn=0;

      memset(scale_used,0,sizeof(int)*16);
      for (j=1; j<gMaxObjID; j++)      
         if (pMInfo[j].idx==i)
         {
            for_us[us_cnt++]=j;
            used++;
         }
      for (j=0; j<us_cnt; j++)
      {
         // if unrendered, track
         if ((ObjRenderType(for_us[j])!=kRenderNormally)&&(ObjRenderType(for_us[j])!=kRenderUnlit))
         {
            notdrawn++;
            notdrawn_obj=for_us[j];
         }
         else if (ObjGetScale(for_us[j],&cur_scale))
         {
            for (k=0; k<scale_cnt; k++)
            {
               mx_sub_vec(&scale_diff,&cur_scale,&scales[k]);
               if (mx_mag2_vec(&scale_diff)<0.01)
                  break;
            }
            if (k==scale_cnt)
            {
               scale_obj[scale_cnt]=for_us[j];
               scales[scale_cnt++]=cur_scale;
            }
            scale_used[k]++;
            if (ObjGetScale(pMInfo[for_us[j]].arch,&arch_scale))
            {
               mx_subeq_vec(&cur_scale,&arch_scale);
               if (mx_mag2_vec(&cur_scale)>0.01)
                  if (WarnLevel>=kReportLevel_Warning)                  
                     rsprintf(&p," %s scaled differently than archetype %s\n",ObjWarnName(for_us[j]),ObjWarnName(pMInfo[for_us[j]].arch));
            }
         }
         else
         {
            unscaled++;
            unscale_obj=for_us[j];
         }
      }
      if (used)
      {
         int mdmem, txtmem;
         objmodelGetSize(i,&mdmem,&txtmem);
         t_model_mem+=mdmem;
         t_txt_mem+=txtmem;
         t_used++;
         if (WarnLevel>=kReportLevel_Info)
         {
            if (used>notdrawn)
               rsprintf(&p,"idx %3.d [%8s] mem %3.dK %3.dK used %d time%s",
                        i,objmodelGetName(i),(mdmem+1023)/1024,(txtmem+1023)/1024,used,used==1?"":"s");
            else
               rsprintf(&p,"idx %3.d [%8s] mem %3.dK %3.dK not drawn (x%d) [inc %s]",
                        i,objmodelGetName(i),(mdmem+1023)/1024,(txtmem+1023)/1024,notdrawn,ObjWarnName(notdrawn_obj));
            if (scale_cnt>1 || (scale_cnt==1 && (!unscaled)))
               if (unscaled)
                  rsprintf(&p," at %d scales\n",scale_cnt+1);
               else
                  rsprintf(&p," at %d scale%s SAD never unscaled\n",scale_cnt,scale_cnt==1?"":"s");
            else
               rsprintf(&p,"\n");
            if (WarnLevel>=kReportLevel_DumpAll)
            {
               int m;
               for (m=0; m<scale_cnt; m++)
                  rsprintf(&p,"  cnt %d scale %f %f %f first %s\n",scale_used[m],
                           scales[m].x,scales[m].y,scales[m].z,ObjWarnName(scale_obj[m]));
               if (scale_cnt&&unscaled)
                  rsprintf(&p,"  and the model appears unscaled (inc. %s)\n",ObjWarnName(unscale_obj));
            }
         }
         else if (scale_cnt && (!unscaled))
         {
            rsprintf(&p,"idx %3.d [%8s] mem %3.dK %3.dK used %d times",
                     i,objmodelGetName(i),(mdmem+1023)/1024,(txtmem+1023)/1024,used);
            rsprintf(&p," at %d scale%s SAD never unscaled\n",scale_cnt,scale_cnt==1?"":"s");
            for (int m=0; m<scale_cnt; m++)
               rsprintf(&p,"  cnt %d scale %f %f %f first %s\n",scale_used[m],
                        scales[m].x,scales[m].y,scales[m].z,ObjWarnName(scale_obj[m]));
         }
      }
   }
   rsprintf(&p,"Total modelnum models loaded %d mem %dK %dK textures\n",t_used,t_model_mem/1024,t_txt_mem/1024);
   Free(pMInfo);
   rsprintf(&p,"Going Straight to objmodel says:\n");
   objmodelListMemory(&p,WarnLevel>=kReportLevel_DumpAll);
}
#endif

////////////////////////////////////////////////////////////
// MODEL NUMBER PROPERTY CREATION 
//

#define MODELNUMBERPROP_IMPL kPropertyImplDense

static sPropertyConstraint modelnumberprop_const[] =
{
   { kPropertyRequires, PROP_MODELNAME_NAME },
   { kPropertyNullConstraint, NULL }
};

static sPropertyDesc modelnumberprop_desc =
{
   PROP_MODELNUMBER_NAME,
   kPropertyTransient|kPropertyNoEdit,
   modelnumberprop_const,
   0, 0,
   {0, 0, 0},
   kPropertyChangeLocally,
};

// called when the number is modified
static void LGAPI ModelNumberListener(sPropertyListenMsg* msg, PropListenerData data)
{
   if (msg->property != modelnumberprop->GetID())
      return;

   int idx = msg->value.intval; 
   if (msg->type & kListenPropModify)
   {
      if (idx >= 0)
      objmodelIncRef(idx);
   }
   else if (msg->type & kListenPropSet)
   {
      modelnumberprop->Set(msg->obj,-1);
   }
      
   if (msg->type & kListenPropUnset)
   {
      if (idx >= 0)
         objmodelDecRef(idx);
   }
}

// Init the property
void ModelNumberPropInit(void)
{
   modelnumberprop = new cModelNumProperty(&modelnumberprop_desc, MODELNUMBERPROP_IMPL);
   // Get notified when model number is set
   modelnumberprop->Listen(kListenPropSet|kListenPropUnset|kListenPropModify, ModelNumberListener, NULL);
   ReportRegisterGenCallback(_mnum_report,kReportAllObj,"ModelNum",NULL);   
}

void ModelNumberPropTerm(void)
{
   SafeRelease(modelnumberprop);   
   ReportUnRegisterGenCallback(_mnum_report,kReportAllObj,NULL);
}

//////////////
// get and set functions
BOOL ObjGetModelNumber(ObjID obj, int *num)
{
   Assert_(modelnumberprop);
   return modelnumberprop->Get(obj, num);
}

void ObjSetModelNumber(ObjID obj, int *num)
{
   Assert_(modelnumberprop);
   modelnumberprop->Set(obj, *num);
}

int ObjGetModelType(ObjID obj)
{
   int temp = -1;
   Assert_(modelnumberprop);
   // no need to test, since the spec for get is 
   // that temp doesn't get modified
   modelnumberprop->Get(obj, &temp);
   return temp;
}
   
////////////////////////////////////////////////////////////
// MODEL LOADING LOGIC
//

// Model name property

static ILabelProperty* modelname_prop()
{
   static ILabelProperty* prop = NULL;

   if (prop == NULL)
   {
      Verify(GetPropertyInterfaceNamed(PROP_MODELNAME_NAME,ILabelProperty,&prop));
   }
   return prop;
}

//
// Model name trait
// 

static ITrait* modelname_trait()
{
   static ITrait* trait = NULL;
   if (trait == NULL)
      Verify(SUCCEEDED(COMQueryInterface(modelname_prop(),IID_ITrait,(void**)&trait)));
   return trait;
}

void ObjLoadModel(ObjID obj)
{
   ObjID donor = obj;
   ILabelProperty* Name = modelname_prop();
   
   // Use the trait to look up who gives us the model name property
   if (!Name->IsSimplyRelevant(obj))
   {
      ITrait* trait = modelname_trait();
      donor = trait->GetDonor(obj); 
   }

   Label* mname;
   if (Name->GetSimple(donor,&mname))
   {
      int idx = objmodelLoadModel(mname->text);
      Assert_(modelnumberprop);
      // clear out old model number
      int oldidx = -1;
      BOOL old = modelnumberprop->GetSimple(donor,&oldidx);
      if (oldidx != idx)
      {
        if (old) modelnumberprop->Delete(obj);
        modelnumberprop->Set(donor,idx);
      }
      // instantiate by hand.
      //      modelnumberprop->Set(obj,idx);
   }
}
