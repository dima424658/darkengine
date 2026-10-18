/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/script/engscrpt.cpp,v 1.8 1998/11/02 06:21:11 dc Exp $
//
//
//

#include <lg.h>
#include <mprintf.h>
#include <appagg.h>

#include <scrptapi.h>
#include <scrptsrv.h>

#include <command.h>
#include <config.h>
#include <contentctx.h>
#include <darklimit.h>
#include <datapath.h>
#include <gen_bind.h>
#include <wrtype.h>
#include <objcast.h>
#include <port.h>
#include <weather.h>
#include <stdlib.h>
#include <string.h>

#include <engscrpt.h>

// must be last header
#include <dbmem.h>


///////////////////////////////////////////////////////////////////////////////
//
// Message implementations
//

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDebugScrSrv
//

#ifdef PLAYTEST
EXTERN void hello_debugger(void);
#endif

DECLARE_SCRIPT_SERVICE_IMPL(cDebugScrSrv, Debug)
{
public:

   STDMETHOD (MPrint)(const string& s1,
                      const string& s2,
                      const string& s3,
                      const string& s4,
                      const string& s5,
                      const string& s6,
                      const string& s7,
                      const string& s8)
   {
#ifdef PLAYTEST
      if (getenv("DARK_SCRIPT_DEBUG"))
         mprintf("%s%s%s%s%s%s%s%s\n",
                 (const char *)s1,
                 (const char *)s2,
                 (const char *)s3,
                 (const char *)s4,
                 (const char *)s5,
                 (const char *)s6,
                 (const char *)s7,
                 (const char *)s8);
#endif
      return S_OK;
   }

   STDMETHOD (Command)(const string& s1,
                      const string& s2,
                      const string& s3,
                      const string& s4,
                      const string& s5,
                      const string& s6,
                      const string& s7,
                      const string& s8)
   {
      char buf[1024];
      sprintf(buf, "%s %s%s%s%s%s%s%s",
              (const char *)s1,
              (const char *)s2,
              (const char *)s3,
              (const char *)s4,
              (const char *)s5,
              (const char *)s6,
              (const char *)s7,
              (const char *)s8);
      CommandExecute(buf);
      return S_OK;
   }
   
   STDMETHOD (Break)()
   {
#ifdef PLAYTEST
      hello_debugger();
#endif
      return S_OK;
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cDebugScrSrv, Debug);

///////////////////////////////////////////////////////////////////////////////

namespace
{

char *NextToken(char *text, const char *delimiters, char **context)
{
#ifdef _WIN32
   return strtok_s(text, delimiters, context);
#else
   return strtok_r(text, delimiters, context);
#endif
}

BOOL IsAbsoluteContentPath(const char *path)
{
   return path && (path[0] == '\\' || path[0] == '/' ||
                   (path[0] && path[1] == ':'));
}

void JoinContentPath(char *output, size_t size, const char *root,
                     const char *leaf)
{
   const size_t rootLength = root ? strlen(root) : 0;
#ifdef _WIN32
   const char *separator = "\\";
#else
   const char *separator = "/";
#endif
   _snprintf(output, size - 1, "%s%s%s", root ? root : "",
             rootLength && root[rootLength - 1] != '\\' &&
             root[rootLength - 1] != '/' ? separator : "",
             leaf ? leaf : "");
   output[size - 1] = '\0';
}

struct sFogSettings
{
   int red, green, blue;
   float distance;
   BOOL assigned;
};

// The stock renderer has one fog definition.  Retain NewDark's eight zone
// values so scripts can reliably read back and animate the zone they set;
// zone-aware rendering can consume this same table when its WR metadata is
// available.
static sFogSettings gScriptFogZones[8];

}

DECLARE_SCRIPT_SERVICE_IMPL(cVersionScrSrv, Version)
{
public:
   STDMETHOD_(void, GetAppName)(BOOL titleOnly, string &result)
   {
#ifdef EDITOR
      result = titleOnly ? "DromEd" : "DromEd 1.28";
#else
      // This describes the executable ABI, not the selected content.  Gold
      // missions still run under the restored Thief II host and native OSMs
      // must therefore select the Thief II script-service vtables.
      const char *title = "Thief 2";
      char full[96];
      if (titleOnly)
         result = title;
      else
      {
         _snprintf(full, sizeof(full) - 1, "%s 1.28", title);
         full[sizeof(full) - 1] = '\0';
         result = full;
      }
#endif
   }

   STDMETHOD_(void, GetVersion)(int &major, int &minor)
   {
      major = 1;
      minor = 28;
   }

   STDMETHOD_(int, IsEditor)()
   {
#ifdef EDITOR
      return 1;
#else
      return 0;
#endif
   }

   STDMETHOD_(void, GetGame)(string &result)
   {
      char value[64] = "dark";
      config_get_raw("game", value, sizeof(value));
      result = value;
   }

   STDMETHOD_(void, GetGamsys)(string &result)
   {
      char value[DARK_CONTENT_MAX_PATH] = "";
      config_get_raw("gamesys", value, sizeof(value));
      result = value;
   }

   STDMETHOD_(void, GetMap)(string &result)
   {
      char value[DARK_CONTENT_MAX_PATH] = "";
      if (!config_get_raw("quickstart", value, sizeof(value)))
         config_get_raw("file", value, sizeof(value));
      result = value;
   }

   STDMETHOD(GetCurrentFM)(string &result)
   {
      const char *path = DarkContentFMPath();
      const char *name;
      if (!path || !path[0])
      {
         result = "";
         return S_FALSE;
      }
      name = strrchr(path, '\\');
      if (!name)
         name = strrchr(path, '/');
      result = name ? name + 1 : path;
      return S_OK;
   }

   STDMETHOD(GetCurrentFMPath)(string &result)
   {
      const char *path = DarkContentFMPath();
      result = path ? path : "";
      return path && path[0] ? S_OK : S_FALSE;
   }

   STDMETHOD_(void, FMizeRelativePath)(const char *path, string &result)
   {
      const char *fm = DarkContentFMPath();
      char full[DARK_CONTENT_MAX_PATH];
      if (fm && fm[0])
      {
         JoinContentPath(full, sizeof(full), fm, path);
         result = full;
      }
      else
         result = path ? path : "";
   }

   STDMETHOD_(void, FMizePath)(const char *path, string &result)
   {
      if (IsAbsoluteContentPath(path))
         result = path;
      else
         FMizeRelativePath(path, result);
   }

   STDMETHOD(GetModPath)(string &result, int which)
   {
      const char *paths = DarkContentSearchPath(kDarkContentResources);
      if (which < 0)
      {
         result = paths ? paths : "";
         return paths && paths[0] ? S_OK : S_FALSE;
      }

      char copy[DARK_LIMIT_RUNTIME_SEARCH_PATH_CHARS];
      char *context = NULL;
      char *entry;
      int index = 0;
      strncpy(copy, paths ? paths : "", sizeof(copy));
      copy[sizeof(copy) - 1] = '\0';
      for (entry = NextToken(copy, "+;", &context); entry;
           entry = NextToken(NULL, "+;", &context), ++index)
         if (index == which)
         {
            result = entry;
            return S_OK;
         }
      result = "";
      return S_FALSE;
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cVersionScrSrv, Version);

DECLARE_SCRIPT_SERVICE_IMPL(cEngineScrSrv, Engine)
{
public:
   STDMETHOD_(BOOL, ConfigIsDefined)(const char *name)
   {
      return name && config_is_defined(name);
   }

   STDMETHOD_(BOOL, ConfigGetInt)(const char *name, int &value)
   {
      return name && config_get_int(name, &value);
   }

   STDMETHOD_(BOOL, ConfigGetFloat)(const char *name, real &value)
   {
      return name && config_get_float(name, &value);
   }

   STDMETHOD_(BOOL, ConfigGetRaw)(const char *name, string &value)
   {
      char text[DARK_LIMIT_RUNTIME_SEARCH_PATH_CHARS] = "";
      const BOOL found = name && config_get_raw(name, text, sizeof(text));
      value = found ? text : "";
      return found;
   }

   STDMETHOD_(real, BindingGetFloat)(const char *name)
   {
      if (!name || !name[0] || !g_pInputBinder)
         return 0.0f;
      char command[256];
      _snprintf(command, sizeof(command) - 1, "echo $%s", name);
      command[sizeof(command) - 1] = '\0';
      const char *value = g_pInputBinder->ProcessCmd(command);
      return value ? (real)strtof(value, NULL) : 0.0f;
   }

   STDMETHOD_(BOOL, FindFileInPath)(const char *pathVariable,
                                    const char *filename,
                                    string &fullName)
   {
      char pathText[DARK_LIMIT_RUNTIME_SEARCH_PATH_CHARS] = "";
      char found[DARK_CONTENT_MAX_PATH] = "";
      Datapath path;
      if (!pathVariable || !filename ||
          !config_get_raw(pathVariable, pathText, sizeof(pathText)))
      {
         fullName = "";
         return FALSE;
      }
      DatapathClear(&path);
      DatapathAdd(&path, pathText);
      DatapathNoCurrent(&path);
      const BOOL result = DatapathFind(&path, filename, found,
                                       sizeof(found)) != 0;
      DatapathFree(&path);
      fullName = result ? found : "";
      return result;
   }

   STDMETHOD_(BOOL, IsRunningDX6)() { return FALSE; }

   STDMETHOD_(void, GetCanvasSize)(int &width, int &height)
   {
      int values[2] = { 640, 480 };
      int count = 2;
      config_get_value("game_screen_size", CONFIG_INT_TYPE, values, &count);
      width = values[0];
      height = count > 1 ? values[1] : 480;
   }

   STDMETHOD_(real, GetAspectRatio)()
   {
      int width, height;
      GetCanvasSize(width, height);
      return height ? (real)width / (real)height : 4.0f / 3.0f;
   }

   STDMETHOD_(void, GetFog)(int &red, int &green, int &blue, float &distance)
   {
      red = (int)portal_fog_color[0];
      green = (int)portal_fog_color[1];
      blue = (int)portal_fog_color[2];
      distance = portal_fog_dist;
   }

   STDMETHOD_(void, SetFog)(int red, int green, int blue, float distance)
   {
      portal_fog_color[0] = red;
      portal_fog_color[1] = green;
      portal_fog_color[2] = blue;
      portal_fog_dist = distance;
   }

   STDMETHOD_(void, GetFogZone)(int zone, int &red, int &green, int &blue,
                                float &distance)
   {
      if (zone < 1 || zone > 8)
         return;
      const sFogSettings &settings = gScriptFogZones[zone - 1];
      if (!settings.assigned)
         GetFog(red, green, blue, distance);
      else
      {
         red = settings.red;
         green = settings.green;
         blue = settings.blue;
         distance = settings.distance;
      }
   }

   STDMETHOD_(void, SetFogZone)(int zone, int red, int green, int blue,
                                float distance)
   {
      if (zone < 1 || zone > 8)
         return;
      sFogSettings &settings = gScriptFogZones[zone - 1];
      settings.red = red;
      settings.green = green;
      settings.blue = blue;
      settings.distance = distance;
      settings.assigned = TRUE;
   }

   STDMETHOD_(void, GetWeather)(int &type, real &frequency, real &speed,
                                real &visibility, real &radius, real &alpha,
                                real &brightness, real &snowJitter,
                                real &rainLength, real &splashFrequency,
                                real &splashRadius, real &splashHeight,
                                real &splashDuration, string &texture,
                                vector &wind)
   {
      sWeatherScriptParams params;
      WeatherGetScriptParams(&params);
      type = params.precipType;
      frequency = params.precipFrequency;
      speed = params.precipSpeed;
      visibility = params.visibleDistance;
      radius = params.renderRadius;
      alpha = params.alpha;
      brightness = params.brightness;
      snowJitter = params.snowJitter;
      rainLength = params.rainLength;
      splashFrequency = params.splashFrequency;
      splashRadius = params.splashRadius;
      splashHeight = params.splashHeight;
      splashDuration = params.splashDuration;
      texture = params.texture;
      wind = vector(params.wind);
   }

   STDMETHOD_(void, SetWeather)(int type, real frequency, real speed,
                                real visibility, real radius, real alpha,
                                real brightness, real snowJitter,
                                real rainLength, real splashFrequency,
                                real splashRadius, real splashHeight,
                                real splashDuration, const char *texture,
                                const vector &wind)
   {
      sWeatherScriptParams params;
      params.precipType = type;
      params.precipFrequency = frequency;
      params.precipSpeed = speed;
      params.visibleDistance = visibility;
      params.renderRadius = radius;
      params.alpha = alpha;
      params.brightness = brightness;
      params.snowJitter = snowJitter;
      params.rainLength = rainLength;
      params.splashFrequency = splashFrequency;
      params.splashRadius = splashRadius;
      params.splashHeight = splashHeight;
      params.splashDuration = splashDuration;
      strncpy(params.texture, texture ? texture : "",
              sizeof(params.texture) - 1);
      params.texture[sizeof(params.texture) - 1] = '\0';
      params.wind = wind;
      WeatherSetScriptParams(&params);
   }

   STDMETHOD_(BOOL, PortalRaycast)(const vector &from, const vector &to,
                                    vector &hit)
   {
      Location start, end, result;
      MakeLocationFromVector(&start, &from);
      MakeLocationFromVector(&end, &to);
      const BOOL clear = ::PortalRaycast(&start, &end, &result, FALSE);
      if (clear)
         hit = to;
      else
         hit = result.vec;
      return !clear;
   }

   STDMETHOD_(int, ObjRaycast)(const vector &from, const vector &to,
                               vector &hit, object &hitObject,
                               int shortCircuit, int flags,
                               object ignore1, object ignore2)
   {
      Location start, end, result;
      MakeLocationFromVector(&start, &from);
      MakeLocationFromVector(&end, &to);

      // NewDark's final two arguments are exclusions.  The stock raycaster
      // accepts an inclusion list instead, so cast normally and suppress an
      // excluded object result.  Terrain results remain authoritative.
      eObjCastResult cast = ::ObjRaycast(&start, &end, &result,
                                         shortCircuit != 0, NULL,
                                         kObjCastAllObjs, NULL,
                                         (flags & 1) != 0);
      if ((cast == kObjCastMD || cast == kObjCastMesh) &&
          (g_ObjCastObjID == (ObjID)ignore1 ||
           g_ObjCastObjID == (ObjID)ignore2))
      {
         cast = ::PortalRaycast(&start, &end, &result, (flags & 1) != 0) ?
                kObjCastNone : kObjCastTerrain;
      }
      if (cast == kObjCastNone)
         hit = to;
      else
         hit = result.vec;
      hitObject.Set(cast == kObjCastMD || cast == kObjCastMesh ?
                    g_ObjCastObjID : OBJ_NULL);
      return cast;
   }

   STDMETHOD_(void, SetEnvMapZone)(int, const char *) {}

   STDMETHOD(SetTagBlock)(const char *, const char *, const char *,
                          eWhichVarFile, int)
   {
      return E_NOTIMPL;
   }

   STDMETHOD_(cMultiParm, GetTagBlock)(const char *, const char *,
                                       eWhichVarFile, int)
   {
      return cMultiParm();
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cEngineScrSrv, Engine);

///////////////////////////////////////////////////////////////////////////////
