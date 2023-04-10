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

#endif // ~_WEATHER_H_
