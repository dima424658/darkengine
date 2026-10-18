#include <platform_services.h>

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dlfcn.h>
#include <filesystem>
#include <fnmatch.h>
#include <functional>
#include <fcntl.h>
#include <mutex>
#include <new>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <unistd.h>
#if defined(__linux__) && !defined(__ANDROID__)
#include <execinfo.h>
#endif

#if defined(__ANDROID__)
#include <android/log.h>
#include <SDL2/SDL_system.h>
#include <jni.h>
#endif

#if defined(__linux__)
#include <SDL2/SDL.h>
#include <malloc.h>
#include <sys/sysinfo.h>
#endif

int PlatformGetDisplayModes(ePlatformDisplayScope scope,
                            sPlatformDisplayMode *modes, int capacity,
                            int *native_width, int *native_height)
{
   if (native_width) *native_width = 0;
   if (native_height) *native_height = 0;
#if defined(__linux__)
   if (!modes || capacity < 0)
      capacity = 0;

   const bool initialized = (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) != 0;
   if (!initialized)
   {
      // Pick the compositor-native provider before the first SDL video call.
      // This startup query runs before the renderer creates its window.
      if (std::getenv("WAYLAND_DISPLAY") && !std::getenv("SDL_VIDEODRIVER"))
         SDL_setenv("SDL_VIDEODRIVER", "wayland", 0);
      if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
         return 0;
   }

   int firstDisplay = 0;
   int displayCount = SDL_GetNumVideoDisplays();
   if (scope == kPlatformCurrentDisplay && displayCount > 0)
   {
      SDL_Window *window = SDL_GetKeyboardFocus();
      int display = window ? SDL_GetWindowDisplayIndex(window) : 0;
      if (display < 0 || display >= displayCount)
         display = 0;
      firstDisplay = display;
      displayCount = 1;
   }

   int count = 0;
   for (int displayOffset = 0; displayOffset < displayCount; ++displayOffset)
   {
      const int display = firstDisplay + displayOffset;
      SDL_DisplayMode current{};
      if (SDL_GetCurrentDisplayMode(display, &current) == 0 &&
          scope == kPlatformCurrentDisplay)
      {
         if (native_width) *native_width = current.w;
         if (native_height) *native_height = current.h;
      }

      const int modeCount = SDL_GetNumDisplayModes(display);
      for (int modeIndex = 0; modeIndex < modeCount && count < capacity;
           ++modeIndex)
      {
         SDL_DisplayMode mode{};
         if (SDL_GetDisplayMode(display, modeIndex, &mode) != 0)
            continue;
         const int depth = SDL_BITSPERPIXEL(mode.format);
         int duplicate = 0;
         for (; duplicate < count; ++duplicate)
            if (modes[duplicate].width == mode.w &&
                modes[duplicate].height == mode.h &&
                modes[duplicate].bit_depth == depth)
               break;
         if (duplicate != count)
            continue;
         modes[count++] = {mode.w, mode.h, depth};
      }
   }

   if (!initialized)
      SDL_QuitSubSystem(SDL_INIT_VIDEO);
   return count;
#else
   (void)scope;
   (void)modes;
   (void)capacity;
   return 0;
#endif
}

int PlatformGetDefaultTextPixelHeight(void)
{
#if defined(__ANDROID__)
   // Android's normal body text is 16sp. scaledDensity includes both the
   // device density and the user's system font-size preference.
   JNIEnv *environment = static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
   jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
   float scaledDensity = 0.0f;
   jclass activityClass = NULL;
   jobject resources = NULL;
   jclass resourcesClass = NULL;
   jobject metrics = NULL;
   jclass metricsClass = NULL;

   if (environment && activity)
   {
      activityClass = environment->GetObjectClass(activity);
      jmethodID getResources = activityClass
         ? environment->GetMethodID(activityClass, "getResources",
                                    "()Landroid/content/res/Resources;")
         : NULL;
      if (getResources)
         resources = environment->CallObjectMethod(activity, getResources);
      resourcesClass = resources ? environment->GetObjectClass(resources)
                                 : NULL;
      jmethodID getDisplayMetrics = resourcesClass
         ? environment->GetMethodID(resourcesClass, "getDisplayMetrics",
                                    "()Landroid/util/DisplayMetrics;")
         : NULL;
      if (getDisplayMetrics)
         metrics = environment->CallObjectMethod(resources,
                                                  getDisplayMetrics);
      metricsClass = metrics ? environment->GetObjectClass(metrics) : NULL;
      jfieldID scaledDensityField = metricsClass
         ? environment->GetFieldID(metricsClass, "scaledDensity", "F")
         : NULL;
      if (scaledDensityField)
         scaledDensity = environment->GetFloatField(metrics,
                                                     scaledDensityField);
      if (environment->ExceptionCheck())
      {
         environment->ExceptionClear();
         scaledDensity = 0.0f;
      }
   }
   if (metricsClass) environment->DeleteLocalRef(metricsClass);
   if (metrics) environment->DeleteLocalRef(metrics);
   if (resourcesClass) environment->DeleteLocalRef(resourcesClass);
   if (resources) environment->DeleteLocalRef(resources);
   if (activityClass) environment->DeleteLocalRef(activityClass);
   if (activity) environment->DeleteLocalRef(activity);
   if (scaledDensity > 0.0f)
      return (int)(16.0f * scaledDensity + 0.5f);

   // Retain an SDL density fallback for unusual activity startup states where
   // Android resources are not yet available.
   float density = 0.0f;
   int display = 0;
   SDL_Window *window = SDL_GetKeyboardFocus();
   if (window)
   {
      const int current = SDL_GetWindowDisplayIndex(window);
      if (current >= 0)
         display = current;
   }
   if (SDL_GetDisplayDPI(display, NULL, NULL, &density) == 0 && density > 0.0f)
      return (int)(16.0f * density / 160.0f + 0.5f);

   // Android's baseline density is 160 dpi. A conservative 32-pixel fallback
   // is preferable to silently reverting to Dark's 8-pixel desktop font.
   return 32;
#else
   return 0;
#endif
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
   const char *resolvedTitle = title ? title : "Dark Engine";
   const char *resolvedMessage = message ? message : "";
#if defined(__ANDROID__)
   __android_log_print(ANDROID_LOG_ERROR, "DarkEngineRestored", "%s: %s",
                       resolvedTitle, resolvedMessage);
#endif
   std::fprintf(stderr, "%s: %s\n", resolvedTitle, resolvedMessage);
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
#if defined(__linux__) && !defined(__ANDROID__)
   malloc_trim(0);
#endif
}

void *PlatformLoadLibrary(const char *name) { return dlopen(name, RTLD_NOW | RTLD_LOCAL); }
void *PlatformFindSymbol(void *library, const char *name) { return dlsym(library, name); }
void PlatformUnloadLibrary(void *library) { if (library) dlclose(library); }
void PlatformDebugOutput(const char *text)
{
   const char *resolvedText = text ? text : "";
#if defined(__ANDROID__)
   __android_log_write(ANDROID_LOG_DEBUG, "DarkEngineRestored", resolvedText);
#endif
   std::fputs(resolvedText, stderr);
}

static void PlatformDiagnosticTimestamp(char *buffer, size_t size,
                                        bool milliseconds)
{
   using namespace std::chrono;
   const system_clock::time_point now = system_clock::now();
   const std::time_t value = system_clock::to_time_t(now);
   struct tm local = {};
#if defined(_WIN32)
   localtime_s(&local, &value);
#else
   localtime_r(&value, &local);
#endif
   if (milliseconds)
   {
      const long fraction = (long)(duration_cast<std::chrono::milliseconds>(
         now.time_since_epoch()).count() % 1000);
      char seconds[32];
      std::strftime(seconds, sizeof(seconds), "%H:%M:%S", &local);
      std::snprintf(buffer, size, "%s.%03ld", seconds, fraction);
   }
   else
      std::strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &local);
}

static const char *PlatformDiagnosticArchitecture(void)
{
   return sizeof(void *) == 8 ? "x64" : "x86";
}

static unsigned long long PlatformDiagnosticProcessId(void)
{
   return (unsigned long long)getpid();
}

#include <platform_diagnostics.inl>

void PlatformDiagnosticsLogStack(const char *severity, const char *subsystem,
                                 int skip_frames)
{
#if defined(__linux__) && !defined(__ANDROID__)
   void *frames[24] = {};
   const int count = backtrace(frames,(int)(sizeof(frames) / sizeof(frames[0])));
   char message[1024];
   int used = snprintf(message,sizeof(message),"stack");
   const int first = std::max(0,skip_frames + 1);
   for (int i = first; i < count && used < (int)sizeof(message) - 24; ++i)
   {
      const int written = snprintf(message + used,sizeof(message) - used,
                                   " %p",frames[i]);
      if (written <= 0)
         break;
      used += written;
   }
   message[sizeof(message) - 1] = '\0';
   PlatformDiagnosticsLog(severity,subsystem,message);
#else
   (void)skip_frames;
   PlatformDiagnosticsLog(severity,subsystem,"stack unavailable");
#endif
}

ePlatformAssertAction PlatformShowAssert(const char *message)
{
   PlatformShowError("Assertion Failed", message);
   return kPlatformAssertExit;
}

void PlatformDebugBreak(void) { __builtin_trap(); }
void PlatformExitProcess(int status) { std::exit(status); }
BOOL PlatformSetClipboardText(const char *) { return FALSE; }
int PlatformGetClipboardText(char *, int) { return 0; }

void PlatformSetTextInputActive(BOOL active)
{
#if defined(__linux__)
   if (!(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO))
      return;
   if (active)
      SDL_StartTextInput();
   else
      SDL_StopTextInput();
#else
   (void)active;
#endif
}

BOOL PlatformGetCurrentDirectory(char *path, int capacity)
{
   return path && capacity > 0 && getcwd(path, (size_t)capacity) != nullptr;
}

BOOL PlatformGetExecutableDirectory(char *path, int capacity)
{
   if (!path || capacity <= 0)
      return FALSE;
   const ssize_t length = readlink("/proc/self/exe",path,(size_t)capacity - 1);
   if (length <= 0 || length >= capacity)
   {
      path[0] = '\0';
      return FALSE;
   }
   path[length] = '\0';
   char *separator = strrchr(path,'/');
   if (!separator)
   {
      path[0] = '\0';
      return FALSE;
   }
   *separator = '\0';
   return TRUE;
}

BOOL PlatformRunProcess(const char *, const char *const *, int, const char *,
                        tPlatformProcessOutput, void *, uint32 *exit_code)
{
   if (exit_code)
      *exit_code = UINT32_MAX;
   return FALSE;
}

BOOL PlatformRelaunchSelf(const char *const *arguments, int argument_count,
                          const char *working_directory)
{
#if defined(__ANDROID__)
   (void)arguments;
   (void)argument_count;
   (void)working_directory;
   return FALSE;
#else
   if (argument_count < 0)
      return FALSE;
   char executable[4096];
   const ssize_t length = readlink("/proc/self/exe",executable,
                                   sizeof(executable) - 1);
   if (length <= 0 || length >= (ssize_t)sizeof(executable))
      return FALSE;
   executable[length] = '\0';

   const pid_t child = fork();
   if (child < 0)
      return FALSE;
   if (child == 0)
   {
      if (working_directory && working_directory[0] &&
          chdir(working_directory) != 0)
         _exit(126);
      std::vector<char *> argv;
      argv.reserve((size_t)argument_count + 2);
      argv.push_back(executable);
      for (int i = 0; i < argument_count; ++i)
         argv.push_back(const_cast<char *>(arguments && arguments[i]
                                          ? arguments[i] : ""));
      argv.push_back(nullptr);
      execv(executable,argv.data());
      _exit(127);
   }
   return TRUE;
#endif
}

namespace
{
std::mutex g_resolvedPathMutex;

struct sDirectoryCaseIndex
{
   std::unordered_map<std::string, std::vector<std::string>> names;
};

struct sResolvedPath
{
   std::string native;
   bool exists;
};

std::unordered_map<std::string, sDirectoryCaseIndex> g_directoryCaseCache;
std::unordered_map<std::string, sResolvedPath> g_resolvedPathCache;

std::string FoldPathComponent(std::string text)
{
   std::transform(text.begin(), text.end(), text.begin(),
      [](unsigned char character)
      {
         return (char)(character >= 'A' && character <= 'Z'
            ? character + ('a' - 'A') : character);
      });
   return text;
}

bool HasWildcard(const std::string &pattern)
{
   return pattern.find_first_of("*?[") != std::string::npos;
}

std::string PathCacheKey(const std::filesystem::path &path)
{
   // Relative keys are valid until PlatformChangeDirectory(), which clears
   // both caches. Avoid std::filesystem::absolute() here: it issues getcwd()
   // for every failed resource candidate and is particularly costly on WSL.
   return path.lexically_normal().generic_string();
}

bool FindPathComponent(const std::filesystem::path &directory,
                       const std::string &wanted,
                       std::filesystem::path *match)
{
   const std::string key = PathCacheKey(directory);
   const std::string folded = FoldPathComponent(wanted);

   {
      std::lock_guard<std::mutex> lock(g_resolvedPathMutex);
      const auto cached = g_directoryCaseCache.find(key);
      if (cached != g_directoryCaseCache.end())
      {
         const auto names = cached->second.names.find(folded);
         if (names != cached->second.names.end())
         {
            for (const std::string &name : names->second)
            {
               if (name == wanted)
               {
                  *match = name;
                  return true;
               }
            }
            if (names->second.size() == 1)
            {
               *match = names->second[0];
               return true;
            }
         }
         return false;
      }
   }

   sDirectoryCaseIndex index;
   std::error_code error;
   for (std::filesystem::directory_iterator it(directory, error), end;
        !error && it != end; it.increment(error))
   {
      const std::string name = it->path().filename().string();
      index.names[FoldPathComponent(name)].push_back(name);
   }
   if (error)
      return false;

   const auto names = index.names.find(folded);
   bool found = false;
   if (names != index.names.end())
   {
      for (const std::string &name : names->second)
      {
         if (name == wanted)
         {
            *match = name;
            found = true;
            break;
         }
      }
      if (!found && names->second.size() == 1)
      {
         *match = names->second[0];
         found = true;
      }
   }
   {
      std::lock_guard<std::mutex> lock(g_resolvedPathMutex);
      g_directoryCaseCache[key] = std::move(index);
   }
   return found;
}

std::string ResolveNativePath(const char *path, bool *exists)
{
   std::string text = path ? path : "";
   std::replace(text.begin(), text.end(), '\\', '/');
   if (exists)
      *exists = false;
   if (text.empty())
      return text;

   const std::filesystem::path input(text);

   std::filesystem::path resolved = input.is_absolute()
      ? input.root_path() : std::filesystem::path(".");
   bool allComponentsExist = true;
   for (const auto &part : input.relative_path())
   {
      //Trailing separator produces an empty part
      if (part.empty() || part == ".")
         continue;
      if (part == "..")
      {
         resolved /= part;
         continue;
      }

      std::filesystem::path match;
      const std::filesystem::path exact = resolved / part;
      std::error_code exactError;
      // Each directory index preserves all original spellings. An exact
      // component wins; only a unique folded match is accepted as fallback.
      // This avoids a stat for every extension candidate during resource
      // search while retaining exact-name-first behavior.
      // Two differently cased names can coexist on a POSIX filesystem. Do
      // not choose one arbitrarily: callers will receive their stated path
      // and the normal operation will fail visibly instead.
      // Check the exact spelling without enumerating the parent first. Apart
      // from being cheaper, this is required for Android absolute paths: an
      // app may traverse /storage to its own directory but may not list /.
      if (std::filesystem::exists(exact, exactError))
         resolved = exact;
      else if (FindPathComponent(resolved, part.string(), &match))
         resolved /= match;
      else
      {
         resolved /= part;
         allComponentsExist = false;
      }
   }

   std::string native = resolved.lexically_normal().generic_string();
   if (!input.is_absolute() && native.rfind("./", 0) == 0)
      native.erase(0, 2);

   const bool found = allComponentsExist;
   if (exists)
      *exists = found;
   return native;
}

std::string ResolveCachedPath(const char *path, bool *exists)
{
   std::string text = path ? path : "";
   std::replace(text.begin(), text.end(), '\\', '/');
   const std::string key = PathCacheKey(std::filesystem::path(text));
   {
      std::lock_guard<std::mutex> lock(g_resolvedPathMutex);
      const auto cached = g_resolvedPathCache.find(key);
      if (cached != g_resolvedPathCache.end())
      {
         if (exists)
            *exists = cached->second.exists;
         return cached->second.native;
      }
   }

   bool found = false;
   std::string native = ResolveNativePath(text.c_str(), &found);
   {
      std::lock_guard<std::mutex> lock(g_resolvedPathMutex);
      g_resolvedPathCache[key] = {native, found};
   }
   if (exists)
      *exists = found;
   return native;
}

void ClearPathCaches()
{
   std::lock_guard<std::mutex> lock(g_resolvedPathMutex);
   g_resolvedPathCache.clear();
   g_directoryCaseCache.clear();
}

bool CopyResolvedPath(const std::string &path, char *resolved, int capacity)
{
   if (!resolved || capacity <= 0)
      return false;
   if (path.size() >= (size_t)capacity)
   {
      resolved[0] = '\0';
      return false;
   }
   std::memcpy(resolved, path.c_str(), path.size() + 1);
   return true;
}
}

BOOL PlatformResolvePath(const char *path, char *resolved, int capacity)
{
   bool exists = false;
   const std::string native = ResolveCachedPath(path, &exists);
   return CopyResolvedPath(native, resolved, capacity) && exists;
}

EXTERN_C FILE *PlatformFopen(const char *path, const char *mode)
{
   const std::string native = ResolveNativePath(path, nullptr);
   FILE *file = std::fopen(native.c_str(), mode);
   if (file && mode && std::strpbrk(mode, "wax"))
      ClearPathCaches();
   return file;
}

EXTERN_C int PlatformOpen(const char *path, int flags, ...)
{
   const std::string native = ResolveNativePath(path, nullptr);
   if ((flags & O_CREAT) == 0)
      return open(native.c_str(), flags);
   va_list arguments;
   va_start(arguments, flags);
   const mode_t mode = (mode_t)va_arg(arguments, int);
   va_end(arguments);
   const int descriptor = open(native.c_str(), flags, mode);
   if (descriptor >= 0)
      ClearPathCaches();
   return descriptor;
}

EXTERN_C int PlatformAccess(const char *path, int mode)
{
   const std::string native = ResolveNativePath(path, nullptr);
   return access(native.c_str(), mode);
}

EXTERN_C int PlatformChangeDirectory(const char *path)
{
   const std::string native = ResolveNativePath(path, nullptr);
   const int result = chdir(native.c_str());
   if (result == 0)
      ClearPathCaches();
   return result;
}

EXTERN_C int PlatformRemoveDirectory(const char *path)
{
   const std::string native = ResolveNativePath(path, nullptr);
   const int result = rmdir(native.c_str());
   if (result == 0)
      ClearPathCaches();
   return result;
}

EXTERN_C int PlatformUnlink(const char *path)
{
   const std::string native = ResolveNativePath(path, nullptr);
   const int result = unlink(native.c_str());
   if (result == 0)
      ClearPathCaches();
   return result;
}

BOOL PlatformCopyFiles(const char *pattern, const char *source_dir,
                       const char *destination_dir)
{
   if (!pattern || !source_dir || !destination_dir)
      return FALSE;
   const std::filesystem::path sourceDirectory = ResolveNativePath(source_dir, nullptr);
   const std::filesystem::path destinationDirectory = ResolveNativePath(destination_dir, nullptr);
   const std::string foldedPattern = FoldPathComponent(pattern);
   if (!HasWildcard(pattern))
   {
      bool exists = false;
      const std::filesystem::path source = ResolveNativePath(
         (sourceDirectory / pattern).string().c_str(), &exists);
      std::error_code copyError;
      if (!exists || !std::filesystem::is_regular_file(source, copyError))
         return FALSE;
      std::filesystem::copy_file(
         source, destinationDirectory / source.filename(),
         std::filesystem::copy_options::overwrite_existing, copyError);
      if (!copyError)
         ClearPathCaches();
      return !copyError;
   }
   std::error_code error;
   std::filesystem::directory_iterator it(sourceDirectory, error), end;
   if (error)
      return FALSE;
   bool matched = false;
   bool result = true;
   for (; !error && it != end; it.increment(error))
   {
      const std::filesystem::path source = it->path();
      if (fnmatch(foldedPattern.c_str(),
                  FoldPathComponent(source.filename().string()).c_str(), 0) != 0 ||
          !std::filesystem::is_regular_file(source, error))
         continue;
      matched = true;
      error.clear();
      std::filesystem::copy_file(
         source, destinationDirectory / source.filename(),
         std::filesystem::copy_options::overwrite_existing, error);
      if (error)
      {
         result = false;
         error.clear();
      }
   }
   const bool copied = matched && result && !error;
   if (copied)
      ClearPathCaches();
   return copied;
}

struct sUnixFileFind
{
   std::vector<std::filesystem::path> matches;
   size_t index;
};

static BOOL CopyFileFind(sUnixFileFind *native, sPlatformFileFind *find)
{
   if (native->index >= native->matches.size())
      return FALSE;
   const std::filesystem::path &path = native->matches[native->index];
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
   std::string text(pattern);
   std::replace(text.begin(), text.end(), '\\', '/');
   const std::filesystem::path input(text);
   if (!HasWildcard(input.filename().string()))
   {
      bool exists = false;
      const std::filesystem::path resolved = ResolveNativePath(text.c_str(), &exists);
      if (!exists)
         return FALSE;
      sUnixFileFind *native = new (std::nothrow) sUnixFileFind{};
      if (!native)
         return FALSE;
      native->matches.push_back(resolved);
      find->implementation = native;
      return CopyFileFind(native, find);
   }
   const std::string parent = input.parent_path().empty()
      ? "." : input.parent_path().string();
   const std::filesystem::path directory = ResolveNativePath(parent.c_str(), nullptr);
   const std::string foldedPattern = FoldPathComponent(input.filename().string());
   sUnixFileFind *native = new (std::nothrow) sUnixFileFind{};
   if (!native)
      return FALSE;
   std::error_code error;
   for (std::filesystem::directory_iterator it(directory, error), end;
        !error && it != end; it.increment(error))
   {
      if (fnmatch(foldedPattern.c_str(),
                  FoldPathComponent(it->path().filename().string()).c_str(), 0) == 0)
         native->matches.push_back(it->path());
   }
   if (error || native->matches.empty())
   {
      delete native;
      return FALSE;
   }
   std::sort(native->matches.begin(), native->matches.end());
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
