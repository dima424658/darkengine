#include <osm_emu/osm32imports.h>

#pragma pack(push, 8)
#include <string.h>
#pragma pack(pop)

namespace
{

bool Listed(const std::string &name, const char *const *names, size_t count)
{
   for (size_t i = 0; i < count; ++i)
      if (name == names[i]) return true;
   return false;
}

bool EqualAsciiNoCase(const std::string &left, const char *right)
{
   size_t i = 0;
   while (i < left.size() && *right)
   {
      char a = left[i++];
      char b = *right++;
      if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
      if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
      if (a != b) return false;
   }
   return i == left.size() && !*right;
}

bool IsCrtModule(const std::string &module)
{
   return EqualAsciiNoCase(module, "MSVCR90.dll") ||
          EqualAsciiNoCase(module, "msvcrt.dll");
}

const char *const kCrtImports[] = {
   "__dllonexit", "_adjust_fdiv", "_CIcos", "_CIsin", "_CIsqrt",
   "_crt_debugger_hook", "_decode_pointer", "_encoded_null",
   "_encode_pointer", "_initterm", "_initterm_e", "_itoa", "_lock",
   "_malloc_crt", "_onexit", "_stricmp", "_strnicmp", "_unlock",
   "cos", "exp", "fflush", "free", "isspace", "log", "log10",
   "malloc", "memchr", "memcmp", "memcpy", "memmove", "memset",
   "pow", "realloc", "sin", "sqrt", "strcat", "strchr", "strcmp",
   "strcpy", "strcspn", "strlen", "strncpy", "strpbrk", "strrchr",
   "strspn", "strstr", "strtod", "strtol", "toupper"
};

const char *const kKernelImports[] = {
   "AddAtomA", "DeleteCriticalSection", "DisableThreadLibraryCalls",
   "EnterCriticalSection",
   "FreeEnvironmentStringsA", "FreeEnvironmentStringsW", "GetACP",
   "FindAtomA", "GetAtomNameA", "GetCommandLineA", "GetCPInfo",
   "GetCurrentThread",
   "GetCurrentThreadId", "GetEnvironmentStrings", "GetEnvironmentStringsA",
   "GetEnvironmentStringsW", "GetFileAttributesA", "GetFileType",
   "GetLastError", "GetModuleFileNameA", "GetModuleHandleA", "GetOEMCP",
   "GetPrivateProfileIntA", "GetPrivateProfileStringA", "GetProcessHeap",
   "GetStartupInfoA", "GetStdHandle", "GetThreadPriority", "GetTickCount",
   "GetVersion", "GlobalAlloc", "HeapAlloc", "HeapCreate", "HeapDestroy", "HeapFree",
   "HeapReAlloc", "HeapSize", "InitializeCriticalSection",
   "InterlockedCompareExchange", "InterlockedDecrement", "InterlockedExchange",
   "InterlockedIncrement", "LeaveCriticalSection",
   "MultiByteToWideChar", "OutputDebugStringA", "SetHandleCount",
   "SetLastError", "SetStdHandle", "SetThreadPriority", "TlsAlloc",
   "TlsFree", "TlsGetValue", "TlsSetValue", "WideCharToMultiByte"
};

} // namespace

namespace osm32
{

bool IsSupportedImport(const sImport &item)
{
   if (item.byOrdinal) return false;
   if (IsCrtModule(item.module))
      return Listed(item.name, kCrtImports,
                    sizeof(kCrtImports) / sizeof(kCrtImports[0]));
   if (item.module == "KERNEL32.dll")
      return Listed(item.name, kKernelImports,
                    sizeof(kKernelImports) / sizeof(kKernelImports[0]));
   return false;
}

} // namespace osm32
