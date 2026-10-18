/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/missrend.cpp,v 1.13 2000/02/13 20:21:04 adurant Exp $
#include <missrend.h>
#include <appagg.h>
#include <filevar.h>
#include <palmgr.h>
#include <resapi.h>
#include <palrstyp.h>
#include <respaths.h>
#include <palette.h>
#include <matrix.h>

#include <sdestool.h>
#include <sdesbase.h>
#include <port.h>

#include <config.h>
#include <csg.h>
#include <string.h>
#include <vector>
#include <dbfile.h>

// must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// Misc Rendering Params
//


// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gRenderParamsDesc = 
{
   kMissionVar,         // Where do I get saved?
   "RENDPARAMS",          // Tag file tag
   "Rendering Parameters",// friendly name
   FILEVAR_TYPE(sMissionRenderParams),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
}; 

static void pal_set(void);

// The actual global variable
class cRenderParams : public cFileVar<sMissionRenderParams,&gRenderParamsDesc> 
{
public: 

   // Silly assignment operator 
   cRenderParams& operator =(const sMissionRenderParams& v) { *(sMissionRenderParams*)this = v; return *this; }; 

protected:
   // NewDark 2.0 retains the complete 1.0 prefix and appends renderer and
   // motion-database fields. Decode the engine-owned prefix and preserve the
   // source layout/version when writing it back.
   virtual TagVersion ReadVersion() const
   {
      TagVersion version = { 2, 0 };
      return version;
   }

   virtual TagVersion WriteVersion() const
   {
      TagVersion version = gRenderParamsDesc.version;
      if (dbFormatGetWriteProfile() == kDBWritePreserveSource)
         dbFormatSourceTag(&gRenderParamsDesc.tag,&version,NULL);
      else if (dbFormatUsesNewDark())
      {
         version.major = 2;
         version.minor = 0;
      }
      return version;
   }

   virtual BOOL ReadBlock(ITagFile* file, const TagVersion& version)
   {
      // Thief 1/Gold stores only a palette name and one scalar ambient-light
      // value in RENDPARAMS 1.0.  Thief 2 kept the same block version while
      // expanding ambient light to RGB and appending the sunlight fields, so
      // the version alone cannot distinguish the two layouts.
      struct sLegacyRenderParams
      {
         char pal_res[16];
         float ambient_light;
      };

      if (file->TellFromEnd() == sizeof(sLegacyRenderParams))
      {
         sLegacyRenderParams legacy;
         if (file->Read((char*)&legacy,sizeof(legacy)) != sizeof(legacy))
            return FALSE;

         Reset();
         memcpy(pal_res,legacy.pal_res,sizeof(pal_res));
         ambient_light.x = legacy.ambient_light;
         ambient_light.y = legacy.ambient_light;
         ambient_light.z = legacy.ambient_light;
         return TRUE;
      }

      const ulong blockSize = file->TellFromEnd();
      const BOOL result = cFileVarBase::ReadBlock(file,version);
      if (result && blockSize > sizeof(sMissionRenderParams))
         dbCompatibilityRewriteTag(&gRenderParamsDesc.tag,
            "source layout retained; engine-owned prefix is writable");
      return result;
   }

   virtual BOOL WriteBlock(ITagFile* file)
   {
      ulong sourceSize = 0;
      dbFormatSourceTag(&gRenderParamsDesc.tag,NULL,&sourceSize);

      if (dbFormatGetWriteProfile() == kDBWritePreserveSource &&
          sourceSize == 20)
      {
         struct sLegacyRenderParams
         {
            char pal_res[16];
            float ambient_light;
         } legacy;
         memcpy(legacy.pal_res,pal_res,sizeof(legacy.pal_res));
         legacy.ambient_light = ambient_light.x;
         return file->Write((char*)&legacy,sizeof(legacy)) == sizeof(legacy);
      }

      const BOOL extended = dbFormatUsesNewDark() ||
                            sourceSize > sizeof(sMissionRenderParams);
      if (!extended)
         return cFileVarBase::WriteBlock(file);

      ulong rawSize = 0;
      const char* raw = (const char*)dbCompatibilityGetRawTag(
         &gRenderParamsDesc.tag,&rawSize,NULL);
      ulong sourceRawSize = 0;
      const char* sourceRaw = (const char*)dbFormatSourceRawTag(
         &gRenderParamsDesc.tag,&sourceRawSize,NULL);
      ulong outputSize = sourceSize > sizeof(sMissionRenderParams)
                       ? sourceSize : 232;
      if (outputSize < 232)
         outputSize = 232;
      std::vector<char> output(outputSize,0);
      if (sourceRaw && sourceRawSize)
         memcpy(output.data(),sourceRaw,
                sourceRawSize < outputSize ? sourceRawSize : outputSize);
      if (raw && rawSize)
         memcpy(output.data(),raw,rawSize < outputSize ? rawSize : outputSize);
      memcpy(output.data(),(sMissionRenderParams*)this,
             sizeof(sMissionRenderParams));

      // Both observed NewDark layouts place eight ambient-zone RGB vectors
      // at byte 100.  A promoted retail mission inherits the global ambient;
      // a source NewDark suffix (including 1.29's extra fields) stays exact.
      if (!raw || rawSize <= sizeof(sMissionRenderParams))
      {
         float* zones = (float*)(output.data() + 100);
         for (int zone = 0; zone < 8; ++zone)
         {
            zones[zone * 3 + 0] = ambient_light.x;
            zones[zone * 3 + 1] = ambient_light.y;
            zones[zone * 3 + 2] = ambient_light.z;
         }
      }
      return file->Write(output.data(),(int)output.size()) ==
             (int)output.size();
   }

public:


   void Update()
   {
      //So the palette gets set when we go into game mode anyway.
      //now, it's possible this will cause problems viewing things in 
      //and editor window.  In which case, I'll ifdef the editor bit, I guess.
      //Let's just see if it works.  AMSD

#ifdef EDITOR
        pal_set();
#endif

#ifdef DBG_ON
#ifndef EDITOR  
        //check for those things in txt16 that maybe shouldn't be...
        if (config_is_defined("wastedpalettes"))
          pal_set();
#endif 
#endif
      int r, g, b;
      portal_convert_hsb_to_rgb(&r, &g, &b, sun_h, sun_s);
      sun_rgb.x = r / 255.0f;
      sun_rgb.y = g / 255.0f;
      sun_rgb.z = b / 255.0f;
      mx_scale_vec(&sun_scaled_rgb, &sun_rgb, sun_b);

      mx_norm_vec(&portal_sunlight_norm, &sunlight_vector);
      mx_scale_vec(&portal_sunlight, &portal_sunlight_norm,
                   kPortalSunlightCastLength);
      mx_scaleeq_vec(&portal_sunlight_norm, -1);
   }

   void Reset()
   {
      pal_res[0] = '\0';
      mx_mk_vec(&ambient_light, .25, .25, .25);
      use_sun = FALSE;
      sunlight_quad = FALSE;
      mx_mk_vec(&sunlight_vector, .25f, .1f, -1.0f);
      sun_h = 0;
      sun_s = 1;
      sun_b = 100;
   }
};

static cRenderParams gRenderParams; 

static TagFileTag gLightmapParamsTag = { "LM_PARAM" };
static sFileVarDesc gLightmapParamsDesc =
{
   kMissionVar,
   "LM_PARAM",
   "NewDark Lightmap Parameters",
   FILEVAR_TYPE(sMissionLightmapParams),
   { 0, 1 },
   { 0, 1 },
};

class cLightmapParams :
   public cFileVar<sMissionLightmapParams,&gLightmapParamsDesc>
{
protected:
   virtual BOOL ShouldWrite() const
   {
      return dbFormatUsesNewDark() ||
             dbFormatTargetHasTag(&gLightmapParamsTag);
   }

   virtual TagVersion WriteVersion() const
   {
      TagVersion version = gLightmapParamsDesc.version;
      if (dbFormatGetWriteProfile() == kDBWritePreserveSource)
         dbFormatSourceTag(&gLightmapParamsTag,&version,NULL);
      return version;
   }

public:
   virtual void Reset()
   {
      data_size = 36;
      attenuation = 1.0f;
      saturation = 1.0f;
      shadow_type = 0;
      shadow_softness = 0;
      center_weight = 0.2f;
      shadow_depth = 0;
      lightmapped_water = FALSE;
      lightmap_scale = 0;
      anim_light_cutoff = 15;
   }
};

static_assert(sizeof(sMissionLightmapParams) == 40,
              "Unexpected LM_PARAM 0.1 layout");
static cLightmapParams gLightmapParams;

//------------------------------------------------------------
// SDESC 
//

static sFieldDesc param_field[] = 
{
   { "Palette Res", kFieldTypeString, FieldLocation(sMissionRenderParams,pal_res), },
   { "Ambient Light", kFieldTypeVector, FieldLocation(sMissionRenderParams,ambient_light), },
   { "Use Sunlight", kFieldTypeBool, FieldLocation(sMissionRenderParams,use_sun), },
   { "Quad Sunlight", kFieldTypeBool, FieldLocation(sMissionRenderParams,sunlight_quad), },
   { "Sunlight Direction", kFieldTypeVector, FieldLocation(sMissionRenderParams,sunlight_vector), },
   { "Sunlight Hue (0-1)", kFieldTypeFloat, FieldLocation(sMissionRenderParams,sun_h), },
   { "Sunlight Saturation (0-1)", kFieldTypeFloat, FieldLocation(sMissionRenderParams,sun_s), },
   { "Sunlight Brightness (0-1023)", kFieldTypeFloat, FieldLocation(sMissionRenderParams,sun_b), },
};


static sStructDesc param_sdesc = StructDescBuild(sMissionRenderParams,kStructFlagNone,param_field); 


//------------------------------------------------------------
// API
// 

void MissionRenderInit(void)
{
   AutoAppIPtr_(StructDescTools,pTools); 
   pTools->Register(&param_sdesc); 
}

void MissionRenderTerm(void)
{
}


//----------------------------------------

#define PRIMARY_PAL 0

static void pal_set(void)
{
   const char* pal_res = gRenderParams.pal_res; 

   char buf[32]; 
   if (!*pal_res)
   {
      buf[0] = '\0';
      if (config_get_raw("default_game_palette",buf,sizeof(buf)))
         pal_res = buf; 
   }

   if (*pal_res)
   {
      AutoAppIPtr(ResMan); 
      IRes* res = pResMan->Bind(pal_res,RESTYPE_PALETTE,NULL,PAL_RES_SUBDIR); 
      if (res)
      {
         // set palette slot zero
         palmgr_set_pal_slot_passively(0,256,(uchar*)res->Lock(),PRIMARY_PAL); 
         res->Unlock(); 
         SafeRelease(res); 
      }
   }
   else
   {
      // build the 8-bit rgb palette
      static uchar bits[] = { 3, 3, 2 };          // bits per rgb channel
      static uchar masks[] = { 0x7, 0x7, 0x3 };   // masks for same

      uchar pal[768]; 
      int n = 0; 
      for (int i = 0; i < 256; i++)
      {
         int c = i;  // use i as our bit pattern 
         int j = 0; 
         // shift up the bits so they are most significant 
         // Really, you could optimize this further by eliminating the arrays.
         
         pal[n++] = (c & masks[j]) << (8 - bits[j]); c >>= bits[j++]; 
         pal[n++] = (c & masks[j]) << (8 - bits[j]); c >>= bits[j++]; 
         pal[n++] = (c & masks[j]) << (8 - bits[j]); c >>= bits[j++]; 
      }

      palmgr_set_pal_slot_passively(0,256,pal,PRIMARY_PAL); 

   }
}

void MissionPalSet(void)
{
   pal_set(); 
   pal_update(); 
}

//----------------------------------------


const sMissionRenderParams* GetMissionRenderParams(void)
{
   return &gRenderParams; 
}


void SetMissionRenderParams(const sMissionRenderParams* params)
{
   gRenderParams = *params; 
   gRenderParams.Update(); 
}

const sMissionLightmapParams* GetMissionLightmapParams(void)
{
   return &gLightmapParams;
}

void SetMissionLightmapParams(const sMissionLightmapParams* params)
{
   if (params)
      *(sMissionLightmapParams*)&gLightmapParams = *params;
}

