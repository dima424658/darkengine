#include <platform_services.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <filesystem>
#include <glob.h>
#include <functional>
#include <mutex>
#include <new>
#include <string>
#include <thread>
#include <unistd.h>

#if defined(__linux__)
#include <malloc.h>
#include <sys/sysinfo.h>
#endif

int PlatformGetDisplayModes(ePlatformDisplayScope, sPlatformDisplayMode *, int,
                            int *native_width, int *native_height)
{
   if (native_width) *native_width = 0;
   if (native_height) *native_height = 0;
   return 0;
}

BOOL PlatformGetMemoryStatus(sPlatformMemoryStatus *status)
{
   if (!status)
      return FALSE;
   std::memset(status, 0, sizeof(*status));
#if defined(__linux__)
   struct sysinfo info;
   if (sysinfo(&info) != 0)
      return FALSE;
   const uint64_t unit = info.mem_unit;
   const uint64_t total = info.totalram * unit;
   const uint64_t available = info.freeram * unit;
   status->total_physical_bytes =
      total > UINT32_MAX ? UINT32_MAX : (uint32)total;
   status->available_physical_bytes =
      available > UINT32_MAX ? UINT32_MAX : (uint32)available;
   status->load_percent = total ? (uint32)(100 - available * 100 / total) : 0;
   return TRUE;
#else
   return FALSE;
#endif
}

BOOL PlatformReadUserUInt(const char *, const char *, uint32 *) { return FALSE; }
BOOL PlatformWriteUserUInt(const char *, const char *, uint32) { return FALSE; }

const char *PlatformGetLanguageCode(void)
{
   const char *language = std::getenv("LANG");
   static char code[3] = "en";
   if (language && std::strlen(language) >= 2)
   {
      code[0] = language[0];
      code[1] = language[1];
   }
   return code;
}

void PlatformShowError(const char *title, const char *message)
{
   std::fprintf(stderr, "%s: %s\n", title ? title : "Dark Engine",
                message ? message : "");
}

BOOL PlatformAskYesNo(const char *title, const char *message)
{
   PlatformShowError(title, message);
   return FALSE;
}

int PlatformPopupMenu(const char *const *, int, int, int) { return 0; }

uint32 PlatformMilliseconds(void)
{
   using namespace std::chrono;
   return (uint32)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

uint64_t PlatformCurrentThreadId(void)
{
   return (uint64_t)std::hash<std::thread::id>{}(std::this_thread::get_id());
}

int PlatformReadKey(void)
{
   return std::getchar();
}

size_t PlatformAllocationSize(void *allocation)
{
#if defined(__linux__)
   return allocation ? malloc_usable_size(allocation) : 0;
#else
   return 0;
#endif
}

void PlatformHeapCheck(void)
{
}

void PlatformHeapMinimize(void)
{
#if defined(__linux__)
   malloc_trim(0);
#endif
}

void *PlatformLoadLibrary(const char *name) { return dlopen(name, RTLD_NOW | RTLD_LOCAL); }
void *PlatformFindSymbol(void *library, const char *name) { return dlsym(library, name); }
void PlatformUnloadLibrary(void *library) { if (library) dlclose(library); }
void PlatformDebugOutput(const char *text) { std::fputs(text ? text : "", stderr); }

ePlatformAssertAction PlatformShowAssert(const char *message)
{
   PlatformShowError("Assertion Failed", message);
   return kPlatformAssertExit;
}

void PlatformDebugBreak(void) { __builtin_trap(); }
void PlatformExitProcess(int status) { std::exit(status); }
BOOL PlatformSetClipboardText(const char *) { return FALSE; }
int PlatformGetClipboardText(char *, int) { return 0; }

BOOL PlatformGetCurrentDirectory(char *path, int capacity)
{
   return path && capacity > 0 && getcwd(path, (size_t)capacity) != nullptr;
}

BOOL PlatformCopyFiles(const char *pattern, const char *source_dir,
                       const char *destination_dir)
{
   if (!pattern || !source_dir || !destination_dir)
      return FALSE;
   std::string search = std::string(source_dir) + "/" + pattern;
   glob_t matches{};
   if (glob(search.c_str(), 0, nullptr, &matches) != 0)
      return FALSE;
   bool result = true;
   for (size_t i = 0; i < matches.gl_pathc; ++i)
   {
      std::filesystem::path source(matches.gl_pathv[i]);
      if (!std::filesystem::is_regular_file(source))
         continue;
      std::error_code error;
      std::filesystem::copy_file(
         source, std::filesystem::path(destination_dir) / source.filename(),
         std::filesystem::copy_options::overwrite_existing, error);
      if (error)
         result = false;
   }
   globfree(&matches);
   return result;
}

struct sUnixFileFind
{
   glob_t matches;
   size_t index;
};

static std::string NativeSearchPattern(const char *pattern)
{
   std::string text(pattern ? pattern : "");
   std::replace(text.begin(), text.end(), '\\', '/');
   std::filesystem::path input(text), leaf = input.filename();
   std::filesystem::path parent = input.parent_path();
   std::filesystem::path resolved = parent.is_absolute() ? parent.root_path() : std::filesystem::path(".");
   for (const auto &part : parent.relative_path())
   {
      std::filesystem::path exact = resolved / part;
      std::error_code error;
      if (std::filesystem::exists(exact, error)) { resolved = exact; continue; }
      std::string wanted = part.string();
      std::transform(wanted.begin(), wanted.end(), wanted.begin(), [](unsigned char c){ return (char)std::tolower(c); });
      bool found = false;
      for (std::filesystem::directory_iterator it(resolved, error), end; !error && it != end; it.increment(error))
      {
         std::string candidate = it->path().filename().string();
         std::transform(candidate.begin(), candidate.end(), candidate.begin(), [](unsigned char c){ return (char)std::tolower(c); });
         if (candidate == wanted) { resolved /= it->path().filename(); found = true; break; }
      }
      if (!found) resolved /= part;
   }
   return (resolved / leaf).lexically_normal().generic_string();
}

static BOOL CopyFileFind(sUnixFileFind *native, sPlatformFileFind *find)
{
   if (native->index >= native->matches.gl_pathc)
      return FALSE;
   std::filesystem::path path(native->matches.gl_pathv[native->index]);
   std::string name = path.filename().string();
   std::error_code error;
   find->attributes = 0;
   if (!name.empty() && name[0] == '.')
      find->attributes |= kPlatformFileHidden;
   if (std::filesystem::is_directory(path, error))
      find->attributes |= kPlatformFileDirectory;
   find->size = std::filesystem::is_regular_file(path, error)
      ? std::filesystem::file_size(path, error) : 0;
   std::strncpy(find->name, name.c_str(), sizeof(find->name) - 1);
   find->name[sizeof(find->name) - 1] = '\0';
   return TRUE;
}

BOOL PlatformFindFirst(const char *pattern, sPlatformFileFind *find)
{
   if (!pattern || !find)
      return FALSE;
   std::string nativePattern = NativeSearchPattern(pattern);
   sUnixFileFind *native = new (std::nothrow) sUnixFileFind{};
   if (!native || glob(nativePattern.c_str(), 0, nullptr, &native->matches) != 0)
   {
      delete native;
      return FALSE;
   }
   find->implementation = native;
   return CopyFileFind(native, find);
}

BOOL PlatformFindNext(sPlatformFileFind *find)
{
   sUnixFileFind *native = find
      ? static_cast<sUnixFileFind *>(find->implementation) : nullptr;
   if (!native)
      return FALSE;
   ++native->index;
   return CopyFileFind(native, find);
}

void PlatformFindClose(sPlatformFileFind *find)
{
   sUnixFileFind *native = find
      ? static_cast<sUnixFileFind *>(find->implementation) : nullptr;
   if (native)
   {
      globfree(&native->matches);
      delete native;
      find->implementation = nullptr;
   }
}

BOOL PlatformMutexInit(sPlatformMutex *mutex)
{
   if (!mutex)
      return FALSE;
   mutex->implementation = new (std::nothrow) std::recursive_mutex;
   return mutex->implementation != nullptr;
}

void PlatformMutexTerm(sPlatformMutex *mutex)
{
   if (mutex)
   {
      delete static_cast<std::recursive_mutex *>(mutex->implementation);
      mutex->implementation = nullptr;
   }
}

void PlatformMutexLock(sPlatformMutex *mutex)
{
   static_cast<std::recursive_mutex *>(mutex->implementation)->lock();
}

void PlatformMutexUnlock(sPlatformMutex *mutex)
{
   static_cast<std::recursive_mutex *>(mutex->implementation)->unlock();
}

long PlatformAtomicLoad(volatile long *value) { return __atomic_load_n(value, __ATOMIC_SEQ_CST); }
long PlatformAtomicIncrement(volatile long *value) { return __atomic_add_fetch(value, 1, __ATOMIC_SEQ_CST); }
long PlatformAtomicDecrement(volatile long *value) { return __atomic_sub_fetch(value, 1, __ATOMIC_SEQ_CST); }
long PlatformAtomicExchange(volatile long *value, long replacement)
{
   return __atomic_exchange_n(value, replacement, __ATOMIC_SEQ_CST);
}
long PlatformAtomicAdd(volatile long *value, long amount)
{
   return __atomic_fetch_add(value, amount, __ATOMIC_SEQ_CST);
}
long PlatformAtomicCompareExchange(volatile long *value, long replacement,
                                   long expected)
{
   __atomic_compare_exchange_n(value, &expected, replacement, false,
                               __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
   return expected;
}
