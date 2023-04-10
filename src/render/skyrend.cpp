/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/skyrend.cpp,v 1.11 2000/02/19 12:35:46 toml Exp $

#include <skyrend.h>
#include <skyint.h>
#include <skyobj.h>
#include <starobj.h>
#include <celstobj.h>
#include <distobj.h>
#include <cloudobj.h>
#include <config.h>
#include <scrnmode.h>

#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

extern "C" BOOL g_lgd3d;


// @TBD: make this a UI element, or maybe just config-based.
BOOL bUIOptionEnhancedSky = TRUE;


///////////////////////////////////////////////////////////

// Instantiations:

static cSky SkyRenderer;
static cStars StarRenderer;
static cCelestialObject CelestialObjectRenderer1(0);
static cCelestialObject CelestialObjectRenderer2(1);
static cCelestialObject CelestialObjectRenderer3(2);
static cCelestialObject CelestialObjectRenderer4(3);
static cDistantArt DistantArtRenderer;
static cCloudDeck CloudDeckRenderer;

// Define in order rendered:
static ISkyObject *(RenderedComponents[])=
{
   &SkyRenderer,
   &StarRenderer,
   &CelestialObjectRenderer1,
   &CelestialObjectRenderer2,
   &CelestialObjectRenderer3,
   &CelestialObjectRenderer4,
   &CloudDeckRenderer,
   &DistantArtRenderer,
   0
};


// Call when game inits
void SkyRendererAppInit()
{
   int i = 0;
 
   int cfgval;
   config_get_int("enhanced_sky",&cfgval);
   bUIOptionEnhancedSky = (cfgval==1);

   while (RenderedComponents[i])
   {
      RenderedComponents[i]->AppInit();
      i++;
   }
}

static BOOL bDidInit = FALSE;

// Call when mission loaded
void SkyRendererInit()
{
   if (bDidInit || !g_lgd3d || !bUIOptionEnhancedSky)
      return;

   int i = 0;
   while (RenderedComponents[i])
   {
      RenderedComponents[i]->Init();
      i++;
   }

   bDidInit = TRUE;
}


// call when mission over
void SkyRendererTerm()
{
   if (!bDidInit)
      return;

   int i = 0;
   while (RenderedComponents[i])
   {
      RenderedComponents[i]->Term();
      i++;
   }

   bDidInit = FALSE;
}


void SkyRendererRender()
{
   if (!g_lgd3d)
      return;

   int i = 0;

   // Make sure we're inited:
   SkyRendererInit();


   // @TBD: time this to make sure it isn't slowing us down too much.
   // Hey, this isn't implemented!
   // lgd3d_zclear();
   //
   // So do this instead:
   sScrnMode Mode;
   ScrnModeGet(&Mode);
   lgd3d_clear_z_rect(0, 0, Mode.w, Mode.h);

   while (RenderedComponents[i])
   {
      RenderedComponents[i]->Render();
      i++;
   }
}

BOOL SkyRendererTryEnhanced()
{
   return(bUIOptionEnhancedSky);
}

void SkyRendererSetEnhanced(BOOL isenhanced)
{
  //hook called by UI to do dirty work.
  if (isenhanced)
    {
      bUIOptionEnhancedSky = TRUE;
      config_set_int("enhanced_sky",1);
    }
  else
    {
      bUIOptionEnhancedSky = FALSE;
      config_set_int("enhanced_sky",0);   
    }
}

BOOL SkyRendererUseEnhanced()
{
   return g_lgd3d && SkyRenderer.Enabled() && bUIOptionEnhancedSky;
}

float SkyRendererGetHorizonDip()
{
   return SkyRenderer.GetHorizonDip();
}

float SkyRendererGetSkyIntensity(mxs_vector *pDir)
{
   return SkyRenderer.GetSkyIntensity(pDir);
}

float SkyRendererGetSkyDist(mxs_vector *pDir)
{
   return SkyRenderer.GetSkyDist(pDir);
}


float SkyRendererGetClipLat()
{
   return SkyRenderer.GetClipLat();
}

