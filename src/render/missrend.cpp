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
   // motion-database fields.  Decode the engine-owned prefix while keeping
   // the writer at 1.0; the compatibility layer preserves the tail.
   virtual TagVersion ReadVersion() const
   {
      TagVersion version = { 2, 0 };
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

      return cFileVarBase::ReadBlock(file,version);
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

