/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/h2ocolor.cpp,v 1.7 1998/10/31 07:13:55 mahk Exp $
#include <h2ocolor.h>
#include <propert_.h>
#include <dataops_.h>
#include <propsprs.h>
#include <objnotif.h>

#include <flowarch.h>
#include <textarch.h>
#include <propman.h>
#include <port.h>
#include <render.h>
#include <texmem.h>
#include <dev2d.h>
#include <family.h>

#include <resdata.h>
#include <_animtxt.h>

#include <dynarray.h>

#include <config.h>
#include <cfgdbg.h>

#include <filevar.h>
#include <propfac_.h>

#include <sdesbase.h>
#include <sdestool.h>
#include <scrnmode.h>
#include <gamescrn.h>

// Include these last
#include <dbmem.h>
#include <initguid.h>
#include <h2ocolid.h>

////////////////////////////////////////////////////////////
// Flow Color Property Implementation 
//

//
// Data ops
//

struct sWaterRGBA : public sRGBA
{
   static sWaterRGBA gDefaults; 

   sWaterRGBA(const sWaterRGBA& copy = gDefaults)
   {
      *this = copy; 
   }

   sWaterRGBA(const sRGBA& copy)
   {
      *(sRGBA*)this = copy; 
   }
};

static sRGBA def_vals = {{50,80,100,}, 0.35 }; 

sWaterRGBA sWaterRGBA::gDefaults = def_vals; 

//------------------------------------------------------------


static IFlowGroupProperty* gpFlowProp = NULL; 


static void GetFlowProp()
{
   if (!gpFlowProp)
   {
      AutoAppIPtr_(PropertyManager,pPropMan); 
      cAutoIPtr<IProperty> prop ( pPropMan->GetPropertyNamed(PROP_FLOW_GROUP_NAME) ); 
      Verify(SUCCEEDED(prop->QueryInterface(IID_IFlowGroupProperty,(void**)&gpFlowProp))); 
   }
}


////////////////////////////////////////

class cWaterColorOps : public cClassDataOps<sWaterRGBA>
{
public:
   cWaterColorOps() : cClassDataOps<sWaterRGBA>(kNoFlags) {}; 

}; 

////////////////////////////////////////

class cWaterColorStore : public cSparseHashPropertyStore<cWaterColorOps>
{
  
};

////////////////////////////////////////

typedef cSpecificProperty<IWaterColorProperty,&IID_IWaterColorProperty,const sRGBA*,cWaterColorStore> cBaseWaterColorProp; 

class cWaterColorProp : public cBaseWaterColorProp
{
protected:
   // Descriptor 
   static sPropertyDesc gDesc; 


   //
   // Data members
   //

   cDynArray<sRGBA*> mTextureColors; 
   

public:

   cWaterColorProp() 
      : cBaseWaterColorProp(&gDesc)
   {
      SetRebuildConcretes(TRUE); 
   }

   //
   // Property Methods
   //
   
   STDMETHOD_(void,Notify)(ePropertyNotifyMsg msg, PropNotifyData data)
   {
      switch(NOTIFY_MSG(msg))
      {
         case kObjNotifyReset: 
         {
            mTextureColors.SetSize(0); 
         }
         break; 
      }
      cBaseWaterColorProp::Notify(msg,data); 
   }

   STDMETHOD_(const sRGBA*,ForTexture)(int idx)
   {
      if (idx < 0 || idx >= mTextureColors.Size())
         return NULL; 
      return mTextureColors[idx]; 
   }



   STANDARD_DESCRIBE_TYPE(sRGBA); 

protected:

 
   //
   // Overrides
   //

   void RebuildConcrete(ObjID obj, BOOL relevant, uPropListenerValue val, ObjID donor)
   {
      sRGBA* pRGBA = relevant ? (sRGBA*)val.ptrval : NULL;

      // 
      // Maintain texture colors 
      //
      int idx = GetObjTextureIdx(obj); 
      if (idx >= 0)
      {
         if (idx >= mTextureColors.Size())
            mTextureColors.Grow(idx+1 - mTextureColors.Size());
            
         mTextureColors[idx] = pRGBA; 
      }
   }

};


//----------------------------------------
// Descriptor
//

sPropertyDesc cWaterColorProp::gDesc = 
{
   PROP_WATER_COLOR_NAME,
   0,  // flags
   NULL, //constraints
   0, 0, // version
   {
      "Renderer",
      "Water Texture Color", 
   }
}; 

//----------------------------------------
// Struct Desc
//

#define RGBA_FIELDS(p,t,f) \
   { p "Red",    kFieldTypeInt, FieldLocation(t,f rgb[0]), kFieldFlagUnsigned }, \
   { p "Green",  kFieldTypeInt, FieldLocation(t,f rgb[1]), kFieldFlagUnsigned }, \
   { p "Blue",   kFieldTypeInt, FieldLocation(t,f rgb[2]), kFieldFlagUnsigned }, \
   { p "Alpha",  kFieldTypeFloat, FieldLocation(t,f alpha), }

#define NOTHING /* nothing */ 

static sFieldDesc rgba_fields[] = 
{
   RGBA_FIELDS("",sRGBA, NOTHING),
}; 

static sStructDesc rgba_sdesc = StructDescBuild(sRGBA,0,rgba_fields); 

//------------------------------------------------------------
// Water colors file var 
//

struct sWaterBanks
{
   sRGBA banks[kNumWaterBanks]; 
}; 

sFileVarDesc gWaterBankDesc = 
{
   kMissionVar,
   "WATERBANKS",
   "Water Colors", 
   FILEVAR_TYPE(sWaterBanks),
   { 1, 0 }, 
   { 1, 0 }, 
};

class cWaterBanks : public cFileVar<sWaterBanks,&gWaterBankDesc>
{
public:
   void Reset()
   {
      // Lookup default rgba

      sRGBA& rgba = sWaterRGBA::gDefaults; 

      int cnt = 3;
      int rgb[3]; 
      if (config_get_value("blend_rgb",CONFIG_INT_TYPE,(config_valptr)rgb,&cnt))
         for (int i = 0; i < cnt; i++)
            rgba.rgb[i] = (uchar)rgb[i];

      config_get_float("water_alpha",&rgba.alpha); 

      for (int b = 0; b < kNumWaterBanks; b++)
         banks[b] = rgba; 
   }
}; 

static cWaterBanks gWaterBanks; 

const sRGBA* GetWaterColorForBank(int i)
{
   // Assert_(i >= 0 && i < kNumWaterBanks); 
   if (i < 0 || i >= kNumWaterBanks)
      i = 0; 
   return &gWaterBanks.banks[i]; 
}

//
// Water RGBA sdesc
//

#define BANK_FIELDS(n) RGBA_FIELDS("color " #n " ",sWaterBanks,banks[n].)

static sFieldDesc bank_fields[] = 
{
   BANK_FIELDS(0),
   BANK_FIELDS(1),
   BANK_FIELDS(2),
   BANK_FIELDS(3),
};

static sStructDesc bank_sdesc = StructDescBuild(sWaterBanks,kStructFlagNone,bank_fields); 

//------------------------------------------------------------
// Flow bank property
//

struct sFlowColor; 

class cFlowColorProp : public cGenericIntProperty
{
protected:
   static sPropertyDesc gDesc; 

   cDynArray<int> mFlowBanks;
   
public:

   cFlowColorProp() 
      : cGenericIntProperty(&gDesc,kPropertyImplSparseHash)
   {
      SetRebuildConcretes(TRUE); 
   }; 

   STDMETHOD_(void,Notify)(ePropertyNotifyMsg msg, PropNotifyData data)
   {
      switch(NOTIFY_MSG(msg))
      {
         case kObjNotifyReset: 
         {
            mFlowBanks.SetSize(0); 
            memset(pt_motion_haze_clut,0,sizeof(pt_motion_haze_clut)); 
         }
      }

      cGenericIntProperty::Notify(msg,data); 
   }

   void RebuildConcrete(ObjID obj, BOOL relevant, uPropListenerValue val, ObjID donor)
   {
      // 
      // Maintain flow groups 
      //
      GetFlowProp(); 
      int group = -1; 
      if (gpFlowProp->Get(obj,&group) && group > 0)
      {
         while (group >= mFlowBanks.Size())
            mFlowBanks.Append(0); 
            
         int bank = (relevant) ? val.intval : 0;
         mFlowBanks[group] = bank;
         pt_motion_haze_clut[group] = GetWaterBankClut(bank); 
      }
   }

   int ByFlow(int idx)
   {
      if (idx > 0 && idx < mFlowBanks.Size())
        return mFlowBanks[idx]; 
      else
        return 0; 
   }


}; 

sPropertyDesc cFlowColorProp::gDesc = 
{
   PROP_FLOW_COLOR_NAME, 
   0,
   NULL,
   0, 0, 
   { "Renderer", "Water Flow Color Index" },
}; 

//------------------------------------------------------------
// Init/Term
//

static IWaterColorProperty* gpColorProp = NULL; 
static cFlowColorProp* gpFlowColorProp = NULL; 

void WaterColorPropInit(void)
{
   gpColorProp = new cWaterColorProp; 
   gpFlowColorProp = new cFlowColorProp;
   AutoAppIPtr_(StructDescTools,pTools); 
   pTools->Register(&rgba_sdesc); 
   pTools->Register(&bank_sdesc); 
}

void WaterColorPropTerm(void) 
{
   SafeRelease(gpColorProp); 
   SafeRelease(gpFlowProp); 
   SafeRelease(gpFlowColorProp); 
}

//------------------------------------------------------------
// Accessors
//

int GetWaterBankForFlow(int idx)
{
   return gpFlowColorProp->ByFlow(idx); 
}

EXTERN ushort pt_default_water_texture_flags;

void WaterColorSetupTextures()
{
   ConfigSpew("water_texture_spew",("Setting up water textures\n")); 
   cDynArray<int> rgbs;
   cDynArray<float> alphas;
   cDynArray<grs_bitmap*> textures; 

   // get default values
   sRGBA& defRgba = gWaterBanks.banks[0]; 

   portal_cleanup_water_hack(); 

   BOOL opaque = GetGameScreenMode()->flags & kScrnMode3dDriver; 
   pt_default_water_texture_flags = opaque ? 0:BMF_TRANS;
   
   for (int i = 0; i < BACKHACK_IDX; i++)
   {
      // skip forward to WATERIN_IDX

      if (i == texmemGetMax())
         i = WATERIN_IDX; 

      if (!family_texture_is_water(i))
         continue; 

      //      ConfigSpew("water_texture_spew",("Getting texture obj\n")); 

      // force texture obj creation 
      ObjID obj = GetTextureObj(i); 

      //      ConfigSpew("water_texture_spew",("Getting color prop obj\n")); 

      const sRGBA* prgba = &defRgba;
      gpColorProp->Get(obj,&prgba); 

      ConfigSpew("water_texture_spew",("Adding %d (obj %d) to water texture list, rgba %d %d %d %f\n",i,obj,prgba->rgb[0],prgba->rgb[1],prgba->rgb[2],prgba->alpha)); 
      
      ectsAnimInfo* info = GetResAnimData(texmemGetHandle(i)); 
      if (info)
      {
         for (int j = 0; j < info->cnt; j++)
         {
            textures.Append((grs_bitmap*)info->frame_hnd[j]->DataPeek()); 
            alphas.Append(prgba->alpha); 
            for (int c = 0; c < 3; c++)
               rgbs.Append(prgba->rgb[c]); 
         }
      }
      else
      {
         textures.Append(texmemGetTexture(i)); 
         alphas.Append(prgba->alpha); 
         for (int c = 0; c < 3; c++)
            rgbs.Append(prgba->rgb[c]); 
      }
   }

   if (textures.Size())
   {
      ConfigSpew("water_texture_spew",("Have size %d\n",textures.Size())); 
      portal_setup_water_hack(textures.Size(),textures.AsPointer(),alphas.AsPointer(),rgbs.AsPointer());
   }
   else
   {  // this, I'm told, also wants to happen after a texture family load
      int rgb[3]; 
      rgb[0] = defRgba.rgb[0]; 
      rgb[1] = defRgba.rgb[1]; 
      rgb[2] = defRgba.rgb[2]; 
      portal_setup_water_hack(1, NULL, &defRgba.alpha, rgb);
      ConfigSpew("water_texture_spew",("Falling through to default\n")); 
   }

   ConfigSpew("water_texture_spew",("Done setting up water textures\n")); 
}
