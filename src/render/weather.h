/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/weather.h,v 1.2 2000/02/05 18:19:43 adurant Exp $

/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\
   weather.h

   Our weather is basically rendering effects.  It's set up through
   properties on room brushes.  So this is the properties and the code
   to render their effects.

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

#ifndef _WEATHER_H_
#pragma once
#define _WEATHER_H_

#include <propdef.h>
#include <tagfile.h>
#include <matrixs.h>


///////////////////////////////////////////////
// weather rendering property (for brushes)

typedef struct sWeather
{
   BOOL m_bDebris;              // bits of stuff following wind
   BOOL m_bFog;                 // actually goes into portal database
   BOOL m_bPrecipitation;       // rain or snow--no level can have both
   BOOL m_bWind;                // do we want any in-betweens here?
} sWeather;

#undef  INTERFACE
#define INTERFACE IWeatherProperty

DECLARE_PROPERTY_INTERFACE(IWeatherProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sWeather *);
};

extern IWeatherProperty *g_pWeatherProperty;

///////////////////////////////////////////////
// global functions

EXTERN void WeatherInit();
EXTERN void WeatherTerm();
EXTERN void WeatherReset();
EXTERN BOOL WeatherIsAllowed();
EXTERN void WeatherSetAllowed(BOOL allow);
EXTERN void WeatherSave(ITagFile *pFile);
EXTERN void WeatherLoad(ITagFile *pFile);
EXTERN void WeatherFrame(ulong nMillisec);

// Script-facing snapshot of the mission weather file variable.  Keep this
// separate from the persisted structure so the script ABI can evolve without
// changing the on-disk WEATHERVAR layout.
#define WEATHER_SCRIPT_TEXTURE_MAX 32
typedef struct sWeatherScriptParams
{
   int precipType;
   float precipFrequency;
   float precipSpeed;
   float visibleDistance;
   float renderRadius;
   float alpha;
   float brightness;
   float snowJitter;
   float rainLength;
   float splashFrequency;
   float splashRadius;
   float splashHeight;
   float splashDuration;
   char texture[WEATHER_SCRIPT_TEXTURE_MAX];
   mxs_vector wind;
} sWeatherScriptParams;

EXTERN void WeatherGetScriptParams(sWeatherScriptParams *params);
EXTERN void WeatherSetScriptParams(const sWeatherScriptParams *params);

#endif // ~_WEATHER_H_
