#ifdef _WIN32
#include <win32_platform.h>
#endif

#include "squirrelhost.h"

#ifdef DARK_ENABLE_SQUIRREL

#include <appagg.h>
#include <scrptsrv.h>
#include "../../../src/framewrk/contentctx.h"
#include <aiscrm.h>
#include <aiscrptapi.h>
#include <antxscpt.h>
#include <arscrs.h>
#include <darklimit.h>
#include <arscrm.h>
#include <bowscrpt.h>
#include <camscrs.h>
#include <contscrs.h>
#include <damgscrm.h>
#include <damgscrs.h>
#include <datascrs.h>
#include <drkinvsc.h>
#include <drkpwscr.h>
#include <drkscrs.h>
#include <drkuiscrapi.h>
#include <drscrpt.h>
#include <engscrpt.h>
#include <frobscrm.h>
#include <keyscpt.h>
#include <linkscpt.h>
#include <litscrpt.h>
#include <lockscpt.h>
#include <mprintf.h>
#include <netscrpt.h>
#include <objscrpt.h>
#include <playrobj.h>
#include <pgrpscpt.h>
#include <pickscrs.h>
#include <propscpt.h>
#include <plyscrpt.h>
#include <pupscrpt.h>
#include <quesscpt.h>
#include <scrguid.h>
#include <scrptman.h>
#include <simscrm.h>
#include <sndscrpt.h>
#include <traitman.h>
#include <twqscrpt.h>
#include <weapscpt.h>
#include "../../../src/physics/phscrpt.h"
#include <2d.h>
#include <fcolor.h>
#include <grscale.h>
#include <imgrstyp.h>
#include <resapi.h>

#undef string
#undef vector
#undef real
#undef integer
#undef boolean
#undef timer_handle

// The historical script project is compiled with /Zp1 for Dark's persisted
// and COM-facing structures.  Squirrel's public value types contain native
// pointers and must retain their normal alignment, especially on x64.
#pragma pack(push, 8)

#include <squirrel.h>
#include <sqstdaux.h>
#include <sqstdblob.h>
#include <sqstdio.h>
#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdsystem.h>

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace
{

#include "newdark_api.nut.inc"

class cNativeSquirrelModule;
static cNativeSquirrelModule* gSquirrelModule = NULL;

struct cOverlayCallback
{
   cOverlayCallback() : active(false) { sq_resetobject(&object); }
   HSQOBJECT object;
   bool active;
};

struct cOverlayBitmap
{
   cOverlayBitmap() : resource(NULL), width(0), height(0) {}
   IRes* resource;
   int width;
   int height;
};

struct cOverlayItem
{
   cOverlayItem() : bitmap(NULL), x(0), y(0), width(0), height(0),
                    alpha(255), draw(false) {}
   grs_bitmap* bitmap;
   grs_canvas canvas;
   int x, y, width, height, alpha;
   bool draw;
};

struct cQuestBinaryValue
{
   cQuestBinaryValue() { sq_resetobject(&value); }
   std::string name;
   std::string key;
   HSQOBJECT value;
};

static std::vector<cOverlayCallback> gOverlayCallbacks;
static std::vector<int> gOverlayHandlers;
static std::vector<cOverlayBitmap> gOverlayBitmaps;
static std::vector<cOverlayItem> gOverlayItems;
static cOverlayItem* gOverlayUpdateItem = NULL;
static bool gOverlayUiDirty = true;
static std::vector<cQuestBinaryValue> gQuestBinaryValues;

static void ClearOverlayState(HSQUIRRELVM vm);

static bool SquirrelTraceEnabled()
{
   return getenv("DARK_SQUIRREL_TRACE") != NULL;
}

static bool SameGuid(REFIID left, const GUID& right)
{
   return memcmp(&left, &right, sizeof(GUID)) == 0;
}

static void Print(HSQUIRRELVM vm, const SQChar* format, ...)
{
   char text[2048];
   va_list args;
   va_start(args, format);
   _vsnprintf(text, sizeof(text) - 1, format, args);
   va_end(args);
   text[sizeof(text) - 1] = '\0';
   mprintf("%s", text);
}

static void ErrorPrint(HSQUIRRELVM vm, const SQChar* format, ...)
{
   char text[2048];
   va_list args;
   va_start(args, format);
   _vsnprintf(text, sizeof(text) - 1, format, args);
   va_end(args);
   text[sizeof(text) - 1] = '\0';
   mprintf("Squirrel: %s", text);
}

static std::string LastError(HSQUIRRELVM vm)
{
   const SQInteger top = sq_gettop(vm);
   const SQChar* text = NULL;
   std::string result = "unknown Squirrel error";
   sq_getlasterror(vm);
   if (SQ_SUCCEEDED(sq_getstring(vm, -1, &text)) && text)
      result = text;
   sq_settop(vm, top);
   return result;
}

static bool RunSource(HSQUIRRELVM vm, const char* source, size_t size,
                      const char* name)
{
   const SQInteger top = sq_gettop(vm);
   if (SQ_FAILED(sq_compilebuffer(vm, source, (SQInteger)size, name, SQTrue)))
   {
      mprintf("Squirrel compile error in %s: %s\n", name,
              LastError(vm).c_str());
      sq_settop(vm, top);
      return false;
   }
   sq_pushroottable(vm);
   if (SQ_FAILED(sq_call(vm, 1, SQFalse, SQTrue)))
   {
      mprintf("Squirrel load error in %s: %s\n", name,
              LastError(vm).c_str());
      sq_settop(vm, top);
      return false;
   }
   sq_settop(vm, top);
   return true;
}

static bool ReadFile(const char* path, std::string& data)
{
   FILE* file = fopen(path, "rb");
   long size;
   if (!file)
      return false;
   if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < 0 ||
       fseek(file, 0, SEEK_SET) != 0)
   {
      fclose(file);
      return false;
   }
   data.resize((size_t)size);
   if (size && fread(&data[0], 1, (size_t)size, file) != (size_t)size)
   {
      fclose(file);
      return false;
   }
   fclose(file);
   return true;
}

static std::string JoinPath(const std::string& root, const char* leaf)
{
   if (root.empty())
      return leaf ? leaf : "";
   return root + ((root.back() == '\\' || root.back() == '/') ? "" : "\\") +
          (leaf ? leaf : "");
}

static bool SamePath(const std::string& left, const std::string& right)
{
   return _stricmp(left.c_str(), right.c_str()) == 0;
}

static int CompareScriptNames(const std::string& left,
                              const std::string& right)
{
   // NewDark uses bytewise filename order.  In particular, TBP deliberately
   // supplies Gen__M.nut after Gen_M.nut so the newer M class replaces the
   // older one.  Locale-aware CRT comparisons can collate '_' before letters.
   const size_t count = left.size() < right.size() ? left.size() : right.size();
   for (size_t i = 0; i < count; ++i)
   {
      const unsigned char a = (unsigned char)toupper((unsigned char)left[i]);
      const unsigned char b = (unsigned char)toupper((unsigned char)right[i]);
      if (a != b)
         return a < b ? -1 : 1;
   }
   if (left.size() == right.size()) return 0;
   return left.size() < right.size() ? -1 : 1;
}

static void AddUniquePath(std::vector<std::string>& paths,
                          const std::string& path)
{
   if (path.empty())
      return;
   for (size_t i = 0; i < paths.size(); ++i)
      if (SamePath(paths[i], path))
         return;
   paths.push_back(path);
}

static std::vector<std::string> SplitSearchPath(const char* list)
{
   std::vector<std::string> result;
   const char* start = list ? list : "";
   while (*start)
   {
      const char* end = start;
      while (*end && *end != '+' && *end != ';')
         ++end;
      std::string path(start, end - start);
      while (!path.empty() && isspace((unsigned char)path.front()))
         path.erase(path.begin());
      while (!path.empty() && isspace((unsigned char)path.back()))
         path.pop_back();
      if (!path.empty())
         result.push_back(path);
      start = *end ? end + 1 : end;
   }
   return result;
}

static std::vector<std::string> ScriptRoots()
{
   // Squirrel declarations loaded later replace earlier declarations.  The
   // content context is stored highest-priority first (uber mod, FM, normal
   // mod, base), so traverse the complete path in reverse.  Prepending the FM
   // separately made normal mods load after it and incorrectly override FM
   // classes such as Black Parade's victory script.
   std::vector<std::string> result;
   std::vector<std::string> paths =
      SplitSearchPath(DarkContentSearchPath(kDarkContentResources));
   for (std::vector<std::string>::reverse_iterator it = paths.rbegin();
        it != paths.rend(); ++it)
      AddUniquePath(result, *it);
   if (result.empty())
   {
      const char* fm = DarkContentFMPath();
      AddUniquePath(result, fm && fm[0] ? fm : DarkContentBasePath());
   }
   return result;
}

static std::vector<std::string> NutFiles(const std::string& root)
{
   std::vector<std::string> result;
   const std::string directory = JoinPath(root, "sq_scripts");
   const std::string pattern = JoinPath(directory, "*.nut");
   WIN32_FIND_DATAA data;
   HANDLE find = FindFirstFileA(pattern.c_str(), &data);
   if (find == INVALID_HANDLE_VALUE)
      return result;
   do
   {
      if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
         result.push_back(JoinPath(directory, data.cFileName));
   } while (FindNextFileA(find, &data));
   FindClose(find);
   std::sort(result.begin(), result.end(),
      [](const std::string& a, const std::string& b)
      {
         return CompareScriptNames(a, b) < 0;
      });
   return result;
}

static bool GetRootObject(HSQUIRRELVM vm, const char* name, HSQOBJECT* object)
{
   const SQInteger top = sq_gettop(vm);
   sq_pushroottable(vm);
   sq_pushstring(vm, name, -1);
   if (SQ_FAILED(sq_get(vm, -2)) ||
       SQ_FAILED(sq_getstackobj(vm, -1, object)))
   {
      sq_settop(vm, top);
      return false;
   }
   sq_settop(vm, top);
   return true;
}

static bool SameObject(const HSQOBJECT& left, const HSQOBJECT& right)
{
   return left._type == right._type && left._unVal.raw == right._unVal.raw;
}

static bool DerivesFrom(HSQUIRRELVM vm, const HSQOBJECT& candidate,
                        const HSQOBJECT& root)
{
   HSQOBJECT current = candidate;
   while (current._type == OT_CLASS)
   {
      const SQInteger top = sq_gettop(vm);
      HSQOBJECT base;
      sq_pushobject(vm, current);
      if (SQ_FAILED(sq_getbase(vm, -1)) ||
          SQ_FAILED(sq_getstackobj(vm, -1, &base)))
      {
         sq_settop(vm, top);
         return false;
      }
      sq_settop(vm, top);
      if (SameObject(base, root))
         return true;
      if (base._type != OT_CLASS || SameObject(base, current))
         return false;
      current = base;
   }
   return false;
}

static void SetSlotInteger(HSQUIRRELVM vm, SQInteger table,
                           const char* name, SQInteger value)
{
   sq_pushstring(vm, name, -1);
   sq_pushinteger(vm, value);
   sq_newslot(vm, table < 0 ? table - 2 : table, SQFalse);
}

static void SetSlotFloat(HSQUIRRELVM vm, SQInteger table,
                         const char* name, SQFloat value)
{
   sq_pushstring(vm, name, -1);
   sq_pushfloat(vm, value);
   sq_newslot(vm, table < 0 ? table - 2 : table, SQFalse);
}

static void SetSlotString(HSQUIRRELVM vm, SQInteger table,
                          const char* name, const char* value)
{
   sq_pushstring(vm, name, -1);
   sq_pushstring(vm, value ? value : "", -1);
   sq_newslot(vm, table < 0 ? table - 2 : table, SQFalse);
}

static void SetSlotBool(HSQUIRRELVM vm, SQInteger table,
                        const char* name, bool value)
{
   sq_pushstring(vm, name, -1);
   sq_pushbool(vm, value ? SQTrue : SQFalse);
   sq_newslot(vm, table < 0 ? table - 2 : table, SQFalse);
}

static bool MessageIs(const sScrMsg* message, const char* name)
{
   return message && message->message &&
          _stricmp(message->message, name) == 0;
}

static bool MessageStartsWith(const sScrMsg* message, const char* prefix)
{
   if (!message || !message->message || !prefix) return false;
   return _strnicmp(message->message, prefix, strlen(prefix)) == 0;
}

static bool MessageEndsWith(const sScrMsg* message, const char* suffix)
{
   if (!message || !message->message || !suffix) return false;
   const size_t messageLength = strlen(message->message);
   const size_t suffixLength = strlen(suffix);
   return messageLength >= suffixLength &&
          _stricmp(message->message + messageLength - suffixLength,
                   suffix) == 0;
}

static void PushVector(HSQUIRRELVM vm, const mxs_vector& vector)
{
   sq_newtable(vm);
   SetSlotFloat(vm, -1, "x", vector.x);
   SetSlotFloat(vm, -1, "y", vector.y);
   SetSlotFloat(vm, -1, "z", vector.z);
   SetSlotString(vm, -1, "__sq_type", "vector");
}

static void PushLinkSet(HSQUIRRELVM vm, linkset& links)
{
   sq_newtable(vm);
   SetSlotString(vm, -1, "__sq_type", "linkset");
   sq_pushstring(vm, "ids", -1);
   sq_newarray(vm, 0);
   while (links.AnyLinksLeft())
   {
      sq_pushinteger(vm, (LinkID)links.Link());
      sq_arrayappend(vm, -2);
      links.NextLink();
   }
   sq_newslot(vm, -3, SQFalse);
}

static void PushMultiParm(HSQUIRRELVM vm, const sMultiParm& value)
{
   switch (value.type)
   {
      case kMT_Int: sq_pushinteger(vm, value.i); break;
      case kMT_Obj: sq_pushinteger(vm, value.o); break;
      case kMT_Float: sq_pushfloat(vm, value.f); break;
      case kMT_String: sq_pushstring(vm, value.psz ? value.psz : "", -1); break;
      case kMT_Vector:
         if (value.pVec) PushVector(vm, *value.pVec); else sq_pushnull(vm);
         break;
      default: sq_pushnull(vm); break;
   }
}

// NewDark models C++ reference arguments as mutable Squirrel objects such as
// int_ref and vector.  __dark_to_host deliberately copies those objects before
// crossing the native boundary, so native calls return an output envelope and
// the bootstrap copies the values back into the original arguments.
static void BeginOutputs(HSQUIRRELVM vm)
{
   sq_pushstring(vm, "__sq_outputs", -1);
   sq_newarray(vm, 0);
}

static void BeginOutput(HSQUIRRELVM vm, SQInteger index)
{
   sq_newtable(vm);
   SetSlotInteger(vm, -1, "index", index);
   sq_pushstring(vm, "value", -1);
   sq_newtable(vm);
}

static void EndOutput(HSQUIRRELVM vm)
{
   // Store the value table in the output record, then append that record to
   // the __sq_outputs array immediately below it.
   sq_newslot(vm, -3, SQFalse);
   sq_arrayappend(vm, -2);
}

static void AppendIntegerOutput(HSQUIRRELVM vm, SQInteger index,
                                SQInteger value)
{
   BeginOutput(vm, index);
   SetSlotInteger(vm, -1, "value", value);
   EndOutput(vm);
}

static void AppendFloatOutput(HSQUIRRELVM vm, SQInteger index, SQFloat value)
{
   BeginOutput(vm, index);
   SetSlotFloat(vm, -1, "value", value);
   EndOutput(vm);
}

static void AppendStringOutput(HSQUIRRELVM vm, SQInteger index,
                               const char* value)
{
   BeginOutput(vm, index);
   SetSlotString(vm, -1, "value", value);
   EndOutput(vm);
}

static void AppendVectorOutput(HSQUIRRELVM vm, SQInteger index,
                               const mxs_vector& value)
{
   BeginOutput(vm, index);
   SetSlotFloat(vm, -1, "x", value.x);
   SetSlotFloat(vm, -1, "y", value.y);
   SetSlotFloat(vm, -1, "z", value.z);
   SetSlotString(vm, -1, "__sq_type", "vector");
   EndOutput(vm);
}

static void EndOutputs(HSQUIRRELVM vm)
{
   // The result table is immediately below the key and output array.
   sq_newslot(vm, -3, SQFalse);
}

static bool PushArgument(HSQUIRRELVM vm, SQInteger index)
{
   sq_pushinteger(vm, index);
   if (SQ_FAILED(sq_get(vm, 4)))
   {
      sq_pushnull(vm);
      return false;
   }
   return true;
}

static SQInteger ArgumentCount(HSQUIRRELVM vm)
{
   return sq_gettop(vm) >= 4 && sq_gettype(vm, 4) == OT_ARRAY ?
          sq_getsize(vm, 4) : 0;
}

static SQInteger ArgumentInteger(HSQUIRRELVM vm, SQInteger index,
                                 SQInteger fallback = 0)
{
   const SQInteger top = sq_gettop(vm);
   SQInteger value = fallback;
   if (PushArgument(vm, index))
   {
      if (SQ_FAILED(sq_getinteger(vm, -1, &value)))
      {
         SQFloat floating;
         if (SQ_SUCCEEDED(sq_getfloat(vm, -1, &floating)))
            value = (SQInteger)floating;
      }
   }
   sq_settop(vm, top);
   return value;
}

static SQFloat ArgumentFloat(HSQUIRRELVM vm, SQInteger index,
                             SQFloat fallback = 0.0f)
{
   const SQInteger top = sq_gettop(vm);
   SQFloat value = fallback;
   if (PushArgument(vm, index) && SQ_FAILED(sq_getfloat(vm, -1, &value)))
   {
      SQInteger integer;
      if (SQ_SUCCEEDED(sq_getinteger(vm, -1, &integer)))
         value = (SQFloat)integer;
   }
   sq_settop(vm, top);
   return value;
}

static std::string ArgumentString(HSQUIRRELVM vm, SQInteger index,
                                  const char* fallback = "")
{
   const SQInteger top = sq_gettop(vm);
   const SQChar* value = NULL;
   std::string result = fallback ? fallback : "";
   if (PushArgument(vm, index) &&
       SQ_SUCCEEDED(sq_getstring(vm, -1, &value)) && value)
      result = value;
   sq_settop(vm, top);
   return result;
}

static ObjID ArgumentObject(HSQUIRRELVM vm, SQInteger index,
                            IScriptMan* manager)
{
   const SQInteger top = sq_gettop(vm);
   ObjID result = 0;
   if (PushArgument(vm, index))
   {
      if (sq_gettype(vm, -1) == OT_STRING)
      {
         const SQChar* name = "";
         sq_getstring(vm, -1, &name);
         IUnknown* unknown = manager ?
            manager->GetService(&IID_IObjectScriptService) : NULL;
         IObjectScriptService* objects = (IObjectScriptService*)unknown;
         if (objects)
         {
            result = (ObjID)objects->Named(name ? name : "");
            objects->Release();
         }
      }
      else if (sq_gettype(vm, -1) == OT_TABLE ||
               sq_gettype(vm, -1) == OT_INSTANCE)
      {
         const SQInteger container = sq_gettop(vm);
         sq_pushstring(vm, "value", -1);
         SQInteger value = 0;
         if (SQ_SUCCEEDED(sq_get(vm, container)) &&
             SQ_SUCCEEDED(sq_getinteger(vm, -1, &value)))
            result = (ObjID)value;
      }
      else
      {
         SQInteger value = 0;
         SQFloat floating = 0;
         if (SQ_SUCCEEDED(sq_getinteger(vm, -1, &value)))
            result = (ObjID)value;
         else if (SQ_SUCCEEDED(sq_getfloat(vm, -1, &floating)))
            result = (ObjID)floating;
      }
   }
   sq_settop(vm, top);
   return result;
}

static mxs_vector ArgumentVector(HSQUIRRELVM vm, SQInteger index)
{
   const SQInteger top = sq_gettop(vm);
   mxs_vector result = { 0, 0, 0 };
   if (PushArgument(vm, index) &&
       (sq_gettype(vm, -1) == OT_TABLE || sq_gettype(vm, -1) == OT_INSTANCE))
   {
      const SQInteger container = sq_gettop(vm);
      const char* names[] = { "x", "y", "z" };
      float* values[] = { &result.x, &result.y, &result.z };
      for (int i = 0; i < 3; ++i)
      {
         sq_pushstring(vm, names[i], -1);
         SQFloat component = 0;
         if (SQ_SUCCEEDED(sq_get(vm, container)) &&
             SQ_SUCCEEDED(sq_getfloat(vm, -1, &component)))
            *values[i] = component;
         sq_settop(vm, container);
      }
   }
   sq_settop(vm, top);
   return result;
}

static linkkind ArgumentLinkKind(HSQUIRRELVM vm, SQInteger index,
                                 IScriptMan* manager)
{
   const SQInteger top = sq_gettop(vm);
   linkkind result(0);
   if (PushArgument(vm, index))
   {
      if (sq_gettype(vm, -1) == OT_STRING)
      {
         const SQChar* name = "";
         sq_getstring(vm, -1, &name);
         IUnknown* unknown = manager ?
            manager->GetService(&IID_ILinkToolsScriptService) : NULL;
         ILinkToolsScriptService* tools = (ILinkToolsScriptService*)unknown;
         if (tools)
         {
            result = linkkind(tools->LinkKindNamed(name));
            tools->Release();
         }
      }
      else
         result = linkkind((int)ArgumentInteger(vm, index));
   }
   sq_settop(vm, top);
   return result;
}

static cMultiParm ArgumentParm(HSQUIRRELVM vm, SQInteger index)
{
   const SQInteger top = sq_gettop(vm);
   cMultiParm result;
   if (!PushArgument(vm, index))
   {
      sq_settop(vm, top);
      return result;
   }
   switch (sq_gettype(vm, -1))
   {
      case OT_BOOL:
      {
         SQBool value = SQFalse;
         sq_getbool(vm, -1, &value);
         result = (int)(value != SQFalse);
         break;
      }
      case OT_INTEGER:
      {
         SQInteger value = 0;
         sq_getinteger(vm, -1, &value);
         result = (int)value;
         break;
      }
      case OT_FLOAT:
      {
         SQFloat value = 0;
         sq_getfloat(vm, -1, &value);
         result = (float)value;
         break;
      }
      case OT_STRING:
      {
         const SQChar* value = "";
         sq_getstring(vm, -1, &value);
         result = value;
         break;
      }
      case OT_TABLE:
      case OT_INSTANCE:
      {
         mxs_vector vector = { 0, 0, 0 };
         const SQInteger container = sq_gettop(vm);
         const char* names[] = { "x", "y", "z" };
         float* values[] = { &vector.x, &vector.y, &vector.z };
         bool valid = true;
         for (int i = 0; i < 3; ++i)
         {
            sq_pushstring(vm, names[i], -1);
            SQFloat component = 0;
            if (SQ_FAILED(sq_get(vm, container)) ||
                SQ_FAILED(sq_getfloat(vm, -1, &component)))
               valid = false;
            else
               *values[i] = component;
            sq_settop(vm, container);
         }
         if (valid)
            result = vector;
         break;
      }
   }
   sq_settop(vm, top);
   return result;
}

static void RegisterFunction(HSQUIRRELVM vm, const char* name,
                             SQFUNCTION function, SQInteger parameters,
                             const char* mask)
{
   sq_pushroottable(vm);
   sq_pushstring(vm, name, -1);
   sq_newclosure(vm, function, 0);
   if (parameters)
      sq_setparamscheck(vm, parameters, mask);
   sq_newslot(vm, -3, SQFalse);
   sq_pop(vm, 1);
}

struct cSquirrelClass
{
   std::string name;
   std::string base;
   sScrClassDesc descriptor;
};

class cNativeSquirrelScript : public IScript
{
public:
   cNativeSquirrelScript(cNativeSquirrelModule* module, const char* className,
                         ObjID object);
   virtual ~cNativeSquirrelScript();

   STDMETHOD(QueryInterface)(REFIID id, void** value);
   STDMETHOD_(ULONG, AddRef)();
   STDMETHOD_(ULONG, Release)();
   STDMETHOD_(const char*, GetClassName)();
   STDMETHOD(ReceiveMessage)(sScrMsg* message, sMultiParm* reply,
                             eScrTraceAction action = kNoAction);

   bool IsValid() const { return mValid; }

private:
   ULONG mReferences;
   cNativeSquirrelModule* mModule;
   std::string mClassName;
   ObjID mObject;
   HSQOBJECT mInstance;
   bool mValid;
};

class cNativeSquirrelModule : public IScriptModule
{
public:
   friend class cNativeSquirrelScript;

   cNativeSquirrelModule(const char* name, IScriptMan* manager,
                         tScriptPrintFunc print);
   virtual ~cNativeSquirrelModule();

   bool Initialize();
   IScript* Create(const char* className, ObjID object);
   HRESULT Dispatch(HSQOBJECT instance, const char* className, ObjID object,
                    sScrMsg* message, sMultiParm* reply,
                    eScrTraceAction action);

   IScriptMan* Manager() const { return mManager; }
   ObjID CurrentObject() const { return mCurrentObject; }
   sScrMsg* CurrentMessage() const { return mCurrentMessage; }
   sMultiParm* CurrentReply() const { return mCurrentReply; }
   const char* CurrentClass() const { return mCurrentClass.c_str(); }
   HSQUIRRELVM Vm() const { return mVm; }
   void RenderOverlays();

   STDMETHOD(QueryInterface)(REFIID id, void** value);
   STDMETHOD_(ULONG, AddRef)();
   STDMETHOD_(ULONG, Release)();
   STDMETHOD_(const char*, GetName)();
   STDMETHOD_(const sScrClassDesc*, GetFirstClass)(tScrIter* iterator);
   STDMETHOD_(const sScrClassDesc*, GetNextClass)(tScrIter* iterator);
   STDMETHOD_(void, EndClassIter)(tScrIter* iterator);

private:
   bool LoadScripts();
   bool InventoryClasses();
   bool Instantiate(const char* className, ObjID object, HSQOBJECT* instance);

   ULONG mReferences;
   std::string mName;
   IScriptMan* mManager;
   tScriptPrintFunc mPrint;
   HSQUIRRELVM mVm;
   std::vector<std::unique_ptr<cSquirrelClass> > mClasses;

   ObjID mCurrentObject;
   sScrMsg* mCurrentMessage;
   sMultiParm* mCurrentReply;
   std::string mCurrentClass;
};

static IScript* CreateSquirrelScript(const char* className, ObjID object)
{
   return gSquirrelModule ? gSquirrelModule->Create(className, object) : NULL;
}

static SQInteger NativeGetSelf(HSQUIRRELVM vm)
{
   sq_pushinteger(vm, gSquirrelModule ? gSquirrelModule->CurrentObject() : 0);
   return 1;
}

static SQInteger NativeGetMessage(HSQUIRRELVM vm)
{
   sScrMsg* message = gSquirrelModule ? gSquirrelModule->CurrentMessage() : NULL;
   sq_newtable(vm);
   if (!message)
      return 1;
   SetSlotInteger(vm, -1, "from", message->from);
   SetSlotInteger(vm, -1, "to", message->to);
   SetSlotString(vm, -1, "message", message->message);
   SetSlotString(vm, -1, "name", message->message);
   SetSlotInteger(vm, -1, "time", message->time);
   SetSlotInteger(vm, -1, "flags", message->flags);
   sq_pushstring(vm, "data", -1); PushMultiParm(vm, message->data); sq_newslot(vm, -3, SQFalse);
   sq_pushstring(vm, "data2", -1); PushMultiParm(vm, message->data2); sq_newslot(vm, -3, SQFalse);
   sq_pushstring(vm, "data3", -1); PushMultiParm(vm, message->data3); sq_newslot(vm, -3, SQFalse);
   if (MessageIs(message, "Timer"))
   {
      const sScrTimerMsg* timer = static_cast<const sScrTimerMsg*>(message);
      SetSlotString(vm, -1, "name", timer->name);
   }
   else if (MessageIs(message, "Sim"))
   {
      const sSimMsg* sim = static_cast<const sSimMsg*>(message);
      SetSlotBool(vm, -1, "starting", sim->starting != FALSE);
   }
   else if (MessageStartsWith(message, "FrobTool") ||
            MessageStartsWith(message, "FrobWorld") ||
            MessageStartsWith(message, "FrobInv"))
   {
      const sFrobMsg* frob = static_cast<const sFrobMsg*>(message);
      SetSlotInteger(vm, -1, "SrcObjId", frob->SrcObjId);
      SetSlotInteger(vm, -1, "DstObjId", frob->DstObjId);
      SetSlotInteger(vm, -1, "Frobber", frob->Frobber);
      SetSlotInteger(vm, -1, "SrcLoc", frob->SrcLoc);
      SetSlotInteger(vm, -1, "DstLoc", frob->DstLoc);
      SetSlotFloat(vm, -1, "Sec", frob->Sec);
      SetSlotBool(vm, -1, "Abort", frob->Abort != FALSE);
   }
   else if (MessageStartsWith(message, "Phys"))
   {
      const sPhysMsg* physics = static_cast<const sPhysMsg*>(message);
      SetSlotInteger(vm, -1, "Submod", physics->Submod);
      SetSlotInteger(vm, -1, "collType", physics->collType);
      SetSlotInteger(vm, -1, "collObj", physics->collObj);
      SetSlotInteger(vm, -1, "collSubmod", physics->collSubmod);
      SetSlotFloat(vm, -1, "collMomentum", physics->collMomentum);
      sq_pushstring(vm, "collNormal", -1); PushVector(vm, physics->collNormal); sq_newslot(vm, -3, SQFalse);
      sq_pushstring(vm, "collPt", -1); PushVector(vm, physics->collPt); sq_newslot(vm, -3, SQFalse);
      SetSlotInteger(vm, -1, "contactType", physics->contactType);
      SetSlotInteger(vm, -1, "contactObj", physics->contactObj);
      SetSlotInteger(vm, -1, "contactSubmod", physics->contactSubmod);
      SetSlotInteger(vm, -1, "transObj", physics->transObj);
      SetSlotInteger(vm, -1, "transSubmod", physics->transSubmod);
   }
   else if (MessageStartsWith(message, "Door"))
   {
      const sDoorMsg* door = static_cast<const sDoorMsg*>(message);
      SetSlotInteger(vm, -1, "ActionType", door->ActionType);
      SetSlotInteger(vm, -1, "PrevActionType", door->PrevActionType);
      SetSlotBool(vm, -1, "isProxy", door->isProxy != FALSE);
   }
   else if (MessageIs(message, "SignalAI"))
   {
      const sAISignalMsg* signal = static_cast<const sAISignalMsg*>(message);
      SetSlotString(vm, -1, "signal", signal->signal);
   }
   else if (MessageIs(message, "PatrolPoint"))
   {
      const sAIPatrolPointMsg* patrol = static_cast<const sAIPatrolPointMsg*>(message);
      SetSlotInteger(vm, -1, "patrolObj", patrol->patrolObj);
   }
   else if (MessageIs(message, "Alertness"))
   {
      const sAIAlertnessMsg* alert = static_cast<const sAIAlertnessMsg*>(message);
      SetSlotInteger(vm, -1, "level", alert->level);
      SetSlotInteger(vm, -1, "oldLevel", alert->oldLevel);
   }
   else if (MessageIs(message, "HighAlert"))
   {
      const sAIHighAlertMsg* alert = static_cast<const sAIHighAlertMsg*>(message);
      SetSlotInteger(vm, -1, "level", alert->level);
      SetSlotInteger(vm, -1, "oldLevel", alert->oldLevel);
   }
   else if (MessageIs(message, "AIModeChange"))
   {
      const sAIModeChangeMsg* mode = static_cast<const sAIModeChangeMsg*>(message);
      SetSlotInteger(vm, -1, "mode", mode->mode);
      SetSlotInteger(vm, -1, "previous_mode", mode->previous_mode);
   }
   else if (MessageIs(message, "ObjActResult"))
   {
      const sAIObjActResultMsg* result = static_cast<const sAIObjActResultMsg*>(message);
      SetSlotInteger(vm, -1, "action", result->action);
      SetSlotInteger(vm, -1, "result", result->result);
      sq_pushstring(vm, "actdata", -1); PushMultiParm(vm, result->data); sq_newslot(vm, -3, SQFalse);
      SetSlotInteger(vm, -1, "target", result->target);
   }
   else if (MessageIs(message, "Damage"))
   {
      const sDamageScrMsg* damage = static_cast<const sDamageScrMsg*>(message);
      SetSlotInteger(vm, -1, "kind", damage->kind);
      SetSlotInteger(vm, -1, "damage", damage->damage);
      SetSlotInteger(vm, -1, "culprit", damage->culprit);
   }
   else if (MessageIs(message, "Slain"))
   {
      const sSlayMsg* slain = static_cast<const sSlayMsg*>(message);
      SetSlotInteger(vm, -1, "culprit", slain->culprit);
      SetSlotInteger(vm, -1, "kind", slain->kind);
   }
   else if (MessageIs(message, "TweqComplete"))
   {
      const sTweqMsg* tweq = static_cast<const sTweqMsg*>(message);
      SetSlotInteger(vm, -1, "Type", tweq->Type);
      SetSlotInteger(vm, -1, "Op", tweq->Op);
      SetSlotInteger(vm, -1, "Dir", tweq->Dir);
   }
   else if (MessageEndsWith(message, "Stimulus"))
   {
      const sStimMsg* stimulus = static_cast<const sStimMsg*>(message);
      SetSlotInteger(vm, -1, "stimulus", (ObjID)stimulus->stimulus);
      SetSlotFloat(vm, -1, "intensity", stimulus->intensity);
      SetSlotInteger(vm, -1, "sensor", stimulus->sensor);
      SetSlotInteger(vm, -1, "source", stimulus->source);
   }
   return 1;
}

static void PushParsedUserParams(HSQUIRRELVM vm, const char* text)
{
   sq_newtable(vm);
   const char* cursor = text ? text : "";
   while (*cursor)
   {
      while (*cursor == ';' || isspace((unsigned char)*cursor)) ++cursor;
      const char* equals = strchr(cursor, '=');
      if (!equals) break;
      const char* end = strchr(equals + 1, ';');
      if (!end) end = cursor + strlen(cursor);
      std::string key(cursor, equals - cursor);
      std::string value(equals + 1, end - equals - 1);
      while (!key.empty() && isspace((unsigned char)key.back())) key.pop_back();
      while (!value.empty() && isspace((unsigned char)value.front())) value.erase(value.begin());
      while (!value.empty() && isspace((unsigned char)value.back())) value.pop_back();
      bool identifier = !key.empty() && (isalpha((unsigned char)key[0]) || key[0] == '_');
      for (size_t i = 1; identifier && i < key.size(); ++i)
         identifier = isalnum((unsigned char)key[i]) || key[i] == '_';
      if (identifier)
      {
         sq_pushstring(vm, key.c_str(), (SQInteger)key.size());
         char* numberEnd = NULL;
         const double number = strtod(value.c_str(), &numberEnd);
         if (numberEnd && numberEnd != value.c_str() && *numberEnd == '\0')
         {
            const double integer = (double)(SQInteger)number;
            if (integer == number) sq_pushinteger(vm, (SQInteger)number);
            else sq_pushfloat(vm, (SQFloat)number);
         }
         else
         {
            if (value.size() >= 2 &&
                ((value.front() == '"' && value.back() == '"') ||
                 (value.front() == '\'' && value.back() == '\'')))
               value = value.substr(1, value.size() - 2);
            sq_pushstring(vm, value.c_str(), (SQInteger)value.size());
         }
         sq_newslot(vm, -3, SQFalse);
      }
      cursor = *end ? end + 1 : end;
   }
}

static SQInteger NativeGetUserParams(HSQUIRRELVM vm)
{
   if (!gSquirrelModule)
   {
      sq_newtable(vm);
      return 1;
   }
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_IPropertyScriptService);
   IPropertyScriptService* property = (IPropertyScriptService*)unknown;
   cMultiParm note;
   if (property)
   {
      note = property->Get(object(gSquirrelModule->CurrentObject()), "Design Note", NULL);
      property->Release();
   }
   PushParsedUserParams(vm, note.type == kMT_String ? note.psz : "");
   return 1;
}

static SQInteger NativeRetainCallback(HSQUIRRELVM vm)
{
   HSQOBJECT object;
   if (SQ_FAILED(sq_getstackobj(vm, 2, &object)))
   {
      sq_pushinteger(vm, 0);
      return 1;
   }
   for (size_t i = 0; i < gOverlayCallbacks.size(); ++i)
      if (gOverlayCallbacks[i].active &&
          SameObject(gOverlayCallbacks[i].object, object))
      {
         sq_pushinteger(vm, (SQInteger)i + 1);
         return 1;
      }
   cOverlayCallback callback;
   callback.object = object;
   callback.active = true;
   sq_addref(vm, &callback.object);
   gOverlayCallbacks.push_back(callback);
   sq_pushinteger(vm, (SQInteger)gOverlayCallbacks.size());
   return 1;
}

static SQInteger ArgumentCallback(HSQUIRRELVM vm, SQInteger index)
{
   const SQInteger top = sq_gettop(vm);
   SQInteger result = 0;
   if (PushArgument(vm, index) &&
       (sq_gettype(vm, -1) == OT_TABLE || sq_gettype(vm, -1) == OT_INSTANCE))
   {
      const SQInteger container = sq_gettop(vm);
      sq_pushstring(vm, "id", -1);
      if (SQ_SUCCEEDED(sq_get(vm, container)))
         sq_getinteger(vm, -1, &result);
   }
   sq_settop(vm, top);
   return result;
}

static bool InvokeOverlayCallback(HSQUIRRELVM vm, int handle,
                                  const char* method)
{
   if (!vm || handle <= 0 || (size_t)handle > gOverlayCallbacks.size())
      return false;
   cOverlayCallback& callback = gOverlayCallbacks[handle - 1];
   if (!callback.active)
      return false;
   const SQInteger top = sq_gettop(vm);
   sq_pushobject(vm, callback.object);
   sq_pushstring(vm, method, -1);
   if (SQ_FAILED(sq_get(vm, -2)))
   {
      sq_settop(vm, top);
      return false;
   }
   sq_pushobject(vm, callback.object);
   if (SQ_FAILED(sq_call(vm, 1, SQFalse, SQTrue)))
   {
      mprintf("Squirrel overlay %s failed: %s\n", method,
              LastError(vm).c_str());
      sq_settop(vm, top);
      return false;
   }
   sq_settop(vm, top);
   return true;
}

static SQInteger CallRoot(HSQUIRRELVM vm, const char* method)
{
   cNativeSquirrelModule* module = gSquirrelModule;
   if (!module) { sq_pushnull(vm); return 1; }
   IScriptMan* manager = module->Manager();
   const ObjID self = module->CurrentObject();

   if (_stricmp(method, "BlockMessage") == 0)
   {
      if (module->CurrentMessage()) module->CurrentMessage()->flags |= kSMF_MsgBlock;
      sq_pushnull(vm); return 1;
   }
   if (_stricmp(method, "Reply") == 0 || _stricmp(method, "ReplyWithObj") == 0)
   {
      if (module->CurrentReply())
         *module->CurrentReply() = ArgumentParm(vm, 0);
      sq_pushnull(vm); return 1;
   }
   if (_stricmp(method, "SendMessage") == 0 || _stricmp(method, "PostMessage") == 0)
   {
      const ObjID to = ArgumentObject(vm, 0, manager);
      const std::string name = ArgumentString(vm, 1);
      cMultiParm data = ArgumentParm(vm, 2), data2 = ArgumentParm(vm, 3), data3 = ArgumentParm(vm, 4);
      if (_stricmp(method, "SendMessage") == 0)
      {
         cMultiParm result = manager->SendMessage2(self, to, name.c_str(), data, data2, data3);
         PushMultiParm(vm, result);
      }
      else
      {
         manager->PostMessage2(self, to, name.c_str(), data, data2, data3, kSMF_MsgPostToOwner);
         sq_pushnull(vm);
      }
      return 1;
   }
   if (_stricmp(method, "SetOneShotTimer") == 0)
   {
      ObjID to = self;
      SQInteger nameArg = 0, timeArg = 1, dataArg = 2;
      if (ArgumentCount(vm) >= 3 && sq_gettype(vm, 4) == OT_ARRAY)
      {
         const SQInteger top = sq_gettop(vm);
         PushArgument(vm, 0);
         if (sq_gettype(vm, -1) == OT_INTEGER) { to = (ObjID)ArgumentInteger(vm, 0); nameArg = 1; timeArg = 2; dataArg = 3; }
         sq_settop(vm, top);
      }
      const std::string name = ArgumentString(vm, nameArg);
      const ulong milliseconds = (ulong)(ArgumentFloat(vm, timeArg) * 1000.0f);
      cMultiParm data = ArgumentParm(vm, dataArg);
      sq_pushinteger(vm, (SQInteger)manager->SetTimedMessage2(to, name.c_str(), milliseconds, kSTM_OneShot, data));
      return 1;
   }
   if (_stricmp(method, "KillTimer") == 0)
   {
      manager->KillTimedMessage((tScrTimer)ArgumentInteger(vm, 0));
      sq_pushnull(vm); return 1;
   }
   if (_stricmp(method, "GetTime") == 0)
   {
      const sScrMsg* message = module->CurrentMessage();
      sq_pushfloat(vm, message ? message->time / 1000.0f : 0.0f); return 1;
   }
   if (_stricmp(method, "IsDataSet") == 0 || _stricmp(method, "GetData") == 0 ||
       _stricmp(method, "SetData") == 0 || _stricmp(method, "ClearData") == 0)
   {
      const std::string name = ArgumentString(vm, 0);
      sScrDatumTag tag = { self, module->CurrentClass(), name.c_str() };
      cMultiParm value;
      if (_stricmp(method, "IsDataSet") == 0)
         sq_pushbool(vm, manager->IsScriptDataSet(&tag) ? SQTrue : SQFalse);
      else if (_stricmp(method, "GetData") == 0)
      {
         manager->GetScriptData(&tag, &value); PushMultiParm(vm, value);
      }
      else if (_stricmp(method, "SetData") == 0)
      {
         value = ArgumentParm(vm, 1); manager->SetScriptData(&tag, &value); PushMultiParm(vm, value);
      }
      else
      {
         manager->ClearScriptData(&tag, &value); PushMultiParm(vm, value);
      }
      return 1;
   }
   if (_stricmp(method, "HasProperty") == 0 || _stricmp(method, "GetProperty") == 0 ||
       _stricmp(method, "SetProperty") == 0)
   {
      IUnknown* unknown = manager->GetService(&IID_IPropertyScriptService);
      IPropertyScriptService* property = (IPropertyScriptService*)unknown;
      if (!property) { sq_pushnull(vm); return 1; }
      const std::string prop = ArgumentString(vm, 0);
      if (_stricmp(method, "HasProperty") == 0)
         sq_pushbool(vm, property->Possessed(object(self), prop.c_str()) ? SQTrue : SQFalse);
      else if (_stricmp(method, "GetProperty") == 0)
      {
         const std::string field = ArgumentString(vm, 1);
         cMultiParm value = property->Get(object(self), prop.c_str(), field.empty() ? NULL : field.c_str());
         PushMultiParm(vm, value);
      }
      else
      {
         HRESULT result;
         if (ArgumentCount(vm) >= 3)
         {
            const std::string field = ArgumentString(vm, 1);
            cMultiParm value = ArgumentParm(vm, 2);
            result = property->Set(object(self), prop.c_str(), field.c_str(), value);
         }
         else
         {
            cMultiParm value = ArgumentParm(vm, 1);
            result = property->Set(object(self), prop.c_str(), value);
         }
         sq_pushbool(vm, SUCCEEDED(result) ? SQTrue : SQFalse);
      }
      property->Release(); return 1;
   }
   if (_stricmp(method, "ObjID") == 0)
   {
      IUnknown* unknown = manager->GetService(&IID_IObjectScriptService);
      IObjectScriptService* objectService = (IObjectScriptService*)unknown;
      const std::string name = ArgumentString(vm, 0);
      const ObjID result = objectService ? (ObjID)objectService->Named(name.c_str()) : 0;
      if (objectService) objectService->Release();
      sq_pushinteger(vm, result); return 1;
   }
   if (_stricmp(method, "linkkind") == 0 || _stricmp(method, "LinkDest") == 0 ||
       _stricmp(method, "LinkSource") == 0)
   {
      IUnknown* unknown = manager->GetService(&IID_ILinkToolsScriptService);
      ILinkToolsScriptService* links = (ILinkToolsScriptService*)unknown;
      SQInteger result = 0;
      if (links && _stricmp(method, "linkkind") == 0)
         result = links->LinkKindNamed(ArgumentString(vm, 0).c_str());
      else if (links)
      {
         sLink linkData;
         if (SUCCEEDED(links->LinkGet((LinkID)ArgumentInteger(vm, 0), linkData)))
            result = _stricmp(method, "LinkDest") == 0 ? linkData.dest : linkData.source;
      }
      if (links) links->Release();
      sq_pushinteger(vm, result); return 1;
   }
   sq_pushnull(vm);
   return 1;
}

static void PushNullResultSlot(HSQUIRRELVM vm)
{
   sq_pushstring(vm, "value", -1);
   sq_pushnull(vm);
   sq_newslot(vm, -3, SQFalse);
}

static SQInteger CallVersion(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(
      &IID_IVersionScriptService);
   IVersionScriptService* service = (IVersionScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }

   sq_newtable(vm);
   PushNullResultSlot(vm);
   BeginOutputs(vm);
   if (_stricmp(method, "GetAppName") == 0)
   {
      cScrStr value;
      service->GetAppName(ArgumentInteger(vm, 0) != 0, value);
      AppendStringOutput(vm, 1, value);
   }
   else if (_stricmp(method, "GetVersion") == 0)
   {
      int major = 0, minor = 0;
      service->GetVersion(major, minor);
      AppendIntegerOutput(vm, 0, major);
      AppendIntegerOutput(vm, 1, minor);
   }
   else if (_stricmp(method, "IsEditor") == 0)
   {
      // This method has no output arguments, so return it directly.
      sq_settop(vm, sq_gettop(vm) - 2);
      sq_settop(vm, sq_gettop(vm) - 1);
      sq_pushinteger(vm, service->IsEditor());
      service->Release();
      return 1;
   }
   else if (_stricmp(method, "GetGame") == 0 ||
            _stricmp(method, "GetGamsys") == 0 ||
            _stricmp(method, "GetMap") == 0 ||
            _stricmp(method, "GetCurrentFM") == 0 ||
            _stricmp(method, "GetCurrentFMPath") == 0)
   {
      cScrStr value;
      HRESULT result = S_OK;
      if (_stricmp(method, "GetGame") == 0) service->GetGame(value);
      else if (_stricmp(method, "GetGamsys") == 0) service->GetGamsys(value);
      else if (_stricmp(method, "GetMap") == 0) service->GetMap(value);
      else if (_stricmp(method, "GetCurrentFM") == 0)
         result = service->GetCurrentFM(value);
      else
         result = service->GetCurrentFMPath(value);
      AppendStringOutput(vm, 0, value);
      // Replace the null result for HRESULT-returning FM queries.
      if (_stricmp(method, "GetCurrentFM") == 0 ||
          _stricmp(method, "GetCurrentFMPath") == 0)
      {
         sq_pushstring(vm, "value", -1);
         sq_pushinteger(vm, result);
         sq_set(vm, -5);
      }
   }
   else if (_stricmp(method, "FMizeRelativePath") == 0 ||
            _stricmp(method, "FMizePath") == 0)
   {
      cScrStr value;
      if (_stricmp(method, "FMizeRelativePath") == 0)
         service->FMizeRelativePath(ArgumentString(vm, 0).c_str(), value);
      else
         service->FMizePath(ArgumentString(vm, 0).c_str(), value);
      AppendStringOutput(vm, 1, value);
   }
   else if (_stricmp(method, "GetModPath") == 0)
   {
      cScrStr value;
      const HRESULT result = service->GetModPath(
         value, (int)ArgumentInteger(vm, 1, -1));
      AppendStringOutput(vm, 0, value);
      sq_pushstring(vm, "value", -1);
      sq_pushinteger(vm, result);
      sq_set(vm, -5);
   }
   EndOutputs(vm);
   service->Release();
   return 1;
}

static SQInteger CallEngine(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(
      &IID_IEngineScriptService);
   IEngineScriptService* service = (IEngineScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }

   if (_stricmp(method, "ConfigIsDefined") == 0)
      sq_pushbool(vm, service->ConfigIsDefined(
         ArgumentString(vm, 0).c_str()) ? SQTrue : SQFalse);
   else if (_stricmp(method, "ConfigGetInt") == 0)
   {
      int value = (int)ArgumentInteger(vm, 1);
      const BOOL result = service->ConfigGetInt(
         ArgumentString(vm, 0).c_str(), value);
      sq_newtable(vm); SetSlotBool(vm, -1, "value", result != FALSE);
      BeginOutputs(vm); AppendIntegerOutput(vm, 1, value); EndOutputs(vm);
   }
   else if (_stricmp(method, "ConfigGetFloat") == 0)
   {
      float value = ArgumentFloat(vm, 1);
      const BOOL result = service->ConfigGetFloat(
         ArgumentString(vm, 0).c_str(), value);
      sq_newtable(vm); SetSlotBool(vm, -1, "value", result != FALSE);
      BeginOutputs(vm); AppendFloatOutput(vm, 1, value); EndOutputs(vm);
   }
   else if (_stricmp(method, "ConfigGetRaw") == 0)
   {
      cScrStr value;
      const BOOL result = service->ConfigGetRaw(
         ArgumentString(vm, 0).c_str(), value);
      sq_newtable(vm); SetSlotBool(vm, -1, "value", result != FALSE);
      BeginOutputs(vm); AppendStringOutput(vm, 1, value); EndOutputs(vm);
   }
   else if (_stricmp(method, "BindingGetFloat") == 0)
      sq_pushfloat(vm, service->BindingGetFloat(
         ArgumentString(vm, 0).c_str()));
   else if (_stricmp(method, "FindFileInPath") == 0)
   {
      cScrStr value;
      const BOOL result = service->FindFileInPath(
         ArgumentString(vm, 0).c_str(), ArgumentString(vm, 1).c_str(), value);
      sq_newtable(vm); SetSlotBool(vm, -1, "value", result != FALSE);
      BeginOutputs(vm); AppendStringOutput(vm, 2, value); EndOutputs(vm);
   }
   else if (_stricmp(method, "IsRunningDX6") == 0)
      sq_pushbool(vm, service->IsRunningDX6() ? SQTrue : SQFalse);
   else if (_stricmp(method, "GetCanvasSize") == 0)
   {
      int width = 0, height = 0;
      service->GetCanvasSize(width, height);
      sq_newtable(vm); PushNullResultSlot(vm); BeginOutputs(vm);
      AppendIntegerOutput(vm, 0, width);
      AppendIntegerOutput(vm, 1, height);
      EndOutputs(vm);
   }
   else if (_stricmp(method, "GetAspectRatio") == 0)
      sq_pushfloat(vm, service->GetAspectRatio());
   else if (_stricmp(method, "GetFog") == 0)
   {
      int red = 0, green = 0, blue = 0; float distance = 0;
      service->GetFog(red, green, blue, distance);
      sq_newtable(vm); PushNullResultSlot(vm); BeginOutputs(vm);
      AppendIntegerOutput(vm, 0, red); AppendIntegerOutput(vm, 1, green);
      AppendIntegerOutput(vm, 2, blue); AppendFloatOutput(vm, 3, distance);
      EndOutputs(vm);
   }
   else if (_stricmp(method, "SetFog") == 0)
   {
      service->SetFog((int)ArgumentInteger(vm, 0),
         (int)ArgumentInteger(vm, 1), (int)ArgumentInteger(vm, 2),
         ArgumentFloat(vm, 3));
      sq_pushnull(vm);
   }
   else if (_stricmp(method, "GetFogZone") == 0)
   {
      int red = 0, green = 0, blue = 0; float distance = 0;
      service->GetFogZone((int)ArgumentInteger(vm, 0), red, green, blue,
                          distance);
      sq_newtable(vm); PushNullResultSlot(vm); BeginOutputs(vm);
      AppendIntegerOutput(vm, 1, red); AppendIntegerOutput(vm, 2, green);
      AppendIntegerOutput(vm, 3, blue); AppendFloatOutput(vm, 4, distance);
      EndOutputs(vm);
   }
   else if (_stricmp(method, "SetFogZone") == 0)
   {
      service->SetFogZone((int)ArgumentInteger(vm, 0),
         (int)ArgumentInteger(vm, 1), (int)ArgumentInteger(vm, 2),
         (int)ArgumentInteger(vm, 3), ArgumentFloat(vm, 4));
      sq_pushnull(vm);
   }
   else if (_stricmp(method, "GetWeather") == 0)
   {
      int type = 0; float frequency = 0, speed = 0, visibility = 0;
      float radius = 0, alpha = 0, brightness = 0, snowJitter = 0;
      float rainLength = 0, splashFrequency = 0, splashRadius = 0;
      float splashHeight = 0, splashDuration = 0;
      cScrStr texture; cScrVec wind;
      service->GetWeather(type, frequency, speed, visibility, radius, alpha,
         brightness, snowJitter, rainLength, splashFrequency, splashRadius,
         splashHeight, splashDuration, texture, wind);
      sq_newtable(vm); PushNullResultSlot(vm); BeginOutputs(vm);
      AppendIntegerOutput(vm, 0, type);
      AppendFloatOutput(vm, 1, frequency); AppendFloatOutput(vm, 2, speed);
      AppendFloatOutput(vm, 3, visibility); AppendFloatOutput(vm, 4, radius);
      AppendFloatOutput(vm, 5, alpha); AppendFloatOutput(vm, 6, brightness);
      AppendFloatOutput(vm, 7, snowJitter); AppendFloatOutput(vm, 8, rainLength);
      AppendFloatOutput(vm, 9, splashFrequency); AppendFloatOutput(vm, 10, splashRadius);
      AppendFloatOutput(vm, 11, splashHeight); AppendFloatOutput(vm, 12, splashDuration);
      AppendStringOutput(vm, 13, texture); AppendVectorOutput(vm, 14, wind);
      EndOutputs(vm);
   }
   else if (_stricmp(method, "SetWeather") == 0)
   {
      const cScrVec wind(ArgumentVector(vm, 14));
      service->SetWeather((int)ArgumentInteger(vm, 0), ArgumentFloat(vm, 1),
         ArgumentFloat(vm, 2), ArgumentFloat(vm, 3), ArgumentFloat(vm, 4),
         ArgumentFloat(vm, 5), ArgumentFloat(vm, 6), ArgumentFloat(vm, 7),
         ArgumentFloat(vm, 8), ArgumentFloat(vm, 9), ArgumentFloat(vm, 10),
         ArgumentFloat(vm, 11), ArgumentFloat(vm, 12),
         ArgumentString(vm, 13).c_str(), wind);
      sq_pushnull(vm);
   }
   else if (_stricmp(method, "PortalRaycast") == 0)
   {
      cScrVec hit;
      const BOOL result = service->PortalRaycast(
         cScrVec(ArgumentVector(vm, 0)), cScrVec(ArgumentVector(vm, 1)), hit);
      sq_newtable(vm); SetSlotBool(vm, -1, "value", result != FALSE);
      BeginOutputs(vm); AppendVectorOutput(vm, 2, hit); EndOutputs(vm);
   }
   else if (_stricmp(method, "ObjRaycast") == 0)
   {
      cScrVec hit; object hitObject;
      const int result = service->ObjRaycast(
         cScrVec(ArgumentVector(vm, 0)), cScrVec(ArgumentVector(vm, 1)),
         hit, hitObject, (int)ArgumentInteger(vm, 4),
         (int)ArgumentInteger(vm, 5),
         object(ArgumentObject(vm, 6, gSquirrelModule->Manager())),
         object(ArgumentObject(vm, 7, gSquirrelModule->Manager())));
      sq_newtable(vm); SetSlotInteger(vm, -1, "value", result);
      BeginOutputs(vm); AppendVectorOutput(vm, 2, hit);
      AppendIntegerOutput(vm, 3, (ObjID)hitObject); EndOutputs(vm);
   }
   else if (_stricmp(method, "SetEnvMapZone") == 0)
   {
      service->SetEnvMapZone((int)ArgumentInteger(vm, 0),
                             ArgumentString(vm, 1).c_str());
      sq_pushnull(vm);
   }
   else
      sq_pushnull(vm);

   service->Release();
   return 1;
}

static SQInteger CallProperty(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_IPropertyScriptService);
   IPropertyScriptService* service = (IPropertyScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   const object obj(ArgumentObject(vm, 0, gSquirrelModule->Manager()));
   const std::string prop = ArgumentString(vm, 1);
   if (_stricmp(method, "Get") == 0)
   {
      const std::string field = ArgumentString(vm, 2);
      cMultiParm value = service->Get(obj, prop.c_str(), field.empty() ? NULL : field.c_str());
      PushMultiParm(vm, value);
   }
   else if (_stricmp(method, "Possessed") == 0)
      sq_pushbool(vm, service->Possessed(obj, prop.c_str()) ? SQTrue : SQFalse);
   else if (_stricmp(method, "Add") == 0)
      sq_pushinteger(vm, service->Add(obj, prop.c_str()));
   else if (_stricmp(method, "Remove") == 0)
      sq_pushinteger(vm, service->Remove(obj, prop.c_str()));
   else if (_stricmp(method, "CopyFrom") == 0)
      sq_pushinteger(vm, service->CopyFrom(obj, prop.c_str(),
         object(ArgumentObject(vm, 2, gSquirrelModule->Manager()))));
   else if (_stricmp(method, "Set") == 0 || _stricmp(method, "SetSimple") == 0 ||
            _stricmp(method, "SetLocal") == 0)
   {
      HRESULT result;
      if (ArgumentCount(vm) >= 4)
      {
         const std::string field = ArgumentString(vm, 2);
         cMultiParm value = ArgumentParm(vm, 3);
         result = _stricmp(method, "SetLocal") == 0 ?
            service->SetLocal(obj, prop.c_str(), field.c_str(), value) :
            service->Set(obj, prop.c_str(), field.c_str(), value);
      }
      else
      {
         cMultiParm value = ArgumentParm(vm, 2);
         result = service->Set(obj, prop.c_str(), value);
      }
      sq_pushinteger(vm, result);
   }
   else
      sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallObject(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_IObjectScriptService);
   IObjectScriptService* service = (IObjectScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   const object obj(ArgumentObject(vm, 0, gSquirrelModule->Manager()));
   if (_stricmp(method, "Named") == 0)
      sq_pushinteger(vm, (ObjID)service->Named(ArgumentString(vm, 0).c_str()));
   else if (_stricmp(method, "Archetype") == 0)
   {
      ITraitManager* traits = AppGetObj(ITraitManager);
      sq_pushinteger(vm, traits ? traits->GetArchetype((ObjID)obj) : 0);
      SafeRelease(traits);
   }
   else if (_stricmp(method, "Exists") == 0)
      sq_pushbool(vm, service->Exists(obj) ? SQTrue : SQFalse);
   else if (_stricmp(method, "Create") == 0)
      sq_pushinteger(vm, (ObjID)service->Create(obj));
   else if (_stricmp(method, "BeginCreate") == 0)
      sq_pushinteger(vm, (ObjID)service->BeginCreate(obj));
   else if (_stricmp(method, "EndCreate") == 0)
      sq_pushinteger(vm, service->EndCreate(obj));
   else if (_stricmp(method, "Destroy") == 0)
      sq_pushinteger(vm, service->Destroy(obj));
   else if (_stricmp(method, "GetName") == 0)
   {
      cScrStr value = service->GetName(obj); sq_pushstring(vm, value, -1);
   }
   else if (_stricmp(method, "SetName") == 0)
      sq_pushinteger(vm, service->SetName(obj, ArgumentString(vm, 1).c_str()));
   else if (_stricmp(method, "AddMetaProperty") == 0)
      sq_pushinteger(vm, service->AddMetaProperty(obj,
         object(ArgumentObject(vm, 1, gSquirrelModule->Manager()))));
   else if (_stricmp(method, "RemoveMetaProperty") == 0)
      sq_pushinteger(vm, service->RemoveMetaProperty(obj,
         object(ArgumentObject(vm, 1, gSquirrelModule->Manager()))));
   else if (_stricmp(method, "HasMetaProperty") == 0)
      sq_pushbool(vm, service->HasMetaProperty(obj,
         object(ArgumentObject(vm, 1, gSquirrelModule->Manager()))) ? SQTrue : SQFalse);
   else if (_stricmp(method, "InheritsFrom") == 0)
      sq_pushbool(vm, service->InheritsFrom(obj,
         object(ArgumentObject(vm, 1, gSquirrelModule->Manager()))) ? SQTrue : SQFalse);
   else if (_stricmp(method, "Position") == 0)
   {
      cScrVec value = service->Position(obj); PushVector(vm, value);
   }
   else if (_stricmp(method, "Facing") == 0)
   {
      cScrVec value = service->Facing(obj); PushVector(vm, value);
   }
   else if (_stricmp(method, "Teleport") == 0)
   {
      const mxs_vector position = ArgumentVector(vm, 1);
      const mxs_vector facing = ArgumentVector(vm, 2);
      sq_pushinteger(vm, service->Teleport(obj, position, facing,
         object(ArgumentObject(vm, 3, gSquirrelModule->Manager()))));
   }
   else if (_stricmp(method, "IsPositionValid") == 0)
      sq_pushbool(vm, service->IsPositionValid(obj) ? SQTrue : SQFalse);
   else if (_stricmp(method, "IsTransient") == 0)
      sq_pushbool(vm, service->IsTransient(obj) ? SQTrue : SQFalse);
   else if (_stricmp(method, "SetTransience") == 0)
      sq_pushinteger(vm, service->SetTransience(obj, ArgumentInteger(vm, 1) != 0));
   else if (_stricmp(method, "RenderedThisFrame") == 0)
      sq_pushbool(vm, service->RenderedThisFrame(obj) ? SQTrue : SQFalse);
   else if (_stricmp(method, "FindClosestObjectNamed") == 0)
      sq_pushinteger(vm, (ObjID)service->FindClosestObjectNamed(
         (ObjID)obj, ArgumentString(vm, 1).c_str()));
   else if (_stricmp(method, "AddMetaPropertyToMany") == 0)
   {
      cScrStr targets(ArgumentString(vm, 1).c_str());
      sq_pushinteger(vm, service->AddMetaPropertyToMany(obj, targets));
   }
   else if (_stricmp(method, "RemoveMetaPropertyFromMany") == 0)
   {
      cScrStr targets(ArgumentString(vm, 1).c_str());
      sq_pushinteger(vm, service->RemoveMetaPropertyFromMany(obj, targets));
   }
   else if (_stricmp(method, "ObjectToWorld") == 0 ||
            _stricmp(method, "WorldToObject") == 0)
   {
      const cScrVec origin = service->Position(obj);
      const cScrVec angles = service->Facing(obj);
      const mxs_angvec facing = {
         (mxs_ang)(((long)(angles.x * MX_ANG_PI)) / 180),
         (mxs_ang)(((long)(angles.y * MX_ANG_PI)) / 180),
         (mxs_ang)(((long)(angles.z * MX_ANG_PI)) / 180) };
      mxs_matrix rotation; mxs_vector value = ArgumentVector(vm, 1), result;
      mx_ang2mat(&rotation, &facing);
      if (_stricmp(method, "ObjectToWorld") == 0)
      {
         mx_mat_mul_vec(&result, &rotation, &value);
         mx_addeq_vec(&result, &origin);
      }
      else
      {
         mx_subeq_vec(&value, &origin);
         mx_mat_tmul_vec(&result, &rotation, &value);
      }
      PushVector(vm, result);
   }
   else if (_stricmp(method, "CalcRelTransform") == 0)
   {
      const object child(ArgumentObject(vm, 1, gSquirrelModule->Manager()));
      const cScrVec parentPos = service->Position(obj);
      const cScrVec parentFacing = service->Facing(obj);
      const cScrVec childPos = service->Position(child);
      const cScrVec childFacing = service->Facing(child);
      const mxs_angvec facing = {
         (mxs_ang)(((long)(parentFacing.x * MX_ANG_PI)) / 180),
         (mxs_ang)(((long)(parentFacing.y * MX_ANG_PI)) / 180),
         (mxs_ang)(((long)(parentFacing.z * MX_ANG_PI)) / 180) };
      mxs_matrix rotation; mxs_vector relative = childPos;
      mx_ang2mat(&rotation, &facing);
      mx_subeq_vec(&relative, &parentPos);
      mx_mat_tmuleq_vec(&rotation, &relative);
      const mxs_vector relativeFacing = {
         childFacing.x - parentFacing.x,
         childFacing.y - parentFacing.y,
         childFacing.z - parentFacing.z };
      sq_newtable(vm); SetSlotBool(vm, -1, "value", true);
      BeginOutputs(vm); AppendVectorOutput(vm, 2, relative);
      AppendVectorOutput(vm, 3, relativeFacing); EndOutputs(vm);
   }
   else
      sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallLink(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_ILinkScriptService);
   ILinkScriptService* service = (ILinkScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   const linkkind kind = ArgumentLinkKind(vm, 0, gSquirrelModule->Manager());
   const object from(ArgumentObject(vm, 1, gSquirrelModule->Manager()));
   const object to(ArgumentObject(vm, 2, gSquirrelModule->Manager()));
   if (_stricmp(method, "Create") == 0)
      sq_pushinteger(vm, (LinkID)service->Create(kind, from, to));
   else if (_stricmp(method, "Destroy") == 0)
      sq_pushinteger(vm, service->Destroy(link((LinkID)ArgumentInteger(vm, 0))));
   else if (_stricmp(method, "AnyExist") == 0)
      sq_pushbool(vm, service->AnyExist(kind, from, to) ? SQTrue : SQFalse);
   else if (_stricmp(method, "GetOne") == 0)
      sq_pushinteger(vm, (LinkID)service->GetOne(kind, from, to));
   else if (_stricmp(method, "GetAll") == 0)
   {
      linkset result = service->GetAll(kind, from, to);
      PushLinkSet(vm, result);
   }
   else if (_stricmp(method, "GetAllInherited") == 0)
   {
      linkset result = service->GetAllInherited(kind, from, to);
      PushLinkSet(vm, result);
   }
   else if (_stricmp(method, "GetAllInheritedSingle") == 0)
   {
      linkset result = service->GetAllInheritedSingle(kind, from, to);
      PushLinkSet(vm, result);
   }
   else if (_stricmp(method, "BroadcastOnAllLinks") == 0 ||
            _stricmp(method, "BroadcastOnAllLinksData") == 0)
   {
      const object self(ArgumentObject(vm, 0, gSquirrelModule->Manager()));
      const std::string message = ArgumentString(vm, 1);
      const linkkind recipients = ArgumentLinkKind(vm, 2, gSquirrelModule->Manager());
      if (ArgumentCount(vm) >= 4)
      {
         cMultiParm data = ArgumentParm(vm, 3);
         sq_pushinteger(vm, service->BroadcastOnAllLinks(self, message.c_str(), recipients, data));
      }
      else
         sq_pushinteger(vm, service->BroadcastOnAllLinks(self, message.c_str(), recipients));
   }
   else if (_stricmp(method, "CreateMany") == 0 || _stricmp(method, "DestroyMany") == 0)
   {
      cScrStr fromSet(ArgumentString(vm, 1).c_str());
      cScrStr toSet(ArgumentString(vm, 2).c_str());
      HRESULT result = _stricmp(method, "CreateMany") == 0 ?
         service->CreateMany(kind, fromSet, toSet) :
         service->DestroyMany(kind, fromSet, toSet);
      sq_pushinteger(vm, result);
   }
   else
      sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallLinkTools(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_ILinkToolsScriptService);
   ILinkToolsScriptService* service = (ILinkToolsScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   if (_stricmp(method, "LinkKindNamed") == 0)
      sq_pushinteger(vm, service->LinkKindNamed(ArgumentString(vm, 0).c_str()));
   else if (_stricmp(method, "LinkKindName") == 0)
   {
      cScrStr name = service->LinkKindName((int)ArgumentInteger(vm, 0));
      sq_pushstring(vm, name, -1);
   }
   else if (_stricmp(method, "LinkGet") == 0)
   {
      sLink value;
      const LinkID id = (LinkID)ArgumentInteger(vm, 0);
      if (FAILED(service->LinkGet(id, value)))
         sq_pushnull(vm);
      else
      {
         sq_newtable(vm);
         SetSlotString(vm, -1, "__sq_type", "link");
         SetSlotInteger(vm, -1, "id", id);
         SetSlotInteger(vm, -1, "source", value.source);
         SetSlotInteger(vm, -1, "dest", value.dest);
         SetSlotInteger(vm, -1, "flavor", LINKID_RELATION(id));
      }
   }
   else if (_stricmp(method, "LinkGetData") == 0)
   {
      const std::string field = ArgumentString(vm, 1);
      cMultiParm value = service->LinkGetData((LinkID)ArgumentInteger(vm, 0),
                                               field.empty() ? NULL : field.c_str());
      PushMultiParm(vm, value);
   }
   else if (_stricmp(method, "LinkSetData") == 0)
   {
      const std::string field = ArgumentString(vm, 1);
      cMultiParm value = ArgumentParm(vm, 2);
      sq_pushinteger(vm, service->LinkSetData((LinkID)ArgumentInteger(vm, 0),
                                              field.empty() ? NULL : field.c_str(), value));
   }
   else
      sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallPhysics(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_IPhysicsScriptService);
   IPhysicsScriptService* service = (IPhysicsScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   const object obj(ArgumentObject(vm, 0, gSquirrelModule->Manager()));
   if (_stricmp(method, "SubscribeMsg") == 0)
      sq_pushinteger(vm, service->SubscribeMsg(obj, (int)ArgumentInteger(vm, 1)));
   else if (_stricmp(method, "UnsubscribeMsg") == 0)
      sq_pushinteger(vm, service->UnsubscribeMsg(obj, (int)ArgumentInteger(vm, 1)));
   else if (_stricmp(method, "LaunchProjectile") == 0)
   {
      const cScrVec velocity(ArgumentVector(vm, 4));
      sq_pushinteger(vm, (ObjID)service->LaunchProjectile(
         obj, object(ArgumentObject(vm, 1, gSquirrelModule->Manager())),
         ArgumentFloat(vm, 2), (int)ArgumentInteger(vm, 3), velocity));
   }
   else if (_stricmp(method, "HasPhysics") == 0)
      sq_pushbool(vm, service->HasPhysics(obj) ? SQTrue : SQFalse);
   else if (_stricmp(method, "IsSphere") == 0)
      sq_pushbool(vm, service->IsSphere(obj) ? SQTrue : SQFalse);
   else if (_stricmp(method, "IsOBB") == 0)
      sq_pushbool(vm, service->IsOBB(obj) ? SQTrue : SQFalse);
   else if (_stricmp(method, "GetGravity") == 0)
      sq_pushfloat(vm, service->GetGravity(obj));
   else if (_stricmp(method, "SetGravity") == 0)
      sq_pushinteger(vm, service->SetGravity(obj, ArgumentFloat(vm, 1)));
   else if (_stricmp(method, "Activate") == 0)
      sq_pushinteger(vm, service->Activate(obj));
   else if (_stricmp(method, "ValidPos") == 0)
      sq_pushbool(vm, service->ValidPos(obj) ? SQTrue : SQFalse);
   else if (_stricmp(method, "DeregisterModel") == 0)
      sq_pushinteger(vm, service->DeregisterModel(obj));
   else if (_stricmp(method, "SetVelocity") == 0)
   {
      const mxs_vector value = ArgumentVector(vm, 1);
      sq_pushinteger(vm, service->SetVelocity(obj, value));
   }
   else if (_stricmp(method, "GetVelocity") == 0)
   {
      cScrVec value;
      const HRESULT result = service->GetVelocity(obj, value);
      sq_newtable(vm); SetSlotInteger(vm, -1, "value", result);
      BeginOutputs(vm); AppendVectorOutput(vm, 1, value); EndOutputs(vm);
   }
   else if (_stricmp(method, "ControlVelocity") == 0)
   {
      const mxs_vector value = ArgumentVector(vm, 1);
      sq_pushinteger(vm, service->ControlVelocity(obj, value));
   }
   else if (_stricmp(method, "StopControlVelocity") == 0)
      sq_pushinteger(vm, service->StopControlVelocity(obj));
   else if (_stricmp(method, "ControlCurrentLocation") == 0)
      sq_pushinteger(vm, service->ControlCurrentLocation(obj));
   else if (_stricmp(method, "ControlCurrentRotation") == 0)
      sq_pushinteger(vm, service->ControlCurrentRotation(obj));
   else if (_stricmp(method, "ControlCurrentPosition") == 0)
      sq_pushinteger(vm, service->ControlCurrentPosition(obj));
   else if (_stricmp(method, "PlayerMotionSetOffset") == 0)
   {
      cScrVec value(ArgumentVector(vm, 1));
      service->PlayerMotionSetOffset((int)ArgumentInteger(vm, 0), value);
      sq_pushnull(vm);
   }
   else if (_stricmp(method, "IsRope") == 0)
      sq_pushbool(vm, service->IsRope(obj) ? SQTrue : SQFalse);
   else if (_stricmp(method, "GetClimbingObject") == 0)
   {
      object climbing;
      service->GetClimbingObject(obj, climbing);
      sq_newtable(vm); PushNullResultSlot(vm); BeginOutputs(vm);
      AppendIntegerOutput(vm, 1, (ObjID)climbing); EndOutputs(vm);
   }
   else
      sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallDoor(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_IDoorScriptService);
   IDoorScriptService* service = (IDoorScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   const object obj(ArgumentObject(vm, 0, gSquirrelModule->Manager()));
   if (_stricmp(method, "CloseDoor") == 0)
      sq_pushbool(vm, service->CloseDoor(obj) ? SQTrue : SQFalse);
   else if (_stricmp(method, "OpenDoor") == 0)
      sq_pushbool(vm, service->OpenDoor(obj) ? SQTrue : SQFalse);
   else if (_stricmp(method, "GetDoorState") == 0)
      sq_pushinteger(vm, service->GetDoorState(obj));
   else if (_stricmp(method, "ToggleDoor") == 0)
      sq_pushinteger(vm, service->ToggleDoor(obj));
   else if (_stricmp(method, "SetBlocking") == 0)
      sq_pushinteger(vm, service->SetBlocking(obj, ArgumentInteger(vm, 1) != 0));
   else if (_stricmp(method, "GetSoundBlocking") == 0)
      sq_pushbool(vm, service->GetSoundBlocking(obj) ? SQTrue : SQFalse);
   else
      sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallLocked(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_ILockedScriptService);
   ILockedScriptService* service = (ILockedScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   if (_stricmp(method, "IsLocked") == 0)
   {
      const object obj(ArgumentObject(vm, 0, gSquirrelModule->Manager()));
      sq_pushbool(vm, service->IsLocked(obj) ? SQTrue : SQFalse);
   }
   else
      sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallData(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_IDataScriptService);
   IDataScriptService* service = (IDataScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   if (_stricmp(method, "GetString") == 0)
   {
      cScrStr value = service->GetString(ArgumentString(vm, 0).c_str(),
         ArgumentString(vm, 1).c_str(), ArgumentString(vm, 2).c_str(),
         ArgumentString(vm, 3, "strings").c_str());
      sq_pushstring(vm, value, -1);
   }
   else if (_stricmp(method, "GetObjString") == 0)
   {
      cScrStr value = service->GetObjString(
         ArgumentObject(vm, 0, gSquirrelModule->Manager()),
         ArgumentString(vm, 1).c_str());
      sq_pushstring(vm, value, -1);
   }
   else if (_stricmp(method, "DirectRand") == 0)
      sq_pushinteger(vm, service->DirectRand());
   else if (_stricmp(method, "RandInt") == 0)
      sq_pushinteger(vm, service->RandInt((int)ArgumentInteger(vm, 0),
                                          (int)ArgumentInteger(vm, 1)));
   else if (_stricmp(method, "RandFlt0to1") == 0)
      sq_pushfloat(vm, service->RandFlt0to1());
   else if (_stricmp(method, "RandFltNeg1to1") == 0)
      sq_pushfloat(vm, service->RandFltNeg1to1());
   else
      sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallDamage(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_IDamageScriptService);
   IDamageScriptService* service = (IDamageScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   const object victim(ArgumentObject(vm, 0, gSquirrelModule->Manager()));
   const object culprit(ArgumentObject(vm, 1, gSquirrelModule->Manager()));
   if (_stricmp(method, "Damage") == 0)
      sq_pushinteger(vm, service->Damage(victim, culprit,
         (int)ArgumentInteger(vm, 2), (int)ArgumentInteger(vm, 3)));
   else if (_stricmp(method, "Slay") == 0)
      sq_pushinteger(vm, service->Slay(victim, culprit));
   else if (_stricmp(method, "Resurrect") == 0)
      sq_pushinteger(vm, service->Resurrect(victim, culprit));
   else
      sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallContainer(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_IContainerScriptService);
   IContainerScriptService* service = (IContainerScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   const object first(ArgumentObject(vm, 0, gSquirrelModule->Manager()));
   const object second(ArgumentObject(vm, 1, gSquirrelModule->Manager()));
   if (_stricmp(method, "Add") == 0)
      sq_pushinteger(vm, service->Add(first, second, (int)ArgumentInteger(vm, 2),
                                      (int)ArgumentInteger(vm, 3, CTF_COMBINE)));
   else if (_stricmp(method, "Remove") == 0)
      sq_pushinteger(vm, service->Remove(first, second));
   else if (_stricmp(method, "MoveAllContents") == 0)
      sq_pushinteger(vm, service->MoveAllContents(first, second,
                                      (int)ArgumentInteger(vm, 2, CTF_COMBINE)));
   else if (_stricmp(method, "StackAdd") == 0)
      sq_pushinteger(vm, service->StackAdd(first, (int)ArgumentInteger(vm, 1)));
   else if (_stricmp(method, "IsHeld") == 0)
      sq_pushinteger(vm, service->IsHeld(first, second));
   else
      sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallQuest(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_IQuestScriptService);
   IQuestScriptService* service = (IQuestScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   const std::string name = ArgumentString(vm, 0);
   if (_stricmp(method, "SubscribeMsg") == 0)
      sq_pushbool(vm, service->SubscribeMsg(
         object(ArgumentObject(vm, 0, gSquirrelModule->Manager())),
         ArgumentString(vm, 1).c_str(),
         (eQuestDataType)ArgumentInteger(vm, 2, kQuestDataUnknown)) ? SQTrue : SQFalse);
   else if (_stricmp(method, "UnsubscribeMsg") == 0)
      sq_pushbool(vm, service->UnsubscribeMsg(
         object(ArgumentObject(vm, 0, gSquirrelModule->Manager())),
         ArgumentString(vm, 1).c_str()) ? SQTrue : SQFalse);
   else if (_stricmp(method, "Set") == 0)
      sq_pushinteger(vm, service->Set(name.c_str(), (int)ArgumentInteger(vm, 1),
                              (eQuestDataType)ArgumentInteger(vm, 2, kQuestDataMission)));
   else if (_stricmp(method, "Get") == 0)
      sq_pushinteger(vm, service->Get(name.c_str()));
   else if (_stricmp(method, "Exists") == 0)
      sq_pushbool(vm, service->Exists(name.c_str()) ? SQTrue : SQFalse);
   else if (_stricmp(method, "Delete") == 0)
      sq_pushbool(vm, service->Delete(name.c_str()) ? SQTrue : SQFalse);
   else if (_stricmp(method, "GetAllVars") == 0)
   {
      IQuestData* data = AppGetObj(IQuestData);
      sq_newtable(vm);
      if (data)
      {
         IQuestDataIter* iterator = data->Iter(
            (eQuestDataType)ArgumentInteger(vm, 0, kQuestDataMission));
         while (iterator && !iterator->Done())
         {
            sq_pushstring(vm, iterator->Name(), -1);
            sq_pushinteger(vm, iterator->Value());
            sq_newslot(vm, -3, SQFalse);
            iterator->Next();
         }
         SafeRelease(iterator);
         data->Release();
      }
   }
   else if (_stricmp(method, "BinSet") == 0 ||
            _stricmp(method, "BinSetTable") == 0)
   {
      const bool tableValue = _stricmp(method, "BinSetTable") == 0;
      const std::string key = tableValue ?
         ArgumentString(vm, 1) : "";
      const SQInteger valueIndex = tableValue ? 2 : 1;
      const SQInteger top = sq_gettop(vm);
      HSQOBJECT value; sq_resetobject(&value);
      if (PushArgument(vm, valueIndex) &&
          SQ_SUCCEEDED(sq_getstackobj(vm, -1, &value)))
      {
         for (size_t i = 0; i < gQuestBinaryValues.size(); ++i)
            if (_stricmp(gQuestBinaryValues[i].name.c_str(), name.c_str()) == 0 &&
                gQuestBinaryValues[i].key == key)
            {
               sq_release(vm, &gQuestBinaryValues[i].value);
               gQuestBinaryValues.erase(gQuestBinaryValues.begin() + i);
               break;
            }
         cQuestBinaryValue entry;
         entry.name = name; entry.key = key; entry.value = value;
         sq_addref(vm, &entry.value);
         gQuestBinaryValues.push_back(entry);
      }
      sq_settop(vm, top);
      sq_pushinteger(vm, S_OK);
   }
   else if (_stricmp(method, "BinGet") == 0 ||
            _stricmp(method, "BinGetTable") == 0)
   {
      const std::string key = _stricmp(method, "BinGetTable") == 0 ?
         ArgumentString(vm, 1) : "";
      bool found = false;
      for (size_t i = 0; i < gQuestBinaryValues.size(); ++i)
         if (_stricmp(gQuestBinaryValues[i].name.c_str(), name.c_str()) == 0 &&
             gQuestBinaryValues[i].key == key)
         {
            sq_pushobject(vm, gQuestBinaryValues[i].value);
            found = true; break;
         }
      if (!found) sq_pushnull(vm);
   }
   else if (_stricmp(method, "BinExists") == 0)
   {
      bool found = false;
      for (size_t i = 0; i < gQuestBinaryValues.size(); ++i)
         if (_stricmp(gQuestBinaryValues[i].name.c_str(), name.c_str()) == 0)
         { found = true; break; }
      sq_pushbool(vm, found ? SQTrue : SQFalse);
   }
   else if (_stricmp(method, "BinDelete") == 0)
   {
      bool removed = false;
      for (size_t i = gQuestBinaryValues.size(); i-- > 0; )
         if (_stricmp(gQuestBinaryValues[i].name.c_str(), name.c_str()) == 0)
         {
            sq_release(vm, &gQuestBinaryValues[i].value);
            gQuestBinaryValues.erase(gQuestBinaryValues.begin() + i);
            removed = true;
         }
      sq_pushbool(vm, removed ? SQTrue : SQFalse);
   }
   else
      sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallLight(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_ILightScriptService);
   ILightScriptService* service = (ILightScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   const object obj(ArgumentObject(vm, 0, gSquirrelModule->Manager()));
   if (_stricmp(method, "Set") == 0)
   {
      service->Set(obj, (int)ArgumentInteger(vm, 1), ArgumentFloat(vm, 2),
                   ArgumentFloat(vm, 3)); sq_pushnull(vm);
   }
   else if (_stricmp(method, "SetMode") == 0)
   {
      service->SetMode(obj, (int)ArgumentInteger(vm, 1)); sq_pushnull(vm);
   }
   else if (_stricmp(method, "Activate") == 0)
   {
      service->Activate(obj); sq_pushnull(vm);
   }
   else if (_stricmp(method, "Deactivate") == 0)
   {
      service->Deactivate(obj); sq_pushnull(vm);
   }
   else if (_stricmp(method, "Subscribe") == 0)
   {
      service->Subscribe(obj); sq_pushnull(vm);
   }
   else if (_stricmp(method, "Unsubscribe") == 0)
   {
      service->Unsubscribe(obj); sq_pushnull(vm);
   }
   else if (_stricmp(method, "GetMode") == 0)
      sq_pushinteger(vm, service->GetMode(obj));
   else
      sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallActReact(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_IActReactScriptService);
   IActReactScriptService* service = (IActReactScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   if (_stricmp(method, "GetReactionNamed") == 0)
      sq_pushinteger(vm, service->GetReactionNamed(ArgumentString(vm, 0).c_str()));
   else if (_stricmp(method, "GetReactionName") == 0)
   {
      cScrStr value = service->GetReactionName((int)ArgumentInteger(vm, 0));
      sq_pushstring(vm, value, -1);
   }
   else
   {
      SQInteger kind = ArgumentInteger(vm, 1);
      const SQInteger top = sq_gettop(vm);
      if (PushArgument(vm, 1) && sq_gettype(vm, -1) == OT_STRING)
      {
         kind = ArgumentObject(vm, 1, gSquirrelModule->Manager());
      }
      sq_settop(vm, top);
      if (_stricmp(method, "React") == 0)
      {
         SQInteger reaction = ArgumentInteger(vm, 0);
         const SQInteger reactionTop = sq_gettop(vm);
         if (PushArgument(vm, 0) && sq_gettype(vm, -1) == OT_STRING)
         {
            const SQChar* name = ""; sq_getstring(vm, -1, &name);
            reaction = service->GetReactionNamed(name);
         }
         sq_settop(vm, reactionTop);
         cMultiParm parms[8];
         for (int i = 0; i < 8; ++i) parms[i] = ArgumentParm(vm, i + 4);
         sq_pushinteger(vm, service->React(reaction_kind((int)reaction),
            ArgumentFloat(vm, 1), object(ArgumentObject(vm, 2, gSquirrelModule->Manager())),
            object(ArgumentObject(vm, 3, gSquirrelModule->Manager())),
            parms[0], parms[1], parms[2], parms[3], parms[4], parms[5], parms[6], parms[7]));
      }
      else if (_stricmp(method, "Stimulate") == 0)
         sq_pushinteger(vm, service->Stimulate(
            object(ArgumentObject(vm, 0, gSquirrelModule->Manager())), object((int)kind),
            ArgumentFloat(vm, 2), object(ArgumentObject(vm, 3, gSquirrelModule->Manager()))));
      else if (_stricmp(method, "SubscribeToStimulus") == 0)
         sq_pushinteger(vm, service->SubscribeToStimulus(
            object(ArgumentObject(vm, 0, gSquirrelModule->Manager())), object((int)kind)));
      else if (_stricmp(method, "UnsubscribeToStimulus") == 0)
         sq_pushinteger(vm, service->UnsubscribeToStimulus(
            object(ArgumentObject(vm, 0, gSquirrelModule->Manager())), object((int)kind)));
      else if (_stricmp(method, "BeginContact") == 0)
         sq_pushinteger(vm, service->BeginContact(
            object(ArgumentObject(vm, 0, gSquirrelModule->Manager())),
            object(ArgumentObject(vm, 1, gSquirrelModule->Manager()))));
      else if (_stricmp(method, "EndContact") == 0)
         sq_pushinteger(vm, service->EndContact(
            object(ArgumentObject(vm, 0, gSquirrelModule->Manager())),
            object(ArgumentObject(vm, 1, gSquirrelModule->Manager()))));
      else if (_stricmp(method, "SetSingleSensorContact") == 0)
         sq_pushinteger(vm, service->SetSingleSensorContact(
            object(ArgumentObject(vm, 0, gSquirrelModule->Manager())),
            object(ArgumentObject(vm, 1, gSquirrelModule->Manager()))));
      else
         sq_pushnull(vm);
   }
   service->Release();
   return 1;
}

static SQInteger CallAI(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_IAIScriptService);
   IAIScriptService* service = (IAIScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   const ObjID ai = ArgumentObject(vm, 0, gSquirrelModule->Manager());
   const object target(ArgumentObject(vm, 1, gSquirrelModule->Manager()));
   if (_stricmp(method, "MakeGotoObj") == 0 ||
       _stricmp(method, "MakeGotoObjLoc") == 0)
      sq_pushbool(vm, service->MakeGotoObjLoc(ai, target,
         (eAIScriptSpeed)ArgumentInteger(vm, 2, kNormalSpeed),
         (eAIActionPriority)ArgumentInteger(vm, 3, kNormalPriorityAction),
         ArgumentParm(vm, 4)) ? SQTrue : SQFalse);
   else if (_stricmp(method, "MakeFrobObj") == 0)
      sq_pushbool(vm, service->MakeFrobObj(ai, target,
         (eAIActionPriority)ArgumentInteger(vm, 2, kNormalPriorityAction),
         ArgumentParm(vm, 3)) ? SQTrue : SQFalse);
   else if (_stricmp(method, "MakeFrobObjWith") == 0)
      sq_pushbool(vm, service->MakeFrobObj(ai, target,
         object(ArgumentObject(vm, 2, gSquirrelModule->Manager())),
         (eAIActionPriority)ArgumentInteger(vm, 3, kNormalPriorityAction),
         ArgumentParm(vm, 4)) ? SQTrue : SQFalse);
   else if (_stricmp(method, "GetAlertLevel") == 0)
      sq_pushinteger(vm, service->GetAlertLevel(ai));
   else if (_stricmp(method, "SetMinimumAlert") == 0)
   {
      service->SetMinimumAlert(ai, (eAIScriptAlertLevel)ArgumentInteger(vm, 1));
      sq_pushnull(vm);
   }
   else if (_stricmp(method, "ClearGoals") == 0)
   {
      service->ClearGoals(ai); sq_pushnull(vm);
   }
   else if (_stricmp(method, "SetScriptFlags") == 0)
   {
      service->SetScriptFlags(ai, (int)ArgumentInteger(vm, 1)); sq_pushnull(vm);
   }
   else if (_stricmp(method, "ClearAlertness") == 0)
   {
      service->ClearAlertness(ai); sq_pushnull(vm);
   }
   else if (_stricmp(method, "Signal") == 0)
   {
      cScrStr signal(ArgumentString(vm, 1).c_str());
      service->Signal(ai, signal); sq_pushnull(vm);
   }
   else if (_stricmp(method, "StartConversation") == 0)
      sq_pushbool(vm, service->StartConversation(ai) ? SQTrue : SQFalse);
   else
      sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallDarkUI(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_IDarkUIScriptService);
   IDarkUIScriptService* service = (IDarkUIScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   if (_stricmp(method, "TextMessage") == 0)
      sq_pushinteger(vm, service->TextMessage(
         ArgumentString(vm, 0).c_str(), (int)ArgumentInteger(vm, 1),
         (int)ArgumentInteger(vm, 2, DEFAULT_TIMEOUT)));
   else if (_stricmp(method, "ReadBook") == 0)
      sq_pushinteger(vm, service->ReadBook(ArgumentString(vm, 0).c_str(),
                                           ArgumentString(vm, 1).c_str()));
   else if (_stricmp(method, "InvItem") == 0)
      sq_pushinteger(vm, (ObjID)service->InvItem());
   else if (_stricmp(method, "InvWeapon") == 0)
      sq_pushinteger(vm, (ObjID)service->InvWeapon());
   else if (_stricmp(method, "InvSelect") == 0)
      sq_pushinteger(vm, service->InvSelect(object(
         ArgumentObject(vm, 0, gSquirrelModule->Manager()))));
   else if (_stricmp(method, "IsCommandBound") == 0)
   {
      cScrStr command(ArgumentString(vm, 0).c_str());
      sq_pushbool(vm, service->IsCommandBound(command) ? SQTrue : SQFalse);
   }
   else if (_stricmp(method, "DescribeKeyBinding") == 0)
   {
      cScrStr command(ArgumentString(vm, 0).c_str());
      cScrStr value = service->DescribeKeyBinding(command);
      sq_pushstring(vm, value, -1);
   }
   else
      sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallDarkGame(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_IDarkGameScriptService);
   IDarkGameScriptService* service = (IDarkGameScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   if (_stricmp(method, "KillPlayer") == 0)
      sq_pushinteger(vm, service->KillPlayer());
   else if (_stricmp(method, "EndMission") == 0)
      sq_pushinteger(vm, service->EndMission());
   else if (_stricmp(method, "FadeToBlack") == 0)
      sq_pushinteger(vm, service->FadeToBlack(ArgumentFloat(vm, 0)));
   else if (_stricmp(method, "FoundObject") == 0)
      sq_pushinteger(vm, service->FoundObject(
         ArgumentObject(vm, 0, gSquirrelModule->Manager())));
   else if (_stricmp(method, "ConfigIsDefined") == 0)
      sq_pushbool(vm, service->ConfigIsDefined(
         ArgumentString(vm, 0).c_str()) ? SQTrue : SQFalse);
   else if (_stricmp(method, "ConfigGetInt") == 0)
   {
      int value = (int)ArgumentInteger(vm, 1);
      const BOOL result = service->ConfigGetInt(
         ArgumentString(vm, 0).c_str(), value);
      sq_newtable(vm); SetSlotBool(vm, -1, "value", result != FALSE);
      BeginOutputs(vm); AppendIntegerOutput(vm, 1, value); EndOutputs(vm);
   }
   else if (_stricmp(method, "ConfigGetFloat") == 0)
   {
      float value = ArgumentFloat(vm, 1);
      const BOOL result = service->ConfigGetFloat(
         ArgumentString(vm, 0).c_str(), value);
      sq_newtable(vm); SetSlotBool(vm, -1, "value", result != FALSE);
      BeginOutputs(vm); AppendFloatOutput(vm, 1, value); EndOutputs(vm);
   }
   else if (_stricmp(method, "BindingGetFloat") == 0)
      sq_pushfloat(vm, service->BindingGetFloat(
         ArgumentString(vm, 0).c_str()));
   else if (_stricmp(method, "GetAutomapLocationVisited") == 0)
      sq_pushbool(vm, service->GetAutomapLocationVisited(
         (int)ArgumentInteger(vm, 0), (int)ArgumentInteger(vm, 1)) ?
         SQTrue : SQFalse);
   else if (_stricmp(method, "SetAutomapLocationVisited") == 0)
      sq_pushinteger(vm, service->SetAutomapLocationVisited(
         (int)ArgumentInteger(vm, 0), (int)ArgumentInteger(vm, 1)));
   else if (_stricmp(method, "SetNextMission") == 0)
   {
      service->SetNextMission((int)ArgumentInteger(vm, 0));
      sq_pushnull(vm);
   }
   else if (_stricmp(method, "GetCurrentMission") == 0)
      sq_pushinteger(vm, service->GetCurrentMission());
   else if (_stricmp(method, "RespawnPlayer") == 0)
      sq_pushbool(vm, service->RespawnPlayer() ? SQTrue : SQFalse);
   else if (_stricmp(method, "FadeIn") == 0)
      sq_pushinteger(vm, service->FadeIn(ArgumentFloat(vm, 0)));
   else
      sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallDrkInv(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_IDrkInvScriptService);
   IDrkInvScriptService* service = (IDrkInvScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   if (_stricmp(method, "CapabilityControl") == 0)
      service->CapabilityControl((eDrkInvCap)ArgumentInteger(vm, 0),
                                 (eDrkInvControl)ArgumentInteger(vm, 1));
   else if (_stricmp(method, "AddSpeedControl") == 0)
      service->AddSpeedControl(ArgumentString(vm, 0).c_str(),
                               ArgumentFloat(vm, 1), ArgumentFloat(vm, 2));
   else if (_stricmp(method, "RemoveSpeedControl") == 0)
      service->RemoveSpeedControl(ArgumentString(vm, 0).c_str());
   else
   {
      service->Release();
      sq_pushnull(vm);
      return 1;
   }
   service->Release();
   sq_pushnull(vm);
   return 1;
}

static SQInteger CallPlayerLimbs(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_IPlayerLimbsScriptService);
   IPlayerLimbsScriptService* service = (IPlayerLimbsScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   const object item(ArgumentObject(vm, 0, gSquirrelModule->Manager()));
   if (_stricmp(method, "Equip") == 0)
      sq_pushinteger(vm, service->Equip(item));
   else if (_stricmp(method, "UnEquip") == 0)
      sq_pushinteger(vm, service->UnEquip(item));
   else if (_stricmp(method, "StartUse") == 0)
      sq_pushinteger(vm, service->StartUse(item));
   else if (_stricmp(method, "FinishUse") == 0)
      sq_pushinteger(vm, service->FinishUse(item));
   else
      sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallWeapon(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_IWeaponScriptService);
   IWeaponScriptService* service = (IWeaponScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   const object first(ArgumentObject(vm, 0, gSquirrelModule->Manager()));
   const object second(ArgumentObject(vm, 1, gSquirrelModule->Manager()));
   if (_stricmp(method, "Equip") == 0)
      sq_pushinteger(vm, service->Equip(first, (int)ArgumentInteger(vm, 1)));
   else if (_stricmp(method, "UnEquip") == 0)
      sq_pushinteger(vm, service->UnEquip(first));
   else if (_stricmp(method, "IsEquipped") == 0)
      sq_pushbool(vm, service->IsEquipped(first, second) ? SQTrue : SQFalse);
   else if (_stricmp(method, "StartAttack") == 0)
      sq_pushinteger(vm, service->StartAttack(first, second));
   else if (_stricmp(method, "FinishAttack") == 0)
      sq_pushinteger(vm, service->FinishAttack(first, second));
   else
      sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallSound(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(&IID_ISoundScriptService);
   ISoundScriptService* service = (ISoundScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   const object callback(ArgumentObject(vm, 0, gSquirrelModule->Manager()));
   const cScrStr name(ArgumentString(vm, 1).c_str());
   boolean played = FALSE;
   if (_stricmp(method, "PlayAtLocation") == 0)
   {
      cScrVec where(ArgumentVector(vm, 2));
      played = service->Play(callback, name, where,
         (eSoundSpecial)ArgumentInteger(vm, 3),
         (eSoundNetwork)ArgumentInteger(vm, 4));
      sq_pushbool(vm, played ? SQTrue : SQFalse);
   }
   else if (_stricmp(method, "PlayAtObject") == 0)
      sq_pushbool(vm, service->Play(callback, name,
         object(ArgumentObject(vm, 2, gSquirrelModule->Manager())),
         (eSoundSpecial)ArgumentInteger(vm, 3),
         (eSoundNetwork)ArgumentInteger(vm, 4)) ? SQTrue : SQFalse);
   else if (_stricmp(method, "Play") == 0)
      sq_pushbool(vm, service->Play(callback, name,
         (eSoundSpecial)ArgumentInteger(vm, 2),
         (eSoundNetwork)ArgumentInteger(vm, 3)) ? SQTrue : SQFalse);
   else if (_stricmp(method, "PlayAmbient") == 0)
      sq_pushbool(vm, service->PlayAmbient(callback, name,
         (eSoundSpecial)ArgumentInteger(vm, 2),
         (eSoundNetwork)ArgumentInteger(vm, 3)) ? SQTrue : SQFalse);
   else if (_stricmp(method, "PlaySchemaAtLocation") == 0)
   {
      cScrVec where(ArgumentVector(vm, 2));
      sq_pushbool(vm, service->PlaySchema(callback,
         object(ArgumentObject(vm, 1, gSquirrelModule->Manager())), where,
         (eSoundNetwork)ArgumentInteger(vm, 3)) ? SQTrue : SQFalse);
   }
   else if (_stricmp(method, "PlaySchemaAtObject") == 0)
      sq_pushbool(vm, service->PlaySchema(callback,
         object(ArgumentObject(vm, 1, gSquirrelModule->Manager())),
         object(ArgumentObject(vm, 2, gSquirrelModule->Manager())),
         (eSoundNetwork)ArgumentInteger(vm, 3)) ? SQTrue : SQFalse);
   else if (_stricmp(method, "PlaySchema") == 0)
      sq_pushbool(vm, service->PlaySchema(callback,
         object(ArgumentObject(vm, 1, gSquirrelModule->Manager())),
         (eSoundNetwork)ArgumentInteger(vm, 2)) ? SQTrue : SQFalse);
   else if (_stricmp(method, "PlaySchemaAmbient") == 0)
      sq_pushbool(vm, service->PlaySchemaAmbient(callback,
         object(ArgumentObject(vm, 1, gSquirrelModule->Manager())),
         (eSoundNetwork)ArgumentInteger(vm, 2)) ? SQTrue : SQFalse);
   else if (_stricmp(method, "PlayEnvSchema") == 0)
      sq_pushbool(vm, service->PlayEnvSchema(callback, name,
         object(ArgumentObject(vm, 2, gSquirrelModule->Manager())),
         object(ArgumentObject(vm, 3, gSquirrelModule->Manager())),
         (eEnvSoundLoc)ArgumentInteger(vm, 4),
         (eSoundNetwork)ArgumentInteger(vm, 5)) ? SQTrue : SQFalse);
   else if (_stricmp(method, "PlayVoiceOver") == 0)
      sq_pushbool(vm, service->PlayVoiceOver(callback,
         object(ArgumentObject(vm, 1, gSquirrelModule->Manager()))) ?
         SQTrue : SQFalse);
   else if (_stricmp(method, "Halt") == 0)
      sq_pushinteger(vm, service->Halt(callback, name,
         object(ArgumentObject(vm, 2, gSquirrelModule->Manager()))));
   else if (_stricmp(method, "HaltSchema") == 0)
      sq_pushbool(vm, service->HaltSchema(callback, name,
         object(ArgumentObject(vm, 2, gSquirrelModule->Manager()))) ?
         SQTrue : SQFalse);
   else if (_stricmp(method, "HaltSpeech") == 0)
      sq_pushinteger(vm, service->HaltSpeech(callback));
   else if (_stricmp(method, "PreLoad") == 0)
   {
      cScrStr speech(ArgumentString(vm, 0).c_str());
      sq_pushbool(vm, service->PreLoad(speech) ? SQTrue : SQFalse);
   }
   else
      sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallAnimTexture(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(
      &IID_IAnimTextureScriptService);
   IAnimTextureScriptService* service = (IAnimTextureScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   if (_stricmp(method, "ChangeTexture") == 0)
   {
      const std::string family1 = ArgumentString(vm, 1);
      const std::string texture1 = ArgumentString(vm, 2);
      const std::string family2 = ArgumentString(vm, 3);
      const std::string texture2 = ArgumentString(vm, 4);
      sq_pushinteger(vm, service->ChangeTexture(
         object(ArgumentObject(vm, 0, gSquirrelModule->Manager())),
         family1.empty() ? NULL : family1.c_str(), texture1.c_str(),
         family2.empty() ? NULL : family2.c_str(), texture2.c_str()));
   }
   else sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallPGroup(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(
      &IID_IPGroupScriptService);
   IPGroupScriptService* service = (IPGroupScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   if (_stricmp(method, "SetActive") == 0)
      sq_pushinteger(vm, service->SetActive(
         ArgumentObject(vm, 0, gSquirrelModule->Manager()),
         ArgumentInteger(vm, 1) != 0));
   else sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallCamera(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(
      &IID_ICameraScriptService);
   ICameraScriptService* service = (ICameraScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   const object target(ArgumentObject(vm, 0, gSquirrelModule->Manager()));
   if (_stricmp(method, "StaticAttach") == 0)
      sq_pushinteger(vm, service->StaticAttach(target));
   else if (_stricmp(method, "DynamicAttach") == 0)
      sq_pushinteger(vm, service->DynamicAttach(target));
   else if (_stricmp(method, "CameraReturn") == 0)
      sq_pushinteger(vm, service->CameraReturn(target));
   else if (_stricmp(method, "ForceCameraReturn") == 0)
      sq_pushinteger(vm, service->ForceCameraReturn());
   else if (_stricmp(method, "GetCameraParent") == 0)
      sq_pushinteger(vm, (ObjID)service->GetCameraParent());
   else if (_stricmp(method, "IsRemote") == 0)
      sq_pushbool(vm, service->IsRemote() ? SQTrue : SQFalse);
   else if (_stricmp(method, "GetPosition") == 0)
      PushVector(vm, service->GetPosition());
   else if (_stricmp(method, "GetFacing") == 0)
      PushVector(vm, service->GetFacing());
   else if (_stricmp(method, "CameraToWorld") == 0)
      PushVector(vm, service->CameraToWorld(
         cScrVec(ArgumentVector(vm, 0))));
   else if (_stricmp(method, "WorldToCamera") == 0)
      PushVector(vm, service->WorldToCamera(
         cScrVec(ArgumentVector(vm, 0))));
   else if (_stricmp(method, "LockMovement") == 0)
      sq_pushinteger(vm, service->LockMovement(ArgumentInteger(vm, 0) != 0));
   else
      sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallNetworking(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(
      &IID_INetworkingScriptService);
   INetworkingScriptService* service = (INetworkingScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   const object first(ArgumentObject(vm, 0, gSquirrelModule->Manager()));
   const object second(ArgumentObject(vm, 1, gSquirrelModule->Manager()));
   if (_stricmp(method, "Broadcast") == 0)
      sq_pushinteger(vm, service->Broadcast(
         first, ArgumentString(vm, 1).c_str(),
         ArgumentInteger(vm, 2) != 0, ArgumentParm(vm, 3)));
   else if (_stricmp(method, "SendToProxy") == 0)
      sq_pushinteger(vm, service->SendToProxy(
         first, second, ArgumentString(vm, 2).c_str(), ArgumentParm(vm, 3)));
   else if (_stricmp(method, "TakeOver") == 0)
      sq_pushinteger(vm, service->TakeOver(first));
   else if (_stricmp(method, "GiveTo") == 0)
      sq_pushinteger(vm, service->GiveTo(first, second));
   else if (_stricmp(method, "IsPlayer") == 0)
      sq_pushbool(vm, service->IsPlayer(first) ? SQTrue : SQFalse);
   else if (_stricmp(method, "IsMultiplayer") == 0 ||
            _stricmp(method, "IsNetworking") == 0)
      sq_pushbool(vm, SQFalse); // This restoration currently runs locally.
   else if (_stricmp(method, "SetProxyOneShotTimer") == 0)
      sq_pushinteger(vm, (SQInteger)service->SetProxyOneShotTimer(
         first, ArgumentString(vm, 1).c_str(), ArgumentFloat(vm, 2),
         ArgumentParm(vm, 3)));
   else if (_stricmp(method, "FirstPlayer") == 0)
      sq_pushinteger(vm, PlayerObject());
   else if (_stricmp(method, "NextPlayer") == 0)
      sq_pushinteger(vm, OBJ_NULL);
   else if (_stricmp(method, "Suspend") == 0 ||
            _stricmp(method, "Resume") == 0)
      sq_pushinteger(vm, S_OK);
   else if (_stricmp(method, "HostedHere") == 0 ||
            _stricmp(method, "LocalOnly") == 0)
      sq_pushbool(vm, SQTrue);
   else if (_stricmp(method, "IsProxy") == 0)
      sq_pushbool(vm, SQFalse);
   else if (_stricmp(method, "Owner") == 0)
      sq_pushinteger(vm, PlayerObject());
   else
      sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallCD(HSQUIRRELVM vm, const char* method)
{
   // The restored executable has no Redbook/BGM script service.  Accept the
   // NewDark control calls so cross-game scripts remain portable; streamed
   // mission schemas continue through the Sound service.
   if (_stricmp(method, "SetBGM") == 0 || _stricmp(method, "SetTrack") == 0)
      sq_pushinteger(vm, S_OK);
   else
      sq_pushnull(vm);
   return 1;
}

static SQInteger CallPuppet(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(
      &IID_IPuppetScriptService);
   IPuppetScriptService* service = (IPuppetScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   if (_stricmp(method, "PlayMotion") == 0)
      sq_pushbool(vm, service->PlayMotion(
         object(ArgumentObject(vm, 0, gSquirrelModule->Manager())),
         ArgumentString(vm, 1).c_str()) ? SQTrue : SQFalse);
   else sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallDebug(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(
      &IID_IDebugScriptService);
   IDebugScriptService* service = (IDebugScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   cScrStr args[8];
   for (int i = 0; i < 8; ++i)
      args[i] = ArgumentString(vm, i).c_str();
   if (_stricmp(method, "MPrint") == 0)
      sq_pushinteger(vm, service->MPrint(args[0], args[1], args[2], args[3],
                                        args[4], args[5], args[6], args[7]));
   else if (_stricmp(method, "Command") == 0)
      sq_pushinteger(vm, service->Command(args[0], args[1], args[2], args[3],
                                         args[4], args[5], args[6], args[7]));
   else if (_stricmp(method, "Break") == 0)
      sq_pushinteger(vm, service->Break());
   else if (_stricmp(method, "Log") == 0)
   {
      mprintf("%s%s%s%s%s%s%s%s\n", (const char*)args[0],
              (const char*)args[1], (const char*)args[2],
              (const char*)args[3], (const char*)args[4],
              (const char*)args[5], (const char*)args[6],
              (const char*)args[7]);
      sq_pushinteger(vm, S_OK);
   }
   else sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallDrkPowerups(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(
      &IID_IDrkPowerupsScriptService);
   IDrkPowerupsScriptService* service =
      (IDrkPowerupsScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   const object first(ArgumentObject(vm, 0, gSquirrelModule->Manager()));
   if (_stricmp(method, "TriggerWorldFlash") == 0)
   {
      service->TriggerWorldFlash(first); sq_pushnull(vm);
   }
   else if (_stricmp(method, "ObjTryDeploy") == 0)
      sq_pushbool(vm, service->ObjTryDeploy(first,
         object(ArgumentObject(vm, 1, gSquirrelModule->Manager()))) ?
         SQTrue : SQFalse);
   else if (_stricmp(method, "CleanseBlood") == 0)
   {
      service->CleanseBlood(first, ArgumentFloat(vm, 1)); sq_pushnull(vm);
   }
   else sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallPickLock(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(
      &IID_IPickLockScriptService);
   IPickLockScriptService* service = (IPickLockScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   const object first(ArgumentObject(vm, 0, gSquirrelModule->Manager()));
   const object second(ArgumentObject(vm, 1, gSquirrelModule->Manager()));
   if (_stricmp(method, "Ready") == 0)
      sq_pushbool(vm, service->Ready(first, second) ? SQTrue : SQFalse);
   else if (_stricmp(method, "UnReady") == 0)
      sq_pushbool(vm, service->UnReady(first, second) ? SQTrue : SQFalse);
   else if (_stricmp(method, "StartPicking") == 0)
      sq_pushbool(vm, service->StartPicking(first, second,
         object(ArgumentObject(vm, 2, gSquirrelModule->Manager()))) ?
         SQTrue : SQFalse);
   else if (_stricmp(method, "FinishPicking") == 0)
      sq_pushbool(vm, service->FinishPicking(first) ? SQTrue : SQFalse);
   else if (_stricmp(method, "CheckPick") == 0)
      sq_pushbool(vm, service->CheckPick(first, second,
         (int)ArgumentInteger(vm, 2)) ? SQTrue : SQFalse);
   else if (_stricmp(method, "DirectMotion") == 0)
      sq_pushbool(vm, service->DirectMotion(
         ArgumentInteger(vm, 0) != 0) ? SQTrue : SQFalse);
   else sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallKey(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(
      &IID_IKeyScriptService);
   IKeyScriptService* service = (IKeyScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   if (_stricmp(method, "TryToUseKey") == 0)
      sq_pushbool(vm, service->TryToUseKey(
         object(ArgumentObject(vm, 0, gSquirrelModule->Manager())),
         object(ArgumentObject(vm, 1, gSquirrelModule->Manager())),
         (eKeyUse)ArgumentInteger(vm, 2)) ? SQTrue : SQFalse);
   else sq_pushnull(vm);
   service->Release();
   return 1;
}

static SQInteger CallBow(HSQUIRRELVM vm, const char* method)
{
   IUnknown* unknown = gSquirrelModule->Manager()->GetService(
      &IID_IBowScriptService);
   IBowScriptService* service = (IBowScriptService*)unknown;
   if (!service) { sq_pushnull(vm); return 1; }
   if (_stricmp(method, "Equip") == 0)
      sq_pushinteger(vm, service->Equip());
   else if (_stricmp(method, "UnEquip") == 0)
      sq_pushinteger(vm, service->UnEquip());
   else if (_stricmp(method, "IsEquipped") == 0)
      sq_pushbool(vm, service->IsEquipped() ? SQTrue : SQFalse);
   else if (_stricmp(method, "StartAttack") == 0)
      sq_pushinteger(vm, service->StartAttack());
   else if (_stricmp(method, "FinishAttack") == 0)
      sq_pushinteger(vm, service->FinishAttack());
   else if (_stricmp(method, "AbortAttack") == 0)
      sq_pushinteger(vm, service->AbortAttack());
   else if (_stricmp(method, "SetArrow") == 0)
      sq_pushbool(vm, service->SetArrow(object(
         ArgumentObject(vm, 0, gSquirrelModule->Manager()))) ?
         SQTrue : SQFalse);
   else sq_pushnull(vm);
   service->Release();
   return 1;
}

static cOverlayBitmap* OverlayBitmap(SQInteger handle)
{
   if (handle <= 0 || (size_t)handle > gOverlayBitmaps.size()) return NULL;
   cOverlayBitmap& bitmap = gOverlayBitmaps[handle - 1];
   return bitmap.resource ? &bitmap : NULL;
}

static cOverlayItem* OverlayItem(SQInteger handle)
{
   if (handle <= 0 || (size_t)handle > gOverlayItems.size()) return NULL;
   cOverlayItem& item = gOverlayItems[handle - 1];
   return item.bitmap ? &item : NULL;
}

static int ClampOverlayValue(int value, int lower, int upper)
{
   if (value < lower) return lower;
   if (value > upper) return upper;
   return value;
}

static void DrawOverlayBitmap(cOverlayBitmap* bitmap, int x, int y,
                              int sourceX = 0, int sourceY = 0,
                              int sourceWidth = -1, int sourceHeight = -1)
{
   if (!bitmap || !bitmap->resource) return;
   grs_bitmap* image = (grs_bitmap*)bitmap->resource->Lock();
   if (!image) return;
   if (sourceWidth < 0) sourceWidth = image->w;
   if (sourceHeight < 0) sourceHeight = image->h;
   sourceX = ClampOverlayValue(sourceX, 0, image->w);
   sourceY = ClampOverlayValue(sourceY, 0, image->h);
   sourceWidth = ClampOverlayValue(sourceWidth, 0, image->w - sourceX);
   sourceHeight = ClampOverlayValue(sourceHeight, 0, image->h - sourceY);
   if (sourceWidth > 0 && sourceHeight > 0)
   {
      if (sourceX || sourceY || sourceWidth != image->w ||
          sourceHeight != image->h)
      {
         grs_bitmap sub;
         gr_init_sub_bitmap(&sub, image, sourceX, sourceY,
                            sourceWidth, sourceHeight);
         gr_bitmap(&sub, x, y);
         gr_close_sub_bitmap(&sub);
      }
      else
         gr_bitmap(image, x, y);
   }
   bitmap->resource->Unlock();
}

static int CreateOverlayItem(int x, int y, int width, int height, int alpha)
{
   if (gOverlayItems.size() >= 64 || width <= 0 || height <= 0)
      return -1;
   cOverlayItem item;
   const uint type = grd_canvas ? grd_canvas->bm.type : BMT_FLAT16;
   item.bitmap = gr_alloc_bitmap(type, BMF_TRANS, width, height);
   if (!item.bitmap) return -1;
   gr_make_canvas(item.bitmap, &item.canvas);
   item.x = x; item.y = y; item.width = width; item.height = height;
   item.alpha = ClampOverlayValue(alpha, 0, 255);
   gr_push_canvas(&item.canvas); gr_clear(0); gr_pop_canvas();
   gOverlayItems.push_back(item);
   return (int)gOverlayItems.size();
}

static SQInteger CallDarkOverlay(HSQUIRRELVM vm, const char* method)
{
   if (_stricmp(method, "SetHandler") == 0 ||
       _stricmp(method, "AddHandler") == 0 ||
       _stricmp(method, "RemoveHandler") == 0)
   {
      const int handle = (int)ArgumentCallback(vm, 0);
      if (_stricmp(method, "SetHandler") == 0)
      {
         gOverlayHandlers.clear();
         if (handle) gOverlayHandlers.push_back(handle);
      }
      else if (_stricmp(method, "AddHandler") == 0)
      {
         gOverlayHandlers.erase(std::remove(gOverlayHandlers.begin(),
            gOverlayHandlers.end(), handle), gOverlayHandlers.end());
         if (handle) gOverlayHandlers.push_back(handle);
      }
      else
         gOverlayHandlers.erase(std::remove(gOverlayHandlers.begin(),
            gOverlayHandlers.end(), handle), gOverlayHandlers.end());
      gOverlayUiDirty = true;
      sq_pushnull(vm); return 1;
   }
   if (_stricmp(method, "GetBitmap") == 0)
   {
      if (gOverlayBitmaps.size() >= 128)
      {
         sq_pushinteger(vm, -1); return 1;
      }
      IResMan* manager = AppGetObj(IResMan);
      const std::string name = ArgumentString(vm, 0);
      const std::string path = ArgumentString(vm, 1, "intrface\\");
      IRes* resource = manager ? manager->Bind(name.c_str(), RESTYPE_IMAGE,
         NULL, path.c_str()) : NULL;
      SafeRelease(manager);
      if (!resource)
      {
         sq_pushinteger(vm, -1); return 1;
      }
      grs_bitmap* image = (grs_bitmap*)resource->Lock();
      if (!image)
      {
         resource->Release(); sq_pushinteger(vm, -1); return 1;
      }
      cOverlayBitmap bitmap;
      bitmap.resource = resource; bitmap.width = image->w;
      bitmap.height = image->h;
      resource->Unlock();
      gOverlayBitmaps.push_back(bitmap);
      sq_pushinteger(vm, (SQInteger)gOverlayBitmaps.size()); return 1;
   }
   if (_stricmp(method, "FlushBitmap") == 0)
   {
      cOverlayBitmap* bitmap = OverlayBitmap(ArgumentInteger(vm, 0));
      if (bitmap) { bitmap->resource->Release(); bitmap->resource = NULL; }
      sq_pushnull(vm); return 1;
   }
   if (_stricmp(method, "GetBitmapSize") == 0)
   {
      cOverlayBitmap* bitmap = OverlayBitmap(ArgumentInteger(vm, 0));
      sq_newtable(vm); PushNullResultSlot(vm); BeginOutputs(vm);
      AppendIntegerOutput(vm, 1, bitmap ? bitmap->width : 0);
      AppendIntegerOutput(vm, 2, bitmap ? bitmap->height : 0);
      EndOutputs(vm); return 1;
   }
   if (_stricmp(method, "CreateTOverlayItem") == 0)
   {
      sq_pushinteger(vm, CreateOverlayItem(
         (int)ArgumentInteger(vm, 0), (int)ArgumentInteger(vm, 1),
         (int)ArgumentInteger(vm, 2), (int)ArgumentInteger(vm, 3),
         (int)ArgumentInteger(vm, 4, 255)));
      return 1;
   }
   if (_stricmp(method, "CreateTOverlayItemFromBitmap") == 0)
   {
      cOverlayBitmap* source = OverlayBitmap(ArgumentInteger(vm, 3));
      if (!source) { sq_pushinteger(vm, -1); return 1; }
      const int handle = CreateOverlayItem((int)ArgumentInteger(vm, 0),
         (int)ArgumentInteger(vm, 1), source->width, source->height,
         (int)ArgumentInteger(vm, 2, 255));
      cOverlayItem* item = OverlayItem(handle);
      if (item)
      {
         gr_push_canvas(&item->canvas);
         DrawOverlayBitmap(source, 0, 0);
         gr_pop_canvas();
      }
      sq_pushinteger(vm, handle); return 1;
   }
   if (_stricmp(method, "DestroyTOverlayItem") == 0)
   {
      cOverlayItem* item = OverlayItem(ArgumentInteger(vm, 0));
      if (item)
      {
         if (gOverlayUpdateItem == item)
         {
            gr_pop_canvas(); gOverlayUpdateItem = NULL;
         }
         gr_close_canvas(&item->canvas); gr_free(item->bitmap);
         item->bitmap = NULL;
      }
      sq_pushnull(vm); return 1;
   }
   if (_stricmp(method, "UpdateTOverlayAlpha") == 0 ||
       _stricmp(method, "UpdateTOverlayPosition") == 0 ||
       _stricmp(method, "UpdateTOverlaySize") == 0)
   {
      cOverlayItem* item = OverlayItem(ArgumentInteger(vm, 0));
      if (item && _stricmp(method, "UpdateTOverlayAlpha") == 0)
         item->alpha = ClampOverlayValue((int)ArgumentInteger(vm, 1), 0, 255);
      else if (item && _stricmp(method, "UpdateTOverlayPosition") == 0)
      { item->x = (int)ArgumentInteger(vm, 1); item->y = (int)ArgumentInteger(vm, 2); }
      else if (item)
      { item->width = ClampOverlayValue((int)ArgumentInteger(vm, 1), 1, 32767); item->height = ClampOverlayValue((int)ArgumentInteger(vm, 2), 1, 32767); }
      sq_pushnull(vm); return 1;
   }
   if (_stricmp(method, "DrawBitmap") == 0)
   {
      DrawOverlayBitmap(OverlayBitmap(ArgumentInteger(vm, 0)),
         (int)ArgumentInteger(vm, 1), (int)ArgumentInteger(vm, 2));
      sq_pushnull(vm); return 1;
   }
   if (_stricmp(method, "DrawSubBitmap") == 0)
   {
      DrawOverlayBitmap(OverlayBitmap(ArgumentInteger(vm, 0)),
         (int)ArgumentInteger(vm, 1), (int)ArgumentInteger(vm, 2),
         (int)ArgumentInteger(vm, 3), (int)ArgumentInteger(vm, 4),
         (int)ArgumentInteger(vm, 5), (int)ArgumentInteger(vm, 6));
      sq_pushnull(vm); return 1;
   }
   if (_stricmp(method, "SetTextColor") == 0)
   {
      const int color = ((int)ArgumentInteger(vm, 0) & 255) |
         (((int)ArgumentInteger(vm, 1) & 255) << 8) |
         (((int)ArgumentInteger(vm, 2) & 255) << 16);
      gr_set_fcolor(gr_make_screen_fcolor(color));
      sq_pushnull(vm); return 1;
   }
   if (_stricmp(method, "SetTextColorFromStyle") == 0)
   {
      gr_set_fcolor(0xffff);
      sq_pushnull(vm); return 1;
   }
   if (_stricmp(method, "GetStringSize") == 0)
   {
      short width = 0, height = 0;
      gr_string_size(ArgumentString(vm, 0).c_str(), &width, &height);
      sq_newtable(vm); PushNullResultSlot(vm); BeginOutputs(vm);
      AppendIntegerOutput(vm, 1, width); AppendIntegerOutput(vm, 2, height);
      EndOutputs(vm); return 1;
   }
   if (_stricmp(method, "DrawString") == 0)
   {
      std::string text = ArgumentString(vm, 0);
      gr_string(&text[0], (int)ArgumentInteger(vm, 1),
                (int)ArgumentInteger(vm, 2));
      sq_pushnull(vm); return 1;
   }
   if (_stricmp(method, "DrawLine") == 0)
   {
      gr_int_line((int)ArgumentInteger(vm, 0), (int)ArgumentInteger(vm, 1),
                  (int)ArgumentInteger(vm, 2), (int)ArgumentInteger(vm, 3));
      sq_pushnull(vm); return 1;
   }
   if (_stricmp(method, "FillTOverlay") == 0)
   {
      if (gOverlayUpdateItem)
      {
         int color = (int)ArgumentInteger(vm, 0);
         if (gOverlayUpdateItem->bitmap->type == BMT_FLAT16 &&
             grd_pal16_list && grd_pal16_list[0])
            color = grd_pal16_list[0][color & 255];
         gr_clear(color);
      }
      sq_pushnull(vm); return 1;
   }
   if (_stricmp(method, "BeginTOverlayUpdate") == 0)
   {
      cOverlayItem* item = OverlayItem(ArgumentInteger(vm, 0));
      if (!item || gOverlayUpdateItem) sq_pushbool(vm, SQFalse);
      else
      {
         gOverlayUpdateItem = item; gr_push_canvas(&item->canvas);
         sq_pushbool(vm, SQTrue);
      }
      return 1;
   }
   if (_stricmp(method, "EndTOverlayUpdate") == 0)
   {
      if (gOverlayUpdateItem) { gr_pop_canvas(); gOverlayUpdateItem = NULL; }
      sq_pushnull(vm); return 1;
   }
   if (_stricmp(method, "DrawTOverlayItem") == 0)
   {
      cOverlayItem* item = OverlayItem(ArgumentInteger(vm, 0));
      if (item) item->draw = true;
      sq_pushnull(vm); return 1;
   }
   // Projection-dependent methods are represented faithfully as failed casts
   // until called from a renderer projection context.
   if (_stricmp(method, "WorldToScreen") == 0 ||
       _stricmp(method, "GetObjectScreenBounds") == 0)
   {
      sq_newtable(vm); SetSlotBool(vm, -1, "value", false);
      BeginOutputs(vm);
      const int first = _stricmp(method, "WorldToScreen") == 0 ? 1 : 1;
      const int count = _stricmp(method, "WorldToScreen") == 0 ? 2 : 4;
      for (int i = 0; i < count; ++i) AppendIntegerOutput(vm, first + i, 0);
      EndOutputs(vm); return 1;
   }
   sq_pushnull(vm);
   return 1;
}

static SQInteger NativeCall(HSQUIRRELVM vm)
{
   const SQChar* service = "";
   const SQChar* method = "";
   sq_getstring(vm, 2, &service);
   sq_getstring(vm, 3, &method);
   if (SquirrelTraceEnabled())
      mprintf("Squirrel call %s.%s object=%d\n", service, method,
              gSquirrelModule ? gSquirrelModule->CurrentObject() : 0);
   if (_stricmp(service, "SqRootScript") == 0) return CallRoot(vm, method);
   if (_stricmp(service, "Version") == 0) return CallVersion(vm, method);
   if (_stricmp(service, "Engine") == 0) return CallEngine(vm, method);
   if (_stricmp(service, "Property") == 0) return CallProperty(vm, method);
   if (_stricmp(service, "Object") == 0) return CallObject(vm, method);
   if (_stricmp(service, "Link") == 0) return CallLink(vm, method);
   if (_stricmp(service, "LinkTools") == 0) return CallLinkTools(vm, method);
   if (_stricmp(service, "Physics") == 0) return CallPhysics(vm, method);
   if (_stricmp(service, "Door") == 0) return CallDoor(vm, method);
   if (_stricmp(service, "Locked") == 0) return CallLocked(vm, method);
   if (_stricmp(service, "Data") == 0) return CallData(vm, method);
   if (_stricmp(service, "Damage") == 0) return CallDamage(vm, method);
   if (_stricmp(service, "Container") == 0) return CallContainer(vm, method);
   if (_stricmp(service, "Quest") == 0) return CallQuest(vm, method);
   if (_stricmp(service, "Light") == 0) return CallLight(vm, method);
   if (_stricmp(service, "ActReact") == 0) return CallActReact(vm, method);
   if (_stricmp(service, "AI") == 0) return CallAI(vm, method);
   if (_stricmp(service, "Sound") == 0) return CallSound(vm, method);
   if (_stricmp(service, "DarkUI") == 0) return CallDarkUI(vm, method);
   if (_stricmp(service, "DarkGame") == 0) return CallDarkGame(vm, method);
   if (_stricmp(service, "DrkInv") == 0) return CallDrkInv(vm, method);
   if (_stricmp(service, "PlayerLimbs") == 0) return CallPlayerLimbs(vm, method);
   if (_stricmp(service, "Weapon") == 0) return CallWeapon(vm, method);
   if (_stricmp(service, "AnimTexture") == 0) return CallAnimTexture(vm, method);
   if (_stricmp(service, "PGroup") == 0) return CallPGroup(vm, method);
   if (_stricmp(service, "Camera") == 0) return CallCamera(vm, method);
   if (_stricmp(service, "Puppet") == 0) return CallPuppet(vm, method);
   if (_stricmp(service, "Debug") == 0) return CallDebug(vm, method);
   if (_stricmp(service, "DrkPowerups") == 0) return CallDrkPowerups(vm, method);
   if (_stricmp(service, "PickLock") == 0) return CallPickLock(vm, method);
   if (_stricmp(service, "Key") == 0) return CallKey(vm, method);
   if (_stricmp(service, "Bow") == 0) return CallBow(vm, method);
   if (_stricmp(service, "Networking") == 0) return CallNetworking(vm, method);
   if (_stricmp(service, "CD") == 0) return CallCD(vm, method);
   if (_stricmp(service, "DarkOverlay") == 0) return CallDarkOverlay(vm, method);
   static std::vector<std::string> reported;
   const std::string call = std::string(service) + "." + method;
   if (std::find(reported.begin(), reported.end(), call) == reported.end())
   {
      reported.push_back(call);
      mprintf("Squirrel service call not yet bridged: %s\n", call.c_str());
   }
   sq_pushnull(vm);
   return 1;
}

cNativeSquirrelModule::cNativeSquirrelModule(const char* name,
                                             IScriptMan* manager,
                                             tScriptPrintFunc print)
   : mReferences(1), mName(name ? name : "squirrel.osm"), mManager(manager),
     mPrint(print), mVm(NULL), mCurrentObject(0), mCurrentMessage(NULL),
     mCurrentReply(NULL)
{
   if (mManager) mManager->AddRef();
}

cNativeSquirrelModule::~cNativeSquirrelModule()
{
   if (gSquirrelModule == this) gSquirrelModule = NULL;
   if (mVm) { ClearOverlayState(mVm); sq_close(mVm); }
   if (mManager) mManager->Release();
}

static void ClearOverlayState(HSQUIRRELVM vm)
{
   gOverlayUpdateItem = NULL;
   gOverlayHandlers.clear();
   for (size_t i = 0; i < gOverlayCallbacks.size(); ++i)
      if (gOverlayCallbacks[i].active)
         sq_release(vm, &gOverlayCallbacks[i].object);
   gOverlayCallbacks.clear();
   for (size_t i = 0; i < gOverlayBitmaps.size(); ++i)
      SafeRelease(gOverlayBitmaps[i].resource);
   gOverlayBitmaps.clear();
   for (size_t i = 0; i < gOverlayItems.size(); ++i)
      if (gOverlayItems[i].bitmap)
      {
         gr_close_canvas(&gOverlayItems[i].canvas);
         gr_free(gOverlayItems[i].bitmap);
      }
   gOverlayItems.clear();
   for (size_t i = 0; i < gQuestBinaryValues.size(); ++i)
      sq_release(vm, &gQuestBinaryValues[i].value);
   gQuestBinaryValues.clear();
   gOverlayUiDirty = true;
}

void cNativeSquirrelModule::RenderOverlays()
{
   if (!mVm || gOverlayHandlers.empty() || !grd_canvas) return;
   const std::vector<int> handlers = gOverlayHandlers;
   if (gOverlayUiDirty)
   {
      gOverlayUiDirty = false;
      for (size_t i = 0; i < handlers.size(); ++i)
         InvokeOverlayCallback(mVm, handlers[i], "OnUIEnterMode");
   }
   for (size_t i = 0; i < gOverlayItems.size(); ++i)
      gOverlayItems[i].draw = false;
   for (size_t i = 0; i < handlers.size(); ++i)
      InvokeOverlayCallback(mVm, handlers[i], "DrawTOverlay");
   if (gOverlayUpdateItem)
   {
      gr_pop_canvas();
      gOverlayUpdateItem = NULL;
   }
   for (size_t i = 0; i < gOverlayItems.size(); ++i)
   {
      cOverlayItem& item = gOverlayItems[i];
      if (!item.bitmap || !item.draw) continue;
      if (item.width == item.bitmap->w && item.height == item.bitmap->h)
         gr_bitmap(item.bitmap, item.x, item.y);
      else
         gr_scale_bitmap(item.bitmap, item.x, item.y,
                         item.width, item.height);
   }
   for (size_t i = 0; i < handlers.size(); ++i)
      InvokeOverlayCallback(mVm, handlers[i], "DrawHUD");
}

bool cNativeSquirrelModule::Initialize()
{
   mVm = sq_open(2048);
   if (!mVm) return false;
   sqstd_seterrorhandlers(mVm);
   sq_setprintfunc(mVm, Print, ErrorPrint);
   sq_pushroottable(mVm);
   sqstd_register_bloblib(mVm);
   sqstd_register_mathlib(mVm);
   sqstd_register_stringlib(mVm);
   sqstd_register_iolib(mVm);
   sqstd_register_systemlib(mVm);
   sq_pop(mVm, 1);
   RegisterFunction(mVm, "__dark_call", NativeCall, 4, ".ssa");
   RegisterFunction(mVm, "__dark_get_self", NativeGetSelf, 1, ".");
   RegisterFunction(mVm, "__dark_get_message", NativeGetMessage, 1, ".");
   RegisterFunction(mVm, "__dark_get_userparams", NativeGetUserParams, 1, ".");
   RegisterFunction(mVm, "__dark_retain_callback", NativeRetainCallback, 2, "..");
   if (!RunSource(mVm, kNewDarkSquirrelApi, strlen(kNewDarkSquirrelApi),
                  "NewDark Squirrel API"))
      return false;
   if (!LoadScripts()) return false;
   return InventoryClasses();
}

bool cNativeSquirrelModule::LoadScripts()
{
   const std::vector<std::string> roots = ScriptRoots();
   int loaded = 0;
   for (size_t i = 0; i < roots.size(); ++i)
   {
      const std::vector<std::string> files = NutFiles(roots[i]);
      for (size_t j = 0; j < files.size(); ++j)
      {
         std::string source;
         if (!ReadFile(files[j].c_str(), source))
         {
            mprintf("Squirrel could not read %s\n", files[j].c_str());
            return false;
         }
         if (!RunSource(mVm, source.data(), source.size(), files[j].c_str()))
            return false;
         if (SquirrelTraceEnabled())
            mprintf("Squirrel loaded source %s\n", files[j].c_str());
         ++loaded;
      }
   }
   mprintf("Squirrel 3.2 loaded %d script source file%s\n", loaded,
           loaded == 1 ? "" : "s");
   return true;
}

bool cNativeSquirrelModule::InventoryClasses()
{
   if (SquirrelTraceEnabled()) mprintf("Squirrel inventory: locating root class\n");
   HSQOBJECT rootClass;
   if (!GetRootObject(mVm, "SqRootScript", &rootClass))
      return false;
   if (SquirrelTraceEnabled()) mprintf("Squirrel inventory: scanning root table\n");
   const SQInteger top = sq_gettop(mVm);
   sq_pushroottable(mVm);
   const SQInteger rootIndex = sq_gettop(mVm);
   sq_pushnull(mVm);
   while (SQ_SUCCEEDED(sq_next(mVm, rootIndex)))
   {
      if (sq_gettype(mVm, -2) == OT_STRING && sq_gettype(mVm, -1) == OT_CLASS)
      {
         const SQChar* name = NULL;
         HSQOBJECT candidate;
         sq_getstring(mVm, -2, &name);
         sq_getstackobj(mVm, -1, &candidate);
         if (name && _stricmp(name, "SqRootScript") != 0 &&
             DerivesFrom(mVm, candidate, rootClass))
         {
            std::unique_ptr<cSquirrelClass> record(new cSquirrelClass);
            record->name = name;
            record->base = "Object";
            mClasses.push_back(std::move(record));
            if (SquirrelTraceEnabled() && (mClasses.size() % 50) == 0)
               mprintf("Squirrel inventory: found %u classes\n",
                       (unsigned)mClasses.size());
         }
      }
      sq_pop(mVm, 2);
   }
   sq_pop(mVm, 1);
   if (SquirrelTraceEnabled())
      mprintf("Squirrel inventory: sorting %u classes\n",
              (unsigned)mClasses.size());
   std::sort(mClasses.begin(), mClasses.end(),
      [](const std::unique_ptr<cSquirrelClass>& a,
         const std::unique_ptr<cSquirrelClass>& b)
      {
         return _stricmp(a->name.c_str(), b->name.c_str()) < 0;
      });
   if (SquirrelTraceEnabled()) mprintf("Squirrel inventory: publishing descriptors\n");
   for (size_t i = 0; i < mClasses.size(); ++i)
   {
      cSquirrelClass& record = *mClasses[i];
      record.descriptor.pszModule = mName.c_str();
      record.descriptor.pszClass = record.name.c_str();
      record.descriptor.pszBaseClass = record.base.c_str();
      record.descriptor.pfnFactory = CreateSquirrelScript;
   }
   sq_settop(mVm, top);
   mprintf("Squirrel 3.2 registered %u script classes\n",
           (unsigned)mClasses.size());
   return true;
}

bool cNativeSquirrelModule::Instantiate(const char* className, ObjID object,
                                        HSQOBJECT* instance)
{
   const SQInteger top = sq_gettop(mVm);
   sq_pushroottable(mVm);
   sq_pushstring(mVm, className, -1);
   if (SQ_FAILED(sq_get(mVm, -2)) || sq_gettype(mVm, -1) != OT_CLASS ||
       SQ_FAILED(sq_createinstance(mVm, -1)) ||
       SQ_FAILED(sq_getstackobj(mVm, -1, instance)))
   {
      mprintf("Squirrel could not instantiate %s: %s\n", className,
              LastError(mVm).c_str());
      sq_settop(mVm, top);
      return false;
   }
   sq_pushstring(mVm, "self", -1);
   sq_pushinteger(mVm, object);
   sq_set(mVm, -3);
   sq_addref(mVm, instance);
   sq_settop(mVm, top);
   return true;
}

IScript* cNativeSquirrelModule::Create(const char* className, ObjID object)
{
   const char* canonicalName = className;
   for (size_t i = 0; i < mClasses.size(); ++i)
      if (_stricmp(mClasses[i]->name.c_str(), className ? className : "") == 0)
      {
         canonicalName = mClasses[i]->name.c_str();
         break;
      }
   cNativeSquirrelScript* script =
      new cNativeSquirrelScript(this, canonicalName, object);
   if (!script->IsValid())
   {
      script->Release();
      return NULL;
   }
   return script;
}

HRESULT cNativeSquirrelModule::Dispatch(HSQOBJECT instance,
                                        const char* className, ObjID object,
                                        sScrMsg* message, sMultiParm* reply,
                                        eScrTraceAction action)
{
   if (SquirrelTraceEnabled())
      mprintf("Squirrel dispatch %s.%s object=%d\n", className,
              message && message->message ? message->message : "(none)", object);
   const ObjID oldObject = mCurrentObject;
   sScrMsg* oldMessage = mCurrentMessage;
   sMultiParm* oldReply = mCurrentReply;
   const std::string oldClass = mCurrentClass;
   mCurrentObject = object;
   mCurrentMessage = message;
   mCurrentReply = reply;
   mCurrentClass = className ? className : "";

   std::string handler = "On";
   handler += message && message->message ? message->message : "Message";
   for (size_t i = 2; i < handler.size(); ++i)
      if (!isalnum((unsigned char)handler[i]) && handler[i] != '_')
         handler[i] = '_';

   const SQInteger top = sq_gettop(mVm);
   sq_pushobject(mVm, instance);
   sq_pushstring(mVm, handler.c_str(), (SQInteger)handler.size());
   if (SQ_FAILED(sq_get(mVm, -2)))
   {
      sq_settop(mVm, top);
      sq_pushobject(mVm, instance);
      sq_pushstring(mVm, "OnMessage", -1);
      if (SQ_FAILED(sq_get(mVm, -2)))
      {
         sq_settop(mVm, top);
         mCurrentObject = oldObject; mCurrentMessage = oldMessage;
         mCurrentReply = oldReply; mCurrentClass = oldClass;
         return S_OK;
      }
   }
   sq_pushobject(mVm, instance);
   const SQRESULT callResult = sq_call(mVm, 1, SQFalse, SQTrue);
   if (SQ_FAILED(callResult))
      mprintf("Squirrel runtime error in %s.%s on object %d: %s\n",
              className, handler.c_str(), object, LastError(mVm).c_str());
   sq_settop(mVm, top);
   mCurrentObject = oldObject; mCurrentMessage = oldMessage;
   mCurrentReply = oldReply; mCurrentClass = oldClass;
   return S_OK;
}

STDMETHODIMP cNativeSquirrelModule::QueryInterface(REFIID id, void** value)
{
   if (!value) return E_POINTER;
   *value = NULL;
   if (SameGuid(id, IID_IUnknown) || SameGuid(id, IID_IScriptModule))
   {
      *value = static_cast<IScriptModule*>(this); AddRef(); return S_OK;
   }
   return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) cNativeSquirrelModule::AddRef() { return ++mReferences; }
STDMETHODIMP_(ULONG) cNativeSquirrelModule::Release()
{
   const ULONG result = --mReferences;
   if (!result) delete this;
   return result;
}
STDMETHODIMP_(const char*) cNativeSquirrelModule::GetName() { return mName.c_str(); }
STDMETHODIMP_(const sScrClassDesc*) cNativeSquirrelModule::GetFirstClass(tScrIter* iterator)
{
   if (!iterator || mClasses.empty()) return NULL;
   *iterator = (tScrIter)0;
   return &mClasses[0]->descriptor;
}
STDMETHODIMP_(const sScrClassDesc*) cNativeSquirrelModule::GetNextClass(tScrIter* iterator)
{
   if (!iterator) return NULL;
   const size_t index = (size_t)*iterator + 1;
   *iterator = (tScrIter)index;
   return index < mClasses.size() ? &mClasses[index]->descriptor : NULL;
}
STDMETHODIMP_(void) cNativeSquirrelModule::EndClassIter(tScrIter*) {}

cNativeSquirrelScript::cNativeSquirrelScript(cNativeSquirrelModule* module,
                                             const char* className,
                                             ObjID object)
   : mReferences(1), mModule(module), mClassName(className ? className : ""),
     mObject(object), mValid(false)
{
   sq_resetobject(&mInstance);
   if (mModule)
   {
      mModule->AddRef();
      mValid = mModule->Instantiate(mClassName.c_str(), mObject, &mInstance);
   }
}

cNativeSquirrelScript::~cNativeSquirrelScript()
{
   if (mValid && mModule && gSquirrelModule == mModule)
      sq_release(mModule->mVm, &mInstance);
   if (mModule) mModule->Release();
}

STDMETHODIMP cNativeSquirrelScript::QueryInterface(REFIID id, void** value)
{
   if (!value) return E_POINTER;
   *value = NULL;
   if (SameGuid(id, IID_IUnknown) || SameGuid(id, IID_IScript))
   {
      *value = static_cast<IScript*>(this); AddRef(); return S_OK;
   }
   return E_NOINTERFACE;
}
STDMETHODIMP_(ULONG) cNativeSquirrelScript::AddRef() { return ++mReferences; }
STDMETHODIMP_(ULONG) cNativeSquirrelScript::Release()
{
   const ULONG result = --mReferences;
   if (!result) delete this;
   return result;
}
STDMETHODIMP_(const char*) cNativeSquirrelScript::GetClassName() { return mClassName.c_str(); }
STDMETHODIMP cNativeSquirrelScript::ReceiveMessage(sScrMsg* message,
                                                   sMultiParm* reply,
                                                   eScrTraceAction action)
{
   return mModule->Dispatch(mInstance, mClassName.c_str(), mObject,
                            message, reply, action);
}

} // namespace

#pragma pack(pop)

BOOL CreateSquirrelScriptModule(const char* moduleName,
                                IScriptMan* scriptManager,
                                tScriptPrintFunc print,
                                IScriptModule** module)
{
   if (!module || !scriptManager || gSquirrelModule)
      return FALSE;
   *module = NULL;
   cNativeSquirrelModule* provider =
      new cNativeSquirrelModule(moduleName, scriptManager, print);
   gSquirrelModule = provider;
   if (!provider->Initialize())
   {
      gSquirrelModule = NULL;
      provider->Release();
      return FALSE;
   }
   *module = provider;
   return TRUE;
}

void SquirrelRenderOverlays(void)
{
   if (gSquirrelModule) gSquirrelModule->RenderOverlays();
}

#else

BOOL CreateSquirrelScriptModule(const char*, IScriptMan*, tScriptPrintFunc,
                                IScriptModule** module)
{
   if (module)
      *module = NULL;
   return FALSE;
}

void SquirrelRenderOverlays(void)
{
}

#endif
