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

const char *const kCrtImports[] = {
   "__dllonexit", "_adjust_fdiv", "_CIcos", "_CIsin", "_CIsqrt",
   "_crt_debugger_hook", "_decode_pointer", "_encoded_null",
   "_encode_pointer", "_initterm", "_initterm_e", "_itoa", "_lock",
   "_malloc_crt", "_onexit", "_stricmp", "_unlock", "free", "isspace",
   "malloc", "memcpy", "memmove", "memset", "realloc", "strncpy",
   "toupper"
};

const char *const kKernelImports[] = {
   "DeleteCriticalSection", "EnterCriticalSection",
   "FreeEnvironmentStringsA", "FreeEnvironmentStringsW", "GetACP",
   "GetCommandLineA", "GetCPInfo", "GetCurrentThread",
   "GetCurrentThreadId", "GetEnvironmentStrings", "GetEnvironmentStringsA",
   "GetEnvironmentStringsW", "GetFileAttributesA", "GetFileType",
   "GetLastError", "GetModuleFileNameA", "GetModuleHandleA", "GetOEMCP",
   "GetPrivateProfileIntA", "GetPrivateProfileStringA", "GetProcessHeap",
   "GetStartupInfoA", "GetStdHandle", "GetThreadPriority", "GetTickCount",
   "GetVersion", "HeapAlloc", "HeapCreate", "HeapDestroy", "HeapFree",
   "HeapReAlloc", "HeapSize", "InitializeCriticalSection",
   "InterlockedCompareExchange", "InterlockedExchange", "LeaveCriticalSection",
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
   if (item.module == "MSVCR90.dll")
      return Listed(item.name, kCrtImports,
                    sizeof(kCrtImports) / sizeof(kCrtImports[0]));
   if (item.module == "KERNEL32.dll")
      return Listed(item.name, kKernelImports,
                    sizeof(kKernelImports) / sizeof(kKernelImports[0]));
   return false;
}

} // namespace osm32
