/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/script/engscrpt.h,v 1.29 1998/08/11 14:12:29 mahk Exp $
//
//

#pragma once

#ifndef __ENGSCRPT_H
#define __ENGSCRPT_H

#include <scrptapi.h>
#include <scrptsrv.h>
#include <objscrt.h>

#include <objscrm.h>
#include <objscrpt.h>
#include <linkscpt.h>
#include <propscpt.h>
#include <pupscrpt.h>
#include <arscrp.h>
#include <arscrs.h>
#include <contscrs.h>
#include <contscrm.h>
#include <simscrm.h>
#include <antxscpt.h>
#include <datascrs.h>


///////////////////////////////////////////////////////////////////////////////
//
// UTILITIES
//

///////////////////////////////////////////////////////////////////////////////
//
// SERVICES
//

///////////////////////////////////////
//
// Debugging and other non-ship functions
//

DECLARE_SCRIPT_SERVICE(Debug, 0xd7)
{
   STDMETHOD (MPrint)(const string&,
                      const string& = NULL_STRING,
                      const string& = NULL_STRING,
                      const string& = NULL_STRING,
                      const string& = NULL_STRING,
                      const string& = NULL_STRING,
                      const string& = NULL_STRING,
                      const string& = NULL_STRING) PURE;

   STDMETHOD (Command)(const string&,
                       const string& = NULL_STRING,
                       const string& = NULL_STRING,
                       const string& = NULL_STRING,
                       const string& = NULL_STRING,
                       const string& = NULL_STRING,
                       const string& = NULL_STRING,
                       const string& = NULL_STRING) PURE;

   STDMETHOD (Break)() PURE;
};

///////////////////////////////////////
//
// NewDark-compatible host information and file lookup services.  Squirrel
// providers use these interfaces to locate scripts without bypassing the
// active content/FM search context.
//


DECLARE_SCRIPT_SERVICE(Version, 0x228)
{
   STDMETHOD_(void, GetAppName)(BOOL titleOnly, string &result) PURE;
   STDMETHOD_(void, GetVersion)(int &major, int &minor) PURE;
   STDMETHOD_(int, IsEditor)() PURE;
   STDMETHOD_(void, GetGame)(string &result) PURE;
   STDMETHOD_(void, GetGamsys)(string &result) PURE;
   STDMETHOD_(void, GetMap)(string &result) PURE;
   STDMETHOD(GetCurrentFM)(string &result) PURE;
   STDMETHOD(GetCurrentFMPath)(string &result) PURE;
   STDMETHOD_(void, FMizeRelativePath)(const char *path, string &result) PURE;
   STDMETHOD_(void, FMizePath)(const char *path, string &result) PURE;
   STDMETHOD(GetModPath)(string &result, int which = -1) PURE;
};

enum eWhichVarFile
{
   kMissionVar,
   kGameSysVar,
   kCampaignVar,
   kNumVarFiles,
};

DECLARE_SCRIPT_SERVICE(Engine, 0x229)
{
   STDMETHOD_(BOOL, ConfigIsDefined)(const char *name) PURE;
   STDMETHOD_(BOOL, ConfigGetInt)(const char *name, int &value) PURE;
   STDMETHOD_(BOOL, ConfigGetFloat)(const char *name, real &value) PURE;
   STDMETHOD_(BOOL, ConfigGetRaw)(const char *name, string &value) PURE;
   STDMETHOD_(real, BindingGetFloat)(const char *name) PURE;
   STDMETHOD_(BOOL, FindFileInPath)(const char *pathVariable,
                                    const char *filename,
                                    string &fullName) PURE;
   STDMETHOD_(BOOL, IsRunningDX6)() PURE;
   STDMETHOD_(void, GetCanvasSize)(int &width, int &height) PURE;
   STDMETHOD_(real, GetAspectRatio)() PURE;
   STDMETHOD_(void, GetFog)(int &red, int &green, int &blue,
                            float &distance) PURE;
   STDMETHOD_(void, SetFog)(int red, int green, int blue,
                            float distance) PURE;
   STDMETHOD_(void, GetFogZone)(int zone, int &red, int &green, int &blue,
                                float &distance) PURE;
   STDMETHOD_(void, SetFogZone)(int zone, int red, int green, int blue,
                                float distance) PURE;
   STDMETHOD_(void, GetWeather)(int &type, real &frequency, real &speed,
                                real &visibility, real &radius, real &alpha,
                                real &brightness, real &snowJitter,
                                real &rainLength, real &splashFrequency,
                                real &splashRadius, real &splashHeight,
                                real &splashDuration, string &texture,
                                vector &wind) PURE;
   STDMETHOD_(void, SetWeather)(int type, real frequency, real speed,
                                real visibility, real radius, real alpha,
                                real brightness, real snowJitter,
                                real rainLength, real splashFrequency,
                                real splashRadius, real splashHeight,
                                real splashDuration, const char *texture,
                                const vector &wind) PURE;
   STDMETHOD_(BOOL, PortalRaycast)(const vector &from, const vector &to,
                                   vector &hit) PURE;
   STDMETHOD_(int, ObjRaycast)(const vector &from, const vector &to,
                               vector &hit, object &hitObject,
                               int shortCircuit, int flags,
                               object ignore1, object ignore2) PURE;
   STDMETHOD_(void, SetEnvMapZone)(int zone, const char *texture) PURE;
   STDMETHOD(SetTagBlock)(const char *name, const char *field,
                          const char *value,
                          eWhichVarFile which = kMissionVar,
                          int index = 0) PURE;
   STDMETHOD_(cMultiParm, GetTagBlock)(const char *name, const char *field,
                                       eWhichVarFile which = kMissionVar,
                                       int index = 0) PURE;
};

///////////////////////////////////////////////////////////////////////////////
//
// MESSAGES
//

///////////////////////////////////////////////////////////////////////////////
//
// Base scripts
//

/////////
// here is where we put headers for script stuff which will be part of the engine
// root script message map, but who have header/script stuff in their own header
#include <twqscrpt.h>  // IMPLEMENTS TweqComplete
#include <lockscpt.h>  // implements NowLocked, NowUnlocked
#include <sndscrpt.h>  // SoundDone and SchemaDone
#include <frobscrp.h>  // frob message maps
#include <keyscpt.h>   // implements the key service, no messages, though
#include <litscrpt.h>  // animated light control
#include <bodscrpt.h>
#include <quesscpt.h>  // quest data change messages

#ifdef SCRIPT

///////////////////////////////////////
//
// SCRIPT: EngineRootScript
//

////////////////////////////////////////
//
// Services
//

BEGIN_ROOT_SCRIPT(EngineRootScript)

   MESSAGE_DEFS:

      SCRIPT_DEFINE_MESSAGE_DEFAULT_(TweqComplete,sTweqMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT(NowLocked)
      SCRIPT_DEFINE_MESSAGE_DEFAULT(NowUnlocked)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(SoundDone, sSoundDoneMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(SchemaDone, sSchemaDoneMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(QuestChange, sQuestMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(Sim,sSimMsg)
      
      BODY_MESSAGE_DEFAULTS()
      OBJSYS_MESSAGE_DEFAULTS()
      FROB_MESSAGE_DEFINES()      
      CONTAINER_MESSAGE_DEFAULTS()

      SCRIPT_BEGIN_MESSAGE_MAP()
         SCRIPT_MSG_MAP_ENTRY(TweqComplete)
         SCRIPT_MSG_MAP_ENTRY(NowLocked)
         SCRIPT_MSG_MAP_ENTRY(NowUnlocked)
         SCRIPT_MSG_MAP_ENTRY(SoundDone)
         SCRIPT_MSG_MAP_ENTRY(SchemaDone)
         SCRIPT_MSG_MAP_ENTRY(QuestChange)
         SCRIPT_MSG_MAP_ENTRY(Sim)
  
         CONTAINER_MESSAGE_MAP_ENTRIES() 
         BODY_MESSAGE_MAP_ENTRIES()
         OBJSYS_MESSAGE_MAP_ENTRIES()
         FROB_MSG_MAP()
      SCRIPT_END_MESSAGE_MAP()

END_SCRIPT(EngineRootScript)

#endif

///////
// i think we want to have scripts which derive from EngRoot include here
// room script messages and message map - derives from the EngineRootScript
 

///////////////////////////////////////////////////////////////////////////////

#endif // __ENGSCRPT_H
